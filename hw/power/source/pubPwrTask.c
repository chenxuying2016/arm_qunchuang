#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <semaphore.h>
#include "pgos/MsOS.h"
#include "common.h"
#include "comStruct.h"
#include "pubpwr.h"
#include "comUart.h"

extern pthread_mutex_t gPwrReadLock;  // 电源串口锁

static MS_APARTMENT *pwrApartMent = 0;
static MS_U32 pwrTimerId;
static volatile int pwrCmdPause = 0;
volatile int gPowerUpdateState = 0; //0-没有升级， 1-正在升级
unsigned int pwrSoftwareVersion = 0; //电源程序版本
//
static volatile int pwrSoftwareVersion_request = 1; //开机请求版本
static sem_t        pwrSoftwareVersion_sem;
static pthread_t    pwrSoftwareVersion_pthread;


INT pwrSetOnOff(BOOLEAN bOnoff, WORD16 wChannel)
{
#if 1
    tPwrMsgHead pwrmsg;
    BYTE ucCmd = 0;

    if(0 == bOnoff)
    {
        ucCmd = PWR_CMD_POWER_OFF;
    }
    else
    {
        ucCmd = PWR_CMD_POWER_ON;
    }
    memset(&pwrmsg, 0, sizeof(tPwrMsgHead));
    pwrmsg.cmd = ucCmd;
    pwrmsg.type = MSG_TYPE_POWER;
    pwrmsg.signl_type = 0x03;
    pwrmsg.ways = wChannel;
    pwrmsg.len = sizeof(tPwrMsgHead) - 4; //headlen = 4
    MsOS_SendMessage(pwrApartMent->MessageQueue,ucCmd, (MS_U32)&pwrmsg,sizeof(pwrmsg));
#else
	tPwrMsgHead* pwrmsg = (tPwrMsgHead*)malloc(sizeof(tPwrMsgHead));
	BYTE ucCmd = 0;

	if(0 == bOnoff)
	{
		ucCmd = PWR_CMD_POWER_OFF;
	}
	else
	{
		ucCmd = PWR_CMD_POWER_ON;
	}
	memset(pwrmsg, 0, sizeof(tPwrMsgHead));
	pwrmsg->cmd = ucCmd;
	pwrmsg->type = MSG_TYPE_POWER;
	pwrmsg->signl_type = 0x03;
	pwrmsg->ways = wChannel;
	pwrmsg->len = sizeof(tPwrMsgHead) - 4; //headlen = 4
	MsOS_SendMessage(pwrApartMent->MessageQueue,ucCmd, (MS_U32)pwrmsg,sizeof(tPwrMsgHead));
#endif

    /*if(bOnoff)
    {
        MsOS_StartTimer(pwrTimerId);
    }
    else
    {
        MsOS_StopTimer(pwrTimerId);
    }*/
    return 0;
}

#if 0
int pwrUpgrade(unsigned char* buffer, unsigned int length) //xujie 2017-4-24
{
	MsOS_PostMessage(box_message_queue_get(),PWR_CMD_UPGRADE,(MS_U32)buffer, length);
    //MsOS_PostMessage(pwrApartMent->MessageQueue, PWR_CMD_UPGRADE, (MS_U32)buffer, length);
    return 0;
}
#endif

INT pwrEnableMonitor(int enable)
{
    if(enable)
    {
        MsOS_StartTimer(pwrTimerId);
    }
    else
    {
        MsOS_StopTimer(pwrTimerId);
    }
}

//set power user setting
INT pwrSetInfo(void *pData)
{
    BYTE cmdBuf[1024];

    tPwrMsgHead PwrHead;

    PwrHead.cmd = PWR_CMD_CFG_POWER_STRUCT;
    PwrHead.type = 0x05;
    PwrHead.len = sizeof(tPwrMsgHead)+sizeof(s1103PwrCfgDb)-4;
    PwrHead.dest_addr = 0xffff;
    PwrHead.ways = 0xff;

    PwrHead.result = 0;

    memcpy(cmdBuf, &PwrHead, sizeof(tPwrMsgHead));
    memcpy(cmdBuf + sizeof(tPwrMsgHead), pData, sizeof(s1103PwrCfgDb));
    // 采用发消息的方式
    MsOS_SendMessage(pwrApartMent->MessageQueue,PWR_CMD_CFG_POWER_STRUCT, cmdBuf, sizeof(tPwrMsgHead)+sizeof(s1103PwrCfgDb));

    return SUCCESS;
}

