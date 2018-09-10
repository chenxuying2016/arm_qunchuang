/*
 * scanDevApi.h
 *
 *  Created on: 2018-7-6
 *      Author: tys
 */

#ifndef SERIALDEVAPI_H_
#define SERIALDEVAPI_H_

#include <termios.h>
#include <unistd.h>

#ifndef __cplusplus
extern "C" {
#endif

typedef enum
{
	E_B2400,
	E_B4800,
	E_B9600,
	E_B57600,
	E_B115200,
}baudRate_e;

typedef enum
{
	E_S_1_BIT,
	E_S_2_BIT,
}stopBits_e;

typedef enum
{
	E_0_PARITY,
	E_1_PARITY,
	E_2_PARITY,
}parity_e;

typedef enum
{
	E_D_5_BIT,
	E_D_6_BIT,
	E_D_7_BIT,
	E_D_8_BIT,
}dataBits_e;

struct serialDevM;

typedef unsigned int SERIAL_DEV_ID;
typedef void (*_serialProcess)(void* pData, unsigned int length);	//It is used to process Serial data
typedef void (*_serialDevMFree)(struct serialDevM* pM);
typedef int (*_serialDevMSend)(struct serialDevM* pM, void* pData, unsigned int length);
typedef int (*_serialDevMIsRunning)(struct serialDevM* pM);

typedef struct serialDevM
{
	SERIAL_DEV_ID devID;
	_serialDevMFree free;
	_serialDevMSend send;
	_serialDevMIsRunning isRunning;
}serialDevM_t;

serialDevM_t* serialDevMMalloc(const char* devName, baudRate_e rate,
		dataBits_e dataBits, stopBits_e stopBits,parity_e parity, _serialProcess process);

#ifndef __cplusplus
}
#endif
#endif /* SCANDEVAPI_H_ */
