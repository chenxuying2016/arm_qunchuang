#include <stdlib.h>
#include "recvcmd.h"
#include "packsocket/packsocket.h"
#include "client.h"
#include "json/cJSON.h"
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "rwini.h"
#include "util/debug.h"
#include "pgDB/pgLocalDB.h"
#include "common.h"
#include "pubPwrMain.h"
#include "pgDB/pgDB.h"
#include "vcom.h"
#include "pubmipi.h"
#include "pubFpga.h"

// id otp times: 3
// vcom otp times: 1
#define QC_DEBUG_ENABLE			1

#if QC_DEBUG_ENABLE
// ingro error
#define ENABLE_OTP_TESE_MODE				(1)

#define QC_DEFAULT_ID_MAX_OTP_TIMES			(3)
#define QC_DEFAULT_VCOM_MAX_OTP_TIMES		(3)
#else
#define QC_DEFAULT_ID_MAX_OTP_TIMES			(0)
#define QC_DEFAULT_VCOM_MAX_OTP_TIMES		(0)
#endif

//xujie
//extern int m_channel_1; 
extern int m_channel_2;
//extern int m_channel_3;
//extern int m_channel_4;
//xujie


static char achomeDir[128];


static recv_file_info_t* get_cur_file_info(void *infoSet)
{
    recv_file_info_t *pCur = 0;
    recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)infoSet;
	
    if(recv_file_info_set->curNo < recv_file_info_set->totalFilesNums)
    {
        pCur = &recv_file_info_set->pRecv_file_info[recv_file_info_set->curNo];
    }
	
    return pCur;
}

static void add_file_index(void *infoSet)
{
    recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)infoSet;
    recv_file_info_set->curNo ++;
}

typedef struct tag_SUpdataFileList
{
    char name[256];
    int filesize;
    int hasnext; //char hasnext; xujie 原始的没有对齐
} SUpdataFileList;

static unsigned char *tmpFirmwareBuf = 0; //buf
static unsigned char *tmpFirmwareWpt = 0; //writer ptr
static unsigned int s_total_data_len = 0;

/*struct shared_use_st
{
    int written;//作为一个标志，非0：表示可读，0表示可写
    int cmd;
};

void sendFirmwareUpdataEvnet()
{
    void *shm = 0;
    struct shared_use_st *shared = 0;
    int shmid;
    //创建共享内存
    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666|IPC_CREAT);
    if(shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        return;
    }
    //将共享内存连接到当前进程的地址空间
    shm = shmat(shmid, (void*)0, 0);
    if(shm == (void*)-1)
    {
        fprintf(stderr, "shmat failed\n");
        return;
    }
    printf("Memory attached at %X\n", (int)shm);
    //设置共享内存
    shared = (struct shared_use_st*)shm;

    //数据还没有被读取，则等待数据被读取,不能向共享内存中写入文本
    while(shared->written == 1)
    {
        sleep(1);
        printf("Waiting...\n");
    }
    //向共享内存中写入数据
    printf("Enter some text: ");
    shared->cmd = 1;
    //写完数据，设置written使共享内存段可读
    shared->written = 1;

    //把共享内存从当前进程中分离
    if(shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed\n");
        return;
    }

//    sleep(2);
}*/
//控制盒升级数据
static unsigned char *tmpBoxUpdateBuf = NULL; //buf
static unsigned int   tmpBoxUpdateSize = 0;
static unsigned char *tmpBoxUpdateWpt = NULL; //writer ptr

//电源升级数据
static unsigned char *tmpPowerUpdateBuf = NULL; //buf
static unsigned int   tmpPowerUpdateSize = 0;
static unsigned char *tmpPowerUpdateWpt = NULL; //writer ptr

char m_current_module_filename[256] = "";

int parse_gamma_reg_data(char* org_data, unsigned char* p_gamma_reg_data_buf, 
							int gamma_reg_data_buf_len)
{
	char* ptr = org_data;
	int len = 0;

	if (org_data == NULL)
		return 0;
	
	while(ptr[0])
	{
		char cc[20];
		cc[0] = ptr[0];
		cc[1] = ptr[1];
		cc[2] = 0;
		
		int val = 0;
		sscanf(cc, "%02X", &val);
		p_gamma_reg_data_buf[len] = (unsigned char)(val & 0xff);
		
		len ++;		
		ptr += 3;
	}

	return len;
}

#if 0

unsigned char qc_do_sleep_out(int channel)
{
	unsigned char cmd[100] = 
							{
								0x39, 0xE0, 0x01, 0x00,
								0x39, 0x28, 0x00, 
								0xff, 0xff, 0xff, 0xff, 0xff, 0xff
							};

	SendCode(channel, cmd);

	usleep(120 * 1000);
}

unsigned char qc_do_sleep_in(int channel)
{
	unsigned char cmd[100] = 
							{
								0x39, 0xE0, 0x01, 0x00,
								0x39, 0x29, 0x00, 
								0xff, 0xff, 0xff, 0xff, 0xff, 0xff
							};

	SendCode(channel, cmd);

	usleep(120 * 1000);
}


unsigned char qc_read_otp_data_reg(int channel, unsigned short otp_index)
{
	unsigned char otp_data = 0;

	unsigned char cmd[100] = 
							{
								0x39, 0xEA, 0x01, 0x00,
								0x39, 0xEB, 0x01, 0x01,
								0x39, 0xEc, 0x01, 0x11,
								0x39, 0xEc, 0x01, 0x10,
								0xff, 0xff, 0xff, 0xff, 0xff, 0xff
							};

	cmd[3] = otp_index >> 8;
	cmd[7] = otp_index & 0xFF;
	printf("qc_read_otp_data_reg index: %x.\n", otp_index);

	SendCode(channel, cmd);

	int read_len = 1;
	int real_read_len = 0;
	unsigned char read_buffer[4] = { 0 };
	real_read_len = ReadModReg(channel, 1, 0xED, read_len, read_buffer);

	otp_data = read_buffer[0];
	printf("read data len = %d, data: %02x.\n", real_read_len, otp_data);
	
	return otp_data;
}



unsigned char qc_read_reg(int channel, unsigned char page, unsigned char reg)
{
	unsigned char reg_data = 0;

	unsigned char cmd[100] = 
						{
							0x39, 0xE0, 0x01, 0x00,
							0xff, 0xff, 0xff, 0xff, 0xff, 0xff
						};

	cmd[3] = page;
	printf("qc_read_reg: page: %d, reg: %x.\n", page, reg);

	SendCode(channel, cmd);

	int read_len = 1;
	int real_read_len = 0;
	unsigned char read_buffer[4] = { 0 };
	real_read_len = ReadModReg(channel, 1, reg, read_len, read_buffer);

	reg_data = read_buffer[0];
	printf("read data len = %d, data: %02x.\n", real_read_len, reg_data);

	
	return reg_data;
}

