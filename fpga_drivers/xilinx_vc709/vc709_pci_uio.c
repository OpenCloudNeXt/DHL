/*
 * vc709_pci_uio.c
 *
 *  Created on: Jul 3, 2017
 *      Author: root
 */
#include "../../fpga_drivers/xilinx_vc709/vc709_pci_uio.h"

#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <rte_mbuf.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_common.h>
#include <rte_malloc.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_config.h>
#include <rte_mempool.h>

#include <dhl_fpga.h>

#include "../../fpga_driver/xilinx_vc709/vc709_fpga.h"
#include "../../fpga_drivers/xilinx_vc709/vc709_log.h"


extern void *pci_map_addr;



/*
 * Return the uioX char device used for a pci device. On success, return
 * the UIO number (X) and fill dstbuf string with the path of the device in
 * sysfs. On error, return a negative value. In this case dstbuf is
 * invalid.
 */
static int
vc709_get_uio_dev_no_and_path(struct rte_pci_device *dev, char *dstbuf,
			   unsigned int buflen)
{
	struct rte_pci_addr *loc = &dev->addr;
	unsigned int uio_num;
	struct dirent *e;
	DIR *dir;
	char dirname[PATH_MAX];

	/* depending on kernel version, uio can be located in uio/uioX
	 * or uio:uioX */

	/* SYSFS_PCI_DEVICES "/sys/bus/pci/devices" */
	snprintf(dirname, sizeof(dirname),
			"%s/" PCI_PRI_FMT "/uio", rte_pci_get_sysfs_path(),
			loc->domain, loc->bus, loc->devid, loc->function);

	dir = opendir(dirname);
	if (dir == NULL) {
		/* retry with the parent directory */
		snprintf(dirname, sizeof(dirname),
				"%s/" PCI_PRI_FMT, rte_pci_get_sysfs_path(),
				loc->domain, loc->bus, loc->devid, loc->function);
		dir = opendir(dirname);

		if (dir == NULL) {
			RTE_LOG(ERR, EAL, "Cannot opendir %s\n", dirname);
			return -1;
		}
	}

	/* take the first file starting with "uio" */
	while ((e = readdir(dir)) != NULL) {
		/* format could be uio%d ...*/
		int shortprefix_len = sizeof("uio") - 1;
		/* ... or uio:uio%d */
		int longprefix_len = sizeof("uio:uio") - 1;
		char *endptr;

		if (strncmp(e->d_name, "uio", 3) != 0)
			continue;

		/* first try uio%d */
		errno = 0;
		uio_num = strtoull(e->d_name + shortprefix_len, &endptr, 10);
		if (errno == 0 && endptr != (e->d_name + shortprefix_len)) {
			snprintf(dstbuf, buflen, "%s/uio%u", dirname, uio_num);
			break;
		}

		/* then try uio:uio%d */
		errno = 0;
		uio_num = strtoull(e->d_name + longprefix_len, &endptr, 10);
		if (errno == 0 && endptr != (e->d_name + longprefix_len)) {
			snprintf(dstbuf, buflen, "%s/uio:uio%u", dirname, uio_num);
			break;
		}
	}
	closedir(dir);

	/* No uio resource found */
	if (e == NULL)
		return -1;

	return uio_num;
}

/*
 * read the uio device info into bd_uio_res
 */
static int
vc709_read_uio_resource_info(struct rte_pci_device *dev,
		struct vc709_mapped_kernel_resource * bd_uio_res)
{
	char dirname[PATH_MAX];
	char devname[PATH_MAX];
	int uio_num;
	struct rte_pci_addr * loc;

	loc = &dev->addr;

	/* find uio resource and fill up the bd_uio_res with
	 * uio_path: the file system path to the /sys/bus/pci/devices/XXXX:XX:XX.X/uio/uioX
	 * path: /dev/uioX
	 * pci_addr: dev->addr (XXXX:XX:XX.X)*/
	uio_num = vc709_get_uio_dev_no_and_path(dev, dirname, sizeof(dirname) );
	if (uio_num < 0) {
		RTE_LOG(WARNING, PMD, "  "PCI_PRI_FMT" not managed by UIO driver, "
				"skipping\n", loc->domain, loc->bus, loc->devid, loc->function);
		return 1;
	}
	snprintf(bd_uio_res->uio_path, sizeof(bd_uio_res->uio_path),"%s", dirname);
	snprintf(devname, sizeof(devname), "/dev/uio%u", uio_num);
	snprintf(bd_uio_res->path, sizeof(bd_uio_res->path), "%s", devname);
	memcpy(&(bd_uio_res->pci_addr), &dev->addr, sizeof(bd_uio_res->pci_addr));

	return 0;
}

