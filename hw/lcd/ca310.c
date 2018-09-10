#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <io.h>
#include "common.h"

#ifdef USE_Z_CA210_LIB
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <termios.h>
#include <unistd.h>

#include "ca310.h"

enum {
	E_CA210_WORK_MODE_IDLE = 0,
	E_CA210_WORK_MODE_XYLV,
	E_CA210_WORK_MODE_FLICK,
};

#define MAX_LCD_PROBE_NUMS			(2)

#define MAX_CA210_DEV_NAME_LEN		(32)
typedef struct tag_ca210_probe_info
{
	int	lcd_channel;
	int uart_fd;
	char dev_name[MAX_CA210_DEV_NAME_LEN];
	int work_mode;
}ca210_probe_info_t;

static ca210_probe_info_t s_ca210_probe[MAX_LCD_PROBE_NUMS] = { 0 };

static int s_uart_speed_arr[] =
{
	B38400, B19200, B9600, B4800, B2400, B1200, B300, B115200, B19200, B9600,
	B4800, B2400, B1200, B300,
};

static int s_uart_speed_name_arr[] =
{
	38400, 19200, 9600, 4800, 2400, 1200, 300, 115200,
	19200, 9600, 4800, 2400, 1200, 300,
};


int uart_open(char *p_dev_name)
{
	int fd_uart = -1;
	fd_uart = open(p_dev_name, O_RDWR | O_NDELAY | O_NOCTTY);
	if (-1 == fd_uart)
	{
		printf("uart_open error: Can't Open Serial Port %s\n", p_dev_name);
		return -1;
	}
	
	return fd_uart;
}

int uart_close(int uart_fd)
{
	close(uart_fd);
	return 0;
}

int uart_set_parity(int uart_fd, int databits, int stopbits, int parity)
{
	struct termios options = { 0 };

	if (tcgetattr(uart_fd, &options) != 0)
	{
		printf("uart_set_parity error: tcgetattr error!\n");
		return -1;
	}

	options.c_cflag &= ~CSIZE;

	switch (databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;
			
		case 8:
			options.c_cflag |= CS8;
			break;
			
		default:
			printf("uart_set_parity error: Unsupported data size: %d. \n", databits);
			return (-1);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	/* Enable parity checking */
			break;
			
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);	/* 设置为奇效验 */
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
			
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;	/* Enable parity */
			options.c_cflag &= ~PARODD;	/* 转换为偶效验 */
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
			
		case 'S':
		case 's':				/*as no parity */
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
			
		default:
			printf("uart_set_parity error: Unsupported parity: %d.\n", parity);
			return (-1);
	}

	/* 设置停止位 */
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
			
		case 2:
			options.c_cflag |= CSTOPB;
			break;
			
		default:
			printf("uart_set_parity error: Unsupported stop bits: %d.n", stopbits);
			return (-1);
	}

	/* Set input parity option */
	if (parity != 'n')
	{ 
		options.c_iflag |= INPCK; 
	}

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | INLCR | ICRNL | IEXTEN);
	//Choosing   Raw   Input
	//options.c_oflag   &=~(OCRNL|ONLCR|ONLRET);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~(ONLCR | OCRNL);	//添加的
	options.c_iflag &= ~(ICRNL | INLCR);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);	//添加的
	
	tcflush(uart_fd, TCIOFLUSH);
	options.c_cflag &= ~CRTSCTS;
	options.c_cc[VTIME] = 0;	/* 设置超时15 seconds */
	options.c_cc[VMIN] = 0;		/* Update the options and do it NOW */

	if (tcsetattr(uart_fd, TCSANOW, &options) != 0)
	{
		printf("uart_set_parity error: tcsetattr error!\n");
		return (-1);
	}

	return (0);
}
 
