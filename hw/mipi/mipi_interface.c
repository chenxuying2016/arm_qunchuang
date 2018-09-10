#include "hi_unf_spi.h"

#ifdef JUST_USED_MIPI_CHANNEL_1
extern int m_channel_1; 
#else
//xujie
extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;
//xujie
#endif


void WriteSSD2828(int channel, char addr, unsigned short data)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = addr;
	HI_UNF_SPI_Write(channel/*i*/, tx, 3/*sizeof(tx)*/);
	tx[0] = 0x72;
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);
}

void WriteCMDSSD2828(int channel, unsigned char command)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = command;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);
}

void WriteDATASSD2828(int channel, unsigned short data)
{
	unsigned char tx[3] = {0x72,0x00,0x00};
	tx[0] = 0x72;
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);
}

unsigned short ReadSSD2828Reg(int channel,/*int fd,*/char addr)
{
    unsigned char tx[3] = {0x70,0x00,0x00};
    unsigned char rx[3] = {0x00,0x00,0x00};
    tx[2] = addr;
    HI_UNF_SPI_Write(channel/*-1*/,tx,3/*sizeof(tx)*/);

    memset(tx,0,3/*sizeof(tx)*/);
    tx[0] = 0x73;
    HI_UNF_SPI_Read(channel/*-1*/,tx,3/*sizeof(tx)*/,rx);

    return (rx[1]<<8)|rx[2];
}

unsigned short ReadDATASSD2828Reg(int channel,/*int fd,*/char addr)
{
    unsigned char tx[3] = {0x73,0x00,0x00};
    unsigned char rx[3] = {0x00,0x00,0x00};

    HI_UNF_SPI_Read(channel/*-1*/,tx,3/*sizeof(tx)*/,rx);
    return (rx[1]<<8)|rx[2];
}


void WriteFPGATo2828_8bit(int channel,char addr,unsigned short data)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = addr;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);

	tx[0] = 0x72;
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);
}

void WriteCMDFPGATo2828_8bit(int channel,char command)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = command;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);
}

void WriteDATAFPGATo2828_8bit(int channel,unsigned short data)
{
	unsigned char tx[3] = {0x72,0x00,0x00};
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	HI_UNF_SPI_Write(channel/*i*/,tx,3/*sizeof(tx)*/);
}


void SSD2828_read_ids()
{	
#ifdef JUST_USED_MIPI_CHANNEL_1	
	unsigned short id = ReadSSD2828Reg(m_channel_1/*1*/, /*0,*/ 0xB0);
	printf("=============================================================\n");
	printf("channel %d: SSD2828_read_ids: 0x%X. \n", m_channel_1, id);
#else
	unsigned short id = ReadSSD2828Reg(m_channel_1/*1*/, /*0,*/ 0xB0);
	printf("=============================================================\n");
	printf("channel %d: SSD2828_read_ids: 0x%X. \n", m_channel_1, id);
	
	if(m_channel_2 > 0)
	{
		unsigned short id = ReadSSD2828Reg(m_channel_2/*2*/, /*0,*/ 0xB0);
		printf("channel %d: SSD2828_read_ids: 0x%X. \n", m_channel_2, id);
		printf("=============================================================\n");
	}

	#if 0
	if(m_channel_3 > 0)
	{
		id = ReadSSD2828Reg(m_channel_3/*2*/, /*0,*/ 0xB0);
		printf("channel %d: SSD2828_read_ids: 0x%X. \n", m_channel_3, id);
		printf("=============================================================\n");
	}
	
	if(m_channel_4 > 0)
	{
		id = ReadSSD2828Reg(m_channel_4/*2*/, /*0,*/ 0xB0);
		printf("channel %d: SSD2828_read_ids: 0x%X. \n", m_channel_4, id);
		printf("=============================================================\n");
	}
	#endif
	
#endif
}

void SSD2828_read_reg(int channel, char reg)
{
	unsigned short id = ReadSSD2828Reg(channel/*1*/, /*0,*/ reg);
	printf("2828 read reg: 0x%02x, value: 0x%04x.\n", reg, id);
}

void SSD2828_write_reg(int channel, char reg, unsigned short data/*char data1, char data2*/)
{
//	unsigned short data = (data1 << 8) | data2;
	WriteSSD2828(channel/*1*/, reg, data);
	printf("2828 write reg: 0x%02x, value: 0x%04x.\n", reg, data);
}



void _time_delay(int delay)  // ms or us?
{
    usleep(delay*1000); //now I think it's us.
}

void Delay_I2C(int delay) // ms or us?
{
    usleep(delay * 1000); //now I think it's us.   //xujie 原始是没有注释的
}

void mtp_power_on(int channel) //15对应的是channel 1,   14对应的是channel 2
{
    //gpio 14,15 high
	if(channel == 3 || channel == 4)
	{
		HI_UNF_GPIO_Open(14);
		HI_UNF_GPIO_SetDirBit(14,0); //output
		HI_UNF_GPIO_WriteBit(14,1);  //mipi pull high
	}
	else if(channel == 1 || channel == 2)
	{
		HI_UNF_GPIO_Open(15);
		HI_UNF_GPIO_SetDirBit(15,0); //output
		HI_UNF_GPIO_WriteBit(15,1);  //mipi pull high
	}
}

void mtp_power_off(int channel) //15对应的是channel 1,   14对应的是channel 2
{
    //gpio 14 15 low
	if(channel == 3 || channel == 4)
    {
		HI_UNF_GPIO_Open(14);
		HI_UNF_GPIO_SetDirBit(14,0); //output
		HI_UNF_GPIO_WriteBit(14,0);  //mipi pull low
	}
	else if(channel == 1 || channel == 2)
    {
		HI_UNF_GPIO_Open(15);
		HI_UNF_GPIO_SetDirBit(15,0); //output
		HI_UNF_GPIO_WriteBit(15,0);  //mipi pull low
	}
}
