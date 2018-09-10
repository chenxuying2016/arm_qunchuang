#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "fpgaFunc.h"
#include "fpgaRegister.h"
#include "fpgaDebug.h"
#include "util/util.h"

#include "comStruct.h"

static int fpgaRegisterWrite(const unsigned long arg)
{
	fpgaRegisterInfo_t *reg = NULL;
	unsigned short offset = 0;
	unsigned short value = 0;

	reg = (fpgaRegisterInfo_t *)arg;

	offset = reg->offset;
	value = reg->value;

	FPGA_SET_REG(offset) = value;

	return 0;
}

static int fpgaRegisterRead(unsigned long arg)
{
	fpgaRegisterInfo_t *reg = NULL;
	unsigned short offset = 0;
	unsigned short value = 0;

	reg = (fpgaRegisterInfo_t *)arg;
	offset = reg->offset;

	value = FPGA_SET_REG(offset);
	reg->value = value;
	FPGA_DBG( "offset = 0x%x, value = 0x%x\n", offset, value);

	return value;
}

static unsigned char fpgaReg2ResetAll(void)
{
	unsigned short reg = 0;

	// FPGA的复位：寄存器2的bit3位先置高再置低
	reg = FPGA_DDR_REG2;
    reg |= (1 << 3);			// 0x2_bit3
    FPGA_DDR_REG2 = reg;		// 0x2_bit3 置高

//	for (j = 0; j < 45000; j++);
    usleep(100);
	reg &= ~(1 << 3);
	FPGA_DDR_REG2 = reg;		// 0x2_bit3 置低

    printf("0reg bit3 reset %d\n",FPGA_DDR_REG2);
	return 0;
}

static unsigned char fpgaReg2ResetAllCheck(void)
{
	unsigned short reg = 0;

	reg = FPGA_DDR_REG3;
	usleep(100);
//	while ((reg & 0x01) != 1)
//	{
//		reg = FPGA_DDR_REG3;	// 0x3_bit0为1时向下执行
//	}

	return 0;
}

static int fpgaReadyEnable(unsigned long arg)
{
	return 0;
}

static int fpgaRightEnable(unsigned long arg)
{
	unsigned short reg = 0;

    //fpgaReg2ResetAll();			//FPGA的复位，spartan6
	reg = FPGA_DDR_REG2;
//	reg &= ~0x10;				// 0x2_bit4，1：显示十字光标；0：不显示十字光标
//	reg &= ~(1 << 5);			// 0x2_bit5 图片显示开关。1，显示；0，关闭
//	reg &= ~(1 << 9);			// 0x2_bit9 屏幕时钟触发开关
//	reg &= ~(1 << 11);			// 0x2_bit11 fpga打图状态置零，说明第一次启动
	reg |= (1 << 15);			// enable LVDS signal
//	reg &= ~(1 << 15);			// disable LVDS signal
	reg &= ~0x10;				// 0x2_bit4，1：显示十字光标；0：不显示十字光标
	reg &= ~(1 << 5);			// 0x2_bit5 图片显示开关。1，显示；0，关闭
	reg &= ~(1 << 9);			// 0x2_bit9 屏幕时钟触发开关
	FPGA_DDR_REG2 = reg;

	fpgaReg2ResetAllCheck();	// 复位检测

	// fpga 初始化
	FPGA_DDR_REGA = 0x0;		// 测试模式0
    FPGA_DDR_REGB = 0x0;		// 图片移动方式：0，不移动，0x8，up，0x10，down，0x1，left，0x2，right，0x18，垂直翻转，0x3，水平翻转，0x1B,中心翻转   bit6~bit13:图片移动速度
	FPGA_DDR_REG8 = 0;			// 水平偏置0~1919
	FPGA_DDR_REG9 = 0;			// 垂直偏置0~1079

	//十字光标设置
	FPGA_DDR_REG6 = 0;			// 十字光标的水平坐标 0~1919
	FPGA_DDR_REG7 = 0;			// 十字光标的垂直坐标 0~1079
	FPGA_DDR_REG1C = 0x0;		// 十字光标字体色默认值
	FPGA_DDR_REG1D = 0x0;
	FPGA_DDR_REG1E = 0x31FF;	// 十字光标背景色默认值
	FPGA_DDR_REG1F = 0xFFF0;

	return 0;
}

static int fpgaGetVersion(unsigned long arg)
{
	fpgaVersionInfo_t version;
	unsigned short tmp = 0;

	tmp = FPGA_DDR_REGD;
	FPGA_DBG( "year = %x\n", tmp);
	version.year = ((tmp >> 12) & 0xf) * 1000 + ((tmp >> 8) & 0xf) * 100 +
			((tmp >> 4) & 0xf) * 10 + ((tmp >> 0) & 0xf) * 1;

	tmp = FPGA_DDR_REGE;
	FPGA_DBG( "mmdd = %x\n", tmp);
	version.mmdd[0] = ((tmp >> 12) & 0xf) * 10 + ((tmp >> 8) & 0xf) * 1;
	version.mmdd[1] = ((tmp >> 4) & 0xf) * 10 + ((tmp >> 0) & 0xf) * 1;

	tmp = FPGA_DDR_REGC;
	FPGA_DBG( "version = %x\n", tmp);
	version.version[0] = ((tmp >> 12) & 0xf) * 10 + ((tmp >> 8) & 0xf) * 1;
	version.version[1] = ((tmp >> 4) & 0xf) * 10 + ((tmp >> 0) & 0xf) * 1;
	FPGA_DBG( "===version = %d, %d\n", version.version[0], version.version[1]);

	version.kernelVersion[0] = FPGA_VERSION_H;
	version.kernelVersion[1] = FPGA_VERSION_M;
	version.kernelVersion[2] = FPGA_VERSION_L;

    memcpy((void *) arg, &version, sizeof(version));

	return 0;
}

