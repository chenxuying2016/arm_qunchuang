#ifndef PWRMAIN_H_
#define PWRMAIN_H_

#include "comStruct.h"

#define POWER_INI_FILE_PATH  "cfg/power/"

enum
{
	PWR_STAT_ON = 0x0,
	PWR_STAT_OFF = 0x1,
};

void pwrThread();

INT initPwrTask(void);

unsigned int power_message_queue_get();

int read_pwr_config(char *pwrName,s1103PwrCfgDb *pPower_info);

#endif

