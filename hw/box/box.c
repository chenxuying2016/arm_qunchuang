#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <semaphore.h>
#include "pgos/MsOS.h"
#include "common.h"
#include "comStruct.h"
#include "box.h"
#include "comUart.h"
#include "pgDB/pgDB.h"
#include "vcom.h"
#include "pubmipi.h"

#ifdef ENABLE_CONTROL_BOX

#ifdef USED_IC_MANAGER
#include "icm/ic_manager.h"
#endif


#define WHILE_TEST           0
#define ENABLE_MTP_ON        1
#define ENABLE_GPIO_RESET    1
#define ENABLE_SWRESET       0
#define ENABLE_POWER_ON_OFF  0
unsigned int test_count = 0;

extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;


static pthread_mutex_t gBoxReadLock;
static pthread_mutex_t gBoxGuardLock;
static volatile int gBoxKeyReturn = 0;
static MS_APARTMENT *boxApartMent = 0;
static time_t boxtime_alive = 0;

static void boxRcvThread();
static void boxHandThread();

volatile int gBoxUpdateState = 0;
extern volatile int gPowerUpdateState;

unsigned int        boxSoftwareVersion = 0;
static volatile int boxSoftwareVersion_request = 1;
static sem_t        boxSoftwareVersion_sem;
static pthread_t    boxSoftwareVersion_pthread;

unsigned int box_get_version()
{
	return boxSoftwareVersion;
}

static void boxThreadCreate(void)
{
    INT ret = 0;
    pthread_t boxthread1;
    pthread_t boxthread2;
    ret = pthread_create(&boxthread1, NULL, (void *) boxHandThread, NULL);
    ret = pthread_create(&boxthread2, NULL, (void *) boxRcvThread, NULL);
    if (ret != 0)
    {
        printf("can't create thread: %s.\r\n", strerror(ret));
        return ;
    }
	
    pthread_detach(boxthread1);
    pthread_detach(boxthread2);
}

static box_cur_state_t  box_cur_state;

#define BOX_TYPE 0xF0

static INT boxCmdTransfer(BYTE cmd, void *pBuf, WORD16 nLen,void *pNode)
{
    int ret  = 0;
    int sendLen = sizeof(SMsgHead)+nLen;
    char *pSendBuf = (char*)malloc(sendLen);
    if(nLen>0)
    {
        memcpy(&pSendBuf[sizeof(SMsgHead)],pBuf,nLen);
    }
	
    SMsgHead *pSmgHead = (SMsgHead *)pSendBuf;
    pSmgHead->type = BOX_TYPE;
    pSmgHead->cmd  = cmd;
    pSmgHead->len  = nLen;
    pthread_mutex_lock(&gBoxReadLock);

    ret = uartSendMsgToBoard(getBoxUartNo(), pSendBuf, sendLen);
	//printf("==== boxCmdTransfer: len = %d. \n", sendLen);
	//dump_data1(pSendBuf, sendLen);
			
    pthread_mutex_unlock(&gBoxReadLock);
	free(pSendBuf);
    return ret;
}

void boxSavePwrData(sByPtnPwrInfo *pPwrData)
{
	if(gBoxKeyReturn == 0) 
		return;
	
    memcpy(&box_cur_state.pwrInfo,pPwrData,sizeof(sByPtnPwrInfo));
    boxpwr_info_t boxpwr_info;
    boxpwr_info.vdd = pPwrData->VDD;
    boxpwr_info.idd = pPwrData->iVDD;
    boxpwr_info.vbl = pPwrData->VBL;
    boxpwr_info.ibl = pPwrData->iVBL;
    boxCmdTransfer(CHECK_PWR_KEY,&boxpwr_info,sizeof(boxpwr_info),0);
}

void boxRebackValue(int pgOptCheckValue)
{
	unsigned char cmd[8];
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0;
	cmd[1] = 0;
	//FlickerResult
	unsigned short FlickerResult = 1;
	cmd[3] = (FlickerResult >> 8) & 0xff;
	cmd[2] = (FlickerResult) & 0xff; //?????????     ???Flicker NG ???????? 6
	//0-Flicker OK ?????? 
	//1-Flicker NG ???????? 1

	//BurnTime x 100      4 = 0.04S
	unsigned short BurnTime = 4 * 100;
	cmd[5] = (BurnTime >> 8) & 0xff;
	cmd[4] = (BurnTime) & 0xff;
	//
//	cmd[7] = 1;
//	cmd[6] = 2; //0-OK,  1-NG
	//
//	cmd[9] = 8;
//	cmd[8] = 1;
	boxCmdTransfer(OPT_KEY, cmd, sizeof(cmd), 0);
}

void boxRebackFlicker(unsigned short FlickerResult, unsigned short BurnTime, char* otp1, char* otp2)
{
	int i;
	box_otp_info_t info;
	unsigned char* ptr;

	memset(&info, 0x20, sizeof(info));

	// channel 1 message.
	if(otp1 != NULL)
	{
		for(i=0; i < MAX_ROW_CHAR; i++)
		{
			unsigned char v = (unsigned char)(otp1[i]);
			if(v == 0) break;
			info.ch1[i] = v;
		}
	}

	// channel 2 message.
	if(otp2 != NULL)
	{
		for(i=0; i < MAX_ROW_CHAR; i++)
		{
			unsigned char v = (unsigned char)(otp2[i]);
			if(v == 0) break;
			info.ch2[i] = v;
		}
	}
	
#if 0
	printf("\n");
	// = (unsigned char*)&info;
	//int i;
	//
	ptr = info.ch1;
	//ptr = otp1;
	printf("OTP1: ");
	for(i=0; i < MAX_ROW_CHAR; i++)
	{
		printf("%02X ", (unsigned char)(ptr[i]));
	}
	printf("\n");
	//
	ptr = info.ch2;
	//ptr = otp2;
	printf("OTP2: ");
	for(i=0; i < MAX_ROW_CHAR; i++)
	{
		printf("%02X ", ptr[i]);
	}
	printf("\n");

#endif

	boxCmdTransfer(OPT_KEY, &info, sizeof(info), 0);
}

static void boxHandThread()
{
    while(1)
    {
		if(gBoxUpdateState == 1 || gPowerUpdateState == 1 || gBoxKeyReturn == 1)
		{
			usleep(10 * 1000);
			continue;
		}
		
        pthread_mutex_lock(&gBoxGuardLock);
        time_t t = time(NULL);
        
        if(t-boxtime_alive>10)
        {
            boxtime_alive = t;
            if (getBoxUartNo() >= 0)
            {
                boxCmdTransfer(HAND_KEY,0,0,0);
                box_cur_state.isConnected = 0;
                //printf("!!!!!!!!!!!!!!!!!!!!!hand!!!!!!!!!!!!!!!!!!!!\n");
            }
        }
        pthread_mutex_unlock(&gBoxGuardLock);
		usleep(10 * 1000);
    }
}

static int use_SendCode = 0;

static int m_box_vcom_info_first = 1;
static vcom_info_t m_box_vcom_info;
#define USE_POWER_ON_OFF   1
#define USE_GPIO_RESET_2828_RESET   0


