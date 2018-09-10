

MAKEFILE      = Makefile

####### Compiler, tools and options

CROSS_COMPILE := arm-xilinx-linux-gnueabi-
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
LINK := $(CROSS_COMPILE)g++

DEFINES := 
CFLAGS := -pipe -g -Wall -W -fPIC $(DEFINES)
CXXFLAGS := -pipe -g -std=gnu++0x -Wall -W -fPIC $(DEFINES)

INCPATH := -I. -I. -Iinc -Ipgos -Ihw/i2c -Ihw/gpio 
INCPATH	+= -Ihw/spi -Ihw/common/include -Ihw/fpga/include 
INCPATH	+= -Ihw/main/include 
INCPATH	+= -Ihw/power/include -Ihw/xml/include -Ihw/fpgaDrv
INCPATH += -Ihw/mipi -Ihw/vcom
INCPATH += -Ihw/lcd
INCPATH += -Ihw/serial
INCPATH += -Icommon

DEL_FILE := rm -f
CHK_DIR_EXISTS := test -d
MKDIR := mkdir -p
COPY := cp -f
COPY_FILE := cp -f
COPY_DIR := cp -f -R
INSTALL_FILE := install -m 644 -p
INSTALL_PROGRAM := install -m 755 -p
INSTALL_DIR := cp -f -R
DEL_FILE := rm -f
SYMLINK := ln -f -s
DEL_DIR := rmdir
MOVE := mv -f
TAR := tar -cf
COMPRESS := gzip -9f
DISTNAME := pgClient1.0.0
LFLAGS := -Wl,-z,origin -Wl,-rpath,\$$ORIGIN
LIBS := $(SUBLIBS) -L lib/ -lsqlite3 -lrt -lpthread
AR := ar cqs
RANLIB := 
SED := sed
STRIP := strip

## FLAGS     ############################################################
LCD_ILI_9806 := no
LCD_ILI_9881C := no
LCD_OTM_8019 := no
LCD_ICN_9706 := no
LCD_HI_8394 := no

# InnoLux
LCD_JD_9367 := yes
LCD_NT_35521S := yes


MIPI_READ_DEBUG := no

USED_NEW_FLICK_ALG := yes

USED_IC_MANAGER := yes

USED_NEW_GPIO := yes

ENABLE_CONSOLE := yes

USED_FAST_FLICK := yes

ENABLE_PROBE_DEBUG := no

ENABlE_OTP_BURN	:= yes

USE_Z_CA210_LIB := yes

SINGLE_IP_DEBUG := no

ENABLE_SHOW_CURSOR := yes

ENABLE_CONTROL_BOX := no

ENABLE_POWER_DEBUG_INFO := no
ENABLE_POWER_OLD_VERSION := no

ENABLE_BMP_DEBUG_INFO := no

JUST_USED_MIPI_CHANNEL_1 := no
ifeq ($(JUST_USED_MIPI_CHANNEL_1), yes) 
	CFLAGS += -D JUST_USED_MIPI_CHANNEL_1
	CXXFLAGS += -D JUST_USED_MIPI_CHANNEL_1 
else
endif

ENABLE_SP6_SPI := yes
ifeq ($(ENABLE_SP6_SPI), yes) 
	CFLAGS += -D ENABLE_SP6_SPI
	CXXFLAGS += -D ENABLE_SP6_SPI 
else
endif

ifeq ($(ENABLE_BMP_DEBUG_INFO), yes) 
	CFLAGS += -D ENABLE_BMP_DEBUG_INFO
	CXXFLAGS += -D ENABLE_BMP_DEBUG_INFO 
else
endif

ifeq ($(ENABLE_POWER_DEBUG_INFO), yes) 
	CFLAGS += -D ENABLE_POWER_DEBUG_INFO
	CXXFLAGS += -D ENABLE_POWER_DEBUG_INFO 
else
endif

ifeq ($(ENABLE_POWER_OLD_VERSION), yes) 
	CFLAGS += -D ENABLE_POWER_OLD_VERSION
	CXXFLAGS += -D ENABLE_POWER_OLD_VERSION 
