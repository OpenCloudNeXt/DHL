/*
 * dh_fpgadev_core.h
 *
 *  Created on: May 7, 2018
 *      Author: lxy
 */

#ifndef LIB_LIBDHL_FPGA_DHL_FPGADEV_CORE_H_
#define LIB_LIBDHL_FPGA_DHL_FPGADEV_CORE_H_

#include "rte_dev.h"

#define DHL_FPGA_NAME_MAX_LEN RTE_DEV_NAME_MAX_LEN

#ifndef DHL_MAX_ENGINE_PER_FPGA
#define DHL_MAX_ENGINE_PER_FPGA 4
#endif

#define DHL_MAX_PR_ACC_NAME_LEN 256
#define DHL_MAX_PR_ACC_NUM	16
/*
 * Definitions of all functions exported by an FPGA driver through the
 * generic structure of type *fpga_dev_ops* supplied in the *dhl_fpga_dev*->dev_ops
 * structure associated with an FPGA device.
 */
struct dhl_fpga_dev;
struct dhl_fpga_dev_data;
struct dhl_fpga_pr_acc;

/**< @internal FPGA device configuration. */
typedef int (*fpga_dev_configure_t)(struct dhl_fpga_dev *dev);

/**< @internal Function used to start a configured FPGA device. */
typedef int  (*fpga_dev_start_t)(struct dhl_fpga_dev *dev);

/**< @internal Function used to stop a configured FPGA device. */
typedef int (*fpga_dev_stop_t)(struct dhl_fpga_dev *dev);

/**< @internal Function used to close a configured FPGA device. */
typedef void (*fpga_dev_close_t)(struct dhl_fpga_dev *dev);

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

/**< @internal Start rx and tx of a queue of an Ethernet device. */
typedef int (*fpga_engine_start_t)(struct dhl_fpga_dev *dev,
				    uint16_t engine_id);

/**< @internal Stop rx and tx of a queue of an Ethernet device. */
typedef int (*fpga_engine_stop_t)(struct dhl_fpga_dev *dev,
				    uint16_t engine_id);

/**< @internal Release memory resources allocated by given RX/TX engine. */
typedef void (*fpga_engine_release_t)(struct dhl_fpga_dev * dev, uint16_t engine_idx);

/** **/
typedef int (*fpga_dev_setup_encap_pkts_pool_t)(struct dhl_fpga_dev *dev, uint32_t nb_dma_bd, unsigned int socket_id);

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

/**
 * @internal
 * The data part, with no function pointers, associated with each FPGA device.
 *
 * This structure is safe to place in shared memory to be common among different
 * processes in a multi-process configuration.
 */
struct dhl_fpga_dev_data {
	char name[DHL_FPGA_NAME_MAX_LEN]; 	/**< Unique identifier name */
	uint8_t card_id;           			/**< Device [external] card identifier. */
	int numa_node;  					/**< NUMA node connection */
	enum rte_kernel_driver kdrv;    	/**< Kernel driver passthrough */
	const char *drv_name;   			/**< Driver name */


	void ** rx_engines;					/**< Array of pointers to DMA rx engines. */
	void ** tx_engines;					/**< Array of pointers to DMA tx engines. */
	uint16_t nb_rx_engines;				/**< Number of RX engines. */
	uint16_t nb_tx_engines; 			/**< Number of TX engines. */

	uint8_t rx_engine_state[DHL_MAX_ENGINE_PER_FPGA];		/** Queues state: STARTED(1) / STOPPED(0) */
	uint8_t tx_engine_state[DHL_MAX_ENGINE_PER_FPGA];		/** Queues state: STARTED(1) / STOPPED(0) */


	struct dhl_fpga_pr_acc pr_acc;

	struct dhl_fpga_conf dev_conf; /**< Configuration applied to device. */

	uint32_t dev_flags; 			/**< Capabilities */
	void *dev_private;              /**< PMD-specific private data */
} __rte_cache_aligned;

/**
 * @internal
 * The partial reconfigurable accelerator partition part, associated with each FPGA data.
 */
struct dhl_fpga_pr_acc {
	uint16_t nb_pf_acc;												/** number of partial reconfiguration accelerator partitions */
	uint16_t pr_acc_id[DHL_MAX_PR_ACC_NUM];							/** id of partial reconfiguration accelerator partitions */
	char pr_acc_name[DHL_MAX_PR_ACC_NUM][DHL_MAX_PR_ACC_NAME_LEN];	/** name of the accelerator modules */
	uint32_t off_set[DHL_MAX_PR_ACC_NUM];							/** offset to the address of base_bar */
	uint32_t base_bar[DHL_MAX_PR_ACC_NUM];							/** base BAR address of the accelerator modules */
	void ** pr_acc_conf[DHL_MAX_PR_ACC_NUM];						/** the pointer of the configuration of accelerator modules */

};
/**
 * @internal
 * The pool of *dhl_fpga_dev* structures. The size of the pool
 * is configured at compile-time in the <dhl_fpga.c> file.
 */
extern struct dhl_fpga_dev dhl_fpga_devices[];

#endif /* LIB_LIBDHL_FPGA_DHL_FPGADEV_CORE_H_ */