static int fpgaShowPicture(unsigned long arg)
{
	unsigned int reg = 0;
	showPictureInfo_t *show_picture_info = NULL;

    show_picture_info = (showPictureInfo_t *)arg;

	reg = (unsigned short) (show_picture_info->position & 0xff);
	reg |= (show_picture_info->size << 8);	// 图形尺寸寄存器

	FPGA_DBG( "position = %d\n", show_picture_info->position);
	FPGA_DBG( "size = %d\n", show_picture_info->size);

//	reg = (0 | 5 << 8);
    FPGA_DDR_REG4 = reg;		//显示图片编号寄存器
	reg = FPGA_DDR_REG2;
	reg |= (1 << 5);			// 0x2_bit5 图片显示开关。1，显示；0，关闭
    reg &= ~(1 << 10);			// 取消RGB调节信号
    reg |= (1 << 15);             //enable LVDS signal
	FPGA_DDR_REG2 = reg;		//置高

	reg = FPGA_DDR_REG13;		//
	reg &= ~(0x07 << 5);		// 清除RGB消色
	reg &= ~(0x01 << 12);		// 清除RGB 调节
	FPGA_DDR_REG13 = reg;

	reg = FPGA_DDR_REG3B;       //取消逻辑画面状态
	reg &= ~(1 << 1);
	FPGA_DDR_REG3B = reg;

    reg = FPGA_DDR_REG2;
    reg |= (1 << 11);
    FPGA_DDR_REG2 = reg;

    reg = FPGA_DDR_REG2;
    reg &= ~(1 << 11);
    FPGA_DDR_REG2 = reg;

	return 0;
}

static int fpgaReset(unsigned long arg)
{
    unsigned int reg = 0;

	//FPGA的复位，寄存器2的bit3位先置高再置低
	reg = FPGA_DDR_REG2;
	reg |= (1 << 3);			//0x2_bit3

    FPGA_DDR_REG2 = reg;		//0x2_bit3 置高

	usleep(5);

	reg &= ~(1 << 3);
	FPGA_DDR_REG2 = reg;		//0x2_bit3 置低

    printf("1reg bit3 reset %d\n",FPGA_DDR_REG2);
	return 0;
}

static int fpgaResetTest(unsigned long arg)
{
	int ret = 0;
	int count = 0;
	unsigned short reg = 0;

	reg = FPGA_DDR_REG3;		//复位检测

    count = arg;
	while ((reg & 0x01) != 1)
	{
		reg = FPGA_DDR_REG3;	//0x3_bit0为1时向下执行

		if (count < 0)
		{
			ret = 0 - fpgaResetTestError;
			break;
		}
        usleep(1000);
		count--;
	}

	return ret;
}

static int fpgaCrossCursorSet(unsigned long arg)
{
	unsigned short reg16, wordColorHigh, wordColorLow, cursorColorHigh, cursorColorLow;
	crossCursorInfo_t crossCursor;
	unsigned char *tmp, startCoordinate;
	unsigned short tmp16 = 0;

	memcpy((void *)&crossCursor, (void *)arg, sizeof(crossCursor));
	tmp = (unsigned char *)arg;

	FPGA_DBG( "crossCursor.x = %d, crossCursor.y = %d\n", crossCursor.x, crossCursor.y);

	// 设置十字光标坐标
	FPGA_DDR_REG6 = crossCursor.x;
	FPGA_DDR_REG7 = crossCursor.y;

    FPGA_DDR_REGF = crossCursor.x;

	cursorColorHigh = (unsigned short)((crossCursor.crossCursorColor) >> 16) & 0xFFFF;
	cursorColorLow = (unsigned short)(crossCursor.crossCursorColor) & 0xFFFF;

	wordColorHigh = (unsigned short)((crossCursor.wordColor) >> 16) & 0xFFFF;
	wordColorLow = (unsigned short)(crossCursor.wordColor) & 0xFFFF;

	startCoordinate = crossCursor.startCoordinate;

	FPGA_DBG( "cursorColorHigh = 0x%x, cursorColorLow = 0x%x, wordColorHigh = 0x%x, wordColorLow = 0x%x\n",
		cursorColorHigh, cursorColorLow, wordColorHigh, wordColorLow);

	// 设置十字光标 颜色
	// 光标和字背景颜色
	FPGA_DDR_REG1E = cursorColorHigh;
	FPGA_DDR_REG1F = cursorColorLow;

	// 字颜色
	FPGA_DDR_REG1C = wordColorHigh;
    // 设置字体颜色，及设置当前的RGB的值
    tmp16 = (wordColorLow & 0xfffc) | (crossCursor.RGBchang & 0x03);
    FPGA_DBG( "tmp16 = 0x%x, RGBchang = 0x%x\r\n", tmp16, crossCursor.RGBchang);
    FPGA_DDR_REG1D = tmp16 ;// 低2bit作为当前RGB的选择，0表示全部。

	if (crossCursor.enable)
	{
		reg16 = FPGA_DDR_REG2;
		reg16 |= (1 << 8) | (1 << 4);	// 4 开始十字光标 8 显示像素信息
		if (startCoordinate == 0)
		{
			reg16 &= ~(1 << 14);
		}
		else
		{
			reg16 |= (1 << 14);
		}

		FPGA_DDR_REG2 = reg16;
		FPGA_DBG( "FPGA_DDR_REG2 = 0x%x\n", reg16);
	}
	else
	{
		reg16 = FPGA_DDR_REG2;
		reg16 &= ~(1 << 8) & ~(1 << 4);
		FPGA_DDR_REG2 = reg16;
	}

	return 0;
}

