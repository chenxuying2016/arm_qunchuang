#include "stdio.h"
#include "hi_unf_gpio.h"

#if USED_NEW_GPIO

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
	   

typedef struct tag_gpio_msg
{
	unsigned char pin;
	unsigned char dir;
	unsigned char value;
	unsigned char data;
}gpio_msg_t;
	
	
#define GPIO_CMD_GET_VALUE				0x3
#define GPIO_CMD_SET_VALUE				0x4
#define GPIO_CMD_SET_OUT_VALUE			0x5

#define GPIO_DEV						"/dev/gpio_helper"

int static s_gpio_fd = -1;

MS_S32 HI_UNF_GPIO_Init()
{
    s_gpio_fd = open(GPIO_DEV,O_RDWR);
	if (s_gpio_fd < 0)
	{
		printf("HI_UNF_GPIO_Init: open %s failed!\n", GPIO_DEV);
		return -1;
	}

	return 0;
}

MS_S32 HI_UNF_GPIO_DeInit()
{
    if (s_gpio_fd > 0)
		close(s_gpio_fd);

	return 0;
}

MS_S32 HI_UNF_GPIO_Open(MS_U32 u32GpioNo)
{
	return 0;
}

MS_S32 HI_UNF_GPIO_Close(MS_U32 u32GpioNo)
{
	return 0;
}

MS_S32 HI_UNF_GPIO_WriteBit(MS_U32 u32GpioNo, MS_BOOL bHighVolt )
{
	gpio_msg_t msg = { 0 };
	msg.pin = u32GpioNo;
	msg.value = bHighVolt;
	ioctl(s_gpio_fd, GPIO_CMD_SET_OUT_VALUE, &msg);
    return 0;
}

MS_S32 HI_UNF_GPIO_SetDirBit(MS_U32 u32GpioNo, MS_BOOL bInput)
{
    return 0;
}

MS_S32 HI_UNF_GPIO_GetDirBit(MS_U32 u32GpioNo, MS_BOOL *pbInput)
{
    return 0;
}

void gpio_set_output_value(int pin, int value)
{
	gpio_msg_t msg = { 0 };
	msg.pin = pin;
	msg.value = value;
	ioctl(s_gpio_fd, GPIO_CMD_SET_OUT_VALUE, &msg);
	//printf("gpio_set_output_value: pin = %d, value = %d.\n", pin, value);
}

static int execute(char* command,char* buf,int bufmax)
{
    FILE* fp;
    int i;
    if((fp=popen(command,"r"))==NULL){
        i=sprintf(buf,"error command line:%s \n",command);
    }else{
        i=0;
        while((buf[i]=fgetc(fp))!=EOF && i<bufmax-1)
            i++;
        pclose(fp);
    }
    buf[i]='\0';
    return i;
}

int gpio_export(int io_pin)
{
    char cmd[255];
    sprintf(cmd, "echo %d > /sys/class/gpio/export", io_pin);
    system(cmd);
    return 0;
}

int gpio_unexport(int io_pin)
{
    char cmd[255];
    sprintf(cmd, "echo %d > /sys/class/gpio/unexport", io_pin);
    system(cmd);
    return 0;
}

int gpin_read_value(int io_pin)
{
    char cmd[255];
    char out[255];
    int  ret;
	
    sprintf(cmd,"cat /sys/class/gpio/gpio%d/value", io_pin);

	printf("%s\n", cmd);
    ret = execute(cmd,out,sizeof(out));
    if(ret > 0)
    {
        int val  = strtol(out,0,0);
		printf("value: %d.\n", val);
        return val;
    }
	
    return 0;
}

int gpio_write_value(int io_pin, int value)
{
    char cmd[255];
    sprintf(cmd,"echo %d > /sys/class/gpio/gpio%d/value", value, io_pin);
    system(cmd);
    return 0;
}

int gpio_set_direction(int io_pin, int is_input)
{
//	if(gpio_info[u32GpioNo].input == bInput) return 0; //xujie
    char cmd[255];
    if(is_input)
    {
        sprintf(cmd," echo in > /sys/class/gpio/gpio%d/direction", io_pin);
    }
    else
    {
        sprintf(cmd," echo out > /sys/class/gpio/gpio%d/direction", io_pin);
    }
    system(cmd);
    return 0;
}

#else

typedef struct tag_gpio_info_s
{
    int open;
    int input;
    int value;
}gpio_info_t;

#define MAX_GPIO_NUM 118

static gpio_info_t   gpio_info[MAX_GPIO_NUM];
static int   gpioIsInit = 0;

