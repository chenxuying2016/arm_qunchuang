#include "mipi2828.h"
#include "vcom.h"
#include "pubmipi.h"
#include "pubFpga.h"

#include "hi_unf_spi.h"

CONFIG2828_STRUCT SSD2828;
//unsigned  char  Channel = 1; //xujie 修改默认值为1,  原始是没有赋值的

uint16_t VsCount;//4;
uint16_t VBPCount;//4;
uint16_t VFPCount;//4;
uint16_t VDPCount;//800;
uint16_t VTotal;// = 1954;//1235;

uint16_t HsCount;//20;
uint16_t HBPCount;//15;
uint16_t HFPCount;//200;
uint16_t HDPCount;//1280;
uint16_t HTotal;// = 1294;//2080;

uint8_t  LaneNum;
uint8_t  SyncMode; // 0:pulse mode 1: event mode
uint32_t DSIFRE = 725000000;//1000000000;    //hz
uint8_t  BitNum;   //6bit 8bit 10bit

uint8_t InitCode_load_mode = 0; //
/*
0-全烧录
1-有条件的烧录
2-烧录完成后
*/
uint8_t InitCode[MAX_MIPI_CODE_LEN];
//uint8_t HsCode[30];

uint8_t HsSignalOnFlag;
//uint8_t SPINUM;
//uint8_t Lane8Flag;
//int g_hasVcomCheck;
//int  spifd;
//int  spifd2;

void ConfigSSD2828CMD(int channel);

uint8_t curflick;
uint8_t curpolar;

flashModuleData_info_t flashModuleData;

//xujie
#ifdef JUST_USED_MIPI_CHANNEL_1
extern int m_channel_1; 
#else
extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;
//xujie
#endif

void DCSShortWriteNoPara(int channel, uint8_t command)
{
    if(HsSignalOnFlag)
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x034b);
    }
    else
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x0340);   //0x0342-->0x0340
    }
    WriteSSD2828(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    WriteSSD2828(channel/*spifd*/,0XBC,0x0000);
    WriteCMDSSD2828(channel/*spifd*/,command);
    printf("write -->0x%x\n",command);
}

void DCSShortWriteOnePara(int channel, uint8_t command, uint8_t para)
{
    uint16_t mipi_data;
    mipi_data = para;
    mipi_data = (mipi_data<<8) + command;
    if(HsSignalOnFlag)
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x034b);
    }
    else
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x0340);   //0x0342-->0x0340
    }
    WriteSSD2828(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    WriteSSD2828(channel/*spifd*/,0XBC,0x0002);
    WriteSSD2828(channel/*spifd*/,0Xbf,mipi_data);
}

void GeneralShortWriteTwoPara(int channel, uint8_t command, uint8_t para)
{
    uint16_t mipi_data;
    mipi_data = para;
    mipi_data = (mipi_data<<8) + command;
    if(HsSignalOnFlag)
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x030b);
    }
    else
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x0300);//0x0302-->0x0300
    }
    WriteSSD2828(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    WriteSSD2828(channel/*spifd*/,0XBC,0x0002);
    WriteSSD2828(channel/*spifd*/,0Xbf,mipi_data);
}

void DCSLongWriteWithPara(const int channel, const uint8_t RegAddr, uint8_t *para, uint8_t count)
{
    uint16_t mipi_data,ii,tt;
    if(HsSignalOnFlag)
    {
    	//printf("hs mode: 1.\n");
        WriteSSD2828(channel,0XB7,0x034b);

		#if 0
		unsigned short read_val = 0;
		read_val = ReadSSD2828Reg(2, 1, 0XB7); //no use
		usleep(10000);
		printf("B7: write: 0x034b, read: %x.\n", read_val);
		#endif
    }
    else
    {
    	//printf("hs mode: 0.\n");
        WriteSSD2828(channel,0XB7,0x0340);   //0x0342-->0x0340

		#if 0
        unsigned short read_val = 0;
		usleep(10000);
		read_val = ReadSSD2828Reg(2, 1, 0XB7); //no use
		printf("B7: write: 0x0340, read: %x.\n", read_val);
		#endif
    }
	
    WriteSSD2828(channel,0XB8,0X0000);//VC Control
    mipi_data = count;
    WriteSSD2828(channel,0XBC,mipi_data); //后面准备需要传输的字节个数
    mipi_data = *para;
    mipi_data = (mipi_data<<8) + RegAddr;
    WriteSSD2828(channel,0Xbf,mipi_data); //寄存器
    //printf("mipi_data 39 is 0x%04x\n",mipi_data);
    for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) 
				*(para+tt) = 0;
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            WriteDATASSD2828(channel,mipi_data); //实际数据
            //printf("mipi_data is 0x%04x      ii=%d\n",mipi_data, ii);
        }
        else
        {
            break;
        }
    }
}

void GerneralLongWriteWithPara(int channel, uint8_t RegAddr, uint8_t *para, uint8_t count)
{
    uint16_t mipi_data,ii,tt;
    if(HsSignalOnFlag)
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x030b);
    }
    else
    {
        WriteSSD2828(channel/*spifd*/,0XB7,0x0300);//0x0302-->0x0300
    }

    WriteSSD2828(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    mipi_data = count;
    WriteSSD2828(channel/*spifd*/,0XBC,mipi_data);

    mipi_data = *para;
    mipi_data = (mipi_data<<8) + RegAddr;
    WriteSSD2828(channel/*spifd*/,0Xbf,mipi_data);

    for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) *(para+tt) = 0;
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            WriteDATASSD2828(channel/*spifd*/,mipi_data);
        }
        else
        {
            break;
        }
    }
}

void CMDDCSShortWriteNoPara(int channel, uint8_t command)
{
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB7,0x0340);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBC,0x0000);
    WriteCMDFPGATo2828_8bit(channel/*spifd*/,command);
}

void CMDDCSShortWriteOnePara(int channel, uint8_t command, uint8_t para)
{
    uint16_t mipi_data;
    mipi_data = para;
    mipi_data = (mipi_data<<8) + command;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB7,0x0340);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBC,0x0002);
    WriteFPGATo2828_8bit(channel/*spifd*/,0Xbf,mipi_data);
}

