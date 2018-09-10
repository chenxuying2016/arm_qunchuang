#ifdef __WIN32__
#include <winsock2.h>
#include <winsock.h>
#include <wininet.h>
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
#else
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include "client.h"
#include "rwini.h"
#include "recvcmd.h"
#include "pgos/MsOS.h"
#include "packsocket/packsocket.h"
#include "json/cJSON.h"
#include "util/debug.h"
#include "xmlparse/xmlparser.h"
#include "loop/loop.h"
#include "comStruct.h"
#include "pgDB/pgLocalDB.h"
#include "threadpool/threadpool.h"
#include "fpgaFunc.h"
#include "fpgaRegister.h"
#include "net_general_command.h"

#include <errno.h>

#define ENABLE_FILE_SYNC_DEBUG_INFO		(0)

static int  heartBackTimerId;
static loop_list_t s_recv_file_list = { 0 };

//ARM程序版本
#define ARM_SW_ID1   2
#define ARM_SW_ID2   0
#define ARM_SW_ID3   6

#ifdef ENABlE_OTP_BURN
//#define ARM_SW_DESC  "OTM8019A, ILI9806E, HX8394, ICN9706, HTB050W592"
#define ARM_SW_DESC  "QC: 9367_cabc14_mipi_all"

#else
#define ARM_SW_DESC  "QC: 9367" "-OTP_disable"
#endif


//version info
#define HARDWAREVER     "ETS-4011 R0.0.1"
#define SOFTWAREVER     "QC ETS"
#define RELEASEDATE     __DATE__
#define RELEASETIME     __TIME__


void recv_file_list_init()
{
    loop_create(&s_recv_file_list, 10, 0);
}

void recv_file_list_insert(void *pData)
{
    loop_push_to_tail(&s_recv_file_list, pData);
}

static int searchRecvConn(const void *precvConn, const void *precv)
{
    void* src = NULL;
	void* dst = NULL;
	
    if(!precvConn || !precv)
    {
        return 0;
    }
	
    src = precvConn;
    dst = precv;
	
    if(src == dst)
    {
        return 1;
    }
	
    return 0;
}

void *recv_file_list_get(void *pData)
{
    void  *p = loop_search( &s_recv_file_list, pData, searchRecvConn );
    return p;
}

void recv_file_list_del(void *pData)
{
    void  *p = loop_search( &s_recv_file_list, pData, searchRecvConn );
    loop_remove(&s_recv_file_list,p);
}

static cliService_t cliservice = { 0 };

cliService_t* get_client_service_data()
{
	return &cliservice;
}


void client_register(char *pSoftVer,char *pHardVer,char *pgCode)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
	
    sync_info_t sync_info;
    read_sync_config(&sync_info);
	
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "syncStatus",sync_info.syncProcess);
    cJSON_AddNumberToObject(root, "syncTimeStamp",sync_info.timeStamp);
    cJSON_AddStringToObject(root, "pgCode",pgCode);
    cJSON_AddStringToObject(root, "hardVersion",pHardVer);
    cJSON_AddStringToObject(root, "softVersion",pSoftVer);
	
    char *out=cJSON_Print(root);
    cJSON_Delete(root);

	printf("client_register: send msg CLIENT_MSG_REGISTER.\n");
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_REGISTER, ipnum, 0, 0, (unsigned char*)out, strlen(out));
    free(out);
}

static int find_file_from_local_db(char *pFileName,int u32FileSize,int u32FileTime)
{
    local_file_info_t *plocal_file_info = localDBGetRec(pFileName);
	
    //通过比较去判断是否更新文件
    if(plocal_file_info)
    {
        if( (plocal_file_info->fileTime == u32FileTime)
			&& (plocal_file_info->fileSize == u32FileSize) )
        {

            return 1;
        }
        else
        {
        	#if ENABLE_FILE_SYNC_DEBUG_INFO
            printf("File Diff: %s, time: %d-%d, size: %d-%d\n", pFileName, plocal_file_info->fileTime, u32FileTime, 
					u32FileSize, plocal_file_info->fileSize);
			#endif
        }
    }
    else
    {
    	#if ENABLE_FILE_SYNC_DEBUG_INFO
        printf("File %s is not exit.\n", pFileName);
		#endif
    }
	
    return 0;
}

int client_getFileFromSet(void *infoSet)
{
    recv_file_info_t* recv_file_info = 0;
    recv_file_info_set_t *recv_file_info_set = infoSet;
	
    while(recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set))
    {
        if(!find_file_from_local_db(recv_file_info->rfiName, recv_file_info->rfiSize, recv_file_info->rfiTime))
        {
            break;
        }
		
        recv_file_info_set->fn_add_file_index(recv_file_info_set);
    }
	
    if(recv_file_info)
    {
    	#if 1
		printf("Req: get file %s.\n", recv_file_info->rfiName);
		client_getFile(recv_file_info->rfiName, infoSet);
		#else
        char *pFileName = recv_file_info->rfiName;
        socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
		
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "fileName", pFileName);
        char *out=cJSON_Print(root);
        cJSON_Delete(root);
		
        pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_SYNCFILE, infoSet, 0, 0, (unsigned char*)out,strlen(out));

		free(out);
		#endif
		
		//printf("---------------------------11111\n");
        return 0;
    }
	
	return -1;
}

