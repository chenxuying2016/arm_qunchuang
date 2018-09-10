
#include <stdio.h>

#include "../ic_manager.h"
#include "chip_hi_8394.h"

#define HX_8394_ANALOG_GAMMA_REG			(0xE0)

#define HX_8394_D3G_BANK_REG				(0xBD)
#define HX_8394_DIGITAL_GAMMA_REG			(0xC1)

#define HX_8394_ANALOG_GAMMA_LEN		(58)
#define HX_8394_D3G_R_REG_LEN			(43)
#define HX_8394_D3G_G_REG_LEN			(42)
#define HX_8394_D3G_B_REG_LEN			(42)
#define HX_8394_ALL_D3G_REG_LEN			(HX_8394_D3G_R_REG_LEN + HX_8394_D3G_G_REG_LEN \
												+ HX_8394_D3G_B_REG_LEN)

typedef struct tag_hx_8394_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;

	unsigned char analog_gamma_reg_data[HX_8394_ANALOG_GAMMA_LEN];

	unsigned char digital_gamma_reg_data[3][HX_8394_D3G_R_REG_LEN];
}hx_8394_info_t;

static hx_8394_info_t s_hx_8394_private_info[MAX_CHIP_CHANNEL_NUMS] = { 0 };


void hi_8394_enable_extend_cmd(int mipi_channel, int enable)
{
	// enable extend command
	if (enable)
	{
		//unsigned reg[3] = { 0xFF, 0x83, 0x94 };
		//DCSLongWriteWithPara(mipi_channel, 0xB9, reg, sizeof(reg));

		unsigned char cmd[100] = {
				0x39, 0xB9, 0x03, 0xFF, 0x83, 0x94,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff
			};

		SendCode(mipi_channel, cmd);
	}
	else
	{
		unsigned char cmd[100] = {
				0x39, 0xB9, 0x03, 0xFF, 0xFF, 0xFF,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff
			};

		SendCode(mipi_channel, cmd);
	}
}

int hi_8394_get_vcom_otp_times(int mipi_channel, unsigned char* p_vcom1_otp_times, 
										unsigned char* p_vcom2_otp_times)
{
	unsigned char reg[3] = { 0 };

	// hardware reset.

	// delay 120 ms.

	// enable extend command
	hi_8394_enable_extend_cmd(mipi_channel, 1);
	
	ReadModReg(mipi_channel, 1, 0xB6, 3, reg);

	printf("hi_8394_get_vcom_otp_times: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

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
			printf("hi_8394_get_vcom_otp_times: invalid times = %d.\n", reg[2]);
			otp_times = 3;
			break;
	}

	*p_vcom1_otp_times = otp_times;
	*p_vcom2_otp_times = otp_times;
	
	printf("hi_8394_get_vcom_otp_times: mipi_channel: %d, vcom otp times: %d. \n", mipi_channel, 
			*p_vcom1_otp_times);
	
	return 0;
}

