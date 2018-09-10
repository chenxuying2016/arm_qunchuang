/*
 * serialDev.cpp
 *
 *  Created on: 2018-7-6
 *      Author: tys
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "serialDev.h"
#include "serialDevApi.h"

static int devSend(struct serialDev* pM, void* pData, unsigned int length)
{
	if (length <= 0)
		return 0;

	return write(pM->fd, pData, length);
}

static void devSetBaud(struct serialDev* pM, unsigned int rate)
{
	struct termios termioInfo = {0};

	if(tcgetattr(pM->fd,&termioInfo)!=0)
		return;

	switch (rate)
	{
	case E_B2400:
		cfsetispeed(&termioInfo,B2400);
		cfsetospeed(&termioInfo,B2400);
		break;

	case E_B4800:
		cfsetispeed(&termioInfo,B4800);
		cfsetospeed(&termioInfo,B4800);
		break;

	case E_B9600:
		cfsetispeed(&termioInfo,B9600);
		cfsetospeed(&termioInfo,B9600);
		break;

	case E_B57600:
		cfsetispeed(&termioInfo,B57600);
		cfsetospeed(&termioInfo,B57600);
		break;

	case E_B115200:
		cfsetispeed(&termioInfo,B115200);
		cfsetospeed(&termioInfo,B115200);
		break;

	default:
		cfsetispeed(&termioInfo,B9600);
		cfsetospeed(&termioInfo,B9600);
	}

	tcflush(pM->fd,TCIFLUSH);

	if((tcsetattr(pM->fd,TCSANOW,&termioInfo))!=0)
		printf("com set error");
}

static void devSetStopBits(struct serialDev* pM, unsigned int stopBits)
{
	struct termios termioInfo = {0};

	if(tcgetattr(pM->fd,&termioInfo)!=0)
		return;

	if(stopBits == E_S_1_BIT)
	{
		termioInfo.c_cflag &= ~CSTOPB;
	}
	else if(stopBits == E_S_2_BIT)
	{
		termioInfo.c_cflag |= CSTOPB;
	}

	tcflush(pM->fd,TCIFLUSH);

	if((tcsetattr(pM->fd,TCSANOW,&termioInfo))!=0)
		printf("serial set error");
}

static void devSetDataBits(struct serialDev* pM, unsigned int dataBits)
{
	struct termios termioInfo = {0};

	if(tcgetattr(pM->fd,&termioInfo)!=0)
		return;

	termioInfo.c_cflag &= ~CSIZE;
	termioInfo.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	termioInfo.c_oflag  &= ~OPOST;   /*Output*/

	switch(dataBits)
	{
	case E_D_5_BIT:
		termioInfo.c_cflag |= CS5;
		break;
	case E_D_6_BIT:
		termioInfo.c_cflag |= CS6;
		break;
	case E_D_7_BIT:
		termioInfo.c_cflag |= CS7;
		break;
	case E_D_8_BIT:
		termioInfo.c_cflag |= CS8;
		break;
	default:
		termioInfo.c_cflag |= CS8;
		break;
	}

	termioInfo.c_cc[VTIME] = 0;
	termioInfo.c_cc[VMIN] = 1;

	tcflush(pM->fd,TCIFLUSH);

	if((tcsetattr(pM->fd,TCSANOW,&termioInfo))!=0)
		printf("serial set error");
}

static void devSetParity(struct serialDev* pM, unsigned int parity)
{
	struct termios termioInfo = {0};

	if(tcgetattr(pM->fd,&termioInfo)!=0)
		return;

	switch(parity)
	{
	case E_2_PARITY:
		termioInfo.c_cflag |= PARENB;
		termioInfo.c_cflag |=PARODD;
		termioInfo.c_iflag |=(INPCK | ISTRIP);
		break;

	case E_1_PARITY:
		termioInfo.c_iflag |=(INPCK | ISTRIP);
		termioInfo.c_cflag |=PARENB;
		termioInfo.c_cflag &= ~PARODD;
		break;

	case E_0_PARITY:
		termioInfo.c_cflag &= ~PARENB;
		break;
	default:
		termioInfo.c_cflag &= ~PARENB;
	}

	tcflush(pM->fd,TCIFLUSH);

	if((tcsetattr(pM->fd,TCSANOW,&termioInfo))!=0)
		printf("serial set error");
}