void client_getFile(char *pFileName,void *infoSet)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "fileName", pFileName);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_SYNCFILE, infoSet, 0, 0, (unsigned char*)out, strlen(out));
    free(out);
}

void client_syncStatu()
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    sync_info_t sync_info;
    read_sync_config(&sync_info);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "syncStatus",sync_info.syncProcess);
    cJSON_AddNumberToObject(root, "syncTimeStamp",sync_info.timeStamp);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);

	//printf("client_syncStatu: cliservice.send_fd = %d.\n", cliservice.send_fd);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SYNCPROC,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_syncFinish()
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    int ret = pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SYNCFIN,ipnum,0,0,0,0);
    write_cur_module_name();
    load_cur_module();
}

void client_sendEdid()
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SENDEDID,ipnum,0,0,0,0);
}

void client_sendVcom(int channel,int vcom, int otp,int flick)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "vcom", vcom);
	cJSON_AddNumberToObject(item, "otp", otp);
	cJSON_AddNumberToObject(item, "flick", flick);

	printf("client_sendVcom: flick: %d.\n", flick);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SENDVCOM,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_sendOtp(int channel, int otp_result, int max_otp_times)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "otp_result", otp_result);
	cJSON_AddNumberToObject(item, "max_otp_times", max_otp_times);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_OTP, ipnum, 0, 0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_send_id_Otp_end(int channel, int result, int max_otp_times) 
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "result", result);	
	cJSON_AddNumberToObject(item, "max_otp_times", max_otp_times);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_WRITE_ID_OTP_INFO, ipnum, 0, 0,(unsigned char*)out,strlen(out));
	free(out);
}


void client_end_rgbw(int channel, int rgbw, double x, double y, double Lv) //完成, GAMMA白平衡, 2017-5-17
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "rgbw", rgbw);
	cJSON_AddNumberToObject(item, "x", x);
	cJSON_AddNumberToObject(item, "y", y);
	cJSON_AddNumberToObject(item, "Lv", Lv);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_RGBW,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_end_gamma_write(int channel, int write_type)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "write_type", write_type);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_GAMMA,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_end_read_gamma(int channel, int len, unsigned char* data, int otpp, int otpn, int read_type)
{
	char str_data[1000];
	memset(str_data, 0, sizeof(str_data));
	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];
		sprintf(cc, "%02X;", data[i]);
		strcat(str_data, cc);
	}

	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "data", str_data);
	cJSON_AddNumberToObject(item, "otpp", otpp);
	cJSON_AddNumberToObject(item, "otpn", otpn);
	cJSON_AddNumberToObject(item, "read_type", read_type);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_READ_GAMMA, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}

void client_end_gamma_otp(int channel, char* msg)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "msg", msg);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_GAMMA_OTP,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_end_gamma_otp_info(int channel, int error_no, int vcom, int flick, 
									double f_x, double f_y, double f_lv)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "error_no", error_no);
	cJSON_AddNumberToObject(item, "vcom", vcom);
	cJSON_AddNumberToObject(item, "flick", flick);
	cJSON_AddNumberToObject(item, "x", f_x);
	cJSON_AddNumberToObject(item, "y", f_y);
	cJSON_AddNumberToObject(item, "lv", f_lv);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	printf("client_end_gamma_otp_info: x: %f, y: %f, lv: %f.\n", f_x, f_y, f_lv);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_GAMMA_OTP, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}


void client_end_realtime_control(int channel, int len, unsigned char* data) //完成, 操作寄存器, 2017-5-22
{
	char str_data[1000];
	memset(str_data, 0, sizeof(str_data));
	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];
		sprintf(cc, "%02X;", data[i]);
		strcat(str_data, cc);
	}
	//
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "data", str_data);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_REALTIME_CONTROL,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_end_firmware_upgrade(int upgrade_type, int upgrade_state) //完成, 操作寄存器, 2017-5-22
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "upgrade_type", upgrade_type);
	cJSON_AddNumberToObject(item, "upgrade_state", upgrade_state);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_UPGRADE_END, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}

