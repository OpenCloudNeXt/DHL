# DHL Installation

This guide helps you build and install DHL.

# System Requirement

First of all, DHL only supports running on Linux system yet. 
Our testbed is based on Centos 7.4 with kernel version of `3.10.0-693.17.1.el7`. 
For the time being, we haven't tested on other Linux distributions, e.g. Ubuntu, but it won't matter.

1. FPGA requirement  

	By now, DHL only provides PMD driver for [Xilinx VC709 board](https://www.xilinx.com/products/boards-and-kits/dk-v7-vc709-g.html). 
	If you own a VC709 board, you can experience the advantages of DHL with a real-world FPGA acceleration test. 
	Otherwise, you can use the [Vitual Channel](./Virtual_channel.md) to replace FPGA borad to experience the DHL framework.  
	
	In future, we are going to develop the PMD driver for [Amazon Aws F1 instance](https://aws.amazon.com/ec2/instance-types/f1/). 
	So everyone can experience the charm of DHL framework in cloud.
	

# Setup Repositories
1. Download source code of DHL
	```sh
	git clone https://github.com/OpenCloudNeXt/DHL.git
	```

2. Download DPDK
	1. Make sure you are at the directory containing DHL, not in the DHL directory
	
	2. You can download tar package of DPDK, like the [latest stable dpdk-17.11.2](https://fast.dpdk.org/rel/dpdk-17.11.2.tar.xz) or
	[latest major dpdk-18.02.1](https://fast.dpdk.org/rel/dpdk-18.02.1.tar.xz). 
		```sh
		xz -d dpdk-18.02.1.tar.xz
		tar -xvf dpdk-18.02.1.tar
		```
		
	3. Alternatively, you can use the latest under-developing version of DPDK.
		```sh
		git clone git://dpdk.org/dpdk
		```
	
# Set up Enviroment	
1. Set environment variable DPDK_VERSION which is the name of the DPDK directory, based on which DPDK version you downloaded.
	```sh
	echo export DPDK_VERSION=dpdk-18.02.1 >> ~/.bashrc
	#echo export DPDK_VERSION=dpdk-17.11.2 >> ~/.bashrc
	#echo export DPDK_VERSION=dpdk >> ~/.bashrc
	```

2. Set environment variable RTE_SDK to the path of the DPDK library.  Make sure that you are in the DPDK directory.
	```sh
	echo export RTE_SDK=$(pwd)/${DPDK_VERSION} >> ~/.bashrc
	```

3. Set environment variable RTE_TARGET to the target architecture of your system.
	```sh
	echo export RTE_TARGET=x86_64-native-linuxapp-gcc  >> ~/.bashrc
	```

4. Set environment variable DPDK_INSTALL_DIR to the path that will hold the compiled DPDK SDK.
	Make sure you are in the directory containing DPDK, not in the DPDK directory.
	```sh
	echo export DPDK_INSTALL_DIR=$(pwd)/${DPDK_VERSION}-install >> ~/.bashrc

5. Set environment variable DHL_SDK to the path of the DHL repository.
	```sh
	echo export DHL_SDK=$(pwd)/DHL >> ~/.bashrc
	```

6. source your shell rc file to set the environment variables:
	```sh
	source ~/.bashrc
	```
	
# Compile and configure DPDK
1. Run the [compile_dpdk script](../scripts/compile_dpdk.sh) to compile DPDK,
	and it will install compiled DPDK SDK in `$(DPDK_INSTALL_DIR)`.
	```
	cd ${DHL_SDK}/scripts
	./compile_dpdk.sh
	```
	
2. Configure hugepages  

	we recommend using 1GB hugepage size to maxmize the performance as recommended by DPDK.

	For 1GB pages, it is not possible to reserve the hugepage memory after the system has booted.
	We need to set the hugepages option to the kernel.

	1. vim /boot/grub2/grub.cfg

		add these options after the `linux16 /vmlinuz-*.*.* ` command line.
		`default_hugepagesz=1G hugepagesz=1G hugepages=4`

	2. using hugepages with DPDK
		`mkdir /mnt/huge_1GB`
		`mount -t hugetlbfs nodev /mnt/huge_1GB`

		The mount point can be made permanent across reboots, by adding the following line to the /etc/fstab file:
		`nodev /mnt/huge_1GB hugetlbfs pagesize=1GB 0 0`
	
	3. you need to reboot the system to bring the 1GB hugepages into effect.
	
		This step can be delayed to the end after you compile the DHL sourde code.
	
# Comiple DHL
1. Compile DHL ,including libdhl_fpga, fpga_drivers, dhl_manager and other libraries
	```sh
	cd ${DHL_SDK}
	make
	```
	
2. Comiple example NFs
	```sh
	cd ${DHL_SDK}\examples
	make
	```
