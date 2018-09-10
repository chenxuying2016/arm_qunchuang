#include "hi_unf_i2c.h"
#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define I2C0_FILE_NAME "/dev/i2c-0"
#define I2C1_FILE_NAME "/dev/i2c-1"

static int i2c_fd[2] = {-1, -1};

//#define way1 0

MS_S32 HI_UNF_I2C_Init(MS_VOID)
{
    // Open a connection to the I2C userspace control file.
    if ((i2c_fd[0] = open(I2C0_FILE_NAME, O_RDWR)) < 0)
    {
        printf("Unable to open i2c-0 control file\n");
        return -1;
    }
    if ((i2c_fd[1] = open(I2C1_FILE_NAME, O_RDWR)) < 0)
    {
        printf("Unable to open i2c-1 control file\n");
        return -1;
    }
    return 0;
}

MS_S32 HI_UNF_I2C_DeInit(MS_VOID)
{
	//xujie
	if(i2c_fd[0] != -1) 
	{
		close(i2c_fd[0]);
		i2c_fd[0] = -1;
	}
	if(i2c_fd[1] != -1)
	{
		close(i2c_fd[1]);
		i2c_fd[1] = -1;
	}
	//xujie
}

MS_S32 HI_UNF_I2C_CreateGpioI2c(MS_U32 *pu32I2cNum, MS_U32 whichGpioClock, MS_U32 whichGpioData, MS_U32 clockBit, MS_U32 dataBit)
{

}

MS_S32 HI_UNF_I2C_DestroyGpioI2c(MS_S32 u32I2cNum)
{

}

MS_S32 HI_UNF_I2C_Read(MS_U32 u32I2cNum, MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U32 u32RegAddrCount, MS_U8 *pu8Buf, MS_U32 u32Length)
{
#if 1
    struct i2c_rdwr_ioctl_data e2prom_data;
    int ret;

    e2prom_data.nmsgs = 2;//因为都时序要两次，所以设为2
    e2prom_data.msgs = (struct i2c_msg *)malloc(e2prom_data.nmsgs * sizeof(struct i2c_msg));
    if (!e2prom_data.msgs){
       printf("Memory alloc error\n");
       return 0;
    }

    ioctl(i2c_fd[u32I2cNum], I2C_TIMEOUT, 2);//设置超时时间
    ioctl(i2c_fd[u32I2cNum], I2C_RETRIES, 1);//设置重发次数

    e2prom_data.nmsgs = 2;//读时序要两次过程，要发两次I2C消息
    e2prom_data.msgs[0].len = 1;//信息长度为1，第一次只写要读的eeprom中存储单元的地址
    e2prom_data.msgs[0].addr  = u8DevAddress;
    e2prom_data.msgs[0].flags = 0;//写命令，看读时序理解
    e2prom_data.msgs[0].buf = (unsigned char*)malloc(1);
    e2prom_data.msgs[0].buf[0] = u32RegAddr;//信息值

    e2prom_data.msgs[1].len = 1;
    e2prom_data.msgs[1].addr = u8DevAddress;
    e2prom_data.msgs[1].flags = I2C_M_RD;//读命令
    e2prom_data.msgs[1].buf = (unsigned char*)malloc(1);
    e2prom_data.msgs[1].buf[0] = 0;//先清空要读的缓冲区

    ret = ioctl (i2c_fd[u32I2cNum], I2C_RDWR, (unsigned long)&e2prom_data);//好了，读吧
    if (ret < 0){
        printf ("ioctl read error\n");
    }
    printf("read x from e2prom address x\n",e2prom_data.msgs[1].buf[0], u32RegAddr);

    pu8Buf[0] = e2prom_data.msgs[1].buf[0];

	free(e2prom_data.msgs[0].buf); //xujie
	free(e2prom_data.msgs[1].buf); //xujie
	free(e2prom_data.msgs); //xujie

    return ret;
#else
    int ret;
    char regAddr = u32RegAddr;
    ioctl(i2c_fd[u32I2cNum], I2C_SLAVE, u8DevAddress);	// 设置从设备地址
    ioctl(i2c_fd[u32I2cNum], I2C_TIMEOUT, 0X02);	// 设置超时
    ioctl(i2c_fd[u32I2cNum], I2C_RETRIES, 0X01);	// 设置重试次数
    ret = write(i2c_fd[u32I2cNum], &regAddr, 1);
    ret |= read(i2c_fd[u32I2cNum], pu8Buf, u32Length);
    if(ret<0)
    {
        printf("dev %x red reg:%d error data :%d!--ret %d\n",u8DevAddress,regAddr,pu8Buf[0],ret);
    }
    return ret;
#endif
}

MS_S32 HI_UNF_I2C_Write(MS_U32 u32I2cNum, MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U32 u32RegAddrCount, MS_U8 *pu8Buf, MS_U32 u32Length)
{
    unsigned char *outbuf;
    outbuf = (unsigned char*)malloc(u32Length+1);
    /* The first byte indicates which register we'll write */
    outbuf[0] = u32RegAddr;
    /*
     * The second byte indicates the value to write.  Note that for many
     * devices, we can write multiple, sequential registers at once by
     * simply making outbuf bigger.
     */
    memcpy(&outbuf[1],pu8Buf,u32Length);
#if way1
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    messages[0].addr  = u8DevAddress;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;


    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;

    if(ioctl(i2c_fd[u32I2cNum], I2C_RDWR, &packets) < 0) {
        printf("Unable to send data reg:%d\n",u32RegAddr);
        perror("Unable to send data\n");
        return 1;
    }
    free(outbuf);
    return 0;
#else
    int ret;
    ioctl(i2c_fd[u32I2cNum], I2C_SLAVE, u8DevAddress);	// 设置从设备地址
    ioctl(i2c_fd[u32I2cNum], I2C_TIMEOUT, 0X02);	// 设置超时
    ioctl(i2c_fd[u32I2cNum], I2C_RETRIES, 0X01);	// 设置重试次数
    ret = write(i2c_fd[u32I2cNum], outbuf, u32Length+1);
    if(ret<0)
    {
        printf("write reg:%d [%d]error!\n",outbuf[0],outbuf[1]);
    }
    free(outbuf);
    return ret;
#endif
}

MS_S32 HI_UNF_I2C_SetRate(MS_U32 u32I2cNum, HI_UNF_I2C_RATE_E enI2cRate)
{

}

void i2c_read_tst(int channel,int devaddr,int regAddr,int countmax)
{
    MS_U8 regVal,count = 0;
    while(count < countmax)
    {
        if(HI_UNF_I2C_Read(channel, devaddr, regAddr, 1, &regVal, 1)>=0)
        {
            printf("read the value is 0x%x\n",regVal);
        }
        count++;
        usleep(1000000);
    }
}

void i2c_write_tst(int channel,int devaddr,int regAddr,int reg)
{
    MS_U8 count = 0;

    char regVal = reg;
    //while(count < 100)
    {
        if(HI_UNF_I2C_Write(channel, devaddr, regAddr, 1, &regVal, 1)>=0)
        {
            printf("write the value is 0x%x\n",regVal);
        }
        count++;
        usleep(1000000);
    }
}
