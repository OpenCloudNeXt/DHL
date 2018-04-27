ifeq ($(RTE_SDK),)
$(error RTE_SDK is not defined)
endif
#
# directory list
#
ROOTDIRS-y := lib fpga_drivers dhl_nflib dhl_manager
export ROOTDIRS-y ROOTDIRS- ROOTDIRS-n

fpga_drivers: | lib
dhl_nflib: | lib fpga_drivers
dhl_manager: | lib fpga_drivers dhl_nflib

# define Q to '@' or not. $(Q) is used to prefix all shell commands to
# be executed silently.
Q=@
ifeq '$V' '0'
override V=
endif
ifdef V
ifeq ("$(origin V)", "command line")
Q=
endif
endif
export Q

#
# export DHL_SRCDIR and BUILDING_DHL_SDK
#
DHL_SRCDIR = $(CURDIR)
export DHL_SRCDIR

#
# Default output is $(DHL_SRCDIR)/build
# output files will go in a seperate directory
#
ifdef O
ifeq ("$(origin O)", "command line")
DHL_OUTPUT := $(abspath $(O))
endif
endif
DHL_OUTPUT ?= $(DHL_SRCDIR)/build
export DHL_OUTPUT

# the directory where intermediate build files are stored, like *.o,
# *.d, *.cmd, ...
BUILDDIR = $(DHL_OUTPUT)/build
export BUILDDIR

# for external lib, drivers compilation
RTE_SRCDIR ?= DHL_SRCDIR
RTE_OUTPUT ?= DHL_OUTPUT
export RTE_SRCDIR
export RTE_OUTPUT






#
# build and clean targets
#
CLEANDIRS = $(addsuffix _clean,$(ROOTDIRS-y) $(ROOTDIRS-n) $(ROOTDIRS-))

.PHONY: build
build: $(ROOTDIRS-y)
	@echo "Build comlete DHL"
	
.PHONY: clean
#clean: $(CLEANDIRS)
#	@echo "clean have not been realised"
clean: 
	rm -rf ./build/

.SECONDEXPANSION:
.PHONY: $(ROOTDIRS-y) $(ROOTDIRS-)
$(ROOTDIRS-y) $(ROOTDIRS-):
	@[ -d $(BUILDDIR)/$@ ] || mkdir -p $(BUILDDIR)/$@
	@echo "==Build $@"
	$(Q)$(MAKE) O=$(BUILDDIR)/$(@) -C $(DHL_SRCDIR)/$@ all
#	@if [ $@ = drivers ]; then \
#		$(MAKE) -f $(RTE_SDK)/mk/rte.combinedlib.mk; \
#	fi
	

%_clean:
	@echo "== Clean $*"
	$(Q)if [ -f $(DHL_SRCDIR)/$*/Makefile -a -d $(BUILDDIR)/$* ]; then \
		$(MAKE) S=$* -f $(DHL_SRCDIR)/$*/Makefile -C $(BUILDDIR)/$* clean ; \
	fi


$(DHL_OUTPUT):
	$(Q)mkdir -p $@

.PHONY: default
default: all

.PHONY: all
all: builddir build

.PHONY: builddir
builddir: $(DHL_OUTPUT)
	@echo "The build dir is" $(DHL_OUTPUT)

.PHONY: FORCE
FORCE:
