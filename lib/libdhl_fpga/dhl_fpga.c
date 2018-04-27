/*
 *  rte_fpga.c
 *
 *  Create on : Oct 9, 2017
 *  	Author: Xiaoyao Li
 *
 */



#include <stdlib.h>
#include <errno.h>

#include <rte_mbuf.h>
#include <rte_pci.h>
#include <rte_memzone.h>
#include <rte_mempool.h>
#include <rte_dev.h>
#include <rte_malloc.h>
#include <rte_debug.h>

#include "dhl_fpga.h"

static const char *MZ_DHL_FPGA_DEV_DATA = "dhl_fpga_dev_data";
struct dhl_fpga_dev dhl_fpga_devices[DHL_MAX_FPGA_CARDS];
static struct dhl_fpga_dev_data * dhl_fpga_dev_data;
static uint8_t fpga_dev_last_created_card;
static uint8_t nb_fpga_cards;

const struct dhl_fpga_rxconf default_rxconf = {
		.rx_drop_en = 0,
};

const struct dhl_fpga_txconf default_txconf = {
		.txe_flags = 0,
};

uint8_t
dhl_fpga_find_next(uint8_t card_id)
{
	while (card_id < DHL_MAX_FPGA_CARDS &&
			dhl_fpga_devices[card_id].state != DHL_FPGA_DEV_ATTACHED)
		card_id++;

	if(card_id >= DHL_MAX_FPGA_CARDS)
		return DHL_MAX_FPGA_CARDS;

	return card_id;
}

static void
dhl_fpga_dev_data_alloc(void)
{
	const unsigned flags = 0;
	const struct rte_memzone * mz;

	if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
		mz = rte_memzone_reserve(MZ_DHL_FPGA_DEV_DATA, DHL_MAX_FPGA_CARDS * sizeof(*dhl_fpga_dev_data),
				rte_socket_id(), flags);
	} else {
		mz = rte_memzone_lookup(MZ_DHL_FPGA_DEV_DATA);
	}

	if (mz == NULL )
		rte_panic("Cannot allocate memzone for fpga card data\n");

	dhl_fpga_dev_data = mz->addr;
	if (rte_eal_process_type() == RTE_PROC_PRIMARY)
		memset(dhl_fpga_dev_data, 0, DHL_MAX_FPGA_CARDS * sizeof(*dhl_fpga_dev_data));
}

struct dhl_fpga_dev *
dhl_fpga_dev_allocated(const char *name){
	unsigned i;

	for ( i = 0; i < DHL_MAX_FPGA_CARDS; i++) {
		if( (dhl_fpga_devices[i].state == DHL_FPGA_DEV_ATTACHED) &&
				strcmp(dhl_fpga_devices[i].data->name, name) == 0)
			return &dhl_fpga_devices[i];
	}
	return NULL;
}

static uint8_t
dhl_fpga_dev_find_free_card(void)
{
	unsigned i;

	for( i = 0; i < DHL_MAX_FPGA_CARDS; i++) {
		if (dhl_fpga_devices[i].state == DHL_FPGA_DEV_UNUSED)
			return i;
	}
	return DHL_MAX_FPGA_CARDS;
}

static struct dhl_fpga_dev *
fpga_dev_get(uint8_t card_id)
{
	if(card_id >= DHL_MAX_FPGA_CARDS)
		return NULL;

	struct dhl_fpga_dev * fpga_dev = &dhl_fpga_devices[card_id];

	fpga_dev->data = &dhl_fpga_dev_data[card_id];
	fpga_dev->state = DHL_FPGA_DEV_ATTACHED;

	fpga_dev_last_created_card = card_id;
	nb_fpga_cards++;

	return fpga_dev;
}

