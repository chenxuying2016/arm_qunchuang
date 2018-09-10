#include "pgos/MsOS.h"
#include "common.h"
#include "comStruct.h"
#include "pubFpga.h"
#include "fpgaFunc.h"
#include "pgDB/pgDB.h"

#define MAX_DOWNLOAD_NUM 20

#define NOT_DOWNLOAD  0
#define DOWNLOAD_OK   1

typedef struct tag_move_ptn_info_s
{
    char bmpName[128];
    int  downLoadStatus;
    int  postion;
}move_ptn_info_t;

static move_ptn_info_t move_ptn_info[MAX_DOWNLOAD_NUM];
static int  s32ForcePostion = 0;

static MS_U32 s_taskId;
static MS_U32 s_semaphore;
static MS_U32 s_taskStop;

void move_ptn_init()
{
    memset(move_ptn_info,0,sizeof(move_ptn_info));
    s_semaphore = MsOS_CreateSemaphore(1);
}

int move_ptn_downloading(char *pBmpName,int width,int height,int ucPosition)
{
    printf("move_ptn_downloading: index=%d, image:%s\n", ucPosition, pBmpName);
	
    MsOS_ObtainSemaphore (s_semaphore, -1);
	
    int dwPicSize = width * height * 4 / 1024 / 1024; //0x0e;
    fpgaDdrBuf ddrBuf = { 0 };
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
        MsOS_ReleaseSemaphore(s_semaphore);
        return -1;
    }
	
    fpga_trans_pic_data(pBmpName);
	
    MsOS_ReleaseSemaphore(s_semaphore);
    return 0;
}

int move_ptn_download(char *pBmpName,int width,int height,int bAuto)
{
    int i=0;
	
    if(bAuto)
    {
        for(i = 0; i < MAX_DOWNLOAD_NUM; i ++)
        {
            if(move_ptn_info[i].downLoadStatus == NOT_DOWNLOAD)
            {
                if(move_ptn_downloading(pBmpName, width, height, i) == 0)
                {
                    move_ptn_info[i].postion = i;
                    strcpy(move_ptn_info[i].bmpName, pBmpName);
                    move_ptn_info[i].downLoadStatus = DOWNLOAD_OK;
                    //printf("move ptn downloading ok\n");
                    return  0;
                }
                else
                {
                    printf("move ptn downloading err\n");
                    return -1;
                }
            }
        }
    }
    else
    {
        if(move_ptn_downloading(pBmpName, width, height, s32ForcePostion) == 0)
        {
            int postion = s32ForcePostion;
            move_ptn_info[s32ForcePostion].postion = s32ForcePostion;
            strcpy(move_ptn_info[s32ForcePostion].bmpName, pBmpName);
            move_ptn_info[s32ForcePostion].downLoadStatus = DOWNLOAD_OK;
            s32ForcePostion++;
            s32ForcePostion = (s32ForcePostion)%MAX_DOWNLOAD_NUM;
            return postion;
        }
        else
        {
            return -1;
        }
    }
}

int move_ptn_getPostionByName(char *pBmpName)
{
    int i=0;
    for(i=0;i<MAX_DOWNLOAD_NUM;i++)
    {
        if(move_ptn_info[i].downLoadStatus == DOWNLOAD_OK)
        {
            if(strcmp(move_ptn_info[i].bmpName,pBmpName) == 0)
            {
                return move_ptn_info[i].postion;
            }
        }
    }
    return -1;
}


void *move_ptn_task(void *param)
{
    int  width = 0;
	int  height = 0;
    char ptnImageName[256] = "";
    char bmpImageName[256] = "";
    int loopId = 0;
    int ptnNum = dbModuleTstPtnNum();
    module_info_t *pModuleInfo = NULL;
	
    printf("move_ptn_task: ptn nusm is %d.\n", ptnNum);
	
    while(!s_taskStop)
    {
    	//printf("loopId: %d, ptnNum: %d.\n", loopId, ptnNum);
		
        if(loopId < ptnNum)
        {
            pModuleInfo = dbModuleGetPatternByLoopId(loopId);
            sprintf(bmpImageName,"./cfg/imageOrigin/%s/%s.bmp", pModuleInfo->displayX, pModuleInfo->ptdpatternname);
            sprintf(ptnImageName,"./cfg/ptn/%s/%s.bmp", pModuleInfo->displayX, pModuleInfo->ptdpatternname);

			if(access(bmpImageName, F_OK)!=0)
            {
                printf("bmp image is not exist:%s\n",bmpImageName);
                loopId++;
                continue;
            }
			
            if(access(ptnImageName, F_OK)!=0)
            {
                printf("ptn image is not exist:%s\n",ptnImageName);
                loopId++;
                continue;
            }
			
            sscanf(pModuleInfo->displayX, "%dX%d", &width, &height);
            move_ptn_download(bmpImageName, width, height, 1);
			
            loopId++;
        }
        else
        {
            //printf("move_ptn_task error: is work over,now is waiting\n");
            //usleep(1000 * 1000);
			break;
        }
    }

	printf("move_ptn_task end.\n");
    return NULL;
}

void move_ptn_createTask(void)
{    
    s_taskId=MsOS_CreateTask((TaskEntry)move_ptn_task,0, E_TASK_PRI_MEDIUM, &s_taskStop,0,"MovePtnTask");
}

void move_ptn_destoryTask(void)
{
    MsOS_DeleteTask(s_taskId);
    MsOS_ObtainSemaphore (s_semaphore, -1);
    memset(move_ptn_info,0,sizeof(move_ptn_info));
    MsOS_ReleaseSemaphore(s_semaphore);
}
