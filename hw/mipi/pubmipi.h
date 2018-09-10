#ifndef __PUB_MIPI__
#define __PUB_MIPI__

#include "pubtiming.h"
#include "vcom.h"
//extern unsigned char Channel;

extern uint16_t VsCount;//4;
extern uint16_t VBPCount;//4;
extern uint16_t VFPCount;//4;
extern uint16_t VDPCount;//800;
extern uint16_t VTotal;// = 1954;//1235;

extern uint16_t HsCount;//20;
extern uint16_t HBPCount;//15;
extern uint16_t HFPCount;//200;
extern uint16_t HDPCount;//1280;
extern uint16_t HTotal;// = 1294;//2080;

extern uint8_t  LaneNum;
extern uint8_t SyncMode;  // 0:pulse mode 1: event mode
extern uint32_t DSIFRE;//725000000;//1000000000;    //hz
extern uint8_t BitNum;  //6bit 8bit 10bit

#define MAX_MIPI_CODE_LEN 8192
extern uint8_t InitCode[MAX_MIPI_CODE_LEN];//3072

extern uint8_t HsSignalOnFlag;
//extern uint8_t SPINUM;
//extern uint8_t Lane8Flag;


int mipi_working(timing_info_t *timing_info, char *pMipiCode, int mipiLen, vcom_info_t* vcom_info);

#endif
