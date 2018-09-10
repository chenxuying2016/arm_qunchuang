#ifndef COMSTRUCT_H_
#define COMSTRUCT_H_

#if 1
#include "common.h"

#pragma pack(push)
#pragma pack(1)

#define MAX_UART_CNT         3     // UART 数目

typedef INT (*RFUNC)(INT, struct timeval);
typedef INT (*SFUNC)(CHAR *, INT, void *);
typedef INT (*FUN)(BYTE , void * , WORD16 wlen, LPVOID);

typedef struct _process_cmd
{
    BYTE cmd;
    FUN cmdFun;
} T_CMD_PROCFUN;

typedef struct _XML_UART_
{
    INT  xmlTtyStatus;  // 0:disable 1:enable
    BYTE xmlUartName[16];
    BYTE xmlTtyName[16];
    BYTE xmlRoleName[16];
}T_XML_UART;

typedef struct sIntComNodeInf
{
    BYTE bResponseFlag;//0:not response 1:response
    BYTE res[2];
    INT fd;
    struct sockaddr_in  cliaddr;
}ComNodeIf;

typedef struct sIntComDevicdInf
{
    ComNodeIf nodeInf;
    SFUNC sndFunc;
    RFUNC rcvFunc;
}ComDeviceIf;

typedef struct PWR_MSG_HEAD
{
    BYTE 	cmd;        //[command word]
    BYTE 	type;       //[command type]
    WORD16 	len;        //[payload size,defalut is 0]
    WORD16	dest_addr;  //[link,level]
    WORD16  signl_type; //[1:ethernet,2:serial,3:arm]
    WORD16  signl_fd;   //offline setup: serial fd or socket fd
    unsigned long   ipaddr;     //pc ip address
    WORD16  ways;       //option ways
    WORD32  result;     //option result: [0:success,1:failure]
}tPwrMsgHead;

typedef struct  smsghead
{
    BYTE cmd;
    BYTE type;
    WORD16 len;
} SMsgHead;

typedef struct __FPGA_REG__
{
	WORD16 ofset;
	WORD16 value;
#define MAX_OFFSET 0x40
} fpgaRegStr;

typedef struct _BY_PTN_PWR_INFO_
{
    short VDD;
    short VDDh;
    short VDDl;
    int iVDD;
    int iVDDh;
    int iVDDl;

    short VDDIO;
    short VDDIOh;
    short VDDIOl;
    int iVDDIO;
    int iVDDIOh;
    int iVDDIOl;

    short ELVDD;
    short ELVDDh;
    short ELVDDl;
    int iELVDD;
    int iELVDDh;
    int iELVDDl;

    short ELVSS;
    short ELVSSh;
    short ELVSSl;
    int iELVSS;
    int iELVSSh;
    int iELVSSl;

	#ifdef ENABLE_POWER_OLD_VERSION
    short VBL;
    short VBLh;
    short VBLl;
    int iVBL;
    int iVBLh;
    int iVBLl;
	#else
	unsigned short VBL;
	short iVBL;
	unsigned char vbl_led_p_open_short;
	unsigned char vbl_led_n_open_short[6];
	unsigned char vbl_reserved[7];
	#endif
	
    short VSP;
    short VSPh;
    short VSPl;
    int iVSP;
    int iVSPh;
    int iVSPl;

    short VSN;
    short VSNh;
    short VSNl;
    int iVSN;
    int iVSNh;
    int iVSNl;

    short MTP;
    short MTPh;
    short MTPl;
    int iMTP;
    int iMTPh;
    int iMTPl;
}sByPtnPwrInfo;