static int fpgaReg3ResetCheck(void)
{
	unsigned short reg;
	int flag = 0;

	while (1)
	{
		reg = FPGA_DDR_REG3;
		reg = reg & 1;
		if (0 != reg)
		{
			break;
		}

		if (flag++ > 300)
		{
			break;
		}

		usleep(100);
	}

    if (flag >= 300)
    {
		return -1;
	}

	return 0;
}


static void fpgaReg2Reset(void)
{
    struct timeval begin_time, end_time;
    unsigned int reg2 = 0;

	reg2 = FPGA_DDR_REG2;
	reg2 |= 0x1;				//0x2_bit0置1，表示对FPGA写复位
	FPGA_DDR_REG2 = reg2;

    //printf("reset A the reg2 0x%x\n",FPGA_DDR_REG2);

    gettimeofday(&begin_time, 0);

    usleep(1000);

    gettimeofday(&end_time, 0);
    reg2 &= ~0x1;				//0x2_bit0置0，表示对FPGA写正常
    FPGA_DDR_REG2 = reg2;

    FPGA_DBG(FPGA_WARNING, "Begin time: %ld.%ld\n", begin_time.tv_sec, begin_time.tv_usec);
    FPGA_DBG(FPGA_WARNING, "End time: %ld.%ld\n", end_time.tv_sec, end_time.tv_usec);

    //printf("reset B the reg2 0x%x\n",FPGA_DDR_REG2);
}

static void fpgaDdrChipSelection(unsigned char select)
{	//FPGA_DDR设置   0x2_bit7写设置,0x2bit8、9读设置
	unsigned short reg2 = 0;

	reg2 = FPGA_DDR_REG2;
	switch (select)
	{
		case 0:
			reg2 &= ~(1 << 7);	//0x2_bit7,write DDR select
			break;
		case 1:
			reg2 |= (1 << 7);
			break;
		default:
			break;
	}

	FPGA_DDR_REG2 = reg2;
	usleep(1);
	FPGA_DDR_REG2 = reg2;
}

static int fpgaPictureDownloadConfig(unsigned long arg)
{
	int fileLength = 0;
	fpgaDdrBufferInfo_t *ddrBuffer = NULL;
	unsigned char ddrCs = 0;
	unsigned char tmpSize = 0;
	unsigned short completion = 0;		// 当X坐标不足以整除32，则补全32
    unsigned int reg = 0;				// fpga寄存器值

	ddrBuffer = (fpgaDdrBufferInfo_t *)arg;

	fileLength = ddrBuffer->fileLength;
	ddrCs = ddrBuffer->ddrCs;

	if (0 != fpgaReg3ResetCheck())
	{
		printf("fpgaPictureDownloadConfig error: ddrCs = %d, ddrCs Reg3 bit 3 can't write.\r\n", ddrCs);
		return -1;
	}

    if (PICTURE_TYPE_JPG == ddrBuffer->pictureType)		// JPG图片
    {
        //printf("fpgaPictureDownloadConfig: JPG Picture.\n");
        reg = FPGA_DDR_REG3B;
        reg = reg | (1 << 12);
        FPGA_DDR_REG3B = reg;
    }
    else		// BMP或逻辑画面
    {
    	//printf("fpgaPictureDownloadConfig: BMP or Logic Picture\n");
        reg = FPGA_DDR_REG3B;
        FPGA_DBG("bmp picture load reg %d\n",reg);
        //reg = reg & (~(1 << 12));
        reg = reg | (1 << 13);
        FPGA_DDR_REG3B = reg;

        FPGA_DBG("bmp picture load\n");
    }

	if (0 == ddrCs)
    {
		// 当X坐标不足以整除32，则补全32， 显示时用
		completion = 32 - (ddrBuffer->hResolution % 32);
		if (32 == completion)
		{
			completion = 0;
		}

		FPGA_DDR_REG13 = completion;
		fpgaDdrChipSelection(0);		// FPGA_DDR设置  0x2_bit7写设置

        tmpSize = ddrBuffer->pictureSize;
        FPGA_DDR_REG5 = ddrBuffer->pictureNo | (tmpSize << 8);

		fpgaReg2Reset();			// 写复位
        FPGA_DBG( "DDR0 %x %x\n",ddrBuffer->pictureNo,tmpSize << 8);
	}
	else if (1 == ddrCs)
	{
		fpgaReg2Reset();
		fpgaDdrChipSelection(1);
		FPGA_DBG( "DDR1\n");
	}

	FPGA_DBG( "fileLength = %d, tmpSize = %d, pictureNo = %d\r\n", fileLength, tmpSize, ddrBuffer->pictureNo);

	return 0;
}

static int fpgaLinkSet(unsigned long arg)
{
	unsigned short reg = 0;
	int *cmd = NULL;

	reg = FPGA_DDR_REG1;
	cmd = (int *)arg;

	switch (*cmd)
	{
		case 1:
			//reg &= LINK1;
			reg &= ~(1 << 2);
			reg &= ~(1 << 3);
			//FPGA_DDR_REG13 = 10;
            FPGA_DBG( "LINK1\r\n");
			break;

		case 2:
			//reg |= LINK2;
			reg |= (1 << 2);
			reg &= ~(1 << 3);
			FPGA_DBG( "LINK2\r\n");
			break;

		case 4:
			//reg |= LINK4;
			reg |= (1 << 2);
			reg |= (1 << 3);
			FPGA_DBG( "LINK4\r\n");
			break;

		default:
			//reg |= LINK2;
			//reg = 0;
			reg |= (1 << 2);
			reg &= ~(1 << 3);
			//reg |= BIT10;
			FPGA_DBG( "default LINK2\r\n");
			break;
	}

	//reg &= LINK1;
	//FPGA_DDR_REG13 = 10;
	FPGA_DDR_REG1 = reg;

	FPGA_DBG( "Link reg = %d\r\n", reg);

	return 0;
}

