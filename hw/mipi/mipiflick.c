#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hi_unf_spi.h"
#include "vcom.h"
#include "pgDB/pgDB.h"
#include "common.h"
#include "comUart.h"
#include "../box/box.h"

#include "ca310.h"

static pthread_mutex_t flickLock;

typedef struct tag_auto_flick_seg_s
{
    int  vcomIndex;
    /*unsigned*/ int  flickValue;
    char *autoFlickCode;
}auto_flick_seg_t;

typedef struct tag_auto_flick_s
{
    char pageCode[32];
    int  curFlickIndex;
    int  totalFlickNum;
    int  curCurve;
    auto_flick_seg_t *auto_flick_seg;
}auto_flick_t;

static auto_flick_t auto_flick;

typedef struct tag_vcom_value_info_s
{
    int minvcom_index;
    int maxvcom_index;
    int width;
    int flickJudgeValue;
    int isBurner;
    int isVcomCheck;

	int idBurner; 
	int vcomBurner;
	int gammaBurner; 
	int pwrOnDelay;

}vcom_value_info_t;

static vcom_value_info_t vcom_value_info;

void vcom_minmax_init(int min,int max,int flickJudgeValue,int isBurner,int isVcomCheck,int idBurner,int vcomBurner,int gammaBurner)
{
    vcom_value_info.minvcom_index = min;
    vcom_value_info.maxvcom_index = max;
    vcom_value_info.width = max-min+1;
    vcom_value_info.flickJudgeValue = flickJudgeValue;
    vcom_value_info.isBurner = isBurner;
    vcom_value_info.isVcomCheck = isVcomCheck;

	vcom_value_info.idBurner = idBurner;
	vcom_value_info.vcomBurner = vcomBurner;
	vcom_value_info.gammaBurner = gammaBurner;

}

static pthread_t flickthread = 0;
static int  stopFlickFlag = 0;

#ifdef JUST_USED_MIPI_CHANNEL_1
#else
//void autoFlickTask(void *param);
#endif

static int  curmode;

extern volatile int m_spi_auto_cs;
static auto_flick_t m_channel_auto_flick[2];
static pthread_t m_channel_flick_thread[2];
sem_t m_channel_flick_thread_start_sem[2];
sem_t m_channel_flick_thread_sem[2]; 
int m_channel_flick_thread_init = 0;
void channelFlickTask(void *param);
void box_channelFlickTask(void *param);

void flick_auto(int enable,int mode) //0:network;1:uart
{
	
#ifdef JUST_USED_MIPI_CHANNEL_1
#else
#if 0
    curmode = mode;
    if(enable)
    {
        int ret = 0;
        stopFlickFlag = 0;
        pthread_create(&flickthread, NULL, (void *) autoFlickTask, &curmode);
        if (ret != 0)
        {
            printf("can't create thread autoFlickTask: %s.\r\n", strerror(ret));
            return ;
        }
    }
    else
    {
        stopFlickFlag = 1;
        if(flickthread!=0)
        {
            pthread_join(flickthread,NULL);
            flickthread = 0;
        }
    }
#endif
	
#endif

}

#if 1

#ifdef JUST_USED_MIPI_CHANNEL_1
int m_channel_1 = 1;
#else
int m_channel_1 = 1;
int m_channel_2 = 4;
//int m_channel_3 = 2; 
//int m_channel_4 = 3;
#endif

int channel_list[4] = {0, 0, 0, 0};
int lcd_channel_list[4] = {0, 0, 0, 0};
int vcom_list[4] = {0, 0, 0, 0};
int flick_list[4] = {0, 0, 0, 0};

int auto_flick_return_code = 0;
int auto_flick_return_code_2 = 0;
int auto_flick_return_code_3 = 0;
int auto_flick_return_code_4 = 0;

int auto_flick_thread_init = 0;
pthread_t auto_flick_thread[2];
sem_t auto_flick_thread_start_sem[2];
sem_t auto_flick_thread_sem[2];

int auto_flick_initVcom = 0;
int auto_flick_maxVcom = 0;
int auto_flick_readDelay = 0;

#ifdef JUST_USED_MIPI_CHANNEL_1
#else

void autoFlickTask(void *param)
{
	#if 0
	printf("====  autoFlickTask run ...\n");

	// Load Module Data.
	int curPtnId = client_pg_ptnId_get();
	if(curPtnId == -1) 
		return;
	
	module_info_t *pModule_info = dbModuleGetPatternById(curPtnId);
	if(!pModule_info) 
		return;
	
	vcom_info_t vcom_info;

	#if 1
	vcom_info_t* p_vcom_info = get_current_vcom_info();
	if (p_vcom_info)
		vcom_info = *p_vcom_info;
	#else
	read_vcom_config(&vcom_info,pModule_info->vcomfile);
	#endif
	
	printf("!!!!!!!!!!!!vcom::%d %d %f-%d %d\n",vcom_info.minvalue,vcom_info.maxvalue,
			vcom_info.f_max_valid_flick, vcom_info.isBurner,vcom_info.isVcomCheck);

	// set vcom init data.
	auto_flick_initVcom = vcom_info.initVcom;
	auto_flick_readDelay = vcom_info.readDelay;
	auto_flick_maxVcom = vcom_info.maxvalue;

	// Only enable channel 1 now.
	// If enable channel 1 and channel 4, just set auto_flick_channel_nums to 2;
	int auto_flick_channel_nums = 1;

	if(auto_flick_thread_init == 0)
	{
		int i;
		for(i=0; i < auto_flick_channel_nums; i++)
		{
			sem_init(&auto_flick_thread_start_sem[i], 0, 0);
			sem_init(&auto_flick_thread_sem[i], 0, 0);
			pthread_create(&auto_flick_thread[i], NULL, (void *) channelFlickTask, (void*)i);
		}
		auto_flick_thread_init = 1;
	}

	//
	int need_check = 1;
	int need_check_2 = 0;

	if (auto_flick_channel_nums == 2)
		need_check_2 = 1;

	// channel 1 check module.	
	if(need_check)
	{
		#ifdef USED_IC_MANAGER
		int check_error = ic_mgr_check_chip_ok(m_channel_1);
		printf("ic_mgr_check_chip_ok: channel %d, error: %d.\n", m_channel_1, check_error);
		
		if (check_error)
			need_check = 0;
		#endif
	}

	// channel 2 check module.
	if(need_check_2)
	{
		#ifdef USED_IC_MANAGER
		int check_error = ic_mgr_check_chip_ok(m_channel_2);
		printf("ic_mgr_check_chip_ok: channel %d, error: %d.\n", m_channel_2, check_error);
		if (check_error)
			need_check_2 = 0;
		#endif
	}

	// channel 1 check CA310.
	if(need_check)
	{
		if(lcd_getStatus(1) == 0)
		{		
			printf("lcd_getStatus: channel %d, ca310 error!\n", m_channel_1);
			need_check = 0;
		}
	}

	// channel 2 check CA310.
	if(need_check_2)
	{
		if(lcd_getStatus(2) == 0)
		{
			printf("lcd_getStatus: channel %d, ca310 error!\n", m_channel_2);
			need_check_2 = 0;
		}
	}

	//
	extern volatile int m_spi_auto_cs;
	m_spi_auto_cs = 0;

#if 1
	// start auto flick ...
	if(need_check)
	{
		printf("start channel 1 flick ...!\n");
		sem_post(&auto_flick_thread_start_sem[0]);
	}
	if(need_check_2)
	{	
		printf("start channel 2 flick ...!\n");
		sem_post(&auto_flick_thread_start_sem[1]);
	}

	// auto flick end.
	if(need_check)
	{
		sem_wait(&auto_flick_thread_sem[0]);
	}
	if(need_check_2)
	{
		sem_wait(&auto_flick_thread_sem[1]);
	}
#endif

	//
	m_spi_auto_cs = 1; 

	// 
	int vcom = 0;
	int flick = 0;
	mipi_channel_get_vcom_and_flick(m_channel_1, &vcom, &flick);
	client_rebackFlickOver(m_channel_1, vcom, flick);
			
	printf("autoFlickTask over\n");
	#endif
}
#endif