//get info
#if 0 //原始的
INT pwrgetInfo(void *pData)
{
	if(pwrCmdPause) return;
    tPwrMsgHead *pPwrHead = (tPwrMsgHead*)malloc(sizeof(tPwrMsgHead));
    pPwrHead->cmd = PWR_CMD_GET_VOL;
    pPwrHead->type = 0x05;
    pPwrHead->len = sizeof(tPwrMsgHead)-4;
    pPwrHead->dest_addr = 0xffff;
    pPwrHead->ways = 0xff;
    pPwrHead->result = 0;
    // 采用发消息的方式
    MsOS_PostMessage(pwrApartMent->MessageQueue,PWR_CMD_GET_VOL, pPwrHead, sizeof(tPwrMsgHead));
    return SUCCESS;
}
#else
INT pwrgetInfo(WORD16 wChannel)
{
	if(pwrCmdPause) return;
	tPwrMsgHead *pPwrHead = (tPwrMsgHead*)malloc(sizeof(tPwrMsgHead));
	memset(pPwrHead, 0, sizeof(tPwrMsgHead));
	pPwrHead->cmd = PWR_CMD_GET_VOL;
	pPwrHead->type = 0x05;
	pPwrHead->len = sizeof(tPwrMsgHead)-4;
	pPwrHead->dest_addr = 0;//0xffff;
//	pPwrHead->ways = 0xff;
	pPwrHead->ways = wChannel;
	pPwrHead->result = 0;
	// 采用发消息的方式
	MsOS_PostMessage(pwrApartMent->MessageQueue,PWR_CMD_GET_VOL, pPwrHead, sizeof(tPwrMsgHead));
	return SUCCESS;
}
#endif

// action 
// packet format:
// header[20] + data[21 - 27].
// 1: calibration.
// 2: burn data.
int power_cmd_calibration_power_item(WORD16 wChannel, unsigned char power_type, unsigned char action, 
										int power_data)
{
	if(pwrCmdPause) 
		return;

	unsigned char direction = 0;
	unsigned char buf[128] = { 0 };
	int data_len = 0;
	
	tPwrMsgHead *pPwrHead = (tPwrMsgHead*)buf;
	pPwrHead->type = MSG_TYPE_POWER;
	pPwrHead->cmd = PWR_CMD_POWER_CALIBRATION;	
	pPwrHead->len = sizeof(tPwrMsgHead)- 4 + 8;
	pPwrHead->dest_addr = 0;
	pPwrHead->ways = wChannel;	// 1: all; 2: slave; 3: master.
	pPwrHead->result = 0;

	switch(wChannel)
	{
		case 2:
			printf("slave channel[2]\n");
			break;

		case 3:
			printf("master channel[3]\n");
			break;

		default:
			printf("Invalid power channel = %d, set to master channel[3]\n", wChannel);
			wChannel = 3;
			break;
	}

	switch(power_type)
	{
		case 1:
			printf("V VDD.\n");
			break;

		case 2:
			printf("V VDDIO.\n");
			break;

		case 3:
			printf("V ELVDD.\n");
			break;

		case 4:
			printf("V ELVSS.\n");
			break;

		case 5:
			printf("V VBL.\n");
			break;

		case 6:
			printf("V VSP.\n");
			break;

		case 7:
			printf("V VSN.\n");
			break;

		// current
		case 8:
			printf("I VDD.\n");
			break;

		case 9:
			printf("I VDDIO.\n");
			break;

		case 10:
			printf("I ELVDD.\n");
			break;

		case 11:
			printf("I ELVSS.\n");
			break;

		case 12:
			printf("I VBL.\n");
			break;

		case 13:
			printf("I VSP.\n");
			break;

		case 14:
			printf("I VSN.\n");
			break;
		
		default:
			printf("*** Unknow Power Type ***.\n");
			break;
	}

	// type
	buf[20] = 0x00;
	buf[21] = power_type;
	buf[22] = action;

	if (power_data >= 0)
	{		
		direction = 1;
		//printf(" >= 0: power_data = %d, direction = %d.\n", power_data, direction);
	}
	else
	{		
		direction = 2;
		power_data = abs(power_data);
		//printf(" < 0: power_data = %d, direction = %d.\n", power_data, direction);
	}
	
	buf[23] = direction;
	
	printf("power_cmd_calibration_power_item: channel = %d, type = %d, action = %d, direction = %d, data = %d.\n",
			wChannel, power_type, action, direction, power_data);

	memcpy(buf + 24, &power_data, sizeof(power_data));

	data_len = pPwrHead->len + 4;
	MsOS_SendMessage(pwrApartMent->MessageQueue, PWR_CMD_POWER_CALIBRATION, buf, data_len);
	
	return SUCCESS;
}