int hi_8394_read_vcom(int mipi_channel, int index, unsigned int *p_vcom)
{
	unsigned char reg[3] = { 0 };

	// enable extend command
	hi_8394_enable_extend_cmd(mipi_channel, 1);
	
	ReadModReg(mipi_channel, 1, 0xB6, 3, reg);
	printf("hi_8394_read_vcom: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

	if (index == 0)
	{
		//*p_vcom = reg[0] | ((reg[3] & 0x01) << 8);
		*p_vcom = reg[0];
	}
	else if (index == 1)
	{
		//*p_vcom = reg[1] | ((reg[3] & 0x02) << 8);
		*p_vcom = reg[1];
	}
	else
	{
		printf("hi_8394_read_vcom error: invalid index = %d.\n", index);
	}
	
	printf("hi_8394_read_vcom: mipi_channel: %d, vcom otp value: %#x. \n", mipi_channel, 
			*p_vcom);
	return 0;
}

int hi_8394_write_vcom(int mipi_channel, unsigned int vcom_value)
{
	unsigned char reg[3] = { 0 };

	reg[0] = vcom_value & 0xFF;
	reg[1] = vcom_value & 0xFF;

	if (vcom_value & 0x100)
	{
		reg[2] |= 0x01;
		reg[2] |= 0x02;
	}
	
	// enable extend command
	hi_8394_enable_extend_cmd(mipi_channel, 1);

	DCSLongWriteWithPara(mipi_channel, 0xB6, reg, sizeof(reg));
	
	return 0;
}

int hi_8394_read_nvm_vcom_z(int mipi_channel, int index, unsigned int* p_vcom_otp_value)
{
	// hw reset
	DCSShortWriteNoPara(mipi_channel, 0x01);

	// sleep out
	DCSShortWriteNoPara(mipi_channel, 0x11);

	usleep(120 * 1000);
	
	// enable extend command
	hi_8394_enable_extend_cmd(mipi_channel, 1);

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


static int hi_8394_read_nvm_vcom(int mipi_channel, int index, unsigned int* p_vcom_otp_value)
{
	return 0;
	
	// hw reset
	DCSShortWriteNoPara(mipi_channel, 0x01);

	// sleep out
	DCSShortWriteNoPara(mipi_channel, 0x11);

	usleep(120 * 1000);
	
	// enable extend command
	hi_8394_enable_extend_cmd(mipi_channel, 1);

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

int hi_8394_burn_vcom(int mipi_channel, unsigned int otp_vcom_value)
{
	int use_internal_vpp = 1;

	printf("hi_8394_burn_vcom: mipi_channel=%d, vcom=%d.\n", mipi_channel, otp_vcom_value);

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

static int hi_8394_select_d3g_bank_r(int mipi_channel)
{
	int code_reg = 0xBD;
	int bank = 0x00;
	mipi_send_code(mipi_channel, code_reg, &bank, 1);
}

static int hi_8394_select_d3g_bank_g(int mipi_channel)
{
	int code_reg = 0xBD;
	int bank = 0x01;
	mipi_send_code(mipi_channel, code_reg, &bank, 1);
}

static int hi_8394_select_d3g_bank_b(int mipi_channel)
{
	int code_reg = 0xBD;
	int bank = 0x02;
	mipi_send_code(mipi_channel, code_reg, &bank, 1);
}


int hx_8394_write_d3g_reg_2(int mipi_channel, unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
							unsigned char *p_d3g_g_reg_data, int d3g_g_reg_data_len,
							unsigned char *p_d3g_b_reg_data, int d3g_b_reg_data_len)
{
	unsigned char analog_gamma_code[1024 * 2] = { 0 };
	int code_offset = 3;
	int code_reg = 0xC1;
	unsigned char bank = 0x00;

	// R
	code_reg = 0xBD;
	bank = 0x00;
	mipi_send_code(mipi_channel, code_reg, &bank, 1);
	code_reg = 0xC1;
	mipi_send_code(mipi_channel, code_reg, p_d3g_r_reg_data, HX_8394_D3G_R_REG_LEN);

	// G
	code_reg = 0xBD;
	bank = 0x01;
	mipi_send_code(mipi_channel, code_reg, &bank, 1);
	code_reg = 0xC1;
	mipi_send_code(mipi_channel, code_reg, p_d3g_g_reg_data, HX_8394_D3G_G_REG_LEN);
	
	// B
	code_reg = 0xBD;
	bank = 0x02;
	mipi_send_code(mipi_channel, code_reg, &bank, 1);
	code_reg = 0xC1;
	mipi_send_code(mipi_channel, code_reg, p_d3g_b_reg_data, HX_8394_D3G_B_REG_LEN);
	return 0;
}
							

static int chip_hi_8394_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
								void* p_chip_data)
{
	unsigned char reg[3];
	ReadModReg(mipi_channel, 1, 0x04, 3, reg);
	printf("chip_hi_8394_get_chip_id: chip channel: %d, mipi_channel: %d, read id4: %02x:%02x:%02x. \n", 
				chip_channel, mipi_channel, reg[0], reg[1], reg[2]);

	if (reg[0] == 0x83 && reg[1] == 0x94 && reg[2] == 0x0f)
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
		printf("chip_hi_8394_get_chip_id: chip channel: %d, mipi_channel: %d, : read id failed!\n", 
				chip_channel, mipi_channel);
		
		return -1;
	}
	return -1;
}


static int chip_hi_8394_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{
	chip_hi_8394_get_chip_id(chip_channel, mipi_channel, s_hx_8394_private_info[chip_channel].id, 
								sizeof(s_hx_8394_private_info[chip_channel].id), p_chip_data);

	unsigned char* p_id = s_hx_8394_private_info[chip_channel].id;
	if ( (p_id[0] != 0x83) || (p_id[1] != 0x94) || (p_id[2] != 0x0F) )
	{
		printf("chip_hi_8394_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_hx_8394_private_info[chip_channel].id_ok = 0;

		return -1;
	}

	s_hx_8394_private_info[chip_channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	hi_8394_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);
	s_hx_8394_private_info[chip_channel].vcom1_otp_times = vcom1_otp_times;
	s_hx_8394_private_info[chip_channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	//hi_8394_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
	//hi_8394_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);

	hi_8394_read_vcom(mipi_channel, 0, &vcom1_otp_value);
	
	s_hx_8394_private_info[chip_channel].vcom1 = vcom1_otp_value;
	s_hx_8394_private_info[chip_channel].vcom2 = vcom1_otp_value;

	printf("chip_hi_8394_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", chip_channel, mipi_channel,
				s_hx_8394_private_info[chip_channel].vcom1_otp_times,
				s_hx_8394_private_info[chip_channel].vcom2_otp_times,
				s_hx_8394_private_info[chip_channel].vcom1,
				s_hx_8394_private_info[chip_channel].vcom2);
	return 0;
}

static int chip_hi_8394_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	if (s_hx_8394_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		hi_8394_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_hi_8394_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_hi_8394_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_hi_8394_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	if (s_hx_8394_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		hi_8394_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		// read vcom otp value
		int vcom1_otp_value = 0;
		int vcom2_otp_value = 0;
		hi_8394_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
		hi_8394_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);

		*p_otp_vcom_times = vcom1_otp_times;
		*p_otp_vcom_value = vcom1_otp_value;

		printf("chip_hi_8394_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_hi_8394_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}


static int chip_hi_8394_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{
	if (s_hx_8394_private_info[chip_channel].id_ok == 1)
	{
		int val = 0;
		#ifdef ENABlE_OTP_BURN
		val = hi_8394_burn_vcom(mipi_channel, otp_vcom_value);
		#else
		printf("chip_hi_8394_write_chip_vcom_otp_value: *** do nothing ***\n");
		#endif
		return val;
	}

	printf("chip_hi_8394_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_hi_8394_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	if (s_hx_8394_private_info[chip_channel].id_ok == 1)
	{
		int vcom = 0;
		hi_8394_read_vcom(mipi_channel, 0, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_hi_8394_read_vcom: end\n");

		return 0;
	}
	printf("chip_hi_8394_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_hi_8394_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	if (s_hx_8394_private_info[chip_channel].id_ok == 1)
	{
		if (s_hx_8394_private_info[chip_channel].vcom1_otp_times == 0)
		{
			//chip_hi_8394_write_vcom(chip_channel, mipi_channel, vcom_value, p_chip_data)
			hi_8394_write_vcom(mipi_channel, vcom_value);
		}
		else
		{
			hi_8394_write_vcom(mipi_channel, vcom_value);
		}
		
		//printf("chip_hi_8394_write_vcom: end\n");

		return 0;
	}
	
	printf("chip_hi_8394_write_vcom error: unknow chip!\n");
	
	return -1;
}

static void* chip_hi_8394_get_and_reset_private_data(int chip_channel)
{
	printf("chip_hi_8394_get_and_reset_private_data.\n");
	s_hx_8394_private_info[chip_channel].id_ok = 0;
	memset(s_hx_8394_private_info[chip_channel].id, 0, sizeof(s_hx_8394_private_info[chip_channel].id));
	s_hx_8394_private_info[chip_channel].vcom1 = 0;
	s_hx_8394_private_info[chip_channel].vcom1_otp_times = 0;
	s_hx_8394_private_info[chip_channel].vcom2 = 0;
	s_hx_8394_private_info[chip_channel].vcom2_otp_times = 0;
	
	return &s_hx_8394_private_info[chip_channel];
}


static int chip_hi_8394_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	// check id 
	int val = chip_hi_8394_check_chip_ok(chip_channel, mipi_channel, p_chip_data);
	if (val != 0)
	{
		printf("chip_hi_8394_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_hx_8394_private_info[chip_channel].vcom1;

	#if 1
	// check times
	if (s_hx_8394_private_info[chip_channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_hi_8394_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_hx_8394_private_info[chip_channel].vcom1_otp_times);

		#ifdef ENABlE_OTP_BURN
		//return E_ICM_OK;
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}
	#endif

	// check vcom value.
	if (s_hx_8394_private_info[chip_channel].vcom1 != vcom_value)
	{
		printf("chip_hi_8394_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_hx_8394_private_info[chip_channel].vcom1);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
		//return E_ICM_OK;
		#else
		return E_ICM_OK;
		#endif
	}

	return E_ICM_OK;
}

// gamma
static int chip_hi_8394_write_analog_reg(int mipi_channel, unsigned char *p_analog_gamma_reg_data, 
								int analog_gamma_data_len)
{
	unsigned char analog_gamma_code[1024 * 2] = { 0 };
	int code_offset = 3;
	int code_reg = HX_8394_ANALOG_GAMMA_REG;

	mipi_to_lp_mode(mipi_channel);
	
	mipi_send_code(mipi_channel, code_reg, p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);
	
	mipi_to_hs_mode(mipi_channel);
	return 0;
}

int hx_8394_digital_gamma_control(int mipi_channel, unsigned char value)
{
	unsigned char analog_gamma_code[1024 * 2] = { 0 };

	mipi_to_lp_mode(mipi_channel);	
	hi_8394_enable_extend_cmd(mipi_channel, 1);
	
	hi_8394_select_d3g_bank_r(mipi_channel);

	int code_offset = 3;
	int code_reg = HX_8394_D3G_BANK_REG;
	mipi_send_code(mipi_channel, code_reg, &value, 1);
	
	mipi_to_hs_mode(mipi_channel);
	return 0;
}

int hx_8394_write_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
										unsigned char *p_analog_gamma_reg_data,
										int analog_gamma_reg_data_len, void* p_chip_data)
{
}

static unsigned char s_analog_gamma_reg_data[HX_8394_ANALOG_GAMMA_LEN] = 
{ 
	0x00, 0x08, 0x16, 0x20, 0x24, 0x2a, 0x30, 0x30, 0x66, 0x7b, 0x8e, 0x8d, 0x94, 0xa1, 0xa2, 0xa2, 
	0xab, 0xad, 0xab, 0xb7, 0xc6, 0x63, 0x61, 0x65, 0x68, 0x6c, 0x73, 0x7b, 0x7f, 0x00, 0x08, 0x15, 
	0x20, 0x24, 0x2a, 0x30, 0x30, 0x66, 0x7b, 0x8c, 0x8b, 0x93, 0xa2, 0xa0, 0xa1, 0xac, 0xae, 0xab, 
	0xb9, 0xc9, 0x61, 0x60, 0x64, 0x68, 0x6c, 0x73, 0x7b, 0x7f
};
	
int hx_8394_write_fix_analog_gamma_reg_data(int chip_channel, int mipi_channel, void* p_chip_data)
{
}

int hx_8394_read_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
										unsigned char *p_analog_gamma_reg_data,
										int analog_gamma_reg_data_len, void* p_chip_data)
{
}

static int hx_8394_write_d3g_control(int chip_channel, int mipi_channel, int enable, void* p_chip_data)
{
}
										
int hx_8394_write_analog_digital_gamma_reg_data(int chip_channel, int mipi_channel, 										
										unsigned char *p_digital_gamma_reg_data,
										int digital_gamma_reg_data_len, void* p_chip_data)
{
}

static int hx_8394_write_fix_digital_gamma_reg_data(int chip_channel, int mipi_channel, void* p_chip_data)
{
}

int hx_8394_burn_gamma_data_2nd(int mipi_channel,  unsigned int vcom,  int burn_vcom,
									unsigned char *p_analog_gamma_reg, int analog_gamma_reg_len,
									unsigned char *p_d3g_r_reg, int d3g_r_reg_len,
									unsigned char *p_d3g_g_reg, int d3g_g_reg_len,
									unsigned char *p_d3g_b_reg, int d3g_b_reg_len)
{
	int use_internal_vpp = 1;	

	printf("hx_8394_burn_gamma_data_2nd: mipi_channel=%d.\n", mipi_channel);

	mipi_to_lp_mode(mipi_channel);
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

	if (burn_vcom)
	{
		// vcom
		unsigned char vcom_value[100] = 
		{
			0x39, 0xB6, 0x03, 0x00, 0x00, 0x00,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		vcom_value[3] = vcom;
		vcom_value[4] = vcom;
		SendCode(mipi_channel, vcom_value);
	}

	// gamma	
	chip_hi_8394_write_analog_reg(mipi_channel, p_analog_gamma_reg, analog_gamma_reg_len);

	// d3g
	hx_8394_write_d3g_reg_2(mipi_channel, p_d3g_r_reg, d3g_r_reg_len, p_d3g_g_reg, d3g_g_reg_len,
								p_d3g_b_reg, d3g_b_reg_len);
	delay_ms(100);
	
	// otp key
	#ifdef ENABlE_OTP_BURN
	unsigned char otp_key[100] = 
	{
		0x39, 0xBB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x55,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_key);
	#endif

	if (burn_vcom)
	{
	#ifdef ENABlE_OTP_BURN
		// vcom
		unsigned char vcom_otp_power[100] = 
		{
			0x39, 0xBB, 0x02, 0x80, 0x0D,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		SendCode(mipi_channel, vcom_otp_power);

		unsigned char vcom_otp_power2[100] = 
		{
			0x39, 0xBB, 0x04, 0x80, 0x0D, 0x00, 0x01,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		SendCode(mipi_channel, vcom_otp_power2);
	#endif
	}
	
	delay_ms(550);

	// second analog gamma
	#ifdef ENABlE_OTP_BURN
	unsigned char gamma_otp_power[100] = 
	{
		0x39, 0xBB, 0x04, 0x81, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};		
	SendCode(mipi_channel, gamma_otp_power);

	unsigned char gamma_otp_power2[100] = 
	{
		0x39, 0xBB, 0x04, 0x81, 0x00, 0x00, 0x01,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, gamma_otp_power2);
	#endif
	
	delay_ms(700);

	// d3g
	#ifdef ENABlE_OTP_BURN
	unsigned char dgc_otp_power[100] = 
	{
		0x39, 0xBB, 0x04, 0x81, 0x3A, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};		
	SendCode(mipi_channel, dgc_otp_power);

	unsigned char dgc_otp_power2[100] = 
	{
		0x39, 0xBB, 0x04, 0x81, 0x3A, 0x00, 0x01,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, dgc_otp_power2);
	#endif
	
	delay_ms(1400);

	// gamma replacement otp
	#ifdef ENABlE_OTP_BURN
	unsigned char gamma_replace_otp_cmd[100] = 
	{
		0x39, 0xBB, 0x04, 0x80, 0xFF, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};		
	SendCode(mipi_channel, gamma_replace_otp_cmd);

	unsigned char gamma_replace_otp_cmd2[100] = 
	{
		0x39, 0xBB, 0x04, 0x80, 0xFF, 0x00, 0x01,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, gamma_replace_otp_cmd2);
	#endif

	#ifdef ENABlE_OTP_BURN
	unsigned char otp_end[100] = 
	{
		0x39, 0xBB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_end);
	#endif
	
	SendCode(mipi_channel, passwd);
	DCSShortWriteNoPara(mipi_channel, 0x10);
	
	mipi_to_hs_mode(mipi_channel);
	return 0;
}


int hx_8394_burn_vcom_and_gamma_data(int mipi_channel, unsigned int vcom,  int burn_vcom,
 										unsigned char *p_analog_gamma_reg, int analog_gamma_reg_len,
										unsigned char *p_d3g_r_reg, int d3g_r_reg_len,
										unsigned char *p_d3g_g_reg, int d3g_g_reg_len,
										unsigned char *p_d3g_b_reg, int d3g_b_reg_len)
{
	int use_internal_vpp = 1;	

	printf("hx_8394_burn_vcom_and_gamma_data: mipi_channel=%d, vcom=%d.\n", mipi_channel, vcom);

	mipi_to_lp_mode(mipi_channel);
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

	if (burn_vcom)
	{
		// vcom
		unsigned char vcom_value[100] = 
		{
			0x39, 0xB6, 0x03, 0x00, 0x00, 0x00,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		vcom_value[3] = vcom;
		vcom_value[4] = vcom;
		SendCode(mipi_channel, vcom_value);
	}

	// gamma	
	chip_hi_8394_write_analog_reg(mipi_channel, p_analog_gamma_reg, analog_gamma_reg_len);

	// d3g
	hx_8394_write_d3g_reg_2(mipi_channel, p_d3g_r_reg, d3g_r_reg_len, p_d3g_g_reg, d3g_g_reg_len,
								p_d3g_b_reg, d3g_b_reg_len);

	delay_ms(100);
	
	// otp key
	#ifdef ENABlE_OTP_BURN
	unsigned char otp_key[100] = 
	{
		0x39, 0xBB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x55,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_key);
	#endif

	if (burn_vcom)
	{
	#ifdef ENABlE_OTP_BURN
		// vcom
		unsigned char vcom_otp_power[100] = 
		{
			0x39, 0xBB, 0x02, 0x80, 0x0D,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		SendCode(mipi_channel, vcom_otp_power);

		unsigned char vcom_otp_power2[100] = 
		{
			0x39, 0xBB, 0x04, 0x80, 0x0D, 0x00, 0x01,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};
		SendCode(mipi_channel, vcom_otp_power2);
	#endif
	}
	
	delay_ms(550);

	// analog gamma
	#ifdef ENABlE_OTP_BURN
	unsigned char gamma_otp_power[100] = 
	{
		0x39, 0xBB, 0x02, 0x80, 0x45,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, gamma_otp_power);

	unsigned char gamma_otp_power2[100] = 
	{
		0x39, 0xBB, 0x04, 0x80, 0x45, 0x00, 0x01,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, gamma_otp_power2);
	#endif
	
	delay_ms(700);

	// d3g
	#ifdef ENABlE_OTP_BURN
	unsigned char dgc_otp_power[100] = 
	{
		0x39, 0xBB, 0x02, 0x80, 0x7F,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, dgc_otp_power);


	unsigned char dgc_otp_power2[100] = 
	{
		0x39, 0xBB, 0x04, 0x80, 0x7F, 0x00, 0x01,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, dgc_otp_power2);
	#endif
	
	delay_ms(1400);

	#ifdef ENABlE_OTP_BURN
	unsigned char otp_end[100] = 
	{
		0x39, 0xBB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	SendCode(mipi_channel, otp_end);
	#endif
	
	SendCode(mipi_channel, passwd);
	DCSShortWriteNoPara(mipi_channel, 0x10);
	
	mipi_to_hs_mode(mipi_channel);
	return 0;
}


static int hx_8394_burn_gamma_otp_values(int chip_channel, int mipi_channel, int burn_flag,
											int enable_burn_vcom, int vcom, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data)
{
}

static int hx_8394_check_gamma_otp_values(int chip_channel, int mipi_channel, 
											int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data)
{
}

static int hx_8394_get_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
											unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len,
											void* p_chip_data)
{
}

static int hx_8394_get_digital_gamma_reg_data(int chip_channel, int mipi_channel, 
											unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len,
											void* p_chip_data)
{
}


icm_chip_t chip_hi_8394 = 
{
	.my_chip_id = E_MY_CHIP_ID_HI_8384,

	.fn_get_and_reset_private_data = chip_hi_8394_get_and_reset_private_data,
	
	.fn_get_chip_id = chip_hi_8394_get_chip_id,
	.fn_check_chip_ok = chip_hi_8394_check_chip_ok,
	.fn_read_chip_vcom_opt_times = chip_hi_8394_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = chip_hi_8394_read_chip_vcom_opt_info,
	
	.fn_read_vcom = chip_hi_8394_read_vcom,
	.fn_write_vcom = chip_hi_8394_write_vcom,
	
	.fn_write_chip_vcom_otp_value = chip_hi_8394_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = chip_hi_8394_check_vcom_otp_burn_ok,

	// gamma
	.fn_write_chip_analog_gamma_reg_data = hx_8394_write_analog_gamma_reg_data,
	.fn_read_chip_analog_gamma_reg_data = hx_8394_read_analog_gamma_reg_data,
	.fn_write_chip_fix_analog_gamma_reg_data = hx_8394_write_fix_analog_gamma_reg_data,
	
	.fn_chip_d3g_control = hx_8394_write_d3g_control,
	.fn_write_chip_digital_gamma_reg_data = hx_8394_write_analog_digital_gamma_reg_data,
	.fn_write_chip_fix_digital_gamma_reg_data = hx_8394_write_fix_digital_gamma_reg_data,

	.fn_chip_burn_gamma_otp_values = hx_8394_burn_gamma_otp_values,
	.fn_chip_check_gamma_otp_values = hx_8394_check_gamma_otp_values,
	
	.fn_chip_get_analog_gamma_reg_data = hx_8394_get_analog_gamma_reg_data,
	.fn_chip_get_digital_gamma_reg_data = hx_8394_get_digital_gamma_reg_data,
};