#if 1
int z_flick_test(int mipi_channel, int porbe_channel, int read_delay, int vcom, int* p_flick_value)
{
	int retval = 0;

	if (p_flick_value == NULL)
	{
		printf("z_flick_test error: Invalid param!\n");
		return PROBE_STATU_OTHER_ERROR;
	}
	
	HI_UNF_SPI_Lock(mipi_channel);

	#ifdef USED_IC_MANAGER
	ic_mgr_write_vcom(mipi_channel, vcom);
	#else
	printf("*** NULL Flick process ***!\n");
	#endif
	
	HI_UNF_SPI_UnLock(mipi_channel);

	if(read_delay > 0) 
		usleep(1000 * read_delay); 

	#ifdef USE_Z_CA210_LIB
	float f_flick = 0.00;
	retval = ca210_capture_flick_data(porbe_channel, &f_flick);
	if (retval == PROBE_STATU_OK)
	{
		*p_flick_value = f_flick * 100;
		return PROBE_STATU_OK;
	}
	else if (retval == PROBE_STATU_OUT_RANGE)
	{
		printf("z_flick_test: flick value out range!\n");
		*p_flick_value = INVALID_FLICK_VALUE;
		return PROBE_STATU_OUT_RANGE;
	}
	else
	{
		printf("z_flick_test: flick value out range!\n");
		*p_flick_value = INVALID_FLICK_VALUE;
		return PROBE_STATU_OTHER_ERROR;
	}
	
	#else
	retval = lcd_autoFlickWork(porbe_channel, 0); 
	if(retval == -1) //timeout
	{
		printf("z_flick_test: timout\n");
		return -2;
	}

	if(retval < -1)
	{
		printf("z_flick_test: Invalid value!\n");
		return -1;
	}

	*p_flick_value = retval;
	
	return retval;
	#endif
}


#define Z_MIN_VCOM 		(0)
#define Z_MAX_VCOM 		(255)

#ifdef JUST_USED_MIPI_CHANNEL_1
#else

void flick_notify_pc(int mipi_channel, int vcom, int flick)
{
	#if 0
	if(mipi_channel == m_channel_1)
	{
		if (flick < 0)
			flick = INVALID_FLICK_VALUE * 100;
		client_rebackautoFlick(mipi_channel, vcom, flick);
	}
	#endif
}
#endif

#ifdef USED_FAST_FLICK
static int change_direct(int direction)
{
	if (direction == -1)
	{
		direction = 1;
	}
	else
	{
		direction = -1;
	}

	printf("--- change dirction: ==> %d.\n", direction);

	return direction;
}

static int s_max_error_cnt = 10;
static int s_max_change_direct_cnt = 5;
// find_direction: -1: left; 0: none; 1: right;
int z_auto_flick_2(int mipi_channel, int probe_channel, int read_delay, int min_vcom, int max_vcom, 
					int init_vcom, int *p_find_vcom, int* p_find_flick, int send_net_notify, int last_vcom,
					int last_flick, int find_direction, int min_ok_flick, int flick_error_cnt,
					int max_ok_flick, int left_find_min_flick, int right_find_min_vcom,
					int right_find_min_flick, int change_direction_cnt)
{
	int vcom = 0;
	int flick = 0;
	int step = 0;

	printf("====================================================================================\n");
	
	if (last_flick < 0)
	{
		// flick is too large to get right flick value.
		flick_error_cnt ++;
		step = 10;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom <= max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom >= min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		}
		
		
	}
	else if (last_flick >= (int)INVALID_FLICK_VALUE)
	{
		// flick is too large to get right flick value.
		flick_error_cnt ++;
		step = 10;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom <= max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom >= min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		}
		
		
	}
	else if (last_flick > 50 * 100)
	{
		step = 5;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom < max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 5: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end 5! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom > min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 5: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end 5! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++; 
			}
		
		}
	}
	else if (last_flick > 20 * 100)
	{
		step = 3;
		
		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom < max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 3: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end 3! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom > min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 3: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end 3! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		
		}
	}
	else
	{
		step = 1;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom < max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 1: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end 1! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom > min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 1: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end 1! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		
		}
	}

	#ifdef USE_Z_CA210_LIB
	int ret = PROBE_STATU_OK;
	ret = z_flick_test(mipi_channel, probe_channel, read_delay, vcom, &flick);
	if (ret == PROBE_STATU_OUT_RANGE)
	{
	
	}
	else if (ret == PROBE_STATU_OTHER_ERROR)
	{
		
	}
	
	#else
	z_flick_test(mipi_channel, probe_channel, read_delay, vcom, &flick);
	#endif
	
	printf("channel: %d, last vcom: %d, flick: %d. ==> now: VCOM: %d. flick: %d. \n", mipi_channel, last_vcom, last_flick, vcom, flick);

	
#ifdef JUST_USED_MIPI_CHANNEL_1
#else
	if (send_net_notify)
		flick_notify_pc(mipi_channel, vcom, flick);
#endif

	int show_vcom = 1;
	int show_ok = 0;
	int ok1 = 0;
	int ok2 = 0;
	show_lcd_msg(mipi_channel, show_vcom, show_ok, vcom, flick, ok1, ok2);

	if (flick > 0)
	{
		if (last_flick > 0)
		{
			if (last_flick < flick)
			{
				if (step == 1 && change_direction_cnt >= 1)
				{
					// find ok.
					printf("channel: %d, *** step 1: find min value: vcom = %d, flick = %d, min_ok = %d. ***\n",
							mipi_channel, last_vcom, last_flick, min_ok_flick);
					*p_find_vcom = last_vcom;
					*p_find_flick = last_flick;
					return 0;
				}
				
				if (find_direction == 0)
				{
					// detect direction
					find_direction = -1;
				}
				else
				{
					find_direction = -find_direction;
				}

				change_direction_cnt ++;
				// change direction.
				//if (find_direction == -1)
				//	find_direction = 1;
			}
		}

	}

	if (flick > 0 )
	{
		if (flick < min_ok_flick )
		{
			printf("channel: %d, *** find end: flick = %d, min_ok = %d. ***\n", mipi_channel, flick, min_ok_flick);
			*p_find_vcom = vcom;
			*p_find_flick = flick;
			return 0;
		}
		else if (flick < max_ok_flick && change_direction_cnt >= s_max_change_direct_cnt)
		{
			// find right flick value.
			if (last_flick < flick)
			{
				*p_find_vcom = last_vcom;
				*p_find_flick = last_flick;				
			}
			else
			{
				*p_find_vcom = vcom;
				*p_find_flick = flick;
			}
			
			printf("channel: %d, ====== find end. find min vcom: vcom = %d, flick = %d, min_ok = %d. ***\n",
					mipi_channel, *p_find_vcom, *p_find_flick, min_ok_flick);
			
			return 0;
		}
	}

	if (flick_error_cnt >= s_max_error_cnt && flick >= (int)INVALID_FLICK_VALUE)
	{
		printf("channel: %d, *** find error: flick = %d. try error cnt: %d. ***\n", mipi_channel, flick, flick_error_cnt);
		return 0;
	}

	if (change_direction_cnt >= s_max_change_direct_cnt)
	{
		printf("channel: %d, *** find error: change direction times = %d.\n", change_direction_cnt);
		return -1;
	}

	last_vcom = vcom;
	last_flick = flick;
	return z_auto_flick_2(mipi_channel, probe_channel, read_delay, min_vcom, max_vcom, 
					init_vcom, p_find_vcom, p_find_flick, send_net_notify, last_vcom,
					last_flick, find_direction, min_ok_flick, flick_error_cnt, max_ok_flick, 0, 0, 0,
					change_direction_cnt);
}
#endif

