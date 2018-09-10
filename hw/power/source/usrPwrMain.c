#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comStruct.h"
#include "comUart.h"
#include "pubPwrMain.h"

pthread_mutex_t gPwrReadLock; // 电源模块串口使用的锁

static void usrPwrInitVar()
{
    pthread_mutex_init(&gPwrReadLock, NULL);
    printf("usrPwrInitVar %x\n",&gPwrReadLock);
}

int usrPwrInit()
{   
    int nRet = SUCCESS;
    usrPwrInitVar();
    if (getPwrUartNo() > 0)
    {
        initPwrTask();
    }
    else 
    {
        printf("the pwr uart is error %d.\r\n", getPwrUartNo());
        nRet = FAILED;        
    }    
    return nRet;
}