static int devIsRunning(serialDev_t* pM)
{
	return pM->bRcvStarted;
}

static void devFree(serialDev_t* pM)
{
	if (pM)
	{
		if (pM->bRcvStarted)
		{
			pM->bRcvStarted = false;
			pthread_join(pM->rcvTaskId, NULL);
		}

		if (pM->bProcStarted)
		{
			pM->bProcStarted = false;
			pM->pListM->end(pM->pListM);
			pthread_join(pM->proTaskId, NULL);
		}

		if (pM->pRcvBuf)
			pM->pRcvBuf->free(pM->pRcvBuf);

		if (pM->pListM)
			pM->pListM->free(pM->pListM);

		close(pM->fd);
		free(pM);
	}
}

static int parseSerialData(serialDev_t* pM)
{
	char keyword[4] = "@";	//start with '@' and end with '@'

	while (pM->pRcvBuf->readAvailable(pM->pRcvBuf) >= 0)
	{
		int availableBytes = pM->pRcvBuf->readAvailable(pM->pRcvBuf);
		char temp[128] = "";
		if (availableBytes > sizeof(temp))
			availableBytes = sizeof(temp);

		pM->pRcvBuf->clone(pM->pRcvBuf, temp, availableBytes);

		char* pEnd = strstr(temp, keyword);
		if (NULL == pEnd)
			break;

		unsigned int needBytes = pEnd - temp + strlen(keyword);
		nodeM_t* pNodeM = NULL;

		if (pM->pRcvBuf->readAvailable(pM->pRcvBuf) < needBytes)
			break;

		unsigned int dataAreaLen = pEnd - temp;
		pNodeM = nodeMMalloc(dataAreaLen);
		dataAreaM_t* pDataAreaM = pNodeM->data(pNodeM);

		pM->pRcvBuf->read(pM->pRcvBuf, temp, needBytes);

		memcpy(pDataAreaM->data(pDataAreaM), temp, dataAreaLen);

		pM->pListM->add2Tail(pM->pListM, pNodeM);
	}

	return 0;
}

static void* rcvTask(void* pArg)
{
	usleep(1*1000*1000);
	printf("%s\r\n", __FUNCTION__);
	serialDev_t* pM = (serialDev_t*)pArg;
	if (NULL == pM)
		return NULL;

#define TEMP_SIZE	(1500)
	char temp[TEMP_SIZE] = "";

	fd_set rdFd;

	while (pM->bRcvStarted)
	{
        struct timeval timeout = {0};
        timeout.tv_sec = 10;

    	FD_ZERO(&rdFd);
    	FD_SET(pM->fd, &rdFd);

        int ret = select(pM->fd + 1, &rdFd, NULL, NULL, &timeout);
        if (ret == 0)	//time out
        {
        	pM->pRcvBuf->clear(pM->pRcvBuf);
        	continue;
        }
        else if (ret < 0)	//Error happens with socket
        	break;

        int bytes = read(pM->fd, temp, TEMP_SIZE);
        if (bytes <= 0)	// Error happened
        	break;

        temp[bytes] = '\0';

        if (pM->pRcvBuf->writeAvailable(pM->pRcvBuf) < bytes)
        	continue;

        pM->pRcvBuf->write(pM->pRcvBuf, temp, bytes);

        parseSerialData(pM);
	}

	printf("disconnect<%d>\r\n", pM->fd);
	pM->bRcvStarted = false;
	close(pM->fd);
	return NULL;
}

