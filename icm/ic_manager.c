
#include <stdio.h>

#include "ic_manager.h"

static int s_my_chip_id = E_MY_CHIP_ID_DEFAULT;
static int s_chip_channel_nums = 2;
static icm_channel_t s_chip_channel_data[MAX_CHIP_CHANNEL_NUMS] = { 0 };
static icm_chip_t* p_current_chip = NULL;


extern icm_chip_t default_chip;

#ifdef LCD_ILI_9806
extern icm_chip_t chip_ili_9806e;
#endif

#ifdef LCD_ILI_9881C
extern icm_chip_t chip_ili_9881c;
#endif

#ifdef LCD_OTM_8019
extern icm_chip_t chip_otm_8019a;
#endif

#ifdef LCD_ICN_9706
extern icm_chip_t chip_icn_9706;
#endif

#ifdef LCD_HI_8394
extern icm_chip_t chip_hi_8394;
#endif

#ifdef LCD_JD_9367
extern icm_chip_t chip_jd_9367;
#endif

#ifdef LCD_ID_NT_35521S
extern icm_chip_t chip_nt_35521s;
#endif

int ic_mgr_set_my_chip_id(char* str_chip_id)
{
	printf("ic_mgr_set_my_chip_id: %s.\n", str_chip_id);
	//int my_chip_id = E_MY_CHIP_ID_DEFAULT;

	s_my_chip_id = E_MY_CHIP_ID_DEFAULT;
	p_current_chip = &default_chip;
		
	// check chip id.
	if (strcmp(str_chip_id, CHIP_ID_JD_9367) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_JD_9367;
		p_current_chip = &chip_jd_9367;
		return 0;
	}
	
	#ifdef LCD_ILI_9806
	if (strcmp(str_chip_id, CHIP_ID_ILI_9806E) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_ILI_9806E;
		p_current_chip = &chip_ili_9806e;
		return 0;
	}
	#endif
	
	#ifdef LCD_ILI_9881C
	if (strcmp(str_chip_id, CHIP_ID_ILI_9881C) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_ILI_9881C;
		p_current_chip = &chip_ili_9881c;
		return 0;
	}
	#endif
	
	#ifdef LCD_OTM_8019
	if (strcmp(str_chip_id, CHIP_ID_OTM_8019A) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_OTM_8019A;
		p_current_chip = &chip_otm_8019a;
		return 0;
	} 
	#endif
	
	#ifdef LCD_ICN_9706
	if (strcmp(str_chip_id, CHIP_ID_ICN_9706) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_ICN_9706;
		p_current_chip = &chip_icn_9706;
		return 0;
	}
	#endif
	
	#ifdef LCD_HI_8394
	if (strcmp(str_chip_id, CHIP_ID_HI_8394) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_HI_8384;
		p_current_chip = &chip_hi_8394;
		return 0;
	} 
	#endif
	
	#ifdef LCD_ID_NT_35521S
	if (strcmp(str_chip_id, CHIP_ID_NT_35521S) == 0)
	{
		s_my_chip_id = E_MY_CHIP_ID_NT_35521S;
		p_current_chip = &chip_nt_35521s;
		return 0;
	} 
	#endif

	return -1;
}

int ic_mgr_reset_mipi_channel(int channel, int mipi_channel)
{
	s_chip_channel_data[channel].mipi_channel = -1;
	s_chip_channel_data[channel].chip_private_data = NULL;
	
	return 0;
}

int ic_mgr_set_mipi_channel(int channel, int mipi_channel)
{
	s_chip_channel_data[channel].mipi_channel = mipi_channel;
	if (p_current_chip)
	{
		s_chip_channel_data[channel].chip_private_data = p_current_chip->fn_get_and_reset_private_data(channel);
	}
	return 0;
}

static int get_chip_channel_by_mipi_channel(int mipi_channel)
{
	int i = 0;
	for (; i < s_chip_channel_nums; i ++)
	{
		printf("s_chip_channel_data[i].mipi_channel = %d.\n", s_chip_channel_data[i].mipi_channel);
		if (s_chip_channel_data[i].mipi_channel == mipi_channel)
			return i;
	}
	return -1;
}

