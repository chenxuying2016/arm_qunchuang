/*
 * scanDev.h
 *
 *  Created on: 2018-7-16
 *      Author: tys
 */

#ifndef SCANDEV_H_
#define SCANDEV_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*serialDataCallback)(void* pData, unsigned int length);

void scanDevInit(serialDataCallback pFunc);
void scanDevDeinit();
void scanDevStartScan();

#ifdef __cplusplus
}
#endif
#endif /* SCANDEV_H_ */
