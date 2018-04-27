#!/bin/bash

# Confirm environment variables
if [ -z "$RTE_TARGET" ]; then
    echo "Please export \$RTE_TARGET"
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

DPDK_DEVBIND=$DPDK_INSTALL_DIR/sbin/dpdk-devbind
#DPDK_DEVBIND=$RTE_SDK/usertools/dpdk-devbind.py	# for DPDK 17 and up

# Load uio kernel modules
grep -m 1 "igb_uio" /proc/modules | cat 
if [ ${PIPESTATUS[0]} != 0 ]; then
    echo "Loading uio kernel modules"
    sleep 1
	kernel_version=$(uname -r)
    sudo modprobe uio
    sudo insmod ${DPDK_INSTALL_DIR}/kmod/igb_uio.ko
else
    echo "IGB UIO module already loaded."
fi

#echo "Checking NIC status"
#sleep 1
#$DPDK_DEVBIND --status

echo "Binding NIC status"
if [ -z "$DHL_NIC_PCI" ];then
	tenG_and_40G_nics=$($DPDK_DEVBIND --status | grep -v Active | grep -e "10G" -e "10G-Gigabit" -e "40G" | grep unused=igb_uio | cut -f 1 -d " " | wc -l)
	if [ ${tenG_and_40G_nics} == 0 ];then
		echo "There is no NICs that can be binded to igb_uio driver"
		exit 1
	else
		for id in $($DPDK_DEVBIND --status | grep -v Active | grep -e "10G" -e "10G-Gigabit" -e "40G" | grep unused=igb_uio | cut -f 1 -d " ")
		do
			read -r -p "Bind interface $id to DPDK? [Y/N] " response
			if [[ $response =~ ^([yY][eE][sS]|[yY])$ ]];then
				echo "Binding $id to dpdk"
				$DPDK_DEVBIND -b igb_uio $id
			fi
		done
	fi
else
	# Auto binding example format: export DPDK_NIC_PCI=" 07:00.0  07:00.1 "
    for nic_id in $DHL_NIC_PCI
    do
        echo "Binding $nic_id to DPDK"
        sudo $DPDK_DEVBIND -b igb_uio $nic_id
    done
fi

echo "Finished Binding"
$DPDK_DEVBIND --status