int uart_set_speed(int uart_fd, int speed)
{
	int i = 0;
	int status = 0;
	struct termios Opt = { 0 };
	tcgetattr(uart_fd, &Opt);

	for (i = 0; i < sizeof(s_uart_speed_arr) / sizeof(int); i++)
	{
		if (speed == s_uart_speed_name_arr[i])
		{
			tcflush(uart_fd, TCIOFLUSH);
			cfsetispeed(&Opt, s_uart_speed_arr[i]);
			cfsetospeed(&Opt, s_uart_speed_arr[i]);
			status = tcsetattr(uart_fd, TCSANOW, &Opt);

			if (status != 0)
			{
				printf("uart_set_speed error: tcsetattr failed!\n");
				return -1;
			}

			tcflush(uart_fd, TCIOFLUSH);
			break;
		}
	}
	
	return 0;
}

int uart_read(int uart_fd, struct timeval *p_timeout, unsigned char *pOutBuf)
{
	int ret = -1;
	int maxFd = 0;
	fd_set readFds;
	
	#define MAX_BUFF_SIZE 2048
	unsigned char buf[MAX_BUFF_SIZE] = { 0 };

	/* 判断输出buff是否为NULL */
	if (pOutBuf == NULL)
	{
		printf("uart_read Error: Output buff is NULL!\r\n");
		return -3;
	}

    //printf("uart fd is %d\n",gsUartInfo[serial].fd);
	FD_ZERO(&readFds);
	FD_SET(uart_fd, &readFds);

	ret = select(uart_fd + 1, &readFds, NULL, NULL, p_timeout);
	if (ret > 0)
	{
		if (FD_ISSET(uart_fd, &readFds))
		{
			ret = read(uart_fd, buf, MAX_BUFF_SIZE);
			if (ret > 0)
				memcpy(pOutBuf, buf, ret);
		}
	}

	return ret;
}

int uart_write(int uart_fd, unsigned char *p_data, int data_len)
{
	int slen = write(uart_fd, p_data, data_len);
  if(slen <= 0)
  {
    printf("\n write data error: ret = %d.\n", slen);
		return 0;
  }

	return slen;
}

int uart_read_end(int uart_fd, char* p_read_buf, int *p_read_len)
{
	int ret = 0;
	unsigned char buf[1024] = { 0 };
	int offset = 0;
		
	struct timeval time_out = { 0 };
	time_out.tv_sec = 2;
	time_out.tv_usec = 0;
	
	while (ret >= 0)
	{
		ret = uart_read(uart_fd, &time_out, buf + offset);
		if (ret > 0)
		{
			offset += ret;

			if (buf[offset -1] == '\r')
			{
				break;
			}
			
			continue;
		}
		else
		{
			break;
		}
	}
	
	//printf("read data len = %d.\n", offset);
	//printf("%s\n", buf);
	if (offset > 0)
	{
		memcpy(p_read_buf, buf, offset);
	}

	*p_read_len = offset;
	
	return offset;
}

int ca210_test(char* p_dev)
{
	//#define Z_UART_DEV		"/dev/ttyUSB1"
	#define SYS_CMD_LEN			(8)
	char cmd[] = "COM,0\n";
	
	int uart_fd = -1;
	int ret = -1;
	
	printf("ca210_test: %s\n", p_dev);
	
	// open uart
	uart_fd = uart_open(p_dev);
	if (uart_fd > 0)
	{
		// CA210
		ret = uart_set_parity(uart_fd, 7, 2, 'E');		
		ret = uart_set_speed(uart_fd, 38400);
		
		ret = uart_write(uart_fd, cmd, strlen(cmd));
		printf("Write len = %d.\ncmd: %s", ret, cmd); 
		
		unsigned char buf[1024] = "";
		int buf_len = 1024;		
		ret = uart_read_end(uart_fd, buf, &buf_len);		
		printf("Read: %s\n", buf);
		
		// close uart
		uart_close(uart_fd);
	}
	return 0;
}


int ca210_reset(int lcd_channel)
{
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_reset error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}
	
	//ca210_close(lcd_channel);
	memset(s_ca210_probe + lcd_channel, 0, sizeof(ca210_probe_info_t));
	s_ca210_probe[lcd_channel].uart_fd = -1;
	return 0;
}