static int fpgaSeqSet(unsigned long arg)
{
	fpgaSeqInfo_t *fpgaSeq = NULL;

	fpgaSeq = (fpgaSeqInfo_t *)arg;

	FPGA_DBG( "fpgaSeq.T0 = %d\n", fpgaSeq->T0);
	FPGA_DBG( "fpgaSeq.T1 = %d\n", fpgaSeq->T1);
	FPGA_DBG( "fpgaSeq.T2 = %d\n", fpgaSeq->T2);
	FPGA_DBG( "fpgaSeq.T3 = %d\n", fpgaSeq->T3);
	FPGA_DBG( "fpgaSeq.T4 = %d\n", fpgaSeq->T4);
	FPGA_DBG( "fpgaSeq.T5 = %d\n", fpgaSeq->T5);
	FPGA_DBG( "fpgaSeq.T6 = %d\n", fpgaSeq->T6);
	FPGA_DBG( "fpgaSeq.T7 = %d\n", fpgaSeq->T7);

	HSD_BACK_PORCH = fpgaSeq->T0;
	HSD_PLUSE = fpgaSeq->T1;
	HSD_FRONT = fpgaSeq->T2;
	HSD_DISPLAY = fpgaSeq->T3;
	VSD_BACK_PORCH = fpgaSeq->T4;
	VSD_PLUSE = fpgaSeq->T5;
	VSD_FRONT = fpgaSeq->T6;
	VSD_DISPLAY = fpgaSeq->T7;

	return 0;
}

static int fpgaBitSet(unsigned long arg)
{
	unsigned short reg = 0;
	int *cmd = NULL;

	reg = FPGA_DDR_REG1;
	cmd = (int *)arg;
	FPGA_DBG( "setFpgaBit reg = %d\n", reg);

	switch (*cmd)
	{
		case 6:
            //reg |= VBIT6;
			reg |= (1 << 0);
			reg &= ~(1 << 1);
			FPGA_DBG( "Bit6\r\n");
			break;

		case 8:
            //reg |= VBIT8;
			reg &= ~(1 << 0);
			reg |= (1 << 1);
			FPGA_DBG( "Bit8\r\n");
			break;

		case 10:
            reg |= VBIT10;
			FPGA_DBG( "Bit10\r\n");
			break;

		default:
//			reg = 0;
//			reg |= (1 << 2);
//			reg &= ~(1 << 3);
            reg |= VBIT10;
			FPGA_DBG( "default Bit10\r\n");
			break;
	}

	FPGA_DDR_REG1 = reg;
	FPGA_DBG( "setFpgaBit reg = %d\n", reg);

	return 0;
}

static int fpgaImageMove(unsigned long arg)
{
	imageMoveInfo_t imageMove;
	unsigned short regB, reg2B;
	unsigned char xdirect , ydirect, xstep, ystep;

	memcpy(&imageMove, (void *)arg, sizeof(imageMove));

	xdirect = imageMove.Xmsg & 0xFF;
	ydirect = imageMove.Ymsg & 0xFF;

	xstep = imageMove.Xmsg >> 16;
	ystep = imageMove.Ymsg >> 16;

	FPGA_DBG( "Xmsg = 0x%x\r\n", imageMove.Xmsg);
	FPGA_DBG( "Ymsg = 0x%x\r\n", imageMove.Ymsg);
	FPGA_DBG( "xdirect = 0x%x, ydirect = 0x%x\r\n", xdirect, ydirect);
	FPGA_DBG( "xstep = 0x%x, ystep = 0x%x\r\n", xstep, ystep);

	regB = (xdirect & 0x7) | ((ydirect & 0x7) << 3) | (xstep << 6);
	reg2B = ystep << 6;

	FPGA_DDR_REGB = regB;
	FPGA_DDR_REG2B = reg2B;
	FPGA_DBG( "FPGA_DDR_REGB = 0x%x\n", regB);
	FPGA_DBG( "FPGA_DDR_REG2B = 0x%x\n", reg2B);

	return 0;
}

static int fpgaSignalModeSet(unsigned long arg)
{
	signalModeInfo_t signalMode;
	unsigned short reg = 0;

	FPGA_DBG( "fpgaSetSignalMode\n");
	reg = FPGA_DDR_REG1;
	memcpy(&signalMode, (void *)arg, sizeof(signalMode));

	reg &= 0x00FF;

	reg |= (signalMode.link0 & 0x3) << 8;
	reg |= (signalMode.link1 & 0x3) << 10;
	reg |= (signalMode.link2 & 0x3) << 12;
	reg |= (signalMode.link3 & 0x3) << 14;

	FPGA_DBG( "FPGA_DDR_REG1 = 0x%04x\r\n", reg);

	FPGA_DDR_REG1 = reg;

	return 0;
}

static int fpgaFreqSet (unsigned long arg)
{
	int tmp /*,freq, division, multiplication, maxMultip */ ;
	unsigned short reg, reg3, reg2;
	freqConfigInfo_t *freqConfig = NULL;
	unsigned short d0, m, d1;

	freqConfig = (freqConfigInfo_t *) arg;

	d0 = freqConfig->d0;
	m = freqConfig->m;
	d1 = freqConfig->d1;
	FPGA_DBG( "d0 = %d m = %d d1 = %d\r\n", d0, m, d1);

	reg = ((d1 << 8) & 0xff00) | ((m) & 0x00ff);
	FPGA_DBG( "reg26 = 0x%x\r\n", reg);
	FPGA_DDR_REG26 = reg;

	reg = ((d0 << 8) & 0xff00);
	reg = d0;
	FPGA_DBG( "reg27 = 0x%x\r\n", reg);
	FPGA_DDR_REG27 = reg;

	usleep(1000);

	reg3 = FPGA_DDR_REG3;
	tmp = 0;
	while ((reg3 & (0x1 << 4)) != (1 << 4))
	{
		reg3 = FPGA_DDR_REG3;
		tmp++;
		if (tmp > 0xffff)
		{
			FPGA_DBG( "freq timeout\n");
			return 1;
		}
    }

	reg2 = FPGA_DDR_REG2;
	reg2 |= (1 << 9);			//0x2_bit9位先置高再置低，高电平保持>200ns
    reg2 |= (1 << 11);
	FPGA_DDR_REG2 = reg2;

	usleep(100);

	reg2 = FPGA_DDR_REG2;
	reg2 &= ~(1 << 9);
    reg2 &= ~(1 << 11);

	FPGA_DDR_REG2 = reg2;

	return 0;
}