int ic_mgr_init()
{
	printf("ic_mgr_init ...\n");
	s_chip_channel_nums = MAX_CHIP_CHANNEL_NUMS;
	

	int i = 0;
	for (; i < s_chip_channel_nums; i ++)
	{
		s_chip_channel_data[i].channel = i;
		s_chip_channel_data[i].my_chip_id = E_MY_CHIP_ID_DEFAULT;
		s_chip_channel_data[i].mipi_channel = -1;
		
		// init chip module.
		s_chip_channel_data[i].chip_private_data = NULL;
		
	}

	// register chip list.
	
	return 0;
}


int ic_mgr_term()
{
	int i = 0;

	// unregister chip list

	
	for (; i < s_chip_channel_nums; i ++)
	{
		s_chip_channel_data[i].channel = 0;
		s_chip_channel_data[i].my_chip_id = E_MY_CHIP_ID_DEFAULT;
		s_chip_channel_data[i].mipi_channel = -1;
		s_chip_channel_data[i].chip_private_data = NULL;
	}

	s_chip_channel_nums = 0;
	
	return 0;
}


int ic_mgr_read_chip_id(int mipi_channel, unsigned char* p_id_data, int id_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_get_chip_id)
			{
	 			val = p_current_chip->fn_get_chip_id(channel, mipi_channel, p_id_data, id_data_len, 
													s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_read_chip_id error: p_current_chip->fn_get_chip_id is NULL!\n");
			}
				
		}
		else
		{
			printf("ic_mgr_read_chip_id: p_current_chip is NULL!\n");
		}
	}
	
	return val;
}

int ic_mgr_check_chip_ok(int mipi_channel)
{
	// read chip id

	// check chip id

	// read chip otp times

	// read chip otp values

	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if(p_current_chip->fn_check_chip_ok)
			{
		 		val = p_current_chip->fn_check_chip_ok(channel, mipi_channel, 
													s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_check_chip_ok error: p_current_chip->fn_check_chip_ok is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_check_chip_ok: p_current_chip is NULL!\n");
		}
	}
	else
	{
		printf("chip channel = %d, > chip number = %d.\n", channel, s_chip_channel_nums);
	}
	
	return val;
}

// vcom
int ic_mgr_read_chip_vcom_otp_times(int mipi_channel, int* p_otp_vcom_times)
{
	int val = -1;

	//printf("ic_mgr_read_chip_vcom_otp_times: mipi_channel = %d.\n", mipi_channel);
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_read_chip_vcom_opt_times)
			{
				val = p_current_chip->fn_read_chip_vcom_opt_times(channel, mipi_channel, p_otp_vcom_times, 
		 													s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_read_chip_vcom_otp_times error: p_current_chip->fn_read_chip_vcom_opt_times is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_read_chip_vcom_otp_times: p_current_chip is NULL!\n");
		}													
	}
	else
	{
		printf("ic_mgr_read_chip_vcom_otp_times: invalid chip channel = %d.\n", channel);
	}
	
	return val;
}

int ic_mgr_read_chip_vcom_otp_value(int mipi_channel, int* p_otp_vcom_times, int* p_otp_vcom_value)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_read_chip_vcom_opt_info)
			{
				val = p_current_chip->fn_read_chip_vcom_opt_info(channel, mipi_channel, p_otp_vcom_times, p_otp_vcom_value, 
		 													s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_read_chip_vcom_otp_value error: p_current_chip->fn_read_chip_vcom_opt_info is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_read_chip_vcom_otp_value: p_current_chip is NULL!\n");
		} 													
	}
	
	return val;
}