//vdim
int  pwrSetVdim(WORD vdim) //发现没有使用
{
    BYTE cmdBuf[1024], *pTmp = NULL;
    tPwrMsgHead PwrHead;
    PwrHead.type = MSG_TYPE_POWER;
    PwrHead.signl_type = PWR_MSG_FROM_LOCAL;
    PwrHead.ways = 0xff;
    pTmp = cmdBuf;
    PwrHead.cmd = PWR_CMD_ADJUST_VDIM;
    PwrHead.len = sizeof(WORD16) + sizeof(tPwrMsgHead) + 1 - 4;
    pTmp = pTmp + sizeof(tPwrMsgHead);
    *pTmp++ = 0x01;
    memcpy(pTmp, &vdim, sizeof(WORD16));
    MsOS_PostMessage(pwrApartMent->MessageQueue,PWR_CMD_ADJUST_VDIM, cmdBuf, sizeof(WORD16) + sizeof(tPwrMsgHead) + 1);
}

//Pwm
int  pwrSetPwm(WORD pwmlevel,WORD freq,BYTE duty) //发现没有使用
{
    BYTE cmdBuf[1024], *pTmp = NULL;
    tPwrMsgHead PwrHead;
    bzero(cmdBuf, sizeof(cmdBuf));
    PwrHead.cmd  = PWR_CMD_ADJUST_PWR_FREQ;
    PwrHead.type = MSG_TYPE_POWER;
    PwrHead.len  = sizeof(WORD)*2 + 1 + sizeof(tPwrMsgHead) - 4;
    PwrHead.ways = 0;
    memcpy(cmdBuf,&PwrHead,sizeof(tPwrMsgHead));
    pTmp = cmdBuf + sizeof(tPwrMsgHead);
    memcpy(pTmp, &pwmlevel, sizeof(WORD16));
    pTmp = pTmp + sizeof(WORD16);
    memcpy(pTmp, &freq, sizeof(WORD16));
    pTmp = pTmp + sizeof(WORD16);
    memcpy(pTmp, &duty, sizeof(BYTE));
    MsOS_PostMessage(pwrApartMent->MessageQueue,PWR_CMD_ADJUST_PWR_FREQ, cmdBuf, sizeof(WORD)*2 + 1 + sizeof(tPwrMsgHead));
}

//invert
int  pwrSetInvert(WORD invert) //发现没有使用
{
    BYTE cmdBuf[1024],*pTmp;
    tPwrMsgHead PwrHead;
    PwrHead.cmd = PWR_CMD_ADJUST_INVERT;
    PwrHead.len = sizeof(WORD16) + sizeof(tPwrMsgHead)  - 4;
    memcpy(cmdBuf,&PwrHead,sizeof(tPwrMsgHead));
    pTmp = cmdBuf + sizeof(tPwrMsgHead);
    memcpy(pTmp, &invert, sizeof(WORD16));
    MsOS_PostMessage(pwrApartMent->MessageQueue,PWR_CMD_ADJUST_INVERT, cmdBuf, sizeof(WORD16) + sizeof(tPwrMsgHead) + 1);
}

//adjust
int pwrSetAdjust(sByPtnPwrInfo *pPtnPwrInfo) //发现没有使用
{
    BYTE cmdBuf[1024],*pTmp;
    tPwrMsgHead PwrHead;
    PwrHead.cmd = PWR_CMD_ADJUST_BY_PTN;
    PwrHead.type = MSG_TYPE_POWER;
    PwrHead.signl_type = PWR_MSG_FROM_LOCAL;
    PwrHead.ways = 0xff;
    PwrHead.len = sizeof(sByPtnPwrInfo) + sizeof(tPwrMsgHead) - 4;
    memcpy(cmdBuf,&PwrHead,sizeof(tPwrMsgHead));
    pTmp = cmdBuf + sizeof(tPwrMsgHead);
    memcpy(pTmp, pPtnPwrInfo, sizeof(sByPtnPwrInfo));
    MsOS_PostMessage(pwrApartMent->MessageQueue,PWR_CMD_ADJUST_BY_PTN, cmdBuf, sizeof(sByPtnPwrInfo) + sizeof(tPwrMsgHead));
}

