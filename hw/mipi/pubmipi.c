#include "stdio.h"
#include "string.h"
#include "hi_unf_spi.h"
#include "mipi2828.h"
#include "pubmipi.h"
#include "pubtiming.h"
#include "util/debug.h"
#include "fpgaRegister.h"
#include "MsOS.h"
#include "vcom.h"

#ifdef USED_IC_MANAGER
#include "icm/ic_manager.h"
#endif


int g_chip_helitai_8019 = 0;

int chip_set_helitai_8019_status(int enable)
{
	g_chip_helitai_8019 = enable;
	return 0;
}

int chip_get_helitai_8019_status()
{
	return g_chip_helitai_8019;
}

int chip_is_helitai_8019()
{
	return g_chip_helitai_8019;
}

#define INITCODE_PATH "cfg/initcode/"
#define MTPCODE_PATH  "cfg/mtpcode/"

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

static int split(char dst[][16], char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    while( result != NULL )
    {
        if(result[0]!='\r' && result[0]!='\n')
        {
            memset(dst[n],0,16);
            strcpy(dst[n++], result);
        }
        result = strtok(NULL, spl);
    }
    return n;
}

int z_parse_init_code(char* p_init_code_name)
{
	vcom_info_t vcom_info = { 0 };
	char initCodePath[128];
	sprintf(initCodePath,"%s/%s", INITCODE_PATH, p_init_code_name);
	memset(InitCode,0xff,sizeof(InitCode));
	mipi_parse_init_code(initCodePath, InitCode, vcom_info);
	return 0;
}

void mipi_write_init_code(char *pName,unsigned char *code,int len)
{
    FILE* file;
    file=fopen(pName,"w");
    if(!file)
    {
        printf("write mipi code faild\n");
        return;
    }
    fwrite(code,1,len,file);
    fclose(file);
}

static void replace_enter(char * line)
{
    char *p = line;
    while ((*p == '\r') || (*p == '\n'))
    {
        *p='\0';
    }
}

