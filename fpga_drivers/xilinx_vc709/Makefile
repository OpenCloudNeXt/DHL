#
#   calm create
#


include $(RTE_SDK)/mk/rte.vars.mk

#
# library name
#
LIB = libdhl_pmd_xilinx_vc709.a

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

#EXPORT_MAP := rte_pmd_vc709_version.map

LIBABIVER := 2

VPATH += $(SRCDIR)/dma
#INC += $(wildcard *.h) $(wildcard dma/*.h)

CFLAGS += -DX86_PC -DXLITTLE_ENDIAN -DUSE_IO_MACROS -DNWLDMA -DX86_64
CFLAGS += -I$(SRCDIR) -I$(SRCDIR)/dma
CFLAGS += -DNOLOG


LDLIBS += -lrte_eal -lrte_mbuf -lrte_mempool -lrte_ring
LDLIBS += -lrte_ethdev -lrte_net -lrte_kvargs -lrte_hash
LDLIBS += -lrte_bus_pci
#
# all source are stored in SRCS-y
#
SRCS-y += vc709_fpga.c
SRCS-y += vc709_rxtx.c
SRCS-y += vc709_pci_uio.c
SRCS-y += vc709_get_stats.c

SRCS-y += xdma.c
SRCS-y += xdma_bdring.c


include $(DHL_SDK)/mk/dhl.extlib.mk
