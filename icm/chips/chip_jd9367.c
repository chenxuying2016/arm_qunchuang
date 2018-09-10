
#include <stdio.h>

#include "../ic_manager.h"


#include "chip_jd9367.h"

#if 1

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
		case 2:
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

		#if 0
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
		#endif

		#if 0
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
		#endif
		
		case 3:
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


#define ENABLE_VCOM1				(1)
#define ENABLE_VCOM2				(0)


#define MAX_VCOM_OTP_TIMES		(3)

void jd_9367_set_page(int channel, unsigned char page_no)
{
	unsigned char cmd_page[100] = 
	{
		0x39, 0xE0, 0x01, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	cmd_page[3] = page_no;	
	SendCode(channel, cmd_page);
}

static int jd_9367_check_id(int channel)
{
	char reg[3] = { 0 };
	
    jd_9367_set_page(channel, 0);

	#if 0
	// read id1, id2, id3.
	ReadModReg(channel, 1, 0xDA, 1,&reg[0]);
	printf("read id1: %#x. \n", reg[0]);

	ReadModReg(channel, 1, 0xDB, 1,&reg[0]);
	printf("read id2: %#x. \n", reg[0]);

	ReadModReg(channel, 1, 0xDC, 1,&reg[0]);
	printf("read id3: %#x. \n", reg[0]);
	#endif

	
	
	printf("read id4: %02x:%02x:%02x. \n", reg[2], reg[1], reg[0]);

	if (reg[0] == 0x93 && reg[1] == 0x67 && reg[2] == 0xFF)
	{
		return 0;
	}
	else
	{
		printf("jd_9367_check_id: read id failed!\n");
		return -1;
	}

	return 0;
}

static get_otp_times(unsigned char otp_value)
{
	unsigned char times = 0;
	
	switch (otp_value)
	{
		case 0:
			times = 0;
			break;

		case 1:
			times = 1;
			break;

		case 3:
			times = 2;
			break;

		case 7:
			times = 3;
			break;

		default:
			times = 3;
			break;
	}

	return times;
}

static int jd_9367_get_vcom_otp_times(int channel, unsigned char* vcom1_otp_times, 
										unsigned char* vcom2_otp_times)
{
	// set page 1.
	jd_9367_set_page(channel, 1);

	// read vcom write times
	// 0xE8
	unsigned char vcom_otp_status = 0x00;
	ReadModReg(channel, 1, 0xE8, 1, &vcom_otp_status);
	printf("read vcom write status value: %02x.\n",  vcom_otp_status);

	*vcom1_otp_times = vcom_otp_status & 0x07;
	*vcom2_otp_times = (vcom_otp_status >> 3) & 0x07;

	*vcom1_otp_times = get_otp_times(*vcom1_otp_times);
	*vcom2_otp_times = get_otp_times(*vcom2_otp_times);
	printf("chip_jd_9367_get_vcom_otp_times: %d, %d.\n",  *vcom1_otp_times, *vcom2_otp_times);

	return 0;
}

static int jd_9367_read_nvm_vcom(int channel, int index, int *p_vcom)
{
	jd_9367_set_page(channel,1);

	unsigned char vcom1_otp_status = 0x00;
	unsigned char vcom2_otp_status = 0x00;
	unsigned char otp_write_times = 0;
	
	jd_9367_get_vcom_otp_times(channel, &vcom1_otp_status, &vcom2_otp_status);
	printf("read vcom otp status: 0x%02x 0x%02x.\n", vcom1_otp_status, vcom2_otp_status);
	
	{
		
		unsigned char vcom_otp_read_addr1 = 0x15;
		DCSLongWriteWithPara(channel, 0xE0, &vcom_otp_read_addr1, 2);
	
		unsigned char vcom_otp_read_addr2 = 0x00;
		if (index == 0)
		{
			printf("Read VCOM1.\n");
			if (vcom1_otp_status == 1)
			{
				vcom_otp_read_addr2 = 0x16;
				otp_write_times = 1;
			}
			else if (vcom1_otp_status == 2)
			{
				vcom_otp_read_addr2 = 0x18;
				otp_write_times = 2;
			}
			else if (vcom1_otp_status == 3)
			{
				vcom_otp_read_addr2 = 0x19;
				otp_write_times = 3;
			}
			else
			{
				otp_write_times = 0;
			}
		}
		else if (index == 1)
		{
			printf("Read VCOM2.\n");
			if (vcom2_otp_status == 1)
			{
				vcom_otp_read_addr2 = 0x1A;
				otp_write_times = 1;
			}
			else if (vcom2_otp_status == 2)
			{
				vcom_otp_read_addr2 = 0x1C;
				otp_write_times = 2;
			}
			else if (vcom2_otp_status == 3)
			{
				vcom_otp_read_addr2 = 0x1D;
				otp_write_times = 3;
			}
			else
			{
				otp_write_times = 0;
			}
		}
		else
		{
			otp_write_times = 0;
		}
		
		DCSLongWriteWithPara(channel, 0xE1, &vcom_otp_read_addr2, 2);
		//printf("otp write status: times: %d times. \n", otp_write_times);
		printf("vcom read addr: 0x%02x 0x%02x.\n", vcom_otp_read_addr1, vcom_otp_read_addr2);
		

		// nv memory read protection key
		unsigned char reg_data[3] = { 0 };
		reg_data[0] = 0x11;	
		DCSLongWriteWithPara(channel, 0xE3, reg_data, 2);
		reg_data[0] = 0x66;	
		DCSLongWriteWithPara(channel, 0xE4, reg_data, 2);
		reg_data[0] = 0x88;	
		DCSLongWriteWithPara(channel, 0xE5, reg_data, 2);

		// nv memory data read.
		// 0xEA
		unsigned char vcom = 0;
		ReadModReg(channel, 1, 0xEA, 1, &vcom);	
		*p_vcom = vcom;
		printf("read otp vcom value: 0x%02x. %d. \n", vcom, vcom);
	}

	return 0;
}

static int jd_9367_read_vcom(int channel, int* p_vcom)
{
	char reg[3] = { 0 };

	jd_9367_set_page(channel, 0);	

	// set page 1.
	jd_9367_set_page(channel, 1);

#if	ENABLE_VCOM1
	ReadModReg(channel, 1, 0x52, 1, &reg[0]);
	ReadModReg(channel, 1, 0x53, 1, &reg[1]);
	//printf("read vcom1 data: %02x:%02x. \n", reg[0], reg[1]);
#endif

#if	ENABLE_VCOM2
	ReadModReg(channel, 1, 0x54, 1, &reg[0]);
	ReadModReg(channel, 1, 0x55, 1, &reg[1]);
	//printf("read vcom2 data: %02x:%02x. \n", reg[0], reg[1]);
#endif
	
	*p_vcom = (reg[0] << 8) | reg[1];
	return 0;
}

int jd_9367_write_vcom(int channel, int vcom, int otp_burned)
{
	//printf("chip_ili_9806_write_vcom channel: %d. [%d, 0x%02x] ...\n", channel, vcom, vcom);
	char reg[3] = { 0 };

	//channel = 4;
	
	jd_9367_set_page(channel, 0);

	// set page 1.
	jd_9367_set_page(channel, 1);

	unsigned char temp = 0x00;
	if (otp_burned)
		temp = 0x11;

	//printf("write reg 0x56 ==> 0x%02x.\n", temp);
	DCSLongWriteWithPara(channel, 0x56, &temp, 2);

#if ENABLE_VCOM1
	unsigned char temp_vcom = (vcom >> 8) & 0x01;
	DCSLongWriteWithPara(channel, 0x52, &temp_vcom, 2);

	temp_vcom = vcom & 0xFF;
	DCSLongWriteWithPara(channel, 0x53, &temp_vcom, 2);
	//printf("write vcom: %04x, %d.\n", vcom, vcom);
#endif

#if ENABLE_VCOM2
	unsigned char temp_vcom2 = (vcom >> 8) & 0x01;
	DCSLongWriteWithPara(channel, 0x54, &temp_vcom2, 2);

	temp_vcom2 = vcom & 0xFF;
	DCSLongWriteWithPara(channel, 0x55, &temp_vcom2, 2);
#endif

#if 0
	unsigned short read_vcom = 0;
	jd_9367_read_vcom(channel, &read_vcom);
#endif
	return 0;
}


static int jd_9367_burn_vcom(int channel, int vcom)
{
	#if 1
	unsigned char reg = 0;
	int power_channel = 0;

	if (channel == 1)
	{
		power_channel = 3;
	}
	else if (channel == 4)
	{
		power_channel = 2;
	}
	
	// reset
	jd_9367_set_page(channel, 0);
	DCSShortWriteNoPara(channel, 0x01);

	// sleep out	
	DCSShortWriteNoPara(channel, 0x11);

	usleep(120 * 1000);

	jd_9367_set_page(channel, 1);

	// check E8
	unsigned char vcom1_count = 0x00;
	unsigned char vcom2_count = 0x00;
	jd_9367_get_vcom_otp_times(channel, &vcom1_count, &vcom2_count);

	if (vcom1_count == 0x07)
		return -1;

	if (vcom2_count == 0x07)
		return -1;

	// vsp power on
	//pwr_contorl(1, power_channel, 6);

	usleep(20 * 1000);

	//unsigned char temp_reg = 0x00;
	//DCSLongWriteWithPara(channel, 0x56, &temp_reg, 2);

	// write addr.
	// vcom1
	#if	ENABLE_VCOM1
	{
		unsigned char vcom_otp_addr = 0x04;
		unsigned char vcom_data = (vcom >> 8) & 0x01;		

		printf("write vcom1: %02x ", vcom_data);
		DCSLongWriteWithPara(channel, 0xE1, &vcom_otp_addr, 2);
		DCSLongWriteWithPara(channel, 0xE0, &vcom_data, 2);

		vcom_otp_addr = 0x05;
		vcom_data = vcom & 0xFF;
		printf("%02x.\n", vcom_data);
		DCSLongWriteWithPara(channel, 0xE1, &vcom_otp_addr, 2);
		DCSLongWriteWithPara(channel, 0xE0, &vcom_data, 2);

		// nv memory write protection key
		unsigned char reg_data[3] = { 0 };
		reg_data[0] = 0x55;	
		DCSLongWriteWithPara(channel, 0xE3, reg_data, 2);
		reg_data[0] = 0xAA;	
		DCSLongWriteWithPara(channel, 0xE4, reg_data, 2);
		reg_data[0] = 0x66;	
		DCSLongWriteWithPara(channel, 0xE5, reg_data, 2);

		unsigned char otp_is_busy = 1;
		do {
			usleep(10 * 1000);
			ReadModReg(channel, 1, 0xE9, 1, &otp_is_busy);
			printf("otp busy: %#x.\n", otp_is_busy);
		}while (otp_is_busy == 0x01);
	}
	#endif

	// vcom2
	#if	ENABLE_VCOM2
	{
		unsigned char vcom_otp_addr = 0x06;
		unsigned char vcom_data = (vcom >> 8) & 0x01;

		printf("write vcom2: %02x ", vcom_data);
		DCSLongWriteWithPara(channel, 0xE1, &vcom_otp_addr, 2);
		DCSLongWriteWithPara(channel, 0xE0, &vcom_data, 2);

		vcom_otp_addr = 0x07;
		vcom_data = vcom;
		printf("%02x.\n", vcom_data);
		DCSLongWriteWithPara(channel, 0xE1, &vcom_otp_addr, 2);
		DCSLongWriteWithPara(channel, 0xE0, &vcom_data, 2);
		
	
		// nv memory write protection key
		unsigned char reg_data[3] = { 0 };
		reg_data[0] = 0x55;	
		DCSLongWriteWithPara(channel, 0xE3, reg_data, 2);
		reg_data[0] = 0xAA;	
		DCSLongWriteWithPara(channel, 0xE4, reg_data, 2);
		reg_data[0] = 0x66;	
		DCSLongWriteWithPara(channel, 0xE5, reg_data, 2);

		unsigned char otp_is_busy = 1;
		do {
			usleep(10 * 1000);
			ReadModReg(channel, 1, 0xE9, 1, &otp_is_busy);
			printf("otp busy: %#x.\n", otp_is_busy);
		}while (otp_is_busy == 0x01);

	}
	#endif
	
	// vsp power off
	//pwr_contorl(0, power_channel, 6);
	//pwr_contorl(0, 1, 6);

	// reset
	DCSShortWriteNoPara(channel, 0x01);

	usleep(200 * 1000);
	#endif

	return 0;
}

static int jd_9367_otp_check_vcom(int channel, int new_vcom, int old_otp_times)
{
	// check id
	int val = jd_9367_check_id(channel);
	if (val != 0)
	{
		printf("chip_jd_9367_otp_check_vcom: check module id failed!\n");
		return -1;
	}

	#if 1
	// check otp times
	unsigned char vcom1_otp_time = 0;
	unsigned char vcom2_otp_times = 0;
	val = jd_9367_get_vcom_otp_times(channel, &vcom1_otp_time, &vcom2_otp_times);
	if (val != 0)
	{
		printf("chip_jd_9367_otp_check_vcom: get vcom otp times failed!\n");
		return -2;
	}

	if (vcom1_otp_time <= old_otp_times)
	{
		printf("chip_jd_9367_otp_check_vcom: vcom otp times is not change! last: %d. now: %d.\n",
				old_otp_times, vcom1_otp_time);
		return -3;
	}
	#endif
	
	// check vcom
	int index = 0;
	int vcom = 0;
	val = jd_9367_read_nvm_vcom(channel, index, &vcom);
	if (val != 0)
	{
		printf("chip_jd_9367_otp_check_vcom: read nvm vcom failed!\n");
		return -4;
	}

	if (vcom != new_vcom)
	{
		printf("chip_jd_9367_otp_check_vcom: nvm vcom is not change! read vcom: %d. burn vcom: %d.\n",
				vcom, new_vcom);
		return -5;
	}
	
	return 0;
}


#if 1

typedef struct tag_jd_9367_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;
}jd_9367_info_t;