static int fpgaShowRgb(unsigned long arg)
{
	unsigned short reg16 = 0;
	showRgbInfo_t showRgb;

	memcpy(&showRgb, (unsigned char *)arg, sizeof(showRgb));

	// 设定颜色
	reg16 = ((showRgb.rgb_r) << 2) + ((showRgb.rgb_g) << 12);
	FPGA_DDR_REG11 = reg16;

	reg16 = ((showRgb.rgb_b) << 6) + ((showRgb.rgb_g) >> 4);
	FPGA_DDR_REG10 = reg16;

	FPGA_DBG( "RGB R = %02x\n", showRgb.rgb_r);
	FPGA_DBG( "RGB G = %02x\n", showRgb.rgb_g);
	FPGA_DBG( "RGB B = %02x\n", showRgb.rgb_b);

    // 使能RGB调节
    reg16 = FPGA_DDR_REG2;
    reg16 |= (1 << 10);
    FPGA_DDR_REG2 = reg16;

//	reg16 = FPGA_DDR_REG3B;   //使能RGB调节
//	reg16 |= (1 << 1);
//	FPGA_DDR_REG3B = reg16;

	return 0;
}

static int fpgaVesaJedaSwitch(unsigned long arg)
{
	unsigned short reg16 = 0;
	vesaJedaSwitchInfo_t *vesaJedaSwitch;

	memcpy(&vesaJedaSwitch, (unsigned char *)arg, sizeof (vesaJedaSwitch));
	vesaJedaSwitch = (vesaJedaSwitchInfo_t *)arg;

	reg16 = FPGA_DDR_REG1;
	switch (vesaJedaSwitch->lvdsChange)
	{
		case VESA:
			reg16 &= ~(1 << 7);
            FPGA_DBG( "VESA mode \r\n");
			break;

		case JEDA:
			reg16 |= (1 << 7);
            FPGA_DBG( "JEDA mode \r\n");
			break;

		default:
			reg16 &= ~(1 << 7);
            FPGA_DBG( "default VESA mode \r\n");
			break;
	}

	FPGA_DDR_REG1 = reg16;

	return 0;
}

static int fpgaLvdsTest(unsigned long arg)
{
	int i = 0, j = 0;
	unsigned short reg = 0;
	int len = 0;

	lvdsOpenShortTestInfo_t lvdsOpenShortTest;

	memcpy(&lvdsOpenShortTest, (void *)arg, sizeof(lvdsOpenShortTest));

	len = lvdsOpenShortTest.link * 6;

	FPGA_DBG( "len = %d\n", len);

	FPGA_DDR_REG1 = 0x1;		//REG1_bit0 复位，下降沿，高电平保持1us

	usleep(10);

	FPGA_DDR_REG1 = 0;

	for (i = 0; i < len; i++)
	{
		lvdsOpenShortTest.status[i] = 0;
	}

	for (i = 0; i < len; i++)
	{
		FPGA_DDR_REG2 = 0x1;	//REG2_bit0 触发测试，下降沿，高电平保持1us

		usleep(1);

		FPGA_DDR_REG2 = 0;
		reg = FPGA_DDR_REG3;

		j = 0;
		while (((reg & 0x1) != 0) && (j < 0xfff))
		{
			reg = FPGA_DDR_REG3;
			j++;
		}

		lvdsOpenShortTest.status[i] = FPGA_DDR_REG4;
	}

	for (i = 0; i < len; i++)
	{
		FPGA_DBG( "lvdsOpenShortTest[%d] = %02x\n", i, lvdsOpenShortTest.status[i]);
	}

	memcpy((void *) arg, &lvdsOpenShortTest, sizeof(lvdsOpenShortTest));

	return 0;
}

static int fpgaClockkSet(unsigned long arg)
{
	int enableClock = 0;
	unsigned short reg16 = FPGA_DDR_REG2;

	memcpy(&enableClock, (unsigned char *)arg, sizeof(enableClock));

	if (1 == enableClock)		// 开启clock
	{
		reg16 |= (1 << 12);
	}
	else						// 关闭clock
	{
		reg16 &= ~(1 << 12);
	}

	FPGA_DDR_REG2 = reg16;

	return 0;
}

static int fpgaImageOffset(unsigned long arg)
{
	imageOffsetInfo_t *pOffset;

	pOffset = (imageOffsetInfo_t *)arg;

	FPGA_DDR_REG8 = pOffset->Xoffset;
	FPGA_DDR_REG9 = pOffset->Yoffset;

	return 0;
}

