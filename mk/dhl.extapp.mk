#add default include and lib paths for DHL libraries
CFLAGS += -I$(DHL_SDK)/build/include
LDFLAGS += -L$(DHL_SDK)/build/lib
#LDFLAGS += -ldhl_fpga
#_LDLIBS-y += -ldhl_pmd_xilinx_vc709



MAKEFLAGS += --no-print-directory

# we must create the output dir first and recall the same Makefile
# from this directory
ifeq ($(NOT_FIRST_CALL),)

NOT_FIRST_CALL = 1
export NOT_FIRST_CALL

all:
	$(Q)mkdir -p $(RTE_OUTPUT)
	$(Q)$(MAKE) -C $(RTE_OUTPUT) -f $(RTE_EXTMK) \
		S=$(RTE_SRCDIR) O=$(RTE_OUTPUT) SRCDIR=$(RTE_SRCDIR)

%::
	$(Q)mkdir -p $(RTE_OUTPUT)
	$(Q)$(MAKE) -C $(RTE_OUTPUT) -f $(RTE_EXTMK) $@ \
		S=$(RTE_SRCDIR) O=$(RTE_OUTPUT) SRCDIR=$(RTE_SRCDIR)
else
include $(DHL_SDK)/mk/dhl.app.mk
endif