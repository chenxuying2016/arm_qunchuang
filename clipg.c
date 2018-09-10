#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "client.h"
#include "util/debug.h"
#include "util/util.h"
#include "rwini.h"
#include "pgDB/pgDB.h"
#include "pgDB/pgLocalDB.h"
#include "pubFpga.h"
#include "fpgaFunc.h"
#include "comStruct.h"
#include "pubtiming.h"
#include "fpgaRegister.h"
#include "recvcmd.h"
#include "pubmipi.h"


static int bSetTime = 0;

static s1103PwrCfgDb s_power_info = { 0 };
static timing_info_t s_timing_info = { 0 };
static vcom_info_t s_vcom_info = { 0 };
static int s_vcom_ptn_index = -1;
static gamma_cfg_t s_gamma_cfg = { 0 };

int get_module_res(int* p_width, int* p_height)
{
	if ( (p_width == NULL) || (p_height == NULL) )
	{
		printf("get_module_res: Invalid param!\n");
		return -1;
	}

	*p_width = s_timing_info.timhactivetime;
	*p_height = s_timing_info.timvactivetime;
	printf("get_module_res: %d x %d.\n", *p_width, *p_height);
	
	return 0;
}


void client_pg_setTime(int stampTime)
{
    DBG("set time %d",stampTime);
#if 1
    if(!stampTime)
        return;
    if(bSetTime == 0)
    {
        struct timeval tv_set;

        tv_set.tv_sec  = stampTime;
        tv_set.tv_usec = 0;
        if (settimeofday(&tv_set,NULL)<0) {
            perror("settimeofday");
        }
        bSetTime = 1;
    }
#endif
    //client_rebackHeart();
}

void client_pg_firmware(void *param)
{
    recv_file_info_set_t *recv_file_info_set = param;
    char dst_file[256];
    recv_file_info_t *recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
    char *pstrNewFileName = recv_file_info->rfiName;
    sprintf(dst_file,"/home/updata/%s",pstrNewFileName);
    FILE  *local_file = fopen(dst_file, "wb+");
    fwrite(recv_file_info->pFileData, recv_file_info->rfiSize,1,local_file);
    fclose(local_file);
}