static void boxProcCmd(int cmd)
{
    switch(cmd)
    {
        case  UP_KEY:
        {
            if(!box_cur_state.isConnected)
                break;
			
            if(box_cur_state.boxLayer == MODULE_LAYER)
            {
                module_self_t *pModule_Self  = dbModuleSelfGetByIndex(box_cur_state.curModuleIndex - 1);
                if(pModule_Self)
                { 
					strncpy(box_cur_state.curModuleName, pModule_Self->moduleselfName, 31);
                    strncpy(box_cur_state.curModulePwrName, pModule_Self->modulePowerName, 31);
					box_cur_state.curModuleIndex = pModule_Self->index;
					box_cur_state.curTotalPtnNums = pModule_Self->totalPattern;
					
                    change_info_t change_info;
                    change_info.isModuleFlag = 1;
                    strncpy(change_info.moduleName, pModule_Self->moduleselfName, 31);
                    strncpy(change_info.pwrName, pModule_Self->modulePowerName, 31);
                    strncpy(change_info.pictureName, pModule_Self->patternFirstName, 31);
                    change_info.curindex = box_cur_state.curModuleIndex;                    
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
					
                    printf("UP_KEY totalNum %d\n",change_info.totalNum);
                    boxCmdTransfer(UP_KEY, &change_info, sizeof(change_info), 0);
                }
            }
            else if(PICTURE_LAYER == box_cur_state.boxLayer)
            {
                int ptnId = box_cur_state.curPtnId - 1;
                module_info_t *pModule_info = dbModuleGetPatternById(ptnId);
                if(pModule_info)
                {
                    box_cur_state.curPtnId = ptnId;
                    client_pg_showPtn(ptnId);
					
                    change_info_t change_info = { 0 };
                    change_info.isModuleFlag = 2;
                    strncpy(change_info.moduleName, box_cur_state.curModuleName, 31);
                    strncpy(change_info.pwrName, box_cur_state.curModulePwrName, 31);
                    strncpy(change_info.pictureName, pModule_info->ptdpatternname, 31);
                    change_info.curindex = box_cur_state.curPtnId;
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
                    change_info.lockSec  = pModule_info->ptddisptime;
					
                    printf("UP_KEY NAME %s CURIndex %d curLock %d\n",change_info.pictureName,change_info.curindex,change_info.lockSec);
                    boxCmdTransfer(UP_KEY, &change_info, sizeof(change_info), 0);
                }
            }
        }
        break;

        case  DOWN_KEY:
        {
            if(!box_cur_state.isConnected)
                break;
			
            if(box_cur_state.boxLayer == MODULE_LAYER)
            {
                module_self_t *pModule_Self  = dbModuleSelfGetByIndex(box_cur_state.curModuleIndex + 1);
                if(pModule_Self)
                {
                    box_cur_state.curModuleIndex = pModule_Self->index;
					box_cur_state.curTotalPtnNums = pModule_Self->totalPattern;
					
                    change_info_t change_info;
                    change_info.isModuleFlag = 1;
                    strncpy(change_info.moduleName, pModule_Self->moduleselfName, 31);
                    strncpy(change_info.pwrName, pModule_Self->modulePowerName, 31);
                    strncpy(change_info.pictureName, pModule_Self->patternFirstName, 31);
                    strncpy(box_cur_state.curModuleName, pModule_Self->moduleselfName, 31);
                    strncpy(box_cur_state.curModulePwrName, pModule_Self->modulePowerName, 31);					
                    change_info.curindex = box_cur_state.curModuleIndex;                    
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
					
                    printf("DW_KEY totalNum %d\n", change_info.totalNum);
                    boxCmdTransfer(DOWN_KEY, &change_info, sizeof(change_info), 0);
                }
            }
            else if(PICTURE_LAYER == box_cur_state.boxLayer)
            {
				if(box_cur_state.pgPwrStatus == 0) 
					break; //xujie
					
                int ptnId = box_cur_state.curPtnId + 1;
				
                module_info_t *pModule_info = dbModuleGetPatternById(ptnId);
                if(pModule_info)
                {
                    box_cur_state.curPtnId = ptnId;
					
                    client_pg_showPtn(ptnId);
					
                    change_info_t change_info;
                    change_info.isModuleFlag = 2;
                    strncpy(change_info.moduleName, box_cur_state.curModuleName, 31);
                    strncpy(change_info.pwrName, box_cur_state.curModulePwrName, 31);
                    strncpy(change_info.pictureName, pModule_info->ptdpatternname, 31);
                    change_info.curindex = box_cur_state.curPtnId;
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
                    change_info.lockSec = pModule_info->ptddisptime;
					
                    printf("DWN_KEY NAME %s CURIndex %d curLock %d ptnNum %d\n", change_info.pictureName, change_info.curindex, change_info.lockSec, box_cur_state.curTotalPtnNums);
                    boxCmdTransfer(DOWN_KEY, &change_info, sizeof(change_info), 0);
                }
            }
        }
        break;

        case  POWER_KEY:
        {
			//printf("POWER_KEY\n");
			if(box_cur_state.boxLayer == MODULE_LAYER) 
				break;
			
            if(!box_cur_state.isConnected)
                break;
			
			printf("\n\n\n\n");
			gBoxKeyReturn = 1;
            printf("###############ptn num %d\n",box_cur_state.curTotalPtnNums);
			
            if(box_cur_state.pgPwrStatus)
            {
            	// power off.
				boxRebackFlicker(0, 0, NULL, NULL);

				client_pg_shutON(0, 0, NULL, 0);
				
                box_shut_info_t box_shut_info;
                box_shut_info.vcom = 0;
                box_shut_info.vcom_otp  = 0;
                box_shut_info.pgPwrStatus = client_pg_pwr_status_get();

				box_cur_state.pgPwrStatus = box_shut_info.pgPwrStatus;
				box_cur_state.curPtnId = box_shut_info.ptnIndex = 0;
                
                module_info_t *pModule_info = dbModuleGetPatternById(box_shut_info.ptnIndex);
                if(pModule_info)
                {
                    strncpy(box_shut_info.pictureName, pModule_info->ptdpatternname, 31);
                    box_shut_info.lockSec = 0;
                }
				
				gBoxKeyReturn = 0;
                boxCmdTransfer(POWER_KEY, &box_shut_info, sizeof(box_shut_info), 0);
				set_fpga_text_show(0, 0, 0, 0);
            }
            else
            {
						
            	printf("g_chip_helitai_8019: %d. ======\n", chip_is_helitai_8019());
            	if (chip_is_helitai_8019() == 0)
				{
					printf("====== other chip ======\n");
					#if 1
		            {
		            	// power on
						struct timeval tpstart, tpend; 
						float timeuse; 
						gettimeofday(&tpstart,NULL);

						extern int auto_flick_return_code;
						extern int auto_flick_return_code_2;
						auto_flick_return_code = FLICKER_OK;
						auto_flick_return_code_2 = FLICKER_OK;

						int readVcom1 = 0;
						int readVcom2 = 0;
						int vcomVcomValue1 = 0;
						int vcomFlickValue1 = 0;
						int vcomVcomValue2 = 0;
						int vcomFlickValue2 = 0;
						int id1 = 0;
						int id2 = 0;
						int id3 = 0;

						int vcom_otp_2 = 0;
						int id1_2 = 0;
						int id2_2 = 0;
						int id3_2 = 0;
						vcom_info_t vcom_info;
						
						{
							memset(&vcom_info, 0, sizeof(vcom_info_t));
							vcom_info.vcomBurner = -1;

							vcom_info_t* p_vcom_info = get_current_vcom_info();
							if (p_vcom_info)
							{
								vcom_info = *p_vcom_info;
							}
							
						}
						
						if(vcom_info.vcomBurner == -1)
						{
							vcom_info.vcomBurner = 1;
							vcom_info.vcomotpmax = 1;
						}
						
#if 1
						//xujie
						char otp1_text[100] = "开电，请等待...";
						char otp2_text[100] = "开电，请等待...";
						
						//Clear FPGA Show Message.
						set_fpga_text_show(0,0,0,0); //VCOM, OTP, OK, OK2
						set_fpga_text_vcom(m_channel_1, 0);
						set_fpga_text_flick(m_channel_1, 0);
						set_fpga_text_vcom(m_channel_2, 0);
						set_fpga_text_flick(m_channel_2, 0);
						
						boxRebackFlicker(0, 0, otp1_text, otp2_text);
						
						client_pg_shutON(1, box_cur_state.curModuleName, NULL, 0);

						sprintf(otp1_text, "开电完成");
						sprintf(otp2_text, "开电完成");
						boxRebackFlicker(0, 0, otp1_text, otp2_text);
#endif

						module_info_t* p_module_vcom_info = db_get_vcom_module_info();						
						if (p_module_vcom_info)
						{
							setHsmode(0);
							
			                box_cur_state.pgPwrStatus = client_pg_pwr_status_get();
			                box_shut_info_t box_shut_info;
							memset(&box_shut_info, 0, sizeof(box_shut_info));
							
							// check LCD Module Status. read lcd id.
							#ifdef USED_IC_MANAGER
							{
								printf("check LCD Module ...\n");
								int channel1_mod_statsu = ic_mgr_check_chip_ok(m_channel_1);
								int channel2_mod_statsu = ic_mgr_check_chip_ok(m_channel_2);
								if (channel1_mod_statsu != 0)
								{
									printf("***  channel %d: check id error!  ***\n", m_channel_1);
									auto_flick_return_code = FLICKER_NG_1;
									sprintf(otp1_text, "NG 无法读取屏ID信息");
									printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
								}
								
								if (channel2_mod_statsu != 0)
								{
									printf("***  channel %d: check id error!  ***\n", m_channel_2);
									auto_flick_return_code_2 = FLICKER_NG_1;
									sprintf(otp2_text, "NG 无法读取屏ID信息");
									printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
								}
							}
							#endif

							// read VCOM OTP times.
							#ifdef USED_IC_MANAGER
							if (1)
							{
								box_shut_info.alloptStatus = 1; 
								if(auto_flick_return_code == FLICKER_OK)
								{
									unsigned char vcom_times = 0;
									int read_error = ic_mgr_read_chip_vcom_otp_times(m_channel_1, &vcom_times);
									if( read_error != 0)
									{
										auto_flick_return_code = FLICKER_NG_1;
										sprintf(otp1_text, "NG 读取烧录次数失败");
										printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
									}

									box_shut_info.vcom_otp = vcom_times;
									printf("=== read channel %d otp times: %d. ===\n", m_channel_1, vcom_times);
								}
								
								if(auto_flick_return_code_2 == FLICKER_OK) 
								{
									unsigned char vcom_times = 0;
									int read_error = ic_mgr_read_chip_vcom_otp_times(m_channel_2, &vcom_times);
									if( read_error != 0)
									{
										auto_flick_return_code_2 = FLICKER_NG_1; 
										sprintf(otp2_text, "NG 读取烧录次数失败");
										printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
									}

									vcom_otp_2 = vcom_times;
									printf("=== read channel %d otp times: %d. ===\n", m_channel_2, vcom_times);
								}
							}
							#endif
							
							// read current VCOM
							#ifdef USED_IC_MANAGER
							{
								if(auto_flick_return_code == FLICKER_OK)
								{
									int temp_vcom = 0;
									int temp_vcom_times = 0;
									int read_error = ic_mgr_read_chip_vcom_otp_value(m_channel_1, &temp_vcom_times, &temp_vcom);
									if(read_error != 0)
									{
										auto_flick_return_code = FLICKER_NG_1;
										sprintf(otp1_text, "NG 读取VCOM失败");
										printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
									}
									else
									{
										printf("=== read channel %d vcom: %d. ===\n", m_channel_1, temp_vcom);
									}

									readVcom1 = temp_vcom;
								}
								
								if(auto_flick_return_code_2 == FLICKER_OK) 
								{
									int temp_vcom = 0;
									int temp_vcom_times = 0;
									int read_error = ic_mgr_read_chip_vcom_otp_value(m_channel_2, &temp_vcom_times, &temp_vcom);
									if(read_error != 0)
									{
										auto_flick_return_code_2 = FLICKER_NG_1; 
										sprintf(otp2_text, "NG 读取VCOM失败");
										printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
									}
									else
									{
										printf("=== read channel %d vcom: %d. ===\n", m_channel_2, temp_vcom);
									}
									
									readVcom2 = temp_vcom;
								}
							}
							#endif
							
							//Active Output, screen on.
							WriteSSD2828(m_channel_1, 0XB7, 0X030b);
							WriteSSD2828(m_channel_2, 0XB7, 0X030b);
							setHsmode(1);

							//Set init vcom for auto flick.
							#ifdef USED_IC_MANAGER
							{
								printf("read vcom otp times: %d, vcom max otp times: %d.\n", box_shut_info.vcom_otp, vcom_info.vcomotpmax);

								if(vcom_info.vcomBurner > 0)
								{
									if(box_shut_info.vcom_otp < vcom_info.vcomotpmax && auto_flick_return_code == FLICKER_OK)
									{
										ic_mgr_write_vcom(m_channel_1, vcom_info.initVcom);
										printf("channel: %d set init VCOM: %d.\n", m_channel_1, vcom_info.initVcom);
									}
									
									if(vcom_otp_2 < vcom_info.vcomotpmax && auto_flick_return_code_2 == FLICKER_OK)
									{
										ic_mgr_write_vcom(m_channel_2, vcom_info.initVcom);
										printf("channel: %d set init VCOM: %d.\n", m_channel_2, vcom_info.initVcom);
									}
								}
							}
							#endif
							
							// wait a moment for screen on.
							if(vcom_info.pwrOnDelay > 0 && vcom_info.vcomBurner > 0)
							{
								int do_vcom_adjust = 0;
								if (box_shut_info.vcom_otp < vcom_info.vcomotpmax && auto_flick_return_code == FLICKER_OK) 
								{
									sprintf(otp1_text, "点屏后延时%dms,等屏稳定", vcom_info.pwrOnDelay);
									do_vcom_adjust = 1;
								}

								if (vcom_otp_2 < vcom_info.vcomotpmax && auto_flick_return_code_2 == FLICKER_OK)
								{
									sprintf(otp2_text, "点屏后延时%dms,等屏稳定", vcom_info.pwrOnDelay);
									do_vcom_adjust = 1;
								}

								if (do_vcom_adjust)
								{
									boxRebackFlicker(0, 0, otp1_text, otp2_text);
									usleep(vcom_info.pwrOnDelay * 1000);
								}
							}
							
			                box_shut_info.pgPwrStatus = client_pg_pwr_status_get();
			                box_cur_state.curPtnId = box_shut_info.ptnIndex = 0;
			                module_info_t *pModule_info = dbModuleGetPatternById(box_shut_info.ptnIndex);
			                if(pModule_info)
			                {
			                    box_shut_info.lockSec = pModule_info->ptddisptime;
			                    strncpy(box_shut_info.pictureName,pModule_info->ptdpatternname,31);
								printf("]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]%s\n", pModule_info->initcodefile);
			                }
			                printf("POWER_KEY pictrue Name %s vom %d %d %d sec:%d\n",box_shut_info.pictureName,box_shut_info.vcom,box_shut_info.vcom_otp,box_shut_info.alloptStatus,box_shut_info.lockSec );
							
			                boxCmdTransfer(POWER_KEY, &box_shut_info, sizeof(box_shut_info),0);

							// check OTP burn times.
							#if 1
							if(vcom_info.idBurner || vcom_info.vcomBurner || vcom_info.gammaBurner || vcom_info.codeBurner)
							{
								printf("check OTP burn times ...\n");
								
								if(box_shut_info.vcom_otp >= vcom_info.vcomotpmax && auto_flick_return_code == FLICKER_OK)
								{
									auto_flick_return_code = FLICKER_NG_1;
									// NG 已烧录次数是%d
									sprintf(otp1_text, "NG 已烧录次数是%d", box_shut_info.vcom_otp);
									printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
								}
								
								if(vcom_otp_2 >= vcom_info.vcomotpmax && auto_flick_return_code_2 == FLICKER_OK) 
								{
									auto_flick_return_code_2 = FLICKER_NG_1; 
									sprintf(otp2_text, "NG 已烧录次数是%d", vcom_otp_2);
									printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
								}
							}
							#endif

							// check FLICK Device status.
							#if 1
							if(vcom_info.vcomBurner > 0)
							{
								printf("check FLICK Device status ...\n");
								
								if(auto_flick_return_code == FLICKER_OK)
								{
									if(lcd_getStatus(1) == 0)
									{
										auto_flick_return_code = FLICKER_NG_1;
										sprintf(otp1_text, "NG 没有检查到FLIC设备");
										printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
									}
								}
								
								if(auto_flick_return_code_2 == FLICKER_OK) 
								{
									if(lcd_getStatus(2) == 0)
									{
										auto_flick_return_code_2 = FLICKER_NG_1; 
										sprintf(otp2_text, "NG 没有检查到FLIC设备");
										printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
									}
								}
							}
							#endif
							
							////////////////////////////////////////////////////////////////////////////
							////////////////////////////////////////////////////////////////////////////
							// Start Box Auto Flick Task.
							#if 1				
							if(vcom_info.vcomBurner > 0) //VCOM
							{
								printf("Start Box Auto Flick Task ...\n");
								if(auto_flick_return_code == FLICKER_OK || auto_flick_return_code_2 == FLICKER_OK)
								{
									if (auto_flick_return_code == FLICKER_OK)
										sprintf(otp1_text, "自动FLICK, 请等待...");

									if (auto_flick_return_code_2 == FLICKER_OK)
										sprintf(otp2_text, "自动FLICK, 请等待...");
									
									boxRebackFlicker(0, 0, otp1_text, otp2_text);

									int vcom_ptn_index = get_current_vcom_ptn_index();
									if (vcom_ptn_index >= 0)
									{
										printf("switch flick picture: %d.\n", vcom_ptn_index);
										client_pg_showPtn(vcom_ptn_index);
										box_autoFlick(&vcom_info, &vcomVcomValue1, &vcomFlickValue1, &vcomVcomValue2, &vcomFlickValue2, otp1_text, otp2_text);
										printf("auto flick is end. vcom1: %d, flick1: %d. vcom2: %d, flick2: %d.\n",
												vcomVcomValue1, vcomFlickValue1, vcomVcomValue2, vcomFlickValue2);
									}
									else
									{
										auto_flick_return_code == FLICKER_NG_1;
										auto_flick_return_code_2 == FLICKER_NG_1;

										sprintf(otp1_text, "NG: 找不到Flick图片!");
										sprintf(otp1_text, "NG: 找不到Flick图片!");
									}
									
									if(auto_flick_return_code == FLICKER_OK) 
									{
										if(vcomVcomValue1 == 0)
										{
											auto_flick_return_code = FLICKER_NG_1; 
											sprintf(otp1_text, "NG VCOM值无效：%d,请重试", vcomVcomValue1);
											printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
										}

										if (vcomFlickValue1 < vcom_info.f_min_valid_flick * 100)
										{
											float temp_flick = ((float)vcomFlickValue1) / 100.00F;
											printf("temp_flick = %f.\n", temp_flick);
											auto_flick_return_code = FLICKER_NG_1; 
											sprintf(otp1_text, "NG Flick值无效：%0.2f,请重试", temp_flick);
											printf("mipi_channel: %d error: %s, min_valid_flick is %f. \n", m_channel_1, 
												otp1_text, vcom_info.f_min_valid_flick);
										}
									}
									
									if(auto_flick_return_code_2 == FLICKER_OK) 
									{
										if(vcomVcomValue2 == 0)
										{
											auto_flick_return_code_2 = FLICKER_NG_1; 
											sprintf(otp2_text, "NG VCOM值无效：%d,请重试", vcomVcomValue2);
											printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
										}

										if (vcomFlickValue2 < vcom_info.f_min_valid_flick * 100)
										{
											float temp_flick = ((float)vcomFlickValue2) / 100.00F;
											printf("temp_flick = %f.\n", temp_flick);
											auto_flick_return_code_2 = FLICKER_NG_1; 
											sprintf(otp2_text, "NG Flick值无效：%0.2f,请重试", temp_flick);
											printf("mipi_channel: %d error: %s, min_valid_flick is %f. \n", m_channel_2, 
												otp2_text, vcom_info.f_min_valid_flick);
										}
									}
								}
							}
							#endif
							
							////////////////////////////////////////////////////////////////////////////
							// VCOM Burn.
							////////////////////////////////////////////////////////////////////////////
							if((vcom_info.idBurner || vcom_info.vcomBurner || vcom_info.gammaBurner || vcom_info.codeBurner) && (auto_flick_return_code == FLICKER_OK || (auto_flick_return_code_2 == FLICKER_OK))) //??????
							{
								//start otp burn, waitting ...
								if (auto_flick_return_code == FLICKER_OK)
									sprintf(otp1_text, "烧录开始，请等待...");

								if (auto_flick_return_code_2 == FLICKER_OK)
									sprintf(otp2_text, "烧录开始，请等待...");
								
								boxRebackFlicker(0, 0, otp1_text, otp2_text);

								#ifdef USED_IC_MANAGER
								{
									printf("VCOM Burn ...\n");
									
									#if 1
									if(auto_flick_return_code == FLICKER_OK)
									{
										int old_vcom1 = 0;
										int old_vcom2 = 0;
										ic_mgr_write_chip_vcom_otp_value(m_channel_1, vcomVcomValue1);
									}
									
									if(auto_flick_return_code_2 == FLICKER_OK)
									{
										ic_mgr_write_chip_vcom_otp_value(m_channel_2, vcomVcomValue2);
									}
									#else
									printf("===== VCOM Burn: Do Nothing! ===================\n");
									#endif
								}
								#endif
								
								#if 1
								//otp burn end. screen on.
								{
									if(auto_flick_return_code == FLICKER_OK || auto_flick_return_code_2 == FLICKER_OK)
									{
										if (auto_flick_return_code == FLICKER_OK)
											sprintf(otp1_text, "烧录结束, 检查中，请等待...");

										if (auto_flick_return_code_2 == FLICKER_OK)
											sprintf(otp2_text, "烧录结束, 检查中，请等待...");
								
										boxRebackFlicker(0, 0, otp1_text, otp2_text);
									}

									{
#if ENABLE_GPIO_RESET
										setHsmode(0);
										mipi_reset(m_channel_1);
										mipi_reset(m_channel_2);
										printf("--- check mipi_gpio_reset \n");
										usleep(5 * 1000);
#endif
									}
								}

								#if 0
								//screen on
								//if((vcom_info.idBurner || vcom_info.vcomBurner || vcom_info.gammaBurner || vcom_info.codeBurner) && (auto_flick_return_code == FLICKER_OK || auto_flick_return_code_2 == FLICKER_OK))
								{
									SendCode(m_channel_1, InitCode);
									printf("channel: %d send InitCode \n", m_channel_1);
									SendCode(m_channel_2, InitCode);
									printf("channel: %d send InitCode \n",  m_channel_2);
									WriteSSD2828(m_channel_1,0XB7,0X030b);
									WriteSSD2828(m_channel_2,0XB7,0X030b);
									setHsmode(1);
								}
								#else
								//#define MAX_MIPI_CODE_LEN 8192
								//extern unsigned char InitCode[MAX_MIPI_CODE_LEN];

								#if 0
								mipi_to_lp_mode(m_channel_1);
								mipi_to_lp_mode(m_channel_2);
								
								SendCode(m_channel_1, InitCode);
								printf("channel: %d send InitCode \n", m_channel_1);
								SendCode(m_channel_2, InitCode);
								printf("channel: %d send InitCode \n",  m_channel_2);
								//WriteSSD2828(mipi_channel,0XB7,0X030b);
								//WriteSSD2828(m_channel_2,0XB7,0X030b);
								//setHsmode(1);
								
								mipi_to_hs_mode(m_channel_1);
								mipi_to_hs_mode(m_channel_2);
								#else	
								client_pg_shutON(1, box_cur_state.curModuleName, NULL, 0);
								#endif

								#endif
				
								////////////////////////////////////////////////////////////////////////////
								//check
								////////////////////////////////////////////////////////////////////////////
								if((vcom_info.idBurner || vcom_info.vcomBurner || vcom_info.gammaBurner || vcom_info.codeBurner) && (auto_flick_return_code == FLICKER_OK || auto_flick_return_code_2 == FLICKER_OK)) //??????????????????
								{
									usleep(150 * 1000);

									#ifdef USED_IC_MANAGER
									{
										printf("Burn end, check VCOM ...\n");
										
										if(auto_flick_return_code == FLICKER_OK)
										{
											int read_vcom = 0;
											int check_value =  ic_mgr_check_vcom_otp_burn_ok(m_channel_1, vcomVcomValue1, box_shut_info.vcom_otp, &read_vcom);
											if (check_value != 0)
											{
												auto_flick_return_code = FLICKER_NG_5;
												if (check_value == E_ICM_READ_ID_ERROR)
												{
													sprintf(otp1_text, "NG 无法读取屏ID信息");
													printf("check_burn_ok error: channel:%d, %s \n", m_channel_1, otp1_text);
												}
												else if (check_value == E_ICM_VCOM_TIMES_NOT_CHANGE)
												{
													sprintf(otp1_text, "NG 烧录失败,次数还是%d", box_shut_info.vcom_otp);
													printf("check_burn_ok error: channel:%d, %s \n", m_channel_1, otp1_text);
												}
												else if (check_value == E_ICM_VCOM_VALUE_NOT_CHANGE)
												{
													//NG 烧录失败,VCOM还是%d
													sprintf(otp1_text, "NG 烧录后,读取到VCOM是%d", read_vcom);
													printf("check_burn_ok error: channel:%d, %s \n", m_channel_1, otp1_text);
												}

											}
										}

										if(auto_flick_return_code_2 == FLICKER_OK)
										{
											int read_vcom = 0;
											int check_value =  ic_mgr_check_vcom_otp_burn_ok(m_channel_2, vcomVcomValue2, vcom_otp_2, &read_vcom);
											if (check_value != 0)
											{
												auto_flick_return_code_2 = FLICKER_NG_5;
												//NG 烧录失败,VCOM还是%d
												if (check_value == E_ICM_READ_ID_ERROR)
												{
													sprintf(otp2_text, "NG 无法读取屏ID信息");
													printf("check_burn_ok error: channel:%d, %s \n", m_channel_2, otp2_text);
												}
												else if (check_value == E_ICM_VCOM_TIMES_NOT_CHANGE)
												{
													sprintf(otp2_text, "NG 烧录失败,次数还是%d", vcom_otp_2);
													printf("check_burn_ok error: channel:%d, %s \n", m_channel_2, otp2_text);
												}
												else if (check_value == E_ICM_VCOM_VALUE_NOT_CHANGE)
												{
													//NG 烧录失败,VCOM还是%d
													sprintf(otp2_text, "NG 烧录后,读取到VCOM是%d", read_vcom);
													printf("check_burn_ok error: channel:%d, %s \n", m_channel_2, otp2_text);
												}
												
											}
										}
									}
									#endif
								
								}

								#endif

							}
							else
							{
								// don't burn.
								//boxRebackFlicker(0, 0, otp1_text, otp2_text);
								//printf("=== Do not burn, not power on. ===\n");
								//client_pg_shutON(1, box_cur_state.curModuleName, NULL);
							}

							// calc total time, and show result.
							unsigned short BurnTime = 0;
							if(auto_flick_return_code == FLICKER_OK || auto_flick_return_code_2 == FLICKER_OK)
							{
								gettimeofday(&tpend,NULL); 
								timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; 
								timeuse/=1000000; 
								printf("Used Time:%f\n", timeuse); 
								BurnTime = (unsigned short)(timeuse*100.0);
							}
							
							if(vcom_info.idBurner || vcom_info.vcomBurner || vcom_info.gammaBurner || vcom_info.codeBurner) //???
							{
								if(auto_flick_return_code == FLICKER_OK)
								{
									sprintf(otp1_text, "OK 时间=%.1f VCOM=%d FLICK=%.1f", ((float)BurnTime)/100, vcomVcomValue1, ((float)vcomFlickValue1)/100);
									printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
								}
								if(auto_flick_return_code_2 == FLICKER_OK)
								{
									sprintf(otp2_text, "OK 时间=%.1f VCOM=%d FLICK=%.1f", ((float)BurnTime)/100, vcomVcomValue2, ((float)vcomFlickValue2)/100);
									printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
								}
							}
							else
							{
								//just read.
								if(auto_flick_return_code == FLICKER_OK)
								{
									sprintf(otp1_text, "读取,烧录次数是%d,VCOM是%d", box_shut_info.vcom_otp, readVcom1);
									printf("mipi_channel: %d error: %s \n", m_channel_1, otp1_text);
								}
								if(auto_flick_return_code_2 == FLICKER_OK)
								{
									sprintf(otp2_text, "读取,烧录次数是%d,VCOM是%d", vcom_otp_2, readVcom2);
									printf("mipi_channel: %d error: %s \n", m_channel_2, otp2_text);
								}
							}

							// show message on the box.
							boxRebackFlicker(0, BurnTime, otp1_text, otp2_text);

							// show vcom, flick info on the lcd.
							//show OK/NG, VCOM, Flick by FPGA.
							if( vcom_info.idBurner || vcom_info.vcomBurner 
								|| vcom_info.gammaBurner || vcom_info.codeBurner)
							{
								int show_vcom1 = 0;
								int show_otp1 = 0;
								int show_ok1 = 0;
								int show_vcom2 = 0;
								int show_opt1 = 0;
								int show_ok2 = 0;

								printf("show lcd message!\n");

								#if 0
								auto_flick_return_code = FLICKER_OK;
								auto_flick_return_code_2 = FLICKER_OK;
								#endif
								
								// mipi channel 1.
								set_fpga_text_vcom(m_channel_1, vcomVcomValue1);
								set_fpga_text_flick(m_channel_1, vcomFlickValue1);
								
								if(auto_flick_return_code == FLICKER_OK)
								{
									// mipi channel 1 ok.
									set_fpga_text_color(m_channel_1, 0);
									show_ok1 = 1;
								}
								else
								{	
									// NG
									set_fpga_text_color(m_channel_1, 1);
									show_ok1 = 0;
								}

								// mipi channel 2.
								set_fpga_text_vcom(m_channel_2, vcomVcomValue2);
								set_fpga_text_flick(m_channel_2, vcomFlickValue2);
								if(auto_flick_return_code_2 == FLICKER_OK)
								{
									// mipi channel 2 ok.
									set_fpga_text_color(m_channel_2, 0);
									show_ok2 = 1; 
								}
								else
								{
									// mipi channel 2 ng.
									set_fpga_text_color(m_channel_2, 1);
									show_ok2 = 0;; 
								}

								set_fpga_text_show(1, 1, show_ok1, show_ok2);  //VCOM, OTP, OK, OK2

								if (show_ok1 && show_ok2)
								{
									// 2 channel VCOM OTP are ok, auto shutdown.
									usleep(2 * 1000 * 1000);
									client_pg_shutON(0, 0, NULL, 0);
								}
							}
							
						}
						else
						{							
							box_cur_state.pgPwrStatus = client_pg_pwr_status_get();
							
							box_shut_info_t box_shut_info;
							memset(&box_shut_info, 0, sizeof(box_shut_info));
							
							box_shut_info.pgPwrStatus = box_cur_state.pgPwrStatus;
							box_cur_state.curPtnId = box_shut_info.ptnIndex = 0;

							#if 1
							module_info_t *pModule_info = dbModuleGetPatternById(box_shut_info.ptnIndex);
							if(pModule_info)
							{
								box_shut_info.lockSec = pModule_info->ptddisptime;
								strncpy(box_shut_info.pictureName,pModule_info->ptdpatternname,31);
							}
							#endif
							
							boxCmdTransfer(POWER_KEY,&box_shut_info,sizeof(box_shut_info),0);

							client_pg_shutON(1, box_cur_state.curModuleName, NULL, 0);
							
							boxRebackFlicker(0, 0, otp1_text, otp2_text);
						}
						
						gBoxKeyReturn = 0;						
						printf("====== Box Power On End =======\n");

		            }
					#endif
            	}
				else
				{
					printf("====== Box Power on: Unknow Driver IC. =======\n");
				}
            }

        }
        break;

        case  OK_KEY:
        {
            if(!box_cur_state.isConnected)
                break;
			
            if(box_cur_state.boxLayer == MODULE_LAYER)
            {
				gBoxKeyReturn = 1; //xujie
               set_cur_module_name(box_cur_state.curModuleName);
               write_cur_module_name();
			   
               load_cur_module();
			   gBoxKeyReturn = 0; //xujie
			   //
               box_cur_state.boxLayer = PICTURE_LAYER;
               box_cur_state.curPtnId = 0;
			   //
               box_pwr_status_t box_pwr_status;
               box_pwr_status.pgPwrStatus = client_pg_pwr_status_get();
               boxCmdTransfer(OK_KEY,&box_pwr_status,sizeof(box_pwr_status),0);

               printf("!!!!!!!!!!!!!!ptn num %d\n",box_cur_state.curTotalPtnNums);
            }
        }
        break;

        case  HOME_KEY:
        {
			//printf("HOME_KEY\n");
			boxRebackFlicker(0, 0, NULL, NULL);
            if(!box_cur_state.isConnected)
            {
            	printf("home key: connected!\n");
                //1.connected
                box_cur_state.isConnected = 1;
                char modelName[256];
                int ret = read_cur_module_name(modelName);
                //2.send cur module file name to box by uart
                if(ret == 0)
                {
                    int moduelNums = dbInitModuleSelf();
                    box_cur_state.totalModuleNum = moduelNums;
                    strcpy(box_cur_state.curModuleName,modelName);
                    module_self_t *pModule_Self  = dbModuleSelfGetByName(modelName);
                    if(pModule_Self)
                    {
                        box_cur_state.curModuleIndex = pModule_Self->index;
                        int pgpwrStatus = client_pg_pwr_status_get();;

                        hand_info_t  hand_info;
                        memset(&hand_info,0,sizeof(hand_info_t));
                        strncpy(hand_info.moduleName,modelName,31);
                        strncpy(box_cur_state.curModuleName,modelName,31);
                        strncpy(hand_info.pwrName,pModule_Self->modulePowerName,31);
                        strncpy(box_cur_state.curModulePwrName,pModule_Self->modulePowerName,31);


                        hand_info.pgPwrStatus = pgpwrStatus;
                        hand_info.moduleTotalNum  = moduelNums;
                        hand_info.curModuleIndex  = pModule_Self->index;
                        hand_info.pictureTotalNum = pModule_Self->totalPattern;
                        if(pgpwrStatus == 0)
                        {
                            box_cur_state.boxLayer = MODULE_LAYER;
                            hand_info.curPictureIndex = 0;
                            strncpy(hand_info.pictureName,pModule_Self->patternFirstName,31);
                        }
                        else
                        {
                            box_cur_state.curPtnId = client_pg_ptnId_get();
                            box_cur_state.boxLayer = PICTURE_LAYER;
                            hand_info.curPictureIndex = box_cur_state.curPtnId;
                            module_info_t *pModule_info = dbModuleGetPatternById(box_cur_state.curPtnId);
                            if(pModule_info)
                            {
                                strncpy(hand_info.pictureName,pModule_info->ptdpatternname,31);
                            }
                        }

                        box_cur_state.curTotalPtnNums = pModule_Self->totalPattern;
                        boxCmdTransfer(HOME_KEY,&hand_info,sizeof(hand_info),0);
                        printf("model name %s picture total is %d\n",modelName,pModule_Self->totalPattern);
						boxRebackFlicker(0, 0, NULL, NULL); 
                    }
                }
                break;
            }
			
            if(PICTURE_LAYER == box_cur_state.boxLayer)
            {
            	printf(" home key： box layer!\n");
				
                box_cur_state.boxLayer = MODULE_LAYER;

                int ptnId = box_cur_state.curPtnId;
                module_info_t *pModule_info = dbModuleGetPatternById(ptnId);
                if(pModule_info)
                {
                    hand_info_t  hand_info;
                    memset(&hand_info,0,sizeof(hand_info_t));
                    strncpy(hand_info.moduleName,box_cur_state.curModuleName,31);
                    strncpy(hand_info.pwrName,box_cur_state.curModulePwrName,31);
                    strncpy(hand_info.pictureName,pModule_info->ptdpatternname,31);
                    hand_info.pgPwrStatus = client_pg_pwr_status_get();
                    hand_info.moduleTotalNum = box_cur_state.totalModuleNum;
                    hand_info.curModuleIndex = box_cur_state.curModuleIndex;
                    hand_info.pictureTotalNum = box_cur_state.curTotalPtnNums;
                    hand_info.curPictureIndex = 0;
                    printf("2model name %s picture total is %d xx:%s\n",box_cur_state.curModuleName,box_cur_state.curTotalPtnNums,hand_info.pwrName);
                    boxCmdTransfer(HOME_KEY,&hand_info,sizeof(hand_info),0);
					boxRebackFlicker(0, 0, NULL, NULL);
                }
            }
        }
        break;

        case  OPT_KEY: 
        {
        }
        break;

        case CHECK_PWR_KEY:
        {
        }
        break;

        default:
			printf("%d\n", cmd);
        break;
    }
}


