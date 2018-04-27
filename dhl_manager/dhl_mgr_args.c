
/********************************************************************************************

                                   dhl_args.c

		File containing the function parsing all DHL arguments.

********************************************************************************************/
#include <getopt.h>

#include "dhl_mgr.h"
#include "dhl_buffer_queue.h"
#include "dhl_mgr_stats.h"

/***********************Internal Functions prototypes*************************/
static void usage(void);

static int parse_stats_output(const char * stats_output);

static int parse_stats_sleep_time(const char * sleeptime);

static int parse_packer_lcores(const char *p_conf);

static int parse_distributor_lcores(const char * p_conf);


static struct option lgopts[] = {
		{"stats-output",	no_argument,	NULL,	'o'},
		{"stats-sleep-time",no_argument,	NULL,	's'},
		{"packer-lcores", 	required_argument, 	NULL,	'p'},
		{"distributor-lcores", required_argument,	NULL,	'd'},
};

/* global variable for program name */
const char *progname;

/*********************************Interfaces**********************************/


int
parse_manager_args(uint8_t max_fpgas, int argc, char * argv[]) {
	int opt, ret;
	char ** argvopt;
	int option_index;

	progname = argv[0];
	argvopt = argv;

	uint32_t arg_output = 0;
	uint32_t arg_sleeptime = 0;
	uint32_t arg_packer_lcores = 0;
	uint32_t arg_distributor_lcores = 0;
	uint32_t arg_bsz = 0;

	/* Error or normal output strings. */
	while ((opt = getopt_long(argc, argvopt, "o:s:p:d:", lgopts, &option_index)) != EOF) {
		switch (opt) {
		case 'o':
			if (parse_stats_output(optarg) != 0) {
				usage();
				return -1;
			}

			dhl_stats_set_output(manager.stats_destination);
			arg_output = 1;
			break;

		case 's':
			if (parse_stats_sleep_time(optarg) != 0) {
				usage();
				return -1;
			}
			arg_sleeptime = 1;
			break;

		case 'p':
			ret = parse_packer_lcores(optarg);
			if (ret) {
				printf("Invalid packer lcores configuration!\n");
				usage();
				return -1;
			}
			arg_packer_lcores = 1;
			break;
		case 'd':
			ret = parse_distributor_lcores(optarg);
			if (ret) {
				printf("Invalid distributor lcores configurations!\n");
				usage();
				return -1;
			}
			arg_distributor_lcores = 1;
			break;
		default:
			printf("ERROR: Unknown option '%c'\n", opt);
			usage();
			return -1;
		}
	}

	/* Check that all mandatory arguments are provided */
	if ((arg_packer_lcores == 0) || (arg_distributor_lcores == 0) ){
		printf("Not all mandatory arguments are present\n");
		usage();
		return -1;
	}

	if (arg_bsz == 0) {
		manager.burst_size_ibq_out = BQ_DEFAULT_BURST_SIZE_IBQ_OUT;
		manager.burst_size_obq_out = BQ_DEFAULT_BURST_SIZE_OBQ_OUT;
	}

	return 0;
}

static void
usage(void) {
	printf(
			"%s [EAL options] -- \n"
			"\t-o STATS_OUTPUT: where to output DHL runtime stats (stdout/stderr/web).\n"
			"\t-s STATS_SLEEP_TIME: the stats update interval (in seconds).\n"
			"\t-p (manatory) PACKER_LCORES: specify the lcores for packer thread.\n"
			"\t-d (manatory) DISTRIBUTOR_LCORES: specify the lcores for distributor thread.\n",
			progname);
}

static int
parse_stats_output(const char * stats_output) {
	if(!strcmp(stats_output, DHL_STR_STATS_STDOUT)) {
		manager.stats_destination = DHL_STATS_STDOUT;
		return 0;
	} else if(!strcmp(stats_output, DHL_STR_STATS_STDERR)) {
		manager.stats_destination = DHL_STATS_STDERR;
		return 0;
	} else if(!strcmp(stats_output, DHL_STR_STATS_WEB)) {
		manager.stats_destination = DHL_STATS_WEB;
		return 0;
	} else {
		return -1;
	}
}

