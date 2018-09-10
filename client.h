#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "pgos/MsOS.h"
#include <pthread.h>
#include "packsocket/packsocket.h"
#include "loop/loop.h"
#include "mypath.h"
#include "vcom.h"

enum {
	E_ERROR_OK = 0,
	E_ERROR_OTP_TIMES_END = -1,
	E_ERROR_OTP_TIMES_NOT_CHANGE = -2,
	E_ERROR_OTP_DATA_WRITE_FAILED = -3,
	
};


typedef struct tag_pointer_xy_s
{
    int x;
    int y;
}pointer_xy_t;


typedef enum
{
    SYNC_FILE_ERROR,
}PG_ERROR_INFO_E;

void client_register(char *pSoftVer,char *pHardVer,char *pgCode);

int client_getFileFromSet(void *infoSet);

void client_getFile(char *pFileName, void *infoSet);

void client_syncStatu();

void client_syncFinish();

void client_sendEdid();

void client_sendVcom(int channel,int vcom, int otp,int flick);

//void client_sendPower(int channel, sByPtnPwrInfo *pwrInfo, unsigned char current_type);

void client_sendReg(int regAddr,int regValue);

void client_sendRegStatus(int regAddr,int regstatu);

void client_updateStatus();

void client_rebackHeart();

void client_rebackPower(int powerOn);

void client_sendOtp(int channel, int otp_result, int max_otp_times);

/**************pg local****************************/
typedef struct tag_dwn_file_info_s
{
    char srcFileName[256];
    char srcFileType[16];
    char dstFileName[256];
}dwn_file_info_t;

typedef struct tag_recv_file_info_s
{
    char  rfiName[256];
    int   rfiSize;
    int   rfiTime;
    unsigned char  *pFileData;
    int   actRecvSize;
}recv_file_info_t;

typedef void (*Fn_recv_file_t)(const void *);
typedef recv_file_info_t* (*Fn_get_cur_file_info_t)(void *);
typedef void (*Fn_add_file_index_t)(void *);

typedef struct tag_recv_file_info_set_s
{
    int						curNo;
    int						totalFilesNums;
    recv_file_info_t		*pRecv_file_info;
    Fn_recv_file_t			fn_recv_file;
    void					*param;
    Fn_get_cur_file_info_t	fn_get_cur_file_info;
    Fn_add_file_index_t     fn_add_file_index;
}recv_file_info_set_t;



void client_pg_setTime(int stampTime);

void client_pg_syncFile(void *param);

void client_pg_downFile(void *param);

void client_pg_shutON(int enable, char *pModelName, vcom_info_t* vcom_info, int ptn_id);

void client_pg_showPtn(int ptnId);

void client_pg_showCrossCur(pointer_xy_t *pPointer_xy,int num);

void client_pg_hideCrossCur(void);

int  client_pg_readReg(int regAddr);

void client_pg_writeReg(int regAddr,int regValue);

void client_pg_upgradeFile(char *pSrcFile,char *pFileType,char *pDstFile);
void client_pg_firmware(void *param);

void client_send_scan_result(void* pData, unsigned int length);

void client_send_flick_read_result(unsigned int channel, float value);

/**********************************************/
#define LISTENED_SERVER_EVENT  0x1000
#define LOST_SERVER_EVENT      0x1001

typedef struct tag_cliservice_s
{
    unsigned short local_port;
    unsigned short server_port;
    char       server_ip[32];
    int        srv_timeout;
    int        srv_working;
    int        mutil_working;
    int        guard_working;
    int        listen_socket;
    int        client_socket;
    int        send_fd;
    int        muticast_fd;
    pthread_t  thread_guard;
    pthread_t  thread_muticast;
    pthread_t  thread_server;
	pthread_t  thread_connect;
	pthread_t  thread_network;
	
	int is_network;
	
    pthread_mutex_t m;
	int 			link_on;	// 0: link down; 1: link on;
	int        		time_alive_timeout;
	
    MS_EVENT_HANDLER_LIST *pEventHanderList;
    socket_cmd_class_t     socketCmdClass;
}cliService_t;

#define SERVER_CLIENT_PORT  8013  //send to server port
#define CLIENT_SERVER_PORT  8014  //recv from server port

#define MCAST_PORT 8888
#define MCAST_ADDR "224.0.0.88"

int msgProc(socket_cmd_t *pSocketCmd);
void client_init();
void client_destory();

void recv_file_list_init();
void recv_file_list_insert(void *pData);
void *recv_file_list_get(void *pData);
void recv_file_list_del(void *pData);

#define thread_pool  1

#endif