void CMDGeneralShortWriteTwoPara(int channel, uint8_t command, uint8_t para)
{
    uint16_t mipi_data;
    mipi_data = para;
    mipi_data = (mipi_data<<8) + command;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB7,0x0300);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBC,0x0002);
    WriteFPGATo2828_8bit(channel/*spifd*/,0Xbf,mipi_data);
}

void CMDDCSLongWriteWithPara(int channel, uint8_t RegAddr, uint8_t *para, uint8_t count)
{
    uint16_t mipi_data,ii,tt;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB7,0x0340);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB8,0X0000);//VC Control
    mipi_data = count;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBC,mipi_data);
    mipi_data = *para;
    mipi_data = (mipi_data<<8) + RegAddr;
    WriteFPGATo2828_8bit(channel/*spifd*/,0Xbf,mipi_data);
    for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) *(para+tt) = 0;
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            WriteDATAFPGATo2828_8bit(channel/*spifd*/,mipi_data);
        }
        else
        {
            break;
        }
    }
}

void CMDGerneralLongWriteWithPara(int channel, uint8_t RegAddr, uint8_t *para, uint8_t count)
{
    uint16_t mipi_data,ii,tt;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB7,0x0300);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    mipi_data = count;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBC,mipi_data);
    mipi_data = *para;
    mipi_data = (mipi_data<<8) + RegAddr;
    WriteFPGATo2828_8bit(channel/*spifd*/,0Xbf,mipi_data);
    for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) *(para+tt) = 0;
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            WriteDATAFPGATo2828_8bit(channel/*spifd*/,mipi_data);
        }
        else
        {
            break;
        }
    }
}

static int _ReadModReg(uint8_t channel, uint8_t type, uint8_t RegAddr, uint8_t len, uint8_t *pRead)
{
    uint16_t mipi_data, Temp, i, ReadValue;
    int count = 0;
	int RetLen = 0;
	
    mipi_data = len;
	
    while(count < 3)
    {
		WriteSSD2828(channel, 0XC1, len);		
        if(type == 0)   //0:Gerneric
        {
            WriteSSD2828(channel, 0XB7, 0x0380);
        }
        else if(type == 1)   //1:DCS
        {
            WriteSSD2828(channel, 0XB7, 0x03c0);
        }
		
        WriteSSD2828(channel, 0XC0, 0x0100);	// reset
        WriteSSD2828(channel, 0XBC, 0x0001);

        mipi_data = RegAddr;
        WriteSSD2828(channel,0Xbf,mipi_data);	// write packet data

       // _time_delay(100); //xujie
   		usleep(10 * 1000);
		
        Temp = ReadSSD2828Reg(channel, 0XC6);	

		#ifdef MIPI_READ_DEBUG
        printf("read sdo[0xC6] 0x%x to sure ready??? with channel %d\n", Temp, channel);
		#endif
		
        if((Temp & 0x0005) == 0x0005)
        {
            Temp = ReadSSD2828Reg(channel, 0XC2);
			
			#ifdef MIPI_READ_DEBUG
            printf("read reg 0XC2, len 0x%x at channel %d with type %d\n", Temp,channel,type);
			#endif
			
			RetLen = Temp & 0xffff; //xujie 返回需要读取的长度, 是否与设置的长度一至
            for(i=0; i<len; i+=2)
            {
                if(i==0)
                {
                    ReadValue = ReadSSD2828Reg(channel, 0XFF);
                }
                else
                {
                    ReadValue = ReadDATASSD2828Reg(channel, 0XFF);
                }
				
                pRead[i]   =  ReadValue&0xFF;
                if(i+1<len)
                {
                    pRead[i+1] = (ReadValue&0xFF00)>>8;
                }

				#ifdef MIPI_READ_DEBUG
                printf("[0x%x]-[0x%x]-",pRead[i],pRead[i+1]);
				#endif
            }

			#ifdef MIPI_READ_DEBUG
            printf("\n");
			#endif
			
            break;
        }
        else
        {
            count++;
        }
    }
	
    if(HsSignalOnFlag == 1)
    {
        WriteSSD2828(channel, 0XB7,0x03c9);
    }

	if (RetLen <= 0)
	{
		printf("****** ReadModReg error: RetLen = %d. count = %d. ******\n", RetLen, count);
	}
	
	
	return RetLen;
}

int ReadModReg(uint8_t channel, uint8_t type, uint8_t RegAddr, uint8_t len, uint8_t *pRead)
{
	int read_len = _ReadModReg(channel, type, RegAddr, len, pRead);
	//if (read_len <= 0)
	//	read_len = _ReadModReg(channel, type, RegAddr, len, pRead);

	return read_len;
}

//int ONLY_USE_11_29 = 0;
int no_use_initcode = 0;
void Set2828_V(uint8_t ch) //上电，用了这个
{
    GetConfigPara(); //只是配置变量
	
	ConfigSSD2828(ch); //里面全部是用WriteSSD2828
	_time_delay(1);
	
	SendCode(ch,InitCode);
	_time_delay(10);
	
	WriteSSD2828(ch, 0XB7, 0X030b); //激活输出, 点屏
	
    HsSignalOnFlag = 1;
}

