
#include <stdio.h>

#include "../ic_manager.h"
#include "chip_nt_35521s.h"


typedef struct tag_nt_35521s_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom;
	unsigned int  vcom_otp_times;
}nt_35521s_info_t;

static nt_35521s_info_t s_nt_35521s_private_info[MAX_CHIP_CHANNEL_NUMS] = { 0 };


void nt_35521s_set_page(int mipi_channel, unsigned char page_no)
{
//	unsigned char reg[] = { 0xF0, 0x55, 0xaa, 0x52,0x08,0x00 };
//	reg[5] = page_no;
//	DCSLongWriteWithPara(mipi_channel, 0xB9, reg, sizeof(reg));

	int n = 0;
	unsigned char cmd[100] = { 0 };
	cmd[n++] = 0x39;
	cmd[n++] = 0xF0;
	cmd[n++] = 0x5;
	cmd[n++] = 0x55;
	cmd[n++] = 0xaa;
	cmd[n++] = 0x52;
	cmd[n++] = 0x08;
	cmd[n++] = page_no;

	SendCode(mipi_channel, cmd);
}

int nt_35521s_get_vcom_otp_times(int mipi_channel, unsigned char* p_vcom1_otp_times, 
										unsigned char* p_vcom2_otp_times)
{
	unsigned char reg[3] = { 0 };

	// hardware reset.

	// delay 120 ms.

	// enable extend command
//	nt_35521s_enable_extend_cmd(mipi_channel, 1);
	
	ReadModReg(mipi_channel, 1, 0xB6, 3, reg);

	printf("nt_35521s_get_vcom_otp_times: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

	reg[2] >>= 5;

	unsigned char otp_times = 0;
	switch (reg[2])
	{
		case 0x07:
			otp_times = 0;
			break;
		
		case 0x03:
			otp_times = 1;
			break;

		case 0x01: 
			otp_times = 2;
			break;

		case 0x00:
			otp_times = 3;
			break;

		default:
			printf("nt_35521s_get_vcom_otp_times: invalid times = %d.\n", reg[2]);
			otp_times = 3;
			break;
	}

	*p_vcom1_otp_times = otp_times;
	*p_vcom2_otp_times = otp_times;
	
	printf("nt_35521s_get_vcom_otp_times: mipi_channel: %d, vcom otp times: %d. \n", mipi_channel, 
			*p_vcom1_otp_times);
	
	return 0;
}

int nt_35521s_read_vcom(int mipi_channel, unsigned int *p_vcom)
{
	unsigned char reg[3] = { 0 };

	nt_35521s_set_page(mipi_channel,1);
	
	ReadModReg(mipi_channel, 1, 0xBE, 1, &reg[0]);
	printf("nt_35521s_read_vcom: %02x.\n", reg[0]);

	*p_vcom = reg[0];
	
	printf("nt_35521s_read_vcom: mipi_channel: %d, vcom otp value: %#x. \n", mipi_channel, 
			*p_vcom);
	return 0;
}

int nt_35521s_write_vcom(int mipi_channel, unsigned int vcom_value)
{
	unsigned char reg[3] = { 0 };

	reg[0] = vcom_value & 0xFF;

	nt_35521s_set_page(mipi_channel,1);

	DCSLongWriteWithPara(mipi_channel, 0xBE, reg, sizeof(reg));
	
	return 0;
}

static int nt_35521s_read_nvm_vcom(int mipi_channel, int index, unsigned int* p_vcom_otp_value)
{
	return 0;
	
	// hw reset
	DCSShortWriteNoPara(mipi_channel, 0x01);

	// sleep out
	DCSShortWriteNoPara(mipi_channel, 0x11);

	usleep(120 * 1000);
	
	// enable extend command
	nt_35521s_enable_extend_cmd(mipi_channel, 1);

	// set otp index: 0xBB
	unsigned char otp_key[7] = { 0 };
	otp_key[1] = 0x0D;	// 0x0D -> 0x14.
	DCSLongWriteWithPara(mipi_channel, 0xBB, otp_key, sizeof(otp_key));

	// set otp por: 0xbb	
	otp_key[3] = 0x80;	// 0x0D -> 0x14.
	DCSLongWriteWithPara(mipi_channel, 0xBB, otp_key, sizeof(otp_key));

	// clear otp por: 0xbb	
	otp_key[3] = 0x00;	// 0x0D -> 0x14.
	DCSLongWriteWithPara(mipi_channel, 0xBB, otp_key, sizeof(otp_key));

	// read data: 0xbb
	unsigned char read_data[5] = { 0 };
	ReadModReg(mipi_channel, 1, 0xBB, sizeof(read_data), read_data);

	// compare xxx

	return 0;
}

void delay_ms(int ms)
{
	usleep(ms * 1000);
}

int nt_35521s_burn_vcom(int mipi_channel, unsigned int otp_vcom_value)
{
	int use_internal_vpp = 1;

	printf("nt_35521s_burn_vcom: mipi_channel=%d, vcom=%d.\n", mipi_channel, otp_vcom_value);

	setHsmode(0);
	mipi_gpio_reset(0);
	mipi_gpio_reset(1);

	DCSShortWriteNoPara(mipi_channel, 0x11);

	delay_ms(150);

	// password
	unsigned char passwd[100] = 
	{
		0x39, 0xB9, 0x03, 0xFF, 0x83, 0x94,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, passwd);

	// vcom
	unsigned char vcom_value[100] = 
	{
		0x39, 0xB6, 0x03, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	vcom_value[3] = otp_vcom_value;
	vcom_value[4] = otp_vcom_value;
	SendCode(mipi_channel, vcom_value);

	delay_ms(100);
	
	// otp key
	#ifdef ENABlE_OTP_BURN
	unsigned char otp_key[100] = 
	{
		0x39, 0xBB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x55,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_key);

	// interal power
	unsigned char otp_power[100] = 
	{
		0x39, 0xBB, 0x02, 0x80, 0x0D,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_power);

	unsigned char otp_power2[100] = 
	{
		0x39, 0xBB, 0x04, 0x80, 0x0D, 0x00, 0x01,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_power2);
	#endif
	
	delay_ms(550);

	unsigned char otp_end[100] = 
	{
		0x39, 0xBB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_end);

	// 
	SendCode(mipi_channel, passwd);

	DCSShortWriteNoPara(mipi_channel, 0x10);
	
	setHsmode(1);
	
	return 0;
}
					

static int chip_nt_35521s_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
								void* p_chip_data)
{
	unsigned char reg[3];
	ReadModReg(mipi_channel, 1, 0x04, 3, reg);
	printf("chip_nt_35521s_get_chip_id: chip channel: %d, mipi_channel: %d, read id4: %02x:%02x:%02x. \n", 
				chip_channel, mipi_channel, reg[0], reg[1], reg[2]);

	if (reg[0] == 0x00 && reg[1] == 0x80 && reg[2] == 0x00)
	{
		if (p_id_data && id_data_len >= 3)
		{
			p_id_data[0] = reg[0];
			p_id_data[1] = reg[1];
			p_id_data[2] = reg[2];
			return 3;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		printf("chip_nt_35521s_get_chip_id: chip channel: %d, mipi_channel: %d, : read id failed!\n", 
				chip_channel, mipi_channel);
		
		return -1;
	}
	return -1;
}


static int chip_nt_35521s_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{
	chip_nt_35521s_get_chip_id(chip_channel, mipi_channel, s_nt_35521s_private_info[chip_channel].id, 
								sizeof(s_nt_35521s_private_info[chip_channel].id), p_chip_data);

	unsigned char* p_id = s_nt_35521s_private_info[chip_channel].id;
	if ( (p_id[0] != 0x00) || (p_id[1] != 0x80) || (p_id[2] != 0x00) )
	{
		printf("chip_nt_35521s_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_nt_35521s_private_info[chip_channel].id_ok = 0;

		return -1;
	}

	s_nt_35521s_private_info[chip_channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	nt_35521s_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);
	s_nt_35521s_private_info[chip_channel].vcom_otp_times = vcom1_otp_times;

	// read vcom otp value
	unsigned int vcom_otp_value = 0;
	nt_35521s_read_vcom(mipi_channel, &vcom_otp_value);
	
	s_nt_35521s_private_info[chip_channel].vcom = vcom_otp_value;
	printf("chip_nt_35521s_check_chip_ok:chip:%d, mipi: %d. vcom_otp_times: %d."
			"vcom_otp_value: %d. .\n", chip_channel, mipi_channel,
				s_nt_35521s_private_info[chip_channel].vcom_otp_times,
				s_nt_35521s_private_info[chip_channel].vcom);
	return 0;
}

static int chip_nt_35521s_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	if (s_nt_35521s_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		nt_35521s_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_nt_35521s_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_nt_35521s_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_nt_35521s_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	if (s_nt_35521s_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		nt_35521s_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		// read vcom otp value
		int vcom1_otp_value = 0;
		int vcom2_otp_value = 0;
		nt_35521s_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
		nt_35521s_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);

		*p_otp_vcom_times = vcom1_otp_times;
		*p_otp_vcom_value = vcom1_otp_value;

		printf("chip_nt_35521s_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_nt_35521s_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}


static int chip_nt_35521s_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{
	if (s_nt_35521s_private_info[chip_channel].id_ok == 1)
	{
		int val = 0;
		#ifdef ENABlE_OTP_BURN
		val = nt_35521s_burn_vcom(mipi_channel, otp_vcom_value);
		#else
		printf("chip_nt_35521s_write_chip_vcom_otp_value: *** do nothing ***\n");
		#endif
		return val;
	}

	printf("chip_nt_35521s_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_nt_35521s_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	if (s_nt_35521s_private_info[chip_channel].id_ok == 1)
	{
		int vcom = 0;
		nt_35521s_read_vcom(mipi_channel, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_nt_35521s_read_vcom: end\n");

		return 0;
	}
	printf("chip_nt_35521s_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_nt_35521s_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	if (s_nt_35521s_private_info[chip_channel].id_ok == 1)
	{
		if (s_nt_35521s_private_info[chip_channel].vcom_otp_times == 0)
		{
			//chip_nt_35521s_write_vcom(chip_channel, mipi_channel, vcom_value, p_chip_data)
			nt_35521s_write_vcom(mipi_channel, vcom_value);
		}
		else
		{
			nt_35521s_write_vcom(mipi_channel, vcom_value);
		}
		
		//printf("chip_nt_35521s_write_vcom: end\n");

		return 0;
	}
	
	printf("chip_nt_35521s_write_vcom error: unknow chip!\n");
	
	return -1;
}

static void* chip_nt_35521s_get_and_reset_private_data(int chip_channel)
{
	printf("chip_nt_35521s_get_and_reset_private_data.\n");
	s_nt_35521s_private_info[chip_channel].id_ok = 0;
	memset(s_nt_35521s_private_info[chip_channel].id, 0, sizeof(s_nt_35521s_private_info[chip_channel].id));
	s_nt_35521s_private_info[chip_channel].vcom = 0;
	s_nt_35521s_private_info[chip_channel].vcom_otp_times = 0;
	
	return &s_nt_35521s_private_info[chip_channel];
}


static int chip_nt_35521s_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	// check id 
	int val = chip_nt_35521s_check_chip_ok(chip_channel, mipi_channel, p_chip_data);
	if (val != 0)
	{
		printf("chip_nt_35521s_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_nt_35521s_private_info[chip_channel].vcom;

	#if 1
	// check times
	if (s_nt_35521s_private_info[chip_channel].vcom_otp_times != last_otp_times + 1)
	{
		printf("chip_nt_35521s_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_nt_35521s_private_info[chip_channel].vcom_otp_times);

		#ifdef ENABlE_OTP_BURN
		//return E_ICM_OK;
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}
	#endif

	// check vcom value.
	if (s_nt_35521s_private_info[chip_channel].vcom != vcom_value)
	{
		printf("chip_nt_35521s_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_nt_35521s_private_info[chip_channel].vcom);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
		//return E_ICM_OK;
		#else
		return E_ICM_OK;
		#endif
	}

	return E_ICM_OK;
}


icm_chip_t chip_nt_35521s = 
{
	.my_chip_id = E_MY_CHIP_ID_NT_35521S,

	.fn_get_and_reset_private_data = chip_nt_35521s_get_and_reset_private_data,
	
	.fn_get_chip_id = chip_nt_35521s_get_chip_id,
	.fn_check_chip_ok = chip_nt_35521s_check_chip_ok,
	.fn_read_chip_vcom_opt_times = chip_nt_35521s_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = chip_nt_35521s_read_chip_vcom_opt_info,
	
	.fn_read_vcom = chip_nt_35521s_read_vcom,
	.fn_write_vcom = chip_nt_35521s_write_vcom,
	
	.fn_write_chip_vcom_otp_value = chip_nt_35521s_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = chip_nt_35521s_check_vcom_otp_burn_ok,

};