//vddorvbl
int pwrSetVol() //发现没有使用
{
    BYTE cmdBuf[1024];
    tPwrMsgHead PwrHead;
    PwrHead.cmd  = PWR_CMD_SET_VOL;
    PwrHead.type = MSG_TYPE_POWER;
    PwrHead.len  = sizeof(PwrVddVblSet) + sizeof(tPwrMsgHead) - 4;
    PwrHead.ways = 0;
    //pVddVbl = (PwrVddVblSet *)(buf + sizeof(tPwrMsgHead));
    //memcpy(pVddVbl, &(pState->VddVblset), sizeof(PwrVddVblSet));
    MsOS_PostMessage(pwrApartMent->MessageQueue,MSG_TYPE_POWER, cmdBuf,sizeof(tPwrMsgHead) + sizeof(PwrVddVblSet));
}

void userPwrUpgrade(unsigned char* file_buffer, unsigned int file_size) //xujie 电源升级 2017-4-24
{
	//读取文件到内存
/*	FILE* fp = fopen(filename, "rb");
	if(fp == NULL) return;
	fseek(fp, 0, SEEK_END);
	unsigned int file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* file_buffer = (unsigned char*)malloc(file_size);
	fread(file_buffer, file_size, 1, fp);
	fclose(fp);*/

	//
	int UartNo;
	unsigned char type;
	pthread_mutex_t* lock;
// 	if(1)
// 	{
// 		UartNo = getBoxUartNo();
// 		lock = &gBoxReadLock;
// 		type = 0x50;
// 	}
// 	else
// 	{
// 		extern pthread_mutex_t gPwrReadLock;
		UartNo = getPwrUartNo();
		type = 0x05;
		lock = &gPwrReadLock;
//	}
	pthread_mutex_lock(lock/*&gPwrReadLock*/);

	//
	int ret;
	unsigned int num;
	unsigned char txt[1500];
	//开始升级(低位在前，高位在后), 04 00 = 0x0004
	//pSocketCmdInfo->crc = crc32_le(CRCPOLY_LE,pSocketCmdInfo->pValid,pSocketCmdInfo->length);
	num = 0;
	// 	txt[num++] = 0x55;
	// 	txt[num++] = 0xaa;
	//
	// 	txt[num++] = 0x10; //报文大小
	// 	txt[num++] = 0x0;
	//
	// 	txt[num++] = 0x7e;
	// 	txt[num++] = 0x7e;
	//
	txt[num++] = 0x72; //72开始升级   73数据包
	txt[num++] = type; //05电源， 0B控制盒
	//
	txt[num++] = 0x4; //长度(低位在前，高位在后)
	txt[num++] = 0x0;
	//
	txt[num++] = 0x0; //根据上面的长度来设定的数据
	txt[num++] = 0x0;
	txt[num++] = 0x0;
	txt[num++] = 0x0;
	//
	// 	txt[num++] = 0x17; //CRC32
	// 	txt[num++] = 0x87;
	// 	txt[num++] = 0xa6;
	// 	txt[num++] = 0xcd;
	//
	// 	txt[num++] = 0x7e;
	// 	txt[num++] = 0x7e;
	//pthread_mutex_lock(lock/*&gBoxReadLock*/);
	ret = uartSendMsgToBoard(UartNo, txt, num);
	//pthread_mutex_unlock(lock/*&gBoxReadLock*/);
	if(ret != 0) printf("72 %02X error\n", type);
	printf("power upgrade mode...\n");
	sleep(4);

	//数据报文
	unsigned int offset = 0;
	unsigned char* buf = file_buffer;
	unsigned int len = file_size;
	while(len > 0)
	{
		unsigned int datalen = 0;
		if(len >= 1024)
		{
			datalen = 1024;
		}
		else
		{
			datalen = len;
		}

		//
		num = 0;
		// 		txt[num++] = 0x55;
		// 		txt[num++] = 0xaa;
		//
		// 		unsigned short packlen = /*7e 7e*/2  +  /*73 0B*/2  + /*整个报文数据长度[0c 04]*/2  + /*在升级文件中的偏移位置*/4  +  /*本包升级文件块长度*/4  +  /*升级文件总大小*/4  +  /*升级文件块数据*/datalen  +  /*CRC[b6 3c a6 af]*/4  +  /*7e 7e*/2;
		// 		txt[num++] = (packlen) & 0xff; //包长
		// 		txt[num++] = (packlen>>8) & 0xff;
		//
		// 		txt[num++] = 0x7e;
		// 		txt[num++] = 0x7e;
		//数据包开始
		txt[num++] = 0x73; //73=数据包
		txt[num++] = type; //0B=控制盒
		//2
		unsigned short packdatalen = /*升级文件数据块偏移位置*/4 + /*升级文件数据块长度*/4 + /*升级文件总大小*/4 + /*升级文件数据块长度*/datalen;
		txt[num++] = (packdatalen) & 0xff; //整个包内容数据长度
		txt[num++] = (packdatalen>>8) & 0xff;
		//4
		txt[num++] = (offset) & 0xff; //升级文件数据块在总文件中的偏移位置
		txt[num++] = (offset>>8) & 0xff;
		txt[num++] = (offset>>16) & 0xff;
		txt[num++] = (offset>>24) & 0xff;
		//4
		txt[num++] = (datalen) & 0xff; //升级文件数据块长度
		txt[num++] = (datalen>>8) & 0xff;
		txt[num++] = (datalen>>16) & 0xff;
		txt[num++] = (datalen>>24) & 0xff;
		//4
		txt[num++] = (file_size) & 0xff; //升级文件总大小
		txt[num++] = (file_size>>8) & 0xff;
		txt[num++] = (file_size>>16) & 0xff;
		txt[num++] = (file_size>>24) & 0xff;
		//datalen
		memcpy((&(txt[num])), buf, datalen);  //升级文件数据块
		num += datalen;
		//数据包结束
		// 		unsigned int pack_crc = ~(jcrc32_le(~0, buf, sizeof(buf)));//pSocketCmdInfo->crc = crc32_le(CRCPOLY_LE,pSocketCmdInfo->pValid,pSocketCmdInfo->length);
		// 		txt[num++] = (pack_crc) & 0xff; //CRC32
		// 		txt[num++] = (pack_crc>>8) & 0xff;
		// 		txt[num++] = (pack_crc>>16) & 0xff;
		// 		txt[num++] = (pack_crc>>24) & 0xff;
		//
		// 		txt[num++] = 0x7e;
		// 		txt[num++] = 0x7e;
		//pthread_mutex_lock(lock/*&gBoxReadLock*/);
		ret = uartSendMsgToBoard(UartNo, txt, num);
		//pthread_mutex_unlock(lock/*&gBoxReadLock*/);
		if(ret != 0) printf("73 %02X error, offset=%d\n", type, offset);
		printf("power upgrade data %d  %d\n", offset, datalen);
		usleep(800*1000);

		//
		offset += datalen; //偏移位置
		buf += datalen;
		len -= datalen;
	}

	//
	pthread_mutex_unlock(lock/*&gPwrReadLock*/);
	//free(file_buffer);

	//先清空上次的版本信息, 再次重新请求版本
	pwrSoftwareVersion = 0;
	pwrSoftwareVersion_request = 1;
	sem_post(&pwrSoftwareVersion_sem);
}

