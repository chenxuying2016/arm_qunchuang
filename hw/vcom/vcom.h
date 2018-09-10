#ifndef __VCOM_H__
#define __VCOM_H__


typedef struct tag_vcom_info_s{
    int maxvalue;
    int minvalue;
    //int checkvalue;//flick radio range
    //int min_check_value;
	//int max_check_value;
    int isBurner;
    int isVcomCheck;
    int type; //0:spi,1:i2c
    int i2cChannel;
    int i2cDevAddr;
    int i2cRegAddr;
	int vcomotpmax;//xujie 最大烧录次数
	int vcomBurner; //xujie VCOM烧录
	int idBurner; //xujie ID烧录
	int gammaBurner; //xujie GAMMA烧录
	int codeBurner; //xujie CODE烧录
	int pwrOnDelay; //xujie 开电后延时
	int gammaotpmax; //xujie GAMMA最大烧录次数
	int readDelay; //读取FLICK延时
	int initVcom; //开电后,默认VCOM的初始值
	int id1;
	int id2;
	int id3;

	float f_min_valid_flick;
	float f_max_valid_flick;
	float f_ok_flick;
}vcom_info_t;


#define MAX_GAMMA_REG_NUMS		(255)
typedef struct tag_gamma_cfg
{
	int enable_gamma_reg;
	int gamma_reg_nums;
	unsigned char gamma_reg[MAX_GAMMA_REG_NUMS];
}gamma_cfg_t;

#endif