//unsigned char current_type = 0;	
//				   0x01: 1a;
//				   0x02: 0.1a;
//				   0x03: 0.01a;
// aucreadbuf[20]: 0x04: 1ma; 0x001a;
//				   0x05: 0.1ma;
void client_sendPower(int channel, sByPtnPwrInfo *pwrInfo, unsigned char current_type)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "Channel",channel);

    cJSON_AddNumberToObject(item, "VDD",  pwrInfo->VDD);
    cJSON_AddNumberToObject(item, "VDDh", pwrInfo->VDDh);
    cJSON_AddNumberToObject(item, "VDDl", pwrInfo->VDDl);
    cJSON_AddNumberToObject(item, "iVDD", pwrInfo->iVDD);
    cJSON_AddNumberToObject(item, "iVDDh",pwrInfo->iVDDh);
    cJSON_AddNumberToObject(item, "iVDDl",pwrInfo->iVDDl);

    cJSON_AddNumberToObject(item, "VDDIO", pwrInfo->VDDIO);
    cJSON_AddNumberToObject(item, "VDDIOh",pwrInfo->VDDIOh);
    cJSON_AddNumberToObject(item, "VDDIOl",pwrInfo->VDDIOl);
    cJSON_AddNumberToObject(item, "iVDDIO",pwrInfo->iVDDIO);
    cJSON_AddNumberToObject(item, "iVDDIOh",pwrInfo->iVDDIOh);
    cJSON_AddNumberToObject(item, "iVDDIOl",pwrInfo->iVDDIOl);

    cJSON_AddNumberToObject(item, "ELVDD", pwrInfo->ELVDD);
    cJSON_AddNumberToObject(item, "ELVDDh",pwrInfo->ELVDDh);
    cJSON_AddNumberToObject(item, "ELVDDl",pwrInfo->ELVDDl);
    cJSON_AddNumberToObject(item, "iELVDD",pwrInfo->iELVDD);
    cJSON_AddNumberToObject(item, "iELVDDh",pwrInfo->iELVDDh);
    cJSON_AddNumberToObject(item, "iELVDDl",pwrInfo->iELVDDl);

    cJSON_AddNumberToObject(item, "ELVSS", pwrInfo->ELVSS);
    cJSON_AddNumberToObject(item, "ELVSSh",pwrInfo->ELVSSh);
    cJSON_AddNumberToObject(item, "ELVSSl",pwrInfo->ELVSSl);
    cJSON_AddNumberToObject(item, "iELVSS",pwrInfo->iELVSS);
    cJSON_AddNumberToObject(item, "iELVSSh",pwrInfo->iELVSSh);
    cJSON_AddNumberToObject(item, "iELVSSl",pwrInfo->iELVSSl);

    cJSON_AddNumberToObject(item, "VBL", pwrInfo->VBL);
	cJSON_AddNumberToObject(item, "iVBL",pwrInfo->iVBL);

	#ifdef ENABLE_POWER_OLD_VERSION
    cJSON_AddNumberToObject(item, "VBLh", pwrInfo->VBLh);
    cJSON_AddNumberToObject(item, "VBLl", pwrInfo->VBLl);    
    cJSON_AddNumberToObject(item, "iVBLh", pwrInfo->iVBLh);
    cJSON_AddNumberToObject(item, "iVBLl", pwrInfo->iVBLl);
	#else	
    cJSON_AddNumberToObject(item, "VBLh", 0);
    cJSON_AddNumberToObject(item, "VBLl", 0);    
    cJSON_AddNumberToObject(item, "iVBLh", 0);
    cJSON_AddNumberToObject(item, "iVBLl", 0);
	#endif

    cJSON_AddNumberToObject(item, "VSP", pwrInfo->VSP);
    cJSON_AddNumberToObject(item, "VSPh",pwrInfo->VSPh);
    cJSON_AddNumberToObject(item, "VSPl",pwrInfo->VSPl);
    cJSON_AddNumberToObject(item, "iVSP",pwrInfo->iVSP);
    cJSON_AddNumberToObject(item, "iVSPh",pwrInfo->iVSPh);
    cJSON_AddNumberToObject(item, "iVSPl",pwrInfo->iVSPl);

    cJSON_AddNumberToObject(item, "VSN", pwrInfo->VSN);
    cJSON_AddNumberToObject(item, "VSNh",pwrInfo->VSNh);
    cJSON_AddNumberToObject(item, "VSNl",pwrInfo->VSNl);
    cJSON_AddNumberToObject(item, "iVSN",pwrInfo->iVSN);
    cJSON_AddNumberToObject(item, "iVSNh",pwrInfo->iVSNh);
    cJSON_AddNumberToObject(item, "iVSNl",pwrInfo->iVSNl);

    cJSON_AddNumberToObject(item, "MTP", pwrInfo->MTP);
    cJSON_AddNumberToObject(item, "MTPh",pwrInfo->MTPh);
    cJSON_AddNumberToObject(item, "MTPl",pwrInfo->MTPl);
    cJSON_AddNumberToObject(item, "iMTP",pwrInfo->iMTP);
    cJSON_AddNumberToObject(item, "iMTPh",pwrInfo->iMTPh);
    cJSON_AddNumberToObject(item, "iMTPl",pwrInfo->iMTPl);
	
    cJSON_AddNumberToObject(item, "current_type", current_type);

    char *out=cJSON_Print(item);
    cJSON_Delete(item);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_SENDPOW,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_sendReg(int regAddr,int regValue)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "regAddr",regAddr);
    cJSON_AddNumberToObject(root, "regValue",regValue);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SENDREG,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_sendRegStatus(int regAddr,int regStatu)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "regAddr",regAddr);
    cJSON_AddNumberToObject(root, "regStatu",regStatu);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_REGSTAT,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_updateStatus()
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_UPDSTAT,ipnum,0,0,0,0);
}

#if 1
void client_rebackHeart()
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;

	if (cliservice.send_fd != -1)
	{
		int ret = pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_REBACK, ipnum, 0, 0, 0, 0);
		if (ret != 0)
		{
			shutdown(cliservice.send_fd, SHUT_RDWR);
			cliservice.send_fd = -1;
		}
	}

    //DBG("reback the heart %d",time(NULL));
}
#endif