struct dhl_fpga_dev *
dhl_fpga_dev_allocate(const char * name)
{
	uint8_t card_id;
	struct dhl_fpga_dev * fpga_dev;

	card_id = dhl_fpga_dev_find_free_card();
	if (card_id == DHL_MAX_FPGA_CARDS) {
		RTE_PMD_DEBUG_TRACE("Reached maximum number of FPGA cards\n");
		return NULL;
	}

	if (dhl_fpga_dev_data == NULL)
		dhl_fpga_dev_data_alloc();

	if (dhl_fpga_dev_allocated(name) != NULL) {
		RTE_PMD_DEBUG_TRACE("FPGA card with name %s already allocated!\n",
				name);
		return NULL;
	}

	memset(&dhl_fpga_dev_data[card_id], 0, sizeof(struct dhl_fpga_dev_data));
	fpga_dev = fpga_dev_get(card_id);
	snprintf(fpga_dev->data->name, sizeof(fpga_dev->data->name), "%s", name);
	fpga_dev->data->card_id = card_id;

	return fpga_dev;
}


/*
 * Attach to a port already registered by the primary process, which makes sure
 * that the same device would have the same port id both in the primary and secondary process.
 */
struct dhl_fpga_dev *
dhl_fpga_dev_attach_secondary(const char * name)
{
	uint8_t i;
	struct dhl_fpga_dev * fpga_dev;

	if(dhl_fpga_dev_data == NULL)
		dhl_fpga_dev_data_alloc();

	for ( i = 0; i < DHL_MAX_FPGA_CARDS; i++) {
		if (strcmp(dhl_fpga_dev_data[i].name, name) == 0)
			break;
	}
	if(i ==DHL_MAX_FPGA_CARDS){
		RTE_PMD_DEBUG_TRACE(
					"device %s is not driven by the primary process\n",
					name);
		return NULL;
	}

	fpga_dev = fpga_dev_get(i);
	RTE_ASSERT(fpga_dev->data->port_id == i);

	return fpga_dev;
}

int
dhl_fpga_dev_release_card(struct dhl_fpga_dev * fpga_dev){
	if (fpga_dev == NULL)
		return -EINVAL;

	fpga_dev->state = DHL_FPGA_DEV_UNUSED;
	nb_fpga_cards--;
	return 0;
}

int
dhl_fpga_dev_is_valid_card(uint8_t card_id)
{
	if (card_id >= DHL_MAX_FPGA_CARDS ||
			dhl_fpga_devices[card_id].state != DHL_FPGA_DEV_ATTACHED)
		return 0;
	else
		return 1;
}

const struct rte_memzone *
dhl_fpga_dma_zone_reserve( const struct dhl_fpga_dev * dev, const char * ring_name,
		uint16_t engine_id, size_t size, unsigned align, int socket_id)
{
	char z_name[RTE_MEMZONE_NAMESIZE];
	const struct rte_memzone *mz;

	snprintf(z_name, sizeof(z_name), "%s_%s_%d_%d",
			dev->data->drv_name, ring_name,
			dev->data->card_id, engine_id);

	mz = rte_memzone_lookup(z_name);
	if(mz)
		return mz;


//	return rte_memzone_reserve_bounded(z_name, size, socket_id,
//					0, align, RTE_PGSIZE_4G);
	return rte_memzone_reserve_aligned(z_name, size, socket_id,
					   0, align);
}


unsigned int
dhl_fpga_dev_socket_id(uint8_t card_id)
{
	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -1);
	return dhl_fpga_devices[card_id].data->numa_node;
}


uint8_t
dhl_fpga_dev_count(void){
	return nb_fpga_cards;
}




