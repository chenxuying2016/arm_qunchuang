/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "hi_unf_spi.h"

// spi1:
// cs1: 90 => mipi1
// cs2: 91 => mipi2
#define MIPI1_CS_PIN		(90)
#define MIPI2_CS_PIN		(91)
#define MIPI_SP6_CS_PIN		(92)

// spi2:
// cs1: 78 => mipi3
// cs2: 79 => mipi4
#define MIPI3_CS_PIN		(78)
#define MIPI4_CS_PIN		(79)
#define TTL_MCU_CS_PIN		(80)
#define TTL_SP6_CS_PIN		(81)


// right
#define MIPI_SPI_DEV_NAME	"/dev/spidev32766.0"
#define MIPI_CS_PIN			(MIPI1_CS_PIN)
// left
#define MIPI_SPI2_DEV_NAME	"/dev/spidev32765.0"
#define MIPI_SPI2_CS_PIN	(MIPI4_CS_PIN)

static uint32_t mode  = 0;
static uint8_t  bits  = 8;
static uint32_t speed = 10000000;//1500000;
static uint16_t delay = 0;

static int s_spi_handle = -1;
static int s_spi2_handle = -1;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int HI_UNF_SPI_Read(int spiNo, char *pInData, int dataLen, char *poutData)
{
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pInData,
        .rx_buf = poutData,
        .len = dataLen,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

	// 
	switch (spiNo)
	{
		// spi 1: mipi 1 and mipi 2.
		case SPI1_CS_MIPI1:
			gpio_set_output_value(MIPI2_CS_PIN, 1);
			gpio_set_output_value(MIPI1_CS_PIN, 0);
			ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI1_CS_PIN, 1);
			break;

		case SPI1_CS_MIPI2:
			gpio_set_output_value(MIPI1_CS_PIN, 1);
			gpio_set_output_value(MIPI2_CS_PIN, 0);
			ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI2_CS_PIN, 1);
			break;		

		// spi 2:  mipi 3 and mipi 4.
		case SPI2_CS_MIPI3:
			gpio_set_output_value(MIPI4_CS_PIN, 1);
			gpio_set_output_value(MIPI3_CS_PIN, 0);
			ret = ioctl(s_spi2_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI3_CS_PIN, 1);
			break;

		case SPI2_CS_MIPI4:
			gpio_set_output_value(MIPI3_CS_PIN, 1);
			gpio_set_output_value(MIPI4_CS_PIN, 0);
			ret = ioctl(s_spi2_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI4_CS_PIN, 1);
			break;

		default: 
			printf("HI_UNF_SPI_Read error: Invalid param. channel = %d.\n.", spiNo);
			break;
	}	

    if (ret < 1)
    {
        printf("HI_UNF_SPI_Read error: ioctl failed! ret = %d. \n", ret);
        return -1;
    }
	
    return 0;
}

