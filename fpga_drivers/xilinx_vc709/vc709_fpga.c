/*
 * vc709_fpga.c
 *
 *  Created on: Jun 19, 2017
 *      Author: root
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>

#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_mbuf.h>
#include <rte_log.h>
#include <rte_eal.h>
#include <rte_cycles.h>
#include <rte_mempool.h>

#include <dhl_fpga_pci.h>
#include <dhl_fpga.h>

#include "dma/xdma.h"
#include "dma/xdma_bdring.h"
#include "dma/xdma_user.h"
#include "dma/xpmon_be.h"
#include "vc709_type.h"
#include "dma/xdma_hw.h"
#include "vc709_get_stats.h"
#include "vc709_log.h"
#include "vc709_pci_uio.h"
#include "vc709_rxtx.h"
#include "vc709_specific.h"

#include "vc709_fpga.h"



/************************** Variable Names ***********************************/

uint64_t clock_hz;


/************************** Function Prototypes ******************************/
static int  vc709_dev_configure(struct dhl_fpga_dev *dev);
static int  vc709_dev_start(struct dhl_fpga_dev *dev);
static int  vc709_dev_stop(struct dhl_fpga_dev *dev);
static void vc709_dev_close(struct dhl_fpga_dev *dev);

void vc709_dev_rx_engine_release(struct dhl_fpga_dev * dev, uint16_t engine_idx);
int vc709_dev_rx_engine_setup(struct dhl_fpga_dev *dev,
		uint16_t sw_engine_id,
		uint16_t nb_rx_desc,
		unsigned int socket_id,
		const struct dhl_fpga_rxconf *rx_conf,
		struct rte_mempool *mb_pool);
int vc709_dev_rx_engine_start(struct dhl_fpga_dev * dev, uint16_t sw_engine_id);
int vc709_dev_rx_engine_stop(struct dhl_fpga_dev * dev, uint16_t rx_engine_id);
void vc709_dev_tx_engine_release(struct dhl_fpga_dev * dev, uint16_t engine_idx);
int vc709_dev_tx_engine_setup(struct dhl_fpga_dev * dev,
		uint16_t sw_engine_id,
		uint16_t nb_tx_desc,
		unsigned int socket_id,
		const struct dhl_fpga_txconf *tx_conf);
int vc709_dev_tx_engine_start(struct dhl_fpga_dev * dev, uint16_t engine_id);
int vc709_dev_tx_engine_stop(struct dhl_fpga_dev * dev, uint16_t engine_id);


//static void poll_stats(struct vc709_adapter * adapter);
static void ReadDMAEngineConfiguration(struct dhl_fpga_dev *fpga_dev);
/*
 * The set of PCI devices this driver supports
 */
static const struct rte_pci_id pci_id_vc709_map[] = {
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_DMA, PCI_DEVICE_ID_DMA)},
	{ .vendor_id = 0,/* sentinel */},
};

static const struct fpga_dev_ops vc709_fpga_dev_ops = {
		.dev_configure        = vc709_dev_configure,
		.dev_start            = vc709_dev_start,
		.dev_stop             = vc709_dev_stop,
		.dev_close            = vc709_dev_close,

		.rx_engine_release	  = vc709_dev_rx_engine_release,
		.rx_engine_setup	  = vc709_dev_rx_engine_setup,
		.rx_engine_start      = vc709_dev_rx_engine_start,
		.rx_engine_stop		  = vc709_dev_rx_engine_stop,

		.tx_engine_release	  = vc709_dev_tx_engine_release,
		.tx_engine_setup	  = vc709_dev_tx_engine_setup,
		.tx_engine_start   	  = vc709_dev_tx_engine_start,
		.tx_engine_stop       = vc709_dev_tx_engine_stop,

		.dev_get_stats	  = vc709_dev_get_stats,
};


static int
vc709_dev_configure(struct dhl_fpga_dev * dev __rte_unused){

	return 0;
}

/*
 * Configure device link speed and setup link.
 * It returns 0 on success.
 */
static int
vc709_dev_start(struct dhl_fpga_dev *dev __rte_unused)
{
	return 0;
}

/*
 * Stop device: disable rx and tx functions to allow for reconfiguring.
 */
static int
vc709_dev_stop(struct dhl_fpga_dev *dev __rte_unused)
{
	return 0;
}

/*
 * Reest and stop device.
 */
static void
vc709_dev_close(struct dhl_fpga_dev *dev __rte_unused)
{

}




