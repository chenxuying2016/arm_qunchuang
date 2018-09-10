
#include <stdio.h>

#include "../ic_manager.h"


#include "chip_otm_8019a.h"


#define MAX_VCOM_OTP_TIMES		(3)

static void otm_8019a_ready(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x39, 0x00, 0x01, 0x00, 
		0x39, 0xff, 0x03, 0x80, 0x19, 0x01, 
		0x39, 0x00, 0x01, 0x80, 
		0x39, 0xff, 0x02, 0x80, 0x19, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, cmd);
}



#if 1
typedef struct tag_otm_8019a_info
{
	unsigned char id[5];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom;
	unsigned int vcom_otp_times;
	
}otm_8019a_info_t;

static otm_8019a_info_t s_otm_8019a_private_info[MAX_CHIP_CHANNEL_NUMS] = { 0 };


static int otm_8019a_read_vcom(int mipi_channel, int *p_vcom_value)
{
	unsigned char cmd[100] = 
	{
		0x39, 0x00, 0x01, 0x00, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, cmd);
	unsigned char data[8] = {0xff,0xff,0xff,0xff};
	int ret = ReadModReg(mipi_channel, 1, 0xD9, 1, data);                         //D9
	printf("chip_otm_8019a_read_chip_vcom_opt_info: mipi_channel: %d, VCOM1|VCOM2 0xD9 = 0x%02X(%d)   ret=%d\n", 
				mipi_channel, data[0], data[0], ret);
	if (ret <= 0)
	{
		printf("otm_8019a_read_vcom error! ret = %d.\n", ret);
		return -1;
	}
	
	if (data[0] == 0xFF)
	{
		printf("chip_otm_8019a_read_chip_vcom_opt_info error!\n");
		return -1;
	}

	if (p_vcom_value)
		*p_vcom_value = data[0];

	return 0;
}


static int otm_8019a_write_vcom(int mipi_channel, int vcom_value)
{
	int n = 0;
	unsigned char cmd2[100];
	memset(cmd2, 0xff, sizeof(cmd2));
	cmd2[n++] = 0x39;
	cmd2[n++] = 0x00;
	cmd2[n++] = 0x01;
	cmd2[n++] = 0x00;
	cmd2[n++] = 0x39;
	cmd2[n++] = 0xD9;
	cmd2[n++] = 0x01;
	cmd2[n++] = vcom_value & 0xff;
		
	SendCode(mipi_channel, cmd2);
	//printf("otm_write_vcom: mipi_channel:%d. vcom: %d.\n", mipi_channel, vcom_value);
	return 0;
}

static int chip_otm_8019a_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
										void* p_chip_data)
{	
    unsigned char data[8] = {0,0,0,0,0,0,0,0};
	memset(data, 0, sizeof(data));
	int ret = ReadModReg(mipi_channel, 1, 0xA1, 5, data);
	
	printf("chip_otm_8019a_get_chip_id: mipi_channel: %d, Driver_ID 0xA1 = %02X %02X %02X %02X %02X   ret=%d\n", 
				mipi_channel, data[0], data[1], data[2], data[3], data[4], ret);
	if( (data[0]== 0x01) && (data[1]==0x8B) && (data[2]==0x80) 
			&& (data[3]==0x19) && (data[4]==0xFF) )
	{
		if (p_id_data)
		{
			p_id_data[0] = data[0];
			p_id_data[1] = data[1];
			p_id_data[2] = data[2];
			p_id_data[3] = data[3];
			p_id_data[4] = data[4];
		}
		
		return 5;
	}

	printf("chip_otm_8019a_get_chip_id: read id failed!\n");
	return -1;
}