void client_rebackPower(int powerOn)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "whatInfo",1); //power = 1
    cJSON_AddNumberToObject(root, "powerOn",powerOn);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_POWER_STATUS<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_rebackError(int errorNo)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "errorNo", errorNo); //power = 1
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_ERROR_INFO<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_rebackMipiCode(char *pMipiCode,int len)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;

    if(len>0)
    {
        int i,lastNo;
        int packetLen = pSock_cmd_class->getPackageMaxLen();
        int validDataLen  = len;
        int remainDataLen = len;
        lastNo = validDataLen/packetLen;
        for(i=0;i<lastNo+1;i++)
        {
            int sendLen = 0;
            if(remainDataLen>=packetLen)
            {
                sendLen = packetLen;
            }
            else
            {
                sendLen = remainDataLen;
            }
            pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_MIPI_CODE<<16)|CLIENT_MSG_REBACK,ipnum,i,lastNo,(unsigned char*)&pMipiCode[i*packetLen],sendLen);
            remainDataLen -= sendLen;
        }
    }
}

void client_rebackFlick(char flick,char polar)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "curflick",flick);
    cJSON_AddNumberToObject(root, "curpolar",polar);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_FLICK_INFO<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_rebackautoFlick(int channel,int index,int autoflick)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "index",index); //VCOM
    cJSON_AddNumberToObject(root, "autoflick",autoflick); //FLICK
	cJSON_AddNumberToObject(root, "Channel",channel); //xujie 哪一路屏
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_AUTO_FLICK<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
    free(out);
}

void client_rebackFlickOver(int channel, int vcom, int flick)
{
    socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;

	printf("client_rebackFlickOver: vcom: %d, flick: %d.\n");
	
#if 0 //原始的
    int ipnum = 0;
    pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_FLICK_OVER<<16)|CLIENT_MSG_REBACK,ipnum,0,0,0,0);
#else
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "Channel", channel);
	cJSON_AddNumberToObject(root, "vcom", vcom);
	cJSON_AddNumberToObject(root, "flicker", flick);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,(REBACK_FLICK_OVER<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
#endif
}

void client_sendPowerVersion(unsigned int channel, unsigned int version) //电源版本 2017-4-26
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "channel", channel);
	cJSON_AddNumberToObject(root, "version", version);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_POWER_VER,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

int client_send_cabc_info_ack(unsigned int channel, unsigned short cabc_freq, unsigned short cabc_duty)
{
	client_send_gen_cmd_ack_read_cabc_end(&cliservice.socketCmdClass, cliservice.send_fd, channel, 
											cabc_freq, cabc_duty);
}

void client_send_scan_result(void* pData, unsigned int length)
{
	if (length == 0)
		return;

	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "gen_cmd_id", E_NET_GEN_CMD_SCAN_RESULT);
	cJSON_AddStringToObject(root, "gen_cmd_tag", "scan result");
	cJSON_AddStringToObject(root, "bar_code", (char *)pData);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);

	client_send_gen_cmd_data(pSock_cmd_class, cliservice.send_fd ,out ,strlen(out) + 1);
	free(out);
}

void client_send_flick_read_result(unsigned int channel, float value)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "gen_cmd_id", E_NET_GEN_CMD_CA310_READ_FLICK_RESULT);
	cJSON_AddStringToObject(root, "gen_cmd_tag", "flick read result");
	cJSON_AddNumberToObject(root, "channel", channel);
	cJSON_AddNumberToObject(root, "flicker", value);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);

	client_send_gen_cmd_data(pSock_cmd_class, cliservice.send_fd ,out ,strlen(out) + 1);
	free(out);
}

void client_sendBoxVersion(unsigned int channel, unsigned int version) //控制盒版本 2017-4-26
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "channel", channel);
	cJSON_AddNumberToObject(root, "version", version);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd,CLIENT_MSG_BOX_VER,ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_lcd_read_reg_ack(int channel, int len, unsigned char* data)
{
	char str_data[1024 * 2];
	memset(str_data, 0, sizeof(str_data));
	
	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];
		
		if (i != 0 && i % 16 == 0)
		{
			sprintf(cc, "\r\n");
			strcat(str_data, cc);
		}		
		
		sprintf(cc, "%02X ", data[i]);
		strcat(str_data, cc);

		
	}

	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "read_data", str_data);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_LCD_READ_REG, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}

void client_flick_test_ack(int channel, int vcom, double flick)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "mipi_channel", channel);
	cJSON_AddNumberToObject(item, "vcom", vcom);
	cJSON_AddNumberToObject(item, "flick", flick);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_FLICK_TEST, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}

void client_get_vcom_otp_info_ack(int channel, unsigned char vcom_otp_h, unsigned char vcom_otp_l, unsigned char vcom_otp_times)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "vcom_otp_h", vcom_otp_h);
	
	cJSON_AddNumberToObject(item, "vcom_otp_l", vcom_otp_l);
	cJSON_AddNumberToObject(item, "vcom_otp_times", vcom_otp_times);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_READ_VCOM_OTP_INFO, ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_get_id_otp_info_ack(int channel, unsigned char id1, unsigned char id2, unsigned char id3, 
								unsigned char otp_times)
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "id1", id1);	
	cJSON_AddNumberToObject(item, "id2", id2);
	cJSON_AddNumberToObject(item, "id3", id3);
	cJSON_AddNumberToObject(item, "otp_times", otp_times);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_READ_ID_OTP_INFO, ipnum,0,0,(unsigned char*)out,strlen(out));
	free(out);
}