static jd_9367_info_t s_jd_9367_private_info[MAX_CHIP_CHANNEL_NUMS] = { 0 };
// read chip id

// check chip id

// read chip otp times

// read chip otp values


static int chip_jd_9367_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
								void* p_chip_data)
{
	unsigned char reg[3] = { 0 };

	#if 1
	printf("chip_jd_9367_get_chip_id: channel = %d.\n", mipi_channel);
	qc_do_sleep_out(mipi_channel);
	reg[0] = qc_read_otp_data_reg(mipi_channel, 0x07);
	reg[1] = qc_read_otp_data_reg(mipi_channel, 0x08);
	reg[2] = qc_read_otp_data_reg(mipi_channel, 0x09);
	qc_do_sleep_in(mipi_channel);

	p_id_data[0] = reg[0];
	p_id_data[1] = reg[1];
	p_id_data[2] = reg[2];
	#else
    jd_9367_set_page(mipi_channel, 0);
	
	// page 1.
	jd_9367_set_page(mipi_channel, 1);

	// read id4
    ReadModReg(mipi_channel, 1, 0x00, 1, &reg[0]);
	ReadModReg(mipi_channel, 1, 0x01, 1, &reg[1]);
	ReadModReg(mipi_channel, 1, 0x02, 1, &reg[2]);
	printf("chip channel: %d, mipi_channel: %d, read id4: %02x:%02x:%02x. \n", 
				chip_channel, mipi_channel, reg[2], reg[1], reg[0]);

	if (reg[0] == 0x98 && reg[1] == 0x06 && reg[2] == 0x04)
	{
		if (p_id_data && id_data_len >= 3)
		{
			p_id_data[0] = 0x98;
			p_id_data[1] = 0x06;
			p_id_data[2] = 0x04;
			return 3;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		printf("chip channel: %d, mipi_channel: %d, chip_ili_9806e_get_chip_id: read id failed!\n", 
				chip_channel, mipi_channel);
		
		return -1;
	}
	#endif
	
	return -1;
}

static int chip_jd_9367_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{	
	return 0;

	#if 0
	// read and check id;
	chip_jd_9367_get_chip_id(chip_channel, mipi_channel, s_jd_9367_private_info[chip_channel].id, 
								sizeof(s_jd_9367_private_info[chip_channel].id), p_chip_data);

	unsigned char* p_id = s_jd_9367_private_info[chip_channel].id;
	if ( (p_id[0] != 0x98) || (p_id[1] != 0x06) || (p_id[2] != 0x04) )
	{
		printf("chip_jd_9367_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_jd_9367_private_info[chip_channel].id_ok = 0;

		return -1;
	}

	s_jd_9367_private_info[chip_channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	jd_9367_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);
	s_jd_9367_private_info[chip_channel].vcom1_otp_times = vcom1_otp_times;
	s_jd_9367_private_info[chip_channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	jd_9367_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
	jd_9367_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);
	s_jd_9367_private_info[chip_channel].vcom1 = vcom1_otp_value;
	s_jd_9367_private_info[chip_channel].vcom2 = vcom2_otp_value;

	printf("chip_jd_9367_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", chip_channel, mipi_channel,
				s_jd_9367_private_info[chip_channel].vcom1_otp_times,
				s_jd_9367_private_info[chip_channel].vcom2_otp_times,
				s_jd_9367_private_info[chip_channel].vcom1,
				s_jd_9367_private_info[chip_channel].vcom2);
	#endif
	
	return 0;
}

static void* chip_jd_9367_get_and_reset_private_data(int chip_channel)
{
	printf("chip_jd_9367_get_and_reset_private_data.\n");
	s_jd_9367_private_info[chip_channel].id_ok = 0;
	memset(s_jd_9367_private_info[chip_channel].id, 0, sizeof(s_jd_9367_private_info[chip_channel].id));
	s_jd_9367_private_info[chip_channel].vcom1 = 0;
	s_jd_9367_private_info[chip_channel].vcom1_otp_times = 0;
	s_jd_9367_private_info[chip_channel].vcom2 = 0;
	s_jd_9367_private_info[chip_channel].vcom2_otp_times = 0;
	
	return &s_jd_9367_private_info[chip_channel];
}

static int chip_jd_9367_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	if (s_jd_9367_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		jd_9367_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_jd_9367_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_jd_9367_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_jd_9367_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	unsigned char vcom_h = 0;
	unsigned char vcom_l = 0;
	unsigned char vcom_otp_times = 0;
	
	qc_otp_data_read_vcom(mipi_channel, &vcom_h, &vcom_l, &vcom_otp_times);

	*p_otp_vcom_times = vcom_otp_times;
	*p_otp_vcom_value = (vcom_h << 8) | vcom_l;
	
	printf("chip_jd_9367_read_chip_vcom_opt_info: otp times = %d, vcom = %d, %#x.\n", vcom_otp_times, *p_otp_vcom_value);
	return -1;
}

static int chip_jd_9367_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	if (s_jd_9367_private_info[chip_channel].id_ok == 1)
	{
		int vcom = 0;
		jd_9367_read_vcom(mipi_channel, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	printf("chip_jd_9367_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_jd_9367_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	//printf("chip_jd_9367_write_vcom: chip channel = %d, mipi_channel: %d. ...\n",
	//		chip_channel, mipi_channel);
	
	if (s_jd_9367_private_info[chip_channel].id_ok == 1)
	{
		if (s_jd_9367_private_info[chip_channel].vcom1_otp_times == 0)
		{
			//chip_ili_9806e_write_vcom(chip_channel, mipi_channel, vcom_value, p_chip_data)
			jd_9367_write_vcom(mipi_channel, vcom_value, 0);
		}
		else
		{
			jd_9367_write_vcom(mipi_channel, vcom_value, 1);
		}
		
		//printf("chip_jd_9367_read_vcom: end\n");

		return 0;
	}
	
	printf("chip_jd_9367_write_vcom error: unknow chip!\n");
	
	return -1;
}

static int chip_jd_9367_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{

	if (s_jd_9367_private_info[chip_channel].id_ok == 1)
	{
		int val = 0;
		#ifdef ENABlE_OTP_BURN
		val = jd_9367_burn_vcom(mipi_channel, otp_vcom_value);
		#else
		printf("chip_jd_9367_write_chip_vcom_otp_value: *** do nothing ***\n");
		#endif
		return val;
	}

	printf("chip_jd_9367_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_jd_9367_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	// check id 
	int val = chip_jd_9367_check_chip_ok(chip_channel, mipi_channel, p_chip_data);
	if (val != 0)
	{
		printf("chip_jd_9367_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_jd_9367_private_info[chip_channel].vcom1;

	#if 1
	// check times
	if (s_jd_9367_private_info[chip_channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_jd_9367_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_jd_9367_private_info[chip_channel].vcom1_otp_times);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}
	#endif

	// check vcom value.
	if (s_jd_9367_private_info[chip_channel].vcom1 != vcom_value)
	{
		printf("chip_jd_9367_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_jd_9367_private_info[chip_channel].vcom1);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}

	return 0;
}
#endif

static int chip_jd_9367_get_chip_id_otp_info(int chip_channel, int mipi_channel, unsigned char *p_id_data, 
											int* p_id_data_len, unsigned char* p_otp_times, void* p_chip_data)
{
	if (p_id_data == NULL)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, p_id_data = NULL.\n");
		return -1;
	}
	
	if (p_id_data_len == NULL)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, id_data_len = NULL.\n");
		return -1;
	}

	if (*p_id_data_len < 3)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, id_data_len = %d.\n", *p_id_data_len);
		return -1;
	}

	if (p_otp_times == NULL)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, p_otp_times = NULL.\n");
		return -1;
	}

	unsigned char id1 = 0;
	unsigned char id2 = 0;
	unsigned char id3 = 0;
	unsigned char otp_times = 0;
	
	qc_otp_data_read_id(mipi_channel, &id1, &id2, &id3, &otp_times);

	p_id_data[0] = id1;
	p_id_data[1] = id2;
	p_id_data[2] = id3;
	*p_otp_times = otp_times;
	*p_id_data_len = 3;
	
	return 0;
}

static int chip_jd_9367_burn_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, 
											int id_data_len, int ptn_id, void* p_chip_data)
{
	qc_otp_id(mipi_channel, p_id_data[0], p_id_data[1], p_id_data[2], ptn_id);
	
	return 0;
}

											
icm_chip_t chip_jd_9367 = 
{
	.my_chip_id = E_MY_CHIP_ID_JD_9367,

	.fn_get_and_reset_private_data = chip_jd_9367_get_and_reset_private_data,

	.fn_get_chip_id = chip_jd_9367_get_chip_id,
	.fn_check_chip_ok = chip_jd_9367_check_chip_ok,
	.fn_read_chip_vcom_opt_times = chip_jd_9367_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = chip_jd_9367_read_chip_vcom_opt_info,

	.fn_read_vcom = chip_jd_9367_read_vcom,
	.fn_write_vcom = chip_jd_9367_write_vcom,
	
	.fn_write_chip_vcom_otp_value = chip_jd_9367_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = chip_jd_9367_check_vcom_otp_burn_ok,

	.fn_get_chip_id_otp_info = chip_jd_9367_get_chip_id_otp_info,
	.fn_burn_chip_id = chip_jd_9367_burn_chip_id,
};