static int
dhl_fpga_dev_rx_engine_config(struct dhl_fpga_dev * dev, uint16_t nb_engines)
{
	uint16_t old_nb_engines = dev->data->nb_rx_engines;
	void ** rxe;
	unsigned i;

	if(dev->data->rx_engines == NULL && nb_engines != 0) { /* first time configuration */
		dev->data->rx_engines = rte_zmalloc("fpgadev->rx_engines",
				sizeof(dev->data->rx_engines[0]) * nb_engines,
				RTE_CACHE_LINE_SIZE);
		if (dev->data->rx_engines == NULL) {
			dev->data->nb_rx_engines = 0;
			return -(ENOMEM);
		}
	} else if (dev->data->rx_engines != NULL && nb_engines != 0) { /* re-configure */
		DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_release, -ENOTSUP);

		rxe = dev->data->rx_engines;

		for (i = nb_engines; i < old_nb_engines; i++)
			(*dev->dev_ops->rx_engine_release)(dev, i);
//			(*dev->dev_ops->rx_engine_release)(rxe[i]);
		rxe = rte_realloc(rxe, sizeof(rxe[0]) * nb_engines, RTE_CACHE_LINE_SIZE);
		if (rxe == NULL)
			return -(ENOMEM);
		if (nb_engines > old_nb_engines) {
			uint16_t new_es = nb_engines - old_nb_engines;

			memset(rxe + old_nb_engines, 0, sizeof(rxe[0]) * new_es);
		}

		dev->data->rx_engines = rxe;

	} else if (dev->data->rx_engines != NULL && nb_engines == 0) {
		DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_release, -ENOTSUP);

		rxe = dev->data->rx_engines;
		for (i = nb_engines; i < old_nb_engines; i++)
			(*dev->dev_ops->rx_engine_release)(dev, i);
//			(*dev->dev_ops->rx_engine_release)(rxe[i]);

		rte_free(dev->data->rx_engines);
		dev->data->rx_engines = NULL;
	}
	dev->data->nb_rx_engines = nb_engines;
	return 0;
}

int
dhl_fpga_dev_rx_engine_setup(uint8_t card_id, uint16_t rx_engine_id,
		uint16_t nb_rx_desc, unsigned int socket_id,
		const struct dhl_fpga_rxconf * rx_conf, struct rte_mempool *mp)
{
	int ret;
	struct dhl_fpga_dev *dev;
	void ** rxe;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];
	if (rx_engine_id >= dev->data->nb_rx_engines) {
		RTE_PMD_DEBUG_TRACE("Invalid RX engine_id=%d\n", rx_engine_id);
		return -EINVAL;
	}

	if (dev->data->dev_started) {
		RTE_PMD_DEBUG_TRACE(
			"port %d must be stopped to allow configuration\n", card_id);
		return -EBUSY;
	}

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_setup, -ENOTSUP);

	/*
	 * Check the size of the mbuf data buffer.
	 * This value must be provided in the private data of the memory pool.
	 * First check that the memory pool has a valid private data.
	 */
	if (mp->private_data_size < sizeof(struct rte_pktmbuf_pool_private)) {
		RTE_PMD_DEBUG_TRACE("%s private_data_size %d < %d\n",
				mp->name, (int) mp->private_data_size,
				(int) sizeof(struct rte_pktmbuf_pool_private));
		return -ENOSPC;
	}

	//it does nothing, to be contiune, but it works

	if(nb_rx_desc > DHL_MAX_RING_DESC || nb_rx_desc < DHL_MIN_RING_DESC) {
		RTE_PMD_DEBUG_TRACE(
				"Number of DMA engines descriptors requested (%u) is not in the supported range [%d - %d]\n",
				nb_rx_desc, DHL_MIN_RING_DESC, DHL_MAX_RING_DESC);
		return -EINVAL;
	}

	rxe = dev->data->rx_engines;
	if (rxe[rx_engine_id]) {
		DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_release,
					-ENOTSUP);
		(*dev->dev_ops->rx_engine_release)(dev, rx_engine_id);
//		(*dev->dev_ops->rx_engine_release)(rxe[rx_engine_id]);
		rxe[rx_engine_id] = NULL;
	}

	if(rx_conf == NULL)
		rx_conf = &default_rxconf;

	ret = (*dev->dev_ops->rx_engine_setup)(dev, rx_engine_id, nb_rx_desc,
			socket_id, rx_conf, mp);
	if(ret){
		RTE_LOG(ERR, PMD , "Setup rx engine %d for fpga card %d error", rx_engine_id, card_id);
		return -1;
	}
	return 0;
}