static int fill_host_addr(struct sockaddr_in *host, char * host_ip_addr, int port)
{
    if( port <= 0 || port > 65535)
	{
		printf("fill_host_addr error: Invalid port = %d.\n", port);
		return -1;
	}
	
    memset(host, 0, sizeof(struct sockaddr_in));
    host->sin_family = AF_INET;
	
    if(inet_addr(host_ip_addr) != -1)
    {
        host->sin_addr.s_addr = inet_addr(host_ip_addr);
    }
    else
    {
        struct hostent * server_hostent = NULL;
        if( (server_hostent = gethostbyname(host_ip_addr) ) != 0)
        {
            memcpy(&host->sin_addr, server_hostent->h_addr, sizeof(host->sin_addr));
        }
        else
        {
        	printf("fill_host_addr error: gethostbyname error! ip = %s.\n", host_ip_addr);
            return -2;
        }
    }
	
    host->sin_port = htons(port);
    return 1;
}

static int xconnect(struct sockaddr_in* saddr, int type)
{
    int set =0;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0)
    {
    	printf("xconnect error: create socket failed!\n");
        return -1;
    }

    //connect to the server
    if (connect(s, (struct sockaddr *)saddr, sizeof(struct sockaddr_in)) < 0)
    {
		close(s);
        printf("xconnect error: Can't connect to server %s\n", inet_ntoa(saddr->sin_addr));
        return -1;
    }
	
    return s;
}


#if 0
#define NETLINK_OK  1
int get_netlink_status(void/*const char *if_name*/)// if_name 是网卡名称，如 eth0
{
	// -1 -- error , details can check errno
	// 1 -- interface link up
	// 0 -- interface link down.
	
	int skfd;
	struct ifreq ifr;
	struct ethtool_value edata;
	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "eth0"/*if_name*/, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char *) &edata;
	if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) == 0)
	{
		printf("get_netlink_status error: create socket failed!\n");
		return -1;
	}
	
	if(ioctl( skfd, SIOCETHTOOL, &ifr ) == -1)
	{
		printf("get_netlink_status error: SIOCETHTOOL failed!\n");
		close(skfd);
		return -1;
	}
	
	close(skfd);
	return edata.data;
}
#endif

static void* server_start(void *data);

static unsigned int s_server_ipaddr = 0;

unsigned int get_server_ip()
{
	return s_server_ipaddr;
}

void set_server_ip(unsigned int new_ip)
{
	s_server_ipaddr = new_ip;
}

void close_all_socket()
{
	printf("close_all_socket ...\n");

	if(cliservice.send_fd != -1)
	{
		printf("close send socket!\n");
		//close(cliservice.send_fd);
		shutdown(cliservice.send_fd, SHUT_RDWR);
		cliservice.send_fd = -1;
	}
	
	if(cliservice.client_socket != -1)
	{
		printf("close client socket!\n");
		//close(cliservice.client_socket);
		shutdown(cliservice.client_socket, SHUT_RDWR);
		cliservice.client_socket = -1;
	}

	#if 0
	if (cliservice.server_fd != -1)
	{
		close(cliservice.server_fd);
		cliservice.server_fd = -1;
	}
	
	if (cliservice.muticast_fd != -1)
	{
		close(cliservice.muticast_fd);
		cliservice.muticast_fd = -1;
	}
	
	if (cliservice.listen_socket != -1)
	{
		close(cliservice.listen_socket);
		cliservice.listen_socket = -1;
	}	
	#endif
}

static void * server_guard(void* data)
{
    cliService_t *pcliSrv = (struct cliService_t*)data;
    while(pcliSrv->guard_working)
    {
		pthread_mutex_lock(&pcliSrv->m);
		
		pcliSrv->time_alive_timeout ++;
		
		if(pcliSrv->time_alive_timeout >= 5)
		{
			
			if (pcliSrv->link_on)
			{
				set_server_ip(0);

				// close all connect.
				close_all_socket();
				
				pcliSrv->link_on = 0;

				printf("server_guard: net link is break, close all socket!\n");
			}
		}
		
		pthread_mutex_unlock(&pcliSrv->m);
		
        sleep(1);
    }
	
    DBG("left the thread:server_guard\n");
	pthread_exit(0);
    return NULL;
}

