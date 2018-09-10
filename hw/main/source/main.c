
#include "comStruct.h"
//#include "usrArm.h"
//#include "usrFpga.h"
//#include "usrCim.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef __WIN32__
#include <linux/input.h>
#endif

#if 1
void PcuartInit();
INT uartInit();

//  log 模块私有代码 begin
//extern struct zlog *zlog_default;
//extern int mn_main_start();
//  log 模块私有代码 end

extern INT ifnetInit(void);
extern INT usrsysInit(void);
extern INT cliInit(char *pArgv);
extern void ifnetRecv(void);
extern int usrPwrInit();
extern int usrExtInit();
extern INT usrI2cInit(void);
extern INT jobInit(void);
extern int mn_main_start (void);


INT pgJob_init(void)
{
	INT nRet = SUCCESS;

    uartInit();  //  串口初始化
    
#if 1	
	printf("well done. \r\n");

	nRet = initFpga();
	if (SUCCESS != nRet)
	{
		printf("initFpga failed.\r\n");
		return FAILED;
	}

	
    nRet = initLcd();
	if (SUCCESS != nRet)
	{
		printf("initLcd failed.\r\n");
		return FAILED;
    }
	
	nRet = usrPwrInit();  // 电源模块初始化函数
	if (SUCCESS != nRet)
    {
        printf("init pwr failed.\r\n");
		return FAILED;
    }

#endif

	return SUCCESS;
}
#endif


