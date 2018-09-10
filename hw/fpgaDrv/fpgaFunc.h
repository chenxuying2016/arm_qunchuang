
#ifndef _FPGA_FUN_H_
#define _FPGA_FUN_H_


#define FPGA_VERSION_H			4
#define FPGA_VERSION_M			1
#define FPGA_VERSION_L			41

#define LINK1						(~0x03)
#define LINK2						(0x01 << 2)
#define LINK4						(0x03 << 2)
#define VBIT6						(0x01)
#define VBIT8						(0x02)
#define VBIT10						(0x03)


enum fpgaReturnValue
{	// 返回值
	fpgaOK,						// 正常的返回值
	fpgaEnableReadyError,		// 预备下载FPGA错误
	fpgaEnableRightError,		// 下载FPGA失败
	fpgaSetShowModeError,		// FPGA模式设置
	fpgaResetError,				// fpga复位操作
	fpgaResetTestError,			// fpga复位测试
	fpgaSetCrossCursorError,	// 设置十字光标
	fpgaDownPictureError,		// 下载图片
};

enum e_FpgaCommand
{	// fpga操作命令字
	eFPGA_SUPER_CMD,		// 0x00.保留
	eSET_FPGA_REG,			// 0x01.设置FPGA的REG
	eREADD_FPGA_REG = 0xef, // 0xef.读出FPGA的REG
	eREAD_FPGA_REG = 0x02,	// 0x02.读出FPGA的REG
	eENABLE_READY_FPGA,		// 0x03.使能FPGA前的准备
	eENABLE_RIGHT_FPGA,		// 0x04.测试FPGA
	eGET_FPGA_VER,			// 0x05.获取FPGA版本号
	eSET_FPGA_SHOW_MODE,	// 0x06.图片及十字光标显示使能
	eRESET_FPGA,			// 0x07.FPGA复位
	eRESET_FPGA_TEST,		// 0x08.测试FPGA是否复位成功
	eSET_CROSSCURSOR,		// 0x09.设置十字光标是坐标和颜色
	eDOWN_PICTURE,			// 0x0a.下载图片到FPGA
	eSET_LINK,				// 0x0b.设置FPGA link
	eSET_SEQ,				// 0x0c.设置FPGA 时序
	eSET_BIT,				// 0x0d.设置FPGA bit数
	eSET_CLOCK,				// 0x0e.设置FPGA 时钟
	ePIC_MOV,				// 0x0f.图形移动
	eSET_SIGNAL_MODE,		// 0x10.信号的模式，奇偶，主从
	eSET_FPGA_FREQ,			// 0x11.设置FPGA输出的频率
	eSET_FPGA_RGB,			// 0x12.设置FPGA RGB逻辑画面显示
	eSET_VESA_JEDA,			// 0x13.设置VESA JEDA模式
	eTEST_LVDS,				// 0x14.LVDS测试
	eFPGA_CLOCK_ENABLE,		// 0x15.开关FPGA clock
	eIMAGE_OFFSET,			// 0x16.图形偏移
	eDISABLE_RGB,			// 0x17.RGB消色
	eGRAY_SCALE_ADJUST,		// 0x18.灰阶调节
	eSIGNAL_SYNC_LEVEL,		// 0x19.同步信号电平
    eFPGA_3D_PIN,			// 0x1a.使能3DPIN
    eFPGA_SHOW_LOGIC,		// 0x1b.显示逻辑画面
    eFPGA_ONOFF_SIGNAL,		// 0x1c.信号开关
    eSET_FPGA_REG_BIT,		// 0x1d.设置FPGA寄存器位操作
};


#pragma pack(push)
#pragma pack(1)

typedef struct
{
	unsigned short offset;	// 寄存器偏移地址
	unsigned short value;	// 寄存器值
}fpgaRegisterInfo_t;

typedef struct
{
	unsigned short year;		// xxxx
	unsigned char mmdd[2];		// xx-xx
	unsigned char version[2];	// x.x.xx

	unsigned char userVersion[3];		// FPGA用户版本
	unsigned char kernelVersion[3];		// FPGA内核版本
}fpgaVersionInfo_t;

typedef struct
{
	unsigned char position;			// 图在DDR中存放的位置
	unsigned char size;				// 图大小
}showPictureInfo_t;

typedef struct
{
	unsigned char enable;			// 十字光标使能
	unsigned short x;				//
	unsigned short y;				//
	unsigned int wordColor;			// 字颜色
	unsigned int crossCursorColor;	// 光标颜色
	unsigned char RGBchang;			// RGB步进时用的颜色,0:ALL 1 R; 2 G; 3 B.
	unsigned char HVflag;			// 选择是横向还是纵向的RGB点
	unsigned char startCoordinate;	// 保留位
}crossCursorInfo_t;