static void* client_connect(void *data)
{
	cliService_t *pcliSrv = (cliService_t*)data;
	
	while(pcliSrv->srv_working)
	{
		//PG上位机IP地址
		while(pcliSrv->srv_working)
		{
			if(get_server_ip() > 0) 
				break;
			
			usleep(1000 * 200);
		}

		struct sockaddr_in pgServer;
		fill_host_addr(&pgServer, cliservice.server_ip, cliservice.server_port);
		cliservice.send_fd = xconnect(&pgServer, 1);
		if(cliservice.send_fd == -1)
		{
			printf("client_connect: xconnect failed!\n");
			
			cliservice.send_fd = -1;
			usleep(1000 * 200);
			continue;
		}

		printf("client_connect: connect to server[%s:%d] ok ...\n", cliservice.server_ip, cliservice.server_port);

		// send client register message.
		{
			//软件版本 ////////////////////////////////////////////////////////////////////////////////
			char swTxt[256] = "";

			char descTxt[100] = "";
			sprintf(descTxt, "%s\r\n", ARM_SW_DESC);
			strcat(swTxt, descTxt);

			//Feb 10 2017:16:14:47
			char datetimeTxt[100] = "";
			sprintf(datetimeTxt, "%s %s\r\n", __DATE__, __TIME__);
			strcat(swTxt, datetimeTxt);

			//arm:1.1.15
			char armTxt[100] = "";
			sprintf(armTxt, "arm: %d.%d.%d\r\n", ARM_SW_ID1, ARM_SW_ID2, ARM_SW_ID3);
			strcat(swTxt, armTxt);

			//fpga:1.0.0.3
			char fpgaTxt[100] = "";
			unsigned int version = 0;
			
			if(g_fpga_reg_addr != NULL) 
				version = FPGA_DDR_REGC;
			
			sprintf(fpgaTxt, "fpga: %d.%d.%d.%d\r\n", (version>>12)&0xf, (version>>8)&0xf, (version>>4)&0xf, (version)&0xf);
			strcat(swTxt, fpgaTxt);

			//pwr:01.01.05
			extern unsigned int pwrSoftwareVersion;
			if(pwrSoftwareVersion > 0)
			{
				unsigned int ver = pwrSoftwareVersion;
				char pwrTxt[100];
				sprintf(pwrTxt, "pwr: %d.%d.%d\r\n", (ver>>16)&0xff, (ver>>8)&0xff, (ver)&0xff);
				strcat(swTxt, pwrTxt);
			}

			//box:1.0.5
			unsigned int box_version = 0;
			#ifdef ENABLE_CONTROL_BOX
			box_version = box_get_version();
			#endif
			
			if(box_version > 0)
			{
				char boxTxt[100];
				sprintf(boxTxt, "box: %d.%d.%d\r\n", (box_version>>16)&0xff, (box_version>>8)&0xff, (box_version)&0xff);
				strcat(swTxt, boxTxt);
			}

			// hardware version:
			char hwTxt[256];
			sprintf(hwTxt, "%s %s\r\nOSD5018: %d.%d.%d\r\n", __DATE__, __TIME__, 0, 0, 1);

			client_register(swTxt, hwTxt, "OSD5018");
			/*
			软件版本
			arm:01.01.15
			pwr:01.01.05
			Feb 10 2017:16:14:47
			fpga:0.10.0.2

			硬件版本
			OSD5018.0.0.1.
			Feb 10 2017:16:14:47
			*/
		}

		//检测读是否正常
		char buf[1024] = "";
		
		while(pcliSrv->srv_working)
		{
			int ret = recv(cliservice.send_fd, buf, sizeof(buf), 0);
			if(ret <= 0)
			{
				printf("client_connect error: recv error.\n");
				break;
			}
			
		}
		
		printf("client_connect: server is shutdown!\n");
		if(cliservice.send_fd != -1)
		{
			close(cliservice.send_fd);
			cliservice.send_fd = -1;
			//printf("client_connect: task end, close send_fd.\n");
		}
		
		set_server_ip(0);
	}
	
	pthread_exit(0);
	return NULL;
}