#if 0
int z_auto_flick(int mipi_channel, int probe_channel, int read_delay, int min_vcom, int max_vcom, 
					int init_vcom, int *p_find_vcom, int* p_find_flick, int send_net_notify)
{
	int vcom_l = 0;
	int vcom_r = 0;
	int vcom_0 = init_vcom;

	int flick_l = 0;
	int flick_r = 0;
	int flick_0 = 0;
	
	int left_end = 0;
	int right_end = 0;

	#if 1
	if (min_vcom >= max_vcom)
	{
		printf("z_auto_flick: Invalid param. min_vcom: %d >= max_vcom: %d.\n", min_vcom, max_vcom);
		return -1;
	}

	if (min_vcom + 2 > max_vcom)
	{
		printf("z_auto_flick: Invalid param. max_vcom: %d. - min_vcom: %d <2 \n", max_vcom, min_vcom);
		return -1;
	}

	if ( (min_vcom > init_vcom ) || (init_vcom > max_vcom) )
	{
		printf("z_auto_flick: Invalid param. min_vcom: %d, max_vcom: %d. init_vcom: %d.\n", min_vcom, max_vcom, init_vcom);
		return -1;
	}
	#endif
	
	printf("****** set vcom: %d. ******\n", init_vcom);

	// set init value.
	if (vcom_0 > min_vcom)
	{
		vcom_l = vcom_0 - 1;
	}
	else 
	{
		printf("init_vcom <= Z_MIN_VCOM\n");
		vcom_l = vcom_0;
		left_end = 1;
	}

	if (vcom_0 < max_vcom)
	{
		vcom_r = vcom_0 + 1;
	}
	else
	{
		printf("init_vcom >= Z_MAX_VCOM\n");
		vcom_r = vcom_0;
		right_end = 1;
	}

	if (vcom_l == min_vcom)
	{
		left_end = 1;
	}

	if (vcom_r == max_vcom)
	{
		right_end = 1;
	}

	// vcom_l flick
	
	flick_l = z_flick_test(mipi_channel, probe_channel, read_delay, vcom_l);
	printf("VCOM: %d. flick: %d. \n", vcom_l, flick_l);

	// vcom_0 flick
	flick_0 = z_flick_test(mipi_channel, probe_channel, read_delay, vcom_0);
	printf("VCOM: %d. flick: %d. \n", vcom_0, flick_0);

	// vcom_r flick
	flick_r = z_flick_test(mipi_channel, probe_channel, read_delay, vcom_r);
	printf("VCOM: %d. flick: %d. \n", vcom_r, flick_r);

	// check value.
	if (flick_l > flick_0 && flick_0 > flick_r)
	{
		// go to right

		// notify PC
		if (send_net_notify)
			flick_notify_pc(mipi_channel, vcom_r, flick_r);
		
		if (right_end)
		{
			// find it.
			printf("right end. find min vcom = %d.\n", flick_r);
			*p_find_vcom = vcom_r;
			*p_find_flick = flick_r;
			
			return 0;
		}
		
		vcom_0 ++;
		printf("go right: set init_vcom = %d.\n", vcom_0);
	}
	else if (flick_l < flick_0 && flick_0 < flick_r)
	{
		// go to left
		
		// notify PC
		if (send_net_notify)
			flick_notify_pc(mipi_channel, vcom_l, flick_l);

		if (left_end)
		{
			// find it.
			printf("left end. find min vcom = %d.\n", flick_l);
			*p_find_vcom = vcom_l;
			*p_find_flick = flick_l;
			return 0;
		}
		
		vcom_0 --;
		printf("go left: set init_vcom = %d.\n", vcom_0);
	}
	else if (flick_l > flick_r && flick_0 > flick_r)
	{

		printf(" >>>>>>>> \n");
		// goto right
		// notify PC
		if (send_net_notify)
			flick_notify_pc(mipi_channel, vcom_r, flick_r);
		
		if (right_end)
		{
			// find it.
			printf("right end. find min vcom = %d.\n", flick_r);
			*p_find_vcom = vcom_r;
			*p_find_flick = flick_r;
			
			return 0;
		}
		
		vcom_0 ++;
		printf("go right: set init_vcom = %d.\n", vcom_0);
	}
	else
	{
		// find it.
		printf("OK. find min vcom = %d.\n", vcom_0);
		*p_find_vcom = vcom_0;
		*p_find_flick = flick_0;

		
		// notify PC
		if (send_net_notify)
			flick_notify_pc(mipi_channel, vcom_0, flick_0);
		
		return 0;
	}

	// do next flick.
	return z_auto_flick(mipi_channel, probe_channel, read_delay, min_vcom, max_vcom, vcom_0, p_find_vcom, p_find_flick, send_net_notify);
}
#endif

#endif

#define MAX_MIPI_CHANNEL_NUMS	(4)
int s_find_vcom[MAX_MIPI_CHANNEL_NUMS] = { 0 };
int s_find_flick[MAX_MIPI_CHANNEL_NUMS] = { 0 };

int mipi_channel_get_vcom_and_flick(int mipi_channel, int *p_vcom, int *p_flick)
{
	if (p_vcom == NULL)
		return -1;

	if (p_flick == NULL)
		return -1;
	
	if (mipi_channel > 0 && mipi_channel <= MAX_MIPI_CHANNEL_NUMS)
	{
		*p_vcom = s_find_vcom[mipi_channel];
		*p_flick = s_find_flick[mipi_channel];
		return 0;
	}
	
	return -1;
}

int mipi_channel_set_vcom_and_flick(int mipi_channel, int vcom, int flick)
{
	if (mipi_channel > 0 && mipi_channel <= MAX_MIPI_CHANNEL_NUMS)
	{
		s_find_vcom[mipi_channel] = vcom;
		s_find_flick[mipi_channel] = flick;
		return 0;
	}
	
	return -1;
}