static int
vc709_parse_sysfs_value(const char * filename, uint64_t * val)
{
	FILE *f;
	char buf[BUFSIZ];
	char *end = NULL;

	if ((f = fopen(filename, "r")) == NULL) {
		RTE_LOG(ERR, PMD, "%s(): cannot open sysfs value %s\n",
			__func__, filename);
		return -1;
	}

	if (fgets(buf, sizeof(buf), f) == NULL) {
		RTE_LOG(ERR, PMD, "%s(): cannot read sysfs value %s\n",
			__func__, filename);
		fclose(f);
		return -1;
	}
	*val = strtoul(buf, &end, 0);
	if ((buf[0] == '\0') || (end == NULL) || (*end != '\n')) {
		RTE_LOG(ERR, PMD, "%s(): cannot parse sysfs value %s\n",
				__func__, filename);
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

void *
pci_find_max_end_va1(void)
{
	const struct rte_memseg *seg = rte_eal_get_physmem_layout();
	const struct rte_memseg *last = seg;
	unsigned i = 0;

	for (i = 0; i < RTE_MAX_MEMSEG; i++, seg++) {
		if (seg->addr == NULL)
			break;

		if (seg->addr > last->addr)
			last = seg;

	}
	return RTE_PTR_ADD(last->addr, last->len);
}

/*
 * map the uio requested coherent dma memory address to the usersapce
 * bd_res: the buff descriptor struct
 * phys_addr: kernel coherent dma memory physical address
 * len: the memory length
 * map_num: the mapN number, in the bd_res->uio_path/maps/mapN, the N is the map_num,
 * 			and which is used for mmap that N*pagesize() for mmap the N resources.
 * map_idx: means this kernel coherent dma memory will be stored in be_res->maps[map_idx]
 */
int
vc709_uio_map_bd_resource_by_index(struct vc709_mapped_kernel_resource * bd_res,
		uint64_t phys_addr, size_t len, int map_num, int map_idx)
{
	int fd;
	char devname[PATH_MAX];
	void *mapaddr;
	struct pci_map *maps;

	maps = bd_res->maps;

	/* update devname for mmap  */
	snprintf(devname, sizeof(devname),"%s",bd_res->path);

	/* allocate memory to keep path */
	maps[map_idx].path = rte_malloc(NULL, strlen(devname) + 1, 0);
	if (maps[map_idx].path == NULL) {
		RTE_LOG(ERR, EAL, "Cannot allocate memory for path: %s\n",
				strerror(errno));
		return -1;
	}

	/*
	 * open resource file, to mmap it
	 */
	fd = open(devname, O_RDWR);
	if (fd < 0) {
		RTE_LOG(ERR, EAL, "Cannot open %s: %s\n",
				devname, strerror(errno));
		goto error;
	}

	/* try mapping somewhere close to the end of hugepages */
	if (pci_map_addr == NULL)
		pci_map_addr = pci_find_max_end_va1();

	mapaddr = pci_map_resource(pci_map_addr, fd, map_num * getpagesize(), len, 0);
//	mapaddr = pci_map_resource(NULL, fd, map_num * getpagesize(), len, 0);
	close(fd);
	if (mapaddr == MAP_FAILED)
		goto error;

	pci_map_addr = RTE_PTR_ADD(mapaddr,(size_t)len);

	maps[map_idx].phaddr = phys_addr;
	maps[map_idx].size = len;
	maps[map_idx].addr = mapaddr;
	maps[map_idx].offset = 0;
	strcpy(maps[map_idx].path, devname);

	return 0;

error:
	rte_free(maps[map_idx].path);
	return -1;
}




int
vc709_uio_map_bd_memeory(struct rte_pci_device * dev, struct vc709_mapped_kernel_resource * bd_res)
{
	int i, map_idx = 0, ret;
	uint64_t phaddr, len;
	DIR *dir;
	struct dirent * e;
	char dirname[PATH_MAX];
	char mapname[PATH_MAX];
	unsigned int map_num;
	uint64_t tmp;

	ret = vc709_read_uio_resource_info(dev, bd_res);
	if(ret)
		return ret;

	/* parse driver mapping address, which located in /sys/bus/pci/devices/XXXX:XX:XX.X/uio/uioX/maps */
	snprintf(dirname, sizeof(dirname), "%s/maps", bd_res->uio_path);

	dir = opendir(dirname);
	if (dir == NULL) {
		RTE_LOG(ERR, PMD, "Cannot opendir %s\n", dirname);
		return -1;
	}

	while ( (e = readdir(dir)) != NULL){
		int prefix_len = sizeof("map") - 1;
		char * endptr;

		if(strncmp(e->d_name, "map", 3) != 0)
			continue;

		ret = 0;
		map_num = strtoull(e->d_name + prefix_len, &endptr, 10);
		if(map_num != 0 && endptr != (e->d_name + prefix_len)) {
			snprintf(mapname, sizeof(mapname), "%s/map%u/addr", dirname, map_num);
			if( vc709_parse_sysfs_value(mapname, &tmp) < 0){
				rte_free(bd_res);
				break;
			}
			phaddr = tmp;

			snprintf(mapname, sizeof(mapname), "%s/map%u/size", dirname, map_num);
			if( vc709_parse_sysfs_value(mapname, &tmp) < 0){
				rte_free(bd_res);
				break;
			}
			len = tmp;

			VC709_LOG("bd_res map[%i], phaddr is %lx, len is 0x%lx \n",map_idx, phaddr, len);
			ret = vc709_uio_map_bd_resource_by_index(bd_res, phaddr, len, map_num, map_idx);
			if (ret)
				goto error;

			map_idx++;
		}
	}
	bd_res->nb_maps = map_idx;

	return 0;
error:
	for (i = 0; i <map_idx; i++) {
		pci_unmap_resource(bd_res->maps[i].addr,
						(size_t)bd_res->maps[i].size);
		rte_free(bd_res->maps[i].path);
	}
	rte_free(bd_res);

	return -1;
}
