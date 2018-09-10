#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#ifdef __WIN32__
#include <winsock2.h>
#include <winsock.h>
#include <wininet.h>
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
#endif
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
//
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <ctype.h>  
#include <sys/un.h>  
#include <sys/ioctl.h>  
#include <sys/socket.h>  
#include <linux/types.h>  
#include <linux/netlink.h>  
#include <errno.h> 

#include<errno.h>

#include<unistd.h>

#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>

//
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "json/cJSON.h"
#include "util/util.h"
#include "rwini.h"
#include "packsocket/packsocket.h"
#include "client.h"
#include "util/debug.h"
#include "comStruct.h"
#include "fpgaFunc.h"
#include "pubFpga.h"
#include "common.h"
#include "comStruct.h"
#include "hw/box/box.h"
//#include "lua.h"

//xujie
#ifdef JUST_USED_MIPI_CHANNEL_1
extern int m_channel_1; 
#else
extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;
#endif
//xujie

//USB????
#define MODE (S_IRWXU | S_IRWXG | S_IRWXO)
#define MAX_USB_UPGRADE_LIST_NUMBER   8
pthread_t  m_usb_upgrade_thread;
sem_t      m_usb_upgrade_sem;
int        m_usb_upgrade_read_index = 0;
int        m_usb_upgrade_write_index = 0;
char       m_usb_upgrade_dev_list[MAX_USB_UPGRADE_LIST_NUMBER][40];

void* usb_upgrade_task(void* arg)
{
	char txt[1000];
	while(1)
	{
		//�ȴ�USB̽����
		sem_wait(&m_usb_upgrade_sem);

		//�õ�һ���豸
		char* dev = m_usb_upgrade_dev_list[m_usb_upgrade_read_index];
		m_usb_upgrade_read_index += 1;
		if(m_usb_upgrade_read_index >= MAX_USB_UPGRADE_LIST_NUMBER)
		{
			m_usb_upgrade_read_index = 0;
		}

		//
		printf("usb: %s \n", dev);
		system("rm -rf /usb");
		if(1)
		{
			int i, is_ok = 0;
			sprintf(txt, "/mnt/%s/update.txt", dev);
			for(i=0; i < 50; i++)
			{
			/*	DIR *mydir = NULL;
				if((mydir= opendir(dir))==NULL)//�ж�Ŀ¼ 
				{
					int ret = mkdir(dir, MODE);//����Ŀ¼
					if (ret != 0)
					{
						return -1;
					}
					printf("%s created sucess!/n", dir);
				}*/
				FILE* fp = fopen(txt, "rb");
				if(fp == NULL)
				{
					//sprintf(txt, "umount /opt");
					//system(txt); //û�м�鵽�����ļ�, ��رչ���
					//printf("fopen: /usb/update.txt is not found.\n");
					//continue;
				}
				else
				{
					fclose(fp);
					is_ok = 1;
					break;
				}
				/*if(access(txt, 0) == 0)
				{
					is_ok = 1;
					break;
				}*/
				usleep(1000 * 100);
			}
			if(is_ok == 0)
			{
				printf("fopen: /mnt/%s/update.txt is not found.\n", dev);
				continue;
			}
		}
		//usleep(1000 * 500);

		//����U�̵�/optĿ¼
		//sprintf(txt, "mount /dev/%s /opt", dev);
		sprintf(txt, "ln -s /mnt/%s /usb", dev);
		system(txt);
/*#if 1
		int i;
		int is_mounted = 0;
		for(i=0; i < 2; i++)
		{
			int ret = system(txt);
			if(ret == 0)
			{
				is_mounted = 1;
				break;
			}
			usleep(1000*500);
		}
		if(is_mounted == 0)
		{
			printf("mount: /dev/%s /opt is error.\n", dev);
			continue;
		}
#else
		if(0 != system(txt)) continue; //����ʧ��
#endif
		printf("mount: /dev/%s /opt is ok.\n", dev);*/

		//�����û�������ļ�
		FILE* fp = fopen("/usb/update.txt", "rb");
		if(fp == NULL)
		{
			//sprintf(txt, "umount /opt");
			//system(txt); //û�м�鵽�����ļ�, ��رչ���
			printf("fopen: /usb/update.txt is not found.\n");
			continue;
		}
		printf("fopen: /usb/update.txt is ok.\n");

		//֪ͨ���ƺ�������ʼ
		#ifdef ENABLE_CONTROL_BOX
		boxRebackFlicker(0, 0, "������ʼ, ��ȴ�...", NULL);
		#endif
		//
		int parse_count = 0;
		memset(txt, 0, sizeof(txt));
		while(fgets(txt, sizeof(txt), fp))
		{
			switch(txt[0]){
			case ';':  //ע����
			case '\r': //����
			case '\n': //����
				break;
			default: //ƥ��ָ��
				if(!strncmp(txt, "[FOPEN]", strlen("[FOPEN]"))) //ֱ�Ӷ��ļ�
				{
					char* ptr = txt + strlen("[FOPEN]");
					//����1, ������BOX, ����POWER
#define USB_FILE_TYPE_POWER 1
#define USB_FILE_TYPE_BOX   2
					int type = 0; //0-������, 1-��Դ, 2-���ƺ�
					char* ptr_type_start = strstr(ptr, "[");
					char* ptr_type_end = strstr(ptr, "]");
					if(ptr_type_end)
					{
						ptr = ptr_type_end + 1;
					}
					if(ptr_type_start && ptr_type_end)
					{
						ptr_type_start += 1;
						*ptr_type_end = 0;
						//
						int len = strlen(ptr_type_start);
						if(len > 0)
						{
							if(!strncmp(ptr_type_start, "POWER", strlen("POWER")))
							{
								type = USB_FILE_TYPE_POWER;
							}
							else if(!strncmp(ptr_type_start, "BOX", strlen("BOX")))
							{
								type = USB_FILE_TYPE_BOX;
							}
						}
					}
					//����2, �ļ�
					char* ptr_file_start = strstr(ptr, "[");
					char* ptr_file_end = strstr(ptr, "]");
					if(ptr_file_end)
					{
						ptr = ptr_file_end + 1;
					}
					if(ptr_file_start && ptr_file_end)
					{
						ptr_file_start += 1;
						*ptr_file_end = 0;
						//
						int len = strlen(ptr_file_start);
						if(len > 0)
						{
							FILE* fpFile = fopen(ptr_file_start, "rb");
							if(fpFile != NULL)
							{
								fseek(fpFile, 0, SEEK_END);
								unsigned int file_size = ftell(fpFile);
								fseek(fpFile, 0, SEEK_SET);
								unsigned char* file_buffer = (unsigned char*)malloc(file_size);
								fread(file_buffer, file_size, 1, fpFile);
								fclose(fpFile);
								if(type == USB_FILE_TYPE_POWER)
								{
									userPwrUpgrade(file_buffer, file_size);
								}
								else if(type == USB_FILE_TYPE_BOX)
								{
									#ifdef ENABLE_CONTROL_BOX
									box_upgrade(file_buffer, file_size);
									#endif
								}
								free(file_buffer);
							}
						}
					}
				}
				else if(!strncmp(txt, "[SYSTEM]", strlen("[SYSTEM]"))) //����system
				{
					char* ptr = txt + strlen("[SYSTEM]");
					//����1
					char* ptr_start = strstr(ptr, "[");
					char* ptr_end = strstr(ptr, "]");
					if(ptr_end)
					{
						ptr = ptr_end + 1;
					}
					if(ptr_start && ptr_end)
					{
						ptr_start += 1;
						*ptr_end = 0;
						//
						int len = strlen(ptr_start);
						if(len > 0)
						{
							system(ptr_start);
							printf("system: %s\n", ptr_start);
						}
					}
				}
				else if(!strncmp(txt, "[DELAY]", strlen("[DELAY]"))) //��ʱ
				{
					char* ptr = txt + strlen("[DELAY]");
					//����1
					char* ptr_start = strstr(ptr, "[");
					char* ptr_end = strstr(ptr, "]");
					if(ptr_end)
					{
						ptr = ptr_end + 1;
					}
					if(ptr_start && ptr_end)
					{
						ptr_start += 1;
						*ptr_end = 0;
						//
						int ms = atoi(ptr_start);
						usleep(1000 * ms);
					}
				}
				else if(!strncmp(txt, "[TEXT]", strlen("[TEXT]"))) //��ʾ����
				{
					char* ptr = txt + strlen("[TEXT]");
					//����1
					char* ptr_start = strstr(ptr, "[");
					char* ptr_end = strstr(ptr, "]");
					if(ptr_end)
					{
						ptr = ptr_end + 1;
					}
					if(ptr_start && ptr_end)
					{
						ptr_start += 1;
						*ptr_end = 0;

						#ifdef ENABLE_CONTROL_BOX
						boxRebackFlicker(0, 0, ptr_start, NULL);
						#endif
					}
				}
				break;
			}
			memset(txt, 0, sizeof(txt));
		}
		
		fclose(fp);

		#ifdef ENABLE_CONTROL_BOX
		boxRebackFlicker(0, 0, "�������,ϵͳ������...", NULL);
		#endif
		
		system("sync"); //�ȴ�ϵͳдͬ�����
		usleep(1000*100); //100ms
		//system("umount /opt");
		//system("umount /tmp");
		//֪ͨ���ƺ��������
		//usleep(1000*100); //100ms
		system("reboot");
	}
}