int ca210_open(int lcd_channel, char* p_dev_name)
{
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_open error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}

	printf("== ca210_open: lcd channel: %d, dev: %s.\n", lcd_channel, p_dev_name);
	s_ca210_probe[lcd_channel].uart_fd = uart_open(p_dev_name);
	strcpy(s_ca210_probe[lcd_channel].dev_name, p_dev_name);
	s_ca210_probe[lcd_channel].lcd_channel = lcd_channel;
	s_ca210_probe[lcd_channel].work_mode = E_CA210_WORK_MODE_IDLE;
	return 0;
}

int ca210_close(int lcd_channel)
{
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_close error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}
	
	if (s_ca210_probe[lcd_channel].uart_fd > 0)
	{
		uart_close(s_ca210_probe[lcd_channel].uart_fd);

		s_ca210_probe[lcd_channel].uart_fd = -1;
		s_ca210_probe[lcd_channel].work_mode = E_CA210_WORK_MODE_IDLE;
	}
	
	return 0;
}

#define CMD_OK_KEY			"OK00"
#define CMD_OK_01_KEY		"OK0"

#define FMT_FLICK_DATA		"OK00,P%d  %f"

#define FMT_XYLV_DATA		"OK%d,P%d %d;%d; %lf"
#define FMT_XYLV1_DATA		"OK%*d,P%d %d;%d; %lf"


// error
#define ERR_INVALID_COMMAND	"ER10"
#define ERR_VALUE_OUT		"ER50"
#define ERR_LOW_LUMI		"ER52"

char str_cmd_mode_remote_on[] = "COM,2\n";
char str_cmd_mode_remote_off[] = "COM,0\n";
char str_cmd_sync[] = "SCS,3\n";
char str_cmd_mds_flick[] = "MDS,6\n";
char str_cmd_mds_xylv[] = "MDS,0\n";

char str_cmd_meas[] = "MES\n";

int ca310_uart_open(int channel)
{
	if(channel <= 0)
		return -1;

	channel -= 1;

	if (channel == 0)
	{
		s_ca210_probe[channel].uart_fd = uart_open("/dev/ttyUSB0");
		printf("********************fd = %d, name:%s\r\n", s_ca210_probe[channel].uart_fd, "/dev/ttyUSB0");
	}
	else
	{
		s_ca210_probe[channel].uart_fd = uart_open("/dev/ttyUSB1");
		printf("********************fd = %d, name:%s\r\n", s_ca210_probe[channel].uart_fd, "/dev/ttyUSB1");
	}

	uart_set_parity(s_ca210_probe[channel].uart_fd, 7, 2, 'E');
	uart_set_speed(s_ca210_probe[channel].uart_fd, 38400);

	return 0;
}

void ca310_uart_close(int channel)
{
	if(channel <= 0)
		return;

	channel -= 1;

	if(s_ca210_probe[channel].uart_fd >= 0)
	{
		close(s_ca210_probe[channel].uart_fd);
		s_ca210_probe[channel].uart_fd = -1;
	}
}

int ca210_start_flick_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;
	
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_start_flick_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	// COM,2
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_mode_remote_on, 
					strlen(str_cmd_mode_remote_on));

	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_start_flick_mode error: remote on => %s.\n", buf);
			return PROBE_STATU_OTHER_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_start_flick_mode: remote on => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// SCS,3
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_sync, strlen(str_cmd_sync));
	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_start_flick_mode error: sync => %s.\n", buf);
			return PROBE_STATU_OTHER_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_start_flick_mode:  sync => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// MDS,6
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_mds_flick, 
					strlen(str_cmd_mds_flick));

	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_start_flick_mode error: mds: flick => %s.\n", buf);
			return PROBE_STATU_OTHER_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_start_flick_mode:  mds: flick => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	s_ca210_probe[lcd_channel].work_mode = E_CA210_WORK_MODE_FLICK;
	
	return PROBE_STATU_OK;
}