#ifdef JUST_USED_MIPI_CHANNEL_1
#else
void channelFlickTask(void *param)
{
	#if 0
	int index = (int)param;
	int channel;
	int lcd_channel;
	
	if(index == 0)
	{
		channel = m_channel_1;
		lcd_channel = 1;
		printf("channel 1 channelFlickTask run ...\n");
	}
	else
	{
		channel = m_channel_2;
		lcd_channel = 2;
		printf("channel 2 channelFlickTask run ...\n");
	}
	
	while(1)
	{
		sem_wait(&auto_flick_thread_start_sem[index]);

		//
		int vcom = auto_flick_initVcom;
		int last_flick = 0;
		int last_x = 0;
		int min_vcom = vcom;
		int min_vcom_flick = 0;
		int retVal = 0;
		int ok_vcom = 0;
		int ok_flick = 0;

		#ifdef USE_Z_CA210_LIB
		ca210_stop_flick_mode(lcd_channel);
		ca210_start_flick_mode(lcd_channel);
		#else
		lcd_autoFlickEnd(lcd_channel);
		lcd_autoFlickStart(lcd_channel);
		#endif
		
		#ifdef USED_NEW_FLICK_ALG
		int find_vcom = 0;
		int find_vcom2 = 0;
		int find_flick = 0;

		struct timeval tv1 = { 0 };
		struct timeval tv2 = { 0 };
		gettimeofday(&tv1, NULL);
		
		// read vcom config.
		vcom_info_t* p_vcom_info = get_current_vcom_info();
		printf("VCOM Info: \n");
		printf("min_vcom: %d.\n", p_vcom_info->minvalue);
		printf("max_vcom: %d.\n", p_vcom_info->maxvalue);
		printf("init_vcom: %d.\n", p_vcom_info->initVcom);
		printf("read delay: %d.\n", p_vcom_info->readDelay);

		#ifdef USED_FAST_FLICK
		{
			int min_ok_flick = p_vcom_info->f_ok_flick * 100;
			int max_ok_flick = p_vcom_info->f_max_valid_flick * 100;
			//int max_ok_flick = p_vcom_info->max_check_value * 100;

			int flick = 0;
			printf("min_ok_flick: %d, max_ok_flick: %d.\n", min_ok_flick, max_ok_flick);
			z_flick_test(channel, lcd_channel, p_vcom_info->pwrOnDelay, vcom, &flick);				

			#ifdef USE_Z_CA210_LIB				
			float f_flick = 0.00;
			ca210_capture_flick_data(lcd_channel, &f_flick);
			flick = f_flick * 100;
			#else
			flick = lcd_autoFlickWork(lcd_channel, 0);
			#endif

			printf("init vcom: %d, flick: %d.\n", vcom, flick);

			int flick_erro = z_auto_flick_2(channel, lcd_channel, p_vcom_info->readDelay, 
											p_vcom_info->minvalue, p_vcom_info->maxvalue, 
											p_vcom_info->initVcom, &find_vcom, &find_flick, 1,
											vcom, flick, 0, min_ok_flick, 0, max_ok_flick, 0, 0, 0, 0);

			printf("z_auto_flick_2: end. find: vcom = %d, flick = %d.\n", find_vcom, find_flick);
			mipi_channel_set_vcom_and_flick(channel, find_vcom, find_flick);

			if (flick_erro == 0)
			{
			#ifdef USED_IC_MANAGER
			printf("*** write ok vcom: %d, %x ***\n", find_vcom, find_vcom);
			ic_mgr_write_vcom(channel, find_vcom);
			#else
			printf("*** NULL Flick process ***!\n");
			#endif
			}
		}
		#else
		int flick_erro = z_auto_flick(channel, lcd_channel, p_vcom_info->readDelay, p_vcom_info->minvalue, 
											p_vcom_info->maxvalue, p_vcom_info->initVcom, &find_vcom, &find_flick, 1);
		if (flick_erro == 0)
		{
			printf("===========   auto flick end, find vcom: %d. try again: ============\n", find_vcom);			
			flick_erro = z_auto_flick(channel, lcd_channel, p_vcom_info->readDelay, p_vcom_info->minvalue, p_vcom_info->maxvalue, 
										find_vcom, &find_vcom2, &find_flick, 1);
			printf("== flick2 end. error: %d. vcom1: %d, vcom2: %d.\n", flick_erro, find_vcom, find_vcom2);

			if(channel == m_channel_1)
			{
				client_rebackautoFlick(channel, ok_vcom, ok_flick); 
			}
		}
		#endif
		
		gettimeofday(&tv2, NULL);
		int diff = ((tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec - tv1.tv_usec) / 1000;
		printf("=== Auto Flick time: %dms.  tv1: %d.%d, tv2: %d.%d.	===\n", diff, tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_sec);

		#ifdef USE_Z_CA210_LIB
		ca210_stop_flick_mode(lcd_channel);
		#else
		lcd_autoFlickEnd(lcd_channel);
		#endif
		
		#endif
		
		sem_post(&auto_flick_thread_sem[index]);
	}
	#endif
	
}
#endif

#endif


int box_autoFlick_initVcom = 0;
int box_autoFlick_maxVcom = 0;
int box_autoFlick_readDelay = 0;
int box_autoFlick_started[4] = {0, 0, 0, 0};

void box_autoFlick(vcom_info_t* vcom_info, int* vcomVcomValue1, int* vcomFlickValue1, int* vcomVcomValue2, int* vcomFlickValue2, char* otp1_text, char* otp2_text)
{
	
#ifdef JUST_USED_MIPI_CHANNEL_1
#else
	#if 0
	box_autoFlick_initVcom = vcom_info->initVcom;
	box_autoFlick_readDelay = vcom_info->readDelay;
	box_autoFlick_maxVcom = vcom_info->maxvalue;

	channel_list[0] = m_channel_1;
	channel_list[1] = m_channel_2;

	//
	lcd_channel_list[0] = 1;
	lcd_channel_list[1] = 2;

	if(m_channel_flick_thread_init == 0)
	{
		int i;
		for(i=0; i < 2; i++)
		{
			sem_init(&m_channel_flick_thread_start_sem[i], 0, /*u32InitCnt*/0);
			sem_init(&m_channel_flick_thread_sem[i], 0, /*u32InitCnt*/0);
			pthread_create(&m_channel_flick_thread[i], NULL, (void *) box_channelFlickTask, (void*)i);
		}

		m_channel_flick_thread_init = 1;
	}
	
	vcom_list[0] = 0;
	flick_list[0] = 0;
	vcom_list[1] = 0;
	flick_list[1] = 0;

	m_spi_auto_cs = 0;

	// start auto flick.
	int vcom_return_code = auto_flick_return_code;
	int vcom_return_code_2 = auto_flick_return_code_2;
	if(vcom_return_code == FLICKER_OK)
	{
		box_autoFlick_started[0] = 1;
		sem_post(&m_channel_flick_thread_start_sem[0]);
	}
	
	if(vcom_return_code_2 == FLICKER_OK)
	{
		box_autoFlick_started[1] = 1;
		sem_post(&m_channel_flick_thread_start_sem[1]);
	}

	// Wait auto flick end.
	if(vcom_return_code == FLICKER_OK)
	{
		sem_wait(&m_channel_flick_thread_sem[0]);
		box_autoFlick_started[0] = 0;
	}
	
	if(vcom_return_code_2 == FLICKER_OK)
	{
		sem_wait(&m_channel_flick_thread_sem[1]);
		box_autoFlick_started[1] = 0;
	}

	m_spi_auto_cs = 1;

	// check channel 1 flick value.
	if(auto_flick_return_code == FLICKER_OK)
	{
		int vcom = vcom_list[0];
		int flick = flick_list[0];
		printf("\nauto flick: channel=%d  vcom=%d  flick=%d\n", m_channel_1, vcom/*minIndex*/, flick/*minFlickValue*/);

		if(flick <= 0)
		{	
			// flick value error!
			// Can not find flick device!
			auto_flick_return_code = FLICKER_NG_2;
			sprintf(otp1_text, "NG FLIC�豸����,�븴λ");
			printf("line: %d. mipi_channel %d, %s\n", __LINE__, m_channel_1, otp1_text);
		}
		else
		{
			*vcomVcomValue1 = vcom;
			*vcomFlickValue1 = flick;
					
			if(vcom_info->f_max_valid_flick*100 > flick)
			{
				if(flick <= 70)
				{
					// NG, FLICK is xx, please reburn.
					auto_flick_return_code = FLICKER_NG_2;
					sprintf(otp1_text, "NG FLICK��%.1f�쳣,������!", ((float)flick)/100.0);
					printf("line: %d. %d, %s\n", __LINE__, m_channel_1, otp1_text);
				}
			}
			else //������¼
			{
				auto_flick_return_code = FLICKER_NG_2;
				sprintf(otp1_text, "NG FLICK��%.1f,������!", ((float)flick)/100.0);
				printf("line: %d. %d: check value: %f. %s\n", __LINE__, m_channel_1, 
						vcom_info->f_max_valid_flick, otp1_text);
			}
		}
	}

	// check channel 2 flick value.
	//��2·
	if(auto_flick_return_code_2 == FLICKER_OK)
	{
		int vcom = vcom_list[1];
		int flick = flick_list[1];
		printf("\nauto flick: channel=%d  vcom=%d  flick=%d\n", m_channel_2, vcom/*minIndex*/, flick/*minFlickValue*/);

		//���FLICKֵʧ��
		if(flick <= 0)
		{
			auto_flick_return_code_2 = FLICKER_NG_2;
			sprintf(otp2_text, "NG FLIC�豸����,�븴λ");
			//printf("%d--------------------------------- %s\n", m_channel_2, otp2_text);
			printf("line: %d. mipi_channel %d, %s\n", __LINE__, m_channel_2, otp2_text);
		}
		else
		{
			*vcomVcomValue2 = vcom;
			*vcomFlickValue2 = flick;
					
			if(vcom_info->f_max_valid_flick*100 > flick)
			{
				if(flick <= 70)
				{
					auto_flick_return_code_2 = FLICKER_NG_2;
					sprintf(otp2_text, "NG FLICK��%.1f�쳣,������!", ((float)flick)/100.0);
					printf("mipi channel %d: %s\n", m_channel_2, otp2_text);
					
				}
			}
			else //������¼
			{
				auto_flick_return_code_2 = FLICKER_NG_2;
				sprintf(otp2_text, "NG FLICK��%.1f,������!", ((float)flick)/100.0);
				printf("mipi channel %d: %s\n", m_channel_2, otp2_text);
			}
		}
	}
	#endif
	
	#endif
	
	printf("box_autoFlick over\n");
}

#define MIN_VCOM_FLICK_VALUE  200
void box_channelFlickTask(void *param)
{
	int index = (int)param;
	int channel = channel_list[index];
	int lcd_channel = lcd_channel_list[index];
	int enable_read_vcom = 0;
	int enable_left = 0;
	int enable_right = 0;

	
#ifdef JUST_USED_MIPI_CHANNEL_1
#else
	
	while(1)
	{
		sem_wait(&m_channel_flick_thread_start_sem[index]);
		if(box_autoFlick_started[index] == 0) 
			continue;
		printf("**********************************box_channelFlickTask***********[channel=%d][lcd=%d]\n", channel, lcd_channel);

		int retVal = 0;
		if (chip_is_helitai_8019() == 0)
		{
			#ifdef USED_NEW_FLICK_ALG

			#ifdef USE_Z_CA210_LIB
			ca210_stop_flick_mode(lcd_channel);
			#else
			lcd_autoFlickEnd(lcd_channel);			
			#endif
			
			usleep(1000*100); //2017-5-26

			// check init vcom
			#ifdef USE_Z_CA210_LIB
			float f_flick = 0.00;
			retVal = ca210_start_flick_mode(lcd_channel);
			#else
			retVal = lcd_autoFlickStart(lcd_channel);
			#endif
			
			#if 1
			int find_vcom = 0;
			int find_vcom2 = 0;
			int find_flick = 0;
			
			struct timeval tv1 = { 0 };
			struct timeval tv2 = { 0 };
			gettimeofday(&tv1, NULL);
			
			// read vcom config.
			vcom_info_t* p_vcom_info = get_current_vcom_info();
			printf("VCOM Info: \n");
			printf("min_vcom: %d.\n", p_vcom_info->minvalue);
			printf("max_vcom: %d.\n", p_vcom_info->maxvalue);
			printf("init_vcom: %d.\n", p_vcom_info->initVcom);
			printf("read delay: %d.\n", p_vcom_info->readDelay);

			#ifdef USED_FAST_FLICK
			{
				//int min_ok_flick = 200;
				//int max_ok_flick = 1300;
				int min_ok_flick = p_vcom_info->f_ok_flick * 100;
				int max_ok_flick = p_vcom_info->f_max_valid_flick * 100;
				
				int flick = 0;
				z_flick_test(channel, lcd_channel, p_vcom_info->readDelay, p_vcom_info->initVcom, &flick);				

				#ifdef USE_Z_CA210_LIB
				ca210_capture_flick_data(lcd_channel, &f_flick);
				flick = f_flick * 100;
				#else
				flick = lcd_autoFlickWork(lcd_channel, 0); 
				//retVal = lcd_autoFlickWork(lcd_channel, &Flicker);
				#endif
				
				printf("init vcom: %d, flick: %d.\n", p_vcom_info->initVcom, flick);
	
				int flick_erro = z_auto_flick_2(channel, lcd_channel, p_vcom_info->readDelay, 
												p_vcom_info->minvalue, p_vcom_info->maxvalue, 
												p_vcom_info->initVcom, &find_vcom, &find_flick, 1,
												p_vcom_info->initVcom, flick, 0, min_ok_flick, 0, 
												max_ok_flick, 0, 0, 0, 0);

				printf("z_auto_flick_2: end. find: vcom = %d, flick = %d.\n", find_vcom, find_flick);
				vcom_list[index] = find_vcom;
				flick_list[index] = find_flick;
			}
			#else
			int flick_erro = z_auto_flick(channel, lcd_channel, p_vcom_info->readDelay, p_vcom_info->minvalue, p_vcom_info->maxvalue, 
											p_vcom_info->initVcom, &find_vcom, &find_flick, 0);
			if (flick_erro == 0)
			{
				printf("===========   auto flick end, find vcom: %d. try again: ============\n", find_vcom);			
				flick_erro = z_auto_flick(channel, lcd_channel, p_vcom_info->readDelay, p_vcom_info->minvalue, p_vcom_info->maxvalue, 
											find_vcom, &find_vcom2, &find_flick, 0);
				printf("== flick2 end. error: %d. vcom1: %d, vcom2: %d.\n", flick_erro, find_vcom, find_vcom2);

				vcom_list[index] = find_vcom2;
				flick_list[index] = find_flick;
				
			}
			#endif

			gettimeofday(&tv2, NULL);
			int diff = ((tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec - tv1.tv_usec) / 1000;
			printf("=== Auto Flick time: %dms.  tv1: %d.%d, tv2: %d.%d.	===\n", diff, tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_sec);
			#endif
			
			#endif
		}
		#ifndef USE_Z_CA210_LIB
		else
		{
			#if 1
			//
			int vcom = box_autoFlick_initVcom;
	//		int last_vcom = vcom;
			int last_flick = 0;
			int last_x = 0;
	//		int max_vcom = vcom; //调整过的最大的VCOM
	//		int max_vcom_flick = 0;
			int min_vcom = vcom; //调整过的最小的VCOM
			int min_vcom_flick = 0;
			int check_1_vcom = 0;
			int check_1_vcom_flick = 0;
		
			//FLICK设备准备, 出现异常, 就复位一个CA310
			lcd_autoFlickEnd(lcd_channel);
			
			usleep(1000*100); //2017-5-26

			// check init vcom
			int retVal = lcd_autoFlickStart(lcd_channel, 0);
			if(retVal != 0)
			{
				printf("channel %d lcd_autoFlickStart faild timout or ret failed. mipiflick.c %d\n", channel, __LINE__);
				int ret = lcd_autoFlickEnd(lcd_channel);//ca310Transfer(lcd_channel, LCD_CMD_FLICK_END, NULL, 0, NULL);
				ret = lcd_autoFlickStart(lcd_channel, 0);//ca310Transfer(lcd_channel, LCD_CMD_FLICK_START, NULL, 0, NULL);
				if(ret == 0)
				{
					printf("ca310 remote ok. channel=%d  lcd_channel=%d\n", channel, lcd_channel);
				}
				//sem_post(&m_channel_flick_thread_sem[index]); //此次调节结束�?
				//continue;
			}
			
			usleep(1000*100); //2017-5-26

			//检查是否出现异�? 读出来的值是0, 或者是-1
			{
				int i,twoError = 1;
				for(i=0; i < 2; i++)
				{
					retVal = lcd_autoFlickWork(lcd_channel, 0);
					if(retVal <= 0)
					{
					/*	int ret = lcd_autoFlickEnd(lcd_channel);//ca310Transfer(lcd_channel, LCD_CMD_FLICK_END, NULL, 0, NULL);
						ret = lcd_autoFlickStart(lcd_channel, 0);//ca310Transfer(lcd_channel, LCD_CMD_FLICK_START, NULL, 0, NULL);
						if(ret == 0)
						{
							printf("ca310 remote ok. channel=%d  lcd_channel=%d\n", channel, lcd_channel);
						}*/
					}
					else
					{
						twoError = 0;
						break;
					}
				}
				
				if(twoError)
				{
					printf("channel=%d lcd_channel=%d -> lcd_autoFlickWork faild. mipiflick.c %d\n", channel, lcd_channel, __LINE__);
					lcd_autoFlickEnd(lcd_channel);
					sem_post(&m_channel_flick_thread_sem[index]); //此次调节结束�?
					continue;
				}
			}
			
			if(retVal <= MIN_VCOM_FLICK_VALUE)
			{
				if(box_autoFlick_readDelay > 0) 
					usleep(1000 * box_autoFlick_readDelay); //ms
					
				int retVal2 = lcd_autoFlickWork(lcd_channel, 0); //2次确�?
				if(retVal2 <= MIN_VCOM_FLICK_VALUE) //结果还是小于2.0, OK
				{
					if(retVal2 > 150) //看看默认向左, 还是向右的值哪个比这个值还�? 就用哪个
					{
						if(enable_left && vcom >= 1) //�?
						{
							int left_vcom = vcom - 1;
							
							HI_UNF_SPI_Lock(channel); 

							#ifdef USED_IC_MANAGER
							ic_mgr_write_vcom(channel, left_vcom);
							#endif
							
							HI_UNF_SPI_UnLock(channel); 
							
							//
							if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay);
							int retVal3 = lcd_autoFlickWork(lcd_channel, 0);
							if(retVal3 < retVal2) //1次确�?
							{
								if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay);
								int retVal33 = lcd_autoFlickWork(lcd_channel, 0);
								if(retVal33 < retVal2) //2次确�?
								{
									vcom_list[index] = left_vcom;
									flick_list[index] = retVal33;
									//
									lcd_autoFlickEnd(lcd_channel); //关闭CA310
									sem_post(&m_channel_flick_thread_sem[index]); //此次调节结束�?
									continue;
								}
							}
						}
						
						if(enable_right && vcom < 255) //�?
						{
							int right_vcom = vcom + 1;
							HI_UNF_SPI_Lock(channel);

							#ifdef USED_IC_MANAGER
							ic_mgr_write_vcom(channel, right_vcom);
							#endif
							
							HI_UNF_SPI_UnLock(channel); 
							//
							if(box_autoFlick_readDelay > 0) 
								usleep(1000 * box_autoFlick_readDelay);
							
							int retVal4 = lcd_autoFlickWork(lcd_channel, 0);
							if(retVal4 < retVal2) //1次确�?
							{
								if(box_autoFlick_readDelay > 0) 
									usleep(1000 * box_autoFlick_readDelay);
								
								int retVal44 = lcd_autoFlickWork(lcd_channel, 0);
								if(retVal44 < retVal2) //2次确�?
								{
									vcom_list[index] = right_vcom;
									flick_list[index] = retVal44;
									lcd_autoFlickEnd(lcd_channel); //关闭CA310
									sem_post(&m_channel_flick_thread_sem[index]); //此次调节结束�?
									continue;
								}
							}
						}
					}
					
					vcom_list[index] = vcom;
					flick_list[index] = retVal2;
					lcd_autoFlickEnd(lcd_channel); //关闭CA310
					sem_post(&m_channel_flick_thread_sem[index]); //此次调节结束�?
					continue;
				}
				else //之前是小�? 现在又变大了
				{
					retVal = retVal2;
				}
			}
			
			min_vcom_flick = last_flick = retVal;

			int count = 0;
			int error_count = 0;
			int up_count = 0;
			int down_count = 0;
			int same_count = 0;
			int enter_flick = retVal;

			//
			int flick_big_count = 0;

			//默认, 向后调整2个VCOM�?
			int is_first = 1;
			int is_right = 1; //����
			int is_error = 0;
			vcom += 2; //默认向右偏移2�?
			while(1)
			{
				//CA310已经掉了
				//if(lcd_getStatus(lcd_channel) == 0) break;
				if(vcom == 0) 
					break; //如果依稀到VCOM�?, 则退�?2017-6-4

				//同时只能操作一路SPI
				HI_UNF_SPI_Lock(channel); //保护

				#ifdef USED_IC_MANAGER
				ic_mgr_write_vcom(channel, vcom);
				#endif
				
				HI_UNF_SPI_UnLock(channel); //解除保护

				//读取FLICK�?
				if(box_autoFlick_readDelay > 0) 
					usleep(1000 * box_autoFlick_readDelay); //ms
					
				int yyy;
				for(yyy=0; yyy < 2; yyy++)
				{
					retVal = lcd_autoFlickWork(lcd_channel, 0);
					//if(retVal > 0) break;
					usleep(10 * 1000);
				}
				
				if(retVal <= 0)
				{
					printf("++++++++++++++channel=%d lcd_channel=%d  -> lcd_autoFlickWork faild timout or ret failed %d. mipiflick.c %d++++++++++++++\n", channel, lcd_channel, retVal, __LINE__);
					break;
				}
				
				printf("                           channel %d  vcom = %d  flick = %d\n", channel, vcom, retVal);
			
				//通过FPGA显示VCOM, FLICK�?
		#if 0
				set_fpga_text_vcom(channel, vcom);
				set_fpga_text_flick(channel, retVal);
		#endif

				error_count = 0;

				//最小VCOM
			/*	if(min_vcom > vcom)
				{
					min_vcom = vcom;
					min_vcom_flick = retVal;
				}*/
				//最�?
			/*	if(vcom > max_vcom)
				{
					max_vcom = vcom;
					max_vcom_flick = retVal;
				}*/
				//最小FLICK
				if(retVal > 0 && retVal < min_vcom_flick)
				{
					min_vcom = vcom;
					min_vcom_flick = retVal;
				}

				//是否是VCOM1
				if(vcom == 1)
				{
					check_1_vcom = 1;
					check_1_vcom_flick = retVal;
				}

				//微调总次�?
				count += 1;
				if(count >= 20/*255*/) break;

				//
				if(vcom > (box_autoFlick_maxVcom+6)) break;

				//方向已经确定�? 但是这个方向的FLICK值在逐渐偏大
				if(is_first == 0)
				{
					if(retVal > last_flick) //确定方向, 值还是越来越�? 超过3�? 则退�?
					{
						//if(retVal >= 6*100)
						//{
							flick_big_count += 1;
							if(flick_big_count >= 3)
							{
								break;
							}
						//}
					}
					else
					{
						flick_big_count = 0;
					}
				}

				//如果相同调节了几个VCOM, 值还是没有变�? 则说明CA310不正常了
				if(retVal == last_flick)
				{
					same_count += 1;
					if(same_count >= 3)
					{
						//min_vcom = box_autoFlick_initVcom;
						//min_vcom_flick = 0;
						break;
					}
				}
				else
				{
					//检查每次的FLICK值是否与刚刚进来的FLICK值是否相差无�?
					int x;
					if(retVal > enter_flick)
					{
						x = retVal - enter_flick;
					}
					else
					{
						x = enter_flick - retVal;
					}
					if(x <= 10)
					{
						same_count += 1;
						if(same_count >= 3)
						{
							//min_vcom = box_autoFlick_initVcom;
							//min_vcom_flick = 0;
							break;
						}
					}
					else
					{
						same_count = 0;
					}
				}

				//检�?
				if(retVal <= MIN_VCOM_FLICK_VALUE) //2.0 寻找到最小值了
				{
					if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay); //ms
					int retVal2 = lcd_autoFlickWork(lcd_channel, 0);
					printf("                           channel %d  vcom = %d  flick = %d\n", channel, vcom, retVal2);
					if(retVal2 <= MIN_VCOM_FLICK_VALUE) //再次确认
					{
						if(retVal2 > 150) //看看最小值向�? 还是向右的值哪个比这个值还�? 就用哪个
						{
							if(enable_left && vcom >= 1) //�?
							{
								int left_vcom = vcom - 1;
								HI_UNF_SPI_Lock(channel); 

								#ifdef USED_IC_MANAGER
								ic_mgr_write_vcom(channel, left_vcom);
								#endif
								
								HI_UNF_SPI_UnLock(channel); 
								//
								if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay); 
								int retVal3 = lcd_autoFlickWork(lcd_channel, 0);
								if(retVal3 < retVal2) //1次确�?
								{
									if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay); 
									int retVal33 = lcd_autoFlickWork(lcd_channel, 0);
									if(retVal33 < retVal2) //再次确认
									{
										vcom_list[index] = left_vcom;
										flick_list[index] = retVal33;
										break;
									}
								}
							}
							if(enable_right && vcom < 255) //�?
							{
								int right_vcom = vcom + 1;
								HI_UNF_SPI_Lock(channel); //保护

								#ifdef USED_IC_MANAGER
								ic_mgr_write_vcom(channel, right_vcom);
								#endif
								
								HI_UNF_SPI_UnLock(channel); //解除保护
								//
								if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay); 
								int retVal4 = lcd_autoFlickWork(lcd_channel, 0);
								if(retVal4 < retVal2) //1次确�?
								{
									if(box_autoFlick_readDelay > 0) usleep(1000 * box_autoFlick_readDelay); 
									int retVal44 = lcd_autoFlickWork(lcd_channel, 0);
									if(retVal44 < retVal2) //2次确�?
									{
										vcom_list[index] = right_vcom;
										flick_list[index] = retVal44;
										break;
									}
								}
							}
						}
						retVal = retVal2;
						min_vcom_flick = retVal;
						vcom_list[index] = vcom;
						flick_list[index] = retVal;
						break;
					}
					else //�?次确认时,值变大了,只能再继续微�?
					{
						retVal = retVal2;
						if(retVal > 0 && retVal < min_vcom_flick)
						{
							min_vcom = vcom;
							min_vcom_flick = retVal;
						}
					}
				}
				
				//得到当前FLICK与上次的FLICK之前的差�?
				int x = 0;
				if(retVal < last_flick) //方向对了
				{
					x = last_flick - retVal;
				}
				else //�������
				{
					x = retVal - last_flick;
				}

				//
				int step = 1;
				if(is_first == 1) //刚刚进来, 查找方向
				{
					if(retVal > last_flick) //FLICK值上升的趋势
					{
						last_flick = retVal;
						down_count = 0; //清空向前的计�?
						//
						up_count += 1; //默认往后调3个VCOM�?
						if(up_count >= 3)
						{
							is_first = 0; //已经确认方向反了
							//
							is_right = 0; //==========方向: 向左
							vcom = min_vcom;//把当前VCOM移动到原来最小FLICK值的VCOM上面
							if(min_vcom_flick >= 20*100)
							{
								step = 4;
							}
							else if(min_vcom_flick >= 10*100)
							{
								step = 2;
							}
							else if(min_vcom_flick >= 8*100)
							{
								step = 2;
							}
							else if(min_vcom_flick >= 6*100)
							{
								step = 2;
							}
							else if(min_vcom_flick >= 4*100)
							{
								step = 2;
							}
							else 
							{
								step = 1;
							}
							printf("%d---------------------------goto left.\n", channel);
						}
						else //继续向后面找
						{
							step = 2;
						}
					}
					else //FLICK值下降的趋势
					{
						last_flick = retVal;
						up_count = 0; //清空向后的计�?
						//
						down_count += 1;
						if(down_count >= 3)
						{
							is_first = 0; //关闭探测方向
							//
							is_right = 1; //==========方向: 向右
							if(min_vcom_flick >= 20*100)
							{
								step = 4;
							}
							else if(min_vcom_flick >= 10*100)
							{
								step = 2;
							}
							else if(min_vcom_flick >= 8*100)
							{
								step = 2;
							}
							else if(min_vcom_flick >= 6*100)
							{
								step = 2;
							}
							else if(min_vcom_flick >= 4*100)
							{
								step = 2;
							}
							else
							{
								step = 1;
							}
							printf("%d---------------------------goto right.\n", channel);
						}
						else //
						{
							step = 2;
						}
					}
				}
				else //已经确认了方向了
				{
					if(is_right) //向右
					{
						if(retVal <= last_flick) //正常
						{
							is_error = 0;
							//
							if(retVal > 20*100) //20.0
							{
								step = 5;
							}
							else if(retVal > 10*100) //10.0
							{
								step = 4;
							}
							else if(retVal > 8*100) //8.0
							{
								step = 3;
							}
							else if(retVal > 5*100) //5.0
							{
								step = 2;
							}
							else if(retVal > 3*100) //3.0
							{
								step = 1;
							}
							else if(retVal > 2*100) //2.0
							{
								step = 1;
							}
							else
							{
								step = 1;
							}
						}
						else //还是反了
						{
							is_error += 1;
							if(is_error >= 3)
							{
								is_error = 0;
								//
								is_right = 0; //改变方向:  向左
								vcom = min_vcom;
								//
								if(min_vcom_flick > 12*100)
								{
									step = 3;
								}
								else if(min_vcom_flick > 8*100)
								{
									step = 2;
								}
								else if(min_vcom_flick > 4*100)
								{
									step = 2;
								}
								else if(min_vcom_flick > 3*100)
								{
									step = 2;
								}
								else if(min_vcom_flick > 2*100)
								{
									step = 1;
								}
								else
								{
									step = 1;
								}
							}
							else
							{
								if(retVal > 12*100)
								{
									step = 3;
								}
								else if(retVal > 8*100)
								{
									step = 2;
								}
								else if(retVal > 4*100)
								{
									step = 2;
								}
								else
								{
									step = 1;
								}
							}
						}
					}
					else //向左
					{
						if(retVal <= last_flick) //正常
						{
							is_error = 0;
							//
							if(retVal > 20*100) //20.0
							{
								step = 5;
							}
							else if(retVal > 10*100) //10.0
							{
								step = 4;
							}
							else if(retVal > 8*100) //8.0
							{
								step = 3;
							}
							else if(retVal > 5*100) //5.0
							{
								step = 2;
							}
							else if(retVal > 3*100) //3.0
							{
								step = 1;
							}
							else if(retVal > 2*100) //2.0
							{
								step = 1;
							}
							else
							{
								step = 1;
							}
						}
						else //还是反了?
						{
							is_error += 1;
							if(is_error >= 3)
							{
								is_error = 0;
								//
								is_right = 1; //改变方向: 向右
								vcom = min_vcom;
								if(min_vcom_flick > 12*100)
								{
									step = 3;
								}
								else if(min_vcom_flick > 8*100)
								{
									step = 2;
								}
								else if(min_vcom_flick > 4*100)
								{
									step = 2;
								}
								else
								{
									step = 1;
								}
							}
							else
							{
								if(min_vcom_flick > 12*100)
								{
									step = 3;
								}
								else if(min_vcom_flick > 8*100)
								{
									step = 2;
								}
								else if(min_vcom_flick > 4*100)
								{
									step = 2;
								}
								else
								{
									step = 1;
								}
							}
						}
					}
					last_flick = retVal;
				}

				//微调
				if(is_right == 0) //向前
				{
					if(vcom > 0)
					{
						if((vcom - step) >= 0)
						{
							vcom -= step;
						}
						else
						{
							vcom -= 1;
						}
					}
					else //已经是最前的�?
					{
						is_right = 1; //再次换方�?
						vcom = min_vcom + 1;
					}
				}
				else //往�?
				{
					if(vcom < 255)
					{
						if((vcom + step) <= 255)
						{
							vcom += step;
						}
						else
						{
							vcom += 1;
						}
					}
					else //已经是最后的�?
					{
						is_right = 0; //向左
						vcom = min_vcom - 1;
						break;
					}
				}

				//
				usleep(1000*50);
			}

			//VCOM1没有检查过
			if((flick_list[index]==0 && check_1_vcom == 0) || (min_vcom_flick>MIN_VCOM_FLICK_VALUE && check_1_vcom==0))
			{
				vcom = 1;

				//同时只能操作一路SPI
				HI_UNF_SPI_Lock(channel); //保护

				#ifdef USED_IC_MANAGER
				ic_mgr_write_vcom(channel, vcom);
				#endif
				
				HI_UNF_SPI_UnLock(channel); //解除保护

				//读取FLICK�?
				if(box_autoFlick_readDelay > 0) 
					usleep(1000 * box_autoFlick_readDelay * 2); //ms
					
				int retVal = lcd_autoFlickWork(lcd_channel, 0);
				if(box_autoFlick_readDelay > 0) 
					usleep(1000 * box_autoFlick_readDelay * 2); //ms
					
				retVal = lcd_autoFlickWork(lcd_channel, 0);
				printf("                           channel %d  vcom = %d  flick = %d\n", channel, vcom, retVal);

				//说明VCOM1的FLICK值比自动查询出来的最小的FLICK值还要小, 则赋�?
				if(retVal < min_vcom_flick)
				{
					vcom_list[index] = vcom;
					flick_list[index] = retVal;
				}
			}

			//还是没有找到合适的, 再去确认一�?
			if(flick_list[index] == 0)
			{
				vcom = min_vcom;

				//同时只能操作一路SPI
				HI_UNF_SPI_Lock(channel); //保护

				#ifdef USED_IC_MANAGER
				ic_mgr_write_vcom(channel, vcom);
				#endif
				
				HI_UNF_SPI_UnLock(channel); //解除保护

				//读取FLICK�?
				if(box_autoFlick_readDelay > 0) 
					usleep(1000 * box_autoFlick_readDelay * 2); //ms					
				int retVal = lcd_autoFlickWork(lcd_channel, 0);
				
				if(box_autoFlick_readDelay > 0) 
					usleep(1000 * box_autoFlick_readDelay * 2); //ms
				retVal = lcd_autoFlickWork(lcd_channel, 0);
				
				printf("                           channel %d  vcom = %d  flick = %d\n", channel, vcom, retVal);
				vcom_list[index] = vcom;
				flick_list[index] = retVal;
			}

			#endif
		}
		#endif
		
		//FLICK设备结束
		#ifdef USE_Z_CA210_LIB
		retVal = ca210_stop_flick_mode(lcd_channel);
		#else
		retVal = lcd_autoFlickEnd(lcd_channel);
		#endif
		
		if(retVal == -1)
		{
			printf("channel %d lcd_autoFlickEnd return error. %d\n", channel, __LINE__);
		}

		//
		sem_post(&m_channel_flick_thread_sem[index]); //此次调节VCOM结束
	}
	#endif
	
}