//static void poll_stats(struct vc709_adapter * adapter)
//{
//	struct vc709_bar * bar;
//	struct vc709_statics * statics;
//
//	u64 base;
//
//
//	bar = &(adapter->bar);
//	statics = &(adapter->statics);
//	base = (u64) bar->barBase;
//
//	if(adapter->DriverState == UNREGISTERING)
//		return;
//
//#ifdef PM_SUPPORT
//	if(adapter->DriverState == PM_PREPARE)
//		return;
//#endif
//
//#ifdef PM_SUPPORT
//	statics->pmval.vcc = XIo_In32(base+PVTMON_VCCINT);
//	statics->pmval.vccaux = XIo_In32(base+PVTMON_VCCAUX);
//	statics->pmval.vcc3v3 = XIo_In32(base+PVTMON_VCC3V3);
//	statics->pmval.vadj = XIo_In32(base+PVTMON_VADJ);
//	statics->pmval.vcc2v5 = XIo_In32(base+PVTMON_VCC2V5);
//	statics->pmval.vcc1v5 = XIo_In32(base+PVTMON_VCC1V5);
//	statics->pmval.mgt_avcc = XIo_In32(base+PVTMON_MGT_AVCC);
//	statics->pmval.mgt_avtt = XIo_In32(base+PVTMON_MGT_AVTT);
//	statics->pmval.vccaux_io = XIo_In32(base+PVTMON_VCCAUX_IO);
//	statics->pmval.vccbram = XIo_In32(base+PVTMON_VCC_BRAM);
//	statics->pmval.mgt_vccaux = XIo_In32(base+PVTMON_MGT_VCCAUX);
//	statics->pmval.pwr_rsvd = XIo_In32(base+PVTMON_RSVD);
//	statics->pmval.die_temp = (XIo_In32(base+DIE_TEMP)*504)/1024 - 273;
//
//	printf( "VCCINT=%x ",statics->pmval.vcc);
//	printf( "VCCAUX=%x ",statics->pmval.vccaux);
//	printf( "VCC3V3=%x ",statics->pmval.vcc3v3);
//	printf( "MGT_AVCC=%x ",statics->pmval.mgt_avcc);
//	printf( "MGT_AVTT=%x ",statics->pmval.mgt_avtt);
//	printf( "VCCAUX_IO=%x ",statics->pmval.vccaux_io);
//	printf( "VCCBRAM=%x ",statics->pmval.vccbram);
//	printf( "DIE_TEMP=%x \n\n",statics->pmval.die_temp);
//#endif
//}

static void ReadDMAEngineConfiguration(struct dhl_fpga_dev *fpga_dev)
{
	struct vc709_bar * bar;
	struct vc709_dma_engine * dma_engine;

	u64 base, offset;
	u32 val, type, dirn, num, bc;
	int i;
	int scalval = 0, reg = 0;
	Dma_Engine * eptr;

	bar = VC709_DEV_PRIVATE_TO_BAR(fpga_dev->data->dev_private);
	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(fpga_dev->data->dev_private);

	base = (u64) bar->barBase;

	/* Walk through the capability register of all DMA engines */
	for(offset = DMA_OFFSET, i=0; offset < DMA_SIZE; offset += DMA_ENGINE_PER_SIZE, i++){
		val = Dma_mReadReg((base+offset), REG_DMA_ENG_CAP);

		if(val & DMA_ENG_PRESENT_MASK)
		{
			dma_engine->RawTestMode[i] = TEST_STOP;
			RTE_LOG(INFO, PMD,"\n");
			RTE_LOG(INFO, PMD,"##Engine capability is %x##\n", val);
			scalval = (val & DMA_ENG_SCAL_FACT) >> 30;
			RTE_LOG(INFO, PMD,">> DMA engine scaling factor = 0x%x\n", scalval);
			dma_engine->scal_val[i] = scalval;

			reg = XIo_In32(base + PCIE_CAP_REG);
			XIo_Out32(base + PCIE_CAP_REG,(reg | scalval ));

			eptr = &(dma_engine->Dma[i]);

			RTE_LOG(INFO, PMD,"DMA Engine present at offset %lx: \n", offset);

			dirn = (val & DMA_ENG_DIRECTION_MASK);
			if(dirn == DMA_ENG_C2S)
				RTE_LOG(INFO, PMD,"C2S, ");
			else
				RTE_LOG(INFO, PMD,"S2C, ");

			type = (val & DMA_ENG_TYPE_MASK);
			if(type == DMA_ENG_BLOCK)
				printf("Block DMA, ");
			else if(type == DMA_ENG_PACKET)
				printf("Packet DMA, ");
			else
				printf("Unknown DMA %x, ", type);

			num = (val & DMA_ENG_NUMBER) >> DMA_ENG_NUMBER_SHIFT;
			printf("Eng. Number %d, ", num);

			bc = (val & DMA_ENG_BD_MAX_BC) >> DMA_ENG_BD_MAX_BC_SHIFT;
			printf("Max Byte Count 2^%d\n", bc);

			if(type != DMA_ENG_PACKET) {
				RTE_LOG(INFO, PMD,"This driver is capable of only Packet DMA\n");
				continue;
			}

			/* Initialise this engine's data structure. This will also
			 * reset the DMA engine.
			 */
			Dma_Initialize(eptr, (base + offset), dirn, i);

			dma_engine->engineMask |= (1LL << i);
		}

	}

}