void client_pg_syncFile(void *param) //param = recv_file_info_set_t
{
    recv_file_info_set_t *recv_file_info_set = param;
    char dst_file[256] = "";
    char dst_path[256] = "";	
    recv_file_info_t *recv_file_info = NULL;
    char *pstrNewFileName = NULL;

	recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
	if (recv_file_info == NULL)
	{
		printf("client_pg_syncFile error: recv_file_info = NULL.\n");
		
		if(recv_file_info_set->param)
        {
            free(recv_file_info_set->param);
			recv_file_info_set->param = NULL;
        }

		recv_file_list_del(recv_file_info_set);
        free(recv_file_info_set);
		recv_file_info_set = NULL;
        
        //client_syncFinish();
	}
	
	pstrNewFileName = recv_file_info->rfiName;
    sprintf(dst_file, "./%s", pstrNewFileName);
	
    int  pathBytes = strrchr(dst_file,'/') - dst_file;
    memset(dst_path,0,sizeof(dst_path));
    memcpy(dst_path,dst_file,pathBytes);
	
    int result = mkdir_recursive(dst_path);
    int  local_file = 0;
    local_file = open(dst_file, O_CREAT|O_TRUNC|O_WRONLY);
    write(local_file, recv_file_info->pFileData, recv_file_info->rfiSize);
    close(local_file);
	
    system("sync");
    if(strstr(dst_file, "imageOrigin"))  //save to bmp to ptn,with same name
    {
        saveBmpToPtn(dst_file);
    }
    //DBG("%s  result %d %s",dst_path,result,dst_file);

    local_file_info_t *pLocalFile = 0;
    if(pLocalFile = localDBGetRec(recv_file_info->rfiName))
    {
        pLocalFile->fileSize = recv_file_info->rfiSize;
        pLocalFile->fileTime = recv_file_info->rfiTime;
        localDBUpdate(recv_file_info->rfiName, recv_file_info->rfiTime, recv_file_info->rfiSize);
    }
    else
    {
        localDBAdd(recv_file_info->rfiName, recv_file_info->rfiTime, recv_file_info->rfiSize);
    }

    sync_info_t sync_info;
    sync_info.syncProcess = (recv_file_info_set->curNo + 1) * 100 / recv_file_info_set->totalFilesNums;
    sync_info.timeStamp = cur_time();
    //DBG("SYNC %d timeStamp %d\n",sync_info.syncProcess,sync_info.timeStamp);
    write_sync_config(&sync_info); //写./sync.ini

    client_syncStatu();

    recv_file_info_set->fn_add_file_index(recv_file_info_set);

    if(client_getFileFromSet(recv_file_info_set) != 0)
    {
    	printf("client_pg_syncFile: sync end!\n");
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
}

void client_pg_downFile(void *param) //param = recv_file_info_set_t
{
    DBG("down file success!");
    recv_file_info_set_t *recv_file_info_set = param;
    recv_file_info_t *recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
    dwn_file_info_t  *dwn_file_info  = (dwn_file_info_t*)recv_file_info_set->param;
    if(dwn_file_info)
    {
        char dst_file[256];
        int  local_file;
        strcpy(dst_file,dwn_file_info->dstFileName);
        local_file = open(dst_file, O_CREAT|O_TRUNC|O_WRONLY);
        write(local_file, recv_file_info->pFileData, recv_file_info->actRecvSize);
        close(local_file);
    }
}

// GPIO 70:	GPIO Channel 1 Enable Switch. 
// GPIO 58: GPIO Channel 2 Enable Switch. 
// GPIO 74: 2828 reset.
// GPIO 10: Module Channel 1 Reset.
// GPIO 11: Module Channel 2 Reset.
// GPIO 7:  2828 power.
#define NEW_2828_RESET		(1)
void dp_gpio_config()
{
	gpio_set_output_value(7, 1); 	// 2828 power

	gpio_set_output_value(10, 0);
	gpio_set_output_value(11, 0);
	usleep(10 * 1000);
	gpio_set_output_value(10, 1);
	gpio_set_output_value(11, 1);

	fpga_write(0xa9, 0x10); 
	usleep(10 * 100);
	fpga_write(0xa9, 0x12); 
	usleep(10 * 100);
	fpga_write(0xa9, 0x10); 
	
	// gpio channel 1 enable.
	gpio_set_output_value(70, 0); 	//之前是1, zdx
	gpio_set_output_value(58, 0); //zdx
	
	fpga_write(0x56, 0); // mipi数据时钟输出
}

void dp_gpio_deconfig()
{
	// disable cabc pwm capture
	fpga_write(0xa9, 0x00);

	#if NEW_2828_RESET

	#if 1
	gpio_set_output_value(74, 0);
	gpio_set_output_value(75, 0);
	usleep(10 * 1000);
	gpio_set_output_value(74, 1);
	gpio_set_output_value(75, 1);
	#endif
	
	gpio_set_output_value(10, 0);
	gpio_set_output_value(11, 0);
	usleep(10 * 1000);
	gpio_set_output_value(10, 1);
	gpio_set_output_value(11, 1);
	#else	
	gpio_set_output_value(74, 0);
	#endif
	
	gpio_set_output_value(7, 0);
	
	fpga_write(0x56, 1); // LVDS数据时钟输出
	
	gpio_set_output_value(70, 1); //之前是1, zdx
	gpio_set_output_value(58, 1); //zdx
}

// gpio 9:
// default: 1 => 1.8V
//  	  : 0 => 3.3v.

void volt_v33_v18_set_to_v33()
{
	printf("***     volt_v33_v18_set_to_v33: ==> V3.3.\n");
	gpio_set_output_value(9, 0);
}

void volt_v33_v18_set_to_v18()
{
	printf("***     volt_v33_v18_set_to_v33: ==> V1.8.\n");
	gpio_set_output_value(9, 1);
}

void volt_v33_v18_reset()
{
	volt_v33_v18_set_to_v18();
}




void dp_gpio_set()
{
    int i;
    //HI_UNF_GPIO_Init(); xujie

    for(i=54;i<73;i++)
    {
        HI_UNF_GPIO_Open(i);
        HI_UNF_GPIO_SetDirBit(i,0); //output
        HI_UNF_GPIO_WriteBit(i,1);
    }
    HI_UNF_GPIO_WriteBit(60,0);
	
    HI_UNF_GPIO_Open(11);
    HI_UNF_GPIO_SetDirBit(11,0); //output
    HI_UNF_GPIO_WriteBit(11,1);
}

int load_current_vcom_info(vcom_info_t* p_vcom_info)
{
	if (p_vcom_info == NULL)
		return -1;
	
	module_info_t *p_vcom_Module_info =  db_get_vcom_module_info();
	if (p_vcom_Module_info)
	{
		printf("VCOM cfg: %s. ptdindex = %d.\n", p_vcom_Module_info->vcomfile, p_vcom_Module_info->ptdindex);
		read_vcom_config(p_vcom_info, p_vcom_Module_info->vcomfile);
		printf("init vcom: %d, min vcom: %d, max vcom: %d, min_valid_flick: %f,"
				"max_valid_flick: %f, ok_flick: %f.\n", 
				p_vcom_Module_info->vcomfile, p_vcom_info->initVcom, p_vcom_info->minvalue, 
				p_vcom_info->maxvalue, p_vcom_info->f_min_valid_flick, p_vcom_info->f_max_valid_flick,
				p_vcom_info->f_ok_flick);

		s_vcom_ptn_index = p_vcom_Module_info->ptdindex;
	}
	else
	{
		printf("*** find vcom pattern failed! ***\n");
		s_vcom_ptn_index = -1;
	}
	
	return 0;
}

int get_current_vcom_ptn_index()
{
	return s_vcom_ptn_index;
}

vcom_info_t* get_current_vcom_info()
{
	return &s_vcom_info;
}

int load_current_gamma_info(gamma_cfg_t* p_gamma_cfg)
{
	if (p_gamma_cfg == NULL)
	{
		printf("load_current_gamma_info: Null gamma config.\n");
		return -1;
	}
	
	module_info_t *p_gamma_Module_info =  db_get_gamma_module_info();
	if (p_gamma_Module_info)
	{
		printf("Gamma cfg: %s.\n", p_gamma_Module_info->signaltest);		
		load_gamma_config(p_gamma_cfg, p_gamma_Module_info->signaltest);
	}
	else
	{
		printf("*** find gamma pattern failed!  %s. ***\n", p_gamma_cfg);
	}
	
	return 0;
}

gamma_cfg_t* get_current_gamma_cfg()
{
	return &s_gamma_cfg;
}

int load_cur_module()
{
    int isDir = 0;
	int ret = 0;
    unsigned int u32QueueId = 0;
    char modelName[256] = "";
    char reallyModelName[256] = "";
    char pwrCfgName[256] = "";
    char timCfgName[256] = "";
	char chip_name[64] = "";

	printf("\n\nload_cur_module.\n");

    ret = read_cur_module_name(modelName);
    if(ret!=0)
    {
    	printf("load_cur_module: read_cur_module_name failed!\n");
        return ret;
    }

    sprintf(reallyModelName, "%s/%s", TST_PATH, modelName);
    DBG("PG sync with model %s %s", modelName, reallyModelName);
    if(file_exists(reallyModelName, &isDir))
    {
        if (dbModuleInit(reallyModelName) != 0)
		{
			printf("load_cur_module: dbModuleInit failed!\n");
			return -1;
		}

		// select chip module.
		if (db_module_get_chip_name(chip_name) != 0)
		{
			printf("load_cur_module error: db_module_get_chip_name failed!\n");
			strcpy(chip_name, "Unknow");
		}

		{	
			chip_set_helitai_8019_status(0);
			ic_mgr_set_my_chip_id(chip_name);	
		
			printf("======  chiptype: %s, g_chip_helitai_8019 = %d.\n", chip_name, chip_get_helitai_8019_status());
		}
		
        if (db_module_get_power_cfg_file_name(pwrCfgName)!= 0)
		{
			printf("load_cur_module: dbModuleGetPwr failed!\n");
			return -1;
		}
		
        if (db_module_get_timing_cfg_file_name(timCfgName) != 0)
        {
			printf("load_cur_module: dbModuleGetTim failed!\n");
			return -1;
		}
		
        if (read_pwr_config(pwrCfgName, &s_power_info) != 0)
        {
			printf("load_cur_module: read_pwr_config failed!\n");
			return -1;
		}
		
        if (read_timging_config(timCfgName, &s_timing_info) != 0)
        {
			printf("load_cur_module: read_timging_config failed!\n");
			return -1;
		}

		printf("pwrSetInfo:\n");
        pwrSetInfo(&s_power_info);
		printf("=== led channel: %x. ===\n", s_power_info.LEDChannel);

        printf("!!!!!!!!!!edp is %d\n", s_timing_info.timsignaltype);

        fpgaSeqStr  fpgaSeq;
        int hor_T3;

        /*if(timing_info.timsignaltype == 2 || timing_info.timsignaltype == 1) //mipi
        {
            timing_info.timsignalmode = 1;
        }*/
#if 0
        fpgaSeq.T0 = timing_info.timhbackporch/timing_info.timsignalmode;
        fpgaSeq.T1 = (timing_info.timhbackporch+timing_info.timhsyncpulsewidth)/timing_info.timsignalmode;
        fpgaSeq.T2 = (timing_info.timhbackporch+timing_info.timhsyncpulsewidth+\
                      timing_info.timhfrontporch)/timing_info.timsignalmode;
        fpgaSeq.T3 = (timing_info.timhbackporch+timing_info.timhsyncpulsewidth+\
                      timing_info.timhfrontporch+timing_info.timhactivetime)/timing_info.timsignalmode;

        hor_T3 = timing_info.timhbackporch+timing_info.timhsyncpulsewidth+\
                timing_info.timhfrontporch+timing_info.timhactivetime;

        fpgaSeq.T4 = timing_info.timvbackporch;
        fpgaSeq.T5 = timing_info.timvbackporch+timing_info.timvsyncpulsewidth;
        fpgaSeq.T6 = timing_info.timvbackporch+timing_info.timvsyncpulsewidth+\
                       timing_info.timvfrontporch;
        fpgaSeq.T7 = timing_info.timvbackporch+timing_info.timvsyncpulsewidth+\
                       timing_info.timvfrontporch+timing_info.timvactivetime;
#else
        fpgaSeq.T0 = (s_timing_info.timhbackporch+s_timing_info.timhsyncpulsewidth+\
                      s_timing_info.timhactivetime)/s_timing_info.timsignalmode;
        fpgaSeq.T1 = s_timing_info.timhsyncpulsewidth/s_timing_info.timsignalmode;
        fpgaSeq.T2 = (s_timing_info.timhsyncpulsewidth+s_timing_info.timhbackporch)/s_timing_info.timsignalmode;
        fpgaSeq.T3 = (s_timing_info.timhbackporch+s_timing_info.timhsyncpulsewidth+\
                      s_timing_info.timhfrontporch+s_timing_info.timhactivetime)/s_timing_info.timsignalmode;
        fpgaSeq.T4 = s_timing_info.timvbackporch+s_timing_info.timvsyncpulsewidth+\
                        s_timing_info.timvactivetime;
        fpgaSeq.T5 = s_timing_info.timvsyncpulsewidth;
        fpgaSeq.T6 = s_timing_info.timvbackporch+s_timing_info.timvsyncpulsewidth;
        fpgaSeq.T7 = s_timing_info.timvbackporch+s_timing_info.timvsyncpulsewidth+\
                       s_timing_info.timvfrontporch+s_timing_info.timvactivetime;
#endif
        int clock = fpgaSeq.T3*fpgaSeq.T7*s_timing_info.timclockfreq/1000;

        //5338
        clock_5338_setFreq(clock);

        fpga_reg_dev_ioctl(eRESET_FPGA, 0);

        int dpClock = s_timing_info.timsignalmode*clock;

        FPGA_CLOCK_REGA4 = (dpClock&0xff0000)>>16;
        FPGA_CLOCK_REGA5 = (dpClock&0x00ff00)>>8;
        FPGA_CLOCK_REGA6 =  dpClock&0xff;

        printf("clock is %d %x %x %x\n", clock, FPGA_CLOCK_REGA4, FPGA_CLOCK_REGA5, FPGA_CLOCK_REGA6);

        u32QueueId = fpga_message_queue_get();
        MsOS_SendMessage(u32QueueId, FPGA_CMD_SET_SEQ, &fpgaSeq, 0);

        // FPGA REG1 寄存器说明
        // bit0~bit1:图形量化位宽(01:6bit；10:8bit；11:10bit)。只写不能读。
        // bit2~bit3:link数选择(00:单link；01:双link；11:4link)
        // bit4 DE信号电平
        // bit5 行同步信号电平
        // bit6 场同步电平
        // bit7 0:vasa 1:jeida
        // bit8~bit15 控制输出信号连接模式，每个link均可以配置为0、1、2、3中的一种;
        // 其中link0由bit9~bit8控制，link1由bit11~bit10控制，link2由bit13~bit12控制，link3由bit15~bit14控制；
        printf("set module bit %d\n",s_timing_info.timbit);
        MsOS_PostMessage(u32QueueId, FPGA_CMD_SET_BIT, s_timing_info.timbit,0);
		
        printf("set module link num\n");
        MsOS_PostMessage(u32QueueId, FPGA_CMD_SET_LINK, s_timing_info.timsignalmode,0);
        printf("set singal polar sync_pri_h:%d sync_pri_v:%d sync_pri_de:%d\n",\
               s_timing_info.timhsyncpri,s_timing_info.timvsyncpri,s_timing_info.timde);
		
        syncSingalLevelStr *pSyncSingalLevel = malloc(sizeof(syncSingalLevelStr));
        pSyncSingalLevel->de = s_timing_info.timde;
        pSyncSingalLevel->sync_pri_h = s_timing_info.timhsyncpri;
        pSyncSingalLevel->sync_pri_v = s_timing_info.timvsyncpri;
        MsOS_PostMessage(u32QueueId, FPGA_CMD_SYN_SIGNAL_LEVEL, pSyncSingalLevel,0);
		
        printf("set jeda vesa info %d\n",s_timing_info.timvesajeda);
        vesaJedaSwitchInfo_t *pVesaJedaSwitchInfo = malloc(sizeof(vesaJedaSwitchInfo_t));
        pVesaJedaSwitchInfo->lvdsChange = s_timing_info.timvesajeda;
        MsOS_PostMessage(u32QueueId,FPGA_CMD_SET_VESA_JEDA,pVesaJedaSwitchInfo,0);
		
        printf("set link num %d\n",s_timing_info.timsignalmode);
        lvdsSignalModuleStr *plvdsSignalModule = malloc(sizeof(lvdsSignalModuleStr));
        plvdsSignalModule->linkCount = s_timing_info.timsignalmode;
        plvdsSignalModule->module[0] = s_timing_info.timlink[0];
        plvdsSignalModule->module[1] = s_timing_info.timlink[1];
        plvdsSignalModule->module[2] = s_timing_info.timlink[2];
        plvdsSignalModule->module[3] = s_timing_info.timlink[3];
        printf("module link %d %d %d %d\n",plvdsSignalModule->module[0],
							                plvdsSignalModule->module[1],
							                plvdsSignalModule->module[2],
							                plvdsSignalModule->module[3]);
        MsOS_PostMessage(u32QueueId,FPGA_CMD_SET_LINK_MODE,plvdsSignalModule,0);

        usleep(5 * 1000);
        fpga_reg_dev_ioctl(eRESET_FPGA_TEST, 5000); //wait fpga ddr ready
        usleep(100 * 1000);

		#if 1
        move_ptn_destoryTask();
		
        move_ptn_createTask();
		#endif
		
		#if 0
		printf("\nLoad VCOM Config: ...\n");
		load_current_vcom_info(&s_vcom_info);

		printf("\nLoad Gamma Config ...\n");
		load_current_gamma_info(&s_gamma_cfg);
		#endif
		
		// update lcd msg pos.
		refresh_lcd_msg_pos();

		if(strlen(s_timing_info.timinitcodename) > 0)
		{
			vcom_info_t vcom_info = { 0 };
			z_parse_init_code(s_timing_info.timinitcodename);
		}
    }
    else
    {
        client_rebackError(SYNC_FILE_ERROR);
        DBG("file is not exist %s",reallyModelName);
        return -1;
    }
	
    return 0;
}

static int pg_pwr_status = 0;

int  client_pg_pwr_status_get()
{
    return pg_pwr_status;
}

void  client_pg_pwr_status_set(int enable)
{
    pg_pwr_status = enable;
}

void client_pg_shutON(int enable,char *pModelName, vcom_info_t* vcom_info, int ptn_id)
{
	printf("%s, %d\r\n", __FUNCTION__, __LINE__);

    char reallyModelName[256];
    char pwrCfgName[256];
    char timCfgName[256];

	struct timeval tv1 = { 0 };
	struct timeval tv2 = { 0 };
	
	gettimeofday(&tv1, NULL);
	
	printf("%s, %d\r\n", __FUNCTION__, __LINE__);

    if(enable == 1)
    {
    	printf("%s, %d\r\n", __FUNCTION__, __LINE__);
        int isDir;
        sprintf(reallyModelName,"%s/%s",TST_PATH,pModelName);
        DBG("PG shut on with model %s %s",pModelName,reallyModelName);
        if(file_exists(reallyModelName,&isDir))
        { 
            printf("!!!!!!!!!!edp is %d\n", s_timing_info.timsignaltype);
            if(s_timing_info.timsignaltype == 1) //edp
            {
            	printf("dp_gpio_set\n");
              dp_gpio_set();  //zhaobaoren
            }

			// gpio config
			dp_gpio_config();

			// set default io volt to 1.8 v.
			volt_v33_v18_reset();

			if (check_power_min_volt_is_v1_8(&s_power_info))
				volt_v33_v18_set_to_v18();
			else
				volt_v33_v18_set_to_v33();

			fpga_show_color_picture(0, 0, 0);			
			printf("pwrSetOnOff:\n");
            pwrSetOnOff(1,1);
			
			printf("client_rebackPower:\n");
            client_rebackPower(1); //回显电脑PG工具，电源已上电

			// shut VSP power.
			pwr_contorl(0, 1, 6);

			// clear fpga text.
			set_fpga_text_show(0,0,0,0);
			
            if(s_timing_info.timsignaltype == 2)
            {
				printf("mipi_working ...\n");
                mipi_working(&s_timing_info,0,0,vcom_info); //操作SSD2828在这里
				printf("mipi_working end.\n");
                client_rebackMipiCode(InitCode,sizeof(InitCode));
            }			
			
            pg_pwr_status = 1;
			printf("client_pg_showPtn: ptn_id = %d.\n", ptn_id);
            client_pg_showPtn(ptn_id); //默认显示第0张图片

            if(s_timing_info.timsignaltype == 1)  //edp
            {
                FPGA_CLOCK_REGA8 = 1;
                printf("!!!!!!!!!!!!!!!!!!reg8 is %d\n",FPGA_CLOCK_REGA8);
            }
            if(s_timing_info.timsignaltype == 2)
            {
                //set mipi;
                FPGA_CLOCK_REGA8 = 2;
				
            }
            printf("!!!!!!!!!!!!!!!!!!!!\n");
        }
        else
        {
            DBG("file is not exist %s",reallyModelName);
        }

		ca210_start_flick_mode(0);
    }
    else
    {
    	printf("%s, %d\r\n", __FUNCTION__, __LINE__);
		printf("mipi_close ...\n");
        mipi_close();
		//printf("----------------------------------3\n");
        FPGA_CLOCK_REGA8 = 0; //输出信号模式选择  3'b001:DP   3'b010:MIPI
        printf("!!!!!!!!!!!!!!!!!!reg8 is %d\n",FPGA_CLOCK_REGA8);
        pwrSetOnOff(0,1);
		//printf("----------------------------------4\n");
		dp_gpio_deconfig();
		//printf("----------------------------------5\n");
        client_rebackPower(0);
		//printf("----------------------------------6\n");
        pg_pwr_status = 0;
        DBG("PG shut down");
    }

    printf("%s, %d\r\n", __FUNCTION__, __LINE__);

	gettimeofday(&tv2, NULL);
	int diff = ((tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec - tv1.tv_usec) / 1000;
	printf("=== diff: %d.  tv1: %d.%d, tv2: %d.%d.  ===\n", diff, tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_sec);
}

void client_pg_mipiShutON(int enable,char *pMipiCode,int mipiLen)
{
    if(enable == 1)
    {
        if(isdbMoudlelInit())
        {
            //pwrSetInfo(&s_power_info);
            pwrSetOnOff(1,1);
            client_rebackPower(1);
            mipi_working(&s_timing_info,pMipiCode,mipiLen,NULL);
            client_rebackMipiCode(InitCode,sizeof(InitCode));
            DBG("PG mipi shut on len %d",mipiLen);
        }
    }
    else
    {
        mipi_close();
        pwrSetOnOff(0,1);
		
        client_rebackPower(0);
        DBG("PG mipi shut down");
    }
}

static int pg_cur_ptnId = 0;

int  client_pg_ptnId_get()
{
    return pg_cur_ptnId;
}

void  client_pg_ptnId_set(int ptnId)
{
    pg_cur_ptnId = ptnId;
}

void client_pg_showPtn(int ptnId)
{
    //DBG("PG show ptnid %d",ptnId);
    unsigned int u32QueueId = fpga_message_queue_get();

	printf("client_pg_showPtn: %d.\n", ptnId);
	MsOS_SendMessage(u32QueueId, FPGA_CMD_SET_PHOTO, ptnId,0);
	
    client_pg_ptnId_set(ptnId);
}

void client_pg_showCrossCur(pointer_xy_t *pPointer_xy,int num)
{
    int i;
    //DBG("CROSS num %d",num);
    for(i=0;i<num;i++)
    {
       //DBG("x[%d] y[%d]",pPointer_xy[i].x,pPointer_xy[i].y);
       crossCursorInfo_t  crossCursorInfo;
       crossCursorInfo.enable = 1;
       crossCursorInfo.x = pPointer_xy[i].x;
       crossCursorInfo.y = pPointer_xy[i].y;
       crossCursorInfo.wordColor = 0xFFFFFF;
       crossCursorInfo.crossCursorColor = 0;
       crossCursorInfo.RGBchang = 0;
       crossCursorInfo.HVflag = 0;
       crossCursorInfo.startCoordinate = 0;
       fpga_reg_dev_ioctl(eSET_CROSSCURSOR, (unsigned long)&crossCursorInfo);
    }
}

void client_pg_hideCrossCur()
{
    DBG("hide the cross cursor");
    crossCursorInfo_t  crossCursorInfo;
    crossCursorInfo.enable = 0;
    crossCursorInfo.x = 0;
    crossCursorInfo.y = 0;
    crossCursorInfo.wordColor = 0xFFFFFF;
    crossCursorInfo.crossCursorColor = 0;
    crossCursorInfo.RGBchang = 0;
    crossCursorInfo.HVflag = 0;
    crossCursorInfo.startCoordinate = 0;
    fpga_reg_dev_ioctl(eSET_CROSSCURSOR, (unsigned long)&crossCursorInfo);
}

int client_pg_readReg(int regAddr)
{
    DBG("regAddr 0x%x",regAddr);
    unsigned int u32QueueId = fpga_message_queue_get();
    return MsOS_SendMessage(u32QueueId,FPGA_CMD_READ_REG,regAddr,0);
}

void client_pg_writeReg(int regAddr,int regValue)
{
    //DBG("regAddr 0x%x regValue 0x%x",regAddr,regValue);
    unsigned int u32QueueId = fpga_message_queue_get();
    MsOS_PostMessage(u32QueueId,FPGA_CMD_SET_REG,regAddr,regValue);
}

void client_pg_upgradeFile(char *pSrcFile,char *pFileType,char *pDstFile)
{
}

#if 0
//读取initcode
#define INITCODE_PATH "cfg/initcode/"
#define MTPCODE_PATH  "cfg/mtpcode/"

void client_pg_read_init_code(char *pModelName, vcom_info_t* vcom_info)
{
	char reallyModelName[256];
	int isDir;
	sprintf(reallyModelName,"%s/%s",TST_PATH,pModelName);
	DBG("PG shut on with model %s %s",pModelName,reallyModelName);
	if(file_exists(reallyModelName,&isDir))
	{  

		if(strlen(s_timing_info.timinitcodename))
		{
			char initCodePath[128];
			sprintf(initCodePath,"%s/%s",INITCODE_PATH,s_timing_info.timinitcodename);
			memset(InitCode,0xff,sizeof(InitCode));
			mipi_parse_init_code(initCodePath,InitCode,vcom_info);
		}
		printf("!!!!!!!!!!!!!!!!!!!!\n");
	}
	else
	{
		DBG("file is not exist %s",reallyModelName);
	}
}
#endif