int HI_UNF_SPI_Write(int spiNo, char *pInData, int dataLen)
{
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pInData,
        .rx_buf = 0,
        .len = dataLen,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

	#if 1
	switch (spiNo)
	{
		// spi 1: mipi 1 and mipi 2.
		case SPI1_CS_MIPI1:
			gpio_set_output_value(MIPI2_CS_PIN, 1);
			gpio_set_output_value(MIPI1_CS_PIN, 0);
			ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI1_CS_PIN, 1);
			break;

		case SPI1_CS_MIPI2:
			gpio_set_output_value(MIPI1_CS_PIN, 1);
			gpio_set_output_value(MIPI2_CS_PIN, 0);
			ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI2_CS_PIN, 1);
			break;

		case SPI1_CS_ALL:
			gpio_set_output_value(MIPI1_CS_PIN, 0);
			gpio_set_output_value(MIPI2_CS_PIN, 0);
			ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI1_CS_PIN, 1);
			gpio_set_output_value(MIPI2_CS_PIN, 1);
			break;

		// spi 2:  mipi 3 and mipi 4.
		case SPI2_CS_MIPI3:
			gpio_set_output_value(MIPI4_CS_PIN, 1);
			gpio_set_output_value(MIPI3_CS_PIN, 0);
			ret = ioctl(s_spi2_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI3_CS_PIN, 1);
			break;

		case SPI2_CS_MIPI4:
			gpio_set_output_value(MIPI3_CS_PIN, 1);
			gpio_set_output_value(MIPI4_CS_PIN, 0);
			ret = ioctl(s_spi2_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI4_CS_PIN, 1);
			break;

		case SPI2_CS_ALL:
			gpio_set_output_value(MIPI3_CS_PIN, 0);
			gpio_set_output_value(MIPI4_CS_PIN, 0);
			ret = ioctl(s_spi2_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI3_CS_PIN, 1);
			gpio_set_output_value(MIPI4_CS_PIN, 1);
			break;
		
		case SPI_ALL_CS:
			gpio_set_output_value(MIPI1_CS_PIN, 0);
			gpio_set_output_value(MIPI2_CS_PIN, 0);
			ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI1_CS_PIN, 1);
			gpio_set_output_value(MIPI2_CS_PIN, 1);

			gpio_set_output_value(MIPI3_CS_PIN, 0);
			gpio_set_output_value(MIPI4_CS_PIN, 0);
			ret = ioctl(s_spi2_handle, SPI_IOC_MESSAGE(1), &tr);
			gpio_set_output_value(MIPI3_CS_PIN, 1);
			gpio_set_output_value(MIPI4_CS_PIN, 1);
			break;

		default: 
			printf("HI_UNF_SPI_Write error: Invalid param. channel = %d.\n.", spiNo);
			break;
	}
	#else
	gpio_set_output_value(MIPI_CS_PIN, 0);
	ret = ioctl(s_spi_handle, SPI_IOC_MESSAGE(1), &tr);
	gpio_set_output_value(MIPI_CS_PIN, 1);
	#endif

    if (ret < 1)
    {
        printf("%s error: ioctl failed!\n", __FUNCTION__);
        return -1;
    }
	
    return 0;
}

int HI_UNF_SPI_Lock(int spiNo)
{
	return 0;
}

int HI_UNF_SPI_UnLock(int spiNo)
{
	return 0;
}


static int __spi_open(char *devName)
{
	int ret = 0;
	int fd;

	fd = open(devName, O_RDWR);
	if (fd < 0)
	{
		printf("can't open device %s\n", devName);
		return -1;
	}

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1)
	{
		printf("can't set spi mode\n");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
	{
		printf("can't get spi mode\n");
		return -1;
	}

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		printf("can't set bits per word\n");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		printf("can't get bits per word\n");
		return -1;
	}

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		printf("can't set max speed hz\n");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		printf("can't get max speed hz\n");
		return -1;
	}

	#if 0
	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	#endif
	
	return fd;
}

static void __spi_close(int fd)
{
	close(fd);
}

void HI_UNF_SPI_Init()
{	
	// spi1
	gpio_set_output_value(MIPI1_CS_PIN, 1);
	gpio_set_output_value(MIPI2_CS_PIN, 1);	
	gpio_set_output_value(MIPI_SP6_CS_PIN, 1);

	// spi2
	gpio_set_output_value(MIPI3_CS_PIN, 1);
	gpio_set_output_value(MIPI4_CS_PIN, 1);	
	gpio_set_output_value(TTL_MCU_CS_PIN, 1);
	gpio_set_output_value(TTL_SP6_CS_PIN, 1);

	s_spi_handle = __spi_open(MIPI_SPI_DEV_NAME);
	s_spi2_handle = __spi_open(MIPI_SPI2_DEV_NAME);
}

void HI_UNF_SPI_Destory()
{
	__spi_close(s_spi_handle);
	__spi_close(s_spi2_handle);
}