static crossCursorInfo_t  gstCrossCursorInfo;

static char *skipWord(char *p)
{
   while((*p)==' ')
   {
       p++;
   }
   return p;
}

static int client_usr_cmd(char * usr_cmd,char arg[][32])
{
    int i=0;
    char *p = usr_cmd;
    char *pFind;
    while(p)
    {
        pFind = p;
        pFind = strstr(pFind," ");
        if(!pFind)
        {
            strcpy(arg[i],p);
            break;
        }
        else
        {
            *pFind = '\0';
            strcpy(arg[i],p);
            *pFind = ' ';
            p= pFind;
        }
        p = skipWord(p);
        i++;
    }

    if(!strcasecmp(arg[0],"quit"))
        return 1;
    if(!strcasecmp(arg[0],"login"))
        return 2;
    if(!strcasecmp(arg[0],"get "))
        return 3;
    if(!strcasecmp(arg[0],"syfn"))
        return 4;
    if(!strcasecmp(arg[0],"rget"))
        return 5;
    if(!strcasecmp(arg[0],"rset"))
        return 6;
    if(!strcasecmp(arg[0],"mipi"))
        return 7;
    if(!strcasecmp(arg[0],"clock"))
        return 8;
    if(!strcasecmp(arg[0],"pwr"))
        return 9;
    if(!strcasecmp(arg[0],"ipwr"))
        return 10;
    if(!strcasecmp(arg[0],"cusor"))
        return 11;
    if(!strcasecmp(arg[0],"move"))
        return 12;
    if(!strcasecmp(arg[0],"reset"))
        return 13;
    if(!strcasecmp(arg[0],"flick"))
        return 14;
    if(!strcasecmp(arg[0],"autof"))
        return 15;
    if(!strcasecmp(arg[0],"i2cr"))
        return 16;
    if(!strcasecmp(arg[0],"i2cw"))
        return 17;
    if(!strcasecmp(arg[0],"reUsb"))
        return 18;
    if(!strcasecmp(arg[0],"tstFun"))
        return 19;
    if(!strcasecmp(arg[0],"rgb"))
        return 20;
    if(!strcasecmp(arg[0],"uart"))
        return 21;
    if(!strcasecmp(arg[0],"vcom"))
        return 22;
    if(!strcasecmp(arg[0],"ca310"))
        return 23;
	if(!strcasecmp(arg[0],"read"))
        return 24;
	if(!strcasecmp(arg[0], "fpga_read"))
        return 25;
	if(!strcasecmp(arg[0], "fpga_write"))
        return 26;
	
	if(!strcasecmp(arg[0], "gpio_set"))
        return 27;

	if(!strcasecmp(arg[0], "id"))
        return 28;
	
	if(!strcasecmp(arg[0], "vtest"))
        return 29;

	if(!strcasecmp(arg[0], "write"))
        return 30;

	if(!strcasecmp(arg[0], "channel"))
		return 31;
	
	if(!strcasecmp(arg[0], "mtpon"))
		return 32;
	
	if(!strcasecmp(arg[0], "mtpoff"))
		return 33;

	if(!strcasecmp(arg[0], "mread"))
		return 34;

	if(!strcasecmp(arg[0], "otpr"))
		return 35;

	if(!strcasecmp(arg[0], "qcread"))
		return 36;

	if(!strcasecmp(arg[0], "qcotp"))
		return 37;

	if(!strcasecmp(arg[0], "cabcread"))
		return 38;

	if(!strcasecmp(arg[0], "sp6r"))
		return 39;

	if(!strcasecmp(arg[0], "sp6w"))
		return 40;

	if(!strcasecmp(arg[0], "powercal"))
		return 41;
	
	if(!strcasecmp(arg[0], "flick_value"))
		return 42;

    return -1;
}


