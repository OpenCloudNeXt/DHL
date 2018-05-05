#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>
#include <linux/io.h>
#include <linux/msi.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#ifdef PM_SUPPORT
#include <linux/pm.h>
#endif


#define DMA_BD_SW_NUM_WORDS	16
#define DMA_BD_MINIMUM_ALIGNMENT    0x40

#define NB_BDS	4096

#ifdef DEBUG_VERBOSE /* Enable both normal and verbose logging */
#define log_verbose(args...)    printk(args)
#define log_normal(args...)     printk(args)
#define calmshow(args...)	printk(args)
#else
#define log_normal(x...)
#define log_verbose(x...)
#define calmshow(args...)
#endif
/** @name Macros for PCI probing
 * @{
 */
#define PCI_VENDOR_ID_DMA   0x10EE      /**< Vendor ID - Xilinx */
#define PCI_DEVICE_ID_DMA   0x7083      /**< Xilinx's Device ID */

/** PCI device structure which probes for targeted design */
static struct pci_device_id ids[] = {
	{ PCI_VENDOR_ID_DMA,    PCI_DEVICE_ID_DMA,
		PCI_ANY_ID,               PCI_ANY_ID,
		0,            0,          0UL },
	{ }     /* terminate list with empty entry */
};

/**
 * Macro to export pci_device_id to user space to allow hot plug and
 * module loading system to know what module works with which hardware device
 */
MODULE_DEVICE_TABLE(pci, ids);


/**
 * A structure describing the private information for a uio device.
 */
struct xilinx_uio_pci_dev {
	struct uio_info info;
	struct pci_dev *pdev;
//	enum rte_intr_mode mode;
};


/* Remap pci resources described by bar #pci_bar in uio resource n. */
static int
vc709_uio_pci_setup_iomem(struct pci_dev *dev, struct uio_info *info,
		       int n, int pci_bar, const char *name)
{
	unsigned long addr, len;
	void *internal_addr;

	if (n >= ARRAY_SIZE(info->mem))
		return -EINVAL;

	addr = pci_resource_start(dev, pci_bar);
	len = pci_resource_len(dev, pci_bar);
	if (addr == 0 || len == 0)
		return -1;
	internal_addr = ioremap(addr, len);
	if (internal_addr == NULL)
		return -1;
	info->mem[n].name = name;
	info->mem[n].addr = addr;
	info->mem[n].internal_addr = internal_addr;
	info->mem[n].size = len;
	info->mem[n].memtype = UIO_MEM_PHYS;
	calmshow(KERN_INFO "BAR %d VAddr is %p, PAddr is %lx",
			n, internal_addr, addr);
	return 0;
}

/* Get pci port io resources described by bar #pci_bar in uio resource n. */
static int
vc709_uio_pci_setup_ioport(struct pci_dev *dev, struct uio_info *info,
		int n, int pci_bar, const char *name)
{
	unsigned long addr, len;

	if (n >= ARRAY_SIZE(info->port))
		return -EINVAL;

	addr = pci_resource_start(dev, pci_bar);
	len = pci_resource_len(dev, pci_bar);
	if (addr == 0 || len == 0)
		return -EINVAL;

	info->port[n].name = name;
	info->port[n].start = addr;
	info->port[n].size = len;
	info->port[n].porttype = UIO_PORT_X86;

	return 0;
}

/* Unmap previously ioremap'd resources */
static void
vc709_uio_pci_release_iomem(struct uio_info *info)
{
	int i;

	for (i = 0; i < MAX_UIO_MAPS; i++) {
		if (info->mem[i].internal_addr)
			iounmap(info->mem[i].internal_addr);
	}
}

static void
vc709_uio_pci_release_bd_iomem(struct pci_dev *pdev, struct uio_info *info)
{
	int i;

	for(i = 0; i < 4; i++) {
		if (info->mem[i+1].internal_addr)
			dma_free_coherent(&pdev->dev, info->mem[i+1].size,
							info->mem[i+1].internal_addr,
							info->mem[i+1].addr);
	}
}