else
endif

ifeq ($(ENABLE_CONTROL_BOX), yes) 
	CFLAGS += -D ENABLE_CONTROL_BOX
	CXXFLAGS += -D ENABLE_CONTROL_BOX 
else
endif

ifeq ($(ENABLE_SHOW_CURSOR), yes) 
	CFLAGS += -D ENABLE_SHOW_CURSOR
	CXXFLAGS += -D ENABLE_SHOW_CURSOR 
else
endif

ifeq ($(SINGLE_IP_DEBUG), yes) 
	CFLAGS += -D SINGLE_IP_DEBUG
	CXXFLAGS += -D SINGLE_IP_DEBUG 
else
endif

ifeq ($(USE_Z_CA210_LIB), yes) 
	CFLAGS += -D USE_Z_CA210_LIB
	CXXFLAGS += -D USE_Z_CA210_LIB 
else
endif

ifeq ($(ENABlE_OTP_BURN), yes) 
	CFLAGS += -D ENABlE_OTP_BURN
	CXXFLAGS += -D ENABlE_OTP_BURN 
else
endif

ifeq ($(ENABLE_CONSOLE), yes) 
	CFLAGS += -D ENABLE_CONSOLE
	CXXFLAGS += -D ENABLE_CONSOLE 
else
endif

ifeq ($(LCD_ILI_9806), yes) 
	CFLAGS += -D LCD_ILI_9806
	CXXFLAGS += -D LCD_ILI_9806 
else
endif

ifeq ($(LCD_ILI_9881C), yes) 
	CFLAGS += -D LCD_ILI_9881C
	CXXFLAGS += -D LCD_ILI_9881C 
else
endif

ifeq ($(LCD_ICN_9706), yes) 
	CFLAGS += -D LCD_ICN_9706
	CXXFLAGS += -D LCD_ICN_9706 
else
endif

ifeq ($(LCD_HI_8394), yes) 
	CFLAGS += -D LCD_HI_8394
	CXXFLAGS += -D LCD_HI_8394 
else
endif

ifeq ($(LCD_JD_9367), yes) 
	CFLAGS += -D LCD_JD_9367
	CXXFLAGS += -D LCD_JD_9367 
else
endif

ifeq ($(LCD_NT_35521S), yes) 
	CFLAGS += -D LCD_NT_35521S
	CXXFLAGS += -D LCD_NT_35521S 
else
endif

ifeq ($(MIPI_READ_DEBUG), yes) 
	CFLAGS += -D MIPI_READ_DEBUG
	CXXFLAGS += -D MIPI_READ_DEBUG 
endif 

ifeq ($(USED_NEW_FLICK_ALG), yes) 
	CFLAGS += -D USED_NEW_FLICK_ALG
	CXXFLAGS += -D USED_NEW_FLICK_ALG 
endif

ifeq ($(USED_NEW_GPIO), yes) 
	CFLAGS += -D USED_NEW_GPIO
	CXXFLAGS += -D USED_NEW_GPIO 
endif 

ifeq ($(USED_IC_MANAGER), yes) 
	CFLAGS += -D USED_IC_MANAGER
	CXXFLAGS += -D USED_IC_MANAGER 
endif 

ifeq ($(USED_FAST_FLICK), yes) 
	CFLAGS += -D USED_FAST_FLICK
	CXXFLAGS += -D USED_FAST_FLICK 
else
endif

ifeq ($(ENABLE_PROBE_DEBUG), yes) 
	CFLAGS += -D ENABLE_PROBE_DEBUG
	CXXFLAGS += -D ENABLE_PROBE_DEBUG 
else
endif

## FLAGS END ############################################################


####### Output directory

OBJECTS_DIR := obj/

