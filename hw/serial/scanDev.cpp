/*
 * scanDev.cpp
 *
 *  Created on: 2018-7-16
 *      Author: tys
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "serialDevApi.h"
#include "scanDev.h"

static serialDevM_t* pSgDevM = NULL;

void scanDevInit(serialDataCallback pFunc)
{
	do
	{
		if (pSgDevM)
		{
			if (pSgDevM->isRunning(pSgDevM))
				break;

			pSgDevM->free(pSgDevM);
			pSgDevM = NULL;
		}

		if (NULL == pSgDevM)
		{
			pSgDevM = serialDevMMalloc("/dev/ttyACM0", E_B115200, E_D_8_BIT, E_S_1_BIT, E_0_PARITY, pFunc);
			break;
		}

	} while (0);
}

void scanDevDeinit()
{
	if (pSgDevM)
		pSgDevM->free(pSgDevM);
}

void scanDevStartScan()
{
	if (NULL == pSgDevM || !pSgDevM->isRunning(pSgDevM))
		return;

	unsigned char req[] = {0x16,0x54,0x0D};
	pSgDevM->send(pSgDevM, req, sizeof(req));
}