#if 1
void dump_data1(unsigned char* p_data, int data_len)
{
	int i = 0;
	for (; i < data_len; i ++)
	{
		if ((i % 16) == 0 && i != 0)
			printf("\n");

		printf("%02x ", p_data[i]);
			
	}

	printf("\n");
}

INT pwr_contorl(BOOLEAN bOnoff, WORD16 wChannel, int power_type)
{
#if 0
	// power type:
	// 1: VDD
	// 2: VDDIO
	// 3: ELVDD
	// 4: ELVSS
	// 5: VBL
	// 6: VSP
	// 7: VSN
	// 0: none.
	
	char temp_buffer[80] = "";
    tPwrMsgHead *pwrmsg = (tPwrMsgHead*)temp_buffer;
    BYTE ucCmd = 0;

	int power_control_len = 6;

    ucCmd = PWR_CMD_CTRL_SWITCH;
    //memset(pwrmsg, 0, sizeof(tPwrMsgHead));
    pwrmsg->cmd = ucCmd;
    pwrmsg->type = MSG_TYPE_POWER;
    pwrmsg->signl_type = 0x03;
    pwrmsg->ways = wChannel;	// 1: all; 2: slaver; 3: master.
    pwrmsg->len = sizeof(tPwrMsgHead) - 4 + power_control_len; //headlen = 4

	int offset = sizeof(tPwrMsgHead);
	temp_buffer[offset] = 6;	// VSP
	offset ++;
	
	if (bOnoff)
		temp_buffer[offset] = 1;
	else
		temp_buffer[offset] = 0;
	offset ++;

	dump_data1(temp_buffer, sizeof(tPwrMsgHead) + power_control_len);
	
    MsOS_SendMessage(pwrApartMent->MessageQueue,ucCmd, (MS_U32)pwrmsg, sizeof(tPwrMsgHead) + power_control_len);
#endif

    return 0;
}
#endif