typedef struct
{
#define PICTURE_TYPE_BMP			0
#define PICTURE_TYPE_LOGIC			0
#define PICTURE_TYPE_JPG			1

	unsigned char ddrCs;			// DDR片选
	unsigned char last;				// 图分次下载标志
	unsigned char *buffer;			// 图信息缓冲区
	int fileLength;					// 图分次下载长度
	unsigned short hResolution; 	// 图水平分辨率
	unsigned short vResolution;		// 图垂直分辨率
	unsigned char pictureNo;		// 图序号
	unsigned char pictureSize;		// 图大小
	unsigned char pictureType;		// 图类型
	unsigned char reserve[3];		// 保留
}fpgaDdrBufferInfo_t;

typedef struct
{
	unsigned short T0;		// hsdForntPorch
	unsigned short T1;		// hsdForntPorch + hsdSyncPures
	unsigned short T2;		// hsdForntPorch + hsdSyncPures + hsdBackPorch
	unsigned short T3;		// hsdForntPorch + hsdSyncPures + hsdBackPorch + hsdDisp
	unsigned short T4;		// vsdForntPorch
	unsigned short T5;		// vsdForntPorch + vsdSyncPures
	unsigned short T6;		// vsdForntPorch + vsdSyncPures + vsdBackPorch
	unsigned short T7;		// vsdForntPorch + vsdSyncPures + vsdBackPorch + vsdDisp
}fpgaSeqInfo_t;

typedef struct
{
	/*	[5:3]	[2:0]
	000		000		停止

	001		000		上			FPGA RGE 2B [13:6]
	010		000		下
	011		000		垂直

	000		001		左			FPGA RGE B	[13:6]
	000		010		右
	000		011		水平

	011		011		中心
	*/

	unsigned int Xmsg;		// [31:16] X方向 [15:0]X步进
	unsigned int Ymsg;		// [31:16] Y方向 [15:0]Y步进
}imageMoveInfo_t;

typedef struct
{
	unsigned char link0;
	unsigned char link1;
	unsigned char link2;
	unsigned char link3;
}signalModeInfo_t;

typedef struct
{
	char d0;		// 分频系数0
	char m;			// 倍频系数
	char d1;		// 分频系数1
}freqConfigInfo_t;

typedef struct
{
	unsigned short rgb_r;
	unsigned short rgb_g;
	unsigned short rgb_b;
}showRgbInfo_t;

typedef struct
{
	unsigned char lvdsChange;

#define VESA 0
#define JEDA 1
}vesaJedaSwitchInfo_t;

typedef struct
{
	unsigned char link;
	unsigned char isOk;
	unsigned int status[32];
}lvdsOpenShortTestInfo_t;

typedef struct
{
	unsigned int Xoffset;	// 水平方向的偏移
	unsigned int Yoffset;	// 垂直方向的偏移
}imageOffsetInfo_t;

typedef struct _DISENABLE_RGB_
{
#define ENABLE_RBG		1
#define DISABLE_RBG	0

// 0 去色 1 恢复
	char disableR;		// 消R
	char disableG;		// 消G
	char disableB;		// 消B
}disableRgbInfo_t;

typedef struct
{
#define RGB_PLUS	1
#define RGB_DEC		0

#define ENABLE_RGB_ADJUST		1
#define DISABLE_RGB_ADJUST		0

	unsigned char enable;			// 灰阶使能

	unsigned char isRPlus;			// 颜色分量Red增量方向位    1：灰阶值增加  0：灰阶值减小
	unsigned char RPlusVal;			// 颜色分量Red增量

	unsigned char isGPlus;			// 颜色分量Green增量方向位
	unsigned char GPlusVal;			// 颜色分量Green增量

	unsigned char isBPlus;			// 颜色分量Blue增量方向位
	unsigned char BPlusVal;			// 颜色分量Blue增量
}rgbAdjustInfo_t;

typedef struct _SYNC_SINGAL_LEVEL_
{
	unsigned char sync_pri_h;		// 行同步信号电平
	unsigned char sync_pri_v;		// 场同步电平
	unsigned char de;				// DE信号电平
}syncSingalLevelInfo_t;

typedef struct
{
#define DIS_FPGA_3D_PIN 0
#define EN_FPGA_3D_PIN  1

    int en;     // 是否使能3D引脚
}fpga3DPinInof_t;

typedef struct __LOGIC_SHOW__
{
    int validX,validY;          // 有效的XY的长度
    int majorType;				// 主图形类型
    int minorType;    			// 次图形类型
    int crossX[4];              // 回型图形中X的位置
    int crossY[4];              // 回型图形中Y的位置
    showPictureInfo_t mgs;
}logicShowInfo_t;

typedef struct
{
#define DIS_FPGA_SIGNAL 0
#define EN_FPGA_SIGNAL  1

    int state;     				// 开关状态
}fpgaOnOffSignalInfo_t;

typedef struct
{
	unsigned short offset;		// 寄存器偏移地址
	unsigned short value;		// 实际值
	unsigned char mask[16];		// 位操作掩码    0：对应位不用修改    1：对应位需要修改
}fpgaRegisterBitInfo_t;

#pragma pack(pop)


int fpga_reg_dev_ioctl(unsigned int cmd, unsigned long arg);

int fpga_trans_pic_data(const char *pName);


#endif