#if 0
void Set2828_CMD(uint8_t ch)
{
/*    if(Lane8Flag == 1)
    {
        if(ch == 1)
        {
            SPINUM = 0x03;
        }
        if(ch == 2)
        {
            SPINUM = 0x0C;
        }
        if(ch == 3)
        {
            SPINUM = 0x0F;
        }
    }
    if(Lane8Flag == 0)
    {
        if(ch == 1)
        {
            SPINUM = 0x02;
        }
        if(ch == 2)
        {

            SPINUM = 0x08;
        }
        if(ch == 3)
        {

            SPINUM = 0x0A;
        }
    }*/
    GetConfigPara();

#ifdef JUST_USED_MIPI_CHANNEL_1	
	if(m_channel_1 > 0)
	{
		ConfigSSD2828CMD(m_channel_1/*1*/);
		//if(m_channel_2 > 0) ConfigSSD2828CMD(m_channel_2/*2*/); //xujie
		_time_delay(10);
		SendCodeCMD(m_channel_1/*1*/,InitCode);
		//if(m_channel_2 > 0) SendCodeCMD(m_channel_2/*2*/,InitCode); //xujie
		_time_delay(10);
		if(HsSignalOnFlag == 0)
		{
			WriteSSD2828(m_channel_1/*1*//*spifd*/,0XB7,0X030b);
			//if(m_channel_2 > 0) WriteSSD2828(m_channel_2/*2*/,0XB7,0X030b); //xujie
			//HsSignalOnFlag = 1;
		}
	}
	#else
	if(m_channel_1 > 0)
	{
		ConfigSSD2828CMD(m_channel_1/*1*/);
		//if(m_channel_2 > 0) ConfigSSD2828CMD(m_channel_2/*2*/); //xujie
		_time_delay(10);
		SendCodeCMD(m_channel_1/*1*/,InitCode);
		//if(m_channel_2 > 0) SendCodeCMD(m_channel_2/*2*/,InitCode); //xujie
		_time_delay(10);
		if(HsSignalOnFlag == 0)
		{
			WriteSSD2828(m_channel_1/*1*//*spifd*/,0XB7,0X030b);
			//if(m_channel_2 > 0) WriteSSD2828(m_channel_2/*2*/,0XB7,0X030b); //xujie
			//HsSignalOnFlag = 1;
		}
	}
	
	if(m_channel_2 > 0)
	{
		//ConfigSSD2828CMD(m_channel_1/*1*/);
		ConfigSSD2828CMD(m_channel_2/*2*/); //xujie
		_time_delay(10);
		//SendCodeCMD(m_channel_1/*1*/,InitCode);
		SendCodeCMD(m_channel_2/*2*/,InitCode); //xujie
		_time_delay(10);
		if(HsSignalOnFlag == 0)
		{
			//WriteSSD2828(m_channel_1/*1*//*spifd*/,0XB7,0X030b);
			WriteSSD2828(m_channel_2/*2*/,0XB7,0X030b); //xujie
			//HsSignalOnFlag = 1;
		}
	}

	if(m_channel_3 > 0)
	{
		//ConfigSSD2828CMD(m_channel_1/*1*/);
		ConfigSSD2828CMD(m_channel_3/*2*/); //xujie
		_time_delay(10);
		//SendCodeCMD(m_channel_1/*1*/,InitCode);
		SendCodeCMD(m_channel_3/*2*/,InitCode); //xujie
		_time_delay(10);
		if(HsSignalOnFlag == 0)
		{
			//WriteSSD2828(m_channel_1/*1*//*spifd*/,0XB7,0X030b);
			WriteSSD2828(m_channel_3/*2*/,0XB7,0X030b); //xujie
			//HsSignalOnFlag = 1;
		}
	}
	
	if(m_channel_4 > 0)
	{
		//ConfigSSD2828CMD(m_channel_1/*1*/);
		ConfigSSD2828CMD(m_channel_4/*2*/); //xujie
		_time_delay(10);
		//SendCodeCMD(m_channel_1/*1*/,InitCode);
		SendCodeCMD(m_channel_4/*2*/,InitCode); //xujie
		_time_delay(10);
		if(HsSignalOnFlag == 0)
		{
			//WriteSSD2828(m_channel_1/*1*//*spifd*/,0XB7,0X030b);
			WriteSSD2828(m_channel_4/*2*/,0XB7,0X030b); //xujie
			//HsSignalOnFlag = 1;
		}
	}
	#endif
	
    //ConfigSSD2828CMD(m_channel_1/*1*/);
	//ConfigSSD2828CMD(m_channel_2/*2*/); //xujie
    //_time_delay(10);
    //SendCodeCMD(m_channel_1/*1*/,InitCode);
	//SendCodeCMD(m_channel_2/*2*/,InitCode); //xujie
    //_time_delay(10);
    if(HsSignalOnFlag == 0)
    {
        //WriteSSD2828(m_channel_1/*1*//*spifd*/,0XB7,0X030b);
		//WriteSSD2828(m_channel_2/*2*/,0XB7,0X030b); //xujie
        HsSignalOnFlag = 1;
    }
    //_time_delay(15);
}
#endif

