include $(RTE_SDK)/mk/rte.vars.mk

#
# module name and path
# 
MODULE = vc709_uio
MODULE_PATH = fpga_drviers/vc709_uio

#
# CFLAGS
#
MODULE_CFLAGS += -I$(SRCDIR) --param max-inline-insns-single=100
MODULE_CFLAGS += -Winline -Wall -Werror

#
# all source are stored in SRCS-y
#
SRCS-y := vc709_uio.c

include $(DHL_SDK)/mk/dhl.extmodule.mk