static void boxSendCmd(int cmd)
{
    unsigned int msgFlag = 0xa0000000;
    switch(cmd)
    {
        case  UP_KEY:
		printf("--------------UP_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  DOWN_KEY:
		printf("--------------DOWN_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  POWER_KEY:
		printf("--------------POWER_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  OK_KEY:
		printf("--------------OK_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  HOME_KEY:
		printf("--------------HOME_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue, msgFlag|cmd, 0,0);
        }
        break;

        case  OPT_KEY:
		printf("--------------OPT_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case CHECK_PWR_KEY:
		//printf("--------------CHECK_PWR_KEY----------\n"); ?3?????????, ?????????
		if(box_cur_state.pgPwrStatus && gBoxKeyReturn == 0) //xujie add    if(box_cur_state.pgPwrStatus)
        {
            //printf("check pwr\n");
#if 0 //????
            pwrgetInfo(0); //??????
#else
			pwrgetInfo(2);
			//pwrgetInfo(3);
#endif
        }
        break;

        default:
		printf("--------------KEY[%d]----------\n", cmd);
		//printf("boxSendCmd = %d\n", cmd);
        break;
    }
}

static void boxRcvThread()
{
    struct timeval delaytime;
    BYTE buf[1024] = {0};
    INT readlen = 0;
    delaytime.tv_sec = 0;
    delaytime.tv_usec = 50000;
    BYTE cmd;
    SMsgHead *pSmsg;

    memset(&box_cur_state,0,sizeof(box_cur_state));

    while (1)
    {
        if (getBoxUartNo() >= 0)
        {
            readlen = uartReadData(getBoxUartNo(), delaytime, buf);
			#if 0
			if (readlen > 0)
			{
				printf("==== boxRcvThread: len = %d. \n", readlen);
				dump_data1(buf, readlen);
			}
			#endif
			
        }

        if(readlen<=0)
        {
			usleep(10 * 1000);
            continue;
        }
        else
        {
        	#if 0
            int i;
            printf("box recv buf data: \n");
            for (i = 0; i < readlen; i++)
            {
                printf("0x%02x ", buf[i]);
            }
            printf("\n");
			#endif
        }

        pthread_mutex_lock(&gBoxGuardLock);
        boxtime_alive = time(NULL);
        pthread_mutex_unlock(&gBoxGuardLock);

		if(buf[0] == 0x08)
		{
			boxSoftwareVersion_request = 0;
			//
			unsigned char* ver = &(buf[21]);
			printf("box.c  box version: %d.%d.%d\n", ver[0], ver[1], ver[2]);

			unsigned int ver1 = ver[0] & 0xff;
			unsigned int ver2 = ver[1] & 0xff;
			unsigned int ver3 = ver[2] & 0xff;
			unsigned int version = 0;
			version = (ver1<<16) | (ver2<<8) | (ver3);
			boxSoftwareVersion = version;
			client_sendBoxVersion(1, version);
			continue;
		}

        pSmsg = buf;
        cmd = pSmsg->cmd;

        boxSendCmd(cmd);
    }
}


/*typedef struct{
	//55 aa
	unsigned short pack_size; //???????????С 7e 7e XX ... XX XX 7e 7e
	//7e 7e
	unsigned short tag; //??????????? [72 0B]???????  [73 0B]????
	unsigned short data_len; //??????????????crc 7e 7e
	unsigned char* data;
	unsigned short crc32;
	//7e 7e
}box_upgrade_pack_t;*/
void box_upgrade(/*char* filename*/unsigned char* file_buffer, unsigned int file_size)
{
	//???????????
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
	if(1)
	{
		UartNo = getBoxUartNo();
		lock = &gBoxReadLock;
		type = 0x50/*0x0B*/;
	}
	else
	{
		extern pthread_mutex_t gPwrReadLock;
		UartNo = getPwrUartNo();
		type = 0x05;
		lock = &gPwrReadLock;
	}

	pthread_mutex_lock(lock/*&gBoxReadLock*/);
	
	//
	int ret;
	unsigned int num;
	unsigned char txt[1500];
	//???????(??λ???????λ???), 04 00 = 0x0004
	//pSocketCmdInfo->crc = crc32_le(CRCPOLY_LE,pSocketCmdInfo->pValid,pSocketCmdInfo->length);
	num = 0;
// 	txt[num++] = 0x55;
// 	txt[num++] = 0xaa;
	//
// 	txt[num++] = 0x10; //?????С
// 	txt[num++] = 0x0;
	//
// 	txt[num++] = 0x7e;
// 	txt[num++] = 0x7e;
	//
	txt[num++] = 0x72; //72???????   73?????
	txt[num++] = /*0x0B*/type; //05????? 0B?????
	//
	txt[num++] = 0x4; //????(??λ???????λ???)
	txt[num++] = 0x0;
	//
	txt[num++] = 0x0; //???????????????趨??????
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
	if(ret != 0) 
		printf("72 0B error\n");
	
	printf("box upgrade mode ...\n");
	sleep(4);

	//???????
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
// 		unsigned short packlen = /*7e 7e*/2  +  /*73 0B*/2  + /*???????????????[0c 04]*/2  + /*??????????е????λ??*/4  +  /*????????????鳤??*/4  +  /*??????????С*/4  +  /*?????????????*/datalen  +  /*CRC[b6 3c a6 af]*/4  +  /*7e 7e*/2;
// 		txt[num++] = (packlen) & 0xff; //????
// 		txt[num++] = (packlen>>8) & 0xff;
		//
// 		txt[num++] = 0x7e;
// 		txt[num++] = 0x7e;
		//????????
		txt[num++] = 0x73; //73=?????
		txt[num++] = /*0x0B*/type; //0B=?????
		//2
		unsigned short packdatalen = /*???????????????λ??*/4 + /*???????????鳤??*/4 + /*??????????С*/4 + /*???????????鳤??*/datalen;
		txt[num++] = (packdatalen) & 0xff; //?????????????????
		txt[num++] = (packdatalen>>8) & 0xff;
		//4
		txt[num++] = (offset) & 0xff; //????????????????????е????λ??
		txt[num++] = (offset>>8) & 0xff;
		txt[num++] = (offset>>16) & 0xff;
		txt[num++] = (offset>>24) & 0xff;
		//4
		txt[num++] = (datalen) & 0xff; //???????????鳤??
		txt[num++] = (datalen>>8) & 0xff;
		txt[num++] = (datalen>>16) & 0xff;
		txt[num++] = (datalen>>24) & 0xff;
		//4
		txt[num++] = (file_size) & 0xff; //??????????С
		txt[num++] = (file_size>>8) & 0xff;
		txt[num++] = (file_size>>16) & 0xff;
		txt[num++] = (file_size>>24) & 0xff;
		//datalen
		memcpy((&(txt[num])), buf, datalen);  //????????????
		num += datalen;
		//?????????
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
		
		if(ret != 0) 
			printf("73 0B error, offset=%d\n", offset);
		
		printf("box upgrade data: offset: %d, len: %d\n", offset, datalen);
		usleep(800*1000);

		//
		offset += datalen;
		buf += datalen;
		len -= datalen;
	}

	printf("box upgrade end. total size: %d.\n", offset);

	pthread_mutex_unlock(lock/*&gBoxReadLock*/);
	usleep(1000*500);

	boxSoftwareVersion = 0;
	boxSoftwareVersion_request = 1;
	sem_post(&boxSoftwareVersion_sem);
}

static MS_U32 box_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message)
{
	// box upgrade.
	if(Message.MessageID == 0x12345678)
	{
		unsigned char* file_buffer = (unsigned char*)Message.Parameter1;
		unsigned int file_size = (unsigned int)Message.Parameter2;
		gBoxUpdateState = 1; //???????
		gBoxKeyReturn = 1; //xujie
		usleep(10 * 1000);
		box_upgrade(file_buffer, file_size/*"/home/updata/box.bin"*/);
		gBoxKeyReturn = 0; //xujie
		gBoxUpdateState = 0; //?????????
		//free(file_buffer);

		//??????????, ????????汾
	/*	boxSoftwareVersion = 0;
		boxSoftwareVersion_request = 1;
		sem_post(&boxSoftwareVersion_sem);*/
		return 0;
	}

	// power upgrade.
	if(Message.MessageID == PWR_CMD_UPGRADE) //???????
	{
		printf("=== box power upgrade\n");
		unsigned char* file_buffer = (unsigned char*)Message.Parameter1;
		unsigned int file_size = (unsigned int)Message.Parameter2;
		gBoxUpdateState = 1;
		gBoxKeyReturn = 1;
		usleep(10 * 1000);
		userPwrUpgrade(file_buffer, file_size);//box_upgrade("/home/updata/box.bin");
		gBoxKeyReturn = 0;
		gBoxUpdateState = 0;
		//free(file_buffer);
		return 0;
	}
	
	if(Message.MessageID == BOX_CMD_GET_VER)
	{
		if(boxSoftwareVersion_request == 1)
		{
			boxCmdTransfer_version();
		}
		return 0;
	}
	

    unsigned char cmd  = Message.MessageID&0xffff;
    boxProcCmd(cmd);
	
    return 0;
}

unsigned int box_message_queue_get()
{
    return boxApartMent->MessageQueue;
}


INT boxCmdTransfer_version()
{
	//BYTE cmd ;
	void *pBuf = NULL;
	WORD16 nLen = 0;
	void *pNode = NULL;

	int ret  = 0;
	int sendLen = sizeof(SMsgHead)+nLen;
	char *pSendBuf = (char*)malloc(sendLen);
	if(nLen>0)
	{
		memcpy(&pSendBuf[sizeof(SMsgHead)],pBuf,nLen);
	}
	SMsgHead *pSmgHead = (SMsgHead *)pSendBuf;
	pSmgHead->type = 0x50;//BOX_TYPE;
	pSmgHead->cmd  = 0x08;//cmd;
	pSmgHead->len  = nLen;
	pthread_mutex_lock(&gBoxReadLock);

	ret = uartSendMsgToBoard(getBoxUartNo(), pSendBuf, sendLen);
	pthread_mutex_unlock(&gBoxReadLock);
	free(pSendBuf);
	return ret;
}

static void* boxSoftwareVersion_Task(void* arg)
{
	sleep(4);
	while(1)
	{
		sem_wait(&boxSoftwareVersion_sem);
		if(boxSoftwareVersion_request > 0)
		{
			while(boxSoftwareVersion_request > 0)
			{
				sleep(2);
				if(gBoxKeyReturn == 1) continue;
				MsOS_PostMessage(boxApartMent->MessageQueue, BOX_CMD_GET_VER, 0, 0);
			}
		}
	}
	return NULL;
}

INT initBox(void)
{
    MS_EVENT_HANDLER_LIST *pEventHandlerList = 0;

    traceMsg(FPGA_PRT_SW1, "func %s\n", __FUNCTION__);

    pthread_mutex_init(&gBoxReadLock, NULL);

    pthread_mutex_init(&gBoxGuardLock, NULL);

    boxApartMent = MsOS_CreateApartment("lcdTask", box_message_proc, pEventHandlerList);

    boxCmdTransfer(HAND_KEY,0,0,0);

    boxtime_alive = time(NULL);

    boxThreadCreate();


	sem_init(&boxSoftwareVersion_sem, 0, 0);
	pthread_create(&boxSoftwareVersion_pthread, NULL, boxSoftwareVersion_Task, NULL);
	if(boxSoftwareVersion_request == 1)
	{
		sem_post(&boxSoftwareVersion_sem);
	}
	
    return SUCCESS;
}

#endif