void show_help()
{
    printf("\[help]--print this command list\n");
    printf("\[login]--login to server\n");
    printf("\get [remote file]--get [remote file] to local host as\n");
    printf("\tif  isn't given, it will be the same with [remote_file] \n");
    printf("\tif there is any \' \' in , write like this \'\\ \'\n");
    printf("\[quit]--quit this ftp client program\n");
}

static int client_cmd()
{
    int  cmd_flag;
    char usr_cmd[256];
    char arg[6][32];

    while(1)
    {
        printf("cmd_svn_z:>");
        fgets(usr_cmd, 256, stdin);
        fflush(stdin);
        if(usr_cmd[0] == '\n')
            continue;
        usr_cmd[strlen(usr_cmd)-1] = '\0';
        cmd_flag = client_usr_cmd(usr_cmd,arg);
        switch(cmd_flag)
        {
            case 1:
            {	
				printf("exit!\n");
                exit(0);
                break;
            }
            case 2:
            {
				printf("regeister!\n");
                client_register("123456","789123","12345678");
            }
            break;

            case 3:
            {
				printf("3!\n");
            }
            break;


            case 4:
            {
				printf("4!\n");
            }
            break;

            case 5:
            {
				printf("5: read reg!\n");
                int regAddr = strtol(arg[1],0,0);
				printf("reg read: %d, %#x.\n", regAddr, regAddr);
                //client_pg_readReg(regAddr);
                break;
            }

            case 6:
            {
				printf("6: write reg!\n");
                int regAddr  = strtol(arg[1],0,0);
                int regValue = strtol(arg[2],0,0);
				printf("reg[%d, %#x] write: %d, %#x.\n", regAddr, regAddr, regValue, regValue);
                //client_pg_writeReg(regAddr,regValue);
                break;
            }

            case 7:
            {
				printf("7: test mipi!\n");
                //test_mipi();
                break;
            }

            case 8:
            {
				printf("8: clock freq!\n");
                clock_5338_setFreq(strtol(arg[1],0,0));
                break;
            }

            case 9:
            {
				printf("9: power on!\n");
                int onoff = strtol(arg[1],0,0);
                pwrSetOnOff(onoff,1);
                break;
            }

            case 10:
            {
				printf("10: power off!\n");
                int i;
                char *pFirst;
                s1103PwrCfgDb pwrInfo;   //read from file
                pFirst = &pwrInfo;
                for(i=0;i<sizeof(s1103PwrCfgDb);i++)
                {
                    pFirst[i] = i;
                }
                pwrSetInfo(&pwrInfo);
            }
            break;

            case 11:
            {
				printf("11: cross cursor!\n");
                crossCursorInfo_t  crossCursorInfo;
                crossCursorInfo.enable = 1;
                crossCursorInfo.x = strtol(arg[1],0,0);
                crossCursorInfo.y = strtol(arg[2],0,0);
                crossCursorInfo.wordColor = 0xFFFFFF;
                crossCursorInfo.crossCursorColor = 0;
                crossCursorInfo.RGBchang = 0;
                crossCursorInfo.HVflag = 0;
                crossCursorInfo.startCoordinate = 0;
                fpga_reg_dev_ioctl(eSET_CROSSCURSOR, (unsigned long)&crossCursorInfo);
                memcpy(&gstCrossCursorInfo,&crossCursorInfo,sizeof(crossCursorInfo_t));
            }
            break;

            case 12:
            {
				printf("12: hide cursor!\n");
                crossCursorInfo_t  crossCursorInfo;
                memcpy(&crossCursorInfo,&gstCrossCursorInfo,sizeof(crossCursorInfo_t));
                int moveup = strtol(arg[1],0,0);
                if(moveup)
                {
                    crossCursorInfo.x += 10;
                }
                else
                {
                    crossCursorInfo.x -= 10;
                }
                printf("crossCursorInfo.x is %d\n",crossCursorInfo.x);
                fpga_reg_dev_ioctl(eSET_CROSSCURSOR, (unsigned long)&crossCursorInfo);
                memcpy(&gstCrossCursorInfo,&crossCursorInfo,sizeof(crossCursorInfo_t));
            }
            break;

            case 13:
            {
				printf("13: fpga!\n");
                fpga_reg_dev_ioctl(eRESET_FPGA, 0);
            }
            break;

            case 14:
            {
				printf("14: flick manual!\n");
                int vaule = strtol(arg[1],0,0);
                //flick_manual(vaule);
#if 0
				int lcd_channel = 2;
				lcd_autoFlickStart(lcd_channel, 1);
				int retVal = lcd_autoFlickWork(lcd_channel, 0);
				lcd_autoFlickEnd(lcd_channel);
				printf("------>>>>>>>>>>>>>flick=%d\n", retVal);
#endif
            }
            break;

            case 15:
            {
				printf("15: auto flick!\n");
                //flick_auto(1,0);
            }
            break;

            case 16:
            {
				printf("16: i2c_read tst!\n");
                int i2cChannel = strtol(arg[1],0,0);
                int i2cdevAddr = strtol(arg[2],0,0);
                int i2cRegAddr = strtol(arg[3],0,0);
                int count = strtol(arg[4],0,0);

				printf("channel: %d, dev addr: %d, %#x, reg: %d, %#x.\n", i2cChannel, i2cdevAddr, i2cdevAddr, i2cRegAddr, i2cRegAddr);
                i2c_read_tst(i2cChannel,i2cdevAddr,i2cRegAddr,count);
            }
            break;

            case 17:
            {
				printf("17: i2c_write tst!\n");
                int i2cChannel = strtol(arg[1],0,0);
                int i2cdevAddr = strtol(arg[2],0,0);
                int i2cRegAddr = strtol(arg[3],0,0);
                int i2cRegVal = strtol(arg[4],0,0);
                i2c_write_tst(i2cChannel,i2cdevAddr,i2cRegAddr,i2cRegVal);
            }
            break;

            case 18:
            {
				printf("18: gpio!\n");
                int gpioCount = 0;
                HI_UNF_GPIO_Open(47);
                HI_UNF_GPIO_SetDirBit(47,0); //output
                while(gpioCount<10000)
                {
                    HI_UNF_GPIO_WriteBit(47,0);
                    usleep(100);
                    HI_UNF_GPIO_WriteBit(47,1);
                    usleep(100);
                }
            }
            break;

            case 19:
            {
				printf("19: module!\n");
                dbInitModuleSelf();
                dbModuleSelfGetByName("8WX");
            }
            break;

            case 20:
            {
				printf("20: rgb!\n");
                showRgbStr   showRgb;
                showRgb.rgb_r = strtol(arg[1],0,0);
                showRgb.rgb_g = strtol(arg[2],0,0);
                showRgb.rgb_b = strtol(arg[3],0,0);
                unsigned int u32QueueId = fpga_message_queue_get();
                MsOS_SendMessage(u32QueueId,FPGA_CMD_SHOW_RGB,&showRgb,0);
                printf("show rgb %x %x %x\n",showRgb.rgb_r,showRgb.rgb_g,showRgb.rgb_b);
            }
            break;

            case 21:
            {
				printf("21: uart fd!\n");
                //unsigned char outBuf[] = {0x55,0xaa,0x0c,0x00,0x7e,0x7e,0x13,0x0c,0x36,0x00,0x7c,0x4f,0xee,0xe3,0x7e,0x7e};
                unsigned char outBuf[] = {0x55,0xaa,0x0d,0x00,0x7e,0x7e,0x13,0x0c,0x37,0x00,0x3d,0x7d,0x5e,0xf5,0xfa,0x7e,0x7e};
                int  len = sizeof(outBuf);
                int  fd = uartGetSerialfd(2);
                if (fd >= 0)
                {
                    int rlen = write(fd, outBuf, len);
                    printf("len is %d %d %d\n",len,rlen,fd);
                }
            }
            break;

            case 22:
            {
				printf("22: vcom!\n");

				int vcom_index = strtol(arg[1],0,0);
				//flick_manual(vcom_index);
            }
            break;

            case 23:
            {
				printf("23: ca310!\n");
				#ifndef USE_Z_CA210_LIB
                ca310_uart_open();
				#endif
            }
            break;

			case 24:
            {
				printf("read:\n");
                int reg = strtol(arg[1],0,0);
				printf("spi read reg: 0x%02x.\n", reg);
				SSD2828_read_reg(1, reg);
            }
            break;

			case 25:
            {
				printf("fpga read:\n");
                int reg = strtol(arg[1], 0, 0);	

				fpgaRegStr fpga_reg = { 0 };
				fpga_reg.ofset = reg;
				int val = fpga_reg_dev_ioctl(eREAD_FPGA_REG, &fpga_reg);
				printf("FPGA Read reg: 0x%02x, val: %#x.\n", fpga_reg.ofset, fpga_reg.value);
				
            }
            break;

			case 26:
            {
				printf("fpga write:\n");
                int reg = strtol(arg[1], 0, 0);	
				int val = strtol(arg[2], 0, 0);	

				fpgaRegStr fpga_reg = { 0 };
				fpga_reg.ofset = reg;
				fpga_reg.value = val;
				int rc = fpga_reg_dev_ioctl(eSET_FPGA_REG, &fpga_reg);
				printf("FPGA Write reg: 0x%02x, val: %#x.\n", fpga_reg.ofset, fpga_reg.value);
				
            }
            break;
			
			case 27:
			{
				printf("gpio_set:\n");
				int pin = strtol(arg[1], 0, 0); 
				int value = strtol(arg[2], 0, 0); 

				gpio_set_output_value(pin, value);
				printf("gpio_set pin: %d, value: %d.\n", pin, value);
				
			}
			break;

			case 28:
            {
				printf("do nothing!\n");
				SSD2828_read_ids();
            }
            break;
			
			case 29:
			{
				printf("do nothing!\n");

			}
			break;

			case 30:
            {
				printf("write:\n");
                int reg = strtol(arg[1], 0, 0);
				int val = strtol(arg[2], 0, 0);
				printf("spi write reg: 0x%02x.\n", reg);
				SSD2828_write_reg(1, reg, val);
            }
            break;

			case 31:
				{
					Generic_Write(2, 0x00, 0x00);
					Generic_Write(4, 0xff, 0x80, 0x19, 0x01);
					Generic_Write(2, 0x00, 0x80);
					Generic_Write(3, 0xff, 0x80, 0x19);
					//extern int m_Channel;
					//printf("read:\n");
					//m_Channel = strtol(arg[1], 0, 0);
					//int val = strtol(arg[2], 0, 0);
					//printf("spi write reg: 0x%02x.\n", reg);
					//SSD2828_write_reg(1, reg, val); //??????0??SPI
				}
				break;

			case 32: //mtpon
				{
					int channel = strtol(arg[1], 0, 0);
					//int val = strtol(arg[2], 0, 0);
					mtp_power_on(channel);
					printf("\n==========mtp_power_on %d\n", channel);
				}
				break;

			case 33: //mtpoff
				{
					int channel = strtol(arg[1], 0, 0);
					//int val = strtol(arg[2], 0, 0);
					mtp_power_off(channel); 
					printf("\n==========mtp_power_off %d\n", channel);
					//extern volatile int is_mtp_exit;
					//is_mtp_exit = 1;
				}
				break;

			case 34:
				{
					#if 0
					unsigned char mipi_channel = 1;
					unsigned char type = 1;	// 0: generic; 1: dcs.
					unsigned char reg = strtol(arg[2], 0, 0);
					unsigned char data = 0x00;
					unsigned char page = strtol(arg[1], 0, 0);
					int vcom = 50;

					page = 1;
					reg = 0x53;

					vcom = 50;

					ili_9806_set_page(mipi_channel, page);
					ili_9806_write_vcom(mipi_channel, vcom, 1);

					
					int i = 1000;
					for (; i ; i --)
					{
						
	    				ReadModReg(mipi_channel, type, reg, 1, &data);
						printf("i = %d.\n", i);
						printf("read page: %d, reg:%02x: => %02x.\n", page, reg, data);

						if (data != vcom)
						{
							printf("read error!\n");
							break;
						}
					}
					#endif
				}
				break;

				case 35:
				{
					//         1 - 4    index
					// otpr <channel>   <index>
					unsigned char mipi_channel = strtol(arg[1], 0, 0);
					unsigned char index = strtol(arg[2], 0, 0);
					unsigned char data = 0x00;

					printf("otp read index = %d, 0x%x.\n", index, index);
					
					qc_do_sleep_out(mipi_channel);
					qc_read_otp_data_reg(mipi_channel, index);
					qc_read_otp_data_reg(mipi_channel, index + 1);
					qc_read_otp_data_reg(mipi_channel, index + 2);
					qc_do_sleep_in(mipi_channel);
				}
				break;

				case 36:
				{
					//         1 - 4    0		 78
					// qcread <channel>   <page> <reg>
					unsigned char mipi_channel = strtol(arg[1], 0, 0);
					unsigned char page = strtol(arg[2], 0, 0);
					unsigned char reg = strtol(arg[3], 0, 0);

					printf("read reg: page = %d, reg = 0x%x.\n", page, reg);
					
					qc_do_sleep_out(mipi_channel);
					qc_read_reg(mipi_channel, page, reg);
					qc_do_sleep_in(mipi_channel);
				}
				break;

			case 37:
				{
					//         1 - 4 
					// qcotp <channel>
					unsigned char mipi_channel = strtol(arg[1], 0, 0);
					printf("otp.\n");
					
					//qc_otp_vcom(mipi_channel, 0x01, 0x23);
					//qc_otp_id(mipi_channel, 0x01, 0x02, 0x03);
					//qc_otp_vcom_by_mtp(mipi_channel, 0x00, 0x75, 0);
					//qc_otp_vcom_r(mipi_channel, 0x00, 0x75, 0);
				}
				break;

			case 38:
				{
					// power read cabc
					unsigned char mipi_channel = strtol(arg[1], 0, 0);
					
					printf("read cabc: channel = %d.\n", mipi_channel);
					
					unsigned short freq = 0;
					unsigned short duty;
					
					qc_cabc_test(mipi_channel, &freq, &duty);
					usleep(1 * 1000);
					qc_cabc_test(mipi_channel, &freq, &duty);

					printf("== cabc pwm: freq = %d, duty = %d.\n", freq, duty);
				}
				break;	
			
			case 39:
				{
					// sp6 read
					unsigned char reg = strtol(arg[1], 0, 0);
					
					printf("sp6r: read spartan6 reg: %x.\n", reg);

					#ifdef ENABLE_SP6_SPI
					int val = ttl_sp6_read_reg(reg);
					printf("read value: 0x%08x.\n", val);
					#endif
				}
				break;	

			case 40:
			{
				// sp6 read
				unsigned char reg = strtol(arg[1], 0, 0);
				unsigned char val = strtol(arg[2], 0, 0);
				
				printf("sp6w: write spartan6 reg: 0x%02x, value: 0x%02x.\n", reg, val);
				#ifdef ENABLE_SP6_SPI
				val = ttl_sp6_write_reg(reg, val);
				#endif
			}
			break;	

			case 41:
			{
				// sp6 read
				unsigned char power_channel = strtol(arg[1], 0, 0);
				unsigned char power_type = strtol(arg[2], 0, 0);
				int power_data = strtol(arg[3], 0, 0);

				
				printf("pwrcal: power calibration: channel = %d, power_type = %d, power_data = %d.\n", 
						power_channel, power_type, power_data);

				power_cmd_calibration_power_item(power_channel, power_type, 1, power_data);
			}
			break;	

			case 42:
			{
				// sp6 read
				unsigned char channel = strtol(arg[1], 0, 0);
				float flickValue = 0.0;

				ca210_start_flick_mode(channel);
				ca210_capture_flick_data(channel, &flickValue);

				printf("channel<%d>, value<%f>\r\n", channel, flickValue);
			}
			break;

            default:
                show_help();
                memset(usr_cmd, '\0',sizeof(usr_cmd));
                break;
        }
    }
    return 1;
}

