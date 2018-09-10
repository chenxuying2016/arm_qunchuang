
#include <stdio.h>

#include "../ic_manager.h"


#include "chip_ili_9806.h"

#define ENABLE_VCOM1				(1)
#define ENABLE_VCOM2				(0)


#define MAX_VCOM_OTP_TIMES		(3)

void ili_9806_set_page(int channel, unsigned char page_no)
{
	char page[] = {0xFF, 0x98, 0x06, 0x04, 00};	
	page[4] = page_no;
	DCSLongWriteWithPara(channel, 0xFF, page, 6);
}


void ili_9806_set_dot_mode(int mipi_channel)
{	
#if 0
	unsigned char temp = 0x02;
	
	// set page 1.
	ili_9806_set_page(mipi_channel, 1);
	DCSLongWriteWithPara(mipi_channel, 0x31, &temp, 2);
#endif
}

void ili_9806_set_row_mode(int mipi_channel)
{	
#if 0
	unsigned char temp = 0x00;
	
	// set page 1.
	ili_9806_set_page(mipi_channel, 1);
	DCSLongWriteWithPara(mipi_channel, 0x31, &temp, 2);
#endif
}

static int ili_9806_check_id(int channel)
{
	char reg[3] = { 0 };
	
    ili_9806_set_page(channel, 0);

	#if 0
	// read id1, id2, id3.
	ReadModReg(channel, 1, 0xDA, 1,&reg[0]);
	printf("read id1: %#x. \n", reg[0]);

	ReadModReg(channel, 1, 0xDB, 1,&reg[0]);
	printf("read id2: %#x. \n", reg[0]);

	ReadModReg(channel, 1, 0xDC, 1,&reg[0]);
	printf("read id3: %#x. \n", reg[0]);
	#endif
	
	// page 1.
	ili_9806_set_page(channel, 1);

	// read id4
    ReadModReg(channel, 1, 0x00, 1, &reg[0]);
	ReadModReg(channel, 1, 0x01, 1, &reg[1]);
	ReadModReg(channel, 1, 0x02, 1, &reg[2]);
	printf("read id4: %02x:%02x:%02x. \n", reg[2], reg[1], reg[0]);

	if (reg[0] == 0x98 && reg[1] == 0x06 && reg[2] == 0x04)
	{
		return 0;
	}
	else
	{
		printf("chip_ili_9806_check_id: read id failed!\n");
		return -1;
	}

	#if 0
	// read vcom1
	ReadModReg(channel, 1, 0x52, 1, &reg[0]);
	ReadModReg(channel, 1, 0x53, 1, &reg[1]);
	printf("read vcom1 data: %02x:%02x. ", reg[0], reg[1]);

	// read vcom2
	ReadModReg(channel, 1, 0x54, 1, &reg[0]);
	ReadModReg(channel, 1, 0x55, 1, &reg[1]);
	printf("read vcom2 data: %02x:%02x. ", reg[0], reg[1]);
	
	ReadModReg(channel, 1, 0x56, 1, &reg[0]);
	printf("read nvm data: %02x. ", reg[0]);
	#endif

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

static int ili_9806_get_vcom_otp_times(int channel, unsigned char* vcom1_otp_times, 
										unsigned char* vcom2_otp_times)
{
	// set page 1.
	ili_9806_set_page(channel, 1);

	// read vcom write times
	// 0xE8
	unsigned char vcom_otp_status = 0x00;
	ReadModReg(channel, 1, 0xE8, 1, &vcom_otp_status);
	printf("read vcom write status value: %02x.\n",  vcom_otp_status);

	*vcom1_otp_times = vcom_otp_status & 0x07;
	*vcom2_otp_times = (vcom_otp_status >> 3) & 0x07;

	*vcom1_otp_times = get_otp_times(*vcom1_otp_times);
	*vcom2_otp_times = get_otp_times(*vcom2_otp_times);
	printf("chip_ili_9806_get_vcom_otp_times: %d, %d.\n",  *vcom1_otp_times, *vcom2_otp_times);

	return 0;
}

static int ili_9806_read_nvm_vcom(int channel, int index, int *p_vcom)
{
	ili_9806_set_page(channel,1);

	unsigned char vcom1_otp_status = 0x00;
	unsigned char vcom2_otp_status = 0x00;
	unsigned char otp_write_times = 0;
	
	ili_9806_get_vcom_otp_times(channel, &vcom1_otp_status, &vcom2_otp_status);
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

static int ili_9806_read_vcom(int channel, int* p_vcom)
{
	char reg[3] = { 0 };

	ili_9806_set_page(channel, 0);	

	// set page 1.
	ili_9806_set_page(channel, 1);

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

int ili_9806_write_vcom(int channel, int vcom, int otp_burned)
{
	//printf("chip_ili_9806_write_vcom channel: %d. [%d, 0x%02x] ...\n", channel, vcom, vcom);
	char reg[3] = { 0 };

	//channel = 4;
	
	ili_9806_set_page(channel, 0);

	// set page 1.
	ili_9806_set_page(channel, 1);

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
	ili_9806_read_vcom(channel, &read_vcom);
#endif
	return 0;
}


static int ili_9806_burn_vcom(int channel, int vcom)
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
	ili_9806_set_page(channel, 0);
	DCSShortWriteNoPara(channel, 0x01);

	// sleep out	
	DCSShortWriteNoPara(channel, 0x11);

	usleep(120 * 1000);

	ili_9806_set_page(channel, 1);

	// check E8
	unsigned char vcom1_count = 0x00;
	unsigned char vcom2_count = 0x00;
	ili_9806_get_vcom_otp_times(channel, &vcom1_count, &vcom2_count);

	if (vcom1_count == 0x07)
		return -1;

	if (vcom2_count == 0x07)
		return -1;

	// vsp power on
	pwr_contorl(1, power_channel, 6);

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
	pwr_contorl(0, 1, 6);

	// reset
	DCSShortWriteNoPara(channel, 0x01);

	usleep(200 * 1000);
	#endif

	return 0;
}

static int ili_9806_otp_check_vcom(int channel, int new_vcom, int old_otp_times)
{
	// check id
	int val = ili_9806_check_id(channel);
	if (val != 0)
	{
		printf("chip_ili_9806_otp_check_vcom: check module id failed!\n");
		return -1;
	}

	#if 1
	// check otp times
	unsigned char vcom1_otp_time = 0;
	unsigned char vcom2_otp_times = 0;
	val = ili_9806_get_vcom_otp_times(channel, &vcom1_otp_time, &vcom2_otp_times);
	if (val != 0)
	{
		printf("chip_ili_9806_otp_check_vcom: get vcom otp times failed!\n");
		return -2;
	}

	if (vcom1_otp_time <= old_otp_times)
	{
		printf("chip_ili_9806_otp_check_vcom: vcom otp times is not change! last: %d. now: %d.\n",
				old_otp_times, vcom1_otp_time);
		return -3;
	}
	#endif
	
	// check vcom
	int index = 0;
	int vcom = 0;
	val = ili_9806_read_nvm_vcom(channel, index, &vcom);
	if (val != 0)
	{
		printf("chip_ili_9806_otp_check_vcom: read nvm vcom failed!\n");
		return -4;
	}

	if (vcom != new_vcom)
	{
		printf("chip_ili_9806_otp_check_vcom: nvm vcom is not change! read vcom: %d. burn vcom: %d.\n",
				vcom, new_vcom);
		return -5;
	}
	
	return 0;
}


#if 1

typedef struct tag_ili_9806e_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;
}ili_9806e_info_t;

static ili_9806e_info_t s_9806e_private_info[MAX_CHIP_CHANNEL_NUMS] = { 0 };
// read chip id

// check chip id

// read chip otp times

// read chip otp values


static int chip_ili_9806e_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
								void* p_chip_data)
{
	char reg[3] = { 0 };
	
    ili_9806_set_page(mipi_channel, 0);
	
	// page 1.
	ili_9806_set_page(mipi_channel, 1);

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
	
	return -1;
}