int
dhl_fpga_dev_rx_engine_start(uint8_t card_id, uint16_t rx_engine_id){
	struct dhl_fpga_dev * dev;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];
	if(rx_engine_id >= dev->data->nb_rx_engines) {
		RTE_PMD_DEBUG_TRACE("Invalid RX engine_id=%d\n", rx_engine_id);
		return -EINVAL;
	}

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_start, -ENOTSUP);

	if (dev->data->rx_engine_state[rx_engine_id] != DHL_FPGA_ENGINE_STATE_STOPPED) {
		RTE_PMD_DEBUG_TRACE("Engine %" PRIu16" of FPGA device with card_id=%" PRIu8
			" already started\n",
			rx_engine_id, card_id);
		return 0;
	}

	return dev->dev_ops->tx_engine_start(dev, rx_engine_id);
}

int
dhl_fpga_dev_rx_engine_stop(uint8_t card_id, uint16_t rx_engine_id){
	struct dhl_fpga_dev * dev;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];
	if (rx_engine_id >= dev->data->nb_rx_engines) {
		RTE_PMD_DEBUG_TRACE("Invalid RX engine_id=%d\n", rx_engine_id);
		return -EINVAL;
	}

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_stop, -ENOTSUP);

	if (dev->data->rx_engine_state[rx_engine_id] == DHL_FPGA_ENGINE_STATE_STOPPED) {
		RTE_PMD_DEBUG_TRACE("Engine %" PRIu16" of device with card_id=%" PRIu8
				" already stopped\n", rx_engine_id, card_id);
		return 0;
	}
	return dev->dev_ops->rx_engine_stop(dev, rx_engine_id);
}

static int
dhl_fpga_dev_tx_engine_config(struct dhl_fpga_dev * dev, uint16_t nb_engines)
{
	uint16_t old_nb_engines = dev->data->nb_tx_engines;
	void ** txe;
	unsigned i;

	if(dev->data->tx_engines == NULL && nb_engines != 0) { /* first time configuration */
		dev->data->tx_engines = rte_zmalloc("fpgadev->tx_engines",
				sizeof(dev->data->tx_engines[0]) * nb_engines,
				RTE_CACHE_LINE_SIZE);
		if (dev->data->tx_engines == NULL) {
			dev->data->nb_tx_engines = 0;
			return -(ENOMEM);
		}
	} else if (dev->data->tx_engines != NULL && nb_engines != 0) { /* re-configure */
		DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->tx_engine_release, -ENOTSUP);

		txe = dev->data->tx_engines;

		for (i = nb_engines; i < old_nb_engines; i++)
			(*dev->dev_ops->tx_engine_release)(dev, i);
//			(*dev->dev_ops->tx_engine_release)(txe[i]);
		txe = rte_realloc(txe, sizeof(txe[0]) * nb_engines, RTE_CACHE_LINE_SIZE);
		if (txe == NULL)
			return -(ENOMEM);
		if (nb_engines > old_nb_engines) {
			uint16_t new_es = nb_engines - old_nb_engines;

			memset(txe + old_nb_engines, 0, sizeof(txe[0]) * new_es);
		}

		dev->data->tx_engines = txe;

	} else if (dev->data->tx_engines != NULL && nb_engines == 0) {
		DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->rx_engine_release, -ENOTSUP);

		txe = dev->data->tx_engines;
		for (i = nb_engines; i < old_nb_engines; i++)
			(*dev->dev_ops->tx_engine_release)(dev, i);
//			(*dev->dev_ops->tx_engine_release)(txe[i]);

		rte_free(dev->data->tx_engines);
		dev->data->tx_engines = NULL;
	}
	dev->data->nb_tx_engines = nb_engines;
	return 0;
}

