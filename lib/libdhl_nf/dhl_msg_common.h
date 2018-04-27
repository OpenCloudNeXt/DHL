/***********************************************************************

							dhl_msg_common.h

***********************************************************************/
#ifndef DHL_MANAGER_DHL_MSG_COMMON_H_
#define DHL_MANAGER_DHL_MSG_COMMON_H_

#include <stdint.h>

#define MSG_NOOP 0
#define MSG_STOP 1
#define MSG_NF_REGISTERING 2
#define MSG_NF_UNREGISTERING 3
#define MSG_NF_STARTING 4
#define MSG_NF_STOPPING 5
#define MSG_NF_READY 6

struct dhl_nf_msg {
        uint8_t msg_type; /* Constant saying what type of message is */
        void *msg_data; /* These should be rte_malloc'd so they're stored in hugepages */
};
#endif /* DHL_MANAGER_DHL_MSG_COMMON_H_ */