static int fpgaRgbDisable(unsigned long arg)
{
	disableRgbInfo_t *pDisableRgb = NULL;
	unsigned short reg = 0;
	unsigned char tmp = 0;

	pDisableRgb = (disableRgbInfo_t *)arg;
	FPGA_DBG( "pDisRgb->disableR == %d\r\n", pDisableRgb->disableR);
	FPGA_DBG( "pDisRgb->disableG == %d\r\n", pDisableRgb->disableG);
	FPGA_DBG( "pDisRgb->disableB == %d\r\n", pDisableRgb->disableB);

	reg = FPGA_DDR_REG13;
	reg &= ~(0x07 << 5);

	tmp = 0;
	tmp |= (pDisableRgb->disableR & 0x01) << 2;
	tmp |= (pDisableRgb->disableG & 0x01) << 1;
	tmp |= (pDisableRgb->disableB & 0x01) << 0;

	FPGA_DBG( "tmp = %d\r\n", tmp);
	switch (tmp)
	{
		case 0:
			// 消除 RGB 000
			reg |= (0x07 << 5);
			break;

		case 1:
			// 消除RG   001
			reg |= (0x04 << 5);;
			break;

		case 2:
			// 消除RB   010
			reg |= (0x05 << 5);
			break;

		case 3:
			// 消除R    011;
			reg |= (0x01 << 5);
			break;

		case 4:
			// 消除GB   100
			reg |= (0x06 << 5);
			break;

		case 5:
			// 消除G    101;
			reg |= (0x02 << 5);
			break;

		case 6:
			// 消除B    110;;
			reg |= (0x03 << 5);
			break;

		case 7:
			// 恢复;
			reg &= ~(0x07 << 5);
			break;

		default:
			// 恢复;
			reg &= ~(0x07 << 5);
			break;
	}

	FPGA_DBG( "reg = 0x%x\r\n", reg);
	FPGA_DDR_REG13 = reg;

	return 0;
}

static int fpgaGrayScaleAdjust(unsigned long arg)
{
	rgbAdjustInfo_t *pRgbAdjust = NULL;
	unsigned short reg13, reg21, reg22;

	pRgbAdjust = (rgbAdjustInfo_t *)arg;

	reg13 = FPGA_DDR_REG13;
	reg21 = FPGA_DDR_REG21;
	reg22 = FPGA_DDR_REG22;

	if (ENABLE_RGB_ADJUST == pRgbAdjust->enable)
	{
		reg13 |= (1 << 12);
	}
	else
	{
		reg13 &= ~(1 << 12);
	}

	if (RGB_PLUS == pRgbAdjust->isRPlus)
	{
		reg13 |= (1 << 11);
	}
	else
	{
		reg13 &= ~(1 << 11);
	}

	if (RGB_PLUS == pRgbAdjust->isGPlus)
	{
		reg13 |= (1 << 10);
	}
	else
	{
		reg13 &= ~(1 << 10);
	}

	if (RGB_PLUS == pRgbAdjust->isBPlus)
	{
		reg13 |= (1 << 9);
	}
	else
	{
		reg13 &= ~(1 << 9);
	}

	reg21 = (pRgbAdjust->RPlusVal & 0xFF) | ((pRgbAdjust->GPlusVal & 0xFF) << 8);
	reg22 = (pRgbAdjust->BPlusVal & 0xFF);

	FPGA_DBG( "en = %d, isRPlus = %d, isGPlus = %d, isBPlus = %d\r\n",
		pRgbAdjust->enable, pRgbAdjust->isRPlus, pRgbAdjust->isGPlus, pRgbAdjust->isBPlus);

	FPGA_DBG( "RPlusVal = %d, RPlusVal = %d RPlusVal = %d\r\n",
		pRgbAdjust->RPlusVal, pRgbAdjust->GPlusVal, pRgbAdjust->BPlusVal);

	FPGA_DBG( "reg13 = 0x%04x reg21 = 0x%04x reg22 = 0x%04x", reg13, reg21, reg22);

	FPGA_DDR_REG13 = reg13;
	FPGA_DDR_REG21 = reg21;
	FPGA_DDR_REG22 = reg22;

	return 0;
}

static int fpgaSignalSyncLevel(unsigned long arg)
{
	syncSingalLevelInfo_t *pSync = NULL;
	unsigned short reg1 = 0;

	pSync = (syncSingalLevelInfo_t *)arg;

	reg1 = FPGA_DDR_REG1;
    FPGA_DBG( "sync_pri_h = 0x%02x, sync_pri_v = 0x%02x, de = 0x%02x\r\n", pSync->sync_pri_h, pSync->sync_pri_v, pSync->de);

	if (0 == pSync->sync_pri_h)
    {// 行同步
        reg1 &= ~(1 << 5);
	}
	else
	{
        reg1 |= (1 << 5);
	}

	if (0 == pSync->sync_pri_v)
    {// 场同步
        reg1 &= ~(1 << 6);
	}
	else
	{
        reg1 |= (1 << 6);
	}

	if (0 == pSync->de)
    {// de信号
        reg1 &= ~(1 << 4);
	}
	else
	{
        reg1 |= (1 << 4);
	}

	FPGA_DDR_REG1 = reg1;

	return 0;
}

static int fpgaEn3DPin(unsigned long arg)
{
	unsigned short reg2 = 0;
	fpga3DPinInof_t *pfpga3DPin = NULL;

    pfpga3DPin = (fpga3DPinInof_t *)arg;

    reg2 = FPGA_DDR_REG2;

    if (DIS_FPGA_3D_PIN == pfpga3DPin->en)
    {
        reg2 &= ~(1 << 6);
    }
    else
    {
        reg2 |= (1 << 6);
    }

    FPGA_DDR_REG2 = reg2;

    return 0;
}

static int fpgaLogicShowOperate(showPictureInfo_t *pShowPicture)
{
	unsigned short reg = 0;

    reg = (unsigned short)(pShowPicture->position & 0xff);
    reg |= (pShowPicture->size << 8);	// 图形尺寸寄存器
    FPGA_DBG( "number = %d\n", pShowPicture->position);
    FPGA_DBG( "picSize = %d\n", pShowPicture->size);

//	reg = (0 | 5 << 8);
    FPGA_DDR_REG4 = reg;		//显示图片编号寄存器
    reg = FPGA_DDR_REG2;
    reg |= (1 << 5);			// 0x2_bit5 图片显示开关。1，显示；0，关闭
    FPGA_DDR_REG2 = reg;		//置高

//	reg = FPGA_DDR_REG13;		//
//	reg &= ~(0x07 << 5);		// 清除RGB消色
//	reg &= ~(0x01 << 12);		// 清除RGB 调节
//	FPGA_DDR_REG13 = reg;

    return 0;
}

