/*
 * dhl_fpga.h
 *
 * Created on : Oct 9, 2017
 * 	Author: Xiaoyao Li
 *
 */

#ifndef LIBDHL_FPGA_H_
#define LIBDHL_FPGA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <rte_dev.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_pci.h>

#ifndef DHL_MAX_FPGA_CARDS
#define DHL_MAX_FPGA_CARDS 4
#endif


#define DHL_MIN_RING_DESC 32
#define DHL_MAX_RING_DESC 2048
#define DHL_MAX_ENGINE_PER_FPGA	4


/**
 * RX/TX engine states
 */
#define DHL_FPGA_ENGINE_STATE_STOPPED 0
#define DHL_FPGA_ENGINE_STATE_STARTED 1

struct dhl_fpga_dev;

/* Macros to check for valid fpga card */
#define DHL_FPGA_VALID_CARDID_OR_ERR_RET(card_id, retval) do { \
	if (!dhl_fpga_dev_is_valid_card(card_id)) { \
		RTE_PMD_DEBUG_TRACE("Invalid card_id=%d\n", card_id); \
		return retval; \
	} \
} while (0)

#define DHL_FPGA_VALID_CARDID_OR_RET(card_id) do { \
	if (!dhl_fpga_dev_is_valid_card(card_id)) { \
		RTE_PMD_DEBUG_TRACE("Invalid card_id=%d\n", card_id); \
		return; \
	} \
} while (0)

/* Macros to check for invalid function pointers */
#define DHL_FUNC_PTR_OR_ERR_RET(func, retval) do { \
	if ((func) == NULL) { \
		RTE_PMD_DEBUG_TRACE("Function not supported\n"); \
		return retval; \
	} \
} while (0)

#define DHL_FUNC_PTR_OR_RET(func) do { \
	if ((func) == NULL) { \
		RTE_PMD_DEBUG_TRACE("Function not supported\n"); \
		return; \
	} \
} while (0)

#define DHL_FPGA_DEV_TO_PCI(fpga_dev)	RTE_DEV_TO_PCI((fpga_dev)->device)

/*
 * A structure used to configure an Rx engine of an FPGA card
 */
struct dhl_fpga_rxconf{
	uint8_t rx_drop_en; /**< Drop packets if no descriptors are available. */
};

/*
 * A structure used to configure a Tx engine of an FPGA card
 */
struct dhl_fpga_txconf {
	uint32_t txe_flags; /**< Set flags for the Tx queue */
};

/**
 * A structure used to configure an RX ring of an Ethernet port.
 */
struct dhl_fpga_conf {
//	struct dhl_fpga_thresh rx_thresh; /**< RX ring threshold registers. */
	uint32_t rx_buf_size; 	/* setupRevBuf size*/
	uint32_t max_batching_size;

	uint32_t per_packet_size;
	uint8_t rx_drop_en; /**< Drop packets if no descriptors are available. */
};

/**
 * @internal
 * statics structure to show the throughput of one FPGA dma channels
 */
struct dhl_fpga_stats {
	float throughput;
};

/*
 * Definitions of all functions exported by an FPGA driver through the
 * generic structure of type *fpga_dev_ops* supplied in the *dhl_fpga_dev*->dev_ops
 * structure associated with an FPGA device.
 */
/**< @internal FPGA device configuration. */
typedef int (*fpga_dev_configure_t)(struct dhl_fpga_dev *dev);

/**< @internal Function used to start a configured FPGA device. */
typedef int  (*fpga_dev_start_t)(struct dhl_fpga_dev *dev);

/**< @internal Function used to stop a configured FPGA device. */
typedef int (*fpga_dev_stop_t)(struct dhl_fpga_dev *dev);

/**< @internal Function used to close a configured FPGA device. */
typedef void (*fpga_dev_close_t)(struct dhl_fpga_dev *dev);

/** **/
typedef int (*fpga_dev_setup_encap_pkts_pool_t)(struct dhl_fpga_dev *dev, uint32_t nb_dma_bd, unsigned int socket_id);