####### Files

		
OBJECTS := obj/dictionary.o \
		obj/iniparser.o \
		obj/comUart.o \
		obj/comUtils.o \
		obj/fpgaDriver.o \
		obj/fpgaFunc.o \
		obj/main.o \
		obj/pubLcdTask.o \
		obj/pubPwrMain.o \
		obj/usrPwrMain.o \
		obj/cJSON.o \
		obj/loop.o \
		obj/packsocket.o \
		obj/pgDB.o \
		obj/MsOS_event.o \
		obj/MsOS_eventhandler.o \
		obj/MsOS_queue.o \
		obj/MsOS_task.o \
		obj/MsOS_test.o \
		obj/MsOS_timer.o \
		obj/debug.o \
		obj/utf8.o \
		obj/util.o \
		obj/xmlparser.o \
		obj/client.o \
		obj/clipg.o \
		obj/clirecvcmd5srv.o \
		obj/rwini.o \
		obj/mmain.o \
		obj/comIfnet.o \
		obj/pubpwr.o \
		obj/MsOS_control.o \
		obj/pubFpgaTask.o \
		obj/pubTiming.o \
		obj/pubPwrTask.o \
		obj/bitmap.o \
		obj/hi_unf_gpio.o \
		obj/hi_unf_i2c.o \
		obj/tst.o \
		obj/mipi2828.o \
		obj/hi_unf_spi.o \
		obj/pgLocalDB.o \
		obj/pubmipi.o \
		obj/pubmoveptn.o \
		obj/threadpool.o \
		obj/dwnFiles.o \
		obj/mipi_interface.o \
		obj/mipiflick.o \
		obj/box.o	\
		obj/ca310.o	\
		obj/mipi_chip.o \
		obj/vcom.o



ifeq ($(USED_IC_MANAGER), yes) 
	OBJECTS += obj/ic_manager.o
	OBJECTS += obj/default_chip.o
	
	ifeq ($(LCD_ILI_9806), yes)
		OBJECTS += obj/chip_ili_9806.o
	endif
	
	ifeq ($(LCD_ILI_9881C), yes)
		OBJECTS += obj/chip_ili_9881c.o
	endif
	
	ifeq ($(LCD_OTM_8019), yes)
		OBJECTS += obj/chip_otm_8019a.o
	endif

	ifeq ($(LCD_ICN_9706), yes) 
		OBJECTS += obj/chip_icn_9706.o
	endif
	
	ifeq ($(LCD_HI_8394), yes) 
		OBJECTS += obj/chip_hi_8394.o
	endif

	ifeq ($(LCD_JD_9367), yes) 
		OBJECTS += obj/chip_jd9367.o
	endif

	ifeq ($(LCD_NT_35521S), yes) 
		OBJECTS += obj/chip_nt_35521s.o
	endif

endif 

ifeq ($(ENABLE_SP6_SPI), yes)
	OBJECTS += obj/spi_sp6.o
endif

OBJECTS += obj/net_general_command.o
		
OBJECTS += obj/serialDev.o

OBJECTS += obj/ringBufApi.o
		
OBJECTS += obj/listApi.o

OBJECTS += obj/semApi.o

OBJECTS += obj/scanDev.o

TARGET := pgClient


all: $(TARGET)

first: all
####### Build rules

$(TARGET):  $(OBJECTS)  
#	@./sw_ver
	##@test -d bin/ || mkdir -p bin/
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)
	cp $@ $@_dbg
#	@arm-xilinx-linux-gnueabi-strip $(TARGET)


#all: Makefile $(TARGET)

clean: 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core
	@rm -rf $(TARGET)


distclean: clean 
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


####### Sub-libraries

check: first



####### Compile

obj/dictionary.o: ini/dictionary.c ini/dictionary.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/dictionary.o ini/dictionary.c

obj/iniparser.o: ini/iniparser.c ini/iniparser.h ini/dictionary.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/iniparser.o ini/iniparser.c

obj/comUart.o: hw/common/source/comUart.c hw/common/include/common.h hw/common/include/comUart.h \
		hw/common/include/comStruct.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/comUart.o hw/common/source/comUart.c

obj/comUtils.o: hw/common/source/comUtils.c hw/common/include/common.h \
		hw/common/include/comStruct.h \
		hw/common/include/comUtils.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/comUtils.o hw/common/source/comUtils.c