void mipi_parse_init_code(char *pName,unsigned char *code,vcom_info_t *vcom_info)
{
    FILE* file;
    file=fopen(pName,"r");
    if(!file)
    {
        printf("pls check file is exist?\n");
        return;
    }
	int id_filter = 0;
	int vcom_filter = 0;
	int gamma_filter = 0;
	int code_filter = 0;
    char buf[200];
    char dst[256][16];
    int count=0;
    int codeByte = 0;
    int i,j,readLen;
	
    memset(buf,0,sizeof(buf));
	
    while(fgets(buf,200,file))
    {
		if(buf[0] == ';')
		{
		
			if(strstr(buf, "**GAMMA**"))
			{
				if(vcom_info != NULL && vcom_info->gammaBurner > 0)
				{
					gamma_filter = 1;
				}
			}
			else if(strstr(buf, "**GAMMA END**"))
			{
				gamma_filter = 0;
			}
			else if(strstr(buf, "**ID**"))
			{
				if(vcom_info != NULL && vcom_info->idBurner > 0)
				{
					id_filter = 1;
				}
			}
			else if(strstr(buf, "**ID END**"))
			{
				id_filter = 0;
			}
			else if(strstr(buf, "**VCOM**"))
			{
				if(vcom_info != NULL && vcom_info->vcomBurner > 0)
				{
					vcom_filter = 1;
				}
			}
			else if(strstr(buf, "**VCOM END**"))
			{
				vcom_filter = 0;
			}
			else if(strstr(buf, "**CODE**"))
			{
				if(vcom_info != NULL && vcom_info->codeBurner > 0)
				{
					code_filter = 1;
				}
			}
			else if(strstr(buf, "**CODE END**"))
			{
				code_filter = 0;
			}
			else
			{
				
			}
			
			memset(buf,0,sizeof(buf));
			continue;
		}
		else if(buf[0] == '\r' || buf[0] == '\n')
		{
			memset(buf,0,sizeof(buf));
			continue;
		}
		else if(buf[0] == '/' || buf[1] == '/') //comment, read next line.
		{
			memset(buf,0,sizeof(buf));
			continue;
		}
		else if(buf[0] == '#') //comment, read next line.
		{
			memset(buf,0,sizeof(buf));
			continue;
		}
		
		//����������
		if(id_filter || vcom_filter || gamma_filter || code_filter)
		{
			memset(buf,0,sizeof(buf));
			continue;
		}
		
		if(1)
		{
			char* strTip = strstr(buf, ";");
			if(strTip != NULL)
			{
				*strTip = 0;
			}
		}

		if(1)
		{
			char* strTip = strstr(buf, "//");
			if(strTip != NULL)
			{
				*strTip = 0;
			}
		}
		
		if(1)
		{
			char* strTip = strstr(buf, "#");
			if(strTip != NULL)
			{
				*strTip = 0;
			}
		}
		
		if(1)
		{
			int strLen = strlen(buf);
			int strLenOrg = strLen;
			if(strLen > 0)
			{
				for(i=strLen-1; i >= 0; i--)
				{
					if(buf[i] == ' ' || buf[i] == '\r' || buf[i] == '\n')
					{
						buf[i] = 0;
						strLenOrg -= 1;
					}
					else
					{
						break;
					}
				}
			}
			
			if(strLen == 0 || strLenOrg == 0)
			{
				memset(buf,0,sizeof(buf));
				continue;
			}
		}
		
        count = split(dst,buf," ");
        
        if(strncmp(dst[0],"channel",7) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xFe;
            code[codeByte+2] = 1;
            code[codeByte+3] = strtol(dst[1],0,0);
            codeByte+=4;
        }
		else if(strncmp(dst[0],"chiptype",8) == 0) //chiptype otm8019a
		{
		}
        else if(strncmp(dst[0],"2828read",8) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xFd;
            code[codeByte+2] = 1+2; //reg+value
            code[codeByte+3] = strtol(dst[1],0,16);
            code[codeByte+4] = 0;
            code[codeByte+5] = 0;
            codeByte += 6;
        }
        else if(strncmp(dst[0],"2828write",9) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xFc;
            code[codeByte+2] = 1+2; //reg+value
            code[codeByte+3] = strtol(dst[1],0,16);
            code[codeByte+4] = strtol(dst[2],0,16)>>8;
            code[codeByte+5] = strtol(dst[2],0,16)&0xFF;
            codeByte += 6;
        }
        else if(strncmp(dst[0],"vcom_ili9881",12) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xE0;
            code[codeByte+2] = 0; //reg+value
            codeByte += 3;
        }
        else if(strncmp(dst[0],"otp_ili9881",10) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xE1;
            code[codeByte+2] = 0; //reg+value
            codeByte += 3;
        }
        else if(strncmp(dst[0],"otp_check",9) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xE2;
            code[codeByte+2] = 0; //reg+value
            codeByte += 3;
        }
        else if(strncmp(dst[0],"hs",2) == 0)
        {
            if(count == 3)
            {
                if(strncmp(dst[1],"mode",4) == 0)
                {
                    if(strncmp(dst[2],"on",2) == 0)
                    {
                        code[codeByte]   = 0x0;
                        code[codeByte+1] = 0xFb;
                        code[codeByte+2] = 0;
                        codeByte += 3;
                    }
                    else if(strncmp(dst[2],"off",3) == 0)
                    {
                        code[codeByte]   = 0x0;
                        code[codeByte+1] = 0xFa;
                        code[codeByte+2] = 0;
                        codeByte += 3;
                    }
                }
            }
        }
        else if(strncmp(dst[0],"mtp",3) == 0)
        {
            if(count == 3)
            {
                if(strncmp(dst[1],"power",5) == 0)
                {
                    if(strncmp(dst[2],"on",2) == 0)
                    {
                        code[codeByte]   = 0x0;
                        code[codeByte+1] = 0xF9;
                        code[codeByte+2] = 0;
                        codeByte += 3;
                    }
                    else if(strncmp(dst[2],"off",3) == 0)
                    {
                        code[codeByte]   = 0x0;
                        code[codeByte+1] = 0xF8;
                        code[codeByte+2] = 0;
                        codeByte += 3;
                    }
                    else
                    {
                        printf("dst[2]len:%d-->%s\n",strlen(dst[2]),dst[2]);
                    }
                }
            }
        }
        else if(strncmp(dst[0],"reset",5) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xF7;
            code[codeByte+2] = 0;
            codeByte += 3;
        }
        else if(strncmp(dst[0],"RGB",3) == 0)
        {
            code[codeByte]   = 0x0;
            code[codeByte+1] = 0xF6;
            code[codeByte+2] = 3; //reg+value
            code[codeByte+3] = strtol(dst[1],0,10);
            code[codeByte+4] = strtol(dst[2],0,10);
            code[codeByte+5] = strtol(dst[3],0,10);
            codeByte += 6;
        }
        else if(strncmp(dst[0],"delay",5) == 0)
        {
            code[codeByte]   = 0;
            code[codeByte+1] = 0;
            code[codeByte+2] = 2;
            code[codeByte+3] = strtol(dst[1],0,0)&0xFF;
            code[codeByte+4] = strtol(dst[1],0,0)>>8;
            codeByte += 5;
        }
        else if(strncmp(dst[0],"dcsread",7) == 0)//0x06
        {
            code[codeByte]   = 0x06;
            code[codeByte+1] = strtol(dst[1],0,16);//reg
            readLen = code[codeByte+3] = strtol(dst[2],0,16);//len
            code[codeByte+2] = 1 +readLen;
            for(i=0;i<readLen;i++)
            {
                code[codeByte+4+i] = 0;
            } 
            if(readLen>0)
            {
                if(strncmp(dst[3],"curflick",8) == 0)
                {
                    printf("!!!!!!!!!!!!%s\n",dst[3]);
                    code[codeByte+4] = 0xaa; //define cur_flick value ==  0xaa
                }
                if(strncmp(dst[3],"curpolar",8) == 0)
                {
                    printf("!!!!!!!!!!!!%s\n",dst[3]);
                    code[codeByte+4] = 0xab; //define curpolar value ==  0xab
                }
            }
            codeByte += 4 + readLen;
        }
        else if(strncmp(dst[0],"genread",7) == 0)//0x14
        {
            code[codeByte]   = 0x14;
            code[codeByte+1] = strtol(dst[1],0,16);//reg
            readLen = code[codeByte+3] = strtol(dst[2],0,16);//len
            code[codeByte+2] = 1 +readLen;
            for(i=0;i<readLen;i++)
            {
                code[codeByte+4+i] = 0;
            }
            codeByte += 4 + readLen;
        }
        else if(strcmp(dst[0],"05") == 0)
        {
            code[codeByte]   = 0x05;
            code[codeByte+1] = strtol(dst[1],0,16);
            code[codeByte+2] = count-2;
            for(i=0;i<count-2;i++)
            {
                code[codeByte+3+i] = strtol(dst[i+2],0,16);
            }
            codeByte += 3 + count -2;
        }
        else if(strcmp(dst[0],"29") == 0)
        {
            code[codeByte]   = 0x29;
            code[codeByte+1] = strtol(dst[1],0,16);
            code[codeByte+2] = count-2;
            for(i=0;i<count-2;i++)
            {
                code[codeByte+3+i] = strtol(dst[i+2],0,16);
            }
            codeByte += 3 + count -2;
        }
        else if(strcmp(dst[0],"39") == 0)
        {
            code[codeByte]   = 0x39;
            code[codeByte+1] = strtol(dst[1],0,16);
            code[codeByte+2] = count-2;
            for(i=0;i<count-2;i++)
            {
                code[codeByte+3+i] = strtol(dst[i+2],0,16);
            }
            codeByte += 3 + count -2;
        }
        else if(strcmp(dst[0],"15") == 0)
        {
            code[codeByte]   = 0x15;
            code[codeByte+1] = strtol(dst[1],0,16);
            code[codeByte+2] = count-2;
            for(i=0;i<count-2;i++)
            {
                code[codeByte+3+i] = strtol(dst[i+2],0,16);
            }
            codeByte += 3 + count -2;
        }
        else if(strcmp(dst[0],"23") == 0)
        {
            code[codeByte]   = 0x23;
            code[codeByte+1] = strtol(dst[1],0,16);
            code[codeByte+2] = count-2;
            for(i=0;i<count-2;i++)
            {
                code[codeByte+3+i] = strtol(dst[i+2],0,16);
            }
            codeByte += 3 + count -2;
        }
        else if(dst[0][0]='R')
        {
            if(count == 1) //no param
            {
                code[codeByte] = 0x05;
            }
            else
            {
                code[codeByte] = 0x39;
            }

            code[codeByte+1] = strtol(&dst[0][1],0,16);
            code[codeByte+2] = count -1;
            for(i=0;i<count-1;i++)
            {
                code[codeByte+3+i] = strtol(dst[i+1],0,16);
            }
            codeByte += 3 + count -1;
        }
        else
        {
            printf("unknow code:%s\n",buf);
        }
        memset(buf,0,sizeof(buf));
    }
    fclose(file);
}