/**< @internal Start rx and tx of a queue of an Ethernet device. */
typedef int (*fpga_engine_start_t)(struct dhl_fpga_dev *dev,
				    uint16_t engine_id);

/**< @internal Stop rx and tx of a queue of an Ethernet device. */
typedef int (*fpga_engine_stop_t)(struct dhl_fpga_dev *dev,
				    uint16_t engine_id);

/**< @internal Set up a receive queue of an Ethernet device. */
typedef int (*fpga_rx_engine_setup_t)(struct dhl_fpga_dev *dev,
				    uint16_t sw_engine_id,
				    uint16_t nb_desc,
				    unsigned int socket_id,
				    const struct dhl_fpga_rxconf *rx_conf,
				    struct rte_mempool *mb_pool);

/**< @internal Setup a transmit queue of an Ethernet device. */
typedef int (*fpga_tx_engine_setup_t)(struct dhl_fpga_dev *dev,
				    uint16_t sw_engine_id,
				    uint16_t nb_desc,
				    unsigned int socket_id,
				    const struct dhl_fpga_txconf *tx_conf);

/**< @internal Release memory resources allocated by given RX/TX engine. */
typedef void (*fpga_engine_release_t)(struct dhl_fpga_dev * dev, uint16_t engine_idx);



typedef int (*fpga_dev_get_statics_t)(struct dhl_fpga_dev * dev, struct dhl_fpga_stats * stats, uint16_t num);

typedef int (*fpga_dma_engine_start_t)(struct dhl_fpga_dev * dev, uint16_t engine_id);

typedef int (*fpga_dma_engine_stop_t)(struct dhl_fpga_dev * dev, uint16_t engine_id);

/**< @internal Set up a receive queue of an Ethernet device. */
typedef int (*fpga_dma_engine_setup_t)(struct dhl_fpga_dev *dev,
				    uint16_t engine_id,
				    uint16_t nb_dma_bd,
				    unsigned int socket_id,
				    struct rte_mempool *mb_pool);


typedef int (*fpga_dma_engine_release_t)(struct dhl_fpga_dev *dev,
	    uint16_t engine_id);




typedef int16_t (*fpga_rx_pkts_t)(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **rx_pkts,
		uint16_t numpkts);
/**< @internal Retrieve input packets from a receive queue of an Ethernet device. */

typedef int16_t (*fpga_tx_pkts_t)(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts);
/**< @Send packets to a transmit DMA engine of an FPGA device. */


/**
 * @internal A structure containing the functions exported by an FPGA driver.
 */
struct fpga_dev_ops {
	fpga_dev_configure_t		dev_configure;	   	/**< Configure device. */
	fpga_dev_start_t            dev_start;     		/**< Start device. */
	fpga_dev_stop_t             dev_stop;      		/**< Stop device. */
	fpga_dev_close_t            dev_close;     		/**< Close device. */

	fpga_rx_engine_setup_t       rx_engine_setup;	/**< Set up device RX engine. */
	fpga_engine_start_t          rx_engine_start;	/**< Start RX for a engine. */
	fpga_engine_stop_t           rx_engine_stop; 	/**< Stop RX for a engine. */
	fpga_engine_release_t        rx_engine_release; /**< Release RX engine. */

	fpga_tx_engine_setup_t       tx_engine_setup;	/**< Set up device TX engine. */
	fpga_engine_start_t          tx_engine_start;	/**< Start TX for a engine. */
	fpga_engine_stop_t           tx_engine_stop; 	/**< Stop TX for a engine. */
	fpga_engine_release_t        tx_engine_release; /**< Release RX engine. */

	fpga_dev_setup_encap_pkts_pool_t dev_setup_encap_pkts_pool;
	fpga_dev_get_statics_t		dev_get_stats;
};

/**
 * A set of values to describe the possible states of an FPGA device.
 */
enum dhl_fpga_dev_state {
	DHL_FPGA_DEV_UNUSED = 0,
	DHL_FPGA_DEV_ATTACHED,
};