volatile int is_mtp_exit = 0;

#if 0
#include        <stdio.h>
#include        "lua/lua.h"
#include        "lua/lualib.h"
#include        "lua/lauxlib.h"

/*the lua interpreter*/
lua_State* L;
int luaadd(int x, int y)
{
        int sum;
/*the function name*/
        lua_getglobal(L,"add");
/*the first argument*/
        lua_pushnumber(L, x);
/*the second argument*/
        lua_pushnumber(L, y);
/*call the function with 2 arguments, return 1 result.*/
        lua_call(L, 2, 1);
/*get the result.*/
        sum = (int)lua_tonumber(L, -1);
/*cleanup the return*/
        lua_pop(L,1);
        return sum;
}
#endif

void refresh_lcd_msg_pos()
{
	int lcd_width = 0;
	int lcd_height = 0;
	get_module_res(&lcd_width, &lcd_height);
	
	// set lcd text message.
	set_fpga_text_show(0,0,0,0);

	int opt_w = 300;
	int opt_h = 240;
	int text_w = 0;
	int text_h = 65;

	int otp_x = (lcd_width - opt_w) / 2;
	int otp_y = lcd_height - text_h * 2 - opt_h;

	int text_x = 5;
	int text_y = lcd_height - text_h * 2;

	#if 0
	set_fpga_text_x(m_channel_1, text_x);
	set_fpga_text_x(m_channel_2, text_x);
	set_fpga_text_y(m_channel_1, text_y);
	set_fpga_text_y(m_channel_2, text_y);
	#endif
	
	printf("otp: x = %d, y = %d. lcd: w=%d, h=%d.\n", otp_x, otp_y, lcd_width, lcd_height);

	#if 0
	set_fpga_otp_x(m_channel_1, otp_x);
	set_fpga_otp_x(m_channel_2, otp_x);
	
	set_fpga_otp_y(m_channel_1, otp_y);
	set_fpga_otp_y(m_channel_2, otp_y);
	#endif
}

