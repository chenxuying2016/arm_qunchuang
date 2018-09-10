#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "pubmipi.h"

#include "spi_sp6.h"

extern uint8_t HsSignalOnFlag;

#define SP6_ENABLE_DEBUG		(0)


#define MIPI_CMD_SPI_DEV			"/dev/spidev32766.0"
//#define SP6_SPI_CS_PIN				(88)

#define SPI_CS_MIPI_1_CHIP_2828_U3		(90)
#define SPI_CS_MIPI_1_CHIP_2828_U4		(91)
#define SPI_CS_MIPI_1_CHIP_SP6			(92)


static int 		s_mipi_cmd_fd = -1;
static uint32_t s_mipi_spi_mode  = 0;
static uint8_t  s_mipi_spi_bits  = 8;
static uint32_t s_mipi_spi_speed = 3000000;//1500000;
static uint16_t s_mipi_spi_delay = 0;


static int mipi_cmd_spi_open()
{
	int ret = 0;

	s_mipi_cmd_fd = open(MIPI_CMD_SPI_DEV, O_RDWR);
	if (s_mipi_cmd_fd < 0)
	{
		printf("mipi_cmd_spi_open: can't open device %s\n", MIPI_CMD_SPI_DEV);
		return -1;
	}

	/*
	 * spi mode
	 */
	ret = ioctl(s_mipi_cmd_fd, SPI_IOC_WR_MODE32, &s_mipi_spi_mode);
	if (ret == -1)
	{
		printf("mipi_cmd_spi_open: can't set spi mode\n");
		return -1;
	}

	ret = ioctl(s_mipi_cmd_fd, SPI_IOC_RD_MODE32, &s_mipi_spi_mode);
	if (ret == -1)
	{
		printf("mipi_cmd_spi_open: can't get spi mode\n");
		return -1;
	}

	/*
	 * bits per word
	 */
	ret = ioctl(s_mipi_cmd_fd, SPI_IOC_WR_BITS_PER_WORD, &s_mipi_spi_bits);
	if (ret == -1)
	{
		printf("mipi_cmd_spi_open: can't set bits per word\n");
		return -1;
	}

	ret = ioctl(s_mipi_cmd_fd, SPI_IOC_RD_BITS_PER_WORD, &s_mipi_spi_bits);
	if (ret == -1)
	{
		printf("mipi_cmd_spi_open: can't get bits per word\n");
		return -1;
	}

	/*
	 * max speed hz
	 */
	ret = ioctl(s_mipi_cmd_fd, SPI_IOC_WR_MAX_SPEED_HZ, &s_mipi_spi_speed);
	if (ret == -1)
	{
		printf("mipi_cmd_spi_open: can't set max speed hz\n");
		return -1;
	}

	ret = ioctl(s_mipi_cmd_fd, SPI_IOC_RD_MAX_SPEED_HZ, &s_mipi_spi_speed);
	if (ret == -1)
	{
		printf("mipi_cmd_spi_open: can't get max speed hz\n");
		return -1;
	}

	printf("mipi_cmd_spi_open: spi mode: 0x%x\n", s_mipi_spi_mode);
	printf("mipi_cmd_spi_open: bits per word: %d\n", s_mipi_spi_bits);
	printf("mipi_cmd_spi_open: max speed: %d Hz (%d KHz)\n", s_mipi_spi_speed, s_mipi_spi_speed/1000);

	return 0;
}

static void mipi_cmd_spi_close()
{
	if (s_mipi_cmd_fd > 0)
	{
		close(s_mipi_cmd_fd);
		s_mipi_cmd_fd = -1;
	}
}

static int mipi_cmd_spi_read(char *pInData, int dataLen, char *poutData)
{
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pInData,
        .rx_buf = poutData,
        .len = dataLen,
        .delay_usecs = s_mipi_spi_delay,
        .speed_hz = s_mipi_spi_speed,
        .bits_per_word = s_mipi_spi_bits,
    };
	
    ret = ioctl(s_mipi_cmd_fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1)
    {
        printf("mipi_cmd_spi_write: %s can't read spi message\n",__FUNCTION__);
        return -1;
    }
	
    return 0;
}