/*


;*********AVDD*****************

;*********AVDD END*************

;*********VGH*****************


;*********VGH END*************

;*********VGL*****************

;*********VGL END*************

;*********TIMING***************
R00 80           ;TCON Setting
RC0 00 64 00 0A 12 00 64 0A 12  ;vfp 10 vbp 18

R00 90           ;Panel Timing Setting
RC0 00 5C 00 01 00 04

R00 A4           ;Panel Timing Setting
RC0 00

R00 B3           ;Panel Driving Mode: Column Inversion
RC0 00 50
;**********TIMING END**********

;**********GAMMA***************


;**********GAMMA END***********

;**********PIEX REVERSAL*******


;**********PIEX REVERSAL END***

;**********FLICKER*************


;**********FLICKER END*********

;**********RGB*****************


;**********RGB END*************
*/

void mipi_reset(int channel)
{
	if(channel == 1 || channel == 2) //CH1,  4, 3
	{
		HI_UNF_GPIO_WriteBit(10,0);  //mipi pull low
		usleep(10*1000);
		HI_UNF_GPIO_WriteBit(10,1);  //mipi pull high
		usleep(100);
	}
	else //CH2, 3, 4
	{
		HI_UNF_GPIO_WriteBit(11,0);  //mipi pull low
		usleep(10*1000);
		HI_UNF_GPIO_WriteBit(11,1);  //mipi pull high
		usleep(100);
	}
}