typedef struct tag_s1103PwrCfgDb_s
{
    WORD16 VDD;
    WORD16 VDDFlyTime;
    WORD16 VDDOverLimit;
    WORD16 VDDUnderLimit;
    WORD32 VDDCurrentOverLimit;
    WORD32 VDDCurrentUnderLimit;
    WORD16 VDDOpenDelay;
    WORD16 VDDCloseDelay;

    WORD16 VDDIO;
    WORD16 VDDIOFlyTime;
    WORD16 VDDIOOverLimit;
    WORD16 VDDIOUnderLimit;
    WORD32 VDDIOCurrentOverLimit;
    WORD32 VDDIOCurrentUnderLimit;
    WORD16 VDDIOOpenDelay;
    WORD16 VDDIOCloseDelay;

    WORD16 ELVDD;
    WORD16 ELVDDFlyTime;
    WORD16 ELVDDOverLimit;
    WORD16 ELVDDUnderLimit;
    WORD32 ELVDDCurrentOverLimit;
    WORD32 ELVDDCurrentUnderLimit;
    WORD16 ELVDDOpenDelay;
    WORD16 ELVDDCloseDelay;

    WORD16 ELVSS;
    WORD16 ELVSSFlyTime;
    WORD16 ELVSSOverLimit;
    WORD16 ELVSSUnderLimit;
    WORD32 ELVSSCurrentOverLimit;
    WORD32 ELVSSCurrentUnderLimit;
    WORD16 ELVSSOpenDelay;
    WORD16 ELVSSCloseDelay;

    WORD16 VBL;
    WORD16 VBLFlyTime;
    WORD16 VBLOverLimit;
    WORD16 VBLUnderLimit;
    WORD32 VBLCurrentOverLimit;
    WORD32 VBLCurrentUnderLimit;
    WORD16 VBLOpenDelay;
    WORD16 VBLCloseDelay;

    WORD16 VSP;
    WORD16 VSPFlyTime;
    WORD16 VSPOverLimit;
    WORD16 VSPUnderLimit;
    WORD32 VSPCurrentOverLimit;
    WORD32 VSPCurrentUnderLimit;
    WORD16 VSPOpenDelay;
    WORD16 VSPCloseDelay;

    WORD16 VSN;
    WORD16 VSNFlyTime;
    WORD16 VSNOverLimit;
    WORD16 VSNUnderLimit;
    WORD32 VSNCurrentOverLimit;
    WORD32 VSNCurrentUnderLimit;
    WORD16 VSNOpenDelay;
    WORD16 VSNCloseDelay;

    WORD16 MTP;
    WORD16 MTPFlyTime;
    WORD16 MTPOverLimit;
    WORD16 MTPUnderLimit;
    WORD32 MTPCurrentOverLimit;
    WORD32 MTPCurrentUnderLimit;
    WORD16 MTPOpenDelay;
    WORD16 MTPCloseDelay;

    WORD16 LEDEnable;//signalType;		//// ttl lvds类型，有的需要在电源板配置
    WORD16 LEDChannel;
    WORD16 LEDNumber;
    WORD16 LEDCurrent;
    WORD16 LEDOVP;
    WORD16 LEDUVP;

    WORD16 signalOpenDelay;
    WORD16 signalCloseDelay;

    WORD16 pwmOpenDelay;
    WORD16 pwmCloseDelay;
    WORD16 pwm;
    WORD16 pwmDuty;
    WORD16 pwmFreq;

    WORD16 vdimOpenDelay;
    WORD16 vdimCloseDelay;
    WORD16 vdim;

    WORD16 invertOpenDelay;
    WORD16 invertCloseDelay;
    WORD16 invert;

    WORD16 gpioPin0;//gpio_power_switch;   //0表示3.3V   1表示5V
    WORD16 gpioPin1;//edid_vcc_en;   // 0表示关     1表示开
    WORD16 gpioPin2;//sw_CRT;     //0表示二通道     1表示第一通道
    WORD16 gpioPin3;
    WORD16 gpioPin4;
    WORD16 gpioPin5;
    WORD16 gpioPin6;
    WORD16 gpioPin7;
}s1103PwrCfgDb ;

typedef struct _PWR_VDD_VBL_SET
{
    unsigned char  flag;
    unsigned char  set;
    unsigned short value[2];
}PwrVddVblSet;

#else
#define MAX_GPIO_CNT         20    // GPIO 数目