static void pwrSetTimeOut(BYTE ucCmd, struct timeval *pTimeOut)
{
    switch (ucCmd)
    {
        case PWR_CMD_POWER_ON:
        case PWR_CMD_POWER_OFF:
            pTimeOut->tv_sec = 6;
            pTimeOut->tv_usec = 0;
            break;
        case PWR_CMD_CFG_POWER_STRUCT:
        case PWR_CMD_CFG_POWER_FILE:
            pTimeOut->tv_sec = 5;
            pTimeOut->tv_usec = 0;
            break;
        default:
            pTimeOut->tv_sec = 3;
            pTimeOut->tv_usec = 0;
    }

    return;
}

/*static*/ INT pwrCmdTransfer(BYTE cmd, void *pBuf, WORD16 nLen,void *pNode)
{
    INT ret = 1;
    BYTE aucreadbuf[1024] = {0};
    struct timeval delaytime;
    tPwrMsgHead *sendMsgHead = (tPwrMsgHead *)pBuf;
    int nBufLen = 0;
    pwrSetTimeOut(cmd, &delaytime);
    pthread_mutex_lock(&gPwrReadLock);

	#ifdef ENABLE_POWER_DEBUG_INFO
	printf("pwrCmdTransfer: send len = %d.\n", nLen);
	dump_data1(pBuf, nLen);
	#endif
	
    if (0 == uartSendMsgToBoard(getPwrUartNo(), pBuf, nLen))
    {
        //printf("send uart success\n");
        // OK: ret = 0;
        // Error: ret = 1;
        ret = uartWaitRecvData(getPwrUartNo(), cmd, delaytime, aucreadbuf, &nBufLen);
		#ifdef ENABLE_POWER_DEBUG_INFO
		printf("pwrCmdTransfer: recv len = %d.\n", nBufLen);
		if (ret == 0)
		{
			dump_data1(aucreadbuf, nBufLen);
		}
		#endif
		
		if (ret != 0)
			printf("pwr: uartWaitRecvData error, ret = %d.\n", ret);
		

		SMsgHead result;
        result = *(SMsgHead *)aucreadbuf;

		//printf("pwrCmdTransfer: result.type: 0x%02x, cmd: 0x%02x.\n",
		//		result.type, result.cmd);
		
        pthread_mutex_unlock(&gPwrReadLock);
        if ((result.type == 0x85) && (result.cmd == 0x04))
        {
            sByPtnPwrInfo *pPwrInfo = &aucreadbuf[21];
			unsigned char current_type = aucreadbuf[20];	
			//				   0x01: 1a;
			//				   0x02: 0.1a;
			//				   0x03: 0.01a;
			// aucreadbuf[20]: 0x04: 1ma; 0x001a;
			//				   0x05: 0.1ma;

			int channel = sendMsgHead->ways == 2 ? 1 : 2;	//电源2=通道1,  电源3=通道2
			// channel 1: left. => mipi channel 3 and 4.
			// channel 2: right. => mipi channel 1 and 2.

			// we used mipi channel 1 for CABC. ==> power channel 2.
            client_sendPower(channel, pPwrInfo, current_type); //通知PG上位机, 显示电源信息

			#ifdef ENABLE_POWER_DEBUG_INFO
			dump_data1(aucreadbuf, nBufLen);
			printf("power channel: %d. current type: %d.\n", channel, current_type);
			printf("vdd: %d, ivdd: %d.\n", pPwrInfo->VDD, pPwrInfo->iVDD);
			printf("vbl: %d, ivbl: %d.\n", pPwrInfo->VBL, pPwrInfo->iVBL);
			printf("vddio: %d, ivddio: %d.\n", pPwrInfo->VDDIO, pPwrInfo->iVDDIO);
			printf("vsp: %d, ivsp: %d.\n", pPwrInfo->VSP, pPwrInfo->iVSP);
			printf("vsn: %d, ivsn: %d.\n", pPwrInfo->VSN, pPwrInfo->iVSN);
			printf("vgh: %d, ivgh: %d.\n", pPwrInfo->ELVDD, pPwrInfo->iELVDD);
			printf("vgl: %d, ivgl: %d.\n", pPwrInfo->ELVSS, pPwrInfo->iELVSS);
			#endif

			#ifdef ENABLE_CONTROL_BOX
			boxSavePwrData(pPwrInfo); //通知控制盒,  刷新电源信息
			#endif
        }
		else if(result.cmd == PWR_CMD_GET_POWER_VER)
		{
			pwrSoftwareVersion_request = 0; //可以不用再请求版本了

			//21,22,23
			unsigned char* ver = &aucreadbuf[21];
			printf("pubPwrTask.c  power version = %d.%d.%d\n", ver[0], ver[1], ver[2]);

			//通知PG上位机, 电源程序版本
			int ver1 = ver[0] & 0xff;
			int ver2 = ver[1] & 0xff;
			int ver3 = ver[2] & 0xff;
			int version = 0;
			version = (ver1<<16) | (ver2<<8) | (ver3);
			pwrSoftwareVersion = version;
			client_sendPowerVersion(1, version);
		}
		else if (result.cmd == PWR_CMD_GET_CABC_PWM)
		{
			#if 0
			unsigned short cabc_freq = 0;
			unsigned short cabc_duty = 0;

			//if (nBufLen > 0)
			//	dump_data1(aucreadbuf, nBufLen);

			int data_index = 21;
			cabc_duty = aucreadbuf[data_index] | aucreadbuf[data_index + 1] << 8;
			cabc_freq = aucreadbuf[data_index + 2] | aucreadbuf[data_index + 3] << 8;

			printf("cabc data: 0x%02x 0x%02x 0x%02x 0x%02x.\n", aucreadbuf[data_index], aucreadbuf[data_index + 1],
					aucreadbuf[data_index + 2], aucreadbuf[data_index + 3]);
			printf("cabc freq: %d, duty: %d.\n", cabc_freq, cabc_duty);

			client_send_cabc_info_ack(1, cabc_freq, cabc_duty);
			#endif
		}
		else
		{

			#ifdef ENABLE_POWER_DEBUG_INFO
			if (nBufLen > 0)
				dump_data1(aucreadbuf, nBufLen);
			
			printf("uartWaitRecvData: ret = %d. nBufLen = %d.\n", ret, nBufLen);
			#endif
		}
    }
    else
    {
		pthread_mutex_unlock(&gPwrReadLock); //xujie
        printf("send uart failed\n");
        traceMsg(PWR_PRT_SW1, "send pwrcmd 0x%x error\d", cmd);
    }

    sendMsgHead = (tPwrMsgHead *)aucreadbuf;

    // 先应答，然后调用hook函数
    if (NULL != pNode && 0 == ret) // // pNode == NULL 表示本地消息，不需要回复，只需要串口返回值
    {
        ifnetDataSend(aucreadbuf, nBufLen, pNode);
    }

    return ret;
}

