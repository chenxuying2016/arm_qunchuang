
#include <stdio.h>

#include "json/cJSON.h"
#include "packsocket/packsocket.h"

#include "recvcmd.h"
#include "net_general_command.h"
#include "scanDev.h"
#include "client.h"


int client_send_gen_cmd_data(socket_cmd_class_t* p_socket_obj, int socket_fd, unsigned char* p_send_data, int send_len)
{
	int ipnum = 0;
	
	if (p_socket_obj)
	{		
		p_socket_obj->sendSocketCmd(socket_fd, CLIENT_MSG_GENERAL_COMMAND_ACK, ipnum, 0, 0, p_send_data, send_len);
	}

	return 0;
}

void client_send_gen_cmd_ack_read_cabc_end(socket_cmd_class_t* p_socket_obj, int socket_fd, int channel, 
											unsigned short cabc_freq, unsigned short cabc_duty) 
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	
	cJSON_AddNumberToObject(item, "gen_cmd_id", E_NET_GEN_CMD_READ_CABC_PWM_ACK);	
	cJSON_AddStringToObject(item, "gen_cmd_tag", "gen_ack_read_cabc_info");
	
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "cabc_freq", cabc_freq);
	cJSON_AddNumberToObject(item, "cabc_duty", cabc_duty);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	client_send_gen_cmd_data(p_socket_obj, socket_fd, out, strlen(out));
	
	free(out);
}

void client_send_gen_cmd_read_lcd_id_notify(socket_cmd_class_t* p_socket_obj, int socket_fd, int channel, 
											unsigned char *p_id_data, int id_data_len, unsigned char otp_times) 
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	
	cJSON_AddNumberToObject(item, "gen_cmd_id", E_NET_GEN_CMD_READ_DISPLAY_ID_NOTIFY);	
	cJSON_AddStringToObject(item, "gen_cmd_tag", "gen_ack_read_display_id");
	
	cJSON_AddNumberToObject(item, "channel", channel);

	int i = 0;
	int temp_id_data[32] = { 0 };
	for (i = 0; i < id_data_len; i++)
	{
		temp_id_data[i] = (int)p_id_data[i];
	}
	
	cJSON* json_id_array = cJSON_CreateIntArray((const int *)temp_id_data, id_data_len);
	
	cJSON_AddItemToObject(item, "id_array", json_id_array);	
	cJSON_AddNumberToObject(item, "otp_times", otp_times);
	
	char *out = cJSON_Print(item);

	//printf("client_send_gen_cmd_read_lcd_id_notify: \n%s\n", out);
	cJSON_Delete(item);
	
	client_send_gen_cmd_data(p_socket_obj, socket_fd, out, strlen(out) + 1);
	
	free(out);
}