typedef INT (*RFUNC)(INT, struct timeval);
typedef INT (*SFUNC)(CHAR *, INT, void *);
typedef INT (*FUN)(BYTE , void * , WORD16 wlen, LPVOID);
typedef INT (*REGFUN)(void);

typedef INT (*PWR_CMD_BEFOR)(BYTE , BYTE *, LPVOID);  
typedef INT (*PWR_CMD_RECV)(BYTE , BYTE *);
typedef INT (*PWR_GPIO_BEFORE)(BYTE, BYTE); 
typedef INT (*PWR_SET_GPIO)(BYTE);
typedef INT (*ARM_MD_SETHOOK)(LPVOID);
typedef INT (*EXT_CFG_FUN)(BYTE);
typedef INT (*EXT_CFG_GPIO)(void);
typedef INT (*EXT_INIT_CODE)(CHAR *);
typedef INT (*EXT_MIPI_ONOFF)(LPVOID); 
typedef INT (*EXT_CFG_VBYONE)(void);
typedef INT (*EXT_CMD_PROC_FUN)(INT, BYTE, void *, WORD16,  void *);
typedef BYTE(*ARM_PGMODE_GETHOOK)(void);
typedef INT (*TExtReplyProcFun)(BYTE *, INT);

typedef struct _register_fun
{
    BYTE PgName[32];
    REGFUN regFun;
}tFpgaRegFun;

typedef struct _process_cmd
{
    BYTE cmd;
    FUN cmdFun;
} T_CMD_PROCFUN;


typedef struct _XML_UART_
{
    INT  xmlTtyStatus;  // 0:disable 1:enable
    BYTE xmlUartName[16];
    BYTE xmlTtyName[16];
    BYTE xmlRoleName[16];
}T_XML_UART;

typedef struct _XML_GPIO_CFG_
{
    INT val;    // GPIO引脚号
    INT cmd;    // 配置该引脚的命令字
    BYTE *pin;  // pin的位置 ep lvds_pin_5
    BYTE *type; // pin的类型 ep AT91_PIN_PB20
    BYTE *name; // 引脚的名字 ep OD_SEL
} T_XML_GPIO_CFGSTR;


typedef struct _XML_GPIO_INFO_
{
    T_XML_GPIO_CFGSTR xmlGpioCfg[MAX_GPIO_CNT];
    INT gpioCnt;    
} T_XML_GPIO_INFO;

typedef struct PWR_MSG_HEAD
{
    BYTE 	cmd;        //[command word]
    BYTE 	type;       //[command type]
    WORD16 	len;        //[payload size,defalut is 0]
    WORD16	dest_addr;  //[link,level]
    WORD16  signl_type; //[1:ethernet,2:serial,3:arm]
    WORD16  signl_fd;   //offline setup: serial fd or socket fd
    unsigned long   ipaddr;     //pc ip address
    WORD16  ways;       //option ways
    WORD32  result;     //option result: [0:success,1:failure]
}tPwrMsgHead;

typedef struct  smsghead
{
    BYTE cmd;
    BYTE type;
    WORD16 len;
} SMsgHead;

typedef struct sIntComNodeInf
{
    BYTE bResponseFlag;//0:not response 1:response
    BYTE res[2];
    INT fd;
    struct sockaddr_in  cliaddr;
}ComNodeIf;

typedef struct sIntComDevicdInf
{
    ComNodeIf nodeInf;
    SFUNC sndFunc;
    RFUNC rcvFunc;
}ComDeviceIf;