static int chip_otm_8019a_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	if (s_otm_8019a_private_info[chip_channel].id_ok == 1)
	{
		otm_8019a_ready(mipi_channel);
		
		// read vcom otp times
		unsigned char cmd2[100] = 
		{
			0x39, 0x00, 0x01, 0x02, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		SendCode(mipi_channel, cmd2);
		
		unsigned char data[8] = {0xff,0xff,0xff,0xff};
		int ret = ReadModReg(mipi_channel, 1, 0xF1, 1, data);

		short opt_count = 0;
		if(data[0] & 0x80) opt_count += 1;
		if(data[0] & 0x40) opt_count += 1;
		if(data[0] & 0x20) opt_count += 1;
		if(data[0] & 0x10) opt_count += 1;
		printf("chip_otm_8019a_read_chip_vcom_opt_times: mipi_channel: %d, OTP-VCOM-TIMES 0xF1 = 0x%X (%d)   ret=%d\n", 
					mipi_channel, data[0], opt_count, ret);
		
		*p_otp_vcom_times = opt_count;
		return 0;
	}
	
	printf("chip_otm_8019a_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_otm_8019a_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{
	// read and check id;
	int val = chip_otm_8019a_get_chip_id(chip_channel, mipi_channel, s_otm_8019a_private_info[chip_channel].id, 
								sizeof(s_otm_8019a_private_info[chip_channel].id), p_chip_data);

	if ( val < 0)
	{
		printf("chip_otm_8019a_check_chip_ok error: read chip id failed!\n");		
		s_otm_8019a_private_info[chip_channel].id_ok = 0;

		return -1;
	}

	s_otm_8019a_private_info[chip_channel].id_ok = 1;

	#if 1
	// read vcom otp times
	int vcom_otp_times = 0;
	chip_otm_8019a_read_chip_vcom_opt_times(chip_channel, mipi_channel, &vcom_otp_times, p_chip_data);
	s_otm_8019a_private_info[chip_channel].vcom_otp_times = vcom_otp_times;

	// read vcom otp value
	unsigned int vcom_otp_value = 0;
	otm_8019a_read_vcom(mipi_channel, &vcom_otp_value);
	s_otm_8019a_private_info[chip_channel].vcom = vcom_otp_value;

	printf("chip_otm_8019a_check_chip_ok:chip:%d, mipi: %d. vcom_otp_times: %d, "
			"vcom_otp_value: %d. \n", chip_channel, mipi_channel,
				s_otm_8019a_private_info[chip_channel].vcom_otp_times,
				s_otm_8019a_private_info[chip_channel].vcom);
	#endif
	
	return 0;
}

static void* chip_otm_8019a_get_and_reset_private_data(int chip_channel)
{
	printf("chip_otm_8019a_get_and_reset_private_data.\n");
	s_otm_8019a_private_info[chip_channel].id_ok = 0;
	memset(s_otm_8019a_private_info[chip_channel].id, 0, sizeof(s_otm_8019a_private_info[chip_channel].id));
	s_otm_8019a_private_info[chip_channel].vcom = 0;
	s_otm_8019a_private_info[chip_channel].vcom_otp_times = 0;
	
	return &s_otm_8019a_private_info[chip_channel];
}


static int chip_otm_8019a_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	if (s_otm_8019a_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		int vcom_otp_times = 0;
		//ili_9806_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);
		chip_otm_8019a_read_chip_vcom_opt_times(chip_channel, mipi_channel, &vcom_otp_times, p_chip_data);
		

		// read vcom otp value
		int vcom_otp_value = 0;
		int val = otm_8019a_read_vcom(mipi_channel, &vcom_otp_value);
		if (val != 0)
		{
			printf("chip_otm_8019a_read_chip_vcom_opt_info error!\n");
			return -1;
		}

		*p_otp_vcom_times = vcom_otp_times;
		*p_otp_vcom_value = vcom_otp_value;

		printf("chip_otm_8019a_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_otm_8019a_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}

static int chip_otm_8019a_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	if (s_otm_8019a_private_info[chip_channel].id_ok == 1)
	{
		int vcom = 0;

		otm_8019a_ready(mipi_channel);
		if (otm_8019a_read_vcom(mipi_channel, &vcom) != 0)
		{
		}
		*p_vcom_value = vcom;
		printf("chip_otm_8019a_read_vcom: end\n");

		return 0;
	}
	printf("chip_otm_8019a_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_otm_8019a_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	//printf("chip_otm_8019a_write_vcom: chip channel = %d, mipi_channel: %d. ...\n",
	//		chip_channel, mipi_channel);
	
	if (s_otm_8019a_private_info[chip_channel].id_ok == 1)
	{
		//otm_8019a_ready(mipi_channel);
		otm_8019a_write_vcom(mipi_channel, vcom_value);

		#if 0
		int temp_vcom = 0;
		otm_read_vcom(mipi_channel, &temp_vcom);
		
		printf("chip_otm_8019a_write_vcom:  read vcom = %d. end\n", temp_vcom);
		#endif
		
		return 0;
	}
	
	printf("chip_otm_8019a_write_vcom error: unknow chip!\n");
	
	return -1;
}


static void otm_8019a_unlock(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x39, 0x00, 0x01, 0x00, 
		0x39, 0xff, 0x03, 0x80, 0x19, 0x01, 
		0x39, 0x00, 0x01, 0x80, 
		0x39, 0xff, 0x02, 0x80, 0x19, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, cmd);
}

static void otm_8019a_sleep_out(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x05, 0x11, 0x00,  
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, cmd);
}

static void otm_8019a_display_on(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x05, 0x29, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};  //29
	SendCode(mipi_channel, cmd);
}


static void otm_8019a_display_off(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x05, 0x28, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};  //28
	SendCode(mipi_channel, cmd);
}


static void otm_8019a_sleep_in(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x05, 0x10, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};  //10
	SendCode(mipi_channel, cmd);
}