int ca210_stop_flick_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_stop_flick_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}
	
	// COM,0
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_mode_remote_off, 
					strlen(str_cmd_mode_remote_off));

	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_stop_flick_mode error: remote off => %s.\n", buf);
			return -1;
		}
	}
	else
	{
		// timeout.
		printf("ca210_stop_flick_mode: remote off => timeout!\n", buf);
		return -1;
	}

	s_ca210_probe[lcd_channel].work_mode = E_CA210_WORK_MODE_IDLE;
	
	return PROBE_STATU_OK;
}

int ca210_capture_flick_data(int lcd_channel, float *p_f_flick)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if (p_f_flick == NULL)
	{
		printf("ca210_capture_flick_data error: invalid param: p_f_flick = NULL.\n");
		return PROBE_STATU_OTHER_ERROR;
	}

	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_capture_flick_data error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	*p_f_flick = FLOAT_INVALID_FLICK_VALUE;
	
	// MES
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_meas, strlen(str_cmd_meas));
	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			if (strncmp(ERR_INVALID_COMMAND, buf, strlen(CMD_OK_KEY)) == 0)
			{
				printf("ca210_stop_flick_mode error: command error! meas => %s.\n", buf);
				return PROBE_STATU_COMMAND_ERROR;
			}

			if (strncmp(ERR_VALUE_OUT, buf, strlen(ERR_VALUE_OUT)) == 0)
			{
				printf("ca210_stop_flick_mode error: out range! meas => %s.\n", buf);
				return PROBE_STATU_OUT_RANGE;
			}

			if (strncmp(ERR_LOW_LUMI, buf, strlen(ERR_LOW_LUMI)) == 0)
			{
				printf("ca210_stop_flick_mode error: low lumi! meas => %s.\n", buf);
				return PROBE_STATU_LOW_LUMI;
			}
			else
			{
				printf("ca210_stop_flick_mode error: meas => %s.\n", buf);
			}
			
			return PROBE_STATU_OTHER_ERROR;
		}
		else
		{
			int porbe_channel = 0;			
			sscanf(buf, FMT_FLICK_DATA, &porbe_channel, p_f_flick);
			printf("capture flick data: %f.\n", *p_f_flick);
		}
	}
	else
	{
		// timeout.
		printf("ca210_stop_flick_mode: meas => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	//OK00,P1  10.6
	return PROBE_STATU_OK;
}

int ca210_start_xylv_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;
	
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_start_xylv_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	// COM,2
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_mode_remote_on, 
					strlen(str_cmd_mode_remote_on));

	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_start_xylv_mode error: remote on => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_start_xylv_mode: remote on => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// SCS,3
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_sync, strlen(str_cmd_sync));
	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_start_xylv_mode error: sync => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_start_xylv_mode:  sync => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// MDS,0
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_mds_xylv, 
					strlen(str_cmd_mds_xylv));

	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_start_xylv_mode error: mds: xylv => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_start_xylv_mode:  mds: xylv => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	s_ca210_probe[lcd_channel].work_mode = E_CA210_WORK_MODE_XYLV;
	
	return PROBE_STATU_OK;
}

int ca210_stop_xylv_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_stop_xylv_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}
	
	// COM,0
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_mode_remote_off, 
					strlen(str_cmd_mode_remote_off));

	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca210_stop_xylv_mode error: remote off => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca210_stop_xylv_mode: remote off => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	s_ca210_probe[lcd_channel].work_mode = E_CA210_WORK_MODE_XYLV;
	
	return PROBE_STATU_OK;
}