static int
parse_stats_sleep_time(const char * sleeptime) {
	char * end = NULL;
	unsigned long temp;

	temp = strtoul(sleeptime, &end, 10);
	if(end == NULL || *end != '\0' || temp == 0)
		return -1;

	manager.global_stats_sleep_time = (uint32_t)temp;

	return 0;
}

static int
parse_packer_lcores(const char * p_conf) {
	char s[256];
	char * end;
	int count, i;


	unsigned long int_core;
	char * str_cores[RTE_MAX_LCORE];

	snprintf(s, sizeof(s), "%.*s", sizeof(p_conf), p_conf);
	count = rte_strsplit(s, sizeof(s), str_cores, RTE_MAX_LCORE, ',');
	if (count <= 0)
		return -1;

	struct worker_lcores * packer_params = &manager.packer.packer_params;
	packer_params->num = 0;

	for(i = 0; i< count; i++){
		errno = 0;
		int_core = strtoul(str_cores[i], &end, 0);
		if (errno != 0 || end == str_cores[i] || int_core > 255)
			return -1;

		packer_params->lcores[packer_params->num++] = int_core;
	}

	return 0;
}

static int
parse_distributor_lcores(const char * p_conf) {
	char s[256];
	char * end;
	int count, i;

	unsigned long int_core;
	char * str_cores[RTE_MAX_LCORE];

	snprintf(s, sizeof(s), "%.*s", sizeof(p_conf), p_conf);
	count = rte_strsplit(s, sizeof(s), str_cores, RTE_MAX_LCORE, ',');
	if (count <= 0)
		return -1;

	struct worker_lcores * distributor_params = &manager.distributor.distributor_params;
	distributor_params->num = 0;

	for(i = 0; i< count; i++){
		errno = 0;
		int_core = strtoul(str_cores[i], &end, 0);
		if (errno != 0 || end == str_cores[i] || int_core > 255)
			return -1;

		distributor_params->lcores[distributor_params->num++] = int_core;
	}

	return 0;
}

/*
static int
parse_packer_config(const char *p_config) {
	char s[256];
	const char *p, *p0 = p_config;
	char *end;
	enum fieldnames {
		FLD_LCORE = 0,
		FLD_IBQ,
		_NUM_FLD
	};
	unsigned long int_fld[_NUM_FLD];
	char *str_fld[_NUM_FLD];
	int i;
	unsigned size;

	nb_packer_config_params = 0;

	while ((p = strchr(p0,'(')) != NULL) {
		++p;
		if((p0 = strchr(p,')')) == NULL)
			return -1;

		size = p0 - p;
		if(size >= sizeof(s))
			return -1;

		snprintf(s, sizeof(s), "%.*s", size, p);
		if (rte_strsplit(s, sizeof(s), str_fld, _NUM_FLD, ',') != _NUM_FLD)
			return -1;
		for (i = 0; i < _NUM_FLD; i++){
			errno = 0;
			int_fld[i] = strtoul(str_fld[i], &end, 0);
			if (errno != 0 || end == str_fld[i] || int_fld[i] > 255)
				return -1;
		}
		if (nb_packer_config_params >= MAX_PACKER_CONFIG_PARAMS) {
			printf("exceeded max number of packer config params: %hu\n",
					nb_packer_config_params);
			return -1;
		}
		packer_config_params_array[nb_packer_config_params].lcore_id =
			(uint8_t)int_fld[FLD_LCORE];
		packer_config_params_array[nb_packer_config_params].ibq_id =
			(uint8_t)int_fld[FLD_IBQ];
		++nb_packer_config_params;
	}
	packer_config_params = packer_config_params_array;

	return 0;
}
*/