static int
vc709_uio_setup_bars(struct pci_dev *dev, struct uio_info *info)
{
	int i, iom, iop, ret;
	unsigned long flags;
	static const char *bar_names[PCI_STD_RESOURCE_END + 1]  = {
		"BAR0",
		"BAR1",
		"BAR2",
		"BAR3",
		"BAR4",
		"BAR5",
	};

	iom = 0;
	iop = 0;

	for (i = 0; i < ARRAY_SIZE(bar_names); i++) {
		if (pci_resource_len(dev, i) != 0 &&
				pci_resource_start(dev, i) != 0) {
			flags = pci_resource_flags(dev, i);
			if (flags & IORESOURCE_MEM) {
				ret = vc709_uio_pci_setup_iomem(dev, info, iom,
							     i, bar_names[i]);
				if (ret != 0)
					return ret;
				iom++;
			} else if (flags & IORESOURCE_IO) {
				ret = vc709_uio_pci_setup_ioport(dev, info, iop,
							      i, bar_names[i]);
				if (ret != 0)
					return ret;
				iop++;
			}
		}
	}

	return (iom != 0 || iop != 0) ? ret : -ENOENT;
}


/* Remap pci resources described by bar #pci_bar in uio resource n. */
static int
vc709_uio_pci_setup_bd_iomem(struct pci_dev *dev, struct uio_info *info,
		       int n, const char *name,
			   dma_addr_t addr, void * internal_addr, unsigned long len)
{

	if (n >= ARRAY_SIZE(info->mem))
		return -EINVAL;

	if (addr == 0 )
		return -1;
	if (internal_addr == NULL)
		return -1;
	info->mem[n].name = name;
	info->mem[n].addr = addr;
	info->mem[n].internal_addr = internal_addr;
	info->mem[n].size = len;
	info->mem[n].memtype = UIO_MEM_PHYS;
	return 0;
}

/*****************************************************************************/
/**
 * This function initializes the DMA BD ring as follows -
 * - Calculates the space required by the DMA BD ring
 * - Allocates the space, and aligns it as per DMA engine requirement
 * - Creates the BD ring structure in the allocated space
 * - If it is a RX DMA engine, allocates buffers from the user driver, and
 *   associates each BD in the RX BD ring with a buffer
 *
 * @param  pdev is the PCI/PCIe device instance
 * @param  eptr is a pointer to the DMA engine instance to be worked on.
 *
 * @return 0 if successful
 * @return negative value if unsuccessful
 *
 *****************************************************************************/
int descriptor_init(struct pci_dev *pdev, uint16_t nb_dma_bd, struct uio_info *info)
{
	unsigned long dftsize;
	int ret, i;

	dma_addr_t BdPhyAddr;
	void * BdPtr;

	static char *engine_BD_names[4]  = {
		"dma_engine0",
		"dma_engine1",
		"dma_engine2",
		"dma_engine3",
	};

	/* Calculate size of descriptor space pool - extra to allow for
	 * alignment adjustment.
	 */
	dftsize = sizeof(u32) * DMA_BD_SW_NUM_WORDS * (nb_dma_bd);
	calmshow(KERN_INFO "XDMA: BD space: %ld (0x%0lx)\n", dftsize, dftsize);

	for(i = 0 ; i < 4; i++){
		if( (BdPtr = dma_alloc_coherent(&pdev->dev, dftsize, &BdPhyAddr,
				GFP_KERNEL)) == NULL )
		{
			printk(KERN_ERR "BD ring %d pci_alloc_consistent() failed\n",i);
			return -1;
		}

		calmshow(KERN_INFO "sizeof(dma_addr_t) is %lu, sizeof(BdPhyAddr) is %lu \n",
					sizeof(dma_addr_t), sizeof(BdPhyAddr) );
		calmshow(KERN_INFO "BdPhyAddr is %lx, \"(unsigned int)BdPhyAddr\" is %lx \n",
					BdPhyAddr ,(unsigned int)BdPhyAddr);

		log_normal(KERN_INFO "BD ring %d space allocated from %p, PA %lx\n", i,
				BdPtr, (unsigned long)BdPhyAddr);

		ret = vc709_uio_pci_setup_bd_iomem(pdev, info,
				i+1, engine_BD_names[i],
				BdPhyAddr, BdPtr, dftsize);
		if (ret != 0)
			return ret;

	}

	return 0;
}