void GetConfigPara(void)
{
    uint32_t i,dsifre,ByteClk,ByteClkMhz,LPClk,LPClkKHz;
    uint8_t reg_balow,syncmode;
    // unsigned char send_buffer[100];
    uint8_t HsPrepareDelay,HsZeroDelay,ClkZeroDelay,ClkPrepareDelay,ClkPreDelay,ClkPostDelay,ClkTrailDelay,HsTrailDelay,TAGoDelay,TAGetDelay;
    uint16_t WakeUpDelay;


    dsifre = DSIFRE;
    //dsifre = 800000000;//DSIFRE;
    ByteClk = dsifre / 8;
    ByteClkMhz = ByteClk / 1000000;

    flashModuleData.module.vsyncpulseWidth = VsCount;
    flashModuleData.module.hsyncpulseWidth = HsCount;
    flashModuleData.module.vBackPorch = VBPCount;
    flashModuleData.module.hBackPorch = HBPCount;
    flashModuleData.module.vFrontPorch = VFPCount;
    flashModuleData.module.hFrontPorch = HFPCount;
    flashModuleData.module.vActiveTime = VDPCount;
    flashModuleData.module.hActiveTime = HDPCount;

    flashModuleData.module.lane = LaneNum;
    flashModuleData.module.bit = BitNum;
    syncmode = SyncMode;


    SSD2828.reg_ba = 0;
    SSD2828.reg_b1 = (uint8_t)flashModuleData.module.vsyncpulseWidth;
    SSD2828.reg_b1 = (uint8_t)flashModuleData.module.hsyncpulseWidth + (SSD2828.reg_b1<<8);
    if(syncmode == 0 )  //pulse
    {
        SSD2828.reg_b2 = flashModuleData.module.vBackPorch;
        SSD2828.reg_b2 = (uint8_t)(flashModuleData.module.hBackPorch) + (SSD2828.reg_b2<<8);
    }
    if((syncmode == 1) || (syncmode == 2))  //burst or event
    {
        SSD2828.reg_b2 = flashModuleData.module.vBackPorch+flashModuleData.module.vsyncpulseWidth;
        SSD2828.reg_b2 = (uint8_t)(flashModuleData.module.hBackPorch+flashModuleData.module.hsyncpulseWidth) + (SSD2828.reg_b2<<8);
    }
    SSD2828.reg_b3 = (uint8_t)flashModuleData.module.vFrontPorch;
    SSD2828.reg_b3 = (uint8_t)flashModuleData.module.hFrontPorch + (SSD2828.reg_b3<<8);

    SSD2828.reg_b4 = flashModuleData.module.hActiveTime;
    SSD2828.reg_b5 = flashModuleData.module.vActiveTime;
#if 1  //25M
      if((dsifre >= 62500000) && (dsifre < 125000000))   //25M   4分频
        {SSD2828.reg_ba = 0x0400;}
        if((dsifre >= 125000000) && (dsifre < 250000000))
        {SSD2828.reg_ba = 0x4400;}
        if((dsifre >= 250000000) && (dsifre < 500000000))
        {SSD2828.reg_ba = 0x8400;}
        if((dsifre >= 500000000) && (dsifre <=1000000000))
        {SSD2828.reg_ba = 0xc400;}
        reg_balow = dsifre/6250000;  // 25M/4 = 6.25M
        SSD2828.reg_ba = SSD2828.reg_ba + reg_balow;
        SSD2828.reg_bb = (dsifre/80000000) - 1;  //lp
#elif 0
    //   SSD2828.reg_bb = dsifre/80000000;

    //   if((dsifre >= 62500000) && (dsifre < 125000000))   //20M  2分频
    //     {SSD2828.reg_ba = 0x0200;}
    //  if((dsifre >= 125000000) && (dsifre < 250000000))
    //  {SSD2828.reg_ba = 0x4200;}
    //  if((dsifre >= 250000000) && (dsifre < 500000000))
    //  {SSD2828.reg_ba = 0x8200;}
    //  if((dsifre >= 500000000) && (dsifre <=1000000000))
    //  {SSD2828.reg_ba = 0xc200;}
    //  reg_balow = dsifre/10000000;  // 20M/2
    //  SSD2828.reg_ba = SSD2828.reg_ba + reg_balow;
    // SSD2828.reg_bb = (dsifre/80000000) - 1;  //lp
#else
    if((dsifre >= 62500000) && (dsifre < 125000000))   //10M
    {SSD2828.reg_ba = 0x0100;}
    if((dsifre >= 125000000) && (dsifre < 250000000))
    {SSD2828.reg_ba = 0x4100;}
    if((dsifre >= 250000000) && (dsifre < 500000000))
    {SSD2828.reg_ba = 0x8100;}
    if((dsifre >= 500000000) && (dsifre <=1000000000))
    {SSD2828.reg_ba = 0xc100;}
    reg_balow = dsifre/10000000;  // 10M
    SSD2828.reg_ba = SSD2828.reg_ba + reg_balow;
    SSD2828.reg_bb = (dsifre/80000000) - 1;  //lp
#endif
    LPClk = ByteClk/(SSD2828.reg_bb+1);
    LPClkKHz = LPClk/1000;
    for(i = 0; i < 256; i ++)
    {
        if(i*500/ByteClkMhz > (48 + 600/ByteClkMhz))  //48+600
        {
            if(i >= 4)
            {
                HsPrepareDelay = i -4;
            }else
            {
                HsPrepareDelay = 0;
            }
            break;
        }
    }

    for(i = 0; i < 256; i ++)
    {
        if(i*500/ByteClkMhz > (150 + 1500/ByteClkMhz))  //  150+1500
        {
            HsZeroDelay = i;
            break;
        }
    }
    SSD2828.reg_c9 = (HsZeroDelay << 8) + HsPrepareDelay;
    /////////////////////////////////////////////////////////////C9
    for(i = 0; i < 256; i ++)
    {
        if(i*500/ByteClkMhz > 44) //44
        {
            if(i >= 3)
            {
                ClkPrepareDelay = i - 3;
                // ClkPrepareDelay = i+1;
            }else
            {
                ClkPrepareDelay = 0;
                //  ClkPrepareDelay = i+1;
            }
            break;
        }
    }

    for(i = 0; i < 256; i ++)
    {
        if(i*500/ByteClkMhz > 280)  //280  300
        {
            ClkZeroDelay = i;
            //ClkPrepareDelay = i+1;
            break;
        }
    }
    SSD2828.reg_ca = (ClkZeroDelay << 8) + ClkPrepareDelay;
    ///////////////////////////////////////////////////////////CA
    for(i = 0; i < 256; i ++)
    {
        if(i*500/ByteClkMhz > (65 + 7000/ByteClkMhz))  //65+7000
        {
            ClkPostDelay = i;
            break;
        }
    }

    for(i = 0; i < 256; i ++)
    {
        if(i*500/ByteClkMhz > (69 + 650/ByteClkMhz))     //调试出来的75+650  69+650ok(origin)
        {
            ClkPreDelay = i;
            break;
        }
    }
    SSD2828.reg_cb = (ClkPreDelay << 8) + ClkPostDelay;
    /////////////////////////////////////////////////////////////CB
    for(i = 0; i < 256; i ++)
    {
        if(i * 500 / ByteClkMhz > (100 + 1250 / ByteClkMhz)) //100ns + 10UI   下限60+4UI 上限 105+12UI
        //if(i*500/ByteClkMhz > (65 + 550/ByteClkMhz)) zhaobaoren
        {
            HsTrailDelay = i;
            break;
        }
    }

    ClkTrailDelay = HsTrailDelay;
    SSD2828.reg_cc = (ClkTrailDelay << 8) + HsTrailDelay;
    ////////////////////////////////////////////////////////////////CC

    for(i = 0; i < 65536; i ++)
    {
        if(i*1000/LPClkKHz > 1050)   //i*1000000/LPClkKHz > 1050000  大于1.05毫秒
        {
            WakeUpDelay = i;
            break;
        }
    }
    SSD2828.reg_cd = WakeUpDelay;
    
    ////////////////////////////////////////////////////////////CD
    for(i = 0; i < 16; i ++)
    {
        if(i*500000/LPClkKHz > 275)   //275
            //   if(i*500/ByteClkMhz > 62)//
        {
            TAGetDelay = i;
            break;
        }
    }

    for(i = 0; i < 16; i ++)
    {
        if(i*500000/LPClkKHz > 220)  //
            // if(i*500/ByteClkMhz > 50)
        {
            TAGoDelay = i;
            break;
        }
    }
    SSD2828.reg_ce = (TAGoDelay << 8) + TAGetDelay;
    //////////////////////////////////////////////////////////////////CE
    
    // module_link = flashModuleData.module.lane;
    // module_bit = flashModuleData.module.bit;
    if(syncmode == 0)  //pulse
    {
        if(flashModuleData.module.bit == 6)SSD2828.reg_b6 =0x0002;  //0x06 event 0x02 pulse 0x0A burst
        if(flashModuleData.module.bit == 8)SSD2828.reg_b6 =0x0003;  //0x07 event 0x03 pulse 0x0B burst
    }
    else if(syncmode == 1)  //event
    {
        if(flashModuleData.module.bit == 6)SSD2828.reg_b6 =0x0006;  //0x06 event 0x02 pulse 0x0A burst
        if(flashModuleData.module.bit == 8)SSD2828.reg_b6 =0x0007;  //0x07 event 0x03 pulse 0x0B burst
    }
    else if(syncmode == 2)  //burst
    {
        if(flashModuleData.module.bit == 6)SSD2828.reg_b6 =0x000a;  //0x06 event 0x02 pulse 0x0A burst
        if(flashModuleData.module.bit == 8)SSD2828.reg_b6 =0x000b;  //0x07 event 0x03 pulse 0x0B burst
    }

    SSD2828.reg_de = LaneNum-1;
    SSD2828.reg_d6 = 4;//5;  //05BGR  04RGB

}