static int fpgaLogicShow(unsigned long arg)
{
	logicShowInfo_t *pShowMsg = NULL;
    unsigned long ptnMsg = 0;
    unsigned short reg16 = 0;

    pShowMsg = (logicShowInfo_t *)arg;
    ptnMsg = (unsigned long)&pShowMsg->mgs;

    // 使能逻辑画面
    reg16 = FPGA_DDR_REG3B;
    reg16 |= (1 << 1);
    FPGA_DDR_REG3B = reg16;

    reg16 = FPGA_DDR_REG2;
    reg16 |= (1 << 15);
    FPGA_DDR_REG2 = reg16;

	// 数据宽度由12位扩展到13位
    FPGA_SET_REG(0x30) = (pShowMsg->validX & 0x1fff);
    FPGA_SET_REG(0x31) = (pShowMsg->validY & 0x1fff);

    reg16 = (pShowMsg->majorType & 0x7) << 3 | (pShowMsg->minorType & 0x7);
    FPGA_DBG( "type = 0x%x\r\n", reg16);
    FPGA_DBG( "pShowMsg->validX = %d, pShowMsg->validY = %d\r\n", pShowMsg->validX, pShowMsg->validY);
    FPGA_SET_REG(0X32) = reg16;

    FPGA_SET_REG(0x33) = pShowMsg->crossX[0];
    FPGA_SET_REG(0x34) = pShowMsg->crossY[0];
    FPGA_SET_REG(0x35) = pShowMsg->crossX[1];
    FPGA_SET_REG(0x36) = pShowMsg->crossY[1];
    FPGA_SET_REG(0x37) = pShowMsg->crossX[2];
    FPGA_SET_REG(0x38) = pShowMsg->crossY[2];
    FPGA_SET_REG(0x39) = pShowMsg->crossX[3];
    FPGA_SET_REG(0x3a) = pShowMsg->crossY[3];

    FPGA_DBG( "pShowMsg->mgs.picNo = %d, pShowMsg->mgs.picSiz = %d\r\n", pShowMsg->mgs.position , pShowMsg->mgs.size);
	fpgaLogicShowOperate(&pShowMsg->mgs);

    reg16 = FPGA_DDR_REG2;
    reg16 |= (1 << 11);
    reg16 &= (~(1 << 10));
    FPGA_DDR_REG2 = reg16;

    reg16 = FPGA_DDR_REG2;
    reg16 &= ~(1 << 11);
    FPGA_DDR_REG2 = reg16;

    return 0;
}

static int fpgaOnoffSignal(unsigned long arg)
{
	unsigned short reg2 = 0;
	fpgaOnOffSignalInfo_t *pOnOff = NULL;

    pOnOff = (fpgaOnOffSignalInfo_t *)arg;
    reg2 = FPGA_DDR_REG2;

    switch (pOnOff->state)
    {
    case DIS_FPGA_SIGNAL:
        reg2 &= ~(1 << 15);     //disable LVDS signal
        break;
    case EN_FPGA_SIGNAL:
        reg2 |= (1 << 15);      //enable LVDS signal
        break;
    default:
        reg2 |= (1 << 15);      //enable LVDS signal
        break;
    }

    FPGA_DDR_REG2 = reg2;

    return 0;
}

static int fpgaRegisterBit(unsigned long arg)
{
	fpgaRegisterBitInfo_t *pRegBits = NULL;
	fpgaRegisterInfo_t reg;
	unsigned short value = 0;
	int i = 0;

	pRegBits = (fpgaRegisterBitInfo_t *)arg;
	for (i = 0; i < 16; i++)
	{
		if (1 == pRegBits->mask[i])
		{
			value |= (1 << i);
		}
	}

	reg.offset = pRegBits->offset;
	fpgaRegisterRead((unsigned long)&reg);

	reg.value &= (~value);
	reg.value |= pRegBits->value;
	fpgaRegisterWrite(((unsigned long)&reg));

	return 0;
}

int fpga_reg_dev_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