int get_lcd_channel_by_mipi_channel(int mipi_channel)
{
	int lcd_channel = 0;
	
	switch(mipi_channel)
	{
		case 1:
		case 2:
			lcd_channel = 1;
			break;

		case 3:
		case 4:
			lcd_channel = 2;
			break;
			
		default :
			lcd_channel = 1;
			break;
	}
}

void flick_test(int mipi_channel, int vcom, int delay)
{
	float f_flick = 0.00;
	int lcd_channel = get_lcd_channel_by_mipi_channel(mipi_channel);
	
	#ifdef USED_IC_MANAGER
	ic_mgr_write_vcom(mipi_channel, vcom);
	#endif

	usleep(delay * 1000);

	#ifdef USE_Z_CA210_LIB
	int retVal = 0;
	ca210_capture_flick_data(lcd_channel, &f_flick);
	retVal = f_flick * 100;
	#endif

	int lcd_flick_value = f_flick * 100;
	show_lcd_msg(mipi_channel, 1, 0, vcom, lcd_flick_value, 0, 0);

	// send flick data to client.
	client_flick_test_ack(mipi_channel, vcom, f_flick);
}

//手动调节FLICK
#ifdef JUST_USED_MIPI_CHANNEL_1
#else

unsigned int m_flick_manual_vcom = 0;
unsigned int m_flick_manual_flick = 0;
static unsigned int m_flick_manual_last_time = 0;
void flick_manual(int vcomIndex)
{
	#if 0
	int channel = m_channel_1;
	int lcd_channel = 1;

	m_flick_manual_vcom = vcomIndex;

	#ifdef USED_IC_MANAGER
	ic_mgr_write_vcom(channel, vcomIndex);
	#endif

	int last_flick = 0;
	//检查CA310串口是否打开
	if(lcd_getStatus(lcd_channel) != 0)
	{
		//通知FLICK设备, 准备开始FLICK操作�?
		#ifdef USE_Z_CA210_LIB
		ca210_stop_flick_mode(lcd_channel);
		usleep(1000*200);
		ca210_start_flick_mode(lcd_channel);
		#else
		lcd_autoFlickEnd(lcd_channel); 
		usleep(1000*200);
		lcd_autoFlickStart(lcd_channel, 0);
		#endif

		int i;
		float f_flick = 0.00;
		
		for(i=0; i < 3; i++)
		{
			#ifdef USE_Z_CA210_LIB
			int retVal = 0;
			ca210_capture_flick_data(lcd_channel, &f_flick);
			retVal = f_flick * 100;
			#else
			int retVal = lcd_autoFlickWork(lcd_channel, 0); //查找FLICK设备返回的FLICK值�?
			#endif
			
			if(retVal == -1) 
				printf("check lcd flick faild timout or ret failed\n"); //timeout
				
			if(last_flick == retVal)
			{
				break;
			}
			
			last_flick = retVal;
			usleep(1000*200);
		}

		//通知FLICK设备, 停止FLICK操作�?
		#ifdef USE_Z_CA210_LIB
		ca210_stop_flick_mode(lcd_channel);
		#else
		lcd_autoFlickEnd(lcd_channel);
		#endif
	}
	
	m_flick_manual_flick = last_flick;

	#if 1
	show_lcd_msg(channel, 1, 0, vcomIndex, last_flick, 0, 0);
	#else
	//只显示VCOM
	set_fpga_text_show(1,0,0,0); //VCOM, OTP, OK, OK2
	set_fpga_text_vcom(channel, vcomIndex);
	set_fpga_text_flick(channel, last_flick);
	#endif
	
	printf("manual flick: vom: %d, flick: %d.\n", vcomIndex, last_flick);

	//printf("manual flick end!\n");
	#endif
	
	return ;
}
#endif

void flick_init()
{
    pthread_mutex_init(&flickLock,0);
}