void ConfigSSD2828(int channel)
{
    //SSD2828->reg_b1;
    WriteSSD2828(channel/*spifd*/,0XB1,SSD2828.reg_b1);//Vertical Sync Active Period    Horizontal Sync Active Period
    WriteSSD2828(channel/*spifd*/,0XB2,SSD2828.reg_b2);//VBP   HBP
    WriteSSD2828(channel/*spifd*/,0XB3,SSD2828.reg_b3);//VFP   HFP
    WriteSSD2828(channel/*spifd*/,0XB4,SSD2828.reg_b4);//x 540
    WriteSSD2828(channel/*spifd*/,0XB5,SSD2828.reg_b5);//y 960
    WriteSSD2828(channel/*spifd*/,0XB6,SSD2828.reg_b6);
    // _time_delay(5);
    WriteSSD2828(channel/*spifd*/,0XDE,SSD2828.reg_de);//4lane
    WriteSSD2828(channel/*spifd*/,0XD6,SSD2828.reg_d6);//Packet Number in Blanking Period  BGR   05BGR  04RGB
    WriteSSD2828(channel/*spifd*/,0XB9,0X0000);//pll powerdown
    WriteSSD2828(channel/*spifd*/,0XC4,0X0000);//Automatically perform BTA after the next write packe
    //_time_delay(5);

    /*printf("SSD2828.reg_c9:%d\n",SSD2828.reg_c9);
    printf("SSD2828.reg_ca:%d\n",SSD2828.reg_ca);
    printf("SSD2828.reg_cb:%d\n",SSD2828.reg_cb);
    printf("SSD2828.reg_cc:%d\n",SSD2828.reg_cc);
    printf("SSD2828.reg_cd:%d\n",SSD2828.reg_cd);
    printf("SSD2828.reg_ce:%d\n",SSD2828.reg_ce);*/

    WriteSSD2828(channel/*spifd*/,0XC9,SSD2828.reg_c9);   //htc 4.7(pa68)
    WriteSSD2828(channel/*spifd*/,0XCA,SSD2828.reg_ca);
    WriteSSD2828(channel/*spifd*/,0XCB,SSD2828.reg_cb);
    WriteSSD2828(channel/*spifd*/,0XCC,SSD2828.reg_cc);
    WriteSSD2828(channel/*spifd*/,0XCD,SSD2828.reg_cd);
    WriteSSD2828(channel/*spifd*/,0XCE,SSD2828.reg_ce);
    //_time_delay(5);
    WriteSSD2828(channel/*spifd*/,0XBA,SSD2828.reg_ba);//pll config     /5   *70
    WriteSSD2828(channel/*spifd*/,0XBB,SSD2828.reg_bb);//LP Clock Control Register
    WriteSSD2828(channel/*spifd*/,0XB9,0X0001);//PLL Control Register
    WriteSSD2828(channel/*spifd*/,0XB8,0X0000);//VC Control Register
    /*int d9v = ReadSSD2828Reg(2,spifd,0XD9); //no use
    printf("bef SSD2828.reg_d9:%d\n",d9v);
    d9v = d9v|0x0600;
    WriteSSD2828(spifd,0XD9,d9v); //no use
    d9v = ReadSSD2828Reg(2,spifd,0XD9); //no use
    printf("aft SSD2828.reg_d9:%d\n",d9v);
    int e1v = ReadSSD2828Reg(2,spifd,0XE1); //no use
    printf("bef SSD2828.reg_e1:%d\n",e1v);
    e1v = 0xAAAA;
    WriteSSD2828(spifd,0XE1,e1v); //no use
    e1v = ReadSSD2828Reg(2,spifd,0XE1); //no use
    printf("aft SSD2828.reg_e1:%d\n",e1v);*/
}

