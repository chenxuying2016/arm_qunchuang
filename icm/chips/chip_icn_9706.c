
#include <stdio.h>


#include "../ic_manager.h"


#include "chip_icn_9706.h"

#define ICN_9706_ANALOG_GAMMA_LEN		(38)

#define ICN_9706_D3G_COMPONENT_NUMS		(3)
#define ICN_9706_D3G_REG_LEN			(26)

#define ICN_9706_ANALOG_GAMMA_REG		(0xC8)


#define ICN_9706_ENABLE_OTP		(1)

typedef struct tag_icn_9706_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;

	unsigned char analog_gamma_reg_data[ICN_9706_ANALOG_GAMMA_LEN];
	unsigned char digital_gamma_reg_data[ICN_9706_D3G_COMPONENT_NUMS][ICN_9706_D3G_REG_LEN];
}icn_9706_info_t;

static icn_9706_info_t s_icn_9706_private_info[MAX_CHIP_CHANNEL_NUMS] = { 0 };

int icn_9706_enable_level_2_cmd(int mipi_channel)
{
	#if 0
	unsigned char passwd1[2] = { 0x5A, 0x5A };
	unsigned char passwd2[2] = { 0xA5, 0xA5 };
	
	DCSLongWriteWithPara(mipi_channel, 0xF0, passwd1, 2);
	DCSLongWriteWithPara(mipi_channel, 0xF1, passwd2, 2);
	#else
	unsigned char cmd[100] = {
				0x39, 0xF0, 0x02, 0x5A, 0x5A,
				0x39, 0xF1, 0x02, 0xA5, 0xA5,
				0x39, 0xF0, 0x02, 0xB4, 0x4B,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff
			};

	SendCode(mipi_channel, cmd);
	#endif
	return 0;
}