int mipi_cmd_spi_write(char *pInData, int dataLen)
{
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pInData,
        .rx_buf = 0,
        .len = dataLen,
        .delay_usecs = s_mipi_spi_delay,
        .speed_hz = s_mipi_spi_speed,
        .bits_per_word = s_mipi_spi_bits,
    };

    ret = ioctl(s_mipi_cmd_fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1)
    {
        printf("mipi_cmd_spi_write: %s can't send spi message\n",__FUNCTION__);
        return -1;
    }
    return 0;
}


void sp6_cs_select(unsigned char pin)
{	
	gpio_set_output_value(pin, 0);
}

void sp6_cs_deselect(unsigned char pin)
{	
	gpio_set_output_value(pin, 1);
}

int mipi_cmd_init()
{
	// cs reset
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_2828_U3);
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_2828_U4);
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_SP6);
	
	mipi_cmd_spi_open();

	//gpio_set_output_value(SP6_SPI_CS_PIN, 0);
}

int mipi_cmd_term()
{
	//gpio_set_output_value(SP6_SPI_CS_PIN, 1);
	mipi_cmd_spi_close();

	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_2828_U3);
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_2828_U4);
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_SP6);
}

int mipi_cmd_write_cmd(unsigned char command)
{
	unsigned char tx[] = {0x70,0x00,0x00};
    tx[2] = command;
    mipi_cmd_spi_write(tx, sizeof(tx));
	return 0;
}

int mipi_cmd_write_data(unsigned short data)
{
	unsigned char tx[] = {0x72,0x00,0x00};
    tx[1] = (data&0xFF00) >> 8;
    tx[2] = data&0xFF;
    mipi_cmd_spi_write(tx, sizeof(tx));
	return 0;
}

int mipi_cmd_write_reg_data(unsigned char addr, unsigned short data)
{
	unsigned char tx[] = {0x70,0x00,0x00};
    tx[2] = addr;
    mipi_cmd_spi_write(tx, sizeof(tx));
	
    usleep(1);
    tx[0] = 0x72;
    tx[1] = (data&0xFF00) >> 8;
    tx[2] = data&0xFF;
    mipi_cmd_spi_write(tx,sizeof(tx));
	return 0;
}

unsigned short mipi_cmd_read_reg_data(unsigned char addr)
{
	unsigned char tx[] = {0x78,0x00,0x00};
    unsigned char rx[] = {0x00,0x00,0x00};

    tx[2] = addr;
    mipi_cmd_spi_write(tx, sizeof(tx));

    memset(tx,0,sizeof(tx));
    tx[0] = 0x73;
    mipi_cmd_spi_read(tx, sizeof(tx), rx);

    return (rx[1]<<8)|rx[2];
}

unsigned short mipi_cmd_read_data()
{
	unsigned char tx[] = {0x78,0x00,0x00};
	unsigned char rx[] = {0x00,0x00,0x00};

	mipi_cmd_spi_read(tx, sizeof(tx), rx);

	return (rx[1]<<8)|rx[2];
}

// CMDDCSShortWriteNoPara
void mipi_cmd_dcs_short_write_no_param(uint8_t command)
{
    /*WriteFPGATo2828_8bit(spifd,0XB7,0x0340);
    WriteFPGATo2828_8bit(spifd,0XB8,0X0000);//VC Control Register
    WriteFPGATo2828_8bit(spifd,0XBC,0x0000);
    WriteCMDFPGATo2828_8bit(spifd,command);*/

	mipi_cmd_write_reg_data(0xB7, 0x0340);
	mipi_cmd_write_reg_data(0xB8, 0X0000);
	mipi_cmd_write_reg_data(0xBC, 0);
	mipi_cmd_write_data(command);
}