void ConfigSSD2828CMD(int channel)
{
    //SSD2828->reg_b1;
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB1,SSD2828.reg_b1);//Vertical Sync Active Period    Horizontal Sync Active Period
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB2,SSD2828.reg_b2);//VBP   HBP
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB3,SSD2828.reg_b3);//VFP   HFP
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB4,SSD2828.reg_b4);//x 540
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB5,SSD2828.reg_b5);//y 960
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB6,SSD2828.reg_b6);
    // _time_delay(5);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XDE,SSD2828.reg_de);//4lane
    WriteFPGATo2828_8bit(channel/*spifd*/,0XD6,SSD2828.reg_d6);//Packet Number in Blanking Period  BGR   05BGR  04RGB
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB9,0X0000);//pll powerdown
    WriteFPGATo2828_8bit(channel/*spifd*/,0XC4,0X0000);//Automatically perform BTA after the next write packe
    //_time_delay(5);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XC9,SSD2828.reg_c9);   //htc 4.7(pa68)
    WriteFPGATo2828_8bit(channel/*spifd*/,0XCA,SSD2828.reg_ca);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XCB,SSD2828.reg_cb);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XCC,SSD2828.reg_cc);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XCD,SSD2828.reg_cd);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XCE,SSD2828.reg_ce);
    //_time_delay(5);
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBA,SSD2828.reg_ba);//pll config     /5   *70
    WriteFPGATo2828_8bit(channel/*spifd*/,0XBB,SSD2828.reg_bb);//LP Clock Control Register
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB9,0X0001);//PLL Control Register
    WriteFPGATo2828_8bit(channel/*spifd*/,0XB8,0X0000);//VC Control Register
}

void mipi_to_hs_mode(int mipi_channel)
{
	WriteSSD2828(mipi_channel, 0XB7, 0X030b);
   	HsSignalOnFlag = 1;
}

void mipi_to_lp_mode(int mipi_channel)
{
	WriteSSD2828(mipi_channel, 0XB7, 0X030a);
    HsSignalOnFlag = 0;
}


void SendCode(int channel, /*uint8_t*/unsigned char *InitCode)
{
    uint16_t i,delaytime;
    uint8_t j;
    uint8_t MipiData[250];
    uint16_t CodePoint;
    //SPINUM = ch;
    CodePoint = 0;
	
    for(i = 0; i < MAX_MIPI_CODE_LEN; i++)
    {
        if(InitCode[CodePoint + 2] <= 250)
        {
            for(j = 0; j < InitCode[CodePoint + 2]; j++)
            {
                MipiData[j] = InitCode[CodePoint + 3 + j];
                //printf("[0x%02x]",MipiData[j]);
            }
            //printf("\n");

			#if 0
			printf("cmd: len = %d. data: ", j);
			int aa = 0;
			for (; aa < j; aa ++)
			{
				printf("%02x ", MipiData[aa]);
			}
			printf("\n");
			#endif
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0x0))  //type 0x05 DCS Short WRITE, no parameters
            {
                delaytime =  MipiData[0] + (MipiData[1] << 8);
                printf("delay %d\n",delaytime); //xujie
                _time_delay(delaytime);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xfe)) //channel
            {
               // Channel = MipiData[0]; //xujie 注释, 原来是用的, 
               // printf("mipi read channel is %d\n",Channel);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xfd)) //read reg
            {
                unsigned char  regAddr = MipiData[0];
                unsigned short regVal;
                regVal = ReadSSD2828Reg(channel/*Channel*/,/*spifd,*/regAddr);
                printf("mipi read reg 0x%x value: 0x%x\n",regAddr,regVal);
                InitCode[CodePoint + 4] = (regVal&0xff00)>>8;
                InitCode[CodePoint + 5] =  regVal&0xff;
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xfc)) //write reg
            {
                unsigned char  regAddr  = MipiData[0];
                unsigned short regVal  = (MipiData[1]<<8)|MipiData[2];
                WriteSSD2828(channel/*spifd*/,regAddr,regVal);
                printf("mipi write reg 0x%x value: 0x%x\n",regAddr,regVal);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xfb))//hs mode on
            {
                //b7 -->0x030b
                printf("hs mode on\n");
                WriteSSD2828(channel/*spifd*/,0XB7,0X030b);
                HsSignalOnFlag = 1;
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xfa))//hs mode off
            {
                //b7-->0x030a
                printf("hs mode off\n");
                WriteSSD2828(channel/*spifd*/,0XB7,0X030a);
                HsSignalOnFlag = 0;
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xf9))//mtp power on
            {
                printf("mtp power on\n");
               // mtp_power_on(m_channel_1);
				//if(m_channel_2 > 0) mtp_power_on(m_channel_2);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xf8))//mtp power off
            {
                printf("mtp power off\n");
                //mtp_power_off(m_channel_1);
				//if(m_channel_2 > 0) mtp_power_off(m_channel_2);
            }
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xf7))//reset
            {
                printf("reset the 2828\n");
                mipi_gpio_reset(0);
                mipi_gpio_reset(1);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xf6))//RGB
            {
            	printf("show rgb.\n");
                showRgbStr   showRgb;
                showRgb.rgb_r = InitCode[CodePoint + 3];
                showRgb.rgb_g = InitCode[CodePoint + 4];
                showRgb.rgb_b = InitCode[CodePoint + 5];
                unsigned int u32QueueId = fpga_message_queue_get();
                MsOS_SendMessage(u32QueueId,FPGA_CMD_SHOW_RGB,&showRgb,0);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xe0))//vcom_ili9881
            {
            	printf("ilc8891_vcom_read.\n");
                //ilc8891_vcom_read(channel/*Channel*/);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xe1))//otp_ili9881
            {
            	printf("ilc8891_otp_read.\n");
                //ilc8891_otp_read(channel/*Channel*/);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0xe2))//otp_ili9881
            {
            	printf("ilc8891_otp_check.\n");
                //ilc8891_otp_check(channel/*Channel*/);
            }
			
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0x3))  //type 0x05 DCS Short WRITE, no parameters
            {
            	printf("HsSignalOnFlag.\n");
                WriteSSD2828(channel/*spifd*/,0XB7,0X030b);
                HsSignalOnFlag = 1;
            }
			
            if(InitCode[CodePoint + 0] == 0x05)   //type 0x05 DCS Short WRITE, no parameters
            {
                printf("send 0x05\n");
                DCSShortWriteNoPara(channel, InitCode[CodePoint+1]);
                Delay_I2C(20);
            }
			
            if(InitCode[CodePoint + 0] == 0x06)   //dcs read
            {
                char flag  = InitCode[CodePoint+4];
                ReadModReg(channel/*Channel*/, 1, InitCode[CodePoint+1], InitCode[CodePoint+3],&InitCode[CodePoint+4]);
                printf("send 0x06 flag %x\n",flag);
                if(flag == 0xaa)
                {
                    curflick = InitCode[CodePoint+4];
                }
                else if(flag == 0xab)
                {
                    curpolar = InitCode[CodePoint+4];
                }
            }
			
            if(InitCode[CodePoint + 0] == 0x14)   //gernal read
            {
                printf("send 0x14\n");
                //printf("InitCode[CodePoint + 0] [0x%x] codepoint %d\n",InitCode[CodePoint + 0],CodePoint);
                ReadModReg(channel/*Channel*/, 0, InitCode[CodePoint+1], InitCode[CodePoint+3],&InitCode[CodePoint+4]);
            }
			
            if(InitCode[CodePoint + 0] == 0x32)   //type 0x05 DCS Short WRITE, no parameters
            {
				printf("cmd 0x32: do nothing.\n");
            }
			
            if(InitCode[CodePoint + 0] == 0x15)   //type 0x15 DCS Short WRITE, 1 parameter
            {
                printf("send 0x15\n");
                DCSShortWriteOnePara(channel, InitCode[CodePoint+1], InitCode[CodePoint+3]);
                Delay_I2C(20);
            }
			
            if(InitCode[CodePoint + 0] == 0x23)   //type 0x23 Generic Short WRITE, 2 parameters
            {
                printf("send 0x23\n");
                GeneralShortWriteTwoPara(channel, InitCode[CodePoint+1], InitCode[CodePoint+3]);
                Delay_I2C(20);
            }
			
            if(InitCode[CodePoint + 0] == 0x29)   //type 0x29 Generic Long Write (Max 8 byte for register access)
            {
                printf("send 0x29\n");
                GerneralLongWriteWithPara(channel, InitCode[CodePoint+1],MipiData,InitCode[CodePoint + 2]+1);
                Delay_I2C(20);
            }
			
            if(InitCode[CodePoint + 0] == 0x39)   //type 0x39 DCS Long Write (Max 8 byte for register access)
            {
                //printf("send 0x39\n"); //xujie
				#if 0 //zdx
				printf("reg:  %02x, data len: 0x%02x. \n", InitCode[CodePoint+1], InitCode[CodePoint + 2]);

				int n = 0;
				for (; n < InitCode[CodePoint + 2]+1; n ++)
				{
					printf("%02x ", MipiData[n]);
				}
				printf("\n");
				#endif
				/*if(InitCode[CodePoint + 2] == 0)
				{
					DCSShortWriteNoPara(channel, InitCode[CodePoint+1]);
				}
                else*/
            	{
            		DCSLongWriteWithPara(channel/*Channel*/, InitCode[CodePoint+1], MipiData, InitCode[CodePoint + 2]+1);
            	}
               // Delay_I2C(20); //xujie
            }
			
            CodePoint += InitCode[CodePoint + 2] + 3;
        }
        else    //碰到0xff 跳出
        {
            break;
        }
    }
	
    if(curflick || curpolar)
    {
        client_rebackFlick(curflick,curpolar);
        printf("reback the curflick and curpolar %x\n",curflick);
    }
}