int ic_mgr_write_chip_vcom_otp_value(int mipi_channel, int otp_vcom_value)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_write_chip_vcom_otp_value)
			{
				val = p_current_chip->fn_write_chip_vcom_otp_value(channel, mipi_channel, otp_vcom_value, 
		 														s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_write_chip_vcom_otp_value error: p_current_chip->fn_write_chip_vcom_otp_value is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_chip_vcom_otp_value: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}


int ic_mgr_read_vcom(int mipi_channel, int* p_vcom_value)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_read_vcom)
			{
				val = p_current_chip->fn_read_vcom(channel, mipi_channel, p_vcom_value, 
												s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_read_vcom error: p_current_chip->fn_read_vcom is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_read_vcom: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_write_vcom(int mipi_channel, int vcom_value)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_write_vcom)
			{
				val = p_current_chip->fn_write_vcom(channel, mipi_channel, vcom_value, 
									s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_write_vcom error: p_current_chip->fn_write_vcom is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_vcom: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_check_vcom_otp_burn_ok(int mipi_channel, int vcom_value, int last_otp_times, int *p_read_vcom)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_check_vcom_otp_burn_ok)
			{
		 		val = p_current_chip->fn_check_vcom_otp_burn_ok(channel, mipi_channel, vcom_value, last_otp_times, 
		 													p_read_vcom, 
		 													s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_check_vcom_otp_burn_ok error: p_current_chip->fn_check_vcom_otp_burn_ok is NULL!\n");
			}
		}
		 else
		{
			printf("ic_mgr_check_vcom_otp_burn_ok: p_current_chip is NULL!\n");
		}
	}
	
	return val;
}


// id