void show_lcd_msg(int mipi_channel, int show_vcom, int show_ok, int vcom, int flick, int ok1, int ok2)
{
	#define LCD_MSG_COLOR_OK		(0)
	#define LCD_MSG_COLOR_NG		(1)
	
	if (show_ok)
	{
		if (ok1)
			set_fpga_text_color(1, LCD_MSG_COLOR_OK);
		else
			set_fpga_text_color(1, LCD_MSG_COLOR_NG);

		if (ok2)
			set_fpga_text_color(4, LCD_MSG_COLOR_OK);
		else
			set_fpga_text_color(4, LCD_MSG_COLOR_NG);
	}
	else
	{
		set_fpga_text_color(mipi_channel, LCD_MSG_COLOR_OK);
	}
	
	set_fpga_text_show(show_vcom, show_ok, ok1, ok2); //VCOM, OTP, OK, OK2
	set_fpga_text_vcom(mipi_channel, vcom);
	set_fpga_text_flick(mipi_channel, flick);
}

#define LOCKFILE "/home/pgClient.pid" 
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* set advisory lock on file */
int lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;  /* write lock */
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;  //lock the whole file

	return(fcntl(fd, F_SETLK, &fl));
}

int already_running(const char *filename)
{
	int fd;
	char buf[16];

	fd = open(filename, O_RDWR | O_CREAT, LOCKMODE);
	if (fd < 0) {
		printf("can't open %s: %m\n", filename);
		exit(1);
	}

	if (lockfile(fd) == -1) {
		if (errno == EACCES || errno == EAGAIN) {
			printf("file: %s already locked", filename);
			close(fd);
			return 1;
		}
		
		printf("can't lock %s: %m\n", filename);
		exit(1);
	}
	
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;
}