static void* procTask(void* pArg)
{
	serialDev_t* pM = (serialDev_t*)pArg;
	if (NULL == pM)
		return NULL;

	while (pM->bProcStarted)
	{
		pM->pListM->wait(pM->pListM);

		nodeM_t* pNodeM = NULL;
		while (NULL != (pNodeM = pM->pListM->takeFirst(pM->pListM)))
		{
			dataAreaM_t* pDataAreaM = pNodeM->data(pNodeM);
			pM->serialProcess(pDataAreaM->data(pDataAreaM), pDataAreaM->size(pDataAreaM));
			pNodeM->free(pNodeM);
		}
	}
}

unsigned int serialDevMalloc(const char* devName, _serialProcess process)
{
	serialDev_t* pM = NULL;
	do
	{
		if (NULL == devName)
			break;

		pM = (serialDev_t *)calloc(1, sizeof(serialDev_t));
		if (NULL == pM)
			break;

		pM->pRcvBuf = ringBufMMalloc(RING_BUF_SIZE);
		if (NULL == pM->pRcvBuf)
			break;

		pM->pListM = listMMalloc();
		if (NULL == pM->pListM)
			break;

		pM->fd = open(devName, O_RDWR|O_NOCTTY|O_NONBLOCK);//以阻塞的方式打开
		if (pM->fd < 0)
		{
			printf("%s open failed\r\n", devName);
			break;
		}

		pM->free = devFree;
		pM->send = devSend;
		pM->setBaud = devSetBaud;
		pM->setDataBits = devSetDataBits;
		pM->setParity = devSetParity;
		pM->setStopBits = devSetStopBits;
		pM->isRunning = devIsRunning;
		pM->serialProcess = process;

		pM->bProcStarted = true;
		pthread_create(&pM->proTaskId, NULL, procTask, pM);

		pM->bRcvStarted = true;
		pthread_create(&pM->rcvTaskId, NULL, rcvTask, pM);

		return (SERIAL_DEV_ID)pM;
	}while (0);

	if (pM)
	{
		if (pM->pListM)
			pM->pListM->free(pM->pListM);

		if (pM->pRcvBuf)
			pM->pRcvBuf->free(pM->pRcvBuf);

		free(pM);
	}

	return 0;
}

static void devMFree(struct serialDevM* pM)
{
	serialDev_t* pDev = (serialDev_t *)pM->devID;
	if (pDev)
	{
		pDev->free(pDev);
	}

	free(pM);
}

static int devMIsRunning(struct serialDevM* pM)
{
	serialDev_t* pDev = (serialDev_t *)pM->devID;
	return pDev->isRunning(pDev);
}

static int devMSend(struct serialDevM* pM, void* pData, unsigned int length)
{
	if (NULL == pData || length < 0)
		return -1;

	serialDev_t* pDev = (serialDev_t *)pM->devID;
	if (pDev)
		return pDev->send(pDev, pData, length);

	return -1;
}

serialDevM_t* serialDevMMalloc(const char* devName, baudRate_e rate,
		dataBits_e dataBits, stopBits_e stopBits,parity_e parity, _serialProcess process)
{
	serialDevM_t* pM = NULL;
	do
	{
		if (NULL == devName)
			break;

		pM = (serialDevM_t *)calloc(1, sizeof(serialDevM_t));
		if (NULL == pM)
			break;

		pM->devID = (SERIAL_DEV_ID)serialDevMalloc(devName, process);
		if (pM->devID == 0)
			break;

		serialDev_t* pDev = (serialDev_t *)pM->devID;
		pDev->setBaud(pDev, rate);
		pDev->setDataBits(pDev, dataBits);
		pDev->setStopBits(pDev, stopBits);
		pDev->setParity(pDev, parity);

		pM->free = devMFree;
		pM->send = devMSend;
		pM->isRunning = devMIsRunning;

		return pM;
	}while (0);

	if (pM)
		free(pM);

	return NULL;
}