/**
 * @internal
 * The generic data structure associated with each FPGA device.
 *
 * Pointers to burst-oriented packet receive and transmit functions are
 * located at the beginning of the structure, along with the pointer to
 * where all the data elements for the particular device are stored in shared
 * memory. This split allows the function pointer and driver data to be per-
 * process, while the actual configuration data for the device is shared.
 */
struct dhl_fpga_dev {
	fpga_rx_pkts_t rx_pkts; /**< Pointer to PMD receive function. */
	fpga_rx_pkts_t rx_pkts_calc_latency;

	fpga_tx_pkts_t tx_pkts; /**< Pointer to PMD transmit function. */
	fpga_tx_pkts_t tx_pkts_noseg;
	fpga_tx_pkts_t tx_pkts_noseg_add_timestamp;
	fpga_tx_pkts_t tx_pkts_burst;

	struct dhl_fpga_dev_data *data;  /**< Pointer to device data */
	const struct fpga_dev_ops *dev_ops; /**< Functions exported by PMD */
	struct rte_device *device; /**< Backing device */
	struct rte_intr_handle *intr_handle; /**< Device interrupt handle */
	/** User application callbacks for NIC interrupts */
//	struct rte_eth_dev_cb_list link_intr_cbs;
	enum dhl_fpga_dev_state state; /**< Flag indicating the port state */
} __rte_cache_aligned;

#define DHL_FPGA_NAME_MAX_LEN (32)

/**
 * @internal
 * The data part, with no function pointers, associated with each FPGA device.
 *
 * This structure is safe to place in shared memory to be common among different
 * processes in a multi-process configuration.
 */
struct dhl_fpga_dev_data {
	char name[DHL_FPGA_NAME_MAX_LEN]; /**< Unique identifier name */

	void ** rx_engines;	/**< Array of pointers to DMA rx engines. */
	void ** tx_engines;	/**< Array of pointers to DMA tx engines. */
	uint16_t nb_rx_engines;	/**< Number of RX engines. */
	uint16_t nb_tx_engines; /**< Number of TX engines. */

	void *dev_private;              /**< PMD-specific private data */

	struct dhl_fpga_conf dev_conf; /**< Configuration applied to device. */

	uint8_t card_id;           /**< Device [external] card identifier. */

	__extension__
	uint8_t promiscuous   : 1, /**< RX promiscuous mode ON(1) / OFF(0). */
		scattered_rx : 1,  /**< RX of scattered packets is ON(1) / OFF(0) */
		all_multicast : 1, /**< RX all multicast mode ON(1) / OFF(0). */
		dev_started : 1,   /**< Device state: STARTED(1) / STOPPED(0). */
		lro         : 1;   /**< RX LRO is ON(1) / OFF(0) */
	uint8_t rx_engine_state[DHL_MAX_ENGINE_PER_FPGA];
	/** Queues state: STARTED(1) / STOPPED(0) */
	uint8_t tx_engine_state[DHL_MAX_ENGINE_PER_FPGA];
	/** Queues state: STARTED(1) / STOPPED(0) */

	uint32_t dev_flags; /**< Capabilities */
	enum rte_kernel_driver kdrv;    /**< Kernel driver passthrough */
	int numa_node;  /**< NUMA node connection */
	const char *drv_name;   /**< Driver name */
};

/**
 * @internal
 * The pool of *dhl_fpga_dev* structures. The size of the pool
 * is configured at compile-time in the <dhl_fpga.c> file.
 */
extern struct dhl_fpga_dev dhl_fpga_devices[];

/********************* interfaces *********************************/

/**
 * Iterates over valid fpgadev cards.
 *
 * @param card_id
 * 		The id of the next possible valid card.
 * @return
 * 		Next valid card id, DHL_MAX_FPGA_CARDS if there is none.
 */
uint8_t dhl_fpga_find_next(uint8_t card_id);

/**
 * Macro to iterate over all enabled fpgadev card.
 */