static void* pwrSoftwareVersion_Task(void* arg) //请求电源版本任务
{
	sleep(4);
	while(1)
	{
		sem_wait(&pwrSoftwareVersion_sem);
		if(pwrSoftwareVersion_request > 0)
		{
			tPwrMsgHead pwrmsg;
			BYTE ucCmd = 0;
			WORD16 wChannel = 1;
			ucCmd = PWR_CMD_GET_POWER_VER;
			memset(&pwrmsg, 0, sizeof(tPwrMsgHead));
			pwrmsg.cmd = ucCmd;
			pwrmsg.type = MSG_TYPE_POWER;
			pwrmsg.signl_type = 0x03;
			pwrmsg.ways = wChannel;
			pwrmsg.len = sizeof(tPwrMsgHead) - 4; //headlen = 4
			
			//需要请求版本
			while(pwrSoftwareVersion_request > 0)
			{
				sleep(2);
				MsOS_SendMessage(pwrApartMent->MessageQueue, ucCmd, (MS_U32)&pwrmsg, sizeof(pwrmsg));
			}
		}
	}
	return NULL;
}

/*static INT pwrCmdTransfer2(BYTE cmd, void *pBuf, WORD16 nLen,void *pNode)
{
    INT ret = 1;
    pthread_mutex_lock(&gPwrReadLock);
    if (0 == uartSendMsgToBoard(getPwrUartNo(), pBuf, nLen))
    {
        ;
    }
    else
    {
        printf("send uart failed\n");
        traceMsg(PWR_PRT_SW1, "send pwrcmd 0x%x error\d", cmd);
    }
    pthread_mutex_unlock(&gPwrReadLock);
    return ret;
}*/