static int
vc709_uio_mmap(struct uio_info *info, struct vm_area_struct *vma)
{
	dma_addr_t PAddr;

//	printk("offset is %lx\n", vma->vm_pgoff);

	if(vma->vm_pgoff > 4){
		printk("offset is larger than 4!\n");
		return -EINVAL;
	}
	PAddr = info->mem[vma->vm_pgoff].addr;

//	void * VAddr;
//	VAddr = info->mem[vma->vm_pgoff].internal_addr;
//	printk("PAddr is 0x%lx, virt_to_phys(Addr) is 0x%lx \n",PAddr,virt_to_phys(VAddr));

	if(remap_pfn_range(vma, vma->vm_start,
				PAddr>>PAGE_SHIFT,
					vma->vm_end -vma->vm_start,
					vma->vm_page_prot))
				return -EAGAIN;

	return 0;
}



#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit
#else
static int
#endif
vc709_uio_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct xilinx_uio_pci_dev * udev;
	int err;
//	dma_addr_t map_dma_addr;
//	void * map_addr;

	udev = kzalloc(sizeof(struct xilinx_uio_pci_dev), GFP_KERNEL);
	if (!udev)
		return -ENOMEM;

	/*
	 * enable device: ask low-level code to enable I/O and
	 * memory
	 */
	err = pci_enable_device(pdev);
	if (err != 0) {
		printk(KERN_ERR "PCI device enable failed.\n");
		goto fail_free;
	}

	/* enable bus mastering on the device */
	pci_set_master(pdev);

	/* remap IO memory */
	err = vc709_uio_setup_bars(pdev, &udev->info);
	if (err != 0)
		goto fail_release_iomem;

	/* set 64-bit DMA mask */
	err = pci_set_dma_mask(pdev,  DMA_BIT_MASK(64));
	if (err != 0) {
		printk(KERN_INFO "Cannot set DMA mask\n");
		goto fail_release_iomem;
	}

	err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
	if (err != 0) {
		printk(KERN_INFO "Cannot set consistent DMA mask\n");
		goto fail_release_iomem;
	}

	err = descriptor_init(pdev, NB_BDS, &udev->info);
	if (err != 0)
		goto fail_realese_bd_iomem;

	/* fill uio infos */
	udev->info.name = "vc709_uio";
	udev->info.version = "0.1";
//	udev->info.handler = igbuio_pci_irqhandler;
//	udev->info.irqcontrol = igbuio_pci_irqcontrol;
	udev->info.irq = UIO_IRQ_NONE;
	udev->info.priv = udev;
	udev->pdev = pdev;

	udev->info.mmap = &vc709_uio_mmap;



	/* register uio driver */
	err = uio_register_device(&pdev->dev, &udev->info);
	if (err != 0)
		goto fail_realese_bd_iomem;
//		goto fail_release_iomem;

	pci_set_drvdata(pdev, udev);

	dev_info(&pdev->dev, "uio device registered with irq %lx\n",
		 udev->info.irq);

	/*
	 * Doing a harmless dma mapping for attaching the device to
	 * the iommu identity mapping if kernel boots with iommu=pt.
	 * Note this is not a problem if no IOMMU at all.
	 */


