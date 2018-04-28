/*
 * vc709_pci_uio.h
 *
 *  Created on: Jul 3, 2017
 *      Author: root
 */

#ifndef VC709_PCI_UIO_H_
#define VC709_PCI_UIO_H_

#include "vc709_fpga.h"
/*
 * Return the uioX char device used for a pci device. On success, return
 * the UIO number and fill dstbuf string with the path of the device in
 * sysfs. On error, return a negative value. In this case dstbuf is
 * invalid.
 */

int vc709_uio_alloc_bd_resource(struct rte_pci_device *dev,
		struct vc709_mapped_kernel_resource * bd_uio_res);

void * pci_find_max_end_va1(void);

int vc709_uio_map_bd_resource_by_index(struct vc709_mapped_kernel_resource * bd_res,
		uint64_t phys_addr, size_t len, int map_num, int map_idx);

int vc709_uio_map_bd_memeory(struct rte_pci_device * dev,
		struct vc709_mapped_kernel_resource * bd_res);

#endif /* VC709_PCI_UIO_H_ */
