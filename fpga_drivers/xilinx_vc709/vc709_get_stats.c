/*
 * vc709_get_statics.c
 *
 *  Created on: Jul 4, 2017
 *      Author: root
 */
#include "../../fpga_drivers/xilinx_vc709/vc709_get_stats.h"

#include <rte_common.h>
#include <dhl_fpga.h>

#include "../../fpga_driver/xilinx_vc709/dma/xdma_user.h"
#include "../../fpga_driver/xilinx_vc709/dma/xpmon_be.h"
#include "../../fpga_drivers/xilinx_vc709/dma/xdma.h"
#include "../../fpga_drivers/xilinx_vc709/dma/xdma_hw.h"
#include "../../fpga_drivers/xilinx_vc709/vc709_fpga.h"
#include "../../fpga_drivers/xilinx_vc709/vc709_log.h"

#define MULTIPLIER  8
#define DIVISOR     (1024*1024*1024)    /* Graph is in Gbits/s */

struct
{
    int Engine;         /* Engine number - for communicating with driver */
    //char *name;         /* Name to be used in Setup screen */
    //int mode;           /* TX/RX - incase of specific screens */
} DMAConfig[MAX_ENGS] =
{
    {0/*, LABEL1, TX_MODE*/ },
    {32/*, LABEL1, RX_MODE*/ },
    {1/*, LABEL2, TX_MODE*/ },
    {33/*, LABEL2, RX_MODE*/ },
    {2},
    {34},
    {3},
    {35}
};


//static int vc709_read_pci_state(struct rte_fpga_dev * fpga_dev, PCIState * pcistate)
//{
//	int pos;
//	u16 valw;
//	u8 valb;
//	u16 link_speed_sta;
//	u16 link_cap2_speed;
//
//	int reg=0,linkUpCap=0;
//	u64 base;
//	base = (dmaData->barInfo[0].baseVAddr);
//
//	/* Since probe has succeeded, indicates that link is up. */
//	pcistate->LinkState = LINK_UP;
//	pcistate->VendorId = PCI_VENDOR_ID_DMA;
//	pcistate->DeviceId = PCI_DEVICE_ID_DMA;
//
//	pcistate->IntMode = INT_NONE;
//}

static inline void vc709_get_led_stats(struct dhl_fpga_dev * fpga_dev, LedStats * lstats)
{
	struct vc709_bar * bar;
	int Status_Reg;

	bar = VC709_DEV_PRIVATE_TO_BAR(fpga_dev->data->dev_private);

	Status_Reg = XIo_In32(bar->barBase + STATUS_REG_OFFSET);
	lstats->DdrCalib0 = (Status_Reg >> 30) & 0x1; /* bit 30 'on' of Status Register indicated DDR3-SODMM0 Calibration done */
	lstats->DdrCalib1 = (Status_Reg >> 31) & 0x1;  /* bit 31 'on' of Status Register indicated DDR3-SODMM1 Calibration done */

	lstats->Phy3 = (Status_Reg >> 29) & 0x1;
	lstats->Phy2 = (Status_Reg >> 28) & 0x1;
	lstats->Phy1 = (Status_Reg >> 27) & 0x1;
	lstats->Phy0 = (Status_Reg >> 26) & 0x1;
}

static inline void vc709_get_dma_engine_state(struct dhl_fpga_dev * fpga_dev, struct ENGINE_state * eng)
{
	int i;
	struct vc709_dma_engine * dma_engines;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;

	dma_engines = VC709_DEV_PRIVATE_TO_DMAENGINE(fpga_dev->data->dev_private);
	i = eng->Engine;

	/* First, check if requested engine is valid */
	if( (i >= MAX_DMA_ENGINES) ||
			(!((dma_engines->engineMask) & (1LL << i))) )
		RTE_LOG(ERR, PMD, "Invalid engine %d, when get engine state \n", i);

	eptr = &dma_engines->Dma[i];
	rptr = &(eptr->BdRing);

	/* Now add the DMA state */
	eng->BDs = dma_engines->Dma_Bd_nums[i];
	eng->BDerrs = rptr->BDerrs;
	eng->BDSerrs = rptr->BDSerrs;
}