#define DHL_FPGA_FOREACH_DEV(p)			\
	for (p = dhl_fpga_find_next(0)		\
			(unsigned int)p < (unsigned int)DHL_MAX_FPGA_CARDS;	\
			p = dhl_fpga_find_next(p + 1))

/**
 * @interal
 * Returns a fpgadev slot specified by the unique identifier name.
 *
 * @param	name
 * 	The pointer to the Unique identifier name for each FPGA device
 * @return
 * 		- The pointer to the fpgadev slot, on success. NULL on error
 */
struct dhl_fpga_dev * dhl_fpga_dev_allocated(const char * name);

/**
 * @internal
 * Allocates a new fpga slot for an fpga card device and returns the pointer
 * to that slot for the driver to use.
 *
 * @param	name	Unique identifier name for each Ethernet device
 * @return
 *   - Slot in the dhl_fpga_devices array for a new device;
 */
struct dhl_fpga_dev *dhl_fpga_dev_allocate(const char *name);

/**
 * @internal
 * Attach to the fpga dev already initialized by the primary
 * process.
 *
 * @param       name    FPGA device's name.
 * @return
 *   - Success: Slot in the dhl_fpga_devices array for attached
 *        device.
 *   - Error: Null pointer.
 */
struct dhl_fpga_dev *dhl_fpga_dev_attach_secondary(const char *name);

/**
 * @internal
 * Release the specified fpga card.
 *
 * @param fpga_dev
 * The *fpga_dev* pointer is the address of the *dhl_fpga_dev* structure.
 * @return
 *   - 0 on success, negative on error
 */
int dhl_fpga_dev_release_card(struct dhl_fpga_dev *fpga_dev);

int dhl_fpga_dev_is_valid_card(uint8_t card_id);

/**
 * Create memzone for HW rings.
 * malloc can't be used as the physical address is needed.
 * If the memzone is already created, then this function returns a ptr
 * to the old one.
 *
 * @param fpga_dev
 *   The *fpga_dev* pointer is the address of the *dhl_fpga_dev* structure
 * @param name
 *   The name of the memory zone
 * @param engine_id
 *   The index of the dma engine to add to name
 * @param size
 *   The sizeof of the memory area
 * @param align
 *   Alignment for resulting memzone. Must be a power of 2.
 * @param socket_id
 *   The *socket_id* argument is the socket identifier in case of NUMA.
 */
const struct rte_memzone *
dhl_fpga_dma_zone_reserve(const struct dhl_fpga_dev *fpga_dev, const char *name,
			 uint16_t engine_id, size_t size,
			 unsigned align, int socket_id);


unsigned int dhl_fpga_dev_socket_id(uint8_t card_id);

/**
 * Get the total number of FPGA devices that have been successfully
 * initialized by the matching Ethernet driver during the PCI probing phase
 * and that are available for application to ues. These devices must be
 * accessed by using the ``DHL_FPGA_FOREACH_DEV()`` macro to deal with
 * non-contiguous ranges of devices.
 * These non-contiguous ranges can be created by calls to hotplug functions or
 * by some PMDs.
 *
 * @return
 * 		- The total number of usable FPGA devices.
 */
uint8_t dhl_fpga_dev_count(void);

int dhl_fpga_dev_rx_engine_setup(uint8_t card_id, uint16_t rx_engine_id,
		uint16_t nb_rx_desc, unsigned int socket_id,
		const struct dhl_fpga_rxconf * rx_conf, struct rte_mempool *mp);

int dhl_fpga_dev_rx_engine_start(uint8_t card_id, uint16_t rx_engine_id);

int dhl_fpga_dev_rx_engine_stop(uint8_t card_id, uint16_t rx_engine_id);

int dhl_fpga_dev_tx_engine_setup(uint8_t card_id, uint16_t tx_engine_id,
		uint16_t nb_tx_desc, unsigned int socket_id,
		const struct dhl_fpga_txconf * tx_conf);