void SendCodeCMD(int channel, uint8_t *InitCode)
{
    uint16_t i,delaytime;
    uint8_t j;
    uint8_t MipiData[250];
    uint16_t CodePoint;
    CodePoint = 0;
    for(i = 0; i < MAX_MIPI_CODE_LEN; i++)
    {
        if(InitCode[CodePoint + 2] <= 250)
        {
            for(j = 0; j < InitCode[CodePoint + 2]; j++)
            {
                MipiData[j] = InitCode[CodePoint + 3 + j];
            }
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0x0))  //type 0x05 DCS Short WRITE, no parameters
            {
                delaytime =  MipiData[0] + (MipiData[1] << 8);
                _time_delay(delaytime);
            }
            if((InitCode[CodePoint + 0] == 0x0) && (InitCode[CodePoint + 1] == 0x3))  //type 0x05 DCS Short WRITE, no parameters
            {
                //WriteFPGATo2828_8bit(channel,0XB7,0X030b);
                //HsSignalOnFlag = 1;
            }
            if(InitCode[CodePoint + 0] == 0x05)   //type 0x05 DCS Short WRITE, no parameters
            {
                CMDDCSShortWriteNoPara(channel, InitCode[CodePoint+1]);
                Delay_I2C(30);
            }
			
            if(InitCode[CodePoint + 0] == 0x32)   //type 0x05 DCS Short WRITE, no parameters
            {

            }
            if(InitCode[CodePoint + 0] == 0x15)   //type 0x15 DCS Short WRITE, 1 parameter
            {
                CMDDCSShortWriteOnePara(channel, InitCode[CodePoint+1], InitCode[CodePoint+3]);
                Delay_I2C(30);
            }
            if(InitCode[CodePoint + 0] == 0x23)   //type 0x23 Generic Short WRITE, 2 parameters
            {
                CMDGeneralShortWriteTwoPara(channel, InitCode[CodePoint+1], InitCode[CodePoint+3]);
                Delay_I2C(30);
            }
            if(InitCode[CodePoint + 0] == 0x29)   //type 0x29 Generic Long Write (Max 8 byte for register access)
            {
                CMDGerneralLongWriteWithPara(channel, InitCode[CodePoint+1],MipiData,InitCode[CodePoint + 2]+1);
                Delay_I2C(30);
            }
            if(InitCode[CodePoint + 0] == 0x39)   //type 0x39 DCS Long Write (Max 8 byte for register access)
            {
                if(HsSignalOnFlag == 0)
                {
                    CMDDCSLongWriteWithPara(channel, InitCode[CodePoint+1],MipiData,InitCode[CodePoint + 2]+1);
                    Delay_I2C(30);
                }
            }
            CodePoint += InitCode[CodePoint + 2] + 3;
        }
        else    //碰到0xff 跳出
        {
            break;
        }
    }
}

