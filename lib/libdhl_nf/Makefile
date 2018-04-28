include $(RTE_SDK)/mk/rte.vars.mk

# library name
LIB	= libdhl_nf.a

CFLAGS += $(WERROR_FLAGS)

LIBABIVER := 6

# all source are stored in SRCS-y
SRCS-y := dhl_nf.c dhl_pkt_print.c

#
# Export include files
#
SYMLINK-y-include += dhl_common.h
SYMLINK-y-include += dhl_includes.h
SYMLINK-y-include += dhl_msg_common.h
SYMLINK-y-include += dhl_nf.h
SYMLINK-y-include += dhl_pkt_common.h
SYMLINK-y-include += dhl_pkt_print.h

include $(DHL_SDK)/mk/dhl.extlib.mk