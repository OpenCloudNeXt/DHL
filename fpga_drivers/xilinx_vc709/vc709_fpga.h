/*
 * vc709_fpga.h
 *
 *  Created on: Jun 19, 2017
 *      Author: root
 */

#ifndef VC709_FPGA_H_
#define VC709_FPGA_H_

#include "../../fpga_driver/xilinx_vc709/vc709_type.h"
#include "../../fpga_drivers/xilinx_vc709/dma/xdma.h"

#define PCI_VENDOR_ID_DMA   0x10EE      /**< Vendor ID - Xilinx */
#define PCI_DEVICE_ID_DMA   0x7083      /**< Xilinx's Device ID */

/** Engine bitmask is 64-bit because there are 64 engines */
#define DMA_ENGINE_PER_SIZE     0x100   /**< Separation between engine regs */
#define DMA_OFFSET              0       /**< Starting register offset */
/**< Size of DMA engine reg space */
#define DMA_SIZE                (MAX_DMA_ENGINES * DMA_ENGINE_PER_SIZE)

/**
 * Default S2C and C2S descriptor ring size.
 * BD Space needed is (DMA_BD_CNT*sizeof(Dma_Bd)).
 */


/* Structures to store statistics - the latest 100 */
#define MAX_STATS   100

#define TX_UTIL_BC               0x900c /* Transmit Utilization Byte Count Register */
#define RX_UTIL_BC               0x9010 /* Receive Utilization Byte Count Register */
#define UPSTR_MEMWR_BC           0x9014 /* Upstream Memory Write Byte Count Register */
#define DOWNSTR_COMPBYTE_COUNTR  0x9018 /* Downstream Completion Byte Count Register */
#define MInitFCCplD              0x901c /* Initial Completion Data Credits for Downstream Port*/
#define MInitFCCplH              0x9020 /* Initial Completion Header Credits for Downstream Port*/
#define MInitFCNPD               0x9024 /* Initial NPD Credits for Downstream Port */
#define MInitFCNPH               0x9028 /* Initial NPH Credits for Downstream Port */
#define MInitFCPD                0x902c /* Initial PD Credits for Downstream Port */
#define MInitFCPH                0x9030 /* Initial PH Credits for Downstream Port */
#define PCIE_DESIGN_VERSION      0x9000

#define PCIE_CAP_REG      0x9034
#ifdef PM_SUPPORT
#define PCIE_CTRL_REG     0x9038
#define PCIE_STS_REG      0x903c

#define PVTMON_VCCINT     0x9040
#define PVTMON_VCCAUX     0x9044
#define PVTMON_VCC3V3     0x9048
#define PVTMON_VADJ       0x904C
#define PVTMON_VCC2V5     0x9050
#define PVTMON_VCC1V5     0x9054
#define PVTMON_MGT_AVCC   0x9058
#define PVTMON_MGT_AVTT   0x905C
#define PVTMON_VCCAUX_IO  0x9060
#define PVTMON_VCC_BRAM   0x9064
#define PVTMON_MGT_VCCAUX 0x9068
#define PVTMON_RSVD       0x906C
#define DIE_TEMP          0x9070
#endif

extern uint64_t clock_hz;

struct vc709_dma_engine {
	u64 engineMask;
	Dma_Engine Dma[MAX_DMA_ENGINES];

	u16 Dma_Bd_nums[MAX_DMA_ENGINES];
	int scal_val[MAX_DMA_ENGINES];

	u32 RawTestMode[MAX_DMA_ENGINES];
};

struct vc709_bar {
	/** BAR information discovered on probe. BAR0 is understood by this driver.
				 * Other BARs will be used as app. drivers register with this driver.
				 */
	u32 barMask;                    /**< Bitmask for BAR information */
	struct {
		unsigned long basePAddr;    /**< Base address of device memory */
		unsigned long baseLen;      /**< Length of device memory */
		void * baseVAddr;   /**< VA - mapped address */
	} barInfo[MAX_BARS];
	u8 * barBase;
};

struct PCIE_stat {
	float Transmit;
	float Receive;
};

struct DMA_stat{
	int engine;
	float throughput;
	float active_time;
	float wait_time;
};

struct ENGINE_state{
    int Engine;                 /**< Engine Number */
    int BDs;                    /**< Total Number of BDs */
    int BDerrs;                 /**< Total BD errors */
    int BDSerrs;                /**< Total BD short errors - only TX BDs */
    unsigned int TestMode;      /**< Current Test Mode */
};

struct vc709_statics {
	struct PCIE_stat pci_s;
	struct DMA_stat dma_s[8];
	struct ENGINE_state engine_s[8];

	PowerMonitorVal pmval;
};


#define VC709_MAX_DMA_COHERENT_RESOUCE 4

struct vc709_mapped_kernel_resource {
	struct rte_pci_addr pci_addr;
	char path[PATH_MAX];		/*  /dev/uioN  */
	char uio_path[PATH_MAX];	/*  /sys/bus/pci/devices/$pci_addr/uio/uioN  */
	int nb_maps;				/* the number of dma coherent address needs to be mapped */
	struct pci_map maps[VC709_MAX_DMA_COHERENT_RESOUCE];	/* records teh coherent address that needs to be mmap to userspace */
};


/*
 * Structure to sotre private data for each driver instance (for each port).
 */
struct vc709_adapter {
	struct vc709_hw hw;
	struct vc709_dma_engine dmaEngine;
	struct vc709_bar bar;
	struct vc709_statics statics;
	struct vc709_mapped_kernel_resource bd_res;
	struct rte_mempool * encap_pool;			/**< mbuf pool to populate encapsulated ring */
	u32 DriverState;
};


#define VC709_DEV_PRIVATE_TO_HW(adapter)\
	(&((struct vc709_adapter *)adapter)->hw)

#define VC709_DEV_PRIVATE_TO_DMAENGINE(adapter)\
	(&((struct vc709_adapter *)adapter)->dmaEngine)

#define VC709_DEV_PRIVATE_TO_BAR(adapter)\
	(&((struct vc709_adapter *)adapter)->bar)

#define VC709_DEV_PRIVATE_TO_STATICS(adapter)\
	(&((struct vc709_adapter *)adapter)->statics)

#define VC709_DEV_PRIVATE_TO_BDRES(adapter)\
	(&((struct vc709_adapter *)adapter)->bd_res)

#define VC709_DEV_PRIVATE_TO_ENCAP_POOL(adapter)\
	((struct vc709_adapter *)adapter)->encap_pool


/*
 * RX/TX function prototypes
 */

int vc709_dev_setup_encap_pkts_pool(struct dhl_fpga_dev *dev,
		uint32_t nb_dma_bd,
		unsigned int socket_id);

int vc709_dev_dma_engine_setup(struct dhl_fpga_dev *dev,
			 uint16_t engine_idx,
			 uint16_t nb_dma_bd,
			 unsigned int socket_id,
			 struct rte_mempool *mp);

int vc709_dev_dma_engine_release(struct dhl_fpga_dev *dev,
	    uint16_t engine_id);

int vc709_get_stats(struct dhl_fpga_dev * fpga_dev, struct dhl_fpga_stats * stats, uint16_t num);


#endif /* VC709_FPGA_H_ */
