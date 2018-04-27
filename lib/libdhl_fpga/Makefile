include $(RTE_SDK)/mk/rte.vars.mk

#
#library name
#
LIB = libdhl_fpga.a

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

EXPORT_MAP := dhl_fpga_version.map

LIBABIVER := 6

SRCS-y += dhl_fpga.c

#
# Export include files
#
SYMLINK-y-include += dhl_fpga.h
SYMLINK-y-include += dhl_fpga_pci.h

include $(DHL_SDK)/mk/dhl.extlib.mk