static int chip_ili_9806e_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{	
	// read and check id;
	chip_ili_9806e_get_chip_id(chip_channel, mipi_channel, s_9806e_private_info[chip_channel].id, 
								sizeof(s_9806e_private_info[chip_channel].id), p_chip_data);

	unsigned char* p_id = s_9806e_private_info[chip_channel].id;
	if ( (p_id[0] != 0x98) || (p_id[1] != 0x06) || (p_id[2] != 0x04) )
	{
		printf("chip_ili_9806e_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_9806e_private_info[chip_channel].id_ok = 0;

		return -1;
	}

	s_9806e_private_info[chip_channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	ili_9806_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);
	s_9806e_private_info[chip_channel].vcom1_otp_times = vcom1_otp_times;
	s_9806e_private_info[chip_channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	ili_9806_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
	ili_9806_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);
	s_9806e_private_info[chip_channel].vcom1 = vcom1_otp_value;
	s_9806e_private_info[chip_channel].vcom2 = vcom2_otp_value;

	printf("chip_ili_9806e_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", chip_channel, mipi_channel,
				s_9806e_private_info[chip_channel].vcom1_otp_times,
				s_9806e_private_info[chip_channel].vcom2_otp_times,
				s_9806e_private_info[chip_channel].vcom1,
				s_9806e_private_info[chip_channel].vcom2);
	return 0;
}

static void* chip_ili_9806e_get_and_reset_private_data(int chip_channel)
{
	printf("chip_ili_9806e_get_and_reset_private_data.\n");
	s_9806e_private_info[chip_channel].id_ok = 0;
	memset(s_9806e_private_info[chip_channel].id, 0, sizeof(s_9806e_private_info[chip_channel].id));
	s_9806e_private_info[chip_channel].vcom1 = 0;
	s_9806e_private_info[chip_channel].vcom1_otp_times = 0;
	s_9806e_private_info[chip_channel].vcom2 = 0;
	s_9806e_private_info[chip_channel].vcom2_otp_times = 0;
	
	return &s_9806e_private_info[chip_channel];
}

static int chip_ili_9806e_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	if (s_9806e_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		ili_9806_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_ili_9806e_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_ili_9806e_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	if (s_9806e_private_info[chip_channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		ili_9806_get_vcom_otp_times(mipi_channel, &vcom1_otp_times, &vcom2_otp_times);

		// read vcom otp value
		int vcom1_otp_value = 0;
		int vcom2_otp_value = 0;
		ili_9806_read_nvm_vcom(mipi_channel, 0, &vcom1_otp_value);
		ili_9806_read_nvm_vcom(mipi_channel, 1, &vcom2_otp_value);

		*p_otp_vcom_times = vcom1_otp_times;
		*p_otp_vcom_value = vcom1_otp_value;

		printf("chip_ili_9806e_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}

static int chip_ili_9806e_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	if (s_9806e_private_info[chip_channel].id_ok == 1)
	{
		int vcom = 0;
		ili_9806_read_vcom(mipi_channel, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	printf("chip_ili_9806e_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_ili_9806e_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	//printf("chip_ili_9806e_write_vcom: chip channel = %d, mipi_channel: %d. ...\n",
	//		chip_channel, mipi_channel);
	
	if (s_9806e_private_info[chip_channel].id_ok == 1)
	{
		if (s_9806e_private_info[chip_channel].vcom1_otp_times == 0)
		{
			//chip_ili_9806e_write_vcom(chip_channel, mipi_channel, vcom_value, p_chip_data)
			ili_9806_write_vcom(mipi_channel, vcom_value, 0);
		}
		else
		{
			ili_9806_write_vcom(mipi_channel, vcom_value, 1);
		}
		
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_write_vcom error: unknow chip!\n");
	
	return -1;
}

static int chip_ili_9806e_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{

	if (s_9806e_private_info[chip_channel].id_ok == 1)
	{
		int val = 0;
		#ifdef ENABlE_OTP_BURN
		val = ili_9806_burn_vcom(mipi_channel, otp_vcom_value);
		#else
		printf("chip_ili_9806e_write_chip_vcom_otp_value: *** do nothing ***\n");
		#endif
		return val;
	}

	printf("chip_ili_9806e_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_ili_9806e_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	// check id 
	int val = chip_ili_9806e_check_chip_ok(chip_channel, mipi_channel, p_chip_data);
	if (val != 0)
	{
		printf("chip_ili_9806e_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_9806e_private_info[chip_channel].vcom1;

	#if 1
	// check times
	if (s_9806e_private_info[chip_channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_ili_9806e_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_9806e_private_info[chip_channel].vcom1_otp_times);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}
	#endif

	// check vcom value.
	if (s_9806e_private_info[chip_channel].vcom1 != vcom_value)
	{
		printf("chip_ili_9806e_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_9806e_private_info[chip_channel].vcom1);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}

	return 0;
}
#endif

icm_chip_t chip_ili_9806e = 
{
	.my_chip_id = E_MY_CHIP_ID_ILI_9806E,

	.fn_get_and_reset_private_data = chip_ili_9806e_get_and_reset_private_data,

	.fn_get_chip_id = chip_ili_9806e_get_chip_id,
	.fn_check_chip_ok = chip_ili_9806e_check_chip_ok,
	.fn_read_chip_vcom_opt_times = chip_ili_9806e_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = chip_ili_9806e_read_chip_vcom_opt_info,

	.fn_read_vcom = chip_ili_9806e_read_vcom,
	.fn_write_vcom = chip_ili_9806e_write_vcom,
	
	.fn_write_chip_vcom_otp_value = chip_ili_9806e_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = chip_ili_9806e_check_vcom_otp_burn_ok,
};