int
dhl_fpga_dev_tx_engine_setup(uint8_t card_id, uint16_t tx_engine_id,
		uint16_t nb_tx_desc, unsigned int socket_id,
		const struct dhl_fpga_txconf * tx_conf)
{
	int ret;
	struct dhl_fpga_dev *dev;
	void ** txe;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];
	if (tx_engine_id >= dev->data->nb_tx_engines) {
		RTE_PMD_DEBUG_TRACE("Invalid TX engine_id=%d\n", tx_engine_id);
		return -EINVAL;
	}

	if (dev->data->dev_started) {
		RTE_PMD_DEBUG_TRACE(
			"port %d must be stopped to allow configuration\n", card_id);
		return -EBUSY;
	}

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->tx_engine_setup, -ENOTSUP);

	if(nb_tx_desc > DHL_MAX_RING_DESC || nb_tx_desc < DHL_MIN_RING_DESC) {
		RTE_PMD_DEBUG_TRACE(
				"Number of DMA engines descriptors requested (%u) is not in the supported range [%d - %d]\n",
				nb_tx_desc, DHL_MIN_RING_DESC, DHL_MAX_RING_DESC);
		return -EINVAL;
	}

	txe = dev->data->tx_engines;
	if (txe[tx_engine_id]) {
		DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->tx_engine_release,
					-ENOTSUP);
		(*dev->dev_ops->tx_engine_release)(dev, tx_engine_id);
//		(*dev->dev_ops->tx_engine_release)(txe[tx_engine_id]);
		txe[tx_engine_id] = NULL;
	}

	if(tx_conf == NULL)
		tx_conf = &default_txconf;

	ret = (*dev->dev_ops->tx_engine_setup)(dev, tx_engine_id, nb_tx_desc,
			socket_id, tx_conf);
	if(ret){
		RTE_LOG(ERR, PMD , "Setup tx engine %d for fpga card %d error", tx_engine_id, card_id);
		return -1;
	}
	return 0;
}

int
dhl_fpga_dev_tx_engine_start(uint8_t card_id, uint16_t tx_engine_id){
	struct dhl_fpga_dev * dev;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];
	if(tx_engine_id >= dev->data->nb_tx_engines) {
		RTE_PMD_DEBUG_TRACE("Invalid TX engine_id=%d\n", tx_engine_id);
		return -EINVAL;
	}

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->tx_engine_start, -ENOTSUP);

	if (dev->data->tx_engine_state[tx_engine_id] != DHL_FPGA_ENGINE_STATE_STOPPED) {
		RTE_PMD_DEBUG_TRACE("Engine %" PRIu16" of FPGA device with card_id=%" PRIu8
			" already started\n",
			tx_engine_id, card_id);
		return 0;
	}

	return dev->dev_ops->tx_engine_start(dev, tx_engine_id);
}

int
dhl_fpga_dev_tx_engine_stop(uint8_t card_id, uint16_t tx_engine_id){
	struct dhl_fpga_dev * dev;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];
	if (tx_engine_id >= dev->data->nb_tx_engines) {
		RTE_PMD_DEBUG_TRACE("Invalid TX engine_id=%d\n", tx_engine_id);
		return -EINVAL;
	}

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->tx_engine_stop, -ENOTSUP);

	if (dev->data->tx_engine_state[tx_engine_id] == DHL_FPGA_ENGINE_STATE_STOPPED) {
		RTE_PMD_DEBUG_TRACE("Engine %" PRIu16" of device with card_id=%" PRIu8
				" already stopped\n", tx_engine_id, card_id);
		return 0;
	}
	return dev->dev_ops->tx_engine_stop(dev, tx_engine_id);
}