static int
fpga_vc709_dev_init(struct dhl_fpga_dev * fpga_dev)
{
	struct rte_pci_device * pci_dev = DHL_FPGA_DEV_TO_PCI(fpga_dev);
//	struct rte_intr_handle * intr_handle = &pci_dev->intr_handle;

	struct vc709_adapter * adapter =
			(struct vc709_adapter *)fpga_dev->data->dev_private;
	struct vc709_hw * hw =
			VC709_DEV_PRIVATE_TO_HW(fpga_dev->data->dev_private);
	struct vc709_dma_engine * dma_engine =
			VC709_DEV_PRIVATE_TO_DMAENGINE(fpga_dev->data->dev_private);
	struct vc709_bar * bar =
			VC709_DEV_PRIVATE_TO_BAR(fpga_dev->data->dev_private);
	struct vc709_mapped_kernel_resource * bd_res =
			VC709_DEV_PRIVATE_TO_BDRES(fpga_dev->data->dev_private);

	int i;

	fpga_dev->dev_ops = &vc709_fpga_dev_ops;

	fpga_dev->rx_pkts = &vc709_dma_recv_pkts;
	fpga_dev->rx_pkts_calc_latency = &vc709_dma_recv_pkts_calc_latency;
	fpga_dev->tx_pkts = &vc709_dma_send_pkts;
	fpga_dev->tx_pkts_noseg = &vc709_dma_send_pkts_noseg;
	fpga_dev->tx_pkts_noseg_add_timestamp = &vc709_dma_send_pkts_noseg_add_timestamp;
	fpga_dev->tx_pkts_burst = &vc709_dma_send_pkts_burst;


	clock_hz = rte_get_timer_hz();

	/*
	 * map the bd coherent BD memory to this user space application
	 * because the BD descriptors need 32 bits dma_addr_t,
	 * we cannot use rte_mem_zone,
	 * the only way is that we create it in UIO kernel driver, and map them to the user space application
	 */
	vc709_uio_map_bd_memeory(pci_dev, bd_res);

	for(i = 0; i < bd_res->nb_maps; i++)
	{
		VC709_LOG("bd_res[%d] Phaddr is 0x%lx, addr is %p, size is 0x%lx \n", i, bd_res->maps[i].phaddr,
				bd_res->maps[i].addr,
				bd_res->maps[i].size);
	}

	/*
	 * For secondary processes, we don't initialise any further as primary
	 * has already done this work. Only check we don't need a different
	 * RX and TX function.
	 */
	if(rte_eal_process_type() != RTE_PROC_PRIMARY) {

	}

	/* Vendor and Device ID need to be set before init of shared code */
	hw->device_id = pci_dev->id.device_id;
	hw->vendor_id = pci_dev->id.vendor_id;
	hw->hw_addr = (void *)pci_dev->mem_resource[0].addr;

	bar->barMask = 0;
	bar->barBase = hw->hw_addr;
	dma_engine->engineMask = 0;

	for(i = 0; i< PCI_MAX_RESOURCE;i++){
		if((void *)pci_dev->mem_resource[i].len == 0) {
			if(i == 0){
				rte_exit(EXIT_FAILURE,"BAR 0 not valid, aborting.\n");
			}
			else
				continue;
		}
		/* Set a bitmask for all the BARs that are present. */
		else
			(bar->barMask) |= ( 1 << i);

		bar->barInfo[i].basePAddr = pci_dev->mem_resource[i].phys_addr;
		bar->barInfo[i].baseLen = pci_dev->mem_resource[i].len;
		bar->barInfo[i].baseVAddr = pci_dev->mem_resource[i].addr;


		VC709_LOG("bar %d basePAddr is 0x%lx, baseLen is %ld, baseVAddr is %p \n",i ,
				bar->barInfo[i].basePAddr,
				bar->barInfo[i].baseLen,
				bar->barInfo[i].baseVAddr);

	}
	/* Disable global interrupts */
	Dma_mIntDisable(hw->hw_addr);
	/* Read DMA engine configuration and initialize data structures */
	ReadDMAEngineConfiguration(fpga_dev);

	adapter->DriverState = INITIALIZED;

//	poll_stats(adapter);

	return 0;
}

static int
fpga_vc709_dev_uninit(struct dhl_fpga_dev * fpga_dev __rte_unused)
{
//	struct rte_pci_device *pci_dev = VC709_DEV_TO_PCI(fpga_dev);
//	struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;

	RTE_LOG(INFO, PMD, "fpga_vc709_dev_uninit() call here.\n");
	printf("fpga_vc709_dev_uninit() call here1.\n");

	return 0;
}

static int vc709_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
	struct rte_pci_device *pci_dev)
{
	return dhl_fpga_dev_pci_generic_probe(pci_dev,
		sizeof(struct vc709_adapter), fpga_vc709_dev_init);
}

static int vc709_pci_remove(struct rte_pci_device *pci_dev)
{
	return dhl_fpga_dev_pci_generic_remove(pci_dev, fpga_vc709_dev_uninit);
}



static struct rte_pci_driver dhl_vc709_pmd = {
	.id_table = pci_id_vc709_map,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING,
	.probe = vc709_pci_probe,
	.remove = vc709_pci_remove,
};

RTE_PMD_REGISTER_PCI(fpga_vc709, dhl_vc709_pmd);
RTE_PMD_REGISTER_PCI_TABLE(fpga_vc709, pci_id_vc709_map);
RTE_PMD_REGISTER_KMOD_DEP(fpga_vc709, "* uio_pci_generic");
