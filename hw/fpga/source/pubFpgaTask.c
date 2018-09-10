#include "pgos/MsOS.h"
#include "common.h"
#include "comStruct.h"
#include "pubFpga.h"
#include "fpgaFunc.h"
#include "pgDB/pgDB.h"

static MS_APARTMENT *fpgaApartMent = 0;

MS_U32 fpga_message_queue_get()
{
    return fpgaApartMent->MessageQueue;
}

int fpga_show_color_picture(unsigned char r, unsigned char g, unsigned char b)
{
	showRgbStr showRgb;
	showRgb.rgb_r = r;
	showRgb.rgb_g = g;
	showRgb.rgb_b = b;
	unsigned int u32QueueId = fpga_message_queue_get();
	MsOS_SendMessage(u32QueueId, FPGA_CMD_SHOW_RGB, &showRgb, 0);
	return 0;
}


MS_U32 fpga_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message)
{
    int ret = 0;
    switch(Message.MessageID)
    {
        case FPGA_CMD_SET_PHOTO:  // 0x01.切换图片
        {
            showPictureInfo_t showPic;
            INT version,width,height,ucPosition,dwPicSize;
            INT ptnId;
            ucPosition = 4;
            //ret = fpga_reg_dev_ioctl(eGET_FPGA_VER, (unsigned long)&version);
            char ptnImageName[256];

            ptnId = Message.Parameter1;
            module_info_t *pModuleInfo = dbModuleGetPatternById(ptnId);
            if(!pModuleInfo)
            {
                printf("!!!!!!!!!!pls notice err ptnId is %d\n",ptnId);
                return -1;
            }
			
            sprintf(ptnImageName,"./cfg/imageOrigin/%s/%s.bmp",pModuleInfo->displayX,pModuleInfo->ptdpatternname);
            //printf("FPGA_CMD_SET_PHOTO %d %s\n",Message.Parameter1,ptnImageName);
            if(access(ptnImageName , F_OK)!=0)
            {
                printf("!image is not exist:%s\n",ptnImageName);
                return -1;
            }

            //printf("pModuleInfo->displayX %s\n",pModuleInfo->displayX);
            sscanf(pModuleInfo->displayX,"%dX%d",&width,&height);

            printf("width %d height %d\n",width,height);
            dwPicSize = width*height*4/1024/1024; //0x0e;

            ucPosition = move_ptn_getPostionByName(ptnImageName);
            //printf("ptnId is %d::postion:%d name:%s\n",ptnId,ucPosition,ptnImageName);
            if(ucPosition == -1)
            {
                ucPosition = move_ptn_download(ptnImageName,width,height,0);
            }
#if 0
            fpgaDdrBuf ddrBuf;
            ddrBuf.ddr  = 0;
            ddrBuf.last = 0;
            ddrBuf.buf  = 0;
            ddrBuf.fileLong = 0;
            ddrBuf.hResolution = width;
            ddrBuf.vResolution = height;
            ddrBuf.picSize = dwPicSize;
            ddrBuf.picNo   = ucPosition;
            ddrBuf.picType = 0;
            if (0 != fpga_reg_dev_ioctl(eDOWN_PICTURE, (unsigned long *)&ddrBuf))
            {
                traceMsg(FPGA_PRT_SW1, "fpga_reg_dev_ioctl error.\n");
                return -2;
            }
            fpga_trans_pic_data(ptnImageName);
            usleep(50 * 1000);
#endif
            // show jpg
            showPic.position   = ucPosition;
            showPic.size = dwPicSize;
            //printf( "showPic.picSize: %d, pos: %d\n", showPic.size, showPic.position);
            ret = fpga_reg_dev_ioctl(eSET_FPGA_SHOW_MODE, (unsigned long)&showPic);
        }
        break;

        case FPGA_CMD_SHOW_RGB:   // 0x02.RGB调节
        {
            showRgbStr   showRgb;
            showRgbStr *pshowRgb;
            pshowRgb = (showRgbStr *)Message.Parameter1;
            memcpy(&showRgb, pshowRgb, sizeof(showRgb));
            ret = fpga_reg_dev_ioctl(eSET_FPGA_RGB, (unsigned long)&showRgb);
        }
        break;

        case FPGA_CMD_SET_VESA_JEDA:  // 0x03.Vesa-jeda切换
        {
            vesaJedaSwitchInfo_t  *pVesaJeda = Message.Parameter1;
            ret = fpga_reg_dev_ioctl(eSET_VESA_JEDA, (unsigned long)pVesaJeda);
        }
        break;

        case FPGA_CMD_SET_CLOCK: //0x04.时钟设置
        {
            //not to do
        }
        break;

        case FPGA_CMD_GET_FPGA_VERSION: // 0x05.Fpga版本
        {
            INT version;
            ret = fpga_reg_dev_ioctl(eGET_FPGA_VER, (unsigned long)&version);
			if(Message.pResult != NULL) *Message.pResult = version;
			return version;
        }
        break;

        case FPGA_CMD_SET_LINK: // 0x06.设置FPGA Link数
        {
            INT link = Message.Parameter1;
            ret = fpga_reg_dev_ioctl(eSET_LINK, (unsigned long)&link);	// 设置Link数
        }
        break;

        case FPGA_CMD_SET_BIT:// 0x07.设置FPGA Bit数
        {
            INT bit = Message.Parameter1;
            traceMsg(FPGA_PRT_SW1, "fpgaSetBit bit = %d\n", bit);
            ret = fpga_reg_dev_ioctl(eSET_BIT, (unsigned long)&bit);	// 设置bit数
        }
        break;

        case FPGA_CMD_SET_SEQ:// 0x08.设置FPGA 时序
        {
            fpgaSeqStr  fpgaSeq;
            fpgaSeqStr *pfpgaSeq = (fpgaSeqStr*)Message.Parameter1;
            memcpy(&fpgaSeq, pfpgaSeq, sizeof(fpgaSeqStr));
            // 把频率信息赋给全局量，方便单幅图切频率使用
            ret = fpga_reg_dev_ioctl(eSET_SEQ, (unsigned long)&fpgaSeq);
        }
        break;

        case FPGA_CMD_SET_TEST_FILE:// 0x09.选择测试文件
        {
            //fpgacmd_SetTestFile();
        }
        break;

        case FPGA_CMD_PIC_MOVE:// 0x0a.图片移动，暂支持灰阶
        {
            sPtnMove *pPtnMoveBuf = (sPtnMove *)Message.Parameter1;
            ret = fpga_reg_dev_ioctl(ePIC_MOV, (unsigned long)pPtnMoveBuf);
        }
        break;

        case FPGA_CMD_SET_REG:// 0x0b.设置寄存器的值
        {
            fpgaRegStr writereg;
            writereg.ofset = Message.Parameter1;
            writereg.value = Message.Parameter2;
            ret = fpga_reg_dev_ioctl(eSET_FPGA_REG, (unsigned long)&writereg);
        }
        break;

        case FPGA_CMD_READ_REG:// 0x0c.设置寄存器的值
        {
            fpgaRegStr readreg;
            readreg.ofset = Message.Parameter1;  //?
            readreg.value = 0x0;
            ret = fpga_reg_dev_ioctl(eREADD_FPGA_REG, (unsigned long)&readreg);
			if(Message.pResult != NULL) *Message.pResult = ret; //xujie
			return ret; //xujie
        }
        break;

        case FPGA_CMD_SET_CROSS_CURSOR:// 0x0d.十字光标设置
        {
            crossCursorStr myCrossCursor = {0};
            CrossCursorStateStr *crossCursorState = NULL;
            WORD16 wordColorRed = 0, wordColorGreen = 0, wordColorBlue = 0;
            WORD16 crossCursorColorRed = 0, crossCursorColorGreen = 0, crossCursorColorBlue = 0;
            BYTE RGBchang = 0, HVflag = 0;
            crossCursorState = (CrossCursorStateStr *) Message.Parameter1;
            myCrossCursor.enable = crossCursorState->enable;
            RGBchang = crossCursorState->RGBchang;
            myCrossCursor.startCoordinate = crossCursorState->startCoordinate;
            HVflag = crossCursorState->HVflag;
            // debug的时候用 从0开始
            //myCrossCursor.startCoordinate = 0;
            crossCursorColorRed = (crossCursorState->crossCursorColorRed) & 0xFF;
            crossCursorColorGreen = (crossCursorState->crossCursorColorGreen) & 0xFF;
            crossCursorColorBlue = (crossCursorState->crossCursorColorBlue) & 0xFF;

            switch (RGBchang)
            {
                case userCrossCursorAll:
                    break;
					
                case userCrossCursorR:
                    //crossCursorColorRed = (~crossCursorColorRed)&0xFF;
                    crossCursorColorGreen = (~crossCursorColorGreen) & 0xFF;
                    crossCursorColorBlue = (~crossCursorColorBlue) & 0xFF;
                    break;
					
                case userCrossCursorG:
                    crossCursorColorRed = (~crossCursorColorRed) & 0xFF;

                    INT ret = SUCCESS;
                    crossCursorStr myCrossCursor = {0};
                    CrossCursorStateStr *crossCursorState = NULL;
                    WORD16 wordColorRed = 0, wordColorGreen = 0, wordColorBlue = 0;
                    WORD16 crossCursorColorRed = 0, crossCursorColorGreen = 0, crossCursorColorBlue = 0;
                    BYTE RGBchang = 0, HVflag = 0;

                    crossCursorState = (CrossCursorStateStr *) Message.Parameter1;
                    myCrossCursor.enable = crossCursorState->enable;
                    RGBchang = crossCursorState->RGBchang;
                    myCrossCursor.startCoordinate = crossCursorState->startCoordinate;
                    HVflag = crossCursorState->HVflag;

                    crossCursorColorRed = (crossCursorState->crossCursorColorRed) & 0xFF;
                    crossCursorColorGreen = (crossCursorState->crossCursorColorGreen) & 0xFF;
                    crossCursorColorBlue = (crossCursorState->crossCursorColorBlue) & 0xFF;

                    switch (RGBchang)
                    {
                        case userCrossCursorAll:
                            ;
                            break;
                        case userCrossCursorR:
                            //crossCursorColorRed = (~crossCursorColorRed)&0xFF;
                            crossCursorColorGreen = (~crossCursorColorGreen) & 0xFF;
                            crossCursorColorBlue = (~crossCursorColorBlue) & 0xFF;
                            break;
                        case userCrossCursorG:
                            crossCursorColorRed = (~crossCursorColorRed) & 0xFF;
                            //crossCursorColorGreen = (~crossCursorColorGreen)&0xFF;
                            crossCursorColorBlue = (~crossCursorColorBlue) & 0xFF;
                            break;
                        case userCrossCursorB:
                            crossCursorColorRed = (~crossCursorColorRed) & 0xFF;
                            crossCursorColorGreen = (~crossCursorColorGreen) & 0xFF;
                            //crossCursorColorBlue = (~crossCursorColorBlue)&0xFF;
                            break;
                        default:
                            ;
                            break;
                    }

                    wordColorRed = (~crossCursorColorRed) & 0xFF;
                    wordColorGreen = (~wordColorGreen) & 0xFF;
                    wordColorBlue = (~crossCursorColorBlue) & 0xFF;

                    myCrossCursor.crossCursorColor = (crossCursorColorRed << 2) | (crossCursorColorGreen << 12) | (crossCursorColorBlue << 22);
                    myCrossCursor.wordColor =	(wordColorRed << 2) | (wordColorGreen << 12) | (wordColorBlue << 22);
                    myCrossCursor.x = crossCursorState->x;
                    myCrossCursor.y = crossCursorState->y;

                    myCrossCursor.RGBchang = crossCursorState->RGBchang;
                    ret = fpga_reg_dev_ioctl(eSET_CROSSCURSOR, (unsigned long)&myCrossCursor);
                    //crossCursorColorGreen = (~crossCursorColorGreen)&0xFF;
                    crossCursorColorBlue = (~crossCursorColorBlue) & 0xFF;
                    break;

                case userCrossCursorB:
                    crossCursorColorRed = (~crossCursorColorRed) & 0xFF;
                    crossCursorColorGreen = (~crossCursorColorGreen) & 0xFF;
                    break;

                default:
                    break;
            }

            wordColorRed = (~crossCursorColorRed) & 0xFF;
            wordColorGreen = (~wordColorGreen) & 0xFF;
            wordColorBlue = (~crossCursorColorBlue) & 0xFF;
                myCrossCursor.crossCursorColor = (crossCursorColorRed << 2) | (crossCursorColorGreen << 12) | (crossCursorColorBlue << 22);
                myCrossCursor.wordColor =	(wordColorRed << 2) | (wordColorGreen << 12) | (wordColorBlue << 22);
                myCrossCursor.x = crossCursorState->x;
                myCrossCursor.y = crossCursorState->y;
                myCrossCursor.RGBchang = crossCursorState->RGBchang;
                ret = fpga_reg_dev_ioctl(eSET_CROSSCURSOR, (unsigned long)&myCrossCursor);

            }
            break;

            case FPGA_CMD_SET_LINK_MODE:// 0x0e.设置link链接方式
            {
                signalModeStr signalMode;
                lvdsSignalModuleStr lvdsSignalModule;
                lvdsSignalModuleStr *plvdsSignalModule = (lvdsSignalModuleStr *)Message.Parameter1;
                memcpy(&lvdsSignalModule, plvdsSignalModule, sizeof(lvdsSignalModule));

                switch (lvdsSignalModule.linkCount)
                {
                    case 2:
                        signalMode.OddEven = lvdsSignalModule.module[0] - 1;
                        signalMode.MasterSlave = lvdsSignalModule.module[1] - 1;
                        signalMode.link2 = lvdsSignalModule.module[0] - 1;
                        signalMode.link3 = lvdsSignalModule.module[1] - 1;
                        break;
                    default:
                        signalMode.OddEven = lvdsSignalModule.module[0] - 1;
                        signalMode.MasterSlave = lvdsSignalModule.module[1] - 1;
                        signalMode.link2 = lvdsSignalModule.module[2] - 1;
                        signalMode.link3 = lvdsSignalModule.module[3] - 1;
                        break;
                }

                traceMsg(FPGA_PRT_SW1, "~~link2 0x%x;link3 0x%x;MasterSlave 0x%x;OddEven 0x%x", signalMode.link2, \
                         signalMode.link3, signalMode.MasterSlave, signalMode.OddEven);
                ret = fpga_reg_dev_ioctl(eSET_SIGNAL_MODE, (unsigned long)&signalMode);
        }
        break;

        case FPGA_CMD_SET_LVDS_TEST:// 0x0f.LVDS开短路测试
        {
            //;
        }
        break;

        case FPGA_CMD_SET_NCLK:// 0x10.nclk功能
        {
            INT enableClock = *(INT*)Message.Parameter1;
            ret = fpga_reg_dev_ioctl(eFPGA_CLOCK_ENABLE, (unsigned long)&enableClock);
        }
        break;

        case FPGA_CMD_SET_RGB_ONOFF:   // 0X11.RGB开关
        {
            disRGBStr *pDisRGB;
            pDisRGB = (disRGBStr *) Message.Parameter1;
            traceMsg(FPGA_PRT_SW1, "pDisRGB->disR = %d\r\n ", pDisRGB->disR);
            traceMsg(FPGA_PRT_SW1, "pDisRGB->disG = %d\r\n ", pDisRGB->disG);
            traceMsg(FPGA_PRT_SW1, "pDisRGB->disB = %d\r\n ", pDisRGB->disB);
            ret = fpga_reg_dev_ioctl(eDISABLE_RGB, (unsigned long)pDisRGB);
        }
        break;

        case FPGA_CMD_SET_PHOTO_INDEX: // 0x12.按照序号来切图
        {
            //
        }
        break;

        case FPGA_CMD_SET_FREQ_BY_REFRESH:// 0x13.按照屏幕刷新频率设置频率
        {
            WORD32     freq;
            fpgaSeqStr *pfpgaSeq;
            sFpgaSetFreqByRefresh *pFpgaSetFreqByRefresh;
            // 得到所需的刷新频率
            pFpgaSetFreqByRefresh = (sFpgaSetFreqByRefresh *) Message.Parameter1;
            // 算出实际的频率
            freq = (pFpgaSetFreqByRefresh->freq) * (pfpgaSeq->T3) * (pfpgaSeq->T7);
            traceMsg(FPGA_PRT_SW1, "freq = %d\r\n", freq);
            //;
        }
        break;

        case FPGA_CMD_ADJUST_RGB:// 0x14.色阶调节
        {
            rgbAdjustStr *pRGBAdjust;
            pRGBAdjust = (rgbAdjustStr *) Message.Parameter1;
            traceMsg(FPGA_PRT_SW1, "is enable RGB adjust (%d 1:enable 0:disable)\r\n",
                     pRGBAdjust->enable);
            traceMsg(FPGA_PRT_SW1, "R sign = %d, R value = %d\r\n", pRGBAdjust->isRPlus,
                     pRGBAdjust->RPlusVal);
            traceMsg(FPGA_PRT_SW1, "G sign = %d, G value = %d\r\n", pRGBAdjust->isGPlus,
                     pRGBAdjust->GPlusVal);
            traceMsg(FPGA_PRT_SW1, "B sign = %d, B value = %d\r\n", pRGBAdjust->isBPlus,
                     pRGBAdjust->BPlusVal);
            ret = fpga_reg_dev_ioctl(eGRAY_SCALE_ADJUST, (unsigned long)pRGBAdjust);
        }
        break;

        case FPGA_CMD_SET_IMAGE_OFFSET:// 0x15.图形偏移
        {
            imageOffsetStr *pImageOffset = (imageOffsetStr *) Message.Parameter1;
            ret = fpga_reg_dev_ioctl(eIMAGE_OFFSET, (unsigned long)pImageOffset);
        }
        break;

        case FPGA_CMD_MOVE_PTN:// 0x16.ARM手动完成图形移动
        {
            //fpgacmd_ArmMvPtn();
        }
        break;

        case FPGA_CMD_SYN_SIGNAL_LEVEL://0x17.同步信号电平设置
        {
            syncSingalLevelStr *pSyncSingalLevel = (syncSingalLevelStr *)Message.Parameter1;
            ret = fpga_reg_dev_ioctl(eSIGNAL_SYNC_LEVEL, (unsigned long)pSyncSingalLevel);
            //free(pSyncSingalLevel);
        }
        break;

        case FPGA_CMD_SET_3D_PIN:// 0x18.使能fpga3D引脚
        {
            fpga3DPinStr *pfpga3DPin = (fpga3DPinStr *)Message.Parameter1;
            ret = fpga_reg_dev_ioctl(eFPGA_3D_PIN, (unsigned long)pfpga3DPin);
        }
        break;

        case FPGA_CMD_SET_TPMODE:// 0x19.切换TP模式
        {
            #define FPGAVIDEOREG 0x3b
            fpgaRegStr reg;
            int type;

            if ((type == TP_PG_EDIDMODE) || (type == TP_PG_CUTMODE))
            {
                reg.ofset = FPGAVIDEOREG;
                reg.value = 0;
                ret = fpga_reg_dev_ioctl(eREADD_FPGA_REG, (unsigned long)&reg);

                if (type == TP_PG_EDIDMODE) //EDID方式 置高
                {
                    reg.value = reg.value | (0x01 << 10);
                }
                else if (type == TP_PG_CUTMODE) //裁剪方式  置低
                {
                    reg.value = reg.value & (~(0x01 << 10));
                }

                traceMsg(FPGA_PRT_SW1, "reg.offset :0x%4x -- reg.value :0x%4x \r\n", reg.ofset, reg.value);
                ret = fpga_reg_dev_ioctl(eSET_FPGA_REG, (unsigned long)&reg);
            }
            else if ((type == TP_PG_PICMODE) || (type == TP_PG_VIDEOMODE))
            {
                reg.ofset = FPGAVIDEOREG;
                reg.value = 0;
                ret = fpga_reg_dev_ioctl(eREADD_FPGA_REG, (unsigned long)&reg);

                if (type == TP_PG_PICMODE)
                {
                    reg.value = reg.value & (~(0x01));
                }
                else if (type == TP_PG_VIDEOMODE)
                {
                    reg.value = reg.value | (0x01);
                }

                traceMsg(FPGA_PRT_SW1, "reg.offset :0x%4x -- reg.value :0x%4x \r\n", reg.ofset, reg.value);
                ret = fpga_reg_dev_ioctl(eSET_FPGA_REG, (unsigned long)&reg);
            }
        }
        break;

        case FPGA_CMD_SET_FPGA_ONEREG:// 0x20.设置fpga的某个寄存器
        {
            fpgaRegStr *pReg = (fpgaRegStr *)Message.Parameter1;
            traceMsg(FPGA_PRT_SW1, "eCmdSettingFpgaReg\n");
            traceMsg(FPGA_PRT_SW1, "reg.offset :0x%4x -- reg.value :0x%4x \r\n", pReg->ofset, pReg->value);
            ret = fpga_reg_dev_ioctl(eSET_FPGA_REG, (unsigned long)pReg);
        }
        break;

        case FPGA_CMD_SET_EDID:// 0x21.设置EDID
        {
            fpgaRegStr reg;
            ret = 0;
            reg.ofset = 0x3b;
            reg.value = 0x0;
            ret = fpga_reg_dev_ioctl(eREADD_FPGA_REG, (unsigned long)&reg);
            reg.value = reg.value | (0x01 << 11); // 置高
            traceMsg(FPGA_PRT_SW1, "置高reg.offset :0x%4x -- reg.value :0x%4x \r\n", reg.ofset, reg.value);
            ret = fpga_reg_dev_ioctl(eSET_FPGA_REG, (unsigned long)&reg);
            reg.value = reg.value & (~(0x01 << 11)); // 置低
            traceMsg(FPGA_PRT_SW1, "置低reg.offset :0x%4x -- reg.value :0x%4x \r\n", reg.ofset, reg.value);
            ret = fpga_reg_dev_ioctl(eSET_FPGA_REG, (unsigned long)&reg);
        }
        break;

    }
	
    return 0;
}

INT initFpga(void)
{
    INT nRet;
    MS_EVENT_HANDLER_LIST *pEventHandlerList = 0;
    traceMsg(FPGA_PRT_SW1, "func %s\n", __FUNCTION__);

    if (0 != fpga_drv_init())
    {
        printf("fpga_drv_init failed.\r\n");
        return SUCCESS;
    }

    fpga_reg_dev_ioctl(eENABLE_RIGHT_FPGA, 0);

    fpgaApartMent = MsOS_CreateApartment("fpgaTask",fpga_message_proc,pEventHandlerList);

    return SUCCESS;
}