//	FPGA_DBG( "cmd = %d\n", cmd);  xujie
	switch(cmd)
	{
		case eSET_FPGA_REG:
		{
			ret = fpgaRegisterWrite(arg);
			break;
		}
		case eREADD_FPGA_REG:
		case eREAD_FPGA_REG:
		{
			ret = fpgaRegisterRead(arg);
			break;
		}
		case eENABLE_READY_FPGA:		// 下载FPGA前的初始化
		{
			ret = fpgaReadyEnable(arg);
			break;
		}
		case eENABLE_RIGHT_FPGA:		// 测试下载FPGA是否成功
		{
			ret = fpgaRightEnable(arg);
			break;
		}
		case eGET_FPGA_VER:			// 获取FPGA版本号
		{
			ret = fpgaGetVersion(arg);
			break;
		}
		case eSET_FPGA_SHOW_MODE:	// 图片显示
		{
			ret = fpgaShowPicture(arg);
			break;
		}
		case eRESET_FPGA:				// FPGA复位
		{
			ret = fpgaReset(arg);
			break;
		}
		case eRESET_FPGA_TEST:			// 测试FPGA复位是否成功
		{
			ret = fpgaResetTest(arg);
			break;
		}
		case eSET_CROSSCURSOR:		// 设置十字光标的坐标及颜色
		{
			ret = fpgaCrossCursorSet(arg);
			break;
		}
		case eDOWN_PICTURE:		// 下载图片
		{
			ret = fpgaPictureDownloadConfig(arg);
			break;
		}
		case eSET_LINK:			// 设置Fpga link
		{
			ret = fpgaLinkSet(arg);
			break;
		}
		case eSET_SEQ:			// 设置FPGA 时序
		{
			ret = fpgaSeqSet(arg);
			break;
		}
		case eSET_BIT:			// FPGA bit数
		{
			ret = fpgaBitSet(arg);
			break;
		}
		case eSET_CLOCK:		// 设置FPGA时钟
		{
			break;
		}
		case ePIC_MOV:			// 图形移动
		{
			ret = fpgaImageMove(arg);
			break;
		}
		case eSET_SIGNAL_MODE:	// 信号模式
		{
			ret = fpgaSignalModeSet(arg);
			break;
		}
		case eSET_FPGA_FREQ:	// 设置FPGA输出频率
		{
			ret = fpgaFreqSet(arg);
			break;
		}
		case eSET_FPGA_RGB:		// 设置RGB
		{
			ret = fpgaShowRgb(arg);
			break;
		}
		case eSET_VESA_JEDA:
		{
			ret = fpgaVesaJedaSwitch(arg);
			break;
		}
		case eTEST_LVDS:
		{
			ret = fpgaLvdsTest(arg);
			break;
		}
		case eFPGA_CLOCK_ENABLE:
		{
			ret = fpgaClockkSet(arg);
			break;
		}
		case eIMAGE_OFFSET:
		{
			ret = fpgaImageOffset(arg);
			break;
		}
		case eDISABLE_RGB:
		{
			ret = fpgaRgbDisable(arg);
			break;
		}
		case eGRAY_SCALE_ADJUST:
		{
			ret = fpgaGrayScaleAdjust(arg);
			break;
		}
		case eSIGNAL_SYNC_LEVEL:
		{
			ret = fpgaSignalSyncLevel(arg);
			break;
		}
		case eFPGA_3D_PIN:
		{
			ret =fpgaEn3DPin(arg);
			break;
		}
		case eFPGA_SHOW_LOGIC:
		{
			ret = fpgaLogicShow(arg);
			break;
		}
		case eFPGA_ONOFF_SIGNAL:
		{
			ret = fpgaOnoffSignal(arg);
			break;
		}
		case eSET_FPGA_REG_BIT:
		{
			ret = fpgaRegisterBit(arg);
			break;
		}
		default:
		{
			FPGA_DBG( "0x%x command does not exist.\r\n", cmd);
			break;
		}
	}

	return ret;
}

int fpga_trans_pic_data(const char *pName)
{
    unsigned char *pRgbBuffer = 0;
    unsigned char *pTemp = 0;
    unsigned int  u32RgbSize  = 0;
    struct timeval begin_time, end_time;
    int sumWriteSize;
    int maxWrite = 320*1024;

    gettimeofday(&begin_time, 0);

    //parseBmp(pName,&pRgbBuffer,&u32RgbSize);
    loadPtnToMem(pName,&pRgbBuffer,&u32RgbSize);

    //printf("pRgbBuffer %x size %d\n",pRgbBuffer,u32RgbSize);

    gettimeofday(&end_time, 0);

    FPGA_DBG(FPGA_WARNING, "Begin time: %ld.%ld\n", begin_time.tv_sec, begin_time.tv_usec);
    FPGA_DBG(FPGA_WARNING, "End time: %ld.%ld\n", end_time.tv_sec, end_time.tv_usec);

    begin_time =  end_time;

    pTemp = pRgbBuffer;

    //printf("g_fpga_buf_dev_fd is %d\n", g_fpga_buf_dev_fd);

    while(u32RgbSize>0)
    {
        int s32WriteSize,actWriteSize = 0;
        if(u32RgbSize>maxWrite)
        {
            s32WriteSize = maxWrite;
        }
        else
        {
            s32WriteSize = u32RgbSize;
        }
        //printf("write size %d\n",s32WriteSize);
        sumWriteSize = 0;
        while(sumWriteSize<s32WriteSize)
        {
            actWriteSize = write(g_fpga_buf_dev_fd,&pTemp[sumWriteSize],s32WriteSize-sumWriteSize);
            if(actWriteSize<0)
            {
                free(pRgbBuffer);
                return -2;
            }
            //usleep(10);
            sumWriteSize+= actWriteSize;
        }
        u32RgbSize -= s32WriteSize;
        pTemp += s32WriteSize;
    }
    gettimeofday(&end_time, 0);

    free(pRgbBuffer);

    int addBytes;
    char testBuf[64];
    memset(testBuf,0,sizeof(testBuf));
    addBytes = write(g_fpga_buf_dev_fd,testBuf,sizeof(testBuf));
    FPGA_DBG(FPGA_WARNING, "sum %d add %d\n",sumWriteSize,addBytes);
    FPGA_DBG(FPGA_WARNING, "Begin time: %ld.%ld\n", begin_time.tv_sec, begin_time.tv_usec);
    FPGA_DBG(FPGA_WARNING, "End time: %ld.%ld\n", end_time.tv_sec, end_time.tv_usec);

	return 0;
}

void fpga_write(unsigned short reg, unsigned short val)
{
	fpgaRegStr fpga_reg = { 0 };
	fpga_reg.ofset = reg;
	fpga_reg.value = val;
	int rc = fpga_reg_dev_ioctl(eSET_FPGA_REG, &fpga_reg);
	printf("fpga_write: 0x%02x, val: %#x.\n", fpga_reg.ofset, fpga_reg.value);
}

unsigned short fpga_read(unsigned short reg)
{
	fpgaRegStr fpga_reg = { 0 };
	fpga_reg.ofset = reg;
	unsigned short val = fpga_reg_dev_ioctl(eREAD_FPGA_REG, &fpga_reg);
	return val;
}

