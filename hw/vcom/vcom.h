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
	int vcomotpmax;//xujie �����¼����
	int vcomBurner; //xujie VCOM��¼
	int idBurner; //xujie ID��¼
	int gammaBurner; //xujie GAMMA��¼
	int codeBurner; //xujie CODE��¼
	int pwrOnDelay; //xujie �������ʱ
	int gammaotpmax; //xujie GAMMA�����¼����
	int readDelay; //��ȡFLICK��ʱ
	int initVcom; //�����,Ĭ��VCOM�ĳ�ʼֵ
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