static int init_hotplug_sock(void)  
{  

	struct sockaddr_nl snl;  
	const int buffersize = 16 * 1024 * 1024;  
	int retval;  

	memset(&snl, 0x00, sizeof(struct sockaddr_nl));  
	snl.nl_family = AF_NETLINK;  
	snl.nl_pid = getpid();  
	snl.nl_groups = 1;  

	int hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);  
	if (hotplug_sock == -1)   
	{  
		printf("error getting socket: %s", strerror(errno));  
		return -1;  
	}  

	/* set receive buffersize */  
	setsockopt(hotplug_sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));  
	retval = bind(hotplug_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));  
	if (retval < 0) {  
		printf("bind failed: %s", strerror(errno));  
		close(hotplug_sock);  
		hotplug_sock = -1;  
		return -1;  
	}  

	return hotplug_sock;  

}

//
void main_cleanup_null(int ret)
{
	
}

void main_cleanup(int ret)
{
	printf("\n%d\n\n", ret);
	
	#ifndef USE_Z_CA210_LIB
	ca310_uart_close(1);
	ca310_uart_close(2);
	ca310_uart_close(3);
	ca310_uart_close(4);
	#endif
	
	mtp_power_off(1);
	mtp_power_off(2);
	mtp_power_off(3);
	mtp_power_off(4);
	HI_UNF_SPI_Destory();
	HI_UNF_I2C_DeInit();
	HI_UNF_GPIO_DeInit();

	exit(ret);
}

#if 1
#define NETWORK_CFG_FILE		"/mnt/mmcblk0p1/eth_cfg"
#define NETWORK_MAC_CFG_FILE	"/mnt/mmcblk0p1/mac_cfg"

typedef struct tab_network_info
{
	char tag_ip[3];			// "IP "
	char ip[16];

	char tag_netmask[8];	// "NETMASK "
	char netmask[16];

	char tag_gateway[8];	// "GATEWAY "
	char gateway[16];
	
}network_info_t;

typedef struct tab_network_mac_info
{
	char tag_mac[4];	// "MAC"
	char mac[18];		// "xx:xx:xx:xx:xx:xx"
}network_mac_info_t;


int net_work_config_write(char* p_str_ip, char* p_str_netmask, char* p_str_gateway)
{
	FILE* f = NULL;

	if ( (p_str_ip == NULL) || (p_str_netmask == NULL) || (p_str_gateway == NULL) )
	{
		printf("net_work_config_write error: invalid param!\n");
		return -1;
	}

	f = fopen(NETWORK_CFG_FILE, "wb");
	if (f == NULL)
	{
		printf("net_work_config_write error: open file %s failed!\n", NETWORK_CFG_FILE);
		return -1;
	}

	network_info_t net_info = { 0 };
	strcpy(net_info.tag_ip, "IP ");
	strcpy(net_info.ip, p_str_ip);

	strcpy(net_info.tag_netmask, "NETMASK ");
	strcpy(net_info.netmask, p_str_netmask);

	strcpy(net_info.tag_gateway, "GATEWAY ");
	strcpy(net_info.gateway, p_str_gateway);

	fwrite(&net_info, 1, sizeof(net_info), f);
	
	fclose(f);
	
	return 0;
}

int net_work_config_read(char* p_str_ip, int ip_buf_len, char* p_str_netmask, int netmask_buf_len, 
							char* p_str_gateway, int gateway_buf_len)
{
	FILE* f = NULL;

	f = fopen(NETWORK_CFG_FILE, "rb");
	if (f == NULL)
	{
		printf("net_work_config_read error: open file %s failed!\n", NETWORK_CFG_FILE);
		return -1;
	}

	network_info_t net_info = { 0 };
	fread(&net_info, 1, sizeof(net_info), f);

	strcpy(p_str_ip, net_info.ip);
	strcpy(p_str_netmask, net_info.netmask);
	strcpy(p_str_gateway, net_info.gateway);

	fclose(f);
	
	return 0;
}

int net_work_mac_config_write(char* p_str_mac)
{
	FILE* f = NULL;

	if ( p_str_mac == NULL )
	{
		printf("net_work_mac_config_write error: invalid param!\n");
		return -1;
	}

	f = fopen(NETWORK_MAC_CFG_FILE, "wb");
	if (f == NULL)
	{
		printf("net_work_mac_config_write error: open file %s failed!\n", NETWORK_MAC_CFG_FILE);
		return -1;
	}

	network_mac_info_t net_mac_info = { 0 };
	strcpy(net_mac_info.tag_mac, "MAC ");
	strcpy(net_mac_info.mac, p_str_mac);
	fwrite(&net_mac_info, 1, sizeof(net_mac_info), f);
	
	fclose(f);
	
	return 0;
}

int net_work_mac_config_read(char* p_str_mac, int mac_buf_len)
{
	FILE* f = NULL;

	f = fopen(NETWORK_MAC_CFG_FILE, "rb");
	if (f == NULL)
	{
		printf("net_work_mac_config_read error: open file %s failed!\n", NETWORK_MAC_CFG_FILE);
		return -1;
	}

	network_mac_info_t mac_info = { 0 };
	fread(&mac_info, 1, sizeof(mac_info), f);

	strcpy(p_str_mac, mac_info.mac);

	fclose(f);
	
	return 0;
}
							

#if 1
int net_work_config_save(char* str_ip, char* str_mask, char* str_gateway)
{	
	printf("net_work_config_save: ip=%s, netmask=%s, gateway=%s.\n", str_ip, str_mask, str_gateway);
	net_work_config_write(str_ip, str_mask, str_gateway);
	return 0;
}

int net_work_mac_config_save(char* str_mac)
{	
	if (str_mac)
	{
		printf("net_work_mac_config_save: mac=%s.\n", str_mac);
		net_work_mac_config_write(str_mac);
	}

	return 0;
}



