#ifndef __HI_UNF_SPI_H__
#define __HI_UNF_SPI_H__

#define SPI1_CS_MIPI1		(0x01)
#define SPI1_CS_MIPI2		(0x02)
#define SPI1_CS_ALL			(0x0F)

#define SPI2_CS_MIPI3		(0x03)
#define SPI2_CS_MIPI4		(0x04)
#define SPI2_CS_ALL			(0xF0)

#define SPI_ALL_CS			(0xFF)


void HI_UNF_SPI_Init();
void HI_UNF_SPI_Destory();
int HI_UNF_SPI_Read(int spiNo,char *pInData,int dataLen,char *poutData);
int HI_UNF_SPI_Write(int spiNo,char *pInData,int dataLen);
int HI_UNF_SPI_Lock(int spiNo);
int HI_UNF_SPI_UnLock(int spiNo);

#endif
