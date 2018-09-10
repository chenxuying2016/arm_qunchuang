
#ifndef __COMMON_H
#define __COMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#ifdef __WIN32__
#include <winsock2.h>
#include <winsock.h>
#include <wininet.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>
#endif


typedef unsigned short      WORD16; 	  /* 16位无符号整型  */
typedef unsigned int        WORD32; 	  /* 32位无符号整型  */

typedef unsigned char       BYTE;         /* 8位无符号整型   */
typedef signed int          INT;          /* 32位有符号整型   */
typedef signed char         CHAR;         /* 8位有符号字符型  */		
typedef signed long         LONG;         /* 32位有符号长整型 */
typedef signed short	    SHORT;        /* 16位有符号整型  */
typedef void                *LPVOID;      /* 空类型           */
typedef unsigned char       BOOLEAN;      /* 布尔类型        */

#define SUCCESS (0)
#define FAILED   (-1)
#define BREAK_OUT (-2)

#define TRUE (1)
#define FALSE (0)

#define YES  (1)
#define NO   (0)

#define ARM_PRT_SW1                  0x00000001
#define FPGA_PRT_SW1                 0x00000010
#define PWR_PRT_SW1                  0x00000100
#define UPDATE_PRT_SW1               0x00001000
#define I2C_PRT_SW1                  0x00010000
#define AUTORUN_PRT_SW1              0x00100000
#define QUEUE_PRT_SW1                0x01000000
#define UART_PRT_SW1                 0x01000000
#define IF_PRT_SW1                   0x10000000
#define SQLITE_PRT_SW1               0x10000001
#define EXTBD_PRT_SW1                0x10000010

#define MAX_FILE_NAME_LEN               64
#define MAX_STRING_LEN                  64
#define MAX_PATH_LEN			        128

#define MAX_CMD_NUM        				64
#define PWR_MAX_CMD_NUM        			32
#define EXT_MAX_CMD_NUM        			32

#define PWR_MSG_FROM_ETH                0x01
#define PWR_MSG_FROM_UART               0x02
#define PWR_MSG_FROM_LOCAL              0x03

#define FPGA_CMD_SET_PHOTO           0X01
#define FPGA_CMD_SHOW_RGB            0X02
#define FPGA_CMD_SET_VESA_JEDA       0X03
#define FPGA_CMD_SET_CLOCK           0X04
#define FPGA_CMD_GET_FPGA_VERSION    0X05
#define FPGA_CMD_SET_LINK            0X06
#define FPGA_CMD_SET_BIT             0X07
#define FPGA_CMD_SET_SEQ             0X08
#define FPGA_CMD_SET_TEST_FILE       0X09
#define FPGA_CMD_PIC_MOVE            0X0A
#define FPGA_CMD_SET_REG             0X0B
#define FPGA_CMD_READ_REG            0X0C
#define FPGA_CMD_SET_CROSS_CURSOR    0X0D
#define FPGA_CMD_SET_LINK_MODE       0X0E
#define FPGA_CMD_SET_LVDS_TEST       0X0F
#define FPGA_CMD_SET_NCLK            0X10
#define FPGA_CMD_SET_RGB_ONOFF       0X11
#define FPGA_CMD_SET_PHOTO_INDEX     0X12
#define FPGA_CMD_SET_FREQ_BY_REFRESH 0X13
#define FPGA_CMD_ADJUST_RGB          0X14
#define FPGA_CMD_SET_IMAGE_OFFSET    0X15
#define FPGA_CMD_MOVE_PTN            0X16
#define FPGA_CMD_SYN_SIGNAL_LEVEL    0X17
#define FPGA_CMD_SET_3D_PIN          0X18
#define FPGA_CMD_SET_TPMODE          0X19
#define FPGA_CMD_SET_FPGA_ONEREG     0X20
#define FPGA_CMD_SET_EDID            0X21
#define FPGA_CMD_ADJUST_LOGICPHOTO   0X22
#define FPGA_CMD_HANDO				 0x23
#define FPGA_CMD_SUDOKU				 0x24
#define FPGA_CMD_LVDS_TEST           0x25
#define FPGA_CMD_UPDATE              0x70
#define FPGA_CMD_GET_BY_PTN_INFO     0x72

#define PWR_CMD_POWER_ON             0X01
#define PWR_CMD_POWER_OFF            0X02
#define PWR_CMD_CTRL_SWITCH          0X03
#define PWR_CMD_GET_VOL              0X04
#define PWR_CMD_GET_DEV_STATE        0X06
#define PWR_CMD_GET_POWER_VER        0X08
#define PWR_CMD_CFG_POWER_STRUCT     0X09
#define PWR_CMD_CFG_POWER_FILE       0X19
#define PWR_CMD_ADJUST_VDIM          0X0B
#define PWR_CMD_ADJUST_PWR_FREQ      0X0C
#define PWR_CMD_ADJUST_INVERT        0X0D
#define PWR_CMD_ON_OFF               0X0D
#define PWR_CMD_ADJUST_BY_PTN        0X0F
#define PWR_CMD_SET_VOL              0X12
#define PWR_CMD_SET_FLY_TIME         0X14
#define PWR_CMD_GET_CABC_PWM		 0x25
#define PWR_CMD_SET_BOE_FREQ_ONOFF   0X30
#define PWR_CMD_GET_ACK_PACKET       0X41
#define PWR_CMD_POWER_CALIBRATION	 0x80
#define PWR_CMD_UPGRADE              0x12345679

#define MSG_TYPE_SYS_STATUS          0x01
#define MSG_TYPE_SYS_PARA            0x02
#define MSG_TYPE_ARM                 0x03
#define MSG_TYPE_FPGA                0x04
#define MSG_TYPE_POWER               0x05
#define MSG_TYPE_I2C                 0x06
#define MSG_TYPE_UPDATE              0x07
#define MSG_TYPE_FILE                0x08
#define MSG_TYPE_PIN		         0x0a
#define MSG_TYPE_BOX                 0x0b
#define MSG_TYPE_LCD                 0X0C
#define MSG_TYPE_MUX                 0x20
#define MSG_TYPE_JOBS                0x21
#define MSG_TYPE_SENSOR_BOARD        0x30
#define MSG_TYPE_AUTO_RUN            0x40
#define MSG_TYPE_EXTBOARD            0X50
#define MSG_TYPE_CIM                 0x70

#define MSG_LCD_CMD_FPGA_LVDS            0X0C01
#define MSG_LCD_CMD_VCOM_START           0X0C02
#define MSG_LCD_CMD_VCOM_READ            0X0C03
#define MSG_LCD_CMD_VCOM_WRITE           0X0C04
#define MSG_LCD_CMD_VCOM_STOP            0X0C05
#define MSG_LCD_CMD_EDID_START           0X0C06
#define MSG_LCD_CMD_EDID_READ            0X0C07
#define MSG_LCD_CMD_EDID_WRITE           0X0C08
#define MSG_LCD_CMD_EDID_STOP            0X0C09
#define MSG_LCD_CMD_FLICK_START          0X0C12
#define MSG_LCD_CMD_SET_FLICK            0X0C13
#define MSG_LCD_CMD_FLICK_END            0X0C14
#define MSG_LCD_CMD_TTYUSB_ADD           0x0C15
#define MSG_LCD_CMD_TTYUSB_REMOVE        0x0C16

#define MSG_LCD_CMD_XYLV_START			0x0C17
#define MSG_LCD_CMD_XYLV_READ			0x0C18
#define MSG_LCD_CMD_XYLV_STOP			0x0C19

#endif