obj/fpgaDriver.o: hw/fpgaDrv/fpgaDriver.c hw/fpgaDrv/fpgaDebug.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/fpgaDriver.o hw/fpgaDrv/fpgaDriver.c

obj/fpgaFunc.o: hw/fpgaDrv/fpgaFunc.c hw/fpgaDrv/fpgaFunc.h \
		hw/fpgaDrv/fpgaRegister.h \
		hw/fpgaDrv/fpgaDebug.h \
		util/util.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/fpgaFunc.o hw/fpgaDrv/fpgaFunc.c

obj/main.o: hw/main/source/main.c hw/common/include/comStruct.h \
		hw/common/include/common.h \
		inc/libxml/xmlmemory.h \
		inc/libxml/xmlversion.h \
		inc/libxml/xmlexports.h \
		inc/libxml/threads.h \
		inc/libxml/globals.h \
		inc/libxml/parser.h \
		inc/libxml/tree.h \
		inc/libxml/xmlstring.h \
		inc/libxml/xmlregexp.h \
		inc/libxml/dict.h \
		inc/libxml/hash.h \
		inc/libxml/valid.h \
		inc/libxml/xmlerror.h \
		inc/libxml/list.h \
		inc/libxml/xmlautomata.h \
		inc/libxml/entities.h \
		inc/libxml/encoding.h \
		inc/libxml/xmlIO.h \
		inc/libxml/SAX.h \
		inc/libxml/xlink.h \
		inc/libxml/SAX2.h 

		
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/main.o hw/main/source/main.c

obj/pubPwrMain.o: hw/power/source/pubPwrMain.c hw/common/include/comStruct.h \
		hw/common/include/common.h \
		hw/power/include/pubPwrMain.h \
		hw/common/include/comUtils.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubPwrMain.o hw/power/source/pubPwrMain.c

obj/usrPwrMain.o: hw/power/source/usrPwrMain.c hw/common/include/comStruct.h \
		hw/common/include/common.h \
		hw/common/include/comUart.h \
		hw/power/include/pubPwrMain.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/usrPwrMain.o hw/power/source/usrPwrMain.c

obj/pubxml.o: hw/xml/source/pubxml.c hw/common/include/comStruct.h \
		hw/common/include/common.h \
		hw/xml/include/pubXmlExternFun.h \
		inc/libxml/xmlmemory.h \
		inc/libxml/xmlversion.h \
		inc/libxml/xmlexports.h \
		inc/libxml/threads.h \
		inc/libxml/globals.h \
		inc/libxml/parser.h \
		inc/libxml/tree.h \
		inc/libxml/xmlstring.h \
		inc/libxml/xmlregexp.h \
		inc/libxml/dict.h \
		inc/libxml/hash.h \
		inc/libxml/valid.h \
		inc/libxml/xmlerror.h \
		inc/libxml/list.h \
		inc/libxml/xmlautomata.h \
		inc/libxml/entities.h \
		inc/libxml/encoding.h \
		inc/libxml/xmlIO.h \
		inc/libxml/SAX.h \
		inc/libxml/xlink.h \
		inc/libxml/SAX2.h 
		
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubxml.o hw/xml/source/pubxml.c

obj/cJSON.o: json/cJSON.c json/cJSON.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/cJSON.o json/cJSON.c

obj/loop.o: loop/loop.c util/debug.h \
		loop/loop.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/loop.o loop/loop.c

obj/packsocket.o: packsocket/packsocket.c packsocket/packsocket.h \
		util/util.h \
		util/debug.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/packsocket.o packsocket/packsocket.c

obj/pgDB.o: pgDB/pgDB.c pgDB/pgDB.h \
		loop/loop.h \
		util/debug.h \
		inc/sqlite3.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pgDB.o pgDB/pgDB.c

obj/MsOS_event.o: pgos/MsOS_event.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_event.o pgos/MsOS_event.c

obj/MsOS_eventhandler.o: pgos/MsOS_eventhandler.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_eventhandler.o pgos/MsOS_eventhandler.c

