#ifndef RECVCMD_H
#define RECVCMD_H

// ARM => PC[pgServer]
typedef enum CLIENT_MSG_E
{
    CLIENT_MSG_REGISTER  =   0x2000, //register to server
    CLIENT_MSG_SYNCFILE  =   0x2001, //get file
    CLIENT_MSG_SYNCPROC  =   0x2002, //notify sync process
    CLIENT_MSG_SYNCFIN   =   0x2003, //notify sync finish
    CLIENT_MSG_SENDEDID  =   0x2004,
    CLIENT_MSG_SENDVCOM  =   0x2005,
    CLIENT_MSG_SENDPOW   =   0x2007,
    CLIENT_MSG_SENDREG   =   0x2008,
    CLIENT_MSG_REGSTAT   =   0x2009,
    CLIENT_MSG_UPDSTAT   =   0x2011,
    CLIENT_MSG_REBACK    =   0x2020,//客户端收到消息的一种反馈
	CLIENT_MSG_POWER_VER =   0x2021, //电源版本
	CLIENT_MSG_BOX_VER   =   0x2022, //控制盒版本
	CLIENT_MSG_RGBW = 0x2038, //完成GAMMA白平衡  2017-5-17
	CLIENT_MSG_OTP = 0x2039, //手动烧录 2017-5-8

	CLIENT_MSG_GAMMA = 0x203A, //GAMMA寄存器
	CLIENT_MSG_READ_GAMMA = 0x203B, //读取GAMMA寄存器的值
	CLIENT_MSG_GAMMA_OTP = 0x203C, //烧录GAMMA寄存器的值
	CLIENT_MSG_REALTIME_CONTROL=0x203D, //调试寄存器,
	CLIENT_MSG_CA310_START = 0x203E,
	CLIENT_MSG_CA310_STOP = 0x203F,

	CLIENT_MSG_LCD_READ_REG = 0x2040,
    CLIENT_MSG_LCD_WRITE_CODE = 0x2041,

	// flick test.
	CLIENT_MSG_FLICK_TEST = 0x2046,	
    CLIENT_MSG_READ_VCOM_OTP_INFO,
    CLIENT_MSG_READ_ID_OTP_INFO,
    CLIENT_MSG_WRITE_ID_OTP_INFO,

	// upgrade end.
	CLIENT_MSG_UPGRADE_END = 0x2050,	

	CLIENT_MSG_GENERAL_COMMAND_ACK = 0x2080,
	
}CLIENT_MSG_E;

// ARM <= PC[pgServer]
typedef enum SERVER2CLI_MSG_E
{
    SERVER2CLI_MSG_REGISTER  =  0x4000,
    SERVER2CLI_MSG_SYNCFILE  =  0x4002,
    SERVER2CLI_MSG_SHUTON   =   0x4003,
    SERVER2CLI_MSG_SHUTDWN  =   0x4004,
    SERVER2CLI_MSG_SHOWPTN  =   0x4005,
    SERVER2CLI_MSG_CROSSCUR =   0x4006,
    SERVER2CLI_MSG_HIDECUR   =   0x4007,
    SERVER2CLI_MSG_READREG  =   0x4008,
    SERVER2CLI_MSG_WRITEREG =   0x4009,
    SERVER2CLI_MSG_SYNCTIME =   0x4010,
    SERVER2CLI_MSG_UPDATEFILE = 0x4011,
    SERVER2CLI_MSG_SENDFILE  =  0x4012,
    SERVER2CLI_MSG_COMPLETESYNC = 0x4021,
    SERVER2CLI_MSG_TSTSHUTON=0x4022,
    SERVER2CLI_MSG_TSTSHUTDWN = 0x4023,
    SERVER2CLI_MSG_TSTTIMON=0x4024,
    SERVER2CLI_MSG_TSTTIMDWN=0x4025,
    SERVER2CLI_MSG_GETPWRON=0x4026,
    SERVER2CLI_MSG_GETPWRDWN=0x4027,
    SERVER2CLI_MSG_GETPARAM=0x4028,
    SERVER2CLI_MSG_AUTOFLICK=0x4029,
    SERVER2CLI_MSG_MANUALFLICK=0x402a,
    SERVER2CLI_MSG_PHASE1=0x4030,
    SERVER2CLI_MSG_PHASE2=0x4031,
    SERVER2CLI_MSG_FIRMWAREUPDATA=0x4032,
    SERVER2CLI_MSG_SYNCMODE=0x4033,
    SERVER2CLI_MSG_FLICKVCOM=0x4034,
    SERVER2CLI_MSG_REALTIMECONTROL=0x4035,
	SERVER2CLI_MSG_BOXUPDATA=0x4036,
	SERVER2CLI_MSG_POWERUPDATA=0x4037,
	
	SERVER2CLI_MSG_RGBW=0x4038, 
	SERVER2CLI_MSG_OTP = 0x4039, 
	SERVER2CLI_MSG_GAMMA_WRITE_REG = 0x403A,
	SERVER2CLI_MSG_READ_GAMMA = 0x403B,
	SERVER2CLI_MSG_GAMMA_OTP = 0x403C,
	SERVER2CLI_MSG_REALTIME_CONTROL=0x403D,
	SERVER2CLI_MSG_CA310_START = 0x403E, 
	SERVER2CLI_MSG_CA310_STOP = 0x403F,

	SERVER2CLI_MSG_LCD_READ_REG = 0x4040,
    SERVER2CLI_MSG_LCD_WRITE_CODE = 0x4041,

	// photo debug cmd.
	SERVER2CLI_MSG_PHOTO_DEBUG = 0x4042,
	
	// otp
	SERVER2CLI_MSG_MIPI_CHANNEL_RESET = 0x4043,
    SERVER2CLI_MSG_MIPI_CHANNEL_MTP = 0x4044,
    SERVER2CLI_MSG_MIPI_MODE = 0x4045,

	// Flick Test
	SERVER2CLI_MSG_FLICK_TEST = 0x4046,	
    SERVER2CLI_MSG_READ_VCOM_OTP_INFO,
    SERVER2CLI_MSG_READ_ID_OTP_INFO,
    SERVER2CLI_MSG_WRITE_ID_OTP_INFO,

	SERVER2CLI_MSG_GENERAL_COMMAND_REQ = 0x4080,
	
}SERVER2CLI_MSG_E;

#define  BINFILE_BEGIN   0x8000
#define  BINFILE_BODY    0x4000
#define  BINFILE_OVER    0x2000


#define REBACK_POWER_STATUS   1
#define REBACK_MIPI_CODE      2
#define REBACK_ERROR_INFO     3
#define REBACK_FLICK_INFO     4
#define REBACK_AUTO_FLICK     5
#define REBACK_FLICK_OVER     6
#endif // RECVCMD_H

