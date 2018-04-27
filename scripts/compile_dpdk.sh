#! /bin/bash

set -e

if [ -z "$RTE_TARGET" ]; then
    echo "Please export \$RTE_TARGET. Or try running this without sudo."
    exit 1
fi

if [ -z "$RTE_SDK" ]; then
    echo "Please export \$RTE_SDK"
    exit 1
fi

if [ -z "$DPDK_INSTALL_DIR" ]; then
	echo "Please export \$DPDK_INSTALL_DIR"
	exit 1
fi

# Validate sudo access
#sudo -v

# Compile dpdk
cd $RTE_SDK
echo "Compiling and installing dpdk in $RTE_SDK"
sleep 1
#make config T=$RTE_TARGET
#make T=$RTE_TARGET

# 1st method
make install T=$RTE_TARGET DESTDIR=$DPDK_INSTALL_DIR

# 2rd method
#make config T=$RTE_TARGET O=$RTE_TARGET
#make O=$RTE_TARGET
#make install O=$RTE_TARGET DESTDIR=$DPDK_INSTALL_DIR