int
dhl_fpga_dev_configure(uint8_t card_id, uint16_t nb_rx_e, uint16_t nb_tx_e,
		const struct dhl_fpga_conf * dev_conf)
{
	int diag;
	struct dhl_fpga_dev * dev;
	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	if(nb_rx_e > DHL_MAX_ENGINE_PER_FPGA) {
		RTE_PMD_DEBUG_TRACE(
			"Number of RX engines requested (%u) is greater than max supported(%d)\n",
			nb_rx_e, DHL_MAX_ENGINE_PER_FPGA);
		return -EINVAL;
	}

	if(nb_tx_e > DHL_MAX_ENGINE_PER_FPGA) {
		RTE_PMD_DEBUG_TRACE(
			"Number of TX engines requested (%u) is greater than max supported(%d)\n",
			nb_tx_e, DHL_MAX_ENGINE_PER_FPGA);
		return -EINVAL;
	}

	dev = &dhl_fpga_devices[card_id];

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->dev_configure, -ENOTSUP);

	if (dev->data->dev_started) {
		RTE_PMD_DEBUG_TRACE(
			"FPGA device %d must be stopped to allow configuration\n", card_id);
		return -EBUSY;
	}

	/* copy the dev_conf parameter into the dev structure */
	memcpy(&dev->data->dev_conf, dev_conf, sizeof(dev->data->dev_conf));

	/*
	 * Check that the numbers of RX and TX queues are not greater
	 * than the maximum number of RX and TX queues supported by the
	 * configured device.
	 */
	if (nb_rx_e == 0 && nb_tx_e == 0) {
		RTE_PMD_DEBUG_TRACE("PFGA dev card_id=%d both rx and tx engine cannot be 0\n", port_id);
		return -EINVAL;
	}
	// it haven't done yet, to be continue. But it works.

	/*
	 * setup new number of DMA engine and reconfigure device
	 */
	diag = dhl_fpga_dev_rx_engine_config(dev, nb_rx_e);
	if (diag != 0) {
		RTE_PMD_DEBUG_TRACE("FPGA device %d dhl_fpga_dev_rx_queue_config = %d\n",
						card_id, diag);
		return diag;
	}

	diag = dhl_fpga_dev_tx_engine_config(dev, nb_tx_e);
	if (diag != 0) {
		RTE_PMD_DEBUG_TRACE("FPGA device %d rte_eth_dev_tx_queue_config = %d\n",
				card_id, diag);
		dhl_fpga_dev_rx_engine_config(dev, 0);
		return diag;
	}

	diag = (*dev->dev_ops->dev_configure)(dev);
	if (diag != 0) {
		RTE_PMD_DEBUG_TRACE("FPGA device %d dev_configure = %d\n",
				card_id, diag);
		dhl_fpga_dev_rx_engine_config(dev, 0);
		dhl_fpga_dev_tx_engine_config(dev, 0);
		return diag;
	}

	return 0;
}

static void
dhl_fpga_dev_config_restore(uint8_t card_id __rte_unused)
{

}


int
dhl_fpga_dev_start(uint8_t card_id)
{
	struct dhl_fpga_dev * dev;
	int diag;

	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->dev_start, -ENOTSUP);

	if (dev->data->dev_started != 0) {
		RTE_PMD_DEBUG_TRACE("FPGA Device with card_id=%" PRIu8
				"already started\n", card_id);
		return 0;
	}

	diag = (*dev->dev_ops->dev_start)(dev);
	if(diag == 0)
		dev->data->dev_started = 1;
	else
		return diag;

	dhl_fpga_dev_config_restore(card_id);

//	if (dev->data->dev_conf.intr_conf.lsc == 0) {
//		RTE_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->link_update, -ENOTSUP);
//		(*dev->dev_ops->link_update)(dev, 0);
//	}
	return 0;
}

int
dhl_fpga_dev_stop(uint8_t card_id)
{
	struct dhl_fpga_dev * dev;
	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->dev_stop, -ENOTSUP);

	if (dev->data->dev_started == 0) {
		RTE_PMD_DEBUG_TRACE("FPGA Device with card_id=%" PRIu8
					" already stopped\n",
					card_id);
		return -1;
	}

	dev->data->dev_started = 0;
	return (*dev->dev_ops->dev_stop)(dev);

}

int
dhl_fpga_get_stats(uint8_t card_id,
		struct dhl_fpga_stats * stats, uint16_t num){

	int diag;
	struct dhl_fpga_dev * dev;
	DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, -EINVAL);

	dev = &dhl_fpga_devices[card_id];

	DHL_FUNC_PTR_OR_ERR_RET(*dev->dev_ops->dev_get_stats, -ENOTSUP);
	diag = (*dev->dev_ops->dev_get_stats)(dev, stats, num);

	if(diag){
		RTE_LOG(ERR, PMD , "Get FPGA statisics error\n");
		return -1;
	}
	return 0;
}