obj/MsOS_queue.o: pgos/MsOS_queue.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_queue.o pgos/MsOS_queue.c

obj/MsOS_task.o: pgos/MsOS_task.c pgos/MsOS.h \
		pgos/MsTypes.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_task.o pgos/MsOS_task.c

obj/MsOS_test.o: pgos/MsOS_test.c pgos/MsOS.h \
		pgos/MsTypes.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_test.o pgos/MsOS_test.c

obj/MsOS_timer.o: pgos/MsOS_timer.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_timer.o pgos/MsOS_timer.c

obj/debug.o: util/debug.c util/debug.h \
		util/util.h \
		util/utf8.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/debug.o util/debug.c

obj/utf8.o: util/utf8.c util/debug.h \
		util/utf8.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/utf8.o util/utf8.c

obj/util.o: util/util.c util/debug.h \
		util/util.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/util.o util/util.c

obj/xmlparser.o: xmlparse/xmlparser.c xmlparse/xmlparser.h \
		util/debug.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/xmlparser.o xmlparse/xmlparser.c

obj/client.o: client.c client.h \
		pgos/MsOS.h \
		pgos/MsTypes.h \
		packsocket/packsocket.h \
		loop/loop.h \
		rwini.h \
		recvcmd.h \
		json/cJSON.h \
		util/debug.h \
		xmlparse/xmlparser.h \
		hw/common/include/comStruct.h \
		hw/common/include/common.h \
		pgDB/pgLocalDB.h \
		threadpool/threadpool.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/client.o client.c

obj/clipg.o: clipg.c client.h \
		pgos/MsOS.h \
		pgos/MsTypes.h \
		packsocket/packsocket.h \
		loop/loop.h \
		util/debug.h \
		util/util.h \
		rwini.h \
		pgDB/pgDB.h \
		pgDB/pgLocalDB.h \
		hw/fpga/include/pubFpga.h \
		hw/common/include/common.h \
		hw/fpgaDrv/fpgaFunc.h \
		hw/common/include/comStruct.h \
		hw/fpga/include/pubtiming.h \
		hw/fpgaDrv/fpgaRegister.h \
		recvcmd.h \
		hw/mipi/pubmipi.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/clipg.o clipg.c

obj/clirecvcmd5srv.o: clirecvcmd5srv.c packsocket/packsocket.h \
		client.h \
		pgos/MsOS.h \
		pgos/MsTypes.h \
		loop/loop.h \
		json/cJSON.h \
		rwini.h \
		util/debug.h \
		pgDB/pgLocalDB.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/clirecvcmd5srv.o clirecvcmd5srv.c

obj/rwini.o: rwini.c ini/iniparser.h \
		ini/dictionary.h \
		util/debug.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/rwini.o rwini.c

obj/mmain.o: mmain.c json/cJSON.h \
		util/util.h \
		rwini.h \
		packsocket/packsocket.h \
		client.h \
		pgos/MsOS.h \
		pgos/MsTypes.h \
		loop/loop.h \
		util/debug.h \
		hw/common/include/comStruct.h \
		hw/common/include/common.h \
		hw/fpgaDrv/fpgaFunc.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/mmain.o mmain.c

obj/comIfnet.o: hw/common/source/comIfnet.c hw/common/include/comStruct.h \
		hw/common/include/common.h \
		hw/common/include/comThread.h \
		hw/common/include/comIfnet.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/comIfnet.o hw/common/source/comIfnet.c

obj/pubpwr.o: hw/power/source/pubpwr.c hw/power/include/pubPwrMain.h \
		hw/common/include/comStruct.h \
		hw/common/include/common.h \
		ini/iniparser.h \
		ini/dictionary.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubpwr.o hw/power/source/pubpwr.c

obj/MsOS_control.o: pgos/MsOS_control.c pgos/MsOS.h \
		pgos/MsTypes.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/MsOS_control.o pgos/MsOS_control.c