//	map_addr = dma_alloc_coherent(&pdev->dev, 1024, &map_dma_addr,
//			GFP_KERNEL);
//	if (map_addr)
//		memset(map_addr, 0, 1024);
//
//	if (!map_addr)
//		dev_info(&pdev->dev, "dma mapping failed\n");
//	else {
//		dev_info(&pdev->dev, "mapping 1K dma=%#llx host=%p\n",
//			 (unsigned long long)map_dma_addr, map_addr);
//
//		dma_free_coherent(&pdev->dev, 1024, map_addr, map_dma_addr);
//		dev_info(&pdev->dev, "unmapping 1K dma=%#llx host=%p\n",
//			 (unsigned long long)map_dma_addr, map_addr);
//	}

	return 0;

fail_realese_bd_iomem:
	vc709_uio_pci_release_bd_iomem(pdev, &udev->info);

fail_release_iomem:
	vc709_uio_pci_release_iomem(&udev->info);
	pci_disable_device(pdev);
fail_free:
	kfree(udev);

	return err;
}

static void
vc709_uio_pci_remove(struct pci_dev * pdev)
{
	struct xilinx_uio_pci_dev *udev = pci_get_drvdata(pdev);

	uio_unregister_device(&udev->info);
	vc709_uio_pci_release_bd_iomem(pdev, &udev->info);

	vc709_uio_pci_release_iomem(&udev->info);


//	if (udev->mode == RTE_INTR_MODE_MSIX)
//			pci_disable_msix(dev);

	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);

	dev_info(&pdev->dev, "remove UIO driver in kernel.\n");
	kfree(udev);
}

static struct pci_driver vc709_uio_pci_driver = {
		/* Initialy we named this uio driver as "vc709_uio". When performing the rte_eal_init(), there is one step
		 * that calls the rte_bus_scan(). When scanning the pci bus in rte_bus_scan(), it calls pci_scan_one("/sys/bus/pci/devices", addr)
		 * to scan all the pci devices in the system, creates a "struct rte_pci_device" for each pci device, set the field of "rte_pci_device->kdrv"
		 * based on the driver name, which is exactly "vc709_uio" when loading this uio driver into kernel and it matches the vc709 in pci bus.
		 *
		 * In order to make the DPDK EAL probe Xilinx vc709 board properly, we need to add one type of "enum rte_kernel_driver" named
		 * RTE_KDRV_VC709_UIO in "rte_bus_pci.h", and accordingly add some few codes to make it correctly probe the vc709 board when performing
		 * rte_pci_map_device() & rte_pci_unmap_device() to map/unmap the device resources into/from userspace.
		 *
		 * However, the initial method needs to modify the source code of DPDK.
		 * In order not to issue any modification, we take a tricky that naming this uio driver as "uio_pci_generic", so that it will
		 * set the rte_pci_device->kdrv as RTE_KDRV_UIO_GENERIC when scanning at vc709 board.
		 *
		 * Because that the DMA engines ues the 32-bits dma address and the DPDK returns the 64-bits physical address in user space,
		 * we need this uio driver to allocate the io memory for buffer descriptors of the DMA engines in descriptor_init(),
		 * and stores the address space of them in "struct uio_info->mem[]" for the vc709_pmd_driver to use.
		 *
		 * When we update the DMA IP core in FPGA to support 64-bits DMA address, we can use the igb_uio driver instead of this.
		 */
//		.name = "vc709_uio",
		.name = "uio_pci_generic",
		.id_table = ids,
		.probe = vc709_uio_pci_probe,
		.remove = vc709_uio_pci_remove,
};

static int __init
vc709_uio_init(void)
{
	printk(KERN_INFO "vc709_uio: insert VC709 UIO DMA driver in kernel.\n");
	return pci_register_driver(&vc709_uio_pci_driver);
}

static void __exit
vc709_uio_exit(void)
{
	pci_unregister_driver(&vc709_uio_pci_driver);
}

module_init(vc709_uio_init);
module_exit(vc709_uio_exit);

MODULE_DESCRIPTION("UIO driver for Xilinx VC709 board");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Calm @HUST SIGGC");
MODULE_VERSION("1.0");
