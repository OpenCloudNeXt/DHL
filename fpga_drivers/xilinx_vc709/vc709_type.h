/*
 * vc709_type.h
 *
 *  Created on: Jun 20, 2017
 *      Author: root
 */

#ifndef BASE_VC709_TYPE_H_
#define BASE_VC709_TYPE_H_

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <rte_common.h>
#include <rte_debug.h>
#include <rte_cycles.h>
#include <rte_log.h>
#include <rte_byteorder.h>
#include <rte_io.h>

#define ASSERT(x) if(!(x)) rte_panic("VC709: x")

#define DELAY(x) rte_delay_us(x)
#define usec_delay(x) DELAY(x)
#define msec_delay(x) DELAY(1000*(x))

#define FALSE               0
#define TRUE                1

#define false               0
#define true                1
#define min(a,b)	RTE_MIN(a,b)


#define STATIC static
#define VC709_NTOHL(_i)	rte_be_to_cpu_32(_i)
#define VC709_NTOHS(_i)	rte_be_to_cpu_16(_i)
#define VC709_CPU_TO_LE16(_i)  rte_cpu_to_le_16(_i)
#define VC709_CPU_TO_LE32(_i)  rte_cpu_to_le_32(_i)
#define VC709_LE32_TO_CPU(_i)  rte_le_to_cpu_32(_i)
#define VC709_LE32_TO_CPUS(_i) rte_le_to_cpu_32(_i)
#define VC709_CPU_TO_BE16(_i)  rte_cpu_to_be_16(_i)
#define VC709_CPU_TO_BE32(_i)  rte_cpu_to_be_32(_i)
#define VC709_BE32_TO_CPU(_i)  rte_be_to_cpu_32(_i)

typedef uint8_t		u8;
typedef int8_t		s8;
typedef uint16_t	u16;
typedef int16_t		s16;
typedef uint32_t	u32;
typedef int32_t		s32;
typedef uint64_t	u64;
#ifndef __cplusplus
typedef int		bool;
#endif

#define mb()	rte_mb()
#define wmb()	rte_wmb()
#define rmb()	rte_rmb()

#define IOMEM

#define prefetch(x) rte_prefetch0(x)

#define VC709_PCI_REG(reg) rte_read32(reg)

static inline uint32_t vc709_read_addr(volatile void* addr)
{
	return rte_le_to_cpu_32(VC709_PCI_REG(addr));
}

#define VC709_PCI_REG_WRITE(reg, value)			\
	rte_write32((rte_cpu_to_le_32(value)), reg)

#define VC709_PCI_REG_WRITE_RELAXED(reg, value)		\
	rte_write32_relaxed((rte_cpu_to_le_32(value)), reg)

#define VC709_PCI_REG_ADDR(hw, reg) \
	((volatile uint32_t *)((char *)(hw)->hw_addr + (reg)))

#define VC709_PCI_REG_ARRAY_ADDR(hw, reg, index) \
	VC709_PCI_REG_ADDR((hw), (reg) + ((index) << 2))

#define VC709_WRITE_FLUSH(a) VC709_READ_REG(a, VC709_STATUS)

#define VC709_READ_REG(hw, reg) \
	vc709_read_addr(VC709_PCI_REG_ADDR((hw), (reg)))

#define VC709_WRITE_REG(hw, reg, value) \
	VC709_PCI_REG_WRITE(VC709_PCI_REG_ADDR((hw), (reg)), (value))

/*
 * calm create
 */

/*
#define Dma_mReadReg(baseaddr, offset) \
	vc709_read_addr((volatile uint32_t *) (baseaddr + offset))

#define Dma_mWriteReg(baseaddr, offset, value) \
	VC709_PCI_REG_WRITE( ((volatile uint32_t *) (baseaddr + offset)), (value))
*/


#define VC709_READ_REG_ARRAY(hw, reg, index) \
	VC709_PCI_REG(VC709_PCI_REG_ARRAY_ADDR((hw), (reg), (index)))

#define VC709_WRITE_REG_ARRAY(hw, reg, index, value) \
	VC709_PCI_REG_WRITE(VC709_PCI_REG_ARRAY_ADDR((hw), (reg), (index)), (value))



/* PCI bus types */
enum vc709_bus_type {
	vc709_bus_type_unknown = 0,
	vc709_bus_type_pci,
	vc709_bus_type_pcix,
	vc709_bus_type_pci_express,
	vc709_bus_type_internal,
	vc709_bus_type_reserved
};

/* PCI bus speeds */
enum vc709_bus_speed {
	vc709_bus_speed_unknown	= 0,
	vc709_bus_speed_33	= 33,
	vc709_bus_speed_66	= 66,
	vc709_bus_speed_100	= 100,
	vc709_bus_speed_120	= 120,
	vc709_bus_speed_133	= 133,
	vc709_bus_speed_2500	= 2500,
	vc709_bus_speed_5000	= 5000,
	vc709_bus_speed_8000	= 8000,
	vc709_bus_speed_reserved
};

/* PCI bus widths */
enum vc709_bus_width {
	vc709_bus_width_unknown	= 0,
	vc709_bus_width_pcie_x1	= 1,
	vc709_bus_width_pcie_x2	= 2,
	vc709_bus_width_pcie_x4	= 4,
	vc709_bus_width_pcie_x8	= 8,
	vc709_bus_width_32	= 32,
	vc709_bus_width_64	= 64,
	vc709_bus_width_reserved
};

/* Bus parameters */
struct vc709_bus_info {
	enum vc709_bus_speed speed;
	enum vc709_bus_width width;
	enum vc709_bus_type type;

	u16 func;
	u8 lan_id;
	u16 instance_id;
};



struct vc709_hw {
	u8 IOMEM *hw_addr;
	struct vc709_bus_info bus;
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	u8 revision_id;
	bool adapter_stopped;
	int api_version;
};

#endif /* BASE_VC709_TYPE_H_ */