obj/pubFpgaTask.o: hw/fpga/source/pubFpgaTask.c pgos/MsOS.h \
		pgos/MsTypes.h \
		hw/common/include/common.h \
		hw/common/include/comStruct.h \
		hw/fpga/include/pubFpga.h \
		hw/fpgaDrv/fpgaFunc.h \
		pgDB/pgDB.h \
		loop/loop.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubFpgaTask.o hw/fpga/source/pubFpgaTask.c

obj/pubTiming.o: hw/fpga/source/pubTiming.c hw/fpga/include/pubtiming.h \
		ini/iniparser.h \
		ini/dictionary.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubTiming.o hw/fpga/source/pubTiming.c

obj/pubPwrTask.o: hw/power/source/pubPwrTask.c pgos/MsOS.h \
		pgos/MsTypes.h \
		hw/common/include/common.h \
		hw/common/include/comStruct.h \
		hw/power/include/pubpwr.h \
		hw/common/include/comUart.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubPwrTask.o hw/power/source/pubPwrTask.c

obj/bitmap.o: util/bitmap.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/bitmap.o util/bitmap.c

obj/hi_unf_gpio.o: hw/gpio/hi_unf_gpio.c hw/gpio/hi_unf_gpio.h \
		pgos/MsTypes.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/hi_unf_gpio.o hw/gpio/hi_unf_gpio.c

obj/hi_unf_i2c.o: hw/i2c/hi_unf_i2c.c hw/i2c/hi_unf_i2c.h \
		pgos/MsTypes.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/hi_unf_i2c.o hw/i2c/hi_unf_i2c.c

obj/tst.o: test/tst.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/tst.o test/tst.c

obj/mipi2828.o: hw/mipi/mipi2828.c hw/mipi/mipi2828.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/mipi2828.o hw/mipi/mipi2828.c

obj/mipi_test.o: hw/mipi/mipi_test.c hw/spi/hi_unf_spi.h \
		hw/mipi/mipi2828.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/mipi_test.o hw/mipi/mipi_test.c

obj/hi_unf_spi.o: hw/spi/hi_unf_spi.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/hi_unf_spi.o hw/spi/hi_unf_spi.c

obj/pgLocalDB.o: pgDB/pgLocalDB.c pgDB/pgLocalDB.h \
		loop/loop.h \
		util/debug.h \
		util/util.h \
		inc/sqlite3.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pgLocalDB.o pgDB/pgLocalDB.c

obj/pubmipi.o: hw/mipi/pubmipi.c hw/spi/hi_unf_spi.h \
		hw/mipi/mipi2828.h \
		hw/mipi/pubmipi.h \
		hw/fpga/include/pubtiming.h \
		util/debug.h \
		hw/fpgaDrv/fpgaRegister.h \
		pgos/MsOS.h \
		pgos/MsTypes.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubmipi.o hw/mipi/pubmipi.c

obj/pubmoveptn.o: hw/fpga/source/pubmoveptn.c pgos/MsOS.h \
		pgos/MsTypes.h \
		hw/common/include/common.h \
		hw/common/include/comStruct.h \
		hw/fpga/include/pubFpga.h \
		hw/fpgaDrv/fpgaFunc.h \
		pgDB/pgDB.h \
		loop/loop.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubmoveptn.o hw/fpga/source/pubmoveptn.c

obj/threadpool.o: threadpool/threadpool.c threadpool/threadpool.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/threadpool.o threadpool/threadpool.c

obj/dwnFiles.o: dwnFiles/dwnFiles.c dwnFiles/dwnFiles.h \
		pgos/MsOS.h \
		pgos/MsTypes.h \
		hw/common/include/common.h \
		hw/common/include/comStruct.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/dwnFiles.o dwnFiles/dwnFiles.c

obj/mipi_interface.o: hw/mipi/mipi_interface.c hw/spi/hi_unf_spi.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/mipi_interface.o hw/mipi/mipi_interface.c

obj/mipiflick.o: hw/mipi/mipiflick.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/mipiflick.o hw/mipi/mipiflick.c