static int execute(char* command,char* buf,int bufmax)
{
    FILE* fp;
    int i;
    if((fp=popen(command,"r"))==NULL){
        i=sprintf(buf,"error command line:%s \n",command);
    }else{
        i=0;
        while((buf[i]=fgetc(fp))!=EOF && i<bufmax-1)
            i++;
        pclose(fp);
    }
    buf[i]='\0';
    return i;
}

MS_S32 HI_UNF_GPIO_Init()
{
    int i;
    if(!gpioIsInit)
    {
        for(i=0;i<MAX_GPIO_NUM;i++)
        {
            gpio_info[i].open = 0;
			gpio_info[i].input = -1; //xujie
			gpio_info[i].value = -1; //xujie
        }
        gpioIsInit = 1;
    }
}

MS_S32 HI_UNF_GPIO_DeInit()
{
    int i;
    for(i=0;i<MAX_GPIO_NUM;i++)
    {
        if(gpio_info[i].open)
        {
            HI_UNF_GPIO_Close(i);
        }
    }
}

MS_S32 HI_UNF_GPIO_Open(MS_U32 u32GpioNo)
{
//	if(gpio_info[u32GpioNo].open == 1) return 0; //xujie
    char cmd[255];
    sprintf(cmd,"echo %d > /sys/class/gpio/export",u32GpioNo);
    system(cmd);
    gpio_info[u32GpioNo].open = 1;
    return 0;
}

MS_S32 HI_UNF_GPIO_Close(MS_U32 u32GpioNo)
{
//	if(gpio_info[u32GpioNo].open == 0) return 0; //xujie
    char cmd[255];
    sprintf(cmd,"echo %d > /sys/class/gpio/unexport",u32GpioNo);
    system(cmd);
    gpio_info[u32GpioNo].open = 0;
	gpio_info[u32GpioNo].input = -1; //xujie
	gpio_info[u32GpioNo].value = -1; //xujie
    return 0;
}

MS_S32 HI_UNF_GPIO_ReadBit(MS_U32 u32GpioNo, MS_BOOL *pbHighVolt)
{
    char cmd[255];
    char out[255];
    int  ret;
    sprintf(cmd,"cat /sys/class/gpio/gpio%d/value",u32GpioNo);
    ret = execute(cmd,out,sizeof(out));
    if(ret > 0)
    {
        *pbHighVolt = strtol(out,0,0);
        return 0;
    }
    return -1;
}

MS_S32 HI_UNF_GPIO_WriteBit(MS_U32 u32GpioNo, MS_BOOL bHighVolt)
{
//	if(gpio_info[u32GpioNo].value == bHighVolt) return 0; //xujie
    char cmd[255];
    sprintf(cmd,"echo %d > /sys/class/gpio/gpio%d/value",bHighVolt,u32GpioNo);
    system(cmd);
    gpio_info[u32GpioNo].value = bHighVolt;
    return 0;
}

MS_S32 HI_UNF_GPIO_SetDirBit(MS_U32 u32GpioNo, MS_BOOL bInput)
{
//	if(gpio_info[u32GpioNo].input == bInput) return 0; //xujie
    char cmd[255];
    if(bInput)
    {
        sprintf(cmd," echo in > /sys/class/gpio/gpio%d/direction",u32GpioNo);
        gpio_info[u32GpioNo].input = 1;
    }
    else
    {
        sprintf(cmd," echo out > /sys/class/gpio/gpio%d/direction",u32GpioNo);
        gpio_info[u32GpioNo].input = 0;
    }
    system(cmd);
    return 0;
}

MS_S32 HI_UNF_GPIO_GetDirBit(MS_U32 u32GpioNo, MS_BOOL *pbInput)
{
    char cmd[255];
    char out[255];
    int  ret;
    sprintf(cmd,"cat /sys/class/gpio/gpio%d/direction",u32GpioNo);
    ret = execute(cmd,out,sizeof(out));
    if(ret > 0)
    {
        if(strcmp(out,"in") == 0)
        {
            *pbInput = 1;
        }
        else //"out"
        {
            *pbInput = 0;
        }
        return 0;
    }
    return -1;
}

MS_S32 HI_UNF_GPIO_SetIntType(MS_U32 u32GpioNo, HI_UNF_GPIO_INTTYPE_E enIntType)
{

}

MS_S32 HI_UNF_GPIO_SetIntEnable(MS_U32 u32GpioNo, MS_BOOL bEnable)
{

}

MS_S32 HI_UNF_GPIO_QueryInt(MS_U32 *p32GpioNo, MS_U32 u32TimeoutMs)
{

}


void gpio_set_output_value(int pin, int value)
{
	HI_UNF_GPIO_Open(pin);
    HI_UNF_GPIO_SetDirBit(pin, 0); //output
    HI_UNF_GPIO_WriteBit(pin, value);

	//printf("set gpio %d: value = %d.\n", pin, value);
}
#endif