int net_general_command_proc(socket_cmd_class_t* p_socket_obj, int socket_fd, char* p_json_command_data, int json_command_data_len)
{
	if (p_json_command_data == NULL)
	{
		printf("net_general_command_proc error: Invalid Param! p_json_command_data = NULL!\n");
		return -1;
	}

	if (json_command_data_len <= 0)
	{
		printf("net_general_command_proc error: Invalid Param! json_command_data_len = %d!\n", 
				json_command_data_len);
		return -2;
	}

	//printf("net command: \n%s\n", p_json_command_data);
	cJSON *json_root = cJSON_Parse(p_json_command_data);
	cJSON *json_gen_cmd_id = cJSON_GetObjectItem(json_root, "gen_cmd_id");
	cJSON *json_gen_cmd_tag = cJSON_GetObjectItem(json_root, "gen_cmd_tag");

	if (json_root == NULL)
	{		
		printf("net_general_command_proc error: Invalid Param! cJSON_Parse root failed!\n");
		return -3;
	}

	if (json_gen_cmd_id == NULL)
	{		
		printf("net_general_command_proc error: Invalid Param! json_gen_cmd_id = NULL!\n");
		cJSON_Delete(json_root);
		return -4;
	}

	int gen_cmd = json_gen_cmd_id->valueint;

	#if 0
	if (json_gen_cmd_tag)
	{
		//printf("net_general_command_proc: cmd_id: %d, 0x%08x, %s.\n", gen_cmd, gen_cmd, json_gen_cmd_tag->valuestring);
		printf("net_general_command_proc: cmd_id: %d, 0x%08x, %s.\n", gen_cmd, gen_cmd, json_gen_cmd_tag->valuestring);
	}
	#endif
	
	printf("net_general_command_proc: cmd_id: %d, 0x%08x\n", gen_cmd, gen_cmd);
	switch(gen_cmd)
	{
		case E_NET_GEN_CMD_READ_CABC_PWM_REQ:
			{
				int channel = 1;
				cJSON *json_channel = cJSON_GetObjectItem(json_root, "channel");
				
				unsigned short freq = 0;
				unsigned short duty;
				int mipi_channel = 1;

				if (json_channel)
					mipi_channel = json_channel->valueint;

				int cabc_channel = 1;

				if (mipi_channel == 1)
				{
					//cabc_channel = 3;	// for hotel device.
					cabc_channel = 1;	// for factory device.
				}
				else if (mipi_channel == 4)
				{
					cabc_channel = 4;
				}
					
				qc_cabc_test(cabc_channel, &freq, &duty);
				usleep(1 * 1000);
				qc_cabc_test(cabc_channel, &freq, &duty);

				printf("== cabc pwm: freq = %d, duty = %d.\n", freq, duty);
				
				// send ack 			
				// E_NET_GEN_CMD_READ_CABC_PWM_ACK;
				client_send_gen_cmd_ack_read_cabc_end(p_socket_obj, socket_fd, channel, freq, duty);
			}		
			break;

		case E_NET_GEN_CMD_SET_IP_INFO_REQ:
			{
				cJSON *json_ip = cJSON_GetObjectItem(json_root, "ip");
				cJSON *json_netmask = cJSON_GetObjectItem(json_root, "net_mask");
				cJSON *json_gateway = cJSON_GetObjectItem(json_root, "gateway");

				if (json_ip && json_netmask && json_gateway)
				{
					printf("E_NET_GEN_CMD_SET_IP_INFO_REQ: ip: %s, netmask: %s, gateway: %s.\n",
							json_ip->valuestring, json_netmask->valuestring, json_gateway->valuestring);
					#if 1
					net_work_config_save(json_ip->valuestring, json_netmask->valuestring, json_gateway->valuestring);
					#endif

					system("sync");
					system("reboot");
				}
			}
			break;
		
		case E_NET_GEN_CMD_SET_MAC_INFO_REQ:
			{
				cJSON *json_mac = cJSON_GetObjectItem(json_root, "mac");

				if (json_mac)
				{
					printf("E_NET_GEN_CMD_SET_MAC_INFO_REQ: mac: %s.\n", json_mac->valuestring);
			#if 1
					net_work_mac_config_save(json_mac->valuestring);
			#endif

					system("sync");
					system("reboot");
				}
			}
			break;

		case E_NET_GEN_CMD_DO_FACOTRY_RESET_REQ:
			{
				printf("E_NET_GEN_CMD_DO_FACOTRY_RESET_REQ: do factory reset ...\n");
				do_factory_reset();

				system("sync");
				system("reboot");
			}
			break;

		case E_NET_GEN_CMD_POWER_CALIBRATION_REQ:
			{
				printf("E_NET_GEN_CMD_POWER_CALIBRATION_REQ:\n");

				// channel: master = 3. slaver 2;
				cJSON *json_power_channel = cJSON_GetObjectItem(json_root, "power_channel");
				cJSON *json_power_type = cJSON_GetObjectItem(json_root, "power_type");
				cJSON *json_power_data = cJSON_GetObjectItem(json_root, "power_data");

				if (json_power_channel == NULL)
				{
					printf("json_power_channel == NULL!\n");
					break;
				}

				if (json_power_type == NULL)
				{
					printf("json_power_type == NULL!\n");
					break;
				}

				if (json_power_data == NULL)
				{
					printf("json_power_data == NULL!\n");
					break;
				}

				unsigned char power_channel = json_power_channel->valueint;
				unsigned char power_type = json_power_type->valueint;
				int power_data = json_power_data->valueint;

				//printf("power data: %d.\n", power_data);

				if (power_channel == 2)
					power_channel = 3;	// master
				else
					power_channel = 2;	// slave	

				//if (power_type == 2)
				{
					power_cmd_calibration_power_item(power_channel, power_type, 1, power_data);
				}
				usleep(20 * 1000);
			}
			break;

		case E_NET_GEN_CMD_POWER_SETING_BURN_REQ:
			{
				printf("E_NET_GEN_CMD_POWER_SETING_BURN_REQ:\n");
				cJSON *json_power_channel = cJSON_GetObjectItem(json_root, "power_channel");
				cJSON *json_power_type = cJSON_GetObjectItem(json_root, "power_type");

				if (json_power_channel == NULL)
				{
					printf("json_power_channel == NULL!\n");
					break;
				}

				unsigned char power_channel = json_power_channel->valueint;

				if (power_channel == 2)
					power_channel = 2;	// slave
				else
					power_channel = 3;	// master	
					
				power_cmd_calibration_power_item(power_channel, 0, 2, 0);

				usleep(20 * 1000);
			}
			break;
			
		case E_NET_GEN_CMD_START_SCAN_REQ:
		{
			printf("E_NET_GEN_CMD_START_SCAN_REQ\r\n");
			scanDevInit(client_send_scan_result);
			scanDevStartScan();

#if 0
			char result[] = "Lx23414432";
			client_send_scan_result(result, strlen(result));
#endif
		}
			break;

		case E_NET_GEN_CMD_CA310_READ_FLICK:
		{
			printf("E_NET_GEN_CMD_CA310_READ_FLICK\r\n");
			if (json_root)
			{
				cJSON *pElem = cJSON_GetObjectItem(json_root, "channel");
				if (pElem)
				{
					int channel = pElem->valueint;
					float flickValue = 0.0;
					usleep(500*1000);
					ca210_capture_flick_data(channel, &flickValue);
					client_send_flick_read_result(channel, flickValue);
				}
			}
		}
		break;

		case E_NET_GEN_CMD_SET_BOMB_REQ:
		{
			printf("E_NET_GEN_CMD_SET_BOMB_REQ\r\n");
			if (json_root)
			{
				unsigned int enable = 0;
				unsigned int type = 0;
				unsigned int red = 0;
				unsigned int green = 0;
				unsigned int blue = 0;
				unsigned int pos_x = 0;
				unsigned int pos_y = 0;

				cJSON *pElem = cJSON_GetObjectItem(json_root, "enable");
				if (pElem)
					enable = pElem->valueint;

				unsigned short value = 0;

				pElem = cJSON_GetObjectItem(json_root, "type");
				if (pElem)
					type = pElem->valueint;

				if (enable)
				{
					pElem = cJSON_GetObjectItem(json_root, "r");
					if (pElem)
						red = pElem->valueint;

					pElem = cJSON_GetObjectItem(json_root, "g");
					if (pElem)
						green = pElem->valueint;

					pElem = cJSON_GetObjectItem(json_root, "b");
					if (pElem)
						blue = pElem->valueint;

					unsigned int color = (red & 0xf) | (green & 0xf) << 9 | (blue & 0xf) << 18;
					fpga_write(0x42, color&0xffff);
					fpga_write(0x43, (color >> 16)&0xffff);

					pElem = cJSON_GetObjectItem(json_root, "x");
					if (pElem)
						pos_x = pElem->valueint;

					fpga_write(0x49, pos_x);

					pElem = cJSON_GetObjectItem(json_root, "y");
					if (pElem)
						pos_y = pElem->valueint;

					fpga_write(0x4a, pos_y);

					value = fpga_read(0x46);
					value = value | 0x1 << (type + 6);	// type: 0-point; 1-H_line; 2-V_line
					fpga_write(0x46, value);
				}
				else
				{
					value = fpga_read(0x46);
					value = value & ~(0x1 << (type + 6)); // type: 0-point; 1-H_line; 2-V_line
					fpga_write(0x46, value);
				}
			}
			break;
		}

		case E_NET_GEN_CMD_PAINT_RECTANGLE:
		{
			printf("E_NET_GEN_CMD_PAINT_RECTANGLE\r\n");
			if (json_root)
			{
				unsigned int enable = 0;
				unsigned int pos_x = 0;
				unsigned int pos_y = 0;
				unsigned int pos_w = 0;
				unsigned int pos_h = 0;

				cJSON *pElem = cJSON_GetObjectItem(json_root, "enable");
				if (pElem)
					enable = pElem->valueint;

				unsigned short value = 0;

				pElem = cJSON_GetObjectItem(json_root, "x");
				if (pElem)
					pos_x = pElem->valueint;

				pElem = cJSON_GetObjectItem(json_root, "y");
				if (pElem)
					pos_y = pElem->valueint;

				pElem = cJSON_GetObjectItem(json_root, "w");
				if (pElem)
					pos_w = pElem->valueint;

				pElem = cJSON_GetObjectItem(json_root, "h");
				if (pElem)
					pos_h = pElem->valueint;

				if (enable)
				{
					fpga_write(0xf4, pos_x);
					fpga_write(0xf5, pos_y);
					fpga_write(0xf6, pos_w);
					fpga_write(0xf7, pos_h);

					value = fpga_read(0x46);
					value = value | (0x1 << 9);
					fpga_write(0x46, value);
				}
				else
				{
					value = fpga_read(0x46);
					value = value & (~(0x1 << 9));
					fpga_write(0x46, value);
				}
			}
			break;
		}

		default:
			printf("net_general_command_proc error: Invalid gen_cmd.\n");
			break;
	}

	cJSON_Delete(json_root);
	return 0;
}