static inline void vc709_get_dma_stats(struct dhl_fpga_dev * fpga_dev, EngStatsArray * es)
{
	struct vc709_dma_engine * dma_engines;

	dma_engines = VC709_DEV_PRIVATE_TO_DMAENGINE(fpga_dev->data->dev_private);

	Dma_Engine * eptr;
	u32 at, wt, cb;
	DMAStatistics * dma_sta;
	int i;

	for(i = 0; i < MAX_DMA_ENGINES; i++)
	{
		if(!((dma_engines->engineMask) & (1LL << i)))
			continue;

		eptr = &dma_engines->Dma[i];
		dma_sta = &es->engptr[i];

		/* First, read the DMA engine payload registers */
		at = Dma_mReadReg(eptr->RegBase, REG_DMA_ENG_ACTIVE_TIME);
		wt = Dma_mReadReg(eptr->RegBase, REG_DMA_ENG_WAIT_TIME);
		cb = Dma_mReadReg(eptr->RegBase, REG_DMA_ENG_COMP_BYTES);

		dma_sta->Engine = i;
		dma_sta->LAT = 4*(at>>2);
		dma_sta->LWT = 4*(wt>>2);
		dma_sta->LBR = 4*(cb>>2);
		dma_sta->scaling_factor =1 << dma_engines->scal_val[i];
	}

}


static inline void vc709_get_trn_stats(struct dhl_fpga_dev * fpga_dev, TRNStatistics * trn_sta)
{
	struct vc709_bar * bar;
	struct vc709_dma_engine * dma_engines;

	bar = VC709_DEV_PRIVATE_TO_BAR(fpga_dev->data->dev_private);
	dma_engines = VC709_DEV_PRIVATE_TO_DMAENGINE(fpga_dev->data->dev_private);



	u32 t1, t2;
	u8 * base;
	int scal_val;

	base = bar->barBase;
	scal_val = dma_engines->scal_val[0];

	t1 = XIo_In32( base +TX_UTIL_BC );
	t2 = XIo_In32( base+RX_UTIL_BC );

	trn_sta->LTX = 4*(t1>>2);

	trn_sta->LRX = 4*(t2>>2);
	trn_sta->scaling_factor = 1 << scal_val;
}

int vc709_dev_get_stats(struct dhl_fpga_dev * fpga_dev, struct dhl_fpga_stats * stats, uint16_t num)
{
	int i;
	TRNStatistics ts;

	EngStatsArray es;
	DMAStatistics ds[MAX_DMA_ENGINES];

	if(num > 10)
		return -1;

	es.Count = MAX_DMA_ENGINES;
	es.engptr = ds;

	struct vc709_statics * stat;
	stat = VC709_DEV_PRIVATE_TO_STATICS(fpga_dev->data->dev_private);

	vc709_get_trn_stats(fpga_dev, &ts);

	stat->pci_s.Transmit = ((double)(ts.LTX) / DIVISOR) * MULTIPLIER * ts.scaling_factor;
	stat->pci_s.Receive = ((double)(ts.LRX) / DIVISOR) * MULTIPLIER * ts.scaling_factor;

	//RTE_LOG(INFO, PMD,"PCI-E transmit is %f \n",stat->pci_s.Transmit);
	//RTE_LOG(INFO, PMD,"PCI-E receive is %f \n",stat->pci_s.Receive);

	vc709_get_dma_stats(fpga_dev, &es);
	for(i = 0; i < MAX_DMA_ENGINES; i++)
	{
		int k;

		for(k =0 ; k < MAX_ENGS ; k++)
		{
			if(DMAConfig[k].Engine == i)
				break;
		}

		if(k >= MAX_ENGS)	continue;
		stat->dma_s[k].engine = ds[i].Engine;
		stat->dma_s[k].throughput = ((double)(ds[i].LBR) / DIVISOR ) * MULTIPLIER * ds[i].scaling_factor;
		stat->dma_s[k].active_time = ds[i].LAT;
		stat->dma_s[k].wait_time = ds[i].LWT;

//		RTE_LOG(INFO, PMD,"engine %d-%d throughput is %f, active_time is %f, wait_time id %f \n", i, ds[i].Engine,
//				stat->dma_s[k].throughput, stat->dma_s[k].active_time, stat->dma_s[k].wait_time);
	}

	for (i = 0; i< MAX_DMA_ENGINES; i++)
	{
		int k;

		for(k =0 ; k < MAX_ENGS ; k++)
		{
			if(DMAConfig[k].Engine == i)
				break;
		}
		if(k >= MAX_ENGS)	continue;
		stat->engine_s[k].Engine = i;
		vc709_get_dma_engine_state(fpga_dev, &stat->engine_s[k]);
	}


	i = 0;
	stats[i++].throughput = stat->pci_s.Transmit;
	stats[i++].throughput = stat->pci_s.Receive;
	for(;i < 10 ; i++)
	{
		stats[i].throughput = stat->dma_s[i-2].throughput;
	}

	return 0;
}