// CMDDCSShortWriteOnePara
void mipi_cmd_dcs_short_write_1_param(uint8_t command, uint8_t para)
{
    /*uint16_t mipi_data;
    mipi_data = para;
    mipi_data = (mipi_data<<8) + command;
    WriteFPGATo2828_8bit(spifd,0XB7,0x0340);
    WriteFPGATo2828_8bit(spifd,0XB8,0X0000);//VC Control Register
    WriteFPGATo2828_8bit(spifd,0XBC,0x0002);
    WriteFPGATo2828_8bit(spifd,0Xbf,mipi_data);*/

	unsigned data = (para << 8) | command ;
	mipi_cmd_write_reg_data(0xB7, 0x0340);
	mipi_cmd_write_reg_data(0xB8, 0X0000);
	mipi_cmd_write_reg_data(0xBC, 2);
	mipi_cmd_write_reg_data(0xBF, data);
}

// CMDGeneralShortWriteTwoPara
void mipi_cmd_gen_short_write_2_params(uint8_t command, uint8_t para)
{
    /*uint16_t mipi_data;
    mipi_data = para;
    mipi_data = (mipi_data<<8) + command;
    WriteFPGATo2828_8bit(spifd,0XB7,0x0300);
    WriteFPGATo2828_8bit(spifd,0XB8,0X0000);//VC Control Register
    WriteFPGATo2828_8bit(spifd,0XBC,0x0002);
    WriteFPGATo2828_8bit(spifd,0Xbf,mipi_data);*/

	unsigned data = (para << 8) | command ;
	mipi_cmd_write_reg_data(0xB7, 0x0300);
	mipi_cmd_write_reg_data(0xB8, 0X0000);
	mipi_cmd_write_reg_data(0xBC, 2);
	mipi_cmd_write_reg_data(0xBF, data);
}

//void CMDDCSLongWriteWithPara(uint8_t RegAddr, uint8_t *para, uint8_t count)	
void mipi_cmd_dcs_long_write_n_params(uint8_t RegAddr, uint8_t *para, uint8_t count)
{
    /*uint16_t mipi_data,ii,tt;
    WriteFPGATo2828_8bit(spifd,0XB7,0x0340);
    WriteFPGATo2828_8bit(spifd,0XB8,0X0000);//VC Control
    mipi_data = count;
    WriteFPGATo2828_8bit(spifd,0XBC,mipi_data);
    mipi_data = *para;
    mipi_data = (mipi_data<<8) + RegAddr;
    WriteFPGATo2828_8bit(spifd,0Xbf,mipi_data);
	
    for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) 
            	*(para+tt) = 0;
            	
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            WriteDATAFPGATo2828_8bit(spifd,mipi_data);
        }
        else
        {
            break;
        }
    }*/
	
	mipi_cmd_write_reg_data(0xB7, 0x0340);
	mipi_cmd_write_reg_data(0xB8, 0X0000);

	uint16_t param_cnt = count;
	mipi_cmd_write_reg_data(0xBC, param_cnt);
	
	unsigned short data = (para[0] << 8) | RegAddr ;
	mipi_cmd_write_reg_data(0xBF, data);

	uint16_t mipi_data = 0;
	uint16_t ii = 0;
	uint16_t tt = 0;
	
	for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) 
            	*(para+tt) = 0;
            	
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            mipi_cmd_write_data(mipi_data);
        }
        else
        {
            break;
        }
    }
}

//void CMDGerneralLongWriteWithPara(uint8_t RegAddr, uint8_t *para, uint8_t count)
void mipi_cmd_gen_long_write_n_params(uint8_t RegAddr, uint8_t *para, uint8_t count)	
{
    /*uint16_t mipi_data,ii,tt;
    WriteFPGATo2828_8bit(spifd,0XB7,0x0300);
    WriteFPGATo2828_8bit(spifd,0XB8,0X0000);//VC Control Register
    mipi_data = count;
    WriteFPGATo2828_8bit(spifd,0XBC,mipi_data);
    mipi_data = *para;
    mipi_data = (mipi_data<<8) + RegAddr;
    WriteFPGATo2828_8bit(spifd,0Xbf,mipi_data);
    for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) *(para+tt) = 0;
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            WriteDATAFPGATo2828_8bit(spifd,mipi_data);
        }
        else
        {
            break;
        }
    }*/

	mipi_cmd_write_reg_data(0xB7, 0x0300);
	mipi_cmd_write_reg_data(0xB8, 0X0000);

	uint16_t param_cnt = count;
	mipi_cmd_write_reg_data(0xBC, param_cnt);
	
	unsigned short data = (para[0] << 8) | RegAddr ;
	mipi_cmd_write_reg_data(0xBF, data);

	uint16_t mipi_data = 0;
	uint16_t ii = 0;
	uint16_t tt = 0;
	
	for(ii = 1; ii < 256; ii ++)
    {
        tt = 2*ii;
        if(count > 2*ii)
        {
            if(count == (tt + 1)) 
            	*(para+tt) = 0;
            	
            mipi_data = *(para+tt);
            mipi_data = (mipi_data<<8) + *(para+tt-1);

            mipi_cmd_write_data(mipi_data);
        }
        else
        {
            break;
        }
    }
}