// route add -net 224.0.0.0 netmask 255.255.255.0 eth0
// route del -net 224.0.0.0 netmask 255.255.255.255 eth0
static void* client_mutilcast(void *data)
{
    cliService_t *pcliSrv = (cliService_t*)data;
    struct sockaddr_in local_addr;
    int err = -1;
	
	while(pcliSrv->mutil_working)
	{
		pcliSrv->muticast_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (pcliSrv->muticast_fd == -1)
		{
			printf("client_mutilcast error: create socket failed!\n");

			pcliSrv->muticast_fd = 0;
			usleep(500*1000);
			continue;
		}
		
		setsockopt(pcliSrv->muticast_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&err, sizeof(err));

		memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		local_addr.sin_port = htons(MCAST_PORT);
		err = 1;
		err = bind(pcliSrv->muticast_fd,(struct sockaddr*)&local_addr, sizeof(local_addr)) ;
		if(err < 0)
		{
			printf("client_mutilcast error: bind socket failed!\n");
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			close(fd);
			usleep(500*1000);
			continue;
		}
		
		int loop = 1;
		err = setsockopt(pcliSrv->muticast_fd, IPPROTO_IP, IP_MULTICAST_LOOP,&loop, sizeof(loop));
		if(err < 0)
		{
			printf("client_mutilcast error: set socketopt(IP_MULTICAST_LOOP) failed!\n");
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			close(fd);
			usleep(500*1000);
			continue;
		}

		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		err = setsockopt(pcliSrv->muticast_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));
		if (err < 0)
		{
			printf("client_mutilcast error: set socketopt(IP_ADD_MEMBERSHIP) failed! errno = %d.\n", errno);
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			close(fd);
			usleep(500*1000);
			continue;
		}

		//
		int addr_len = 0;
#define BUFF_SIZE 256
		char buff[BUFF_SIZE];
		int n = 0;
		
		while(pcliSrv->mutil_working)
		{
			addr_len = sizeof(local_addr);
			memset(buff, 0, BUFF_SIZE);
			n = recvfrom(pcliSrv->muticast_fd, buff, BUFF_SIZE, 0, (struct sockaddr*)&local_addr,&addr_len);
			if( n <= 0)
			{
				printf("client_mutilcast error: recvfrom failed!\n");
				break;
			}

			// Just for debug
			#ifdef SINGLE_IP_DEBUG
			{
				in_addr_t filter_addr = inet_addr("192.168.1.11");
				if (local_addr.sin_addr.s_addr != filter_addr)
				{
					//printf("client_mutilcast: recv other server msg: %s.\n", inet_ntoa(local_addr.sin_addr));
					continue;
				}
			}
			#endif				
			//printf("recv mcast data: from %s.\n", inet_ntoa(local_addr.sin_addr));

			pthread_mutex_lock(&pcliSrv->m);
			
			strcpy(pcliSrv->server_ip, inet_ntoa(local_addr.sin_addr));
			set_server_ip(local_addr.sin_addr.s_addr);

			if (pcliSrv->link_on == 0)
			{
				printf("client_mutilcast: link is on!");
			}
			
			pcliSrv->link_on = 1;
			pcliSrv->time_alive_timeout = 0;
			
			pthread_mutex_unlock(&pcliSrv->m);
			//printf("client_multicast: recv msg, update server ip: %s. %d\n", pcliSrv->server_ip, get_server_ip() );
		}
		
		if(pcliSrv->muticast_fd > 0)
		{
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			err = setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,&mreq, sizeof(mreq));
			close(fd);
		}
		
		printf("client_mutilcast: current loop end\n");
	}
	
	pthread_exit(0);
	return NULL;
}

