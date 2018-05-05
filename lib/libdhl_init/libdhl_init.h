/*
 * libdhl_init.h
 *
 *  Created on: May 5, 2018
 *      Author: lxy
 */

#ifndef LIB_LIBDHL_INIT_LIBDHL_INIT_H_
#define LIB_LIBDHL_INIT_LIBDHL_INIT_H_


/**
 * Initialize the environment for DHL FPGA
 *
 * @param argc
 * 	A non-negative value. If it is greater than 0, the array members
 * 	for argv[0] through argv[argc] (non-inclusive) shall contain pointers to strings.
 * @param argv
 *	An array of strings.  The contents of the array, as well as the strings
 * 	which are pointed to by the array, may be modified by this function.
 * @return
 * 	- On success, the number of parsed arguments, which is greater or equal to zero.
 * 	  After the call to dhl_init(), all arguements argv[x] with x < ret may have been modified
 * 	  by this function call and should not be further interpreted by the application.
 * 	- On failure, -1 and dhl_errno is set to a value indicating the cause for failure. In some
 * 	  instances, the application will need to be restarted as part of clearing the issue.
 *
 * 	Error codes returned via dhl_errno:
 *
 */
int dhl_init(int argc, char ** argv);

#endif /* LIB_LIBDHL_INIT_LIBDHL_INIT_H_ */
