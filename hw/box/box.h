#ifndef  __BOX_H__
#define  __BOX_H__

#include "comStruct.h"


#define FLICKER_OK     0
#define FLICKER_NG_2   2


#ifdef ENABLE_CONTROL_BOX

#pragma pack(push)
#pragma pack(1)

typedef enum
{
    HAND_KEY = 0X80,
    UP_KEY = 0x81,
    DOWN_KEY = 0x82,
    POWER_KEY = 0x83,
    OK_KEY = 0x84,
    HOME_KEY = 0x85,
    OPT_KEY = 0x86,
    CHECK_PWR_KEY = 0x87,
}EBOX_KEY;

typedef struct
{
    int  isConnected;
    int  pgPwrStatus;
    int  boxLayer; //module,picture
    int  isAutoFlick;
    int  totalModuleNum;
    int  curModuleIndex;
    char curModuleName[32];
    char curModulePwrName[32];
    int  curTotalPtnNums;
    int  curPtnId;
    sByPtnPwrInfo pwrInfo;
}box_cur_state_t;


#define MODULE_LAYER  1
#define PICTURE_LAYER 2

typedef struct tag_hand_info_s
{
    char moduleName[32];
    char pictureName[32];
    char pwrName[32];
    short  pgPwrStatus; //0 off 1:on
    short  moduleTotalNum;
    short  curModuleIndex;
    short  pictureTotalNum;
    short  curPictureIndex;
}hand_info_t;

typedef struct tag_change_info_s
{
    short  isModuleFlag; // 1:module 2:picture
    short  curindex;
    short  totalNum;
    short  lockSec;
    char moduleName[32];
    char pictureName[32];
    char pwrName[32];
}change_info_t;

typedef struct tag_box_pwr_info_s
{
    short vdd;
    short idd;
    short vbl;
    short ibl;
}boxpwr_info_t;

typedef struct tag_box_shut_info_s
{
    short vcom;
    short vcom_otp;
    short alloptStatus;
    short pgPwrStatus;
    short ptnIndex;
    short lockSec;
    char  pictureName[32];
}box_shut_info_t;

typedef struct tag_box_pwr_status_s
{
    short   pgPwrStatus;
}box_pwr_status_t;

#define MAX_ROW_CHAR  (32+6)
typedef struct tag_box_otp_info_s
{
//    short   pgOptCheckValue; 原始的
	char ch1[MAX_ROW_CHAR];
	char ch2[MAX_ROW_CHAR];
}box_otp_info_t;

#pragma pack(pop)

void boxSavePwrData(sByPtnPwrInfo *pPwrData);

//
#define BOX_CMD_GET_VER    0x44000001

//Flicker OK 烧录时间 8.23S


//Flicker NG 错误类型 1,   烧录次数已满
#define FLICKER_NG_1   1

//Flicker NG 错误类型 2,   FLICK设备错误, 不正常,  常见的比如出现 ----.--, 或者读取到的值为0
#define FLICKER_NG_2   2

//Flicker NG 错误类型 3,   FLICK设备回应的值要么很大, 要么很小, 很平衡时, 说明设备不正常
#define FLICKER_NG_3   3

//Flicker NG 错误类型 4,   FLICK设备回应最小的值比PG设定的radio range大, 不允许烧录.
#define FLICKER_NG_4   4

//Flicker NG 错误类型 5,   烧录失败, 检查次数没有增加, 检查VCOM值与烧录的不一样
#define FLICKER_NG_5   5

//Flicker NG 错误类型 6,
#define FLICKER_NG_6   6

//Flicker NG 可以烧录, 但是没有烧录进去
#define FLICKER_NG_OTP_COUNT_ERR   7

#endif

#endif
