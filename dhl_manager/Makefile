ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

ifeq ($(DHL_SDK),)
$(error "Please define DHL_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = dhl_manager

# all source are stroe in SRC-y
SRCS-y := main.c dhl_mgr_init.c dhl_mgr_args.c dhl_mgr_stats.c dhl_mgr_nf.c dhl_mgr_packer.c dhl_mgr_distributor.c

#INC := dhl_manager.h dhl_init.h dhl_args.h dhl_msg_common.h dhl_nf.h dhl_stats.h

#CFLAGS += -O3
#CFLAGS += -I$(SRCDIR)/../ -I$(SRCDIR)/../dhl_nflib/

#LDFLAGS += $(DHL_SDK)/dhl_nflib/build/libdhlnf.a


include $(DHL_SDK)/mk/dhl.extapp.mk