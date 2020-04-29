/*
 * nl_bb_msg.h
 *
 *  Created on: 21.01.2015
 *      Author: ssc
 *	last changed: 2020-03-30  KSTR
 *	- SW_VERSION  203
 * */

#ifndef NL_DRV_NL_BB_MSG_H_
#define NL_DRV_NL_BB_MSG_H_

#include <stdint.h>

#include "../../../shared/lpc-defs.h"

//===========================
void BB_MSG_WriteMessage_DBG(uint16_t type, uint16_t length, uint16_t *data);
void BB_MSG_WriteMessage1Arg_DBG(uint16_t type, uint16_t arg);

int32_t BB_MSG_WriteMessage(uint16_t type, uint16_t length, uint16_t *data);
int32_t BB_MSG_WriteMessage2Arg(uint16_t type, uint16_t arg0, uint16_t arg1);
int32_t BB_MSG_WriteMessage1Arg(uint16_t type, uint16_t arg);
int32_t BB_MSG_WriteMessageNoArg(uint16_t type);

int32_t BB_MSG_SendTheBuffer(void);

void BB_MSG_ReceiveCallback(uint16_t type, uint16_t length, uint16_t *data);

#endif /* NL_DRV_NL_BB_MSG_H_ */