// gamma
int ic_mgr_write_analog_gamma_data(int mipi_channel, unsigned char *p_analog_gamma_reg_data,
									int analog_gamma_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_write_chip_analog_gamma_reg_data)
			{
				if (p_current_chip->fn_write_chip_analog_gamma_reg_data)
				{
					val = p_current_chip->fn_write_chip_analog_gamma_reg_data(channel, mipi_channel, 
											p_analog_gamma_reg_data, analog_gamma_reg_data_len, 
											s_chip_channel_data[channel].chip_private_data);
				}
				else
				{
					printf("ic_mgr_write_analog_gamma_data error: p_current_chip->fn_write_chip_analog_gamma_reg_data is NULL!\n");
				}
			}
			else
			{
				printf("ic_mgr_check_vcom_otp_burn_ok error: p_current_chip->fn_check_vcom_otp_burn_ok is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_analog_gamma_data: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}
									
int ic_mgr_read_analog_gamma_data(int mipi_channel, unsigned char *p_analog_gamma_reg_data,
									int analog_gamma_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_read_chip_analog_gamma_reg_data)
			{
				val = p_current_chip->fn_read_chip_analog_gamma_reg_data(channel, mipi_channel, 
											p_analog_gamma_reg_data, analog_gamma_reg_data_len, 
											s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_read_analog_gamma_data error: p_current_chip->fn_read_chip_analog_gamma_reg_data is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_analog_gamma_data: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}								
int ic_mgr_write_fix_analog_gamma_data(int mipi_channel)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_write_chip_fix_analog_gamma_reg_data)
			{
				val = p_current_chip->fn_write_chip_fix_analog_gamma_reg_data(channel, mipi_channel,  
											s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_write_fix_analog_gamma_data error: p_current_chip->fn_write_chip_fix_analog_gamma_reg_data is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_analog_gamma_data: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_write_digital_gamma_data(int mipi_channel, unsigned char *p_digital_gamma_reg_data,
									int digital_gamma_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_write_chip_digital_gamma_reg_data)
			{
				val = p_current_chip->fn_write_chip_digital_gamma_reg_data(channel, mipi_channel, 
											p_digital_gamma_reg_data, digital_gamma_reg_data_len, 
											s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_write_digital_gamma_data error: p_current_chip->fn_write_chip_digital_gamma_reg_data is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_digital_gamma_data: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}
									
int ic_mgr_write_fix_digital_gamma_data(int mipi_channel)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_write_chip_fix_digital_gamma_reg_data)
			{
				val = p_current_chip->fn_write_chip_fix_digital_gamma_reg_data(channel, mipi_channel, 
											s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_write_fix_digital_gamma_data error: p_current_chip->fn_write_chip_fix_digital_gamma_reg_data is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_digital_gamma_data: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_d3g_control(int mipi_channel, int enable)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_chip_d3g_control)
			{
				val = p_current_chip->fn_chip_d3g_control(channel, mipi_channel, enable, 
														s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_d3g_control error: p_current_chip->fn_chip_d3g_control is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_write_digital_gamma_data: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_burn_gamma_otp_values(int mipi_channel, int burn_flag, int enable_burn_vcom, int vcom, 
									unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
									unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_chip_burn_gamma_otp_values)
			{
				val = p_current_chip->fn_chip_burn_gamma_otp_values(channel, mipi_channel, burn_flag, 
																enable_burn_vcom, vcom, 
																p_analog_gamma_reg_data, analog_gamma_reg_data_len,
																p_d3g_r_reg_data, d3g_r_reg_data_len,
																p_d3g_g_reg_data, deg_g_reg_data_len,
																p_d3g_b_reg_data, deg_b_reg_data_len,
																s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_burn_gamma_otp_values error: p_current_chip->fn_chip_burn_gamma_otp_values is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_burn_gamma_otp_values: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_check_gamma_otp_values(int mipi_channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
									unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
									unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_chip_check_gamma_otp_values)
			{
				val = p_current_chip->fn_chip_check_gamma_otp_values(channel, mipi_channel, enable_burn_vcom, 
																new_vcom_value, last_vcom_otp_times,
																p_analog_gamma_reg_data, analog_gamma_reg_data_len,
																p_d3g_r_reg_data, d3g_r_reg_data_len,
																p_d3g_g_reg_data, deg_g_reg_data_len,
																p_d3g_b_reg_data, deg_b_reg_data_len,
																s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_check_gamma_otp_values error: p_current_chip->fn_chip_check_gamma_otp_values is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_check_gamma_otp_values: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}


int ic_mgr_get_analog_gamma_reg_data(int mipi_channel, unsigned char *p_analog_gamma_reg_data, 
										int *p_analog_gamma_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_chip_get_analog_gamma_reg_data)
			{
				val = p_current_chip->fn_chip_get_analog_gamma_reg_data(channel, mipi_channel, 
																p_analog_gamma_reg_data, p_analog_gamma_reg_data_len,
																s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_get_analog_gamma_reg_data error: p_current_chip->fn_chip_get_analog_gamma_reg_data is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_check_gamma_otp_values: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}

int ic_mgr_get_digital_gamma_reg_data_t(int mipi_channel,
									unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_chip_get_digital_gamma_reg_data)
			{
				val = p_current_chip->fn_chip_get_digital_gamma_reg_data(channel, mipi_channel, 
																p_d3g_r_reg_data, p_d3g_r_reg_data_len,
																p_d3g_g_reg_data, p_d3g_g_reg_data_len,
																p_d3g_b_reg_data, p_d3g_b_reg_data_len,
																s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_get_digital_gamma_reg_data_t error: p_current_chip->fn_chip_get_digital_gamma_reg_data is NULL!\n");
			}
		}
		else
		{
			printf("ic_mgr_get_digital_gamma_reg_data_t: p_current_chip is NULL!\n");
		} 
	}
	
	return val;
}


int ic_mgr_read_chip_id_otp_info(int mipi_channel, unsigned char* p_id_data, int *p_id_data_len, unsigned char* p_otp_times)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_get_chip_id_otp_info)
			{
				printf("fn_get_chip_id_otp_info\n");
	 			val = p_current_chip->fn_get_chip_id_otp_info(channel, mipi_channel, p_id_data, p_id_data_len, 
													p_otp_times, s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_read_chip_id_otp_info error: p_current_chip->fn_get_chip_id_otp_info is NULL!\n");
			}
				
		}
		else
		{
			printf("ic_mgr_read_chip_id_otp_info: p_current_chip is NULL!\n");
		}
	}
	else
	{
		printf("ic_mgr_read_chip_id_otp_info error: channel = %d.\n", channel);
	}
	
	return val;
}

int ic_mgr_burn_chip_id(int mipi_channel, unsigned char* p_id_data, int id_data_len, int ptn_id)
{
	int val = -1;
	int channel = get_chip_channel_by_mipi_channel(mipi_channel);	
	if (channel >= 0 && channel < s_chip_channel_nums)
	{
		if (p_current_chip)
		{
			if (p_current_chip->fn_get_chip_id_otp_info)
			{
	 			val = p_current_chip->fn_burn_chip_id(channel, mipi_channel, p_id_data, id_data_len, ptn_id,
													s_chip_channel_data[channel].chip_private_data);
			}
			else
			{
				printf("ic_mgr_burn_chip_id error: p_current_chip->fn_burn_chip_id is NULL!\n");
			}
				
		}
		else
		{
			printf("ic_mgr_burn_chip_id: p_current_chip is NULL!\n");
		}
	}
	
	return val;
}

