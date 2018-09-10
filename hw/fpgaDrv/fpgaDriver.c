#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "fpgaDebug.h"


#define FPGA_BUFFER_DEV			"/dev/hello"
#define FPGA_REG_DEV			"/dev/mem"


#define FPGA_REG_LENGTH			0x10000		// FPGA寄存器地址空间大小
#define PHY_ADDR  				0x43c00000  //7z030
#define PHY_LEN   				0x10000

int g_fpga_buf_dev_fd = -1;

static int s_fpga_reg_fd = -1;
unsigned char *g_fpga_reg_addr = NULL;

static int fpga_reg_dev_init(int *pFd, const char *deviceName, unsigned char **pMmapAddr, unsigned long mmapSize)
{
	if (*pFd < 0)
	{
		*pFd = open(deviceName, O_RDWR);
		if (*pFd < 0)
		{
			FPGA_DBG(FPGA_ERR, "Cannot open device %s\n", deviceName);
			return -1;
		}
	}

    *pMmapAddr = mmap(0, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, *pFd, PHY_ADDR);
	if ((void *)-1 == *pMmapAddr)
	{
		FPGA_DBG(FPGA_ERR, "mmap error.\n");
		*pMmapAddr = NULL;
		return -2;
	}
	return 0;
}

static void fpga_reg_dev_term(int *fd, unsigned char **mmap_addr, unsigned long mmap_size)
{
	if (NULL != *mmap_addr)
	{
		munmap(*mmap_addr, mmap_size);
		*mmap_addr = NULL;
	}

	if (*fd > 0)
	{
		close(*fd);
		*fd = -1;
	}
}

int fpga_drv_init()
{
	int ret = 0;

    if (g_fpga_buf_dev_fd < 0)
	{
        g_fpga_buf_dev_fd = open(FPGA_BUFFER_DEV,O_RDWR);
        if(g_fpga_buf_dev_fd < 0)
        {
            printf("open fpga dma buffer dev error!\n");
            return -1;
        }
	}
	
	if (s_fpga_reg_fd < 0 && NULL == g_fpga_reg_addr)
	{
		ret = fpga_reg_dev_init(&s_fpga_reg_fd, FPGA_REG_DEV, &g_fpga_reg_addr, FPGA_REG_LENGTH);
		if (0 != ret)
		{
            FPGA_DBG(FPGA_ERR, "##fpga_reg_dev_init init error.\n");
            printf("!!fpga_reg_dev_init init error.\n");
			return -2;
		}
	}
	
    FPGA_DBG(FPGA_INFO, "##fpga_drv_init init ok. g_fpga_buf_dev_fd: %d\n", g_fpga_buf_dev_fd);
	
	return 0;
}

void fpga_drv_term()
{
    if(g_fpga_buf_dev_fd > 0)
    {
        close(g_fpga_buf_dev_fd);
        g_fpga_buf_dev_fd = -1;
    }
	
    fpga_reg_dev_term(&s_fpga_reg_fd, &g_fpga_reg_addr, FPGA_REG_LENGTH);
}
