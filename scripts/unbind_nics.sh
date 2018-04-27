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

#echo "Checking NIC status"
#sleep 1
#$DPDK_DEVBIND --status

echo "UnBinding NIC status"
if [ -z "$DHL_NIC_PCI" ];then
	nics_use_igb_uio=$($DPDK_DEVBIND --status | grep -e "10G" -e "10G-Gigabit" -e "40G" | grep drv=igb_uio | cut -f 1 -d " " | wc -l)
    if [ ${nics_use_igb_uio} == 0 ];then
    	echo "There is no NICs that uses the igb_uio driver"
    	exit 1
	else
#		echo "unbind 10G NICs"
#		for nic_id in $($DPDK_DEVBIND --status | grep -e "710" | grep drv=igb_uio | cut -f 1 -d " ")
#		do
#			echo "UnBinding $nic_id from igb_uio and bind it to ixgbe"
#			$DPDK_DEVBIND -b ixgbe $nic_id
#		done
		
		echo "unbind 40G NICs"
		for nic_id in $($DPDK_DEVBIND --status | grep -e "40G" -e "710" | grep drv=igb_uio | cut -f 1 -d " ")
		do
			echo "UnBinding $nic_id from igb_uio and bind it to i40e"
			$DPDK_DEVBIND -b i40e $nic_id
		done
	fi
else
    # Auto binding example format: export DPDK_NIC_PCI=" 07:00.0  07:00.1 "
    for nic_id in $DHL_NIC_PCI
    do
        echo "UnBinding $nic_id from DPDK"
        $DPDK_DEVBIND -b i40e $nic_id
    done
fi

echo "Finished UnBinding"
#$DPDK_DEVBIND --status