static void* server_start(void *data)
{
    cliService_t *pcliSrv = (cliService_t*)data;
    int ret,len;
	
#if thread_pool
    struct threadpool *pool = threadpool_init(10, 20);
#endif

	unsigned char* arg = (unsigned char*)malloc(FIFO_MAX_LEN/2);

	while(pcliSrv->srv_working)
	{
		pcliSrv->listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if(pcliSrv->listen_socket == -1)
		{
			printf("server_start error: create socket failed!\n");
			usleep(500*1000);
			continue; 
		}

		/* Socket Reuse */
		ret = 1;
		setsockopt(pcliSrv->listen_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&ret, sizeof(ret));

		/* Bind Socket */
		struct sockaddr_in addr;
		memset(&addr,0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons( pcliSrv->local_port );
		ret = bind( pcliSrv->listen_socket, (struct sockaddr*)&addr, sizeof( addr ) );
		if(ret < 0)
		{
			printf("server_start error: bind socket failed!\n");
			close(pcliSrv->listen_socket);
			pcliSrv->listen_socket = -1;
			usleep(500*1000);
			continue;
		}

		//try to listen
		if( listen( pcliSrv->listen_socket, 1 ) <0 )
		{
			printf("server_start error: listen failed!\n");
			close(pcliSrv->listen_socket);
			pcliSrv->listen_socket = -1;
			usleep(500*1000);
			continue;
		}
		
		printf("server_start: listening on %d ...\n", pcliSrv->local_port );

		while(pcliSrv->srv_working)
		{
			len = sizeof(addr);
			pcliSrv->client_socket = accept(pcliSrv->listen_socket, (struct sockaddr *)&addr, &len);
			if(pcliSrv->client_socket < -1)
			{
				printf("server_start error: accept failed!\n");
				break;
			}

			printf("server_start: New Client: %s:%d.\n", inet_ntoa(addr.sin_addr), addr.sin_port);

			socket_cmd_class_t *pSock_cmd_class = &pcliSrv->socketCmdClass;
			socket_cmd_t *pSocketCmd = 0;

			while(pcliSrv->srv_working)
			{
				ret = recv(pcliSrv->client_socket, arg, FIFO_MAX_LEN/2, 0);
				if(ret <= 0)
				{
					printf("server_start: server_start->recv Error: %d.\n", ret);
					break;
				}
				else
				{
					pSock_cmd_class->recvSocketCmd(pSock_cmd_class, arg, ret);
					
					while(pcliSrv->srv_working)
					{
						//printf("pSock_cmd_class->parseSocketCmd: 1591. recv len = %d.\n", ret);
						pSocketCmd = pSock_cmd_class->parseSocketCmd(pSock_cmd_class);
						if(pSocketCmd)
						{
#if thread_pool
							if((pSocketCmd->cmd == SERVER2CLI_MSG_SHUTON)
								|| (pSocketCmd->cmd == SERVER2CLI_MSG_SYNCTIME))
							{
								threadpool_add_job(pool, msgProc, pSocketCmd);
							}
							else
							{
								ret = msgProc(pSocketCmd);
							}
#else
							ret = msgProc(pSocketCmd);
#endif
						}
						else
						{
							break;
						}
					}

				}
			}

			printf("server_start: Client is disconnected!\n");
			if(pcliSrv->client_socket != -1)
			{
				close(pcliSrv->client_socket);
				pcliSrv->client_socket = -1;
			}

		}

		if(pcliSrv->listen_socket != -1)
		{
			close(pcliSrv->listen_socket);
			pcliSrv->listen_socket = -1;
			printf("server_start: close org_server_fd socket.\n");
		}
	}
	
#if thread_pool
	threadpool_destroy(pool);
#endif

	free(arg);

	printf("server_start: thread end!\n");
	pthread_exit(0);
    return NULL;
}

static void load_config(cliService_t* cli)
{
    struct XML_DATA* xml = xml_load("config.xml");

    if(!xml)
	{
		printf("load_config error: xml_load failed!\n");
    	return;
	}
	
    int terminal_log = xml_readnum(xml, "client_terminal_log");
    int file_log = xml_readnum(xml, "client_file_log");
	
    debug_set_dir( xml_readstr(xml, "client_file_log:directory") );
    if( terminal_log )
        debug_term_on();
    else
        debug_term_off();
    if( file_log )
        debug_file_on();
    else
        debug_file_off();
    xml_free(xml);
}

#if 1
static void heartBackTimerCallback(MS_U32 u32Timer)
{
    client_rebackHeart();
}
#endif

void client_init()
{
    int result;
	
    //init win32 socket
    #ifdef __WIN32__
    static WSADATA wsa_data;
    result = WSAStartup((WORD)(1<<8|1), &wsa_data); //初始化WinSocket动态连接库
    if( result != 0 ) // 初始化失败
        return -1;
    #endif
	
    memset(&cliservice,0,sizeof(cliservice));
    load_config(&cliservice);
    cliservice.server_port = SERVER_CLIENT_PORT; //send to server;
    cliservice.local_port  = CLIENT_SERVER_PORT; //local monitor;
    cliservice.srv_timeout = 3;
	
    result = pthread_mutex_init(&cliservice.m, 0);

    init_socket_cmd_class(&cliservice.socketCmdClass, PACKAGE_MAX_LEN,FIFO_MAX_LEN);

	#if 0
    MsOS_CreateEventHanderList(&cliservice.pEventHanderList);
    MsOS_AddEventHanderToList(cliservice.pEventHanderList,ListendServerEvHandler, MS_TRUE);
    MsOS_AddEventHanderToList(cliservice.pEventHanderList,LostServerEvHandler, MS_TRUE);
	#endif
	
    cliservice.mutil_working = 1;

	#if 1
    cliservice.guard_working = 1;
    result  = pthread_create(&cliservice.thread_guard, 0, (void*)server_guard, (void*)&cliservice );
	#endif
	
    result |= pthread_create(&cliservice.thread_muticast, 0, (void*)client_mutilcast, (void*)&cliservice );

	cliservice.srv_working = 1;
	result |= pthread_create(&cliservice.thread_server, 0, (void*)server_start, (void*)&cliservice );
	result |= pthread_create(&cliservice.thread_connect, 0, (void*)client_connect, (void*)&cliservice );

    recv_file_list_init();

	#if 1
	int enable_timer = 1;
    heartBackTimerId = MsOS_CreateTimer (heartBackTimerCallback, 0, 1000, enable_timer, "heartBackTimer");
	#endif
	
	if(1)
	{
		//软件版本 ////////////////////////////////////////////////////////////////////////////////
		char swTxt[256] = "";

		//Feb 10 2017:16:14:47
		char datetimeTxt[100];
		sprintf(datetimeTxt, "%s %s\r\n", __DATE__, __TIME__);
		strcat(swTxt, datetimeTxt);

		//arm:1.1.15
		char armTxt[100];
		sprintf(armTxt, "arm: %d.%d.%d\r\n", ARM_SW_ID1, ARM_SW_ID2, ARM_SW_ID3);
		strcat(swTxt, armTxt);

		printf("\nSoftwareVersion: \n");
		printf("%s \n", swTxt);
	}
}

void client_destory()
{
    cliservice.mutil_working = 0;
    cliservice.guard_working = 0;
    cliservice.srv_working = 0;
	
	//把关闭套接字放在前面来
	if (cliservice.client_socket != -1)
		close(cliservice.client_socket);
	
	if (cliservice.muticast_fd != -1)
		close(cliservice.muticast_fd);

	if (cliservice.listen_socket != 1)
		close(cliservice.listen_socket);

	if (cliservice.send_fd != -1)
		close(cliservice.send_fd);
	
    pthread_join(cliservice.thread_guard, NULL);
    pthread_join(cliservice.thread_muticast, NULL);
    pthread_join(cliservice.thread_server, NULL);

	//原来关闭套接字在这个地方
	MsOS_StopTimer(heartBackTimerId);
	MsOS_DeleteTimer(heartBackTimerId);
	
	//MsOS_DestoryEventHanderList(cliservice.pEventHanderList);
	
	free(cliservice.socketCmdClass.cmd_fifo.cmd);
	
    #ifdef __WIN32__
        WSACleanup();
    #endif
}


