#ifndef PUBFPGA_H
#define PUBFPGA_H

#include "common.h"

#pragma pack(push)
#pragma pack(1)
//fpga导入图形结构体
typedef struct __FPGA_DDR_BUF__
{
    BYTE ddr;
    BYTE last;
    BYTE *buf;
    INT fileLong;
    WORD16 hResolution;
    WORD16 vResolution;
    BYTE picNo;
    BYTE picSize;
    BYTE picType;
    BYTE reserve[3];
    #define DDRBUFSIZE	0x4000
}fpgaDdrBuf;

typedef struct __SHOW_RGB__
{
    WORD16 rgb_r;
    WORD16 rgb_g;
    WORD16 rgb_b;
} showRgbStr;

typedef struct __FPGA_SEQ__
{
    WORD16 T0; // 水平后肩
    WORD16 T1;
    WORD16 T2;
    WORD16 T3;
    WORD16 T4; // 垂直后肩
    WORD16 T5;
    WORD16 T6;
    WORD16 T7;
} fpgaSeqStr;


#define PTN_MOV_DIRECTION_STOP	0		// 停止
#define PTN_MOV_DIRECTION_LEFT	1		// 左移
#define PTN_MOV_DIRECTION_RIGHT	2		// 右移
#define PTN_MOV_DIRECTION_VERT	3		// 水平翻转
#define PTN_MOV_DIRECTION_UP	1		// 上移
#define PTN_MOV_DIRECTION_DOWN	2		// 下移
#define PTN_MOV_DIRECTION_HORI	3		// 垂直翻转
typedef struct _PTN_MOVE_
{
    WORD32  Xmsg;			// [31:16] X方向 [15:0]X步进
    WORD32  Ymsg;;		// [31:16] Y方向 [15:0]Y步进
}sPtnMove;


// 十字光标结构体
#define userCrossCursorAll 0
#define userCrossCursorR   1
#define userCrossCursorG   2
#define userCrossCursorB   3

typedef struct __CROSS_CURSOR__
{
    BYTE enable;
    WORD16 x;
    WORD16 y;
    WORD32 wordColor;
    WORD32 crossCursorColor;
    BYTE RGBchang;		// RGB步进时用的颜色,0:ALL 1 R; 2 G; 3 B.
    BYTE HVflag;			// 选择是横向还是纵向的RGB点
    BYTE startCoordinate;	// 选择起始坐标 0开始还是1开始
} crossCursorStr;

// fpga十字光标颜色
#define kernCrossCursorAll 0
#define kernCrossCursorR 1
#define kernCrossCursorG 2
#define kernCrossCursorB 3
typedef struct __State__
{
    BYTE enable;
    WORD16 x;
    WORD16 y;
    WORD16 crossCursorColorRed;
    WORD16 crossCursorColorGreen;
    WORD16 crossCursorColorBlue;
    BYTE RGBchang;		    // RGB步进时用的颜色,0:ALL 1 R; 2 G; 3 B.
    BYTE HVflag;			// 选择是横向还是纵向的RGB点
    BYTE startCoordinate;	// 选择起始坐标 0开始还是1开始
} CrossCursorStateStr;


// link 模式
#define signalOdd		1
#define signalEven		2
#define signalMaster	1
#define siganlSlave 	2
typedef struct __SINGAL_MODE__
{
    BYTE OddEven;			// 奇偶
    BYTE MasterSlave;		// 主从
    BYTE link2;
    BYTE link3;
} signalModeStr;

// 设置模组链接信息
typedef struct __LVDS_SINGNAL_MODULE__
{
    BYTE linkCount;	// link数
    BYTE module[4];	// 链接状态
} lvdsSignalModuleStr;

#define DISABLE_RBG	0
#define ENABLE_RBG	1
typedef struct _DISENABLE_RGB__
{
    // 0 去色 1 恢复
    CHAR disR;
    CHAR disG;
    CHAR disB;
} disRGBStr;

typedef struct _FPGA_SET_FREQ_BY_REFRESH_
{
    WORD16 freq ; 	// 屏幕刷新频率 单位hz
} sFpgaSetFreqByRefresh;


#define RGB_PLUS	1
#define RGB_DEC		0

#define ENABLE_RGB_ADJUST	1
#define DISABLE_RGB_ADJUST	0
typedef struct _RGB_ADJUST_
{
    BYTE enable;

    BYTE isRPlus;
    BYTE RPlusVal;

    BYTE isGPlus;
    BYTE GPlusVal;

    BYTE isBPlus;
    BYTE BPlusVal;
} rgbAdjustStr;

typedef struct _IMAGE_OFFSET_
{
    WORD32 Xoffset;	// 水平方向的偏移
    WORD32 Yoffset;	// 垂直方向的偏移
} imageOffsetStr;

typedef struct _SYNC_SINGAL_LEVEL__
{
    BYTE sync_pri_h;
    BYTE sync_pri_v;
    BYTE de;
} syncSingalLevelStr;

#define TP_PG_PICMODE    0
#define TP_PG_VIDEOMODE  1
#define TP_PG_EDIDMODE   2	// TIP动态
#define TP_PG_CUTMODE    3	// TP裁剪

#define DIS_FPGA_3D_PIN 0
#define EN_FPGA_3D_PIN  1

typedef struct _FPGA_3D_PIN_
{
    INT en;     // 是否使能3D引脚
} fpga3DPinStr;

#pragma pack(pop)

unsigned int fpga_message_queue_get();

#endif // PUBFPGA_H