int net_work_config()
{
	// init net device
	char* net_dev = "eth0";
	char str_ip[16] = "192.168.1.150";
	char str_mask[16] = "255.255.255.0";
	char str_gateway[16] = "192.168.1.1";

	char str_mac[18] = "70:B3:D5:00:00:00";

	int ip_buf_len = 16;
	int netmask_buf_len = 16;
	int gateway_buf_len = 16;
	int mac_buf_len = 18;
	
	net_work_config_read(str_ip, ip_buf_len, str_mask, netmask_buf_len, str_gateway, gateway_buf_len);
	net_work_mac_config_read(str_mac, mac_buf_len);
	
	printf("=== NetWork Info ===\n");
	printf("IP: %s\n", str_ip);
	printf("NetMask: %s\n", str_mask);
	printf("GateWay: %s\n", str_gateway);
	printf("MAC: %s\n", str_mac);

	char str_cmd[128] = { 0 };

	//sprintf(str_cmd,"ifconfig eth0 %s netmask %s", str_ip, str_mask);
	system("ifconfig eth0 down");
	
	// set mac.	
	sprintf(str_cmd, "ifconfig %s hw ether %s", net_dev, str_mac);
	system(str_cmd);
	printf("Set MAC Addr: %s\n", str_cmd);
	
	sprintf(str_cmd,"ifconfig eth0 %s netmask %s", str_ip, str_mask);
	system(str_cmd);
	
	sprintf(str_cmd,"route add default gw %s", str_gateway);
	system(str_cmd);

	system("ifconfig eth0 up");
	return 0;
}
#endif

int do_factory_reset()
{
	printf("do_factory_reset ...\n");
	system("rm /mnt/mmcblk0p1/eth_cfg -f");
	system("rm /mnt/mmcblk0p1/mac_cfg -f");

	system("rm /mnt/mmcblk0p2/cfg -rf");
	system("rm /mnt/mmcblk0p2/local.db -f");

	printf("do_factory_reset end.\n");
	return 0;
}

#endif

//main process
int main(int argc, char * argv[])
{
	if(already_running(LOCKFILE))
	{
		char txt[100] = "";
		FILE* fp = fopen(LOCKFILE, "rb");
		if(fp != NULL)
		{
			memset(txt, 0, sizeof(txt));
			fread(txt, sizeof(txt), 1, fp);
			fclose(fp);
		}
		printf("pgClient: running... %s %s %s \n", __DATE__, __TIME__, txt);
		return 0;
	}

	//printf("run ./listen &\n");
	//system("./listen &");

#ifdef SIGQUIT
	signal(SIGQUIT, main_cleanup); /* Quit (POSIX).  */
#endif

	signal(SIGINT , main_cleanup); /* Interrupt (ANSI).    */
	signal(SIGTERM, main_cleanup); /* Termination (ANSI).  */
	
#ifdef SIGXCPU
	signal(SIGXCPU, main_cleanup);
#endif

	signal(SIGILL, main_cleanup);                               //SIGILL 4 C ??????
	signal(SIGFPE, main_cleanup);                               //SIGFPE 8 C ??????
	signal(SIGKILL, main_cleanup);                              //SIGKILL 9 AEF Kill???
	signal(SIGSEGV, main_cleanup);                              //SIGSEGV 11 C ??��?????????
	signal(SIGPIPE, main_cleanup_null); //2015-12-28


	net_work_config();

	//do_factory_reset();
   
	printf("start main ...\n");
	HI_UNF_GPIO_Init();

	// set gpio default volt.
	volt_v33_v18_reset();

	init_cabc_io_config();
	
    HI_UNF_I2C_Init();
	
	HI_UNF_SPI_Init();

	#ifdef ENABLE_SP6_SPI
	mipi_cmd_init();
	#endif
	
	mtp_power_off(1);
	mtp_power_off(2);
	mtp_power_off(3);
	mtp_power_off(4);
	
	//CH1
	HI_UNF_GPIO_Open(10);
	HI_UNF_GPIO_SetDirBit(10,0); //output
	
	//CH2
	HI_UNF_GPIO_Open(11);
	HI_UNF_GPIO_SetDirBit(11,0); //output
	

#if 1
    MsOS_TimerInit();

    client_init();
#endif

	#if 1
    pgJob_init();

    localDBInit();
	
    clock_5338_init();

    move_ptn_init();

    ca310_uart_open(1);

    ca310_uart_open(2);

	dp_gpio_deconfig();
	
    pwrSetOnOff(0,1);

    client_rebackPower(0);

    load_cur_module();

#ifdef ENABLE_CONTROL_BOX
    initBox();
#endif

	#endif

	#ifdef USED_IC_MANAGER
	ic_mgr_init();
	#endif

	refresh_lcd_msg_pos();

#if ENABLE_CONSOLE
    client_cmd();
#else
	memset(m_usb_upgrade_dev_list, 0, sizeof(m_usb_upgrade_dev_list));
	sem_init(&m_usb_upgrade_sem, 0, /*u32InitCnt*/0);
	pthread_create(&m_usb_upgrade_thread, 0, (void*)usb_upgrade_task, NULL);
	//
	#define UEVENT_BUFFER_SIZE 2048
	int hotplug_sock = init_hotplug_sock();
    while(1)
    {
		char buf[UEVENT_BUFFER_SIZE*2] = {0};
		recv(hotplug_sock, &buf, sizeof(buf), 0);
		printf("%s\n", buf);
		if(!strncmp(buf, "add@", 4))
		{
			/*
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4/1-1.4:1.0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4/1-1.4:1.0/ttyUSB0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4/1-1.4:1.0/ttyUSB0/tty/ttyUSB0
			*/
			char* ptr = strstr(buf, "/tty/"); //????
			if(ptr != NULL)
			{
				ptr += 5;
				lcd_deviceAdd(ptr);

				/*printf("\n");
				int i=0;
				for(i=0; i < 20; i++)
				{
					if(ptr[i] == 0) break;
					printf("%02X ", ptr[i]);
				}
				printf("\n");*/
				//printf("===========add %s\n", ptr);
				continue;
			}
			/*
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/scsi_host/host0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0/0:0:0:0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0/0:0:0:0/scsi_disk/0:0:0:0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0/0:0:0:0/scsi_device/0:0:0:0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0/0:0:0:0/scsi_generic/sg0
			add@/devices/virtual/bdi/8:0
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0/0:0:0:0/block/sda
			add@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.7/1-1.7:1.0/host0/target0:0:0/0:0:0:0/block/sda/sda4
			*/
			ptr = strstr(buf, "/block/"); //U??
			if(ptr != NULL)
			{
				ptr += 7;
				char* ptr2 = strstr(ptr, "/");
				if(ptr2 != NULL)
				{
					ptr = ptr2 + 1;
					//??????????
					/*usleep(1000*500);
					char cmd[100];
					sprintf(cmd, "mount /dev/%s /opt", ptr);
					int ret = system(cmd);
					printf("================%d\n", ret);*/
					strcpy(m_usb_upgrade_dev_list[m_usb_upgrade_write_index], ptr);
					sem_post(&m_usb_upgrade_sem);
					m_usb_upgrade_write_index += 1;
					if(m_usb_upgrade_write_index >= MAX_USB_UPGRADE_LIST_NUMBER)
					{
						m_usb_upgrade_write_index = 0;
					}
				}
				
				/*char* devname = (char*)malloc(64);
				if(devname != NULL)
				{
					strcpy(devname, ptr);
					unsigned int u32QueueId = box_message_queue_get();
					MsOS_PostMessage(u32QueueId, 0x12345699, (MS_U32)devname, strlen(devname));
				}*/
				continue;
			}
		}
		else if(!strncmp(buf, "remove@", 7))
		{
			/*
			remove@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4/1-1.4:1.0/ttyUSB0/tty/ttyUSB0
			remove@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4/1-1.4:1.0/ttyUSB0
			remove@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4/1-1.4:1.0
			remove@/devices/amba.1/e0002000.usb/zynq-ehci.0/usb1/1-1/1-1.4
			*/
			char* ptr = strstr(buf, "/tty/");
			if(ptr != NULL)
			{
				ptr += 5;
				lcd_deviceRemove(ptr);
				//printf("===========remove %s\n", ptr);
				continue;
			}
		}
		//printf("SnoTest---------------%s\n", buf); 
        //usleep(5000);
    }
#endif

    return 1;
}