void sendCloseCommand()
{
	DCSShortWriteNoPara(SPI_ALL_CS, 0x28);

	Delay_I2C(20);
	DCSShortWriteNoPara(SPI_ALL_CS, 0x10);

	Delay_I2C(20);
	WriteSSD2828(SPI_ALL_CS, 0XB7,0x0340);
	WriteSSD2828(SPI_ALL_CS, 0XC4,0x0004);
	
    printf("close the mipi\n");
}


void setHsmode(int enable)
{
    if(enable)
    {
        HsSignalOnFlag = 1;
    }
    else
    {
        HsSignalOnFlag = 0;
    }
}

#define _INTSIZEOF(n) ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v) ( ap = (/*va_list*/char *)&v + _INTSIZEOF(v) )
#define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap) ( ap = (/*va_list*/char *)0 )

void Generic_Write(int Nnumber, ...)
{
	char * ap;
	va_start(ap, Nnumber);
	unsigned char i;
	unsigned char cmd[256];
	for(i=0; i < Nnumber; ++i)
	{
		cmd[i] = va_arg(ap, int);
	}
	va_end(ap);

	printf("len = %d     ", Nnumber);
	for(i=0; i < Nnumber; i++)
		printf("%02X ", cmd[i]);
	printf("\n");
}

void DCSLongWriteWithPara_xujie(int channel,/*uint8_t RegAddr, */unsigned char *para, int count)
{
	unsigned short vhigh, vlow;
	unsigned short mipi_data;
	if(HsSignalOnFlag)
	{
		//printf("hs mode: 1.\n");
		WriteSSD2828(channel,0XB7,0x034b);

#if 0
		unsigned short read_val = 0;
		read_val = ReadSSD2828Reg(2, 1, 0XB7); //no use
		usleep(10000);
		printf("B7: write: 0x034b, read: %x.\n", read_val);
#endif
	}
	else
	{
		//printf("hs mode: 0.\n");
		WriteSSD2828(channel,0XB7,0x0340);   //0x0342-->0x0340

#if 0
		unsigned short read_val = 0;
		usleep(10000);
		read_val = ReadSSD2828Reg(2, 1, 0XB7); //no use
		printf("B7: write: 0x0340, read: %x.\n", read_val);
#endif
	}

	WriteSSD2828(channel,0XB8,0X0000);//VC Control

	//
	mipi_data = count;
	WriteSSD2828(channel,0XBC,mipi_data); //后面准备需要传输的字节个数

	//
	vhigh = para[1] & 0xff;
	vlow = para[0] & 0xff;
//	mipi_data = *para;
	mipi_data = (vhigh/*mipi_data*/<<8) | vlow/*+*/ /*RegAddr*/;
	WriteSSD2828(channel,0Xbf,mipi_data); //寄存器
	if(count == 2) return;
	count -= 2;
	para += 2;
	//printf("mipi_data 39 is 0x%04x\n",mipi_data);

	//
	//if(count > 0)
//	{
		//int i;//unsigned short mipi_data,ii,tt;
	while(count > 0)//for(i = 0; i < count; i++)
	{
		if(count == 1)
		{
			vhigh = 0;
		}
		else
		{
			vhigh = para[1] & 0xff;
		}
		vlow = para[0] & 0xff;
		mipi_data = (vhigh/*mipi_data*/<<8) | vlow/*+*/ /*RegAddr*/;
// 			tt = 2*ii;
// 			if(count > 2*ii)
// 			{
			//if(count == (tt + 1)) 
			//	*(para+tt) = 0;
			//mipi_data = *(para+tt);
		//mipi_data = (mipi_data<<8) + *(para+tt-1);

		WriteDATASSD2828(channel,mipi_data); //实际数据
		para += 2;
		count -= 2;
			//printf("mipi_data is 0x%04x      ii=%d\n",mipi_data, ii);
// 			}
// 			else
// 			{
// 				break;
// 			}
	}
//	}
}


void mipi_send_code(int mipi_channel, unsigned char reg, unsigned char *p_reg_data, int len)
{
	int i = 0;
	int all_code_len = 0;
	int code_offset = 3;	// cmd + reg + len;

	#define MAX_SEND_CODE_LEN		(1024 * 1)
	
	unsigned char mipi_code[MAX_SEND_CODE_LEN] = { 0 };
	memset(mipi_code, 0xFF, MAX_SEND_CODE_LEN);
	
	for (i = 0; i < len; i ++)
	{
		mipi_code[i + code_offset] = p_reg_data[i];
	}
	
	
	mipi_code[0] = 0x39;
	mipi_code[1] = reg;
	mipi_code[2] = len;
	all_code_len = len + 3;

	printf("mipi_send_code: mipi_channel = %d, len = %d.\n", mipi_channel, len);
	dump_data1(mipi_code, all_code_len);

	SendCode(mipi_channel, mipi_code);
}


int mipi_send_code2(int mipi_channel, unsigned char reg, unsigned char *p_reg_data, int len)
{
	char str_mipi_code[1024 * 4] = "";
	unsigned char temp_mipi_code[1024 * 4];

	sprintf(str_mipi_code, "R%02x", reg);

	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];		
		
		sprintf(cc, " %02X", p_reg_data[i]);
		strcat(str_mipi_code, cc);
	}

	printf("mipi_send_code2: gen code: \n%s\n", str_mipi_code);

	mipi_to_lp_mode(mipi_channel);
				
	memset(temp_mipi_code, 0xff, sizeof(temp_mipi_code));
    mipi_write_init_code("/tmp/mipi", str_mipi_code, strlen(str_mipi_code));
    mipi_parse_init_code("/tmp/mipi", temp_mipi_code, NULL);
    SendCode(mipi_channel, temp_mipi_code);

	// leave lp mode
	mipi_to_hs_mode(mipi_channel);

	return 0;
}