int dhl_fpga_dev_tx_engine_start(uint8_t card_id, uint16_t tx_engine_id);

int dhl_fpga_dev_tx_engine_stop(uint8_t card_id, uint16_t tx_engine_id);


/*
 * Configure an FPGA device.
 * This function must be invoked first before any other function in the DHL FPGA API.
 * This function can also be re-invoked when a device is in the stopped state.
 *
 * @param card_id
 * 	the card identifier of the FPGA device to configure
 * @param nb_rx_engine
 *   The number of receive engines to set up for the FPGA device.
 * @param nb_tx_engine
 *   The number of transmit engines to set up for the FPGA device.
 * @param dev_conf
 *   The pointer to the configuration data to be used for the FPGA device.
 *   The *dhl_fpga_conf* structure includes:
 *
 *
 *   Embedding all configuration information in a single data structure
 *   is the more flexible method that allows the addition of new features
 *   without changing the syntax of the API.
 * @return
 *   - 0: Success, device configured.
 *   - <0: Error code returned by the driver configuration function.
 */
int dhl_fpga_dev_configure(uint8_t card_id, uint16_t nb_rx_engine, uint16_t nb_tx_engine,
		const struct dhl_fpga_conf * dev_conf);

/**
 * Start an FPGA device.
 *
 * The device start step is the last one.
 * On success, all basic functions exported by the FPGA API (status, receive/transmit, and so on )
 * can be invoked.
 *
 * @param card_id
 * 		The card identifier of the FPGA device.
 * @return
 *   - 0: Success, FPGA device stated.
 *   - <0: Error code of the driver device start function.
 */
int dhl_fpga_dev_start(uint8_t card_id);

/**
 * Stop an FPGA device. The device can be restarted with a call to
 * dhl_fpga_dev_start()
 *
 * @param card_id
 * 		the card identifier of the FPGA device.
 */
int dhl_fpga_dev_stop(uint8_t card_id);

/**
 *  Retrieve the general I/O statistics of an FPGA device.
 *
 *  @param card_id
 *  	The card identifier of the FPGA device.
 *  @param stats
 *  	A pointer to a structure of type *dhl_fpga_stats* to be filled with
 *  	the values o device counters for the following set of statistics:
 *
 *  @return
 *  	Zero if successful. Non-zero otherwise.
 */
int dhl_fpga_get_stats(uint8_t card_id, struct dhl_fpga_stats * stats, uint16_t num);


/******************** FPGA TX/RX functions **************************************************/
static inline int16_t
dhl_fpga_tx_pkts(uint8_t card_id, uint16_t tx_engine_id,
		struct rte_mbuf ** tx_pkts, uint16_t nb_pkts)
{
	struct dhl_fpga_dev * dev = &dhl_fpga_devices[card_id];

	return (*dev->tx_pkts)(dev, tx_engine_id, tx_pkts, nb_pkts);
}

static inline int16_t
dhl_fpga_rx_pkts(uint8_t card_id, uint16_t rx_engine_id,
		struct rte_mbuf ** rx_pkts, const uint16_t nb_pkts)
{
	struct dhl_fpga_dev * dev = &dhl_fpga_devices[card_id];

	int16_t nb_rx = (*dev->rx_pkts)(dev, rx_engine_id, rx_pkts, nb_pkts);
	return nb_rx;
}


static inline void
dhl_mbuf_add_timestamp(struct rte_mbuf * pkt) {
	uint64_t * buffer;

	buffer = rte_pktmbuf_mtod(pkt, uint64_t *);
	*buffer = rte_rdtsc();
}

static inline void
dhl_mbuf_calc_latency(struct rte_mbuf * pkt) {
	uint64_t * buffer;
	uint64_t latency;

	buffer = rte_pktmbuf_mtod(pkt, uint64_t *);
	if(*buffer == 0)
		latency = 0;
	else
		latency = rte_rdtsc() - (*buffer);

	*buffer = latency;
}



#ifdef __cplusplus
}
#endif


#endif /* LIBdhl_fpga_H_*/