typedef struct _BY_PTN_PWR_INFO_
{
    short VDD;
    short VDDh;
    short VDDl;
    int iVDD;
    int iVDDh;
    int iVDDl;

    short VDDIO;
    short VDDIOh;
    short VDDIOl;
    int iVDDIO;
    int iVDDIOh;
    int iVDDIOl;

    short ELVDD;
    short ELVDDh;
    short ELVDDl;
    int iELVDD;
    int iELVDDh;
    int iELVDDl;

    short ELVSS;
    short ELVSSh;
    short ELVSSl;
    int iELVSS;
    int iELVSSh;
    int iELVSSl;

    short VBL;
    short VBLh;
    short VBLl;
    int iVBL;
    int iVBLh;
    int iVBLl;

    short VSP;
    short VSPh;
    short VSPl;
    int iVSP;
    int iVSPh;
    int iVSPl;

    short VSN;
    short VSNh;
    short VSNl;
    int iVSN;
    int iVSNh;
    int iVSNl;

    short MTP;
    short MTPh;
    short MTPl;
    int iMTP;
    int iMTPh;
    int iMTPl;
}sByPtnPwrInfo;

typedef struct _PWR_VDD_VBL_SET
{
    unsigned char  flag;
    unsigned char  set;
    unsigned short value[2];
}PwrVddVblSet;

typedef struct tag_s1103PwrCfgDb_s
{
    WORD16 VDD;
    WORD16 VDDFlyTime;
    WORD16 VDDOverLimit;
    WORD16 VDDUnderLimit;
    WORD32 VDDCurrentOverLimit;
    WORD32 VDDCurrentUnderLimit;
    WORD16 VDDOpenDelay;
    WORD16 VDDCloseDelay;

    WORD16 VDDIO;
    WORD16 VDDIOFlyTime;
    WORD16 VDDIOOverLimit;
    WORD16 VDDIOUnderLimit;
    WORD32 VDDIOCurrentOverLimit;
    WORD32 VDDIOCurrentUnderLimit;
    WORD16 VDDIOOpenDelay;
    WORD16 VDDIOCloseDelay;

    WORD16 ELVDD;
    WORD16 ELVDDFlyTime;
    WORD16 ELVDDOverLimit;
    WORD16 ELVDDUnderLimit;
    WORD32 ELVDDCurrentOverLimit;
    WORD32 ELVDDCurrentUnderLimit;
    WORD16 ELVDDOpenDelay;
    WORD16 ELVDDCloseDelay;

    WORD16 ELVSS;
    WORD16 ELVSSFlyTime;
    WORD16 ELVSSOverLimit;
    WORD16 ELVSSUnderLimit;
    WORD32 ELVSSCurrentOverLimit;
    WORD32 ELVSSCurrentUnderLimit;
    WORD16 ELVSSOpenDelay;
    WORD16 ELVSSCloseDelay;

    WORD16 VBL;
    WORD16 VBLFlyTime;
    WORD16 VBLOverLimit;
    WORD16 VBLUnderLimit;
    WORD32 VBLCurrentOverLimit;
    WORD32 VBLCurrentUnderLimit;
    WORD16 VBLOpenDelay;
    WORD16 VBLCloseDelay;

    WORD16 VSP;
    WORD16 VSPFlyTime;
    WORD16 VSPOverLimit;
    WORD16 VSPUnderLimit;
    WORD32 VSPCurrentOverLimit;
    WORD32 VSPCurrentUnderLimit;
    WORD16 VSPOpenDelay;
    WORD16 VSPCloseDelay;

    WORD16 VSN;
    WORD16 VSNFlyTime;
    WORD16 VSNOverLimit;
    WORD16 VSNUnderLimit;
    WORD32 VSNCurrentOverLimit;
    WORD32 VSNCurrentUnderLimit;
    WORD16 VSNOpenDelay;
    WORD16 VSNCloseDelay;

    WORD16 MTP;
    WORD16 MTPFlyTime;
    WORD16 MTPOverLimit;
    WORD16 MTPUnderLimit;
    WORD32 MTPCurrentOverLimit;
    WORD32 MTPCurrentUnderLimit;
    WORD16 MTPOpenDelay;
    WORD16 MTPCloseDelay;

    WORD16 LEDEnable;//signalType;		//// ttl lvds类型，有的需要在电源板配置
    WORD16 LEDChannel;
    WORD16 LEDNumber;
    WORD16 LEDCurrent;
    WORD16 LEDOVP;
    WORD16 LEDUVP;

    WORD16 signalOpenDelay;
    WORD16 signalCloseDelay;

    WORD16 pwmOpenDelay;
    WORD16 pwmCloseDelay;
    WORD16 pwm;
    WORD16 pwmDuty;
    WORD16 pwmFreq;

    WORD16 vdimOpenDelay;
    WORD16 vdimCloseDelay;
    WORD16 vdim;

    WORD16 invertOpenDelay;
    WORD16 invertCloseDelay;
    WORD16 invert;

    WORD16 gpioPin0;//gpio_power_switch;   //0表示3.3V   1表示5V
    WORD16 gpioPin1;//edid_vcc_en;   // 0表示关     1表示开
    WORD16 gpioPin2;//sw_CRT;     //0表示二通道     1表示第一通道
    WORD16 gpioPin3;
    WORD16 gpioPin4;
    WORD16 gpioPin5;
    WORD16 gpioPin6;
    WORD16 gpioPin7;
}s1103PwrCfgDb ;


