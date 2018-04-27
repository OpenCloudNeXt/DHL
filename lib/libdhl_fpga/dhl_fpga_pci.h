/***********************************************************************

							dhl_fpga_pci.h

***********************************************************************/
#ifndef LIB_LIBDHL_FPGA_DHL_FPGA_PCI_H_
#define LIB_LIBDHL_FPGA_DHL_FPGA_PCI_H_

#include <rte_malloc.h>
#include <rte_pci.h>
#include <rte_eal.h>

#include "dhl_fpga.h"

/* the callback function for a FPGA drivers*/
typedef int (*fpga_dev_pci_callback_t)(struct dhl_fpga_dev *fpga_dev);

/**
 * Copy pci device info to the FPGA device data.
 *
 * @param eth_dev
 * The *fpga_dev* pointer is the address of the *dhl_fpga_dev* structure.
 * @param pci_dev
 * The *pci_dev* pointer is the address of the *rte_pci_device* structure.
 *
 * @return
 *   - 0 on success, negative on error
 */
static inline void
dhl_fpga_copy_pci_info(struct dhl_fpga_dev *fpga_dev,
	struct rte_pci_device *pci_dev)
{
	if ((fpga_dev == NULL) || (pci_dev == NULL)) {
		RTE_PMD_DEBUG_TRACE("NULL pointer fpga_dev=%p pci_dev=%p\n",
				fpga_dev, pci_dev);
		return;
	}

	fpga_dev->intr_handle = &pci_dev->intr_handle;

	fpga_dev->data->dev_flags = 0;

	fpga_dev->data->kdrv = pci_dev->kdrv;
	fpga_dev->data->numa_node = pci_dev->device.numa_node;
	fpga_dev->data->drv_name = pci_dev->driver->driver.name;
}

/**
 * @internal
 * Allocates a new fpga_dev slot for an fpga card device and returns the pointer
 * to that slot for the driver to use.
 *
 * @param dev
 *	Pointer to the PCI device
 *
 * @param private_data_size
 *	Size of private data structure
 *
 * @return
 *	A pointer to a dhl_fpga_dev or NULL if allocation failed.
 */
static inline struct dhl_fpga_dev *
dhl_fpga_dev_pci_allocate(struct rte_pci_device *dev, size_t private_data_size)
{
	struct dhl_fpga_dev *fpga_dev;
	const char *name;

	if (!dev)
		return NULL;

	name = dev->device.name;

	if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
		fpga_dev = dhl_fpga_dev_allocate(name);
		if (!fpga_dev)
			return NULL;

		if (private_data_size) {
			fpga_dev->data->dev_private = rte_zmalloc_socket(name,
				private_data_size, RTE_CACHE_LINE_SIZE,
				dev->device.numa_node);
			if (!fpga_dev->data->dev_private) {
				dhl_fpga_dev_release_card(fpga_dev);
				return NULL;
			}
		}
	} else {
		fpga_dev = dhl_fpga_dev_attach_secondary(name);
		if (!fpga_dev)
			return NULL;
	}

	fpga_dev->device = &dev->device;
	fpga_dev->intr_handle = &dev->intr_handle;
	dhl_fpga_copy_pci_info(fpga_dev, dev);
	return fpga_dev;
}

static inline void
dhl_fpga_dev_pci_release(struct dhl_fpga_dev *fpga_dev)
{
	/* free fpga device */
	dhl_fpga_dev_release_card(fpga_dev);

	if (rte_eal_process_type() == RTE_PROC_PRIMARY)
		rte_free(fpga_dev->data->dev_private);

	fpga_dev->data->dev_private = NULL;

	fpga_dev->device = NULL;
	fpga_dev->intr_handle = NULL;
}

/**
 * @internal
 * Wrapper for use by dhl fpga drivers in a .probe function to attach to a fpgadev
 * interface.
 */
static inline int
dhl_fpga_dev_pci_generic_probe(struct rte_pci_device *pci_dev,
	size_t private_data_size, fpga_dev_pci_callback_t dev_init)
{
	struct dhl_fpga_dev *fpga_dev;
	int ret;

	fpga_dev = dhl_fpga_dev_pci_allocate(pci_dev, private_data_size);
	if (!fpga_dev)
		return -ENOMEM;

	RTE_FUNC_PTR_OR_ERR_RET(*dev_init, -EINVAL);
	ret = dev_init(fpga_dev);
	if (ret)
		dhl_fpga_dev_pci_release(fpga_dev);

	return ret;
}

/**
 * @internal
 * Wrapper for use by pci drivers in a .remove function to detach a fpgadev
 * interface.
 */
static inline int
dhl_fpga_dev_pci_generic_remove(struct rte_pci_device *pci_dev,
	fpga_dev_pci_callback_t dev_uninit)
{
	struct dhl_fpga_dev *fpga_dev;
	int ret;

	fpga_dev = dhl_fpga_dev_allocated(pci_dev->device.name);
	if (!fpga_dev)
		return -ENODEV;

	if (dev_uninit) {
		ret = dev_uninit(fpga_dev);
		if (ret)
			return ret;
	}

	dhl_fpga_dev_pci_release(fpga_dev);
	return 0;
}

#endif /* LIB_LIBDHL_FPGA_DHL_FPGA_PCI_H_ */