void mipi_gpio_reset(int n)
{
    //HI_UNF_GPIO_Init(); xujie

    if(n == 0)
    {
        HI_UNF_GPIO_Open(74);
        HI_UNF_GPIO_SetDirBit(74,0); //output
        HI_UNF_GPIO_WriteBit(74,0);
        usleep(10*1000);
        HI_UNF_GPIO_WriteBit(74,1);

        HI_UNF_GPIO_Open(10);
        HI_UNF_GPIO_SetDirBit(10,0); //output
        HI_UNF_GPIO_WriteBit(10,0);  //mipi pull low
        usleep(10*1000);
        HI_UNF_GPIO_WriteBit(10,1);  //mipi pull high

		//2017-5-26 ��
        usleep(100);
        HI_UNF_GPIO_WriteBit(10,0);  //mipi pull low
        usleep(10*1000);
        HI_UNF_GPIO_WriteBit(10,1);  //mipi pull high
    }
    else
    {
        HI_UNF_GPIO_Open(75);
        HI_UNF_GPIO_SetDirBit(75,0); //output
        HI_UNF_GPIO_WriteBit(75,0);
        usleep(10*1000);
        HI_UNF_GPIO_WriteBit(75,1);

        HI_UNF_GPIO_Open(11);
        HI_UNF_GPIO_SetDirBit(11,0); //output
        HI_UNF_GPIO_WriteBit(11,0);  //mipi pull low
        usleep(10*1000);
        HI_UNF_GPIO_WriteBit(11,1);  //mipi pull high

		//2017-5-26 ��
        usleep(100);
        HI_UNF_GPIO_WriteBit(11,0);  //mipi pull low
        usleep(10*1000);
        HI_UNF_GPIO_WriteBit(11,1);  //mipi pull high
    }
}


void mipi_gpio_release()
{
    //HI_UNF_GPIO_Init(); xujie

    HI_UNF_GPIO_Open(74);
    HI_UNF_GPIO_SetDirBit(74,0); //output
    HI_UNF_GPIO_WriteBit(74,0);

    HI_UNF_GPIO_Open(10);
    HI_UNF_GPIO_SetDirBit(10,0); //output
    HI_UNF_GPIO_WriteBit(10,0);  //mipi pull low

    HI_UNF_GPIO_Open(75);
    HI_UNF_GPIO_SetDirBit(75,0); //output
    HI_UNF_GPIO_WriteBit(75,0);

    HI_UNF_GPIO_Open(11);
    HI_UNF_GPIO_SetDirBit(11,0); //output
    HI_UNF_GPIO_WriteBit(11,0);  //mipi pull low
}

void mipi_timing_setting(timing_info_t *timing_info)
{
    VsCount  = timing_info->timvsyncpulsewidth;//4;
    VBPCount = timing_info->timvbackporch;//4;
    VFPCount = timing_info->timvfrontporch;//4;
    VDPCount = timing_info->timvactivetime;//800;
    VTotal   = timing_info->timvtotaltime;// = 1954;//1235;

    HsCount  = timing_info->timhsyncpulsewidth;//4;//16;//20;
    HBPCount = timing_info->timhbackporch;//48;//15;
    HFPCount = timing_info->timhfrontporch;//16;//200;
    HDPCount = timing_info->timhactivetime;//1280;
    HTotal   = timing_info->timhtotaltime;//880;// = 1294;//2080;

    LaneNum  = timing_info->timmipilanenum; //4
    SyncMode = timing_info->timmipisyncmode; //1 0:pulse mode 1: event mode 2:burst
    DSIFRE = timing_info->mdlmipidsiclock;//4800000000;    //hz
    BitNum = timing_info->timbit;   //6bit 8bit 10bit

//    SPINUM = 0;
//    Lane8Flag = 0;
 //   Channel   = 0;
}

