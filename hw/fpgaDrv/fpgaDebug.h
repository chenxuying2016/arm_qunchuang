
#ifndef _FPGA_DEBUG_H_
#define _FPGA_DEBUG_H_

#include <stdio.h>

#ifdef ENABLE_FPGA_DBG
#define FPGA_DBG(fmt, args...) printf(fmt, args)
#else
#define FPGA_DBG(fmt, args...)
#endif


#endif