struct sFpgaCfgDb
{
	unsigned long  sig_mode;
#define SINGLE_LINK		1
#define DOUBLE_LINK		2
#define FOUR_LINK		4
	char  sig_type[16];
    char  disp_norm[16];
	unsigned long  bits;
#define SIX_BITS		6
#define EIGHT_BITS		8
#define TEN_BITS		10

	unsigned long   total_time_h;					//horizon
	unsigned long   total_time_v;					//vert
	unsigned long   active_time_h;
	unsigned long   active_time_v;
	unsigned long   back_porch_h;
	unsigned long   back_porch_v;
	unsigned long   front_porch_h;
	unsigned long   front_porch_v;
	unsigned long   sync_pw_h;						//pulse width
	unsigned long   sync_pw_v;

	unsigned long   sync_pri_h;
	unsigned long   sync_pri_v;
	unsigned long   isLED; // De配置
	unsigned long   hvs;
	unsigned long   bist;
	unsigned long  freq;  // 刷新频率
	unsigned long  link[4];
//    WORD16 linkParity;
//    WORD16 linkMs;
// 100字节
};

struct sGpioCfgDb
{
	WORD16  lvdsSet;
	WORD16  bistSet;
	WORD16  odselSet;
	WORD16  dsyncSet;
	WORD16  wp0;
	WORD16  wp1;
	WORD16  reserved1Set;
	WORD16  reserved2Set;
	WORD16  i2cpin0;
	WORD16  i2cpin1;
};

struct sEdidCfgDb
{
	WORD16 isEdid;
};

struct module_config_table
{
	BYTE mdName[MAX_FILE_NAME_LEN];
	struct sFpgaCfgDb sfpga;
	struct sGpioCfgDb sgpio;
    s1103PwrCfgDb spwr;
	struct sEdidCfgDb sedid;
};

struct test_config_table
{
	unsigned long  testId;
	BYTE testName[MAX_FILE_NAME_LEN];
	unsigned long  ptnNum;
//	CHAR  desc[128];
//	unsigned long  hHz;
//	unsigned long  displayInterval;
//	unsigned long  mod_dim;
//	unsigned long  dim_value;
//	unsigned long  createtime;
//	unsigned long  updatetime;
//	unsigned long  mod_onoff;
//	unsigned long  on_time;
//	unsigned long  off_time;
//	unsigned long run_count;
};

typedef struct _module_test_config_table_
{
	struct module_config_table curModuleFile;
	struct test_config_table curTestFile;
}module_test_config_table;

typedef struct __FPGA_REG__
{
	WORD16 ofset;
	WORD16 value;
#define MAX_OFFSET 0x40
} fpgaRegStr;

typedef struct _SAVE_PTN_INFO
{
    BYTE ModuleName[64];
    BYTE TestFileName[64];
    BYTE PtnFileName[64];
    BYTE ModulePowerName[64];
    BYTE ModuleTimName[64];
    WORD32 PtnFileNum;
    WORD32 BmpFlag;
}SavePtnInfo;
#endif

#pragma pack(pop)


#endif