int qc_otp_vcom(int channel, unsigned char vcom_h, unsigned char vcom_l, int ptn_id)
{	
	printf("qc_otp_vcom.\n");
	
	// enable extend command
	unsigned char cmd_enable_extend[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					0x39, 0xE1, 0x01, 0x93,
					0x39, 0xE2, 0x01, 0x65,
					0x39, 0xE3, 0x01, 0xF8,
					
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_enable_extend);

	// E5
	unsigned char cmd_e5[100] = 
				{
					0x39, 0xE5, 0x01, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd_e5);
	
	unsigned char cmd28_10[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					
					0x05, 0x28, 0x00,
					0x05, 0x10, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd28_10);

	usleep(240 * 1000);

	// internal power
	unsigned char cmd_inter_power[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page 1
					0x39, 0x1D, 0x01, 0x01,		// vgh: 7.5
					0x39, 0x1E, 0x01, 0x03,	

					0x39, 0xE0, 0x01, 0x00,		// page 0		// fix page error.
					0x39, 0xEC, 0x01, 0x02,		// otp int vpp
					
					0x39, 0xE0, 0x01, 0x00,		// page 0
					0x05, 0x11, 0x00,			// sleep out
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	
	SendCode(channel, cmd_inter_power);

	usleep(240 * 1000);

	// write vcom reg
	unsigned char cmd_write_vcom[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page
					0x39, 0x00, 0x01, 0x01,		// vcom
					0x39, 0x01, 0x01, 0x03,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	cmd_write_vcom[7] = vcom_h;
	cmd_write_vcom[11] = vcom_l;

	printf("vcom: 0x%02x, 0x%02x.\n", cmd_write_vcom[7], cmd_write_vcom[11]);
	SendCode(channel, cmd_write_vcom);

	#ifdef ENABlE_OTP_BURN
	// enable up key
	unsigned char cmd_enable_up_key[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x39, 0x84, 0x01, 0x5A,		// up key
					0x39, 0x85, 0x01, 0xA5,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
					
	SendCode(channel, cmd_enable_up_key);
	#endif
	
	// otp index
	unsigned char cmd_otp_index[100] = 
				{
					0x39, 0xEA, 0x01, 0x00,
					0x39, 0xEB, 0x01, 0x26,		// up key
					0x39, 0xEC, 0x01, 0x03,		// prog
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	//cmd_otp_index[3] = otp_index >> 8;
	//cmd_otp_index[7] = otp_index & 0xFF;	
	printf("otp index: 0x%02x, 0x%02x.\n", cmd_otp_index[3], cmd_otp_index[7]);
	SendCode(channel, cmd_otp_index);	
	usleep(200 * 1000);
	
	// end.
	#ifdef ENABlE_OTP_BURN
	unsigned char cmd_otp_end[100] = 
				{
					0x39, 0x84, 0x01, 0x00,		// page
					0x39, 0x85, 0x01, 0x00,		// up key
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_otp_end);
	#endif
	
#if 0
	usleep(200 * 1000);
	unsigned char cmd_display_on[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x05, 0x29,	0x00,			// display on
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_display_on);
#else

	#if 1
	client_pg_shutON(0, 0, NULL, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, NULL, ptn_id);
	#else

	printf("mipi reset ...\n");
	//mipi_reset(channel);
	//mipi_reset(channel);

	mipi_reset(1);
	mipi_reset(4);
	
	usleep(1000 * 10); 

	//加载初始化代码
	memset(InitCode, 0xff, sizeof(InitCode));
	mipi_parse_init_code(m_current_module_filename, InitCode);
	SendCode(channel, InitCode);
	usleep(1000 * 10); 

	 //激活输出, 点屏
	WriteSSD2828(channel,0XB7,0X030b);
	usleep(5 * 1000);
	#endif
#endif

	// compare data.
	
	return 0;
}

int qc_otp_vcom_r(int channel, unsigned char vcom_h, unsigned char vcom_l, int ptn_id)
{
	printf("qc_otp_vcom_r.\n");
	
	// enable extend command
	unsigned char cmd_enable_extend[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					0x39, 0xE1, 0x01, 0x93,
					0x39, 0xE2, 0x01, 0x65,
					0x39, 0xE3, 0x01, 0xF8,
					
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_enable_extend);

	// E5
	unsigned char cmd_e5[100] = 
				{
					0x39, 0xE5, 0x01, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd_e5);
	
	unsigned char cmd28_10[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					
					0x05, 0x28, 0x00,
					0x05, 0x10, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd28_10);

	usleep(240 * 1000);

	// internal power
	unsigned char cmd_inter_power[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page 1
					0x39, 0x1D, 0x01, 0x01,		// vgh: 7.5
					0x39, 0x1E, 0x01, 0x03,	

					0x39, 0xE0, 0x01, 0x00,		// page 1
					0x39, 0xEC, 0x01, 0x02,		// otp int vpp
					
					0x39, 0xE0, 0x01, 0x00,		// page 0
					0x05, 0x11, 0x00,			// sleep out
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	
	SendCode(channel, cmd_inter_power);

	usleep(240 * 1000);

	// write vcom reg
	unsigned char cmd_write_vcom[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page
					0x39, 0x03, 0x01, 0x01,		// vcom
					0x39, 0x04, 0x01, 0x03,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	cmd_write_vcom[7] = vcom_h;
	cmd_write_vcom[11] = vcom_l;

	printf("vcom: 0x%02x, 0x%02x.\n", cmd_write_vcom[7], cmd_write_vcom[11]);
	SendCode(channel, cmd_write_vcom);

	#ifdef ENABlE_OTP_BURN
	// enable up key
	unsigned char cmd_enable_up_key[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x39, 0x84, 0x01, 0x5A,		// up key
					0x39, 0x85, 0x01, 0xA5,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
					
	SendCode(channel, cmd_enable_up_key);
	#endif
	
	// otp index
	unsigned char cmd_otp_index[100] = 
				{
					0x39, 0xEA, 0x01, 0x00,
					0x39, 0xEB, 0x01, 0x30,		// up key
					0x39, 0xEC, 0x01, 0x03,		// prog
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	//cmd_otp_index[3] = otp_index >> 8;
	//cmd_otp_index[7] = otp_index & 0xFF;	
	printf("otp index: 0x%02x, 0x%02x.\n", cmd_otp_index[3], cmd_otp_index[7]);
	SendCode(channel, cmd_otp_index);	
	usleep(200 * 1000);
	
	// end.
	#ifdef ENABlE_OTP_BURN
	unsigned char cmd_otp_end[100] = 
				{
					0x39, 0x84, 0x01, 0x00,		// page
					0x39, 0x85, 0x01, 0x00,		// up key
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_otp_end);
	#endif
	
#if 0
	usleep(200 * 1000);
	unsigned char cmd_display_on[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x05, 0x29,	0x00,			// display on
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_display_on);
#else

	#if 1
	client_pg_shutON(0, 0, NULL, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, NULL, ptn_id);
	#else

	printf("mipi reset ...\n");
	//mipi_reset(channel);
	//mipi_reset(channel);

	mipi_reset(1);
	mipi_reset(4);
	
	usleep(1000 * 10); 

	//加载初始化代码
	memset(InitCode, 0xff, sizeof(InitCode));
	mipi_parse_init_code(m_current_module_filename, InitCode);
	SendCode(channel, InitCode);
	usleep(1000 * 10); 

	 //激活输出, 点屏
	WriteSSD2828(channel,0XB7,0X030b);
	usleep(5 * 1000);
	#endif
#endif

	// compare data.
	
	return 0;
}


#define OTP_USED_INTERNAL_VOLT		(0)
#define VCOM_R_TEST					(1)
int qc_otp_vcom_by_mtp(int channel, unsigned char vcom_h, unsigned char vcom_l, int ptn_id)
{
	printf("qc_otp_vcom_by_mtp\n");
	
	// enable extend command
	unsigned char cmd_enable_extend[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					0x39, 0xE1, 0x01, 0x93,
					0x39, 0xE2, 0x01, 0x65,
					0x39, 0xE3, 0x01, 0xF8,
					
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_enable_extend);

	// E5
	unsigned char cmd_e5[100] = 
				{
					0x39, 0xE5, 0x01, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd_e5);
	
	unsigned char cmd28_10[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					
					0x05, 0x28, 0x00,
					0x05, 0x10, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd28_10);

	usleep(240 * 1000);

	#if OTP_USED_INTERNAL_VOLT
	// internal power
	unsigned char cmd_inter_power[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page 1
					0x39, 0x1D, 0x01, 0x01,		// vgh: 7.5
					0x39, 0x1E, 0x01, 0x03,	
					
					0x39, 0xEC, 0x01, 0x02,		// otp int vpp
					
					0x39, 0xE0, 0x01, 0x00,		// page 0
					0x05, 0x11, 0x00,			// sleep out
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_inter_power);
	#else

	mtp_power_on(channel);
	usleep(200 * 1000);
	
	unsigned char cmd_mtp_power[100] = 
				{
					//0x39, 0xE0, 0x01, 0x01,		// page 1
					//0x39, 0x1D, 0x01, 0x01,		// vgh: 7.5
					//0x39, 0x1E, 0x01, 0x03,	

					0x39, 0xE0, 0x01, 0x00,		// page 1
					0x39, 0xEC, 0x01, 0x00,		// otp extern vpp
					
					0x39, 0xE0, 0x01, 0x00,		// page 0
					0x05, 0x11, 0x00,			// sleep out
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_mtp_power);
	#endif
	

	usleep(240 * 1000);

	// write vcom reg
	#if VCOM_R_TEST
	unsigned char cmd_write_vcom[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page
					0x39, 0x03, 0x01, 0x01,		// vcom
					0x39, 0x04, 0x01, 0x03,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	cmd_write_vcom[7] = vcom_h;
	cmd_write_vcom[11] = vcom_l;
	#else
	unsigned char cmd_write_vcom[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page
					0x39, 0x00, 0x01, 0x01,		// vcom
					0x39, 0x01, 0x01, 0x03,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	cmd_write_vcom[7] = vcom_h;
	cmd_write_vcom[11] = vcom_l;
	#endif
	
	printf("vcom: 0x%02x, 0x%02x.\n", cmd_write_vcom[7], cmd_write_vcom[11]);
	SendCode(channel, cmd_write_vcom);

	#ifdef ENABlE_OTP_BURN
	// enable up key
	unsigned char cmd_enable_up_key[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x39, 0x84, 0x01, 0x5A,		// up key
					0x39, 0x85, 0x01, 0xA5,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
					
	SendCode(channel, cmd_enable_up_key);
	#endif
	
	// otp index
	#if OTP_USED_INTERNAL_VOLT
	unsigned char cmd_otp_index[100] = 
				{
					0x39, 0xEA, 0x01, 0x00,
					0x39, 0xEB, 0x01, 0x26,		// up key
					0x39, 0xEC, 0x01, 0x03,		// prog
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	//cmd_otp_index[3] = otp_index >> 8;
	//cmd_otp_index[7] = otp_index & 0xFF;	
	printf("otp index: 0x%02x, 0x%02x.\n", cmd_otp_index[3], cmd_otp_index[7]);
	SendCode(channel, cmd_otp_index);	
	usleep(200 * 1000);
	#else
	unsigned char cmd_mtp_otp_index[100] = 
				{
					0x39, 0xEA, 0x01, 0x00,
					0x39, 0xEB, 0x01, 0x26,		// up key
					0x39, 0xEC, 0x01, 0x01,		// prog
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	#if VCOM_R_TEST
	cmd_mtp_otp_index[3] = 0x00;
	cmd_mtp_otp_index[7] = 0x30;	
	#endif
	
	printf("otp index: 0x%02x, 0x%02x.\n", cmd_mtp_otp_index[3], cmd_mtp_otp_index[7]);
	SendCode(channel, cmd_mtp_otp_index);	
	usleep(20 * 1000);
	#endif
	
	// end.
	#ifdef ENABlE_OTP_BURN
	unsigned char cmd_otp_end[100] = 
				{
					0x39, 0x84, 0x01, 0x00,		// page
					0x39, 0x85, 0x01, 0x00,		// up key
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_otp_end);
	#endif

	#if !OTP_USED_INTERNAL_VOLT
	mtp_power_off(channel);
	#endif
	
#if 0
	usleep(200 * 1000);
	unsigned char cmd_display_on[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x05, 0x29,	0x00,			// display on
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_display_on);
#else

	#if 1
	client_pg_shutON(0, 0, NULL, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, NULL, ptn_id);
	#else

	printf("mipi reset ...\n");
	//mipi_reset(channel);
	//mipi_reset(channel);

	mipi_reset(1);
	mipi_reset(4);
	
	usleep(1000 * 10); 

	//加载初始化代码
	memset(InitCode, 0xff, sizeof(InitCode));
	mipi_parse_init_code(m_current_module_filename, InitCode);
	SendCode(channel, InitCode);
	usleep(1000 * 10); 

	 //激活输出, 点屏
	WriteSSD2828(channel,0XB7,0X030b);
	usleep(5 * 1000);
	#endif
#endif

	// compare data.
	
	return 0;
}

int qc_otp_id(int channel, unsigned char id1, unsigned char id2, unsigned short id3, int ptn_id)
{
	printf("ID OTP:\n");
	
	// enable extend command
	unsigned char cmd_enable_extend[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					0x39, 0xE1, 0x01, 0x93,
					0x39, 0xE2, 0x01, 0x65,
					0x39, 0xE3, 0x01, 0xF8,
					
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_enable_extend);

	// E5
	unsigned char cmd_e5[100] = 
				{
					0x39, 0xE5, 0x01, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd_e5);
	
	unsigned char cmd28_10[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,
					
					0x05, 0x28, 0x00,
					0x05, 0x10, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};

	SendCode(channel, cmd28_10);

	usleep(240 * 1000);

	// internal power
	unsigned char cmd_inter_power[100] = 
				{
					0x39, 0xE0, 0x01, 0x01,		// page 1
					0x39, 0x1D, 0x01, 0x01,		// vgh: 7.5
					0x39, 0x1E, 0x01, 0x03,	
					
					0x39, 0xEC, 0x01, 0x02,		// otp int vpp
					
					0x39, 0xE0, 0x01, 0x00,		// page 0
					0x05, 0x11, 0x00,			// sleep out
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	
	SendCode(channel, cmd_inter_power);

	usleep(240 * 1000);

	// write vcom reg
	unsigned char cmd_write_id[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x39, 0x78, 0x01, 0x01,		// id1
					0x39, 0x79, 0x01, 0x01,		// id2
					0x39, 0x7A, 0x01, 0x01,		// id3
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	cmd_write_id[7] = id1;
	cmd_write_id[11] = id2;
	cmd_write_id[15] = id3;

	printf("id: 0x%02x, 0x%02x, 0x%02x.\n", cmd_write_id[7], cmd_write_id[11], cmd_write_id[15]);
	SendCode(channel, cmd_write_id);

	// enable up key
	#ifdef ENABlE_OTP_BURN
	unsigned char cmd_enable_up_key[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x39, 0x84, 0x01, 0x5A,		// up key
					0x39, 0x85, 0x01, 0xA5,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
					
	SendCode(channel, cmd_enable_up_key);
	#endif
	
	// otp index
	unsigned char cmd_otp_index[100] = 
				{
					0x39, 0xEA, 0x01, 0x00,
					0x39, 0xEB, 0x01, 0x06,		// up key
					0x39, 0xEC, 0x01, 0x03,		// prog
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	printf("otp index: 0x%02x, 0x%02x.\n", cmd_otp_index[3], cmd_otp_index[7]);
	SendCode(channel, cmd_otp_index);	
	usleep(200 * 1000);
	
	// end.
	#ifdef ENABlE_OTP_BURN
	unsigned char cmd_otp_end[100] = 
				{
					0x39, 0x84, 0x01, 0x00,		// enable up key
					0x39, 0x85, 0x01, 0x00,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_otp_end);
	#endif

	#if 1
	usleep(200 * 1000);
	unsigned char cmd_display_on[100] = 
				{
					0x39, 0xE0, 0x01, 0x00,		// page
					0x05, 0x29,	0x00,			// display on
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff
				};
	SendCode(channel, cmd_display_on);
	#endif
	
	#if 1
	client_pg_shutON(0, 0, NULL, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, NULL, ptn_id);
	#endif

	return 0;
}


int qc_otp_data_read_id(int mipi_channel, unsigned char* p_id1, unsigned char *p_id2, unsigned char* p_id3,
						unsigned char* p_otp_times)
{
	unsigned short id_otp_index = 0x06;
	unsigned char otp_times = 0;
	unsigned char id_reg_page = 0;
	unsigned char id_otp_time_reg = 0x7C;
	
	qc_do_sleep_out(mipi_channel);

	// read id otp times.
	otp_times = qc_read_reg(mipi_channel, id_reg_page, id_otp_time_reg);
	*p_otp_times = otp_times;
	
	switch(otp_times)
	{
		case 1:
			id_otp_index = 0x06;
			break;

		case 2:
			id_otp_index = 0x0B;
			break;

		case 3:
			id_otp_index = 0x10;
			break;

		case 4:
			id_otp_index = 0x15;
			break;

		case 5:
			id_otp_index = 0x1A;
			break;

		default:
			printf("qc_otp_data_read_id: Invalid OTP Times. %d.\n", otp_times);
			qc_do_sleep_in(mipi_channel);
			return -1;
			break;
	}

	printf("id otp times: %d, otp read offset: 0x%02X.\n", otp_times, id_otp_index);
	
	
	*p_id1 = qc_read_otp_data_reg(mipi_channel, id_otp_index + 1);
	*p_id2 = qc_read_otp_data_reg(mipi_channel, id_otp_index + 2);
	*p_id3 = qc_read_otp_data_reg(mipi_channel, id_otp_index + 3);
	
	qc_do_sleep_in(mipi_channel);

	*p_otp_times = otp_times;

	return 0;
}

int qc_otp_data_read_vcom(int mipi_channel, unsigned char* p_vcom_h, unsigned char *p_vcom_l, unsigned char* p_otp_times)
{
	unsigned short vcom_otp_index = 0x26;
	unsigned char otp_times = 0;
	unsigned char vcom_reg_page = 1;
	unsigned char vcom_otp_time_reg = 0x02;
	
	qc_do_sleep_out(mipi_channel);

	// read id otp times.
	otp_times = qc_read_reg(mipi_channel, vcom_reg_page, vcom_otp_time_reg);
	*p_otp_times = otp_times;
	
	switch(otp_times)
	{
		case 1:
			vcom_otp_index = 0x26;
			break;

		case 2:
			vcom_otp_index = 0x28;
			break;

		case 3:
			vcom_otp_index = 0x2A;
			break;

		case 4:
			vcom_otp_index = 0x2C;
			break;

		case 5:
			vcom_otp_index = 0x2E;
			break;

		default:
			printf("qc_otp_data_read_vcom: Invalid OTP Times. %d.\n", otp_times);
			qc_do_sleep_in(mipi_channel);
			return -1;
			break;
	}

	printf("vcom otp times: %d, otp read offset: 0x%02X.\n", otp_times, vcom_otp_index);
	
	
	*p_vcom_h = qc_read_otp_data_reg(mipi_channel, vcom_otp_index);
	*p_vcom_l = qc_read_otp_data_reg(mipi_channel, vcom_otp_index + 1);
	
	*p_otp_times = otp_times;
	
	qc_do_sleep_in(mipi_channel);

	return 0;
}

int qc_set_vcom_value(int mipi_channel, unsigned short vcom)
{
	printf("mipi_channel: %d, vcom: %d. 0x%04X.\n", mipi_channel, vcom, vcom);
	
	unsigned char cmd[100] = {
								0x39, 0xE0, 0x01, 0x01,
								0x39, 0x00, 0x01, 0x00,
								0x39, 0x01, 0x01, 0x01,
								0xff, 0xff, 0xff, 0xff, 0xff, 0xff
							 };
	
	cmd[7] = vcom >> 8;
	cmd[11] = vcom & 0xFF;
	SendCode(mipi_channel, cmd);

	return 0;
}

#define CABC_CHANNEL_1_INPUT_PIN		(71)
#define CABC_CHANNEL_2_INPUT_PIN		(59)
#define CABC_CHANNEL_3_INPUT_PIN		(62)
#define CABC_CHANNEL_4_INPUT_PIN		(55)


int init_cabc_io_config()
{
	// id check channel 1.
	gpio_export(CABC_CHANNEL_1_INPUT_PIN);

	// id check channel 2.
	gpio_export(CABC_CHANNEL_2_INPUT_PIN);

	// id check channel 3.
	gpio_export(CABC_CHANNEL_3_INPUT_PIN);

	// id check channel 4.
	gpio_export(CABC_CHANNEL_4_INPUT_PIN);
}


int qc_cabc_test(int mipi_channel, unsigned short *p_pwm_freq, unsigned short *p_duty)
{
	unsigned int high_time = 0;
	unsigned int low_time = 0;
	unsigned int volt_status = 0;

	if (p_pwm_freq == NULL)
	{
		printf("qc_cabc_test: p_pwm_freq = NULL.\n");
		return -1;
	}

	if (p_duty == NULL)
	{
		printf("qc_cabc_test: p_duty = NULL.\n");
		return -1;
	}

	switch(mipi_channel)
	{
		case 1:	// GPIO 71. => FPGA ID_Check Channel 2.
		{	
			printf("channel: 1.\n", mipi_channel);
			unsigned short high_time_l = fpga_read(0xE0);
			unsigned short high_time_h = fpga_read(0xE1);
			unsigned short low_time_l = fpga_read(0xE4);
			unsigned short low_time_h = fpga_read(0xE5);

			high_time = (high_time_h << 16) | high_time_l;
			low_time = (low_time_h << 16) | low_time_l;
			volt_status = gpin_read_value(CABC_CHANNEL_1_INPUT_PIN);
		}
		break;

		case 2:	// gpio 59 => FPGA ID_Check Channel 3.
		{
			printf("channel: 2.\n", mipi_channel);
			unsigned short high_time_l = fpga_read(0xE6);
			unsigned short high_time_h = fpga_read(0xE7);
			unsigned short low_time_l = fpga_read(0xEA);
			unsigned short low_time_h = fpga_read(0xEB);

			high_time = (high_time_h << 16) | high_time_l;
			low_time = (low_time_h << 16) | low_time_l;
			volt_status = gpin_read_value(CABC_CHANNEL_2_INPUT_PIN);
		}
		break;

		case 3: // gpio 62 => FPGA ID_Check Channel 4.
		{
			printf("channel: 3.\n", mipi_channel);
			unsigned short high_time_l = fpga_read(0xE8);
			unsigned short high_time_h = fpga_read(0xE9);
			unsigned short low_time_l = fpga_read(0xEC);
			unsigned short low_time_h = fpga_read(0xED);

			high_time = (high_time_h << 16) | high_time_l;
			low_time = (low_time_h << 16) | low_time_l;
			volt_status = gpin_read_value(CABC_CHANNEL_3_INPUT_PIN);
		}
		break;

		case 4:	// GPIO 55 => FPGA ID_Check Channel 1.
		{
			printf("channel: 4.\n", mipi_channel);
			unsigned short high_time_l = fpga_read(0xDE);
			unsigned short high_time_h = fpga_read(0xDF);
			unsigned short low_time_l = fpga_read(0xE2);
			unsigned short low_time_h = fpga_read(0xE3);

			high_time = (high_time_h << 16) | high_time_l;
			low_time = (low_time_h << 16) | low_time_l;
			volt_status = gpin_read_value(CABC_CHANNEL_4_INPUT_PIN);
		}
		break;
		
		default:
		break;
	}

	printf("high time: %d, %#x.\n", high_time, high_time);
	printf("low time: %d, %#x.\n", low_time, low_time);

	if (high_time == 0 || low_time == 0)
	{
		printf("qc_cabc_test: Invalid high time or low time! volt_status = %d.\n", volt_status);
		if (volt_status == 0)
		{
			printf("duty: 0.\n");
			*p_duty = 0;
		}
		else
		{
			printf("duty: 100.\n");
			*p_duty = 100;
		}

		*p_pwm_freq = 0;
		
	}
	else
	{
		double f_duty = (high_time * 100.00) / (high_time + low_time);
		double f_freq = (1 * 1000 * 1000 * 1000.00F) / ((high_time + low_time) * 20);

		printf("freq: %f, duty: %f.\n", f_freq, f_duty);

		*p_pwm_freq = f_freq;
		*p_duty = f_duty;
	}
}

#endif
							
//处理pgServer发送给ARM的消息
int msgProc(socket_cmd_t *pSocketCmd)
{
    //printf("msgProc: msgid: 0x%04x, %d.\n", pSocketCmd->cmd, pSocketCmd->cmd);
	
    switch (pSocketCmd->cmd)
    {
        case SERVER2CLI_MSG_REGISTER:
        //客户端一旦登录成功，将返回时间和服务端的状态
        {
        	printf("=====: SERVER2CLI_MSG_REGISTER\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *timeStamp = cJSON_GetObjectItem(root,"timeStamp");
            cJSON *homePath  = cJSON_GetObjectItem(root,"homePath");
            //DBG("::%s",pSocketCmd->pcmd);
            DBG("cur timeStamp is %d %s",timeStamp->valueint, homePath->valuestring);
            client_pg_setTime(timeStamp->valueint);
            strcpy(achomeDir,homePath->valuestring);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_SYNCFILE: //同步模组文件
        //客户端将比较文件，然后去下载文件
        {
        	printf("=====: SERVER2CLI_MSG_SYNCFILE\n");
            int i = 0;
            cJSON *item = NULL;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *TotalFiles = cJSON_GetObjectItem(root,"TotalFiles");            
            cJSON *tstModleName = cJSON_GetObjectItem(root,"tstModleName");
			
            set_cur_module_name(tstModleName->valuestring);
            printf("Sync Files: module name: %s, file nums: %d.\n", tstModleName->valuestring, TotalFiles->valueint);
			
            cJSON *FilesArray   = cJSON_GetObjectItem(root, "FilesArray");
            int arraySize = cJSON_GetArraySize(FilesArray);
			
            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
            recv_file_info_set->curNo = 0;
            recv_file_info_set->totalFilesNums  = arraySize;
            recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(arraySize * sizeof(recv_file_info_t));
            recv_file_info_set->fn_recv_file = client_pg_syncFile;
            recv_file_info_set->param = 0;
            recv_file_info_set->fn_get_cur_file_info  = get_cur_file_info;
            recv_file_info_set->fn_add_file_index = add_file_index;
			
            recv_file_list_insert(recv_file_info_set);
            memset(recv_file_info_set->pRecv_file_info, 0, sizeof(recv_file_info_t) * arraySize);

			// parse file info array, save file info array into file_info_set.
            for(i = 0; i < arraySize; i ++)
            {
                item = cJSON_GetArrayItem(FilesArray,i);
                cJSON *FileName = cJSON_GetObjectItem(item, "FileName");
                cJSON *FileSize = cJSON_GetObjectItem(item, "FileSize");
                cJSON *FileTime = cJSON_GetObjectItem(item, "FileTime");
                DBG("FileName:%s FileSize:%d", FileName->valuestring, FileSize->valueint);
				
                recv_file_info_set->pRecv_file_info[i].rfiSize = FileSize->valueint;
                recv_file_info_set->pRecv_file_info[i].rfiTime = FileTime->valueint;
                strcpy(recv_file_info_set->pRecv_file_info[i].rfiName, FileName->valuestring);
            }
			
            if(client_getFileFromSet(recv_file_info_set) != 0)
            {
            	printf("All File is new, do nothing!\n");
				
                if(recv_file_info_set->param)
                {
                    free(recv_file_info_set->param);
					recv_file_info_set->param = NULL;
                }

				if (recv_file_info_set->pRecv_file_info)
				{
					free(recv_file_info_set->pRecv_file_info);
					recv_file_info_set->pRecv_file_info = NULL;
				}

				recv_file_list_del(recv_file_info_set);
                free(recv_file_info_set);
				recv_file_info_set = NULL;

				printf("File sync end! =====\n");
                client_syncFinish();
            }
			
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_SENDFILE: //同步模组文件,  之后,   各个小文件下载
        //接收文件
        {
            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t *)recv_file_list_get((void*)pSocketCmd->ipnum);
            recv_file_info_t *recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
            if(pSocketCmd->type == BINFILE_BEGIN)
            {
            	//printf("recv file data: begin ...\n");
                if(recv_file_info->pFileData)
                {
                    free(recv_file_info->pFileData);
                    recv_file_info->pFileData = NULL;
                }
				
                cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
                cJSON *FileName = cJSON_GetObjectItem(root, "FileName");
                cJSON_Delete(root);
				
                recv_file_info->pFileData = (unsigned char*)malloc(recv_file_info->rfiSize);
                recv_file_info->actRecvSize = 0;
            }
            else if(pSocketCmd->type == BINFILE_BODY)
            {
            	//printf("recv file data: data ...\n");
                if(((recv_file_info->actRecvSize + pSocketCmd->len) <= recv_file_info->rfiSize)
						|| (recv_file_info->rfiSize == 0))
                {
                    memcpy(&recv_file_info->pFileData[recv_file_info->actRecvSize],
                        	pSocketCmd->pcmd, pSocketCmd->len);
					
                    recv_file_info->actRecvSize += pSocketCmd->len;
                    //DBG("ipnum %d",pSocketCmd->ipnum);
                }
                else
                {
                    printf("msgProc: SERVER2CLI_MSG_SENDFILE. recv file size error %d + %d > %d",
                           	recv_file_info->actRecvSize, pSocketCmd->len, recv_file_info->rfiSize);
                }
            }
            else
            {
            	// end.
            	//printf("recv file data: end.\n");
				
                if( (recv_file_info->rfiSize == recv_file_info->actRecvSize)
						|| (recv_file_info->rfiSize == 0) )
                {
                    //DBG("recv file %s success",recv_file_info->rfiName);
					
					// recv next file ...
                    recv_file_info_set->fn_recv_file(recv_file_info_set);
					
					free(recv_file_info->pFileData);
                    recv_file_info->pFileData = 0;
                }
                else
                {
                    DBG("recv file failed rfisize %d actSize %d type %x len %d",recv_file_info->rfiSize,
                        recv_file_info->actRecvSize,pSocketCmd->type,pSocketCmd->len);
                }
            }
        }
        break;

        case SERVER2CLI_MSG_SHUTON:
        //将PG开电
        {
        	printf("=====: SERVER2CLI_MSG_SHUTON\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *tstModelName = cJSON_GetObjectItem(root,"tstModleName");
			cJSON *json_ptn_id = cJSON_GetObjectItem(root, "ptn_index");

			int ptn_id = 0;
			
			if (json_ptn_id)
				ptn_id = json_ptn_id->valueint;
			
			if (tstModelName)
			{
				strcpy(m_current_module_filename, tstModelName->valuestring);
			}
			else
			{
				printf("SERVER2CLI_MSG_SHUTON error: Invalid module name!\n");
				cJSON_Delete(root);
				break;
			}
			
			struct timeval tpstart,tpend; 
			float timeuse; 
			gettimeofday(&tpstart, NULL);

			printf("SERVER2CLI_MSG_SHUTON: ptn_id = %d.\n", ptn_id);
            client_pg_shutON(1, tstModelName->valuestring, NULL, ptn_id);
			
			gettimeofday(&tpend,NULL); 
			timeuse = 1000000*(tpend.tv_sec-tpstart.tv_sec)+ 
						tpend.tv_usec-tpstart.tv_usec; 
			timeuse /= 1000000; 
			printf("client_pg_shutON Used Time:%f\n",timeuse); 

            cJSON_Delete(root);

			pwrEnableMonitor(1);

			#if 0
			//加载模块数据
			vcom_info_t vcom_info;

			#if 1
			module_info_t *p_vcom_Module_info =  db_get_vcom_module_info();
			if (p_vcom_Module_info)
			{
				read_vcom_config(&vcom_info, p_vcom_Module_info->vcomfile);
			}
			else
			{
				printf("*** find vcom pattern failed! ***\n");
			}
			#else
			int curPtnId = client_pg_ptnId_get();
			if(curPtnId == -1) 
				return;
			
			module_info_t *pModule_info = dbModuleGetPatternById(curPtnId);
			if(!pModule_info) 
				return;
			read_vcom_config(&vcom_info,pModule_info->vcomfile);
			#endif
			#endif
        }
        break;

        case SERVER2CLI_MSG_SHUTDWN:
        //将PG关电
        {
        	printf("=====: SERVER2CLI_MSG_SHUTDWN\n");
			pwrEnableMonitor(0);

			usleep(100 * 1000);
			
            client_pg_shutON(0,0,NULL, 0);
        }
        break;

        case SERVER2CLI_MSG_SHOWPTN:
        //PG显示指定的PTN
        {
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *ptnId = cJSON_GetObjectItem(root,"ptnId");
            client_pg_showPtn(ptnId->valueint);
            cJSON_Delete(root);
        }
        break;

		#ifdef ENABLE_SHOW_CURSOR
        case SERVER2CLI_MSG_CROSSCUR:
        //PG显示十字光标
        {
        	printf("=====: SERVER2CLI_MSG_CROSSCUR\n");
            pointer_xy_t *pstPointer;
            int i,pointerNum;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            pointerNum = cJSON_GetArraySize(root);
            pstPointer = (pointer_xy_t*)malloc(sizeof(pointer_xy_t)*pointerNum);
            for(i=0;i<pointerNum;i++)
            {
                cJSON *pxy = cJSON_GetArrayItem(root,i);
                if(pxy)
                {
                    cJSON *x= cJSON_GetObjectItem(pxy,"x");
                    cJSON *y= cJSON_GetObjectItem(pxy,"y");
                    pstPointer[i].x = x->valueint;
                    pstPointer[i].y = y->valueint;
                }
            }
            cJSON_Delete(root);
            client_pg_showCrossCur(pstPointer,pointerNum);
            free(pstPointer);
        }
        break;

        case SERVER2CLI_MSG_HIDECUR:
        {
			printf("=====: SERVER2CLI_MSG_HIDECUR\n");
            client_pg_hideCrossCur();
        }
        break;
		#endif
		
        case SERVER2CLI_MSG_READREG:
        //PG 读寄存器然后通知给 SERVER&SPONSOR
        {
        	printf("=====: SERVER2CLI_MSG_READREG\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *regAddr= cJSON_GetObjectItem(root,"regAddr");
            client_pg_readReg(regAddr->valueint);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_WRITEREG:
        //PG 写寄存器然后通知给 SERVER&SPONSOR
        {
        	printf("=====: SERVER2CLI_MSG_WRITEREG\n");
            cJSON *root   = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *regAddr= cJSON_GetObjectItem(root,"regAddr");
            cJSON *regVal = cJSON_GetObjectItem(root,"regVal");
            client_pg_writeReg(regAddr->valueint,regVal->valueint);
            cJSON_Delete(root);
            break;
        }

        case SERVER2CLI_MSG_SYNCTIME:
        //PG 同步当前的时间与上位机保持一致
        {
        	printf("=====: SERVER2CLI_MSG_SYNCTIME\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *timeStamp= cJSON_GetObjectItem(root,"timeStamp");
            client_pg_setTime(timeStamp->valueint);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_UPDATEFILE:
        // PG 去下载对应的升级文件，并把下载和更新的状态通知给上位机
        {
        	printf("=====: SERVER2CLI_MSG_UPDATEFILE\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *srcFilePath= cJSON_GetObjectItem(root,"srcFilePath");
            cJSON *fileType= cJSON_GetObjectItem(root,"fileType");
            cJSON *dstFilePath= cJSON_GetObjectItem(root,"dstFilePath");
            DBG("src %s type %s dst %d",srcFilePath->valuestring,fileType->valuestring,
                dstFilePath->valuestring);

            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
            recv_file_info_set->curNo = 0;
            recv_file_info_set->totalFilesNums = 1;
            recv_file_info_set->fn_get_cur_file_info = get_cur_file_info;
            recv_file_info_set->fn_add_file_index = add_file_index;
            recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(sizeof(recv_file_info_t));
			
            recv_file_info_t *recv_file_info = recv_file_info_set->pRecv_file_info;
            memset(recv_file_info,0,sizeof(recv_file_info_t));
			
            strcpy(recv_file_info->rfiName,srcFilePath->valuestring);
            recv_file_info_set->fn_recv_file = client_pg_downFile;
			
            dwn_file_info_t *dwn_file_info = (dwn_file_info_t*)malloc(sizeof(dwn_file_info_t));
            strcpy(dwn_file_info->srcFileName,srcFilePath->valuestring);
            strcpy(dwn_file_info->srcFileType,fileType->valuestring);
            strcpy(dwn_file_info->dstFileName,dstFilePath->valuestring);
			
            recv_file_info_set->param = dwn_file_info;
            client_getFile(recv_file_info->rfiName,recv_file_info_set);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_COMPLETESYNC: //完全同步测试文件, 没有清空ARM的所有cfg配置文件
        {
			printf("=====: SERVER2CLI_MSG_COMPLETESYNC\n");
            int i;
            cJSON *item;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *TotalFiles = cJSON_GetObjectItem(root,"TotalFiles");
            cJSON *tstModleName = cJSON_GetObjectItem(root,"tstModleName");
			
            set_cur_module_name(tstModleName->valuestring);
            DBG("TotalFiles is %d %s",TotalFiles->valueint,tstModleName->valuestring);
			
            cJSON *FilesArray = cJSON_GetObjectItem(root, "FilesArray");
            int arraySize = cJSON_GetArraySize(FilesArray);
            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
            recv_file_info_set->curNo = 0;
            recv_file_info_set->totalFilesNums  = arraySize;
            recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(arraySize*sizeof(recv_file_info_t));
            recv_file_info_set->fn_recv_file = client_pg_syncFile;
            recv_file_info_set->param = 0;
            recv_file_info_set->fn_get_cur_file_info  = get_cur_file_info;
            recv_file_info_set->fn_add_file_index = add_file_index;
            recv_file_list_insert(recv_file_info_set);
			
            memset(recv_file_info_set->pRecv_file_info,0,sizeof(recv_file_info_t) * arraySize);
            for(i = 0; i < arraySize; i ++)
            {
                item = cJSON_GetArrayItem(FilesArray,i);
                cJSON *FileName = cJSON_GetObjectItem(item,"FileName");
                cJSON *FileSize = cJSON_GetObjectItem(item,"FileSize");
                cJSON *FileTime = cJSON_GetObjectItem(item,"FileTime");
                DBG("FileName:%s FileSize:%d",FileName->valuestring,FileSize->valueint);
				
                recv_file_info_set->pRecv_file_info[i].rfiSize = FileSize->valueint;
                recv_file_info_set->pRecv_file_info[i].rfiTime = FileTime->valueint;
                strcpy(recv_file_info_set->pRecv_file_info[i].rfiName, FileName->valuestring);
            }
			cJSON_Delete(root);
			
			system("rm -rf ./cfg/*"); //xujie
            localDBDelAll();
			
            if(client_getFileFromSet(recv_file_info_set)!= 0)
            {
                if(recv_file_info_set->param)
                {
                    free(recv_file_info_set->param);
					recv_file_info_set->param = NULL;
                }				

				recv_file_list_del(recv_file_info_set);
                free(recv_file_info_set);
				recv_file_info_set = NULL;
				
                client_syncFinish();
            }
            
            break;
        }
        case SERVER2CLI_MSG_TSTSHUTON:
        {
			printf("=====: SERVER2CLI_MSG_TSTSHUTON\n");
            client_pg_mipiShutON(1,pSocketCmd->pcmd,pSocketCmd->len);
        }
        break;
		
        case SERVER2CLI_MSG_TSTSHUTDWN:
        {
			printf("=====: SERVER2CLI_MSG_TSTSHUTDWN\n");
            client_pg_mipiShutON(0,0,0);
        }
        break;
		
        case SERVER2CLI_MSG_TSTTIMON:
        {
			printf("=====: SERVER2CLI_MSG_TSTTIMON.    ===== do nothing ====\n");
        }
        break;
		
        case SERVER2CLI_MSG_TSTTIMDWN:
        {
			printf("=====: SERVER2CLI_MSG_TSTTIMDWN.    ===== do nothing ====\n");
        }
        break;
		
        case SERVER2CLI_MSG_GETPWRON:
        {
			printf("=====: SERVER2CLI_MSG_GETPWRON. \n");
            pwrEnableMonitor(1);
        }
        break;
		
        case SERVER2CLI_MSG_GETPWRDWN:
        {
			printf("=====: SERVER2CLI_MSG_GETPWRDWN. \n");
            pwrEnableMonitor(0);
        }
        break;

        case SERVER2CLI_MSG_AUTOFLICK: //自动FLICK
        {
			printf("=====: SERVER2CLI_MSG_AUTOFLICK. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *enableFlick = cJSON_GetObjectItem(root,"enableFlick");

			int vcom_ptn_index = get_current_vcom_ptn_index();
			if (vcom_ptn_index >= 0)
			{
				printf("switch flick picture: %d.\n", vcom_ptn_index);
				client_pg_showPtn(vcom_ptn_index);
			}
			
            flick_auto(enableFlick->valueint,0);
            cJSON_Delete(root);

			printf("SERVER2CLI_MSG_AUTOFLICK End.\n");
        }
        break;

        case SERVER2CLI_MSG_MANUALFLICK: //手动FLICK
        {
			printf("=====: SERVER2CLI_MSG_MANUALFLICK. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "channel");
            cJSON *flickValue = cJSON_GetObjectItem(root,"flickValue");
			{
        		int mipi_channel = 1;
				
				if (json_mipi_channel)
					mipi_channel = json_mipi_channel->valueint;

				#if 0
        		unsigned char cmd[100] = 
										{
											0x39, 0xE0, 0x01, 0x01,
											0x39, 0x01, 0x01, 0x01,
											0xff, 0xff, 0xff, 0xff, 0xff, 0xff
										};
				
				cmd[7] = flickValue->valueint;
				printf("flick: %d.\n", cmd[7]);
				printf("mipi channel = %d.\n", mipi_channel);
				SendCode(mipi_channel, cmd);
				#else
				qc_set_vcom_value(mipi_channel, flickValue->valueint);
				#endif
        	}
			
            //flick_manual(flickValue->valueint);
			printf("=====: SERVER2CLI_MSG_MANUALFLICK.  %d  END.\n", flickValue->valueint);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_FIRMWAREUPDATA: //固件升级
        {
        	printf("=====: SERVER2CLI_MSG_FIRMWAREUPDATA. \n");
           	if(pSocketCmd->type == BINFILE_BEGIN)
            {
                cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
                cJSON *FileName = cJSON_GetObjectItem(root, "firmwareFileName");
                cJSON *FileSize = cJSON_GetObjectItem(root, "fileSize");
				
				if( tmpFirmwareBuf != NULL )
                {
                    free(tmpFirmwareBuf);
                    tmpFirmwareBuf = NULL;
                    tmpFirmwareWpt = NULL;
                }
				
                tmpFirmwareBuf = (unsigned char*)malloc(FileSize->valueint);
                tmpFirmwareWpt = tmpFirmwareBuf;
                printf("SERVER2CLI_MSG_FIRMWAREUPDATA: begin. filesize: %u.\n", FileSize->valueint);
                cJSON_Delete(root);

				s_total_data_len = 0;
            }
            else if(pSocketCmd->type == BINFILE_BODY)
            {
                if( tmpFirmwareWpt == NULL )
                {
                    DBG("tmpFirmwareWpt == 0. no send BINFILE_BEGIN");
                    break;
                }

                memcpy(tmpFirmwareWpt, pSocketCmd->pcmd, pSocketCmd->len);
                tmpFirmwareWpt += pSocketCmd->len;
				s_total_data_len += pSocketCmd->len;
				//printf("body: recv len = %u, total len = %u.\n", pSocketCmd->len, s_total_data_len);
            }
            else
            {
                if( tmpFirmwareBuf == 0 )
                {
                    DBG("tmpFirmwareBuf == 0. no send BINFILE_BEGIN");
                    break;
                }

				printf("SERVER2CLI_MSG_FIRMWAREUPDATA: over. recv size: %u.\n", s_total_data_len);
				
                SUpdataFileList *pufl = (SUpdataFileList*)tmpFirmwareBuf;
                SUpdataFileList *poldufl = pufl;
				
                do{
					//printf("=== next file ====.\n");
                    recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
                    recv_file_info_set->curNo = 0;
                    recv_file_info_set->totalFilesNums  = 1;                    
                    recv_file_info_set->fn_recv_file = client_pg_firmware;
                    recv_file_info_set->param = 0;
                    recv_file_info_set->fn_get_cur_file_info  = get_cur_file_info;
                    recv_file_info_set->fn_add_file_index = add_file_index;

					// fill recv file info.
					recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(sizeof(recv_file_info_t));
                    memcpy(recv_file_info_set->pRecv_file_info->rfiName, pufl->name, strlen(pufl->name) + 1);
                    recv_file_info_set->pRecv_file_info->rfiSize = pufl->filesize;

					// copy file data.
                    recv_file_info_set->pRecv_file_info->pFileData = (unsigned char*)malloc(pufl->filesize);					
                    unsigned char* pdata = (unsigned char*)pufl + sizeof(SUpdataFileList);
                    memcpy(recv_file_info_set->pRecv_file_info->pFileData, pdata, pufl->filesize);

					// save file data.
                    recv_file_info_set->fn_recv_file(recv_file_info_set);
                    printf("Upgrade file: %s.\n", recv_file_info_set->pRecv_file_info->rfiName);
                    
                    free(recv_file_info_set->pRecv_file_info->pFileData);
                    free(recv_file_info_set->pRecv_file_info);
                    free(recv_file_info_set);

                    poldufl = pufl;
                    char *poffer = (char*)pufl;

					printf("upgrade image size: %d.\n", pufl->filesize);
                    poffer += pufl->filesize + sizeof(SUpdataFileList);
                    pufl = (SUpdataFileList*)poffer;
					
                }while( poldufl->hasnext == 0x01 );

                DBG("SERVER2CLI_MSG_FIRMWAREUPDATA end");

				int upgrade_type = 0;
				int upgrade_state = 0;
				client_end_firmware_upgrade(upgrade_type, upgrade_state);

				char cmd_buf[1024] = "";
				sprintf(cmd_buf, "%s %s", "/home/updatafirmware.sh", poldufl->name);
				printf("cmd: %s\n", cmd_buf);
				system(cmd_buf);

				if( tmpFirmwareBuf != NULL )
                {
                    free(tmpFirmwareBuf);
                    tmpFirmwareBuf = NULL;
                    tmpFirmwareWpt = NULL;
                }
            }
        }
        break;

        case SERVER2CLI_MSG_SYNCMODE:
        {
			printf("=====: SERVER2CLI_MSG_SYNCMODE. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *modeName = cJSON_GetObjectItem(root, "modeName");
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_FLICKVCOM:
        {
			printf("=====: SERVER2CLI_MSG_FLICKVCOM. \n");
			#if 0
			int channel = m_channel_1;
			int lcd_channel = 1;
			
			//读取FLIC
			int flick = 0;
			if(lcd_getStatus(lcd_channel))
			{
				#ifdef USE_Z_CA210_LIB
				float f_flick = 0.00;
				
				ca210_start_flick_mode(lcd_channel);
				usleep(1000*100);
				ca210_capture_flick_data(lcd_channel, &f_flick);
				printf("=== lcd channel: %d. flick: %f. \n", lcd_channel, f_flick);
				usleep(1000*100);
				ca210_stop_flick_mode(lcd_channel);
				flick = f_flick * 100;
				#else
				lcd_autoFlickStart(lcd_channel, 0); 
				usleep(1000*100);
				int retVal = lcd_autoFlickWork(lcd_channel, 0); //查找FLICK设备返回的FLICK值
				usleep(1000*10);
				lcd_autoFlickEnd(lcd_channel);
				flick = retVal;
				#endif
			}
			
			//读取VCOM,
			int vcom = 0;
			//读取次数
			int otp = 0;
			if (chip_is_helitai_8019())
			{
				printf("Unknow Drive IC!\n");
			}
			else
			{
				#ifdef USED_IC_MANAGER
				ic_mgr_read_chip_vcom_otp_times(m_channel_1, &otp);
				ic_mgr_read_vcom(m_channel_1, &vcom);
				printf("read vcom: otp: %d, vcom: %d.\n", otp, vcom);
				#endif
			}

			int show_vcom = 1;
			int show_ok = 0;
			int ok1 = 0;
			int ok2 = 0;
			show_lcd_msg(channel, show_vcom, show_ok, vcom, flick, ok1, ok2);

			client_sendVcom(channel, vcom, otp, flick);
			#endif
			
        }
        break;

        case SERVER2CLI_MSG_REALTIMECONTROL:
        {
			printf("=====: SERVER2CLI_MSG_REALTIMECONTROL. \n");
            if(pSocketCmd->pcmd && pSocketCmd->len)
            {
                mipi_worked(pSocketCmd->pcmd,pSocketCmd->len);
            }
        }
        break;

		case SERVER2CLI_MSG_BOXUPDATA: //控制盒升级
		{
			#ifdef ENABLE_CONTROL_BOX
			if(pSocketCmd->type == BINFILE_BEGIN)
			{
				printf("=====: SERVER2CLI_MSG_BOXUPDATA. BINFILE_BEGIN\n");
				DBG("pSocketCmd->type == BINFILE_BEGIN");
				if( tmpBoxUpdateBuf != NULL )
				{
					free(tmpBoxUpdateBuf);
					tmpBoxUpdateBuf = tmpBoxUpdateWpt = NULL;
					tmpBoxUpdateSize = 0;
				}

				//解析得到文件大小
				cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
				//cJSON *FileName = cJSON_GetObjectItem(root,"firmwareFileName");
				cJSON *FileSize = cJSON_GetObjectItem(root, "fileSize");
				tmpBoxUpdateSize = FileSize->valueint;
				cJSON_Delete(root);

				//分配内存
				tmpBoxUpdateBuf = (unsigned char*)malloc(tmpBoxUpdateSize);
				DBG("upgrade data: %p, size: %d. \n", tmpBoxUpdateBuf, tmpBoxUpdateSize);
				tmpBoxUpdateWpt = tmpBoxUpdateBuf;
				//DBG("tmpBoxUpdateWpt %d", tmpBoxUpdateWpt);
			}
			else if(pSocketCmd->type == BINFILE_BODY) //升级文件数据块
			{
				printf("=====: SERVER2CLI_MSG_BOXUPDATA. BINFILE_BODY %d\n", pSocketCmd->len);
				if( tmpBoxUpdateWpt == 0 )
				{
					DBG("tmpBoxUpdateWpt == 0. no send BINFILE_BEGIN");
					break;
				}
				
				memcpy(tmpBoxUpdateWpt, pSocketCmd->pcmd, pSocketCmd->len);
				tmpBoxUpdateWpt += pSocketCmd->len;
			}
			else //升级文件接收结束
			{
				printf("=====: SERVER2CLI_MSG_BOXUPDATA. BINFILE_OVER\n");
				if( tmpBoxUpdateBuf == 0 )
				{
					printf("Error: tmpBoxUpdateBuf == NULL. \n");
					break;
				}
				
				//保存升级文件
				unsigned int u32QueueId = box_message_queue_get();
				//MsOS_PostMessage(u32QueueId, 0x12345678, (MS_U32)tmpBoxUpdateBuf, tmpBoxUpdateSize);
				MsOS_SendMessage(u32QueueId, 0x12345678, (MS_U32)tmpBoxUpdateBuf, tmpBoxUpdateSize);

				if (tmpBoxUpdateBuf)
					free(tmpBoxUpdateBuf);
				
				tmpBoxUpdateBuf = tmpBoxUpdateWpt = NULL;
				tmpBoxUpdateSize = 0;

				printf("==== upgrade end ===\n");

				int upgrade_type = 1;
				int upgrade_state = 0;
				client_end_firmware_upgrade(1, 0);
			}
			#endif
		}
		break;

		case SERVER2CLI_MSG_POWERUPDATA: //电源升级 2017-4-24
		{
			//printf("=====: SERVER2CLI_MSG_POWERUPDATA. \n");
			if(pSocketCmd->type == BINFILE_BEGIN)
			{
				printf("=====: SERVER2CLI_MSG_POWERUPDATA = BINFILE_BEGIN\n");
				DBG("pSocketCmd->type == BINFILE_BEGIN");

				//释放之前旧的升级数据
				if( tmpPowerUpdateBuf != NULL )
				{
					free(tmpPowerUpdateBuf);
					tmpPowerUpdateBuf = tmpPowerUpdateWpt = NULL;
					tmpPowerUpdateSize = 0;
				}

				//解析得到文件大小
				cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
				//cJSON *FileName = cJSON_GetObjectItem(root,"firmwareFileName");
				cJSON *FileSize = cJSON_GetObjectItem(root, "fileSize");
				tmpPowerUpdateSize = FileSize->valueint;
				cJSON_Delete(root);

				//分配内存
				tmpPowerUpdateBuf = (unsigned char*)malloc( tmpPowerUpdateSize );
				DBG("tmpPowerUpdateBuf = %x  tmpPowerUpdateSize = %d", tmpPowerUpdateBuf, tmpPowerUpdateSize);
				tmpPowerUpdateWpt = tmpPowerUpdateBuf;
				DBG("tmpPowerUpdateWpt %x", tmpPowerUpdateWpt);
			}
			else if(pSocketCmd->type == BINFILE_BODY) //升级文件数据块
			{
				printf("=====: SERVER2CLI_MSG_POWERUPDATA = BINFILE_BODY %d\n", pSocketCmd->len);
				if( tmpPowerUpdateWpt == 0 )
				{
					DBG("tmpPowerUpdateWpt == 0. no send BINFILE_BEGIN");
					break;
				}
				
				memcpy(tmpPowerUpdateWpt, pSocketCmd->pcmd, pSocketCmd->len);
				tmpPowerUpdateWpt += pSocketCmd->len;
			}
			else //升级文件接收结束
			{
				printf("=====: SERVER2CLI_MSG_POWERUPDATA = BINFILE_OVER\n");
				if( tmpPowerUpdateBuf == 0 )
				{
					DBG("tmpPowerUpdateBuf == 0. no send BINFILE_BEGIN");
					break;
				}
				
				//通知电源模块升级
				unsigned int u32QueueId = power_message_queue_get();
				MsOS_SendMessage(u32QueueId,PWR_CMD_UPGRADE,(MS_U32)tmpPowerUpdateBuf, tmpPowerUpdateSize);

				//清除状态信息
				if (tmpPowerUpdateBuf)
					free(tmpPowerUpdateBuf);
				
				tmpPowerUpdateBuf = tmpPowerUpdateWpt = NULL;
				tmpPowerUpdateSize = 0;

				int upgrade_type = 2;
				int upgrade_state = 0;
				client_end_firmware_upgrade(upgrade_type, upgrade_state);
			}
		}
		break;

		 // VCOM OTP
		case SERVER2CLI_MSG_REALTIME_CONTROL: //调试寄存器 2017-5-22
		setHsmode(0);
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_data = cJSON_GetObjectItem(root, "data");
			int channel = json_channel->valueint;
			int i,j,len = 0;
			unsigned char data[256];
			if(json_data != NULL)
			{
				char* ptr = json_data->valuestring;
				while(ptr[0])
				{
					char* p_start = strstr(ptr, "{");
					if(p_start == NULL) break;
					char* p_end = strstr(p_start, "}");
					if(p_end == NULL) break;

					//w=1;
					char* p_w = strstr(p_start, "w=");
					//if(p_w == NULL) break;					

					//cmd=xx xx xx;
					char* p_cmd = strstr(p_start, "cmd=");
					//if(p_cmd == NULL) break;

					//para=1;
					char* p_para = strstr(p_start, "para=");
					//if(p_para == NULL) break;

					//转换
					int w = -1;
					if(p_w != NULL)
					{
						p_w += 2;
						if(p_w[0] != ';')
						{
							char txt[20];
							memset(txt, 0, sizeof(txt));
							for(i=0; i < 20; i++)
							{
								if(p_w[i] == ';') break;
								txt[i] = p_w[i];
							}
							w = atoi(txt);
						}
					}
					int cmdlen = 0;
					unsigned char cmd[256];
					if(p_cmd != NULL)
					{
						p_cmd += 4;
						if(p_cmd[0] != ';')
						{
							while(p_cmd[0])
							{
								char txt[20];
								txt[0] = p_cmd[0];
								txt[1] = p_cmd[1];
								txt[2] = 0;
								int val = 0;
								sscanf(txt, "%02X", &val);
								cmd[cmdlen++] = (unsigned char)(val & 0xff);
								if(p_cmd[2] == ';') break;
								p_cmd += 3;
							}
						}
					}
					int para = 0;
					if(p_para != NULL)
					{
						p_para += 5;
						if(p_para[0] != ';')
						{
							char txt[20];
							memset(txt, 0, sizeof(txt));
							for(i=0; i < 20; i++)
							{
								if(p_para[i] == ';') break;
								txt[i] = p_para[i];
							}
							para = atoi(txt);
						}
					}

					//
					if(w == 0) //读
					{
						if(cmdlen > 0)
						{
							int ret = ReadModReg(channel, 1, cmd[0], para, data+len);
							if(ret == 0)
							{
								printf("----------------------- ReadModReg ret=%d\n", ret);
							}
							len += para;
						}
					}
					else if(w == 1) //写
					{
						if(cmdlen > 0)
						{
							int n = 0;
							unsigned char code[256];
							memset(code, 0xff, sizeof(code));
							code[n++] = 0x39;
							code[n++] = cmd[0];
							code[n++] = cmdlen - 1;
							for(j=1; j < cmdlen; j++)
							{
								code[n++] = cmd[j];
							}
							SendCode(channel, code);
							printf("----------------------- SendCode %02X\n", cmd[0]);
						}
					}
					else if(w == 2) //Delay
					{
						if(para > 0)
						{
							if(para >= 1000*10) para = 1000*10;
							usleep(1000 * para);
							printf("----------------------- Delay %d\n", para);
						}
					}

					//
					ptr = p_end + 1;
				}
			}
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_REALTIME_CONTROL. channel %d\n", channel);

			//回应
			client_end_realtime_control(channel, len, data);

			//切回高速模式
			WriteSSD2828(channel,0XB7,0X030b);  //WriteSSD2828(channel,0XB7,0x03c9);
		}
		setHsmode(1);
		break;

		// capture x, y, lv data.
		case SERVER2CLI_MSG_RGBW:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *jsonChannel = cJSON_GetObjectItem(root, "channel");
			cJSON *jsonRGBW = cJSON_GetObjectItem(root, "rgbw");
			int channel = jsonChannel->valueint;
			int rgbw = jsonRGBW->valueint;		
			cJSON_Delete(root);
			
			printf("=====: SERVER2CLI_MSG_RGBW. channel %d, r[%3d] g[%3d] b[%3d]\n", channel, (rgbw>>16)&0xff, (rgbw>>8)&0xff, rgbw&0xff);

			//设置FPGA
			showRgbStr showRgb;
			showRgb.rgb_r = (rgbw >> 16) & 0xff;
			showRgb.rgb_g = (rgbw >> 8) & 0xff;
			showRgb.rgb_b = (rgbw) & 0xff;
			unsigned int u32QueueId = fpga_message_queue_get();
			MsOS_SendMessage(u32QueueId, FPGA_CMD_SHOW_RGB, &showRgb, 0);

			//usleep(500 * 1000);
			usleep(300 * 1000);

			double x = 0.1;
			double y = 0.1;
			double Lv = 0.1;
			int lcd_channel = 1;

			#ifdef USE_Z_CA210_LIB
			ca210_capture_xylv_data(lcd_channel, &x, &y, &Lv);
			#else
			int ret = lcd_xyLvWork(lcd_channel, &x, &y, &Lv);
			#endif
			
			printf("channel %d, RGB[%3d:%3d:%3d], x: %f, y: %f, Lv: %f.\n", channel, (rgbw>>16)&0xff, (rgbw>>8)&0xff, 
						rgbw&0xff, x, y, Lv);

			//回应PG上位机, 此操作结束
			client_end_rgbw(channel, rgbw, x, y, Lv);
		}
		break;

		// write gamma reg data.
		case SERVER2CLI_MSG_GAMMA_WRITE_REG:
		printf("====== Write GAMMA REG DATA ===========\n");
		{
			#define GAMMA_MAX_REG_DATA_LEN	(255)
			unsigned char reg_data_buf[GAMMA_MAX_REG_DATA_LEN] = { 0 };
			int reg_data_buf_len = GAMMA_MAX_REG_DATA_LEN;
			
			char *json_ep = NULL;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_write_type = cJSON_GetObjectItem(root, "write_type");
			int channel = json_channel->valueint;

			// default type: analog gamma.
			int write_type = 0;
			if (json_write_type)
				write_type = json_write_type->valueint;

			if (write_type == 0)
			{
				// save new analog gamma
				cJSON *json_data = cJSON_GetObjectItem(root, "data");
				if(json_data != NULL)
				{
					reg_data_buf_len = parse_gamma_reg_data(json_data->valuestring, reg_data_buf, reg_data_buf_len);
					printf("new analog gamma data len: %d.\n", reg_data_buf_len);
					dump_data1(reg_data_buf, reg_data_buf_len);

					// write new analog gamma reg data.
					if (reg_data_buf_len > 0)
					{
						#ifdef USED_IC_MANAGER
						ic_mgr_write_analog_gamma_data(channel, reg_data_buf, reg_data_buf_len);
						ic_mgr_write_fix_digital_gamma_data(channel);
						#endif
					}
					
				}
			}
			else if (write_type == 1)
			{
				// save new digital gamma.
				cJSON *json_data = cJSON_GetObjectItem(root, "data");
				if(json_data != NULL)
				{
					reg_data_buf_len = parse_gamma_reg_data(json_data->valuestring, reg_data_buf, reg_data_buf_len);
					printf("new digital gamma data len: %d.\n", reg_data_buf_len);
					dump_data1(reg_data_buf, reg_data_buf_len);

					// write new analog gamma reg data.
					if (reg_data_buf_len > 0)
					{
						#ifdef USED_IC_MANAGER
						ic_mgr_write_digital_gamma_data(channel, reg_data_buf, reg_data_buf_len);
						#endif
					}
				}
			}
			else if (write_type == 2)
			{
			}
			
			//回应PG上位机, 此操作结束
			client_end_gamma_write(channel, write_type);
		}
		break;

		// read gamma reg data.
		case SERVER2CLI_MSG_READ_GAMMA:
		{	
			#define GAMMA_MAX_REG_DATA_LEN1	(255)
			unsigned char reg_data_buf[GAMMA_MAX_REG_DATA_LEN1] = { 0 };
			int reg_data_buf_len = GAMMA_MAX_REG_DATA_LEN1;

			unsigned char read_buffer[GAMMA_MAX_REG_DATA_LEN1] = { 0 };
			int read_data_len = GAMMA_MAX_REG_DATA_LEN1;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_read_type = cJSON_GetObjectItem(root, "read_type");
			int channel = json_channel->valueint;

			// default read type: analog gamma.
			int read_type = 0;	
			if (json_read_type)
				read_type = json_read_type->valueint;
			
			cJSON *json_data = cJSON_GetObjectItem(root, "data");
			if(json_data != NULL)
			{
				reg_data_buf_len = parse_gamma_reg_data(json_data->valuestring, reg_data_buf, reg_data_buf_len);
				printf("new digital gamma data len: %d.\n", reg_data_buf_len);
				dump_data1(reg_data_buf, reg_data_buf_len);

				unsigned char otp_times = 0;

				// load gamma regdata from config.				
				// get init analog gamma config.
				gamma_cfg_t* p_gamma_cfg = get_current_gamma_cfg();

				#ifdef USED_IC_MANAGER
				ic_mgr_d3g_control(channel, 0);
				#endif
				
				printf("enable gamma: %d.\n", p_gamma_cfg->enable_gamma_reg);

				if (reg_data_buf_len > 0)
				{					
					printf("write e0_ok data!\n");
					#ifdef USED_IC_MANAGER
					ic_mgr_write_analog_gamma_data(channel, reg_data_buf, reg_data_buf_len);
					#endif
				}
				else if (p_gamma_cfg->enable_gamma_reg)
				{
					printf("write gamma cfg data\n");
					#ifdef USED_IC_MANAGER
					ic_mgr_write_analog_gamma_data(channel, p_gamma_cfg->gamma_reg, p_gamma_cfg->gamma_reg_nums);
					#endif
				}
				else
				{
					#ifdef USED_IC_MANAGER
					ic_mgr_write_fix_analog_gamma_data(channel);
					#endif
				}
				
				// read analog gamma value.
				#ifdef USED_IC_MANAGER
				read_data_len = ic_mgr_read_analog_gamma_data(channel, read_buffer, read_data_len);
				#endif

				printf("read alanog reg len: 0x%02x.\n", read_data_len);
				dump_data1(read_buffer, read_data_len);

				#ifdef USED_IC_MANAGER
				ic_mgr_read_chip_vcom_otp_times(channel, &otp_times);
				#endif

				client_end_read_gamma(channel, read_data_len, read_buffer, otp_times, 
										otp_times, read_type);				
			}
			
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_READ_GAMMA. channel %d\n", channel);
		}
		break;

		// burn gamma reg data: analog gamma or digital gamma.
		case SERVER2CLI_MSG_GAMMA_OTP:
		{
			int i = 0;
			int j = 0;
			int check_error = 0;
			int vcom_burn = 0;
			int gamma_burn_2nd = 0;
			
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_otp_flag = cJSON_GetObjectItem(root, "otp_flag");
			int channel = json_channel->valueint;
			int otp_flag = json_otp_flag ? json_otp_flag->valueint : 0x00;
			cJSON_Delete(root);
			
			printf("=====: SERVER2CLI_MSG_GAMMA_OTP. channel %d, otp_flag: %x\n", channel, otp_flag);

			if (otp_flag == 0)
			{
				check_error = -1;
				printf("SERVER2CLI_MSG_GAMMA_OTP: otp flag is %d.\n", otp_flag);
				client_end_gamma_otp_info(channel, check_error, 0, 0, 0, 0, 0);
				break;
			}
			else
			{
				if (otp_flag & 0x02)
				{
					vcom_burn = 1;
				}

				if (otp_flag & 0x08)
				{
					gamma_burn_2nd = 1;
				}
			}
			
			int err = 0;
			
			unsigned int read_vcom = 0;
			unsigned char otp_times = 0;
			int check_flick = 0;
			double f_check_x = 0;
			double f_check_y = 0;
			double f_check_Lv = 0;

			int vcom = 0;
			int flick = 0;
			mipi_channel_get_vcom_and_flick(channel, &vcom, &flick);

			// get analog gamma reg data
			unsigned char analog_gamma_reg_data[255] = { 0 };
			int analog_gamma_reg_data_len = 255;

			// get digital gamma reg data.
			unsigned char d3g_r_reg_data[255] = { 0 };
			int d3g_r_reg_data_len = 255;
			unsigned char d3g_g_reg_data[255] = { 0 };
			int d3g_g_reg_data_len = 255;
			unsigned char d3g_b_reg_data[255] = { 0 };
			int d3g_b_reg_data_len = 255;

			printf("====  get vcom value: %d, 0x%02x. \n", vcom, vcom);
			#ifdef USED_IC_MANAGER
			ic_mgr_read_chip_vcom_otp_times(channel, &otp_times);
			ic_mgr_get_analog_gamma_reg_data(channel, analog_gamma_reg_data, &analog_gamma_reg_data_len);
			ic_mgr_get_digital_gamma_reg_data_t(channel, d3g_r_reg_data, &d3g_r_reg_data_len, 
												d3g_g_reg_data, &d3g_g_reg_data_len, 
												d3g_b_reg_data, &d3g_b_reg_data_len);
			ic_mgr_burn_gamma_otp_values(channel, gamma_burn_2nd, vcom_burn, vcom, 
										analog_gamma_reg_data, analog_gamma_reg_data_len, 
										d3g_r_reg_data, d3g_r_reg_data_len, 
										d3g_g_reg_data, d3g_g_reg_data_len,
										d3g_g_reg_data, d3g_g_reg_data_len
										);
			#endif
			
#if 0
			//Reset
			mipi_reset(channel);
			usleep(1000 * 10); 

			//加载初始化代码
			memset(InitCode, 0xff, sizeof(InitCode));
			mipi_parse_init_code(m_current_module_filename, InitCode);
			SendCode(channel, InitCode);
			usleep(1000 * 10); 

			 //激活输出, 点屏
			WriteSSD2828(channel,0XB7,0X030b);
			usleep(5 * 1000);
#else
			usleep(300 * 1000);
			client_pg_shutON(1, m_current_module_filename, NULL, 0);
#endif

			// check 
			#ifdef USED_IC_MANAGER
			check_error = ic_mgr_check_gamma_otp_values(channel, vcom_burn, vcom, otp_times, 
											analog_gamma_reg_data, analog_gamma_reg_data_len, 
											d3g_r_reg_data, d3g_r_reg_data_len, 
											d3g_g_reg_data, d3g_g_reg_data_len,
											d3g_b_reg_data, d3g_b_reg_data_len);
			#endif
			
			// set flick pic
			client_pg_showPtn(0); 
			usleep(300 * 1000);
			
			// capture flick value.
			int lcd_channel = 1;			
			if(lcd_getStatus(lcd_channel))
			{
				#ifdef USE_Z_CA210_LIB
				float f_flick = 0.00;
				ca210_start_flick_mode(lcd_channel);
				usleep(1000*100);
				ca210_capture_flick_data(lcd_channel, &f_flick);
				usleep(1000*10);
				ca210_stop_flick_mode(lcd_channel);
				#else
				lcd_autoFlickStart(lcd_channel, 0); 
				usleep(1000*100);
				int retVal = lcd_autoFlickWork(lcd_channel, 0);
				usleep(1000*10);
				lcd_autoFlickEnd(lcd_channel);
				check_flick = retVal;
				#endif
			}

			printf("check flick: %d.\n", check_flick);

			// set white photo.			
			showRgbStr showRgb;
			int rgbw = 0xFFFFFF;
			showRgb.rgb_r = (rgbw >> 16) & 0xff;
			showRgb.rgb_g = (rgbw >> 8) & 0xff;
			showRgb.rgb_b = (rgbw) & 0xff;
			unsigned int u32QueueId = fpga_message_queue_get();
			MsOS_SendMessage(u32QueueId, FPGA_CMD_SHOW_RGB, &showRgb, 0);
			
			// capture white point x, y, lv.
			#ifdef USE_Z_CA210_LIB
			ca210_start_xylv_mode(lcd_channel);
			#else
			lcd_xyLvStart(lcd_channel);
			#endif
			
			usleep(300 * 1000);
			#ifdef USE_Z_CA210_LIB
			ca210_capture_xylv_data(lcd_channel, &f_check_x, &f_check_y, &f_check_Lv);
			#else
			int ret = lcd_xyLvWork(lcd_channel, &f_check_x, &f_check_y, &f_check_Lv);
			#endif

			#ifdef USE_Z_CA210_LIB
			ca210_stop_xylv_mode(lcd_channel);
			#else
			lcd_xyLvEnd(lcd_channel);
			#endif
			
			printf("check white point: x: %f, y: %f, lv: %f.\n", f_check_x, f_check_y, f_check_Lv);

			if (check_error)
			{
				set_fpga_text_color(channel, 1);
				set_fpga_text_show(0, 1, 0, 0);
			}
			else
			{
				set_fpga_text_color(channel, 0);
				set_fpga_text_show(0, 1, 1, 0);
			}
			
			// OTP end ack.
			//client_end_gamma_otp(channel, "烧录成功!");
			int is_ok = check_error ? 0 : 1;
			client_end_gamma_otp_info(channel, check_error, read_vcom, check_flick, 
										f_check_x, f_check_y, f_check_Lv);
		}
		break;

		// CA210 start work.
		case SERVER2CLI_MSG_CA310_START:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_lcd_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_mode = cJSON_GetObjectItem(root, "mode");

			int lcd_channel = json_lcd_channel->valueint;
			int mode = json_mode->valueint;
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_CA310_START %d\n", lcd_channel);
			
			if(mode == 0) //xyLv
			{
				#ifdef USE_Z_CA210_LIB
				ca210_start_xylv_mode(lcd_channel);
				#else
				lcd_xyLvStart(lcd_channel);
				#endif
			}
			else if(mode == 1) //Flicker
			{
				#ifdef USE_Z_CA210_LIB
				ca210_start_flick_mode(lcd_channel);
				#else
				lcd_autoFlickStart(lcd_channel); 
				#endif
			}
			//usleep(1000 * 200);
		}
		break;

		// CA210 stop work.
		case SERVER2CLI_MSG_CA310_STOP:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_lcd_channel = cJSON_GetObjectItem(root, "lcd_channel");
			int lcd_channel = json_lcd_channel->valueint;
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_CA310_STOP %d\n", lcd_channel);

			#ifdef USE_Z_CA210_LIB
			ca210_stop_flick_mode(lcd_channel);
			#else
			lcd_autoFlickEnd(lcd_channel);
			#endif
		}
		break;

		case SERVER2CLI_MSG_LCD_READ_REG:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_read_reg = cJSON_GetObjectItem(root, "reg");
			cJSON *json_read_len = cJSON_GetObjectItem(root, "len");
			
			int channel = json_channel ? json_channel->valueint : 1;
			unsigned char read_reg = json_read_reg ? json_read_reg->valueint : 0;
			unsigned char read_len = json_read_len ? json_read_len->valueint : 1;

			#define READ_BUFFER_LEN		(1024)
			unsigned char read_buffer[READ_BUFFER_LEN] = { 0 };
			int real_read_len = 0;

			int cnt = 1000 * 100;
			{
				// entry lp mode
				#if 1
				//mipi_to_lp_mode(channel);

				real_read_len = ReadModReg(channel, 1, read_reg, read_len, read_buffer);
				#endif
				
				// leave lp mode
				//mipi_to_hs_mode(channel);
				//printf("HS\n");

				printf("lcd read reg: 0x%02x, len: %d.\n", read_reg, real_read_len);
				dump_data1(read_buffer, real_read_len);

			}

			// send read data to pc.
			client_lcd_read_reg_ack(channel, real_read_len, read_buffer);
			
			cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_LCD_WRITE_CODE:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_code = cJSON_GetObjectItem(root, "code");
			
			int channel = json_channel ? json_channel->valueint : 1;
			char* code = json_code ? json_code->valuestring : NULL;

			if (code)
			{
				unsigned char temp_mipi_code[1024 * 4];
				
				// entry lp mode
				//mipi_to_lp_mode(channel);
				//printf("LP\n");
				#if 1
				//mipi_worked(pSocketCmd->pcmd,pSocketCmd->len);
				printf("SERVER2CLI_MSG_LCD_WRITE_CODE:\n%s\n", code);
				
				memset(temp_mipi_code, 0xff, sizeof(temp_mipi_code));
		        mipi_write_init_code("/tmp/mipi", code, strlen(code));
		        mipi_parse_init_code("/tmp/mipi", temp_mipi_code, NULL);
		        SendCode(channel, temp_mipi_code);

				// leave lp mode
				//mipi_to_hs_mode(channel);
				#endif
			}
			
			cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_PHOTO_DEBUG:
		{
			int photo_type = 0;
			int photo_value = 0xFFFFFF;
			
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_photo_type = cJSON_GetObjectItem(root, "photo_type");
			cJSON *json_photo_value = cJSON_GetObjectItem(root, "photo_value");
			photo_type = json_photo_type->valueint;
			photo_value = json_photo_value->valueint;
			cJSON_Delete(root);

			showRgbStr showRgb;
			showRgb.rgb_r = (photo_value >> 16) & 0xff;
			showRgb.rgb_g = (photo_value >> 8) & 0xff;
			showRgb.rgb_b = (photo_value) & 0xff;
			unsigned int u32QueueId = fpga_message_queue_get();
			MsOS_SendMessage(u32QueueId, FPGA_CMD_SHOW_RGB, &showRgb, 0);
		}
		break;

		case SERVER2CLI_MSG_MIPI_CHANNEL_RESET:
		{
			int mipi_channel = 0;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			mipi_channel = json_channel ? json_channel->valueint : 1;
			mipi_reset(mipi_channel);
			printf("SERVER2CLI_MSG_MIPI_CHANNEL_RESET: reset channel %d.\n", mipi_channel);
			cJSON_Delete(root);
		};
		break;
		
		case SERVER2CLI_MSG_MIPI_CHANNEL_MTP:
		{
			int mipi_channel = 0;
			int mtp_on = 0;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_mtp_on = cJSON_GetObjectItem(root, "mtp_on");
			mipi_channel = json_channel ? json_channel->valueint : 1;
			mtp_on = json_mtp_on ? json_mtp_on->valueint : 0;
			if (mtp_on)
			{
				mtp_power_on(mipi_channel);
				printf("SERVER2CLI_MSG_MIPI_CHANNEL_MTP: mtp power on channel %d.\n", mipi_channel);
			}
			else
			{
				mtp_power_off(mipi_channel);
				printf("SERVER2CLI_MSG_MIPI_CHANNEL_MTP: mtp power off channel %d.\n", mipi_channel);
			}
			cJSON_Delete(root);
		};
		break;
		
		case SERVER2CLI_MSG_MIPI_MODE:
		{
			int mipi_channel = 0;
			int hs_mode = 0;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_hs_mode = cJSON_GetObjectItem(root, "mode");
			mipi_channel = json_channel ? json_channel->valueint : 1;
			hs_mode = json_hs_mode ? json_hs_mode->valueint : 0;
			
			if (hs_mode)
			{
				printf("SERVER2CLI_MSG_MIPI_MODE: channel %d to HS mode.\n", mipi_channel);
				mipi_to_hs_mode(mipi_channel);
				
			}
			else
			{
				mipi_to_lp_mode(mipi_channel);
				printf("SERVER2CLI_MSG_MIPI_MODE: channel %d to LP mode.\n", mipi_channel);
			}
			cJSON_Delete(root);
		};
		break;

		case SERVER2CLI_MSG_FLICK_TEST: //Flick Test.
        {
			printf("=====: SERVER2CLI_MSG_FLICK_TEST. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "mipi_channel");
            cJSON *json_vcom = cJSON_GetObjectItem(root, "vcom");
			cJSON *json_test_delay = cJSON_GetObjectItem(root, "delay");

			int mipi_channel = json_mipi_channel ? json_mipi_channel->valueint : 1;
			int vcom = json_vcom ? json_vcom->valueint : 0;
			int delay = json_test_delay ? json_test_delay->valueint : 100;

			printf("vcom: %d, delay: %d ms.\n", vcom, delay);
            flick_test(mipi_channel, vcom, delay);
            cJSON_Delete(root);
        }
        break;
		
		case SERVER2CLI_MSG_READ_VCOM_OTP_INFO: //Flick Test.
        {
			printf("=====: SERVER2CLI_MSG_READ_VCOM_OTP_INFO. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "mipi_channel");

			int mipi_channel = json_mipi_channel ? json_mipi_channel->valueint : 1;

			printf("read vcom: channel = %d.\n", mipi_channel);
			
            cJSON_Delete(root);

			unsigned char vcom_otp_h = 0;
			unsigned char vcom_otp_l = 0;
			unsigned char vcom_otp_times = 0;

			#if 0
			qc_do_sleep_out(mipi_channel);
			unsigned char page = 1;
			
			vcom_otp_h = qc_read_reg(mipi_channel, page, 0);
			vcom_otp_l = qc_read_reg(mipi_channel, page, 1);
			vcom_otp_times = qc_read_reg(mipi_channel, page, 2);
			
			qc_do_sleep_in(mipi_channel);
			#else

			#ifdef USED_IC_MANAGER
			int temp_vcom_otp_times = 0;
			int temp_vcom_otp_value = 0;
			ic_mgr_read_chip_vcom_otp_value(mipi_channel, &temp_vcom_otp_times, &temp_vcom_otp_value);
			vcom_otp_times = temp_vcom_otp_times;
			vcom_otp_h = (temp_vcom_otp_value >> 8) & 0xFF;
			vcom_otp_l = temp_vcom_otp_value & 0xFF;
			#else
			qc_otp_data_read_vcom(mipi_channel, &vcom_otp_h, &vcom_otp_l, &vcom_otp_times);
			#endif
			
			#endif
			
			//vcom_otp_h = 1;
			//vcom_otp_l = 2;
			//vcom_otp_times = 3;

			printf("read otp info: vcom: %02x, %02x, times: %02x.\n", vcom_otp_h, vcom_otp_l, vcom_otp_times);

			client_get_vcom_otp_info_ack(mipi_channel, vcom_otp_h, vcom_otp_l, vcom_otp_times);
        }
        break;
		
		case SERVER2CLI_MSG_OTP:
		{
			printf("=====: SERVER2CLI_MSG_OTP. \n%s\n", (char*)pSocketCmd->pcmd);
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *Channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_vcom = cJSON_GetObjectItem(root, "vcom");
			cJSON *json_max_otp_times = cJSON_GetObjectItem(root, "max_vcom_otp_times");
			
			int channel = Channel->valueint;
			int vcom = 0;
			int max_otp_times = QC_DEFAULT_VCOM_MAX_OTP_TIMES;
			
			if (json_vcom)
				vcom = json_vcom->valueint;

			if (json_max_otp_times)
			{
				max_otp_times = json_max_otp_times->valueint;
			}
			
			cJSON_Delete(root);

			printf("manual vcom otp: channel: %d, vcom: %d, %#x, max_otp_times = %d.\n", channel, vcom, vcom, max_otp_times);

			unsigned char vcom_h = (vcom >> 8) & 0xFF;
			unsigned char vcom_l = vcom & 0xFF;			
			int otp_check_error = E_ERROR_OK;

			// read old vcom and times,			
			unsigned char old_vcom_h = 0;
			unsigned char old_vcom_l = 0;
			unsigned char old_otp_times = 0;
			qc_otp_data_read_vcom(channel, &old_vcom_h, &old_vcom_l, &old_otp_times);

			//max_vcom_times = QC_VCOM_MAX_OTP_TIMES;

			if ( (old_vcom_h != vcom_h) || (old_vcom_l != vcom_l) )
			{
				printf("do new otp proc ...\n");
				
				#if ENABLE_OTP_TESE_MODE
				max_otp_times = old_otp_times + 1;
				#endif
				
				if (max_otp_times <= old_otp_times)
				{				
					printf("manual otp: VCOM OTP Times error! max otp times: %d, read otp: %d.\n", max_otp_times, old_otp_times);
					otp_check_error = E_ERROR_OTP_TIMES_END;

					fpga_show_color_picture(0xFF, 0xFF, 0xFF);
					
					client_sendOtp(channel, otp_check_error, max_otp_times);
				}
				else
				{
					int ptn_id = client_pg_ptnId_get();
					// do otp
					qc_otp_vcom(channel, vcom_h, vcom_l, ptn_id);

					fpga_show_color_picture(0xFF, 0xFF, 0xFF);
					
					// read new vcom and times.
					unsigned char new_vcom_h = 0;
					unsigned char new_vcom_l = 0;
					unsigned char new_otp_times = 0;
					qc_otp_data_read_vcom(channel, &new_vcom_h, &new_vcom_l, &new_otp_times);

					// check otp burn result.
					#if ENABLE_OTP_TESE_MODE
					new_otp_times = old_otp_times + 1;
					#endif
					
					if (new_otp_times <= old_otp_times)
					{
						printf("VCOM OTP Times error! old times: %d, new times: %d.\n", old_otp_times, new_otp_times);
						otp_check_error = E_ERROR_OTP_TIMES_NOT_CHANGE;
					}

					if (!otp_check_error)
					{	
						#if ENABLE_OTP_TESE_MODE
						new_vcom_h = vcom_h;
						new_vcom_l = vcom_l;
						#endif
						
						if ( (vcom_h != new_vcom_h) || (vcom_l != new_vcom_l) )
						{
							printf("VCOM OTP data error! write vcom: %d, read otp vcom: %d.\n", vcom_h, vcom_l, 
									new_vcom_h, new_vcom_l);
							otp_check_error = E_ERROR_OTP_DATA_WRITE_FAILED;
						}
					}

					#if 0
							//得到次数, 最佳VCOM值
							extern unsigned int m_flick_manual_vcom;
							unsigned char vcom = m_flick_manual_vcom & 0xff;
							int otp = 0;

							mipi_to_lp_mode(channel);
							
							// check VCOM OTP times.
							if (chip_is_helitai_8019() == 0)
							{
						#ifdef USED_IC_MANAGER
								ic_mgr_read_vcom(channel, &vcom);
								ic_mgr_read_chip_vcom_otp_times(channel, &otp);
						#endif		
							}
							else
							{
								mipi_to_hs_mode(channel);
								printf("Unknow Drive IC!\n");
								client_sendOtp(channel);
								printf("SERVER2CLI_MSG_OTP end.\n");
								break;
							}
							
							//开始烧录
							int err = 0;
							char txt[100];
							memset(txt, 0, sizeof(txt));
							
							mipi_to_lp_mode(channel);
							mipi_reset(channel);
							usleep(5 * 1000);
							printf("%d--------------------------------- mipi_reset \n", channel);

					#ifdef USED_IC_MANAGER
							printf("IC Manager OTP: vcom = %d. \n", vcom);
							//vcom = 0x22;
							ic_mgr_write_chip_vcom_otp_value(channel, vcom);
					#endif

							mipi_to_hs_mode(channel);
							
							usleep(300 * 1000);
							client_pg_shutON(1, m_current_module_filename, NULL);

							// check OTP
					#ifdef USED_IC_MANAGER
							int read_vcom = 0;
							ic_mgr_check_vcom_otp_burn_ok(channel, vcom, otp, &read_vcom);
					#endif
					#endif

					printf("vcom otp msg: result = %d. max_otp_times = %d.\n", otp_check_error, max_otp_times);
					client_sendOtp(channel, otp_check_error, max_otp_times);
				}
			
			}
			else
			{	
				fpga_show_color_picture(0xFF, 0xFF, 0xFF);
				
				printf("manual otp: VCOM OTP data same! don't do otp.\n");
				client_sendOtp(channel, otp_check_error, max_otp_times);
			}
			
			printf("SERVER2CLI_MSG_OTP end.\n");
		}
		break;

		// reg read/write test.
		case SERVER2CLI_MSG_READ_ID_OTP_INFO:
		{
			printf("=====: SERVER2CLI_MSG_READ_ID_OTP_INFO. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "mipi_channel");

			int mipi_channel = json_mipi_channel ? json_mipi_channel->valueint : 1;

			printf("read id: channel = %d.\n", mipi_channel);
			
            cJSON_Delete(root);

			unsigned char otp_id1 = 0;
			unsigned char otp_id2 = 0;
			unsigned char otp_id3 = 0;
			unsigned char otp_times = 0;

			
			unsigned char id_datas[8] = { 0 };
			cliService_t* p_client = get_client_service_data();
			int id_len = 4;

			#if 0
			qc_do_sleep_out(mipi_channel);
			unsigned char page = 0;
			
			otp_id1 = qc_read_reg(mipi_channel, page, 0x78);
			otp_id2 = qc_read_reg(mipi_channel, page, 0x79);
			otp_id3 = qc_read_reg(mipi_channel, page, 0x7A);
			otp_times = qc_read_reg(mipi_channel, page, 0x7C);
			
			qc_do_sleep_in(mipi_channel);
			#else

			#ifdef USED_IC_MANAGER
			//unsigned char id_info[3] = { 0 };
			//int id_data_len = 3;
			int ret = ic_mgr_read_chip_id_otp_info(mipi_channel, id_datas, &id_len, &otp_times);
			if (ret != 0)
			{
				id_len = 0;
			}
			//otp_id1 = id_info[0];
			//otp_id2 = id_info[1];
			//otp_id3 = id_info[2];

			printf("ic_mgr_read_chip_id_otp_info\n");
			#else			
			qc_otp_data_read_id(mipi_channel, &otp_id1, &otp_id2, &otp_id3, &otp_times);
			id_datas[0] = otp_id1;
			id_datas[1] = otp_id2;
			id_datas[2] = otp_id3;
			printf("read otp info: id: %02x, %02x, %02x, times: %02x.\n", otp_id1, otp_id2, otp_id3, otp_times);
			#endif
			
			#endif

			printf("read id otp data: len = %d.\n", id_len);
			dump_data1(id_datas, id_len);

			//client_get_id_otp_info_ack(mipi_channel, otp_id1, otp_id2, otp_id3, otp_times);
			client_send_gen_cmd_read_lcd_id_notify(&p_client->socketCmdClass, p_client->send_fd, mipi_channel,
													id_datas, id_len, otp_times);
        }
        break;

		case SERVER2CLI_MSG_WRITE_ID_OTP_INFO: 
		{
			printf("=====: SERVER2CLI_MSG_WRITE_ID_OTP_INFO. \n%s\n", (char*)pSocketCmd->pcmd);
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *Channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_id1 = cJSON_GetObjectItem(root, "id1");
			cJSON *json_id2 = cJSON_GetObjectItem(root, "id2");
			cJSON *json_id3 = cJSON_GetObjectItem(root, "id3");
			cJSON *json_max_otp_times = cJSON_GetObjectItem(root, "max_id_otp_times");
			
			int channel = Channel->valueint;
			unsigned char id1 = 0;
			unsigned char id2 = 0;
			unsigned char id3 = 0;
			int max_otp_times = QC_DEFAULT_ID_MAX_OTP_TIMES;
			
			if (json_max_otp_times)
				max_otp_times = json_max_otp_times->valueint;
			
			if (json_id1)
				id1 = json_id1->valueint;
			if (json_id2)
				id2 = json_id2->valueint;
			if (json_id3)
				id3 = json_id3->valueint;
			
			cJSON_Delete(root);

			printf("manual otp: channel: %d, id: 0x%02x, 0x%02x, 0x%02x. max_otp_times = %d.\n", channel, 
						id1, id2, id3, max_otp_times);

			int otp_check_error = E_ERROR_OK;

			// read old vcom and times,			
			unsigned char old_id1 = 0;
			unsigned char old_id2 = 0;
			unsigned char old_id3 = 0;
			unsigned char old_otp_times = 0;
			
			qc_otp_data_read_id(channel, &old_id1, &old_id2, &old_id3, &old_otp_times);

			#if ENABLE_OTP_TESE_MODE
			max_otp_times = old_otp_times + 1;
			#endif

			if ( (old_id1 != id1) || (old_id2 != id2) || (old_id3 != id3))
			{
				printf("id otp msg: do otp proc ...\n");
				
				if (max_otp_times <= old_otp_times)
				{				
					printf("ID otp: ID OTP Times error! max otp times: %d, read otp: %d.\n", max_otp_times, old_otp_times);
					otp_check_error = E_ERROR_OTP_TIMES_END;
					client_send_id_Otp_end(channel, otp_check_error, max_otp_times);
				}
				else
				{
					// burn id.
					int ptn_id = client_pg_ptnId_get();

					#ifdef USED_IC_MANAGER
					unsigned char id_data[3] = { 0 }; 
					int id_data_len = 3;
					id_data[0] = id1;
					id_data[1] = id2;
					id_data[2] = id3;
					
					ic_mgr_burn_chip_id(channel, id_data, id_data_len, ptn_id);
					#else
					qc_otp_id(channel, id1, id2, id3, ptn_id);
					#endif

					// read new vcom and times.
					unsigned char new_id1 = 0;
					unsigned char new_id2 = 0;
					unsigned char new_id3 = 0;
					unsigned char new_otp_times = 0;
					qc_otp_data_read_id(channel, &new_id1, &new_id2, &new_id3, &new_otp_times);

					// check otp burn result.
					#if ENABLE_OTP_TESE_MODE
					new_otp_times = old_otp_times + 1;
					#endif
					
					if (new_otp_times <= old_otp_times)
					{
						printf("ID OTP Times error! old times: %d, new times: %d.\n", old_otp_times, new_otp_times);
						otp_check_error = E_ERROR_OTP_TIMES_NOT_CHANGE;
					}

					if (!otp_check_error)
					{	
						#if ENABLE_OTP_TESE_MODE
						new_id1 = id1;
						new_id2 = id2;
						new_id3 = id3;
						#endif
						
						if ( (id1 != new_id1) || (id2 != new_id2) || (id3 != new_id3) )
						{
							printf("VCOM OTP data error! write vcom: %d, read otp vcom: %d.\n", id1, id2, id3, 
									new_id1, new_id2, new_id3);
							otp_check_error = E_ERROR_OTP_DATA_WRITE_FAILED;
						}
					}
					
					printf("id otp msg: result = %d. max_otp_times = %d.\n", otp_check_error, max_otp_times);
					client_send_id_Otp_end(channel, otp_check_error, max_otp_times);
				}
			}
			else
			{				
				printf("id otp msg: id same, don't do otp.\n");
				client_send_id_Otp_end(channel, otp_check_error, max_otp_times);
			}
			
			printf("SERVER2CLI_MSG_WRITE_ID_OTP_INFO end.\n");
		}
		break;

		case SERVER2CLI_MSG_GENERAL_COMMAND_REQ:
		{
			printf("SERVER2CLI_MSG_GENERAL_COMMAND_REQ\n");
			
			// MSG_TAG: command key.
			cliService_t* p_client_obj = get_client_service_data();
			if (p_client_obj == NULL)
			{
				printf("SERVER2CLI_MSG_GENERAL_COMMAND_REQ error: p_client_obj = NULL!\n");
				break;
			}

			net_general_command_proc(&p_client_obj->socketCmdClass, p_client_obj->send_fd, pSocketCmd->pcmd, pSocketCmd->len);
		}
		break;
		
        default:
        	DBG("unknow cmd is 0x%x", pSocketCmd->cmd);
        break;
    }

    if(pSocketCmd->pcmd && pSocketCmd->len)
    {
        free(pSocketCmd->pcmd);
        pSocketCmd->pcmd = 0;
    }
	
    free(pSocketCmd);
}