obj/pubLcdTask.o: hw/lcd/pubLcdTask.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/pubLcdTask.o hw/lcd/pubLcdTask.c

obj/box.o: hw/box/box.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/box.o hw/box/box.c

obj/ca310.o: hw/lcd/ca310.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/ca310.o hw/lcd/ca310.c
	
obj/mipi_chip.o: hw/mipi/mipi_chip.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/mipi_chip.o hw/mipi/mipi_chip.c
	
obj/vcom.o: hw/vcom/vcom.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/vcom.o hw/vcom/vcom.c	
	
obj/serialDev.o: hw/serial/serialDev.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o obj/serialDev.o hw/serial/serialDev.cpp

obj/listApi.o: common/listApi.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o obj/listApi.o common/listApi.cpp
	
obj/ringBufApi.o: common/ringBufApi.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o obj/ringBufApi.o common/ringBufApi.cpp

obj/semApi.o: common/semApi.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o obj/semApi.o common/semApi.cpp

obj/scanDev.o: hw/serial/scanDev.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o obj/scanDev.o hw/serial/scanDev.cpp
			
############################################################################################
##   IC Manager
ifeq ($(USED_IC_MANAGER), yes) 

obj/ic_manager.o: icm/ic_manager.c icm/ic_manager.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/ic_manager.o icm/ic_manager.c

obj/default_chip.o: icm/chips/default_chip.c icm/chips/default_chip.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/default_chip.o icm/chips/default_chip.c
	

# ILI_9806	
ifeq ($(LCD_ILI_9806), yes)
obj/chip_ili_9806.o: icm/chips/chip_ili_9806.c icm/chips/chip_ili_9806.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_ili_9806.o icm/chips/chip_ili_9806.c
endif

# ILI_9881C	
ifeq ($(LCD_ILI_9881C), yes)
obj/chip_ili_9881c.o: icm/chips/chip_ili_9881c.c icm/chips/chip_ili_9881c.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_ili_9881c.o icm/chips/chip_ili_9881c.c
endif

# OTM_8019	
ifeq ($(LCD_OTM_8019), yes)
obj/chip_otm_8019a.o: icm/chips/chip_otm_8019a.c icm/chips/chip_otm_8019a.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_otm_8019a.o icm/chips/chip_otm_8019a.c
endif

# ICN_9706	
ifeq ($(LCD_ICN_9706), yes)
obj/chip_icn_9706.o: icm/chips/chip_icn_9706.c icm/chips/chip_icn_9706.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_icn_9706.o icm/chips/chip_icn_9706.c
endif

# HI_8394	
ifeq ($(LCD_HI_8394), yes)
obj/chip_hi_8394.o: icm/chips/chip_hi_8394.c icm/chips/chip_hi_8394.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_hi_8394.o icm/chips/chip_hi_8394.c
endif

# JD 9367	
ifeq ($(LCD_JD_9367), yes)
obj/chip_jd9367.o: icm/chips/chip_jd9367.c icm/chips/chip_jd9367.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_jd9367.o icm/chips/chip_jd9367.c
endif

# NT 35521S	
ifeq ($(LCD_NT_35521S), yes)
obj/chip_nt_35521s.o: icm/chips/chip_nt_35521s.c icm/chips/chip_nt_35521s.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/chip_nt_35521s.o icm/chips/chip_nt_35521s.c
endif

endif 
##   IC Manager end.
############################################################################################

obj/net_general_command.o: net_general_command.c net_general_command.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/net_general_command.o net_general_command.c

ifeq ($(ENABLE_SP6_SPI), yes)
obj/spi_sp6.o: hw/spi/spi_sp6.c hw/spi/spi_sp6.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o obj/spi_sp6.o hw/spi/spi_sp6.c
endif

####### Install

install:  FORCE
	cp $(TARGET) /tftpboot/ -rf
	cp $(TARGET)_dbg /tftpboot/ -rf

uninstall:  FORCE
	rm -rf /tftpboot/$(TARGET)
	rm -rf /tftpboot/$(TARGET)_dbg
	
FORCE:

