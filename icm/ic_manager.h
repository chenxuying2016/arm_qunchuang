#ifndef _IC_MANAGER_H_
#define _IC_MANAGER_H_

#define MAX_CHIP_CHANNEL_NUMS		(2)


#define CHIP_ID_ILI_9806E			"ili9806e"
#define CHIP_ID_ILI_9881C			"ili9881c"
#define CHIP_ID_OTM_8019A			"otm8019a"
#define CHIP_ID_ICN_9706			"icn9706"
#define CHIP_ID_HI_8394				"hi8394"

#define CHIP_ID_JD_9367				"jd9367"
#define CHIP_ID_NT_35521S			"nt35521s"



enum e_icm_error
{
	E_ICM_OK = 0,
	E_ICM_READ_ID_ERROR = -1,
	E_ICM_VCOM_TIMES_NOT_CHANGE = -2,
	E_ICM_VCOM_VALUE_NOT_CHANGE = -3,
};


enum e_my_chip_id
{
	E_MY_CHIP_ID_DEFAULT = 0,
	E_MY_CHIP_ID_ILI_9806E,		// ili 9806E
	E_MY_CHIP_ID_ILI_9881C,		// ili 9881C
	E_MY_CHIP_ID_OTM_8019A,		// otm8019a
	E_MY_CHIP_ID_ICN_9706,		// icn9706
	E_MY_CHIP_ID_HI_8384,		// hi8394

	E_MY_CHIP_ID_JD_9367,		// JD 9367
	E_MY_CHIP_ID_NT_35521S,		// NT 35521S
	
	E_MY_CHIP_ID_MAX_NUMS,
};

typedef struct tag_icm_channel
{
	int channel;
	int mipi_channel;	// 1, 4.
	int my_chip_id;

	void* chip_private_data;

	// callback.
	
}icm_channel_t;



typedef int (*fn_get_chip_id_t)(int chip_channel, int mipi_channel, unsigned char *p_id_data, int id_data_len, 
									void* p_chip_data);

typedef int (*fn_get_chip_id_otp_info_t)(int chip_channel, int mipi_channel, unsigned char *p_id_data, 
											int* p_id_data_len, unsigned char* p_otp_times, void* p_chip_data);

typedef int (*fn_burn_chip_id_t)(int chip_channel, int mipi_channel, unsigned char *p_id_data, 
											int id_data_len, int ptn_id, void* p_chip_data);


typedef int (*fn_check_chip_ok_t)(int chip_channel, int mipi_channel, void* p_chip_data);

typedef int (*fn_read_chip_vcom_opt_times_t)(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
									void* p_chip_data);

typedef int (*fn_read_chip_vcom_opt_info_t)(int chip_channel, int mipi_channel, int* p_otp_vcom_times, 
											int* p_otp_vcom_value, void* p_chip_data);

typedef int (*fn_write_chip_vcom_otp_value_t)(int chip_channel, int mipi_channel, int otp_vcom_value, void* p_chip_data);

typedef int (*fn_read_vcom_t)(int chip_channel, int mipi_channel, int* p_vcom_value, void* p_chip_data);

typedef int (*fn_write_vcom_t)(int chip_channel, int mipi_channel, int vcom_value, void* p_chip_data);

typedef int (*fn_check_vcom_otp_burn_ok_t)(int chip_channel, int mipi_channel, int vcom_value, int last_otp_times, 
											int *p_read_vcom, void* p_chip_data);

typedef void* (*fn_get_and_reset_private_data_t)(int chip_channel);

typedef int (*fn_write_chip_analog_gamma_reg_data_t)(int chip_channel, int mipi_channel, 
				unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_len, void* p_chip_data);

typedef int (*fn_read_chip_analog_gamma_reg_data_t)(int chip_channel, int mipi_channel, 
				unsigned char *p_analog_gamma_reg_data_buffer, int analog_gamma_reg_len, void* p_chip_data);


typedef int (*fn_write_chip_fix_analog_gamma_reg_data_t)(int chip_channel, int mipi_channel, 
															void* p_chip_data);

typedef int (*fn_write_chip_digital_gamma_reg_data_t)(int chip_channel, int mipi_channel, 
				unsigned char *p_digital_gamma_reg_data, int digital_gamma_reg_len, void* p_chip_data);

typedef int (*fn_write_chip_fix_digital_gamma_reg_data_t)(int chip_channel, int mipi_channel, void* p_chip_data);


typedef int (*fn_chip_d3g_control_t)(int chip_channel, int mipi_channel, int enable, void* p_chip_data);

// burn_flag:
//           0x00000001: second burn flag.
#define ICM_BURN_SECOND_FLAG		(0x00000001)

typedef int (*fn_chip_burn_gamma_otp_values_t)(int chip_channel, int mipi_channel, int burn_flag,
											int enable_burn_vcom, int vcom, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data);

typedef int (*fn_chip_check_gamma_otp_values_t)(int chip_channel, int mipi_channel, 
											int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len,
											void* p_chip_data);

typedef int (*fn_chip_get_analog_gamma_reg_data_t)(int chip_channel, int mipi_channel, 
											unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len,
											void* p_chip_data);

typedef int (*fn_chip_get_digital_gamma_reg_data_t)(int chip_channel, int mipi_channel, 
											unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_deg_b_reg_data_len,
											void* p_chip_data);



typedef struct tag_icm_chip
{
	int my_chip_id;

	// callback
	fn_get_and_reset_private_data_t fn_get_and_reset_private_data;
	
	fn_get_chip_id_t 				fn_get_chip_id;
	fn_check_chip_ok_t 				fn_check_chip_ok;
	
	fn_read_chip_vcom_opt_times_t 	fn_read_chip_vcom_opt_times;
	fn_read_chip_vcom_opt_info_t 	fn_read_chip_vcom_opt_info;
	fn_write_chip_vcom_otp_value_t	fn_write_chip_vcom_otp_value;
	fn_read_vcom_t 					fn_read_vcom;
	fn_write_vcom_t					fn_write_vcom;
	fn_check_vcom_otp_burn_ok_t		fn_check_vcom_otp_burn_ok;

	// gamma 
	fn_write_chip_analog_gamma_reg_data_t fn_write_chip_analog_gamma_reg_data;
	fn_read_chip_analog_gamma_reg_data_t fn_read_chip_analog_gamma_reg_data;
	fn_write_chip_fix_analog_gamma_reg_data_t fn_write_chip_fix_analog_gamma_reg_data;
	
	fn_write_chip_digital_gamma_reg_data_t fn_write_chip_digital_gamma_reg_data;
	fn_write_chip_fix_digital_gamma_reg_data_t fn_write_chip_fix_digital_gamma_reg_data;

	fn_chip_d3g_control_t fn_chip_d3g_control;

	// otp 
	fn_chip_burn_gamma_otp_values_t fn_chip_burn_gamma_otp_values;
	fn_chip_check_gamma_otp_values_t fn_chip_check_gamma_otp_values;

	// read info
	fn_chip_get_analog_gamma_reg_data_t fn_chip_get_analog_gamma_reg_data;
	fn_chip_get_digital_gamma_reg_data_t fn_chip_get_digital_gamma_reg_data;

	// id
	fn_get_chip_id_otp_info_t fn_get_chip_id_otp_info;
	fn_burn_chip_id_t fn_burn_chip_id;
	
}icm_chip_t;

#endif