static void otm_8019a_program_on(int mipi_channel)
{
	unsigned char cmd2[100] = 
	{
		0x39, 0x00, 0x01, 0x00,
		//0x39, 0xEB, 0x01, 0x01,
		0x39, 0xEB, 0x01, 0x81,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	}; //00 00, EB 01
	SendCode(mipi_channel, cmd2);
}

static void otm_8019a_program_end(int mipi_channel)
{
	unsigned char cmd[100] = 
	{
		0x39, 0x00, 0x01, 0x00, 
		0x39, 0xEB, 0x01, 0x00, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	}; //00 00, EB 00
	SendCode(mipi_channel, cmd);
}

static int chip_otm_8019a_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{

	if (s_otm_8019a_private_info[chip_channel].id_ok == 1)
	{
		int val = 0;

		mipi_to_lp_mode(mipi_channel);
		
		printf("mipi reset.\n");
		mipi_reset(mipi_channel);
		
		usleep(1000 * 5);

		// unlock
		printf("unlock \n");
		otm_8019a_unlock(mipi_channel);

		// sleep out
		printf("sleep out \n");
		otm_8019a_sleep_out(mipi_channel);

		usleep(150 * 1000);

		// write vcom
		printf("write vcom \n");
		otm_8019a_write_vcom(mipi_channel, otp_vcom_value);

		// check vcom value.
		int read_vcom = 0;
		printf("read vcom\n");
		otm_8019a_read_vcom(mipi_channel, &read_vcom);
		if (read_vcom != otp_vcom_value)
		{
			printf("write vcom check error: write: %d, read: %d. \n", otp_vcom_value, read_vcom);
			val = -1;
		}

		usleep(1000 * 10);

		// display off
		otm_8019a_display_off(mipi_channel);
		usleep(100 * 1000);

		// sleep in.
		printf("sleep in\n");
		otm_8019a_sleep_in(mipi_channel);

		usleep(800 * 1000); 
		
#ifdef ENABlE_OTP_BURN
		// mtp power on
		printf("mtp power on.\n");
		mtp_power_on(mipi_channel);
		usleep(800 * 1000);

		// program on
		printf("program ...\n");
		otm_8019a_program_on(mipi_channel);
		
		usleep(1200 * 1000);

		// program end.
		printf("program end.\n");
		otm_8019a_program_end(mipi_channel);

		usleep(100*1000);

		// mtp power off
		printf("mtp power off.\n");
		mtp_power_off(mipi_channel); 
#else
		printf("chip_otm_8019a_write_chip_vcom_otp_value: OTP is disable!\n");
#endif

		usleep(100 * 1000);

		printf("mipi reset.\n");
		mipi_reset(mipi_channel);

		usleep(100 * 1000);
		
		// sleep out
		printf("sleep out.\n");
		otm_8019a_sleep_out(mipi_channel);
		
		usleep(150 * 1000);
		
		return val;
	}

	printf("chip_otm_8019a_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_otm_8019a_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	printf(" === chip_otm_8019a_check_vcom_otp_burn_ok  ... === ");

	// sleep out
	otm_8019a_sleep_out(mipi_channel);

	// unlock
	otm_8019a_unlock(mipi_channel);

	// read id
	if (0 != chip_otm_8019a_check_chip_ok(chip_channel, mipi_channel, p_chip_data))
	{
		printf("chip_otm_8019a_check_vcom_otp_burn_ok: read chip id error!\n");
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = vcom_value;

	if (s_otm_8019a_private_info[chip_channel].vcom_otp_times <= last_otp_times)
	{
		printf("chip_otm_8019a_check_vcom_otp_burn_ok: vcom otp times not change:!\n");
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	}

	if (s_otm_8019a_private_info[chip_channel].vcom != vcom_value)
	{
		printf("chip_otm_8019a_check_vcom_otp_burn_ok: vcom otp value not change:!\n");
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}
	
	return E_ICM_OK;
}
#endif

icm_chip_t chip_otm_8019a = 
{
	.my_chip_id = E_MY_CHIP_ID_OTM_8019A,

	.fn_get_and_reset_private_data = chip_otm_8019a_get_and_reset_private_data,

	.fn_get_chip_id = chip_otm_8019a_get_chip_id,
	.fn_check_chip_ok = chip_otm_8019a_check_chip_ok,
	.fn_read_chip_vcom_opt_times = chip_otm_8019a_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = chip_otm_8019a_read_chip_vcom_opt_info,

	.fn_read_vcom = chip_otm_8019a_read_vcom,
	.fn_write_vcom = chip_otm_8019a_write_vcom,
	
	.fn_write_chip_vcom_otp_value = chip_otm_8019a_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = chip_otm_8019a_check_vcom_otp_burn_ok,
};