int icn_9706_get_vcom_otp_times(int mipi_channel, unsigned char *p_vcom1_otp_times, 
										unsigned char *p_vcom2_otp_times)
{
	icn_9706_enable_level_2_cmd(mipi_channel);

	#if 0
	// F0	==> otp read/write enable
	unsigned password1[2] = { 0xB4, 0x4B };
	DCSLongWriteWithPara(mipi_channel, 0xF0, password1, sizeof(password1));
	#endif

	unsigned char reg[3] = { 0 };
	ReadModReg(mipi_channel, 1, 0xB6, 3, reg);
	printf("icn_9706_get_vcom_otp_times: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

	*p_vcom1_otp_times = reg[2];
	*p_vcom2_otp_times = reg[2];
	
	return 0;
}

int icn_9706_read_vcom(int mipi_channel, int index, unsigned int *p_vcom)
{
	icn_9706_enable_level_2_cmd(mipi_channel);

	#if 0
	// F0	==> otp read/write enable
	unsigned password1[2] = { 0xB4, 0x4B };
	DCSLongWriteWithPara(mipi_channel, 0xF0, password1, sizeof(password1));
	#endif

	unsigned char reg[3] = { 0 };
	ReadModReg(mipi_channel, 1, 0xB6, 3, reg);

	if (index == 0)
		*p_vcom = reg[0];
	else if (index == 1)
		*p_vcom = reg[1];
	else
	{
		printf("icn_9706_read_vcom: invalid index = %d.\n", index);
	}
	
	return 0;
}

static int icn_9706_write_vcom(int mipi_channel, unsigned int vcom_value)
{	
	icn_9706_enable_level_2_cmd(mipi_channel);

	#if 0
	unsigned char reg[2] = { 0 };
	reg[0] = vcom_value;
	reg[1] = vcom_value;

	printf("icn_9706_write_vcom: %02x\n", reg[0]);
	DCSLongWriteWithPara(mipi_channel, 0xB6, reg, 2);
	#else
	unsigned char vcom_code[] = {
								0x39, 0xB6, 0x02, vcom_value, vcom_value,
								0xFF, 0xFF, 0xFF, 0xFF
								};
	SendCode(mipi_channel, vcom_code);
	#endif
	
	//unsigned int vcom = 0;
	//icn_9706_read_vcom(mipi_channel, 0, &vcom);
	//printf("read vcom: %02x.\n", vcom);
	return 0;
}


int icn_9706_read_nvm_vcom(int mipi_channel, int index, unsigned int *p_vcom_otp_value)
{
	icn_9706_enable_level_2_cmd(mipi_channel);

	unsigned char reg[4] = { 0 };
	ReadModReg(mipi_channel, 1, 0xB6, 4, reg);

	printf("icn_9706_read_nvm_vcom: %02x, %02x, %02x, %02x.\n", reg[0], reg[1], reg[2], reg[3]);
	
	if (index == 0)
		*p_vcom_otp_value = reg[0];
	else if (index == 1)
		*p_vcom_otp_value = reg[1];
	else
	{
		printf("icn_9706_read_nvm_vcom: invalid index = %d.\n", index);
	}
	
	return 0;
}

// OTP Bank:
//	0: ID;
//	1: VCOM;
//	2: Digigal Gamma;
//	3: Analog Gamma;
int icn_9706_burn_vcom(int mipi_channel, int otp_vcom_value)
{
	
	mipi_to_lp_mode(mipi_channel);

	icn_9706_write_vcom(mipi_channel, otp_vcom_value);

	usleep(100 * 1000);

	#if ICN_9706_ENABLE_OTP
	unsigned char otp_passwd_[100] = {
				0x39, 0xF0, 0x02, 0x5A, 0x5A,
				0x39, 0xF1, 0x02, 0xA5, 0xA5,
				0x39, 0xF0, 0x02, 0xB4, 0x4B,
				0x39, 0xCB, 0x03, 0x02, 0x01, 0x01,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff
			};

	SendCode(mipi_channel, otp_passwd_);
	#endif
	
	usleep(100 * 1000);
	
	unsigned int read_vcom = 0;
	icn_9706_read_vcom(mipi_channel, 0, &read_vcom);

	printf("icn_9706_read_vcom: %d.\n", read_vcom);

	//mipi_to_hs_mode(mipi_channel);
	
	return 0;	
}

static int icn_9706_burn_gamma_data(int mipi_channel, unsigned char *p_analog_gamma_data, int analog_gamma_data_len,
										unsigned char *p_dgc_data_r, int dgc_data_r_len, 
										unsigned char *p_dgc_data_g, int dgc_data_g_len,
										unsigned char *p_dgc_data_b, int dgc_data_b_len )
{
	mipi_to_lp_mode(mipi_channel);

	// write analog gamma
	mipi_send_code(mipi_channel, 0xC8, p_analog_gamma_data, analog_gamma_data_len);

	// write dgc_control
	unsigned char dgc_ctrl = 0x03;
	mipi_send_code(mipi_channel, 0xE3, &dgc_ctrl, sizeof(dgc_ctrl));

	// write digital gamma 
	// r 
	mipi_send_code(mipi_channel, 0xE4, p_dgc_data_r, dgc_data_r_len);
	
	// g
	mipi_send_code(mipi_channel, 0xE5, p_dgc_data_g, dgc_data_g_len);
	
	// b
	mipi_send_code(mipi_channel, 0xE6, p_dgc_data_b, dgc_data_b_len);

	usleep(1000 * 1000);

	#if ICN_9706_ENABLE_OTP
	unsigned char otp_passwd_[100] = {
				0x39, 0xF0, 0x02, 0x5A, 0x5A,
				0x39, 0xF1, 0x02, 0xA5, 0xA5,
				0x39, 0xF0, 0x02, 0xB4, 0x4B,
				0x39, 0xCB, 0x03, 0x0C, 0x01, 0x01,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff
			};

	SendCode(mipi_channel, otp_passwd_);
	#endif
	
	usleep(2 * 1000 * 1000);

	//mipi_to_hs_mode(mipi_channel);
	return 0;
}

 int icn_9706_burn_vcom_and_gamma_data(int mipi_channel, int otp_vcom_value, int burn_vcom,
										unsigned char *p_analog_gamma_data, int analog_gamma_data_len,
										unsigned char *p_dgc_data_r, int dgc_data_r_len, 
										unsigned char *p_dgc_data_g, int dgc_data_g_len,
										unsigned char *p_dgc_data_b, int dgc_data_b_len )
{
	mipi_to_lp_mode(mipi_channel);

	if (burn_vcom)
	{
		icn_9706_write_vcom(mipi_channel, otp_vcom_value);
	}

	// write analog gamma
	mipi_send_code(mipi_channel, 0xC8, p_analog_gamma_data, analog_gamma_data_len);

	// write dgc_control
	unsigned char dgc_ctrl = 0x03;
	mipi_send_code(mipi_channel, 0xE3, &dgc_ctrl, sizeof(dgc_ctrl));

	// write digital gamma 
	// r 
	mipi_send_code(mipi_channel, 0xE4, p_dgc_data_r, dgc_data_r_len);
	
	// g
	mipi_send_code(mipi_channel, 0xE5, p_dgc_data_g, dgc_data_g_len);
	
	// b
	mipi_send_code(mipi_channel, 0xE6, p_dgc_data_b, dgc_data_b_len);

	usleep(150 * 1000);
	
	#if ICN_9706_ENABLE_OTP
	unsigned char otp_passwd_[100] = {
				0x39, 0xF0, 0x02, 0x5A, 0x5A,
				0x39, 0xF1, 0x02, 0xA5, 0xA5,
				0x39, 0xF0, 0x02, 0xB4, 0x4B,
				0x39, 0xCB, 0x03, 0x0E, 0x01, 0x01,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff
			};

	SendCode(mipi_channel, otp_passwd_);
	#endif
	
	usleep(2 * 1000 * 1000);

	if (burn_vcom)
	{
		unsigned int read_vcom = 0;
		icn_9706_read_vcom(mipi_channel, 0, &read_vcom);
		printf("icn_9706_read_vcom: %d.\n", read_vcom);
	}
	
	mipi_to_hs_mode(mipi_channel);
	return 0;
}



#if 0
void icn_9706_otp_test()
{
	int mipi_channel = 1;

	int otp_vcom_value = 0x11;

	unsigned char analog_gamma_data[38] = { 1, 2, 3, 4 };
	int analog_gamma_data_len = 38;

	unsigned char dgc_r_data[26] = { 5, 6, 7, 8 };
	int dgc_r_data_len = 26;

	unsigned char dgc_g_data[26] = { 9, 0x0A, 0x0B, 0x0C };
	int dgc_g_data_len = 26;

	unsigned char dgc_b_data[26] = { 0x0D, 0x0E, 0x0F, 0x10 };
	int dgc_b_data_len = 26;
	
	icn_9706_burn_vcom_and_gamma_data(mipi_channel, otp_vcom_value, analog_gamma_data, analog_gamma_data_len,
										dgc_r_data, dgc_r_data_len, dgc_g_data, dgc_g_data_len,
										dgc_b_data, dgc_b_data_len);

	mipi_to_hs_mode(mipi_channel);
}
#endif

static int chip_icn_9706_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
								void* p_chip_data)
{
	char reg[3] = { 0 };	

	// read id4
	//ReadModReg(mipi_channel, 1, 0x04, 3, reg);
    ReadModReg(mipi_channel, 1, 0x0A, 3, reg);
	printf("chip_icn_9706_get_chip_id: chip channel: %d, mipi_channel: %d, read id: %02x:%02x:%02x. \n", 
				chip_channel, mipi_channel, reg[2], reg[1], reg[0]);

	#if 1
	if (reg[0] == 0x9C)
	{
		p_id_data[0] = reg[0];
		p_id_data[1] = reg[0];
		p_id_data[2] = reg[0];
		return 0;
	}
	#else
	//if (reg[0] == 0x00 && reg[1] == 0x80 && reg[2] == 0x00)
	if (reg[0] == 0x00 && reg[1] == 0x00 && reg[2] == 0x00)
	{
		if (p_id_data && id_data_len >= 3)
		{
			p_id_data[0] = reg[0];
			p_id_data[1] = reg[1];
			p_id_data[2] = reg[2];
			//return 3;
			return 0;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		printf("chip_icn_9706_get_chip_id: chip channel: %d, mipi_channel: %d, read id failed!\n", 
				chip_channel, mipi_channel);
		
		return -1;
	}
	#endif
	
	return -1;
}


static int chip_icn_9706_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{
	// read and check id;
	chip_icn_9706_get_chip_id(chip_channel, mipi_channel, s_icn_9706_private_info[chip_channel].id, 
								sizeof(s_icn_9706_private_info[chip_channel].id), p_chip_data);

	unsigned char* p_id = s_icn_9706_private_info[chip_channel].id;
	//if ( (p_id[0] != 0x00) || (p_id[1] != 0x80) || (p_id[2] != 0x00) )
	//if ( (p_id[0] != 0x00) || (p_id[1] != 0x00) || (p_id[2] != 0x00) )
	if ( (p_id[0] != 0x9C) || (p_id[1] != 0x9C) || (p_id[2] != 0x9C) )
	{
		printf("chip_icn_9706_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_icn_9706_private_info[chip_channel].id_ok = 0;

		return -1;
	}

	s_icn_9706_private_info[chip_channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	icn_9706_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);
	s_icn_9706_private_info[chip_channel].vcom1_otp_times = vcom1_otp_times;
	s_icn_9706_private_info[chip_channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	icn_9706_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
	icn_9706_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);
	s_icn_9706_private_info[chip_channel].vcom1 = vcom1_otp_value;
	s_icn_9706_private_info[chip_channel].vcom2 = vcom2_otp_value;

	printf("chip_icn_9706_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", chip_channel, mipi_channel,
				s_icn_9706_private_info[chip_channel].vcom1_otp_times,
				s_icn_9706_private_info[chip_channel].vcom2_otp_times,
				s_icn_9706_private_info[chip_channel].vcom1,
				s_icn_9706_private_info[chip_channel].vcom2);
	return 0;
}

static int chip_icn_9706_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	if (s_icn_9706_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		icn_9706_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_icn_9706_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_icn_9706_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_icn_9706_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	if (s_icn_9706_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		icn_9706_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		unsigned int vcom = 0;
		icn_9706_read_vcom(mipi_channel, 0, &vcom);

		*p_otp_vcom_value = vcom;

		printf("chip_icn_9706_read_chip_vcom_opt_info: vcom = %d end\n", *p_otp_vcom_value);

		return 0;
	}
	
	printf("chip_icn_9706_read_chip_vcom_opt_info error!\n");
	return -1;
}


static int chip_icn_9706_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{
	if (s_icn_9706_private_info[chip_channel].id_ok == 1)
	{
		int val = 0;
		#ifdef ENABlE_OTP_BURN
		val = icn_9706_burn_vcom(mipi_channel, otp_vcom_value);
		#else
		printf("chip_icn_9706_write_chip_vcom_otp_value: *** do nothing ***\n");
		#endif
		return val;
	}

	printf("chip_icn_9706_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_icn_9706_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	if (s_icn_9706_private_info[chip_channel].id_ok == 1)
	{
		int vcom = 0;
		icn_9706_read_vcom(mipi_channel, 0, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_icn_9706_read_vcom: end\n");

		return 0;
	}
	printf("chip_icn_9706_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_icn_9706_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	if (s_icn_9706_private_info[chip_channel].id_ok == 1)
	{
		if (s_icn_9706_private_info[chip_channel].vcom1_otp_times == 0)
		{
			//chip_ili_9806e_write_vcom(chip_channel, mipi_channel, vcom_value, p_chip_data)
			icn_9706_write_vcom(mipi_channel, vcom_value);
		}
		else
		{
			icn_9706_write_vcom(mipi_channel, vcom_value);
		}
		
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_write_vcom error: unknow chip!\n");
	
	return -1;
}

static void* chip_icn_9706_get_and_reset_private_data(int chip_channel)
{
	printf("chip_icn_9706_get_and_reset_private_data.\n");
	s_icn_9706_private_info[chip_channel].id_ok = 0;
	memset(s_icn_9706_private_info[chip_channel].id, 0, 
			sizeof(s_icn_9706_private_info[chip_channel].id));
	s_icn_9706_private_info[chip_channel].vcom1 = 0;
	s_icn_9706_private_info[chip_channel].vcom1_otp_times = 0;
	s_icn_9706_private_info[chip_channel].vcom2 = 0;
	s_icn_9706_private_info[chip_channel].vcom2_otp_times = 0;
	memset(s_icn_9706_private_info[chip_channel].analog_gamma_reg_data, 1, 
			sizeof(s_icn_9706_private_info[chip_channel].analog_gamma_reg_data));
	memset(s_icn_9706_private_info[chip_channel].digital_gamma_reg_data, 2, 
			sizeof(s_icn_9706_private_info[chip_channel].digital_gamma_reg_data));
	 
	return &s_icn_9706_private_info[chip_channel];
}


static int chip_icn_9706_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	printf("chip_icn_9706_check_vcom_otp_burn_ok: last times: %d. last vcom: %d. \n", last_otp_times, vcom_value);
	// check id 
	int val = chip_icn_9706_check_chip_ok(chip_channel, mipi_channel, p_chip_data);
	if (val != 0)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_icn_9706_private_info[chip_channel].vcom1;
	
	#if 1
	// check times
	if (s_icn_9706_private_info[chip_channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_icn_9706_private_info[chip_channel].vcom1_otp_times);

	#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	#else
		return E_ICM_OK;
	#endif
	}
	#endif
	
	// check vcom value.
	if (s_icn_9706_private_info[chip_channel].vcom1 != vcom_value)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_icn_9706_private_info[chip_channel].vcom1);

	#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	#else
		return E_ICM_OK;
	#endif
	}
	
	return E_ICM_OK;
}


// gamma
static unsigned char s_icn_9706_default_analog_gamma_code[ICN_9706_ANALOG_GAMMA_LEN] = 
					{ 
						0x7c, 0x57, 0x41, 0x31, 0x2a, 0x1a, 0x1f, 0x0b, 
						0x28, 0x2a, 0x2c, 0x4d, 0x3d, 0x47, 0x3b, 0x3a, 
						0x2e, 0x1b, 0x06, 
						0x7c, 0x57, 0x41, 0x31, 0x2a, 0x1a, 0x1f, 0x0b, 
						0x28, 0x2a, 0x2c, 0x4d, 0x3d, 0x47, 0x3b, 0x3a, 
						0x2e, 0x1b, 0x06
					}; 

int icn_9706_write_analog_reg(int mipi_channel, unsigned char *p_analog_gamma_reg_data, 
								int analog_gamma_data_len)
{
	unsigned char analog_gamma_code[1024 * 2] = { 0 };
	int code_offset = 3;
	int code_reg = 0xC8;
	//int code_len = ANALOG_GAMMA_REG_LEN;

	#if 0
	unsigned char init_analog_gamma_code[] = 
	{
		0x7c, 0x61, 0x4f, 0x40, 0x3a, 0x2a, 0x2e, 0x1a, 0x36, 0x38, 0x3a, 0x5a, 0x49, 0x51, 0x45, 0x44,
		0x39, 0x2c, 0x06, 0x7c, 0x61, 0x4f, 0x40, 0x3a, 0x2a, 0x2e, 0x1a, 0x36, 0x38, 0x3a, 0x5a, 0x49,
		0x51, 0x45, 0x44, 0x39, 0x2c, 0x06
	};
	#endif
		
	mipi_send_code(mipi_channel, code_reg, p_analog_gamma_reg_data, analog_gamma_data_len);
	return 0;
}

static int icn_9706_digital_gamma_control(int mipi_channel, unsigned char value)
{
	unsigned char analog_gamma_code[1024 * 2] = { 0 };
	int code_offset = 3;
	int code_reg = 0xE3;
	
	mipi_send_code(mipi_channel, code_reg, &value, 1);

	return 0;
}

static int icn_9706_write_d3g_reg(int mipi_channel, unsigned char reg, unsigned char *p_d3g_reg_data, int d3g_reg_data_len)
{
	unsigned char analog_gamma_code[1024 * 2] = { 0 };
	int code_offset = 3;
	//int code_reg = 0xC8;
	mipi_send_code(mipi_channel, reg, p_d3g_reg_data, d3g_reg_data_len);
	//mipi_send_code2(mipi_channel, reg, p_d3g_reg_data, d3g_reg_data_len);
	return 0;
}

static int chip_icn_9706_write_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
				unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_len, void* p_chip_data)
{
	icn_9706_info_t* p_chip_info = (icn_9706_info_t*)p_chip_data;

	memcpy(p_chip_info->analog_gamma_reg_data, p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	mipi_to_lp_mode(mipi_channel);
	
	icn_9706_enable_level_2_cmd(mipi_channel);	
	
	icn_9706_write_analog_reg(mipi_channel, p_chip_info->analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	mipi_to_hs_mode(mipi_channel);
	
	return 0;
}

static int chip_icn_9706_read_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
				unsigned char *p_analog_gamma_reg_data_buffer, int analog_gamma_reg_len, void* p_chip_data)
{
	mipi_to_lp_mode(mipi_channel);
	
	icn_9706_enable_level_2_cmd(mipi_channel);				

	ReadModReg(mipi_channel, 1, ICN_9706_ANALOG_GAMMA_REG, ICN_9706_ANALOG_GAMMA_LEN, p_analog_gamma_reg_data_buffer);
	mipi_to_hs_mode(mipi_channel);
	
	printf("chip_icn_9706_read_analog_gamma_reg_data: read alanog reg: 0x%02x, len: 0x%02x.\n", p_analog_gamma_reg_data_buffer, ICN_9706_ANALOG_GAMMA_LEN);
	dump_data1(p_analog_gamma_reg_data_buffer, ICN_9706_ANALOG_GAMMA_LEN);
	
	return ICN_9706_ANALOG_GAMMA_LEN;
}

static int chip_icn_9706_write_fix_analog_gamma_reg_data(int chip_channel, int mipi_channel, void* p_chip_data)
{
	printf("chip_icn_9706_write_fix_analog_gamma_reg_data:");
	mipi_to_lp_mode(mipi_channel);
	icn_9706_enable_level_2_cmd(mipi_channel);				
	icn_9706_write_analog_reg(mipi_channel, s_icn_9706_default_analog_gamma_code, ICN_9706_ANALOG_GAMMA_LEN);
	mipi_to_hs_mode(mipi_channel);
	return 0;
}

static int chip_icn_9706_write_digital_gamma_reg_data(int chip_channel, int mipi_channel, 
				unsigned char *p_digital_gamma_reg_data, int digital_gamma_reg_len, void* p_chip_data)
{
	icn_9706_info_t* p_chip_info = (icn_9706_info_t*)p_chip_data;
	int i = 0;
	
	for (i = 0; i < ICN_9706_D3G_REG_LEN; i ++)
	{
		// r
		p_chip_info->digital_gamma_reg_data[0][i] = p_digital_gamma_reg_data[i];

		// g
		p_chip_info->digital_gamma_reg_data[1][i] = p_digital_gamma_reg_data[i + ICN_9706_D3G_REG_LEN];

		// b
		p_chip_info->digital_gamma_reg_data[2][i] = p_digital_gamma_reg_data[i + ICN_9706_D3G_REG_LEN * 2];

		// fix d3g error.
		if (i == 13)
		{
			if (p_chip_info->digital_gamma_reg_data[0][i] 
					== p_chip_info->digital_gamma_reg_data[0][i-1])
				p_chip_info->digital_gamma_reg_data[0][i] = p_chip_info->digital_gamma_reg_data[0][i-1] - 1;

			if (p_chip_info->digital_gamma_reg_data[1][i] 
					== p_chip_info->digital_gamma_reg_data[1][i-1])
				p_chip_info->digital_gamma_reg_data[1][i] = p_chip_info->digital_gamma_reg_data[1][i-1] - 1;

			if (p_chip_info->digital_gamma_reg_data[2][i] 
					== p_chip_info->digital_gamma_reg_data[2][i-1])
				p_chip_info->digital_gamma_reg_data[2][i] = p_chip_info->digital_gamma_reg_data[2][i-1] - 1;
		}
	}

	mipi_to_lp_mode(mipi_channel);
	
	icn_9706_enable_level_2_cmd(mipi_channel);
					
	// write new analog gamma reg data.
	icn_9706_write_analog_reg(mipi_channel, p_chip_info->analog_gamma_reg_data, 
								ICN_9706_ANALOG_GAMMA_LEN);

	// write d3g enable
	icn_9706_digital_gamma_control(mipi_channel, 0x03);

	// write default digital gamma value.
	unsigned char r_reg = 0xE4;
	unsigned char g_reg = 0xE5;
	unsigned char b_reg = 0xE6;	

	//icn_9706_enable_level_2_cmd(mipi_channel);
	icn_9706_write_d3g_reg(mipi_channel, r_reg, p_chip_info->digital_gamma_reg_data[0], ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(mipi_channel, g_reg, p_chip_info->digital_gamma_reg_data[1], ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(mipi_channel, b_reg, p_chip_info->digital_gamma_reg_data[2], ICN_9706_D3G_REG_LEN);

	mipi_to_hs_mode(mipi_channel);
	return 0;
}

static int chip_icn_9706_write_fix_digital_gamma_reg_data(int chip_channel, int mipi_channel, void* p_chip_data)
{
	icn_9706_info_t* p_chip_info = (icn_9706_info_t*)p_chip_data;
	unsigned char default_d3g_data[ICN_9706_D3G_REG_LEN] = {
																255, 254, 252, 250, 248, 244, 240, 232, 
																224, 208, 192, 160, 128, 127, 95,  63, 
																47,  31,  23,  15,  11,  7,   5,   3, 
																1, 0
															};

	mipi_to_lp_mode(mipi_channel);
	
	icn_9706_enable_level_2_cmd(mipi_channel);

	// write d3g enable
	icn_9706_digital_gamma_control(mipi_channel, 0x03);

	// write default digital gamma value.
	unsigned char r_reg = 0xE4;
	unsigned char g_reg = 0xE5;
	unsigned char b_reg = 0xE6;	

	//icn_9706_enable_level_2_cmd(mipi_channel);
	icn_9706_write_d3g_reg(mipi_channel, r_reg, default_d3g_data, ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(mipi_channel, g_reg, default_d3g_data, ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(mipi_channel, b_reg, default_d3g_data, ICN_9706_D3G_REG_LEN);

	mipi_to_hs_mode(mipi_channel);
	
	return 0;
}

static int chip_icn_9706_write_d3g_control(int chip_channel, int mipi_channel, int enable, void* p_chip_data)
{
	mipi_to_lp_mode(mipi_channel);
	
	if (enable)
		icn_9706_digital_gamma_control(mipi_channel, 0x01);
	else
		icn_9706_digital_gamma_control(mipi_channel, 0x00);

	mipi_to_hs_mode(mipi_channel);
	
	return 0;
}

static int chip_icn_9706_burn_gamma_otp_values(int chip_channel, int mipi_channel,  int burn_flag,
											int enable_burn_vcom, int vcom, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data)
{
	printf("analog gamma: \n");
	dump_data1(p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	printf("otp write d3g r data: \n");
	dump_data1(p_d3g_r_reg_data, ICN_9706_D3G_REG_LEN);
	printf("otp write d3g g data: \n");
	dump_data1(p_d3g_g_reg_data, ICN_9706_D3G_REG_LEN);
	printf("otp write d3g b data: \n");
	dump_data1(p_d3g_b_reg_data, ICN_9706_D3G_REG_LEN);
			
	mipi_to_lp_mode(mipi_channel);

	icn_9706_burn_vcom_and_gamma_data(mipi_channel, vcom, enable_burn_vcom,
												p_analog_gamma_reg_data, analog_gamma_reg_data_len,
												p_d3g_r_reg_data, ICN_9706_D3G_REG_LEN,
												p_d3g_g_reg_data, ICN_9706_D3G_REG_LEN,
												p_d3g_b_reg_data, ICN_9706_D3G_REG_LEN);
	return 0;
}

static int chip_icn_9706_check_gamma_otp_values(int chip_channel, int mipi_channel, 
											int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data)
{
	int otp_times = 0;
	int read_vcom = 0;
	int check_error = 0;
	
	mipi_to_lp_mode(mipi_channel);

	if (enable_burn_vcom)
	{
		icn_9706_read_vcom(mipi_channel, 0, &read_vcom);
		icn_9706_get_vcom_otp_times(mipi_channel, &otp_times, &otp_times);

		printf("read vcom: %d. 0x02%x, otp times: %d.\n", read_vcom, read_vcom, otp_times);

		// check times
		if (last_vcom_otp_times + 1 != otp_times)
		{
			printf("Otp error: last times: %d, now times: %d.\n", last_vcom_otp_times, otp_times);
			check_error = 1;
		}

		// check vcom value
		if (new_vcom_value != read_vcom)
		{
			printf("Otp error: write vcom: %d, read vcom: %d.\n", new_vcom_value, read_vcom);
			check_error = 2;
		}
	}
	
	// check analog gamma reg value
	unsigned char analog_gamma_data[ICN_9706_ANALOG_GAMMA_LEN] = { 0 };
	ReadModReg(mipi_channel, 1, 0xC8, ICN_9706_ANALOG_GAMMA_LEN, analog_gamma_data);
	printf("read analog gamma data: \n");
	dump_data1(analog_gamma_data, ICN_9706_ANALOG_GAMMA_LEN);
	if (memcmp(analog_gamma_data, p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN) != 0)
	{
		printf("Otp error: check analog gamma reg data error!\n");
		check_error = 3;
	}

	// check dgc r reg value
	// check dgc g reg value
	// check dgc b reg value
	unsigned char d3g_r_data[ICN_9706_D3G_REG_LEN] = { 0 };
	unsigned char d3g_g_data[ICN_9706_D3G_REG_LEN] = { 0 };
	unsigned char d3g_b_data[ICN_9706_D3G_REG_LEN] = { 0 };			
	ReadModReg(mipi_channel, 1, 0xE4, ICN_9706_D3G_REG_LEN, d3g_r_data);
	printf("read d3g r data: \n");
	dump_data1(d3g_r_data, ICN_9706_D3G_REG_LEN);			
	ReadModReg(mipi_channel, 1, 0xE5, ICN_9706_D3G_REG_LEN, d3g_g_data);
	printf("read d3g g data: \n");
	dump_data1(d3g_g_data, ICN_9706_D3G_REG_LEN);			
	ReadModReg(mipi_channel, 1, 0xE6, ICN_9706_D3G_REG_LEN, d3g_b_data);
	printf("read d3g b data: \n");
	dump_data1(d3g_b_data, ICN_9706_D3G_REG_LEN);

	if (memcmp(d3g_r_data, p_d3g_r_reg_data, ICN_9706_D3G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc r reg data error!\n");
		check_error = 4;
	}

	if (memcmp(d3g_g_data, p_d3g_g_reg_data, ICN_9706_D3G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc g reg data error!\n");
		check_error = 5;
	}

	if (memcmp(d3g_b_data, p_d3g_b_reg_data, ICN_9706_D3G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc b reg data error!\n");
		check_error = 6;
	}

	mipi_to_hs_mode(mipi_channel);
	
	return check_error;
}

static int chip_icn_9706_get_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
											unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len,
											void* p_chip_data)
{
	icn_9706_info_t* p_chip_info = (icn_9706_info_t*)p_chip_data;
	
	memcpy(p_analog_gamma_reg_data, p_chip_info->analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);
	*p_analog_gamma_reg_data_len = ICN_9706_ANALOG_GAMMA_LEN;
	
	printf("analog gamma: \n");
	dump_data1(p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	return 0;
}

static int chip_icn_9706_get_digital_gamma_reg_data(int chip_channel, int mipi_channel, 
											unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len,
											void* p_chip_data)
{
	icn_9706_info_t* p_chip_info = (icn_9706_info_t*)p_chip_data;
	
	memcpy(p_d3g_r_reg_data, p_chip_info->digital_gamma_reg_data[0], ICN_9706_D3G_REG_LEN);
	*p_d3g_r_reg_data_len = ICN_9706_D3G_REG_LEN;
	memcpy(p_d3g_g_reg_data, p_chip_info->digital_gamma_reg_data[1], ICN_9706_ANALOG_GAMMA_LEN);
	*p_d3g_g_reg_data_len = ICN_9706_D3G_REG_LEN;
	memcpy(p_d3g_b_reg_data, p_chip_info->digital_gamma_reg_data[2], ICN_9706_ANALOG_GAMMA_LEN);
	*p_d3g_b_reg_data_len = ICN_9706_D3G_REG_LEN;
	return 0;
}

icm_chip_t chip_icn_9706 = 
{
	.my_chip_id = E_MY_CHIP_ID_ICN_9706,

	.fn_get_and_reset_private_data = chip_icn_9706_get_and_reset_private_data,
	
	.fn_get_chip_id = chip_icn_9706_get_chip_id,
	.fn_check_chip_ok = chip_icn_9706_check_chip_ok,
	.fn_read_chip_vcom_opt_times = chip_icn_9706_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = chip_icn_9706_read_chip_vcom_opt_info,
	
	.fn_read_vcom = chip_icn_9706_read_vcom,
	.fn_write_vcom = chip_icn_9706_write_vcom,
	
	.fn_write_chip_vcom_otp_value = chip_icn_9706_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = chip_icn_9706_check_vcom_otp_burn_ok,

	.fn_chip_d3g_control = chip_icn_9706_write_d3g_control,
	
	.fn_write_chip_analog_gamma_reg_data = chip_icn_9706_write_analog_gamma_reg_data,
	.fn_read_chip_analog_gamma_reg_data = chip_icn_9706_read_analog_gamma_reg_data,
	.fn_write_chip_fix_analog_gamma_reg_data = chip_icn_9706_write_fix_analog_gamma_reg_data,
	
	.fn_write_chip_digital_gamma_reg_data = chip_icn_9706_write_digital_gamma_reg_data,
	.fn_write_chip_fix_digital_gamma_reg_data = chip_icn_9706_write_fix_digital_gamma_reg_data,

	.fn_chip_burn_gamma_otp_values = chip_icn_9706_burn_gamma_otp_values,
	.fn_chip_check_gamma_otp_values = chip_icn_9706_check_gamma_otp_values,
	
	.fn_chip_get_analog_gamma_reg_data = chip_icn_9706_get_analog_gamma_reg_data,
	.fn_chip_get_digital_gamma_reg_data = chip_icn_9706_get_digital_gamma_reg_data,
};