static MS_U32 power_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message)
{
    int ret;
    unsigned char  cmd  = Message.MessageID;
    unsigned char *pBuf = Message.Parameter1;
    unsigned int  nLen  = Message.Parameter2;

    switch(Message.MessageID)
    {
    	case PWR_CMD_POWER_CALIBRATION:
		{
			#ifdef ENABLE_POWER_DEBUG_INFO
			printf("PWR_CMD_POWER_CALIBRATION: \n");
			//dump_data1(pBuf, nLen);
			#endif
			
			ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
		}
		break;
		
        case PWR_CMD_POWER_ON:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
			//free(pBuf);
        }
        break;

        case PWR_CMD_POWER_OFF:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
			//free(pBuf);
        }
        break;

		case PWR_CMD_GET_POWER_VER:
		if(pwrSoftwareVersion_request == 1)
		{
			ret = pwrCmdTransfer(cmd, pBuf, nLen, 0);
		}
		break;

        case PWR_CMD_CFG_POWER_STRUCT:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

        case PWR_CMD_GET_VOL:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
            free(pBuf);
        }
        break;

        case PWR_CMD_ADJUST_VDIM:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

        case PWR_CMD_ADJUST_PWR_FREQ:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

        case PWR_CMD_ON_OFF:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

        case PWR_CMD_SET_VOL:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

        case PWR_CMD_SET_FLY_TIME:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

        case PWR_CMD_SET_BOE_FREQ_ONOFF:
        {
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
        }
        break;

		case PWR_CMD_CTRL_SWITCH:
		{
			printf("Power Message: PWR_CMD_CTRL_SWITCH.\n");
            ret = pwrCmdTransfer(cmd, pBuf, nLen,0);
		}
		break;
		
		case PWR_CMD_GET_CABC_PWM:
		{
			printf("Power Message: PWR_CMD_GET_CABC_PWM.\n");
			ret = pwrCmdTransfer(cmd, pBuf, nLen, 0);
		}
		break;

		case PWR_CMD_UPGRADE: //xujie 电源升级 2017-4-24
		{
			unsigned char* file_buffer = (unsigned char*)Message.Parameter1;
			unsigned int file_size = (unsigned int)Message.Parameter2;
			pwrCmdPause = 1;
			//gPowerUpdateState = 1; //准备升级
			//usleep(10 * 1000);
			//MsOS_StopTimer(pwrTimerId); //先临时停止定时器
			printf("PWR_CMD_UPGRADE: buffer=%p   size=%d\n", file_buffer, file_size);
			userPwrUpgrade(file_buffer, file_size);
			//gPowerUpdateState = 0; //升级完成了
			pwrCmdPause = 0;
			//free(file_buffer);

			//升级结束后, 重新请求版本
		/*	pwrSoftwareVersion_request = 1;
			sem_post(&pwrSoftwareVersion_sem);*/
#if 0
			sleep(2);
			tPwrMsgHead pwrmsg;
			BYTE ucCmd = 0;
			WORD16 wChannel = 1;
			// 		if(0 == bOnoff)
			// 		{
			// 			ucCmd = PWR_CMD_POWER_OFF;
			// 		}
			// 		else
			// 		{
			ucCmd = PWR_CMD_GET_POWER_VER;
			//}
			memset(&pwrmsg, 0, sizeof(tPwrMsgHead));
			pwrmsg.cmd = ucCmd;
			pwrmsg.type = MSG_TYPE_POWER;
			pwrmsg.signl_type = 0x03;
			pwrmsg.ways = wChannel;
			pwrmsg.len = sizeof(tPwrMsgHead) - 4; //headlen = 4
			//MsOS_SendMessage(power_message_queue_get(),ucCmd, (MS_U32)&pwrmsg,sizeof(pwrmsg));
			pwrCmdTransfer(ucCmd, &pwrmsg, sizeof(pwrmsg), 0);
#endif
			//MsOS_StartTimer(pwrTimerId); //打开定时器
		}
		break;

        default:
        break;
    }
}

static void pwrTimerCallback(MS_U32 u32Timer)
{
	pwrgetInfo(2);
	pwrgetInfo(3);
}

INT initPwrTask(void)
{
    MS_EVENT_HANDLER_LIST *pEventHandlerList = 0;

    traceMsg(FPGA_PRT_SW1, "func %s\n", __FUNCTION__);

    pwrApartMent = MsOS_CreateApartment("pwrTask",power_message_proc,pEventHandlerList);

    pwrTimerId = MsOS_CreateTimer (pwrTimerCallback,0,1500,0,"pwrTimer");
	//请求电源版本
	sem_init(&pwrSoftwareVersion_sem, 0, /*u32InitCnt*/0);
	pthread_create(&pwrSoftwareVersion_pthread, NULL, pwrSoftwareVersion_Task, NULL);
	if(pwrSoftwareVersion_request == 1)
	{
		sem_post(&pwrSoftwareVersion_sem);
	}

    return SUCCESS;
}

unsigned int power_message_queue_get()
{
    return pwrApartMent->MessageQueue;
}
