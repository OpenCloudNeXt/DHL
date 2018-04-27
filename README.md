# configure hugepages
we recommend using 1GB hugepage size

For 1GB pages, it is not ppossible to reserve the hugepage memory after the system has booted.
We need to set the hugepages option to the kernel.

1. vim /boot/grubs/grub.cfg

add this options after the `linux16 /vmlinuz-*.*.* ` command line.
`default_hugepagesz=1G hugepagesz=1G hugepages=4`

2. using hugepages with the DPDK
`mkdir /mnt/huge`
`mount -t hugetlbfs nodev /mnt/huge_1GB`

The mount point can be made permanent across reboots, by adding the following line to the /etc/fstab file:
`nodev /mnt/huge_1GB hugetlbfs pagesize=1GB 0 0`