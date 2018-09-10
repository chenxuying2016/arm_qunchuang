#include "comStruct.h"
#include "pubPwrMain.h"
#include "comUtils.h"

extern pthread_mutex_t gPwrReadLock;  // 电源串口锁

static void pwrRcvThread();

void pwrThread(void)
{
	INT ret = 0;
	pthread_t pwrthread;
	pthread_create(&pwrthread, NULL, (void *) pwrRcvThread, NULL);

	if (ret != 0)
	{
		printf("can't create thread: %s.\r\n", strerror(ret));
		return ;
	}

	pthread_detach(pwrthread);
}

void pwrRcvThread()
{
	struct timeval delaytime;
	BYTE buf[1024] = {0};
	INT readlen = 0;
	tPwrMsgHead *msgHead = NULL;
	delaytime.tv_sec = 0;
	delaytime.tv_usec = 50;

	while (1)
	{
		pthread_mutex_lock(&gPwrReadLock);
        if (getPwrUartNo() >= 0)
		{
			readlen = uartReadData(getPwrUartNo(), delaytime, buf) ;
        }
		pthread_mutex_unlock(&gPwrReadLock);

		if (readlen > 0)
		{
			msgHead = (tPwrMsgHead *)buf;

            traceMsg(QUEUE_PRT_SW1, "pwrRcvThread cmd = 0x%02x, cmdtype = 0x%02x readlen=%d\r\n", buf[0], buf[1],readlen);

			if (PWR_MSG_FROM_ETH == msgHead->signl_type || PWR_MSG_FROM_UART == msgHead->signl_type)
				ifnetDataSend((BYTE *)buf, readlen, NULL);

			readlen = 0;
		}
		else
			usleep(300); 
	}
}


