
#include <stdio.h>


#include "../ic_manager.h"


#include "default_chip.h"



static int default_chip_get_chip_id(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
								void* p_chip_data)
{
	printf("default_chip_get_chip_id error: unknow chip!\n");
	return -1;
}


static int default_chip_check_chip_ok(int chip_channel, int mipi_channel, void* p_chip_data)
{
	printf("default_chip_check_chip_ok error: unknow chip!\n");
	return -1;
}

static int default_chip_read_chip_vcom_opt_times(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
													void* p_chip_data)
{
	printf("default_chip_read_chip_vcom_opt_times error: unknow chip!\n");
	return -1;
}


static int default_chip_read_chip_vcom_opt_info(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
												int *p_otp_vcom_value, void* p_chip_data)
{
	printf("default_chip_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}


static int default_chip_write_chip_vcom_otp_value(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data)
{
	printf("default_chip_write_chip_vcom_otp_value error: unknow chip!\n");
	return -1;
}


static int default_chip_read_vcom(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data)
{
	printf("default_chip_read_vcom error: unknow chip!\n");
	return -1;
}


static int default_chip_write_vcom(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data)
{
	printf("default_chip_write_vcom error: unknow chip!\n");
	return -1;
}

static void* default_chip_get_and_reset_private_data(int chip_channel)
{
	printf("default_chip_get_and_reset_private_data error: unknow chip!\n");
	return NULL;
}


static int default_chip_check_vcom_otp_burn_ok(int chip_channel, int mipi_channel, int vcom_value, 
													int last_otp_times, int *p_read_vcom, void* p_chip_data)
{
	printf("default_chip_check_vcom_otp_burn_ok error: unknow chip!\n");
	return -1;
}

// gamma
static int default_chip_write_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
				unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_len, void* p_chip_data)
{
	printf("default_chip_write_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_read_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
				unsigned char *p_analog_gamma_reg_data_buffer, int analog_gamma_reg_len, void* p_chip_data)
{
	printf("default_chip_read_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_write_fix_analog_gamma_reg_data(int chip_channel, int mipi_channel, void* p_chip_data)
{
	printf("default_chip_write_fix_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_write_digital_gamma_reg_data(int chip_channel, int mipi_channel, 
				unsigned char *p_digital_gamma_reg_data, int digital_gamma_reg_len, void* p_chip_data)
{
	printf("default_chip_write_digital_gamma_reg_data error: unknow chip!\n");
	return -1;
}
				
static int default_chip_write_fix_digital_gamma_reg_data(int chip_channel, int mipi_channel, void* p_chip_data)
{
	printf("default_chip_write_fix_digital_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_write_d3g_control(int chip_channel, int mipi_channel, int enable, void* p_chip_data)
{
	printf("default_chip_write_d3g_control error: unknow chip!\n");
	return -1;
}


static int default_chip_burn_gamma_otp_values(int chip_channel, int mipi_channel, int burn_flag,
											int enable_burn_vcom, int vcom, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data)
{
	printf("default_chip_burn_gamma_otp_values error: unknow chip!\n");
	return -1;
}

static int default_chip_check_gamma_otp_values(int chip_channel, int mipi_channel, 
											int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data)
{
	printf("default_chip_check_gamma_otp_values error: unknow chip!\n");
	return -1;
}

static int default_chip_get_analog_gamma_reg_data(int chip_channel, int mipi_channel, 
											unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len,
											void* p_chip_data)
{
	printf("default_chip_get_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_get_digital_gamma_reg_data(int chip_channel, int mipi_channel, 
											unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len,
											void* p_chip_data)
{
	printf("default_chip_get_digital_gamma_reg_data error: unknow chip!\n");
	return -1;
}


icm_chip_t default_chip = 
{
	.my_chip_id = E_MY_CHIP_ID_DEFAULT,

	.fn_get_and_reset_private_data = default_chip_get_and_reset_private_data,
	
	.fn_get_chip_id = default_chip_get_chip_id,
	.fn_check_chip_ok = default_chip_check_chip_ok,
	.fn_read_chip_vcom_opt_times = default_chip_read_chip_vcom_opt_times,
	.fn_read_chip_vcom_opt_info = default_chip_read_chip_vcom_opt_info,
	
	.fn_read_vcom = default_chip_read_vcom,
	.fn_write_vcom = default_chip_write_vcom,
	
	.fn_write_chip_vcom_otp_value = default_chip_write_chip_vcom_otp_value,
	.fn_check_vcom_otp_burn_ok = default_chip_check_vcom_otp_burn_ok,

	// gamma
	.fn_chip_d3g_control = default_chip_write_d3g_control,
	.fn_write_chip_analog_gamma_reg_data = default_chip_write_analog_gamma_reg_data,
	.fn_read_chip_analog_gamma_reg_data = default_chip_read_analog_gamma_reg_data,
	.fn_write_chip_fix_analog_gamma_reg_data = default_chip_write_fix_analog_gamma_reg_data,
	
	.fn_write_chip_digital_gamma_reg_data = default_chip_write_digital_gamma_reg_data,
	.fn_write_chip_fix_digital_gamma_reg_data = default_chip_write_fix_digital_gamma_reg_data,

	.fn_chip_burn_gamma_otp_values = default_chip_burn_gamma_otp_values,
	.fn_chip_check_gamma_otp_values = default_chip_check_gamma_otp_values,
	
	.fn_chip_get_analog_gamma_reg_data = default_chip_get_analog_gamma_reg_data,
	.fn_chip_get_digital_gamma_reg_data = default_chip_get_digital_gamma_reg_data,
};


