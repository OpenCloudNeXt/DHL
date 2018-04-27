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

# Validate sudo access
sudo -v

# Compile dpdk
cd $RTE_SDK
echo "Clean the pre-compiled dpdk in $RTE_SDK"
make clean
sleep 1
echo "Clean the installed dpdk in $DPDK_INSTALL_DIR"
rm -rf $DPDK_INSTALL_DIR