void mipi_cmd_send_code(uint8_t *InitCode)
{
    uint16_t i = 0;
	uint16_t delaytime = 0;
    uint8_t j = 0;
    uint8_t MipiData[250] = { 0 };
    uint16_t CodePoint = 0;
	
    for(i = 0; i < MAX_MIPI_CODE_LEN; i++)
    {
        if(InitCode[CodePoint + 2] <= 250)
        {
            for(j = 0; j < InitCode[CodePoint + 2]; j++)
            {
                MipiData[j] = InitCode[CodePoint + 3 + j];
            }
			
            if((InitCode[CodePoint + 0] == 0x00) && (InitCode[CodePoint + 1] == 0x00))  //type 0x05 DCS Short WRITE, no parameters
            {
                delaytime =  MipiData[0] + (MipiData[1] << 8);
                _time_delay(delaytime);
            }
			
            if((InitCode[CodePoint + 0] == 0x00) && (InitCode[CodePoint + 1] == 0x03))  //type 0x05 DCS Short WRITE, no parameters
            {
                //WriteFPGATo2828_8bit(0XB7,0X030b);
                //HsSignalOnFlag = 1;
            }
			
            if(InitCode[CodePoint + 0] == 0x05)   //type 0x05 DCS Short WRITE, no parameters
            {
                //CMDDCSShortWriteNoPara(InitCode[CodePoint+1]);
                mipi_cmd_dcs_short_write_no_param(InitCode[CodePoint+1]);
                Delay_I2C(30);
            }
			
            if(InitCode[CodePoint + 0] == 0x32)   //type 0x05 DCS Short WRITE, no parameters
            {

            }
			
            if(InitCode[CodePoint + 0] == 0x15)   //type 0x15 DCS Short WRITE, 1 parameter
            {
                //CMDDCSShortWriteOnePara(InitCode[CodePoint+1], InitCode[CodePoint+3]);
				mipi_cmd_dcs_short_write_1_param(InitCode[CodePoint+1], InitCode[CodePoint+3]);
                Delay_I2C(30);
            }
			
            if(InitCode[CodePoint + 0] == 0x23)   //type 0x23 Generic Short WRITE, 2 parameters
            {
                //CMDGeneralShortWriteTwoPara(InitCode[CodePoint+1], InitCode[CodePoint+3]);
				mipi_cmd_gen_short_write_2_params(InitCode[CodePoint+1], InitCode[CodePoint+3]);
                Delay_I2C(30);
            }
			
            if(InitCode[CodePoint + 0] == 0x29)   //type 0x29 Generic Long Write (Max 8 byte for register access)
            {
                //CMDGerneralLongWriteWithPara(InitCode[CodePoint+1],MipiData,InitCode[CodePoint + 2]+1);
				mipi_cmd_gen_long_write_n_params(InitCode[CodePoint+1], MipiData, InitCode[CodePoint + 2]+1);
                Delay_I2C(30);
            }
			
            if(InitCode[CodePoint + 0] == 0x39)   //type 0x39 DCS Long Write (Max 8 byte for register access)
            {
                if(HsSignalOnFlag == 0)
                {
                    //CMDDCSLongWriteWithPara(InitCode[CodePoint+1],MipiData,InitCode[CodePoint + 2]+1);
					mipi_cmd_dcs_long_write_n_params(InitCode[CodePoint+1], MipiData, InitCode[CodePoint + 2]+1);
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

unsigned char sp6_read_reg_data(unsigned char reg)
{
	unsigned char read_cmd[3] = { 0x78, 0x00, 0x00 };
	unsigned char read_buf[3] = "";

	read_cmd[1] = reg;
	mipi_cmd_spi_read(read_cmd, sizeof(read_cmd), read_buf);

	#if SP6_ENABLE_DEBUG
	printf("mipi_cmd_read_reg_data: reg: %02x, data: %02x.\n", reg, read_buf[2]);
	#endif
	
	return read_buf[2];
}

//寄存器名称：lvds_ttl_ctrl地址：20h 类型：-RW
// bit7~bit6:ttl模式选择(00:normal；01: hs+vs mode；10: DE only mode)
// Bit5为通道2的打开ttl使能信号，为低时打开使能。
// Bit4为通道1的打开ttl使能信号，为低时打开使能。
// Bit3为通道2的打开lvds预加重使能信号，为高时打开使能。
// Bit2为通道1的打开lvds预加重使能信号，为高时打开使能。
// Bit1为通道2的打开lvds使能信号，为高时打开使能。
// Bit0为通道1的打开lvds使能信号，为高时打开使能。

int sp6_open_lvds_signal()
{
	unsigned char write_data[3] = { 0x70, 0x20, 0x3F };
	//write_data[2] = 0x33;
	mipi_cmd_spi_write(write_data, sizeof(write_data));
}

int sp6_close_lvds_signal()
{
	unsigned char write_data[3] = { 0x70, 0x20, 0x30 };
	mipi_cmd_spi_write(write_data, sizeof(write_data));
}

int sp6_open_ttl_signal(int channel)
{
	unsigned char write_data[3] = { 0x70, 0x20, 0x0F };
	
	// 此处电平低为有效。
	if (channel == 0)
		write_data[2] = 0x20;
	else if (channel == 1)
		write_data[2] = 0x10;
	
	mipi_cmd_spi_write(write_data, sizeof(write_data));

	#if SP6_ENABLE_DEBUG
	printf("sp6_open_ttl_signal: channel = %d.\n", channel);
	#endif
	
	sp6_read_reg_data(0x20);
}

int sp6_set_ttl_timing_mode(int is_de_mode)
{
	unsigned reg_value = 0;
	reg_value = sp6_read_reg_data(0x20);

	reg_value &= 0x3F;
	
	if (is_de_mode)
	{
		printf("DE mode.\n");
		reg_value |= 0x80;
	}
	else
	{	
		printf("Sync mode.\n");
		reg_value |= 0x40;
	}

	unsigned char write_data[3] = { 0x70, 0x20, 0x0F };
	write_data[2] = reg_value;
	printf("sp6_set_ttl_timing_mode: is_de_mode = %d, reg_value: %x.\n", is_de_mode, reg_value);
	
	mipi_cmd_spi_write(write_data, sizeof(write_data));
	sp6_read_reg_data(0x20);
}

int sp6_open_all_ttl_signal()
{
	unsigned char write_data[3] = { 0x70, 0x20, 0x0F };
	
	// 此处电平低为有效。
	//write_data[2] = 0x40;	// VSync|channel both.
	write_data[2] = 0x80;	// DE|channel both.
	
	mipi_cmd_spi_write(write_data, sizeof(write_data));

	#if SP6_ENABLE_DEBUG
	printf("sp6_open_ttl_signal: channel = %d.\n", channel);
	#endif
	
	sp6_read_reg_data(0x20);
}


int sp6_open_ttl_buffer()
{
	unsigned char write_data[3] = { 0x70, 0x20, 0x0F };
	
	// 此处电平低为有效。
	write_data[2] = 0x00;
	
	mipi_cmd_spi_write(write_data, sizeof(write_data));

	#if SP6_ENABLE_DEBUG
	printf("sp6_open_ttl_signal\n");
	#endif
	
	sp6_read_reg_data(0x20);
}


int sp6_close_ttl_signal(int channel)
{
	unsigned char write_data[3] = { 0x70, 0x20, 0x00 };
	mipi_cmd_spi_write(write_data, sizeof(write_data));
}

// reg29:
// bit 0: open/short
// bit 1: ttl clock pol. channel 1; [right]
// bit 2: ttl clock pol. channel 2; [left]

int sp6_entry_open_short_mode()
{
	#if SP6_ENABLE_DEBUG
	printf("sp6_entry_open_short_mode\n");
	#endif
	
	unsigned char oe_data[3] = { 0x70, 0x29, 0x01 };
	mipi_cmd_spi_write(oe_data, sizeof(oe_data));

	sp6_read_reg_data(0x29);
}

int sp6_leave_open_short_mode()
{
	#if SP6_ENABLE_DEBUG
	printf("sp6_leave_open_short_mode\n");
	#endif
	
	unsigned char oe_data[3] = { 0x70, 0x29, 0x00 };
	mipi_cmd_spi_write(oe_data, sizeof(oe_data));

	sp6_read_reg_data(0x29);
}

unsigned char sp6_read_open_short_mode()
{
	#if SP6_ENABLE_DEBUG
	printf("sp6_read_open_short_mode\n");
	#endif
	
	return sp6_read_reg_data(0x29);
}

int sp6_set_clock_pol_mode(int is_on)
{
	unsigned char oe_data[3] = { 0x70, 0x29, 0x02 };
	if (is_on)
	{
		oe_data[2] = 0x02;
		printf("sp6_set_clock_pol_mode: -.\n");
	}
	else
	{
		printf("sp6_set_clock_pol_mode: +.\n");
		oe_data[2] = 0x00;
	}
	
	//#if SP6_ENABLE_DEBUG
	printf("sp6_set_clock_pol_mode: %x.\n", oe_data[2]);
	//#endif
	
	mipi_cmd_spi_write(oe_data, sizeof(oe_data));

	sp6_read_reg_data(0x29);
}


int sp6_set_open_short_data(int open_short_channel, unsigned char value_1, unsigned char value_2,
							unsigned char value_3, unsigned char value_4)
{
	sp6_entry_open_short_mode();
	
	if (open_short_channel == 0)
	{
		sp6_open_ttl_signal(open_short_channel);

		#if SP6_ENABLE_DEBUG
		printf("sp6_set_open_short_data: channel = %d.\n", open_short_channel);
		#endif
		
		// value 1
		unsigned char value1_data[3] = { 0x70, 0x21, 0x00 };
		value1_data[2] = value_1;
		mipi_cmd_spi_write(value1_data, sizeof(value1_data));
		sp6_read_reg_data(0x21);

		// value 2
		unsigned char value2_data[3] = { 0x70, 0x22, 0x00 };
		value2_data[2] = value_2;
		mipi_cmd_spi_write(value2_data, sizeof(value2_data));
		sp6_read_reg_data(0x22);

		// value 3
		unsigned char value3_data[3] = { 0x70, 0x23, 0x00 };
		value3_data[2] = value_3;
		mipi_cmd_spi_write(value3_data, sizeof(value3_data));
		sp6_read_reg_data(0x23);

		// value 4
		unsigned char value4_data[3] = { 0x70, 0x24, 0x00 };
		value4_data[2] = value_4;
		mipi_cmd_spi_write(value4_data, sizeof(value4_data));
		sp6_read_reg_data(0x24);
	}
	else
	{
		sp6_open_ttl_signal(open_short_channel);

		#if SP6_ENABLE_DEBUG
		printf("sp6_set_open_short_data: channel = %d.\n", open_short_channel);
		#endif
		
		// value 1
		unsigned char value1_data[3] = { 0x70, 0x25, 0x00 };
		value1_data[2] = value_1;
		mipi_cmd_spi_write(value1_data, sizeof(value1_data));
		sp6_read_reg_data(0x25);

		// value 2
		unsigned char value2_data[3] = { 0x70, 0x26, 0x00 };
		value2_data[2] = value_2;
		mipi_cmd_spi_write(value2_data, sizeof(value2_data));
		sp6_read_reg_data(0x26);

		// value 3
		unsigned char value3_data[3] = { 0x70, 0x27, 0x00 };
		value3_data[2] = value_3;
		mipi_cmd_spi_write(value3_data, sizeof(value3_data));
		sp6_read_reg_data(0x27);

		// value 4
		unsigned char value4_data[3] = { 0x70, 0x28, 0x00 };
		value4_data[2] = value_4;
		mipi_cmd_spi_write(value4_data, sizeof(value4_data));
		sp6_read_reg_data(0x28);
	}
}


int ttl_sp6_read_reg(unsigned char reg)
{
	unsigned char read_cmd[3] = { 0x78, 0x00, 0x00 };
	unsigned char read_buf[3] = "";

	read_cmd[1] = reg;

	sp6_cs_select(SPI_CS_MIPI_1_CHIP_SP6);
	
	mipi_cmd_spi_read(read_cmd, sizeof(read_cmd), read_buf);

	//#if SP6_ENABLE_DEBUG
	printf("mipi_cmd_read_reg_data: cmd: %02x, reg: %02x, data: %02x.\n", read_buf[0], read_buf[1], read_buf[2]);
	//#endif
	
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_SP6);
	return read_buf[2];
}

int ttl_sp6_write_reg(unsigned char reg, unsigned char data)
{
	unsigned char write_data[3] = { 0x70, 0x00, 0x00 };
	write_data[1] = reg;
	write_data[2] = data;
	
	sp6_cs_select(SPI_CS_MIPI_1_CHIP_SP6);
	printf("ttl_sp6_write_reg: %02x %02x %02x.\n", write_data[0], write_data[1], write_data[2]);
	mipi_cmd_spi_write(write_data, sizeof(write_data));
	sp6_cs_deselect(SPI_CS_MIPI_1_CHIP_SP6);
	
	return 0;
}


int sp6_rw_test()
{
	printf("************* SP6 Control ********************\n");

	usleep(100 * 1000);

	unsigned char write_data[3] = { 0x70, 0x20, 0x55 };
	//mipi_cmd_spi_write(write_data, sizeof(write_data));
	
	//int read_reg = mipi_cmd_read_reg_data(0, sp6_reg);
	unsigned char read_cmd[3] = { 0x78, 0x20, 0x00 };
	unsigned char read_buf[3] = "";
	mipi_cmd_spi_read(read_cmd, sizeof(read_cmd), read_buf);

	#if SP6_ENABLE_DEBUG
	printf("mipi_cmd_read_reg_data: cmd: %02x, reg: %02x, data: %02x.\n", read_buf[0], read_buf[1], read_buf[2]);
	#endif
	
	usleep(1000 * 1000);

}

// bit1~bit0:数据位宽(01:6bit；10:8bit；00:10bit)。
// bit2 0:vasa 1:jeida
int sp6_set_lcd_bit_and_vesa_jeida(int bit_mode, int is_vesa_mode)
{
	unsigned char write_data[3] = { 0x70, 0x2A, 0x30 };
	unsigned value = 0;

	switch(bit_mode)
	{
		case 6:
			value |= 0x01;
			printf("sp6_set_lcd_bit_and_vesa_jeida: 6bit.\n");
			break;

		case 8:
			value |= 0x02;
			printf("sp6_set_lcd_bit_and_vesa_jeida: 8bit.\n");
			break;

		case 10:
			value |= 0x03;
			printf("sp6_set_lcd_bit_and_vesa_jeida: 10bit.\n");
			break;
	}

	if (!is_vesa_mode)
	{
		value &= ~0x04;
		printf("sp6_set_lcd_bit_and_vesa_jeida: vesa.\n");
	}
	else
	{
		value |= 0x04;
		printf("sp6_set_lcd_bit_and_vesa_jeida: jeida.\n");
	}

	printf("set reg value: %02x.\n", value);
	
	write_data[2] = value;
	mipi_cmd_spi_write(write_data, sizeof(write_data));

	return 0;
}




