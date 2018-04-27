/*
 * vc709_log.h
 *
 *  Created on: Jun 21, 2017
 *      Author: root
 */

#ifndef VC709_LOG_H_
#define VC709_LOG_H_

#define PMD_INIT_LOG(level, fmt, args...) \
	RTE_LOG(level, PMD, "%s(): " fmt "\n", __func__, ##args)

#ifdef RTE_LIBRTE_VC709_DEBUG_INIT
#define PMD_INIT_FUNC_TRACE() PMD_INIT_LOG(DEBUG, " >>")
#else
#define PMD_INIT_FUNC_TRACE() do { } while(0)
#endif

#ifdef RTE_LIBRTE_VC709_DEBUG_RX
#define PMD_RX_LOG(level, fmt, args...) \
	RTE_LOG(level, PMD, "%s(): " fmt "\n", __func__, ## args)
#else
#define PMD_RX_LOG(level, fmt, args...) do { } while(0)
#endif

#ifdef RTE_LIBRTE_VC709_DEBUG_TX
#define PMD_TX_LOG(level, fmt, args...) \
	RTE_LOG(level, PMD, "%s(): " fmt "\n", __func__, ## args)
#else
#define PMD_TX_LOG(level, fmt, args...) do { } while(0)
#endif

#ifdef RTE_LIBRTE_VC709_DEBUG_TX_FREE
#define PMD_TX_FREE_LOG(level, fmt, args...) \
	RTE_LOG(level, PMD, "%s(): " fmt "\n", __func__, ## args)
#else
#define PMD_TX_FREE_LOG(level, fmt, args...) do { } while(0)
#endif

#ifdef RTE_LIBRTE_VC709_DEBUG_DRIVER
#define PMD_DRV_LOG_RAW(level, fmt, args...) \
	RTE_LOG(level, PMD, "%s(): " fmt, __func__, ## args)
#else
#define PMD_DRV_LOG_RAW(level, fmt, args...) do { } while (0)
#endif

#define PMD_DRV_LOG(level, fmt, args...) \
	PMD_DRV_LOG_RAW(level, fmt "\n", ## args)

//#define NOLOG

#ifndef NOLOG
#define VC709_LOG(fmt, args...) \
	RTE_LOG(INFO, PMD, "%s(): " fmt , __func__, ##args)

#define VC709_LOG_NOFUN(fmt, args...) \
	RTE_LOG(INFO, PMD, fmt , ##args)

#define log_verbose(args...)	printf(args);

#define log_rxtx(fmt, args...) \
	printf("%s() rxtx: " fmt , __func__, ##args)

#else
#define VC709_LOG(fmt, args...)
#define VC709_LOG_NOFUN(fmt, args...)
#define log_verbose(args...)
#define log_rxtx(fmt, args...)

#endif






#endif /* VC709_LOG_H_ */