//
int string_number_to_digit_number(char c)
{
	int v = 0;
	switch(c){
	case '0':
		v = 0;
		break;

	case '1':
		v = 1;

		break;
	case '2':
		v = 2;
		break;

	case '3':
		v = 3;
		break;

	case '4':
		v = 4;
		break;

	case '5':
		v = 5;
		break;

	case '6':
		v = 6;
		break;

	case '7':
		v = 7;
		break;

	case '8':
		v = 8;
		break;

	case '9':
		v = 9;
		break;

	default:
		v = 0;
		break;
	}
	return v;
}

int convert_to_fpga_text_val(float val)
{
	int fpga_val = 0;
	char txt[100];
	sprintf(txt, "%f", val); // "100.000"  "10.000"  "1.000"

	char* ptr_dot = strstr(txt, ".");
	if(ptr_dot != NULL)
	{
		*ptr_dot = 0;
		ptr_dot += 1;
	}

	char* ptr = txt;
	int len = strlen(txt);

	if(len >= 3)
	{
		int v = string_number_to_digit_number(ptr[0]);
		fpga_val |= (v << 12);
		ptr += 1;
	}

	if(len >= 2)
	{
		int v = string_number_to_digit_number(ptr[0]);
		fpga_val |= (v << 8);
		ptr += 1;
	}

	if(len >= 1)
	{
		int v = string_number_to_digit_number(ptr[0]);
		fpga_val |= (v << 4);
		ptr += 1;
	}

	if(ptr_dot != NULL)
	{
		int v = string_number_to_digit_number(ptr_dot[0]);
		fpga_val |= v;
		//ptr += 1;
	}

	return fpga_val;
}

void set_fpga_text_y(int channel, int y)
{
	if(channel == 1 || channel == 3)
	{
		client_pg_writeReg(0x45, y);
	}
	else
	{
		
	}
}

void set_fpga_text_x(int channel, int x)
{
	if(channel == 1 || channel == 3)
	{
		client_pg_writeReg(0x44, x);
	}
	else
	{
		
	}
}

void set_fpga_otp_y(int channel, int y)
{
	if(channel == 1 || channel == 3)
	{
		client_pg_writeReg(0x4A, y);
	}
	else
	{
		
	}
}

void set_fpga_otp_x(int channel, int x)
{
	if(channel == 1 || channel == 3)
	{
		client_pg_writeReg(0x49, x);
	}
	else
	{
		
	}
}

void set_fpga_text_color(int channel, int red)
{
	//FPGA REG42~REG43
	// Flick??Vcom??OTP???????????
	// ?????????????????30bits???????????????
	// FPGA_DDR_REG42???16bits???????FPGA_DDR_REG1F???14bits???????
	// Bit29~bit20?????????????????��0~1023??
	// Bit19~bit10?????????????????��0~1023??
	// Bit9~bit0?????????????????��0~1023??

	if(red)
	{
		if(channel == 1 || channel == 3) //1,3
		{
			client_pg_writeReg(0x42, 0x3ff);
			client_pg_writeReg(0x43, 0);
		}
		else //2,4
		{
			client_pg_writeReg(0x4B, 0x3ff);
			client_pg_writeReg(0x4C, 0);
		}
	}
	else
	{
		if(channel == 1 || channel == 3) //1,3
		{
			client_pg_writeReg(0x42, 0);
			client_pg_writeReg(0x43, 0x3fc0);
		}
		else //2,4
		{
			client_pg_writeReg(0x4B, 0);
			client_pg_writeReg(0x4C, 0x3fc0);
		}
	}
}

void set_fpga_text_show(int vcom, int otp, int ok, int ok2)
{
	int ret = 0;
	if(vcom) //VCOM, FLICK
	{
		ret |= 0x01; //001  Bit0???Flick,Vcom??????????
	}
	
	if(otp) //OTP
	{
		ret |= 0x02; //010  Bit1???OTP??????????
	}
	
	if(ok)
	{
		ret |= 0x04; //100  Bit2?OTP flag????????????OK?????????NG??
	}
	else
	{
	}
	
	if(ok2) //???2
	{
		ret |= 0x08; //1000  Bit3?OTP flag????????????OK?????????NG??
	}
	else
	{
	}

	client_pg_writeReg(0x46, ret);
}

void set_fpga_text_vcom(int channel, int vcom)
{
	int val = 0;
	if(vcom > 0)
	{
		float v = (float)vcom;
		val = convert_to_fpga_text_val(v);
	}
	if(channel == 1 || channel == 3)
	{
		client_pg_writeReg(0x41, val);
	}
	else
	{
		client_pg_writeReg(0x4E, val);
	}
}

void set_fpga_text_flick(int channel, int flick)
{
	int val = 0;
	if(flick > 0)
	{
		float v = (float)flick;
		v = v / 100.0;
		val = convert_to_fpga_text_val(v);
	}
	if(channel == 1 || channel == 3)
	{
		client_pg_writeReg(0x40, val);
	}
	else
	{
		client_pg_writeReg(0x4D, val);
	}
}