/**
 * @brief isReturnData
 * @param code
 * @return
 *
 * this is attempt that save cmd of genread data
 */
int isReturnData(uint8_t *code)
{
    uint16_t CodePoint;
    CodePoint = 0;

    if( code == 0 ) return 0;

    printf("this is isReturnData parse fun [0x%x]\n",code[CodePoint + 0]);

    int i;
    for(i = 0; i < MAX_MIPI_CODE_LEN; i++)
    {
        if(code[CodePoint + 2] <= 250)
        {
            if(code[CodePoint + 0] == 0x14)   //gernal read
            {
                printf("isReturnData parse cmd 0x14 that is genread\n");
                int len = code[CodePoint+3];
                FILE *pfile = fopen("tmpgenreadcmddata.txt","wb+");
                if( pfile != 0)
                {
                    fwrite(&code[CodePoint+4],len,1,pfile);
                    fclose(pfile);
                }
                return 0x14;
            }

            CodePoint += code[CodePoint + 2] + 3;
        }
    }

    return 0;
}

int mipi_worked(char *pMipiCode,int mipiLen)
{
    if(pMipiCode)
    {
        memset(InitCode,0xff,sizeof(InitCode));
        //testMipidCode(pMipiCode,mipiLen);
        mipi_write_init_code("/tmp/mipi",pMipiCode,mipiLen);
        mipi_parse_init_code("/tmp/mipi",InitCode,NULL);
        SendCode(1,InitCode);
		SendCode(2,InitCode);//xujie
        return 0;//isReturnData(InitCode);
    }

    return -1;
}

//uint8_t InitCodeAll[MAX_MIPI_CODE_LEN]; //xujie
int mipi_working(timing_info_t *timing_info,char *pMipiCode,int mipiLen, vcom_info_t* vcom_info)
{

    mipi_timing_setting(timing_info);

    if(pMipiCode)
    {
        memset(InitCode,0xff,sizeof(InitCode));
        mipi_write_init_code("/tmp/mipi",pMipiCode,mipiLen);
        mipi_parse_init_code("/tmp/mipi",InitCode,vcom_info);
    }
    else
    {
        if(strlen(timing_info->timinitcodename))
        {
            char initCodePath[128];
            sprintf(initCodePath,"%s/%s",INITCODE_PATH,timing_info->timinitcodename);
            memset(InitCode,0xff,sizeof(InitCode));
            mipi_parse_init_code(initCodePath, InitCode, NULL/*vcom_info*/); //�������κ�CODE
        }
    }

    setHsmode(0);
    mipi_gpio_reset(0);
    mipi_gpio_reset(1);
	
	unsigned short sB0Value = ReadSSD2828Reg(SPI1_CS_MIPI1, 0xb0);
	printf("sB0Value1:%d\n",sB0Value);
	
	unsigned short sB0Value2 = ReadSSD2828Reg(SPI1_CS_MIPI2, 0xb0);
	printf("sB0Value2:%d\n",sB0Value2);

	unsigned short sB0Value3 = ReadSSD2828Reg(SPI2_CS_MIPI3, 0xb0);
	printf("sB0Value3:%d\n",sB0Value3);

	unsigned short sB0Value4 = ReadSSD2828Reg(SPI2_CS_MIPI4, 0xb0);
	printf("sB0Value4:%d\n",sB0Value4);

    if(timing_info->timmipicommandenable)
    {
		//printf("----+++++11111111111\n");
        //Set2828_CMD(1);
    }
    else
    {
		//printf("---------12121212121\n");
        //Set2828_V(1); //helitai_8019A �õ������
        Set2828_V(SPI_ALL_CS);
    }
	
	// select mipi channel
	ic_mgr_set_mipi_channel(0, SPI1_CS_MIPI1);
	ic_mgr_set_mipi_channel(1, SPI2_CS_MIPI4);
}

void mipi_close()
{
	ic_mgr_reset_mipi_channel(0, SPI1_CS_MIPI1);
	ic_mgr_reset_mipi_channel(1, SPI2_CS_MIPI4);
	
    setHsmode(0);
    sendCloseCommand();
    mipi_gpio_release();
}