int ca210_capture_xylv_data(int lcd_channel, double *p_f_x, double *p_f_y, double *p_f_lv)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if ( (p_f_x == NULL) || (p_f_y == NULL) || (p_f_lv == NULL) )
	{
		printf("ca210_capture_xylv_data error: invalid param: p_f_x or p_f_y or p_f_lv = NULL.\n");
		return PROBE_STATU_OTHER_ERROR;
	}
	
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca210_capture_xylv_data error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	// MES
	ret = uart_write(s_ca210_probe[lcd_channel].uart_fd, str_cmd_meas, strlen(str_cmd_meas));
	ret = uart_read_end(s_ca210_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if ( strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0 
				&& strncmp(CMD_OK_01_KEY, buf, strlen(CMD_OK_01_KEY)) != 0)
		{
			printf("ca210_capture_xylv_data error: meas => %s.\n", buf);

			if (strncmp(ERR_INVALID_COMMAND, buf, strlen(CMD_OK_KEY)) == 0)
			{
				printf("ca210_capture_xylv_data error: command error! meas => %s.\n", buf);
				return PROBE_STATU_COMMAND_ERROR;
			}

			if (strncmp(ERR_VALUE_OUT, buf, strlen(ERR_VALUE_OUT)) == 0)
			{
				printf("ca210_capture_xylv_data error: out range! meas => %s.\n", buf);
				return PROBE_STATU_OUT_RANGE;
			}

			if (strncmp(ERR_LOW_LUMI, buf, strlen(ERR_LOW_LUMI)) == 0)
			{
				printf("ca210_capture_xylv_data error: low lumi! meas => %s.\n", buf);
				return PROBE_STATU_LOW_LUMI;
			}
			else
			{		
				printf("ca210_capture_xylv_data other error: meas => %s.\n", buf);
			}
			
			return PROBE_STATU_OTHER_ERROR;
		}
		else
		{
			int porbe_channel = 0;	
			int x = 0;
			int y = 0;
			int err = 0;

			sscanf(buf, FMT_XYLV_DATA, &err, &porbe_channel, &x, &y, p_f_lv);
			//sscanf(buf, FMT_XYLV1_DATA, &porbe_channel, &x, &y, p_f_lv);

			*p_f_x = x / 10000.00;
			*p_f_y = y / 10000.00;

			#ifdef ENABLE_PROBE_DEBUG
			printf("capture data: \n%s\n. err: %d, x: %d, y: %d, lv: %f.\n", buf, err, x, y, *p_f_lv);
			printf("capture xylv data: %f, %f, %f\n", *p_f_x, *p_f_y, *p_f_lv);
			#endif
		}
	}
	else
	{
		// timeout.
		printf("ca210_capture_xylv_data: meas => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}
	
	return PROBE_STATU_OK;
}

#else

#define UART_BUF_SIZE 1100
#define MAX_BUFF_SIZE 2048

typedef struct __CA310_UART_BUF__
{
    unsigned long  uartrecvcnt ;
    unsigned long  uartleftdatalen ;
    BYTE  uartcatch[UART_BUF_SIZE]; // 串口缓存
    BYTE  uartbuff[MAX_BUFF_SIZE] ; // 串口剩余数据buff
    BYTE  *curuartptr ;
    BYTE  uartdo ;
}CA310_UART_BUF;

static const char zeroCAL[32] = {"ZRC\n"}; //Zero calibration
static const char comType[32]  = {"COM,2\n"}; //
//static const char memCH[32]    = {"MCH,97\n"};
static const char syncMode[32] = {"SCS,3\n"}; //UNIV mode
static const char showMode[32] = {"MDS,6\n"}; //Flicker mode
//OK00,P1 30.0\r

static const char flickGet[32] = {"MES\n"};
static const char stopCom[32]  = {"COM,0\n"};
static const char okRet[32]    = {"OK00\n"};

static const char xyLvGet[32]  = {"MES\n"};
static const char xyLvMode[32] = {"MDS,0\n"}; //xyLv mode
//OK00,P1 3130;3290;200.0\r

static const char xyzMode[32]  = {"MDS,7\n"}; //XYZ mode
//OK00,P1 175.7;209.7;75.34\r


static CA310_UART_BUF ca310UartBuf[4];
static int ca310fd[4] = {-1, -1, -1, -1};
static int ca310mode[4] = {-1, -1, -1, -1};

static int _ca310_uart_open(int channel,char *pUartName,int baud,int databit,int stopbit,char ecc)
{
    char aucDev[32] = "/dev/";

    strcat(aucDev, pUartName); /* 组串口节点名 */
 //   printf("Dev:%s\r\n", aucDev);
    ca310UartBuf[channel].uartdo = 0;
    ca310UartBuf[channel].uartrecvcnt = 0;
    ca310UartBuf[channel].uartleftdatalen = 0;
    ca310UartBuf[channel].uartleftdatalen = 0;
    ca310UartBuf[channel].curuartptr = ca310UartBuf[channel].uartcatch;
    ca310fd[channel] = uartOpenAndSetData((BYTE*)aucDev,baud,databit,stopbit,ecc); //INT uartOpenAndSetData(BYTE *devName,int baud,int databit,int stopbit,char ecc);

    printf("open uart fd %s %d\n",aucDev,ca310fd[channel]);
    return ca310fd[channel];
}

static INT _ca310_uart_read(int channel,struct timeval timeout,BYTE *pOutBuf)
{
    int ret = 0;
    INT maxFd;
    fd_set readFds;
    INT readlen = 0;
//    BYTE buf[MAX_BUFF_SIZE];

    /* 判断输出buff是否为NULL */
  /*  if (pOutBuf == NULL)
    {
        //traceMsg(UART_PRT_SW1,"Error! Output buff is NULL!\r\n");
        ret = -2;
        return ret;
    }*/

    maxFd = ca310fd[channel];
    FD_ZERO(&readFds);
    FD_SET(ca310fd[channel], &readFds);

    /* 处理上次未收完的数据 */
/*    if (ca310UartBuf[channel].uartleftdatalen > 0)
    {
        _ca310_uart_processData(channel,0,0,&ca310UartBuf[channel]);
    }*/

    ret = select(maxFd + 1, &readFds, NULL, NULL, &timeout);

    if (ret > 0)
    {
       // if (ca310fd[channel] >= 0)
       // {
            if (FD_ISSET(ca310fd[channel], &readFds))
            {
                readlen = read(ca310fd[channel], /*buf*/pOutBuf, MAX_BUFF_SIZE);
               /* #if 0
                int i;
                printf("uart recv buf data len: %d\n",ret);
                for (i = 0; i < readlen; i++)
                {
                    printf("0x%02x ", buf[i]);
                }
                printf("\n");
                #endif*/

            /*    if (readlen > 0)
                {
                    _ca310_uart_processData(channel, buf, readlen, &ca310UartBuf[channel]);
                }*/
            }
      //  }
       // else
       // {
            //traceMsg(UART_PRT_SW1,"ttyS%d is closed\n", serialNo);
       // }
    }
 /*   if(ca310UartBuf[channel].uartdo)
    {
        memcpy(pOutBuf,ca310UartBuf[channel].uartbuff,ca310UartBuf[channel].uartleftdatalen);
        ret = ca310UartBuf[channel].uartleftdatalen;
        ca310UartBuf[channel].uartleftdatalen = 0;
        ca310UartBuf[channel].uartdo = 0;
    }*/
    return readlen;
}

static INT ca310_uartWaitRecvData(int channel,struct timeval delaytime, BYTE *aucreadbuf, INT *nBufLen)
{
    long long tmp = 0;
    struct timeval start, stop;	//delay for 5sec until get the ringht packet
    INT ret = -1;
    INT readlen = 0;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50;
    gettimeofday(&start, 0);
	int aucreadbuf_len = 0;

    do
    {
        readlen = _ca310_uart_read(channel, timeout, aucreadbuf+aucreadbuf_len);
        if (readlen > 0)
        {
#if 1
			int i,isok = 0;
			for(i=aucreadbuf_len; i < (aucreadbuf_len+readlen); i++)
			{
				if(aucreadbuf[i] == '\r' || aucreadbuf[i] == '\n')
				{
					isok = 1;
					aucreadbuf[i] = 0;
					ret = 0;
					*nBufLen = i;
					break;
				}
			}
			if(isok) //读取结束
			{
				break;
			}
			else //还有
			{
				aucreadbuf_len += readlen;
			}
#else
            ret = 0;
            *nBufLen = readlen;
            break;
#endif
        }

        usleep(10 * 1000);
        gettimeofday(&stop, 0);
        tmp = (start.tv_sec * 1000000 + start.tv_usec + delaytime.tv_sec * 1000000 + delaytime.tv_usec) - (stop.tv_sec * 1000000 + stop.tv_usec);
    }while (tmp > 0);

    //ret = (0 == ret) ? 0 : 1 ;
//	printf("======== %s =======\n", aucreadbuf);
    return ret;
}

static INT ca310_uart_write(int channel,CHAR *pBuf , INT writeLen)
{
    INT ret=-1, len;
    /* 检查待发送数据是否为NULL */
    if (pBuf == NULL)
    {
        return ret;
    }

	//printf("[CA310 write:] %s\n", pBuf);
    len = write(ca310fd[channel], pBuf, writeLen);
    if(len>0)
    {
        ret = 0;
    }
    return ret;
}

int ca310_uart_open(int channel,char* pUartName)
{
	if(channel <= 0) return -1;
	channel -= 1;
	//memset(&ca310UartBuf[channel], 0, sizeof(CA310_UART_BUF));
	//
 //   struct timeval delaytime;
  //  delaytime.tv_sec = 3;
  //  delaytime.tv_usec = 0;
  //  char aucreadbuf[1024];
  //  int  nBufLen = 0;
 //   closeUart(2); //for
    return _ca310_uart_open(channel,pUartName/*"ttyUSB0"*/,38400,7,2,'E');
    /*ca310_uart_write(comType,strlen(comType));
    ca310_uartWaitRecvData(delaytime, aucreadbuf, &nBufLen);
    if(nBufLen>0)
    {
        printf("read back %d\n",nBufLen);
    }*/
}

void ca310_uart_close(int channel)
{
	if(channel <= 0) return;
	channel -= 1;

	//
	if(ca310fd[channel] != -1)
	{
		printf("close uart fd %d\n",ca310fd[channel]);
		close(ca310fd[channel]);
		ca310fd[channel] = -1;
	}
}

int ca310_uart_is_open(int channel)
{
	if(channel <= 0) return 0;
	channel -= 1;
	if(ca310fd[channel] == -1) return 0;
	return 1;
}


int ca310Transfer(int channel, int cmd, int nMode, float *pFlick, double* pX, double* pY, double* pLv)
{
	if(channel == 0) 
		return -1;
	
	channel -= 1;
	if(ca310fd[channel] == -1) 
		return -1;

	//
    INT ret = -1;
    int nBufLen = 0;
    BYTE aucreadbuf[1024] = {0};
    struct timeval delaytime;
    delaytime.tv_sec  = 2;
    delaytime.tv_usec = 0;
	
    if(LCD_CMD_FLICK_START == cmd) //打开 REMOTE
    {
		//检查是否已经打开过
		if(ca310mode[channel] != -1)
		{
			if(ca310mode[channel] == nMode) //模式相同
			{
				return 0;
			}
			else //不同, 则先关闭前面一个模式
			{
				ca310Transfer(channel, LCD_CMD_FLICK_END, ca310mode[channel], NULL, NULL,NULL,NULL);
			}
		}
		
		//
		if(nMode == 0) //Flicker mode
		{
			if (0 == ca310_uart_write(channel,comType, strlen(comType)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][START] send %s,return %s\n",channel,comType,aucreadbuf);
						return -1;
					}
				}
			}
			if (0 == ca310_uart_write(channel,syncMode, strlen(syncMode)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][START] send %s,return %s\n",channel,syncMode,aucreadbuf);
						return -1;
					}
				}
			}
			if (0 == ca310_uart_write(channel,showMode, strlen(showMode)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][START] send %s,return %s\n",channel,showMode,aucreadbuf);
						return -1;
					}
				}
			}
		}
        else if(nMode == 1) //xyLv mode
		{
			if (0 == ca310_uart_write(channel,comType, strlen(comType)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][START] send %s,return %s\n",channel,comType,aucreadbuf);
						return -1;
					}
				}
			}
			if (0 == ca310_uart_write(channel,syncMode, strlen(syncMode)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][START] send %s,return %s\n",channel,syncMode,aucreadbuf);
						return -1;
					}
				}
			}
			if (0 == ca310_uart_write(channel,xyLvMode, strlen(xyLvMode)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][START] send %s,return %s\n",channel,xyLvMode,aucreadbuf);
						return -1;
					}
				}
			}
		}
		else //模式错误
		{
			return -1;
		}
		ca310mode[channel] = nMode;
        ret = 0;
    }
    else if(LCD_CMD_SET_FLICK == cmd) //读取
    {
		if(nMode == 0) //Flicker mode
		{
			if (0 == ca310_uart_write(channel,flickGet, strlen(flickGet)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,4) != 0)
					{
						printf("[%d][READ] send %s,return %s\n",channel, flickGet,aucreadbuf);
						return -1;
					}
					else
					{
						#ifdef ENABLE_PROBE_DEBUG
						printf("[%d][READ] set flick %s\n",channel,aucreadbuf);
						#endif
						
						char *pValue = strchr(aucreadbuf,0x20);
						float fValue;
						sscanf(pValue, "%f", &fValue);
					//	int   value  = fValue*100;
					//	*pFlick = value;
						if(pFlick)
						{
							*pFlick = fValue;
						}
					}
				}
			}
		}
        else if(nMode == 1) //xyLv mode
		{
			if (0 == ca310_uart_write(channel, xyLvGet, strlen(xyLvGet)))
			{
				ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
				if(ret == 0)
				{
					if(strncmp(aucreadbuf,okRet,2) != 0)
					{
						printf("[%d][READ] send %s,return %s\n",channel,xyLvGet,aucreadbuf);
						return -1;
					}
					else
					{
						#ifdef ENABLE_PROBE_DEBUG
						printf("[%d][READ] %s\n",channel,aucreadbuf); //OK01,P1 2909;3401;86.79
						#endif
						
					//	char *pValue = strchr(aucreadbuf,0x20);
					//	float fValue;
						char i,txt[100];

						//x
						char* ptr = strstr(aucreadbuf, " ");
						if(ptr != NULL)
						{
							ptr += 1;
							memset(txt, 0, sizeof(txt));
							for(i=0; i < 8; i++)
							{
								if(ptr[i] == ';')
								{
									break;
								}
								txt[i] = ptr[i];
							}
							int nx = atoi(txt);
							float fx = (float)nx;
							if(pX != NULL)
							{
								*pX = fx / 10000;
							}

							//y
							ptr = strstr(ptr, ";");
							if(ptr != NULL)
							{
								ptr += 1;
								memset(txt, 0, sizeof(txt));
								for(i=0; i < 8; i++)
								{
									if(ptr[i] == ';')
									{
										break;
									}
									txt[i] = ptr[i];
								}
								int ny = atoi(txt);
								float fy = (float)ny;
								if(pY != NULL)
								{
									*pY = fy / 10000;
								}

								//Lv
								ptr = strstr(ptr, ";");
								if(ptr != NULL)
								{
									ptr += 1;
									float flv = 0;
									sscanf(ptr, "%f", &flv);
									if(pLv != NULL)
									{
										*pLv = flv;
									}
								}
							}
						}
					}
				}
			}
		}
		else //模式错误
		{
			return -1;
		}
		ret = 0;
    }
    else if(LCD_CMD_FLICK_END == cmd) //断开 REMOTE
    {
		//已经过关闭过了
		if(ca310mode[channel] == -1)
		{
			return 0;
		}

		//
        if (0 == ca310_uart_write(channel,stopCom, strlen(stopCom)))
        {
            ret = ca310_uartWaitRecvData(channel,delaytime, aucreadbuf, &nBufLen);
            if(ret == 0)
            {
                if(strncmp(aucreadbuf,okRet,4) != 0)
                {
                    printf("[%d][END] send %s,return %s\n",channel,stopCom,aucreadbuf);
                 //   *pFlick = -1;
                    return -1;
                }
            }
        }
		ca310mode[channel] = -1;
		ret = 0;
    }
    return ret;
}

#endif

