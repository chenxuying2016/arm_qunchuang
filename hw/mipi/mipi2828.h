#ifndef __MIPI_2828__
#define __MIPI_2828__

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define ON  1
#define OFF 0

typedef struct tag_CONFIG2828_STRUCT
{
    unsigned short reg_b0;
    unsigned short reg_b1;
    unsigned short reg_b2;
    unsigned short reg_b3;
    unsigned short reg_b4;
    unsigned short reg_b5;
    unsigned short reg_b6;
    unsigned short reg_b7;
    unsigned short reg_b8;
    unsigned short reg_b9;
    unsigned short reg_ba;
    unsigned short reg_bb;
    unsigned short reg_bc;
    unsigned short reg_bd;
    unsigned short reg_be;
    unsigned short reg_bf;

    unsigned short reg_c0;
    unsigned short reg_c1;
    unsigned short reg_c2;
    unsigned short reg_c3;
    unsigned short reg_c4;
    unsigned short reg_c5;
    unsigned short reg_c6;
    unsigned short reg_c7;
    unsigned short reg_c8;
    unsigned short reg_c9;
    unsigned short reg_ca;
    unsigned short reg_cb;
    unsigned short reg_cc;
    unsigned short reg_cd;
    unsigned short reg_ce;
    unsigned short reg_cf;

    unsigned short reg_d0;
    unsigned short reg_d1;
    unsigned short reg_d2;
    unsigned short reg_d3;
    unsigned short reg_d4;
    unsigned short reg_d5;
    unsigned short reg_d6;
    unsigned short reg_d7;
    unsigned short reg_d8;
    unsigned short reg_d9;
    unsigned short reg_da;
    unsigned short reg_db;
    unsigned short reg_dc;
    unsigned short reg_dd;
    unsigned short reg_de;
    unsigned short reg_df;

    unsigned short reg_e0;
    unsigned short reg_e1;
    unsigned short reg_e2;
    unsigned short reg_e3;
    unsigned short reg_e4;
    unsigned short reg_e5;
    unsigned short reg_e6;
    unsigned short reg_e7;
    unsigned short reg_e8;
    unsigned short reg_e9;
    unsigned short reg_ea;
    unsigned short reg_eb;
    unsigned short reg_ec;
    unsigned short reg_ed;
    unsigned short reg_ee;
    unsigned short reg_ef;

    unsigned short reg_f0;
    unsigned short reg_f1;
    unsigned short reg_f2;
    unsigned short reg_f3;
    unsigned short reg_f4;
    unsigned short reg_f5;
    unsigned short reg_f6;
    unsigned short reg_f7;
    unsigned short reg_f8;
    unsigned short reg_f9;
    unsigned short reg_fa;
    unsigned short reg_fb;
    unsigned short reg_fc;
    unsigned short reg_fd;
    unsigned short reg_fe;
    unsigned short reg_ff;
}CONFIG2828_STRUCT;

typedef struct tag_module_timing_info_s
{
    int vsyncpulseWidth;
    int hsyncpulseWidth;
    int vBackPorch;
    int hBackPorch;
    int vFrontPorch;
    int hFrontPorch;
    int vActiveTime;
    int hActiveTime;
    int lane;
    int bit;
}module_timing_info_t;

#if 1
typedef struct tag_flashModuleData_info_s
{
    module_timing_info_t module;

}flashModuleData_info_t;
#endif

void SendCode(int channel, /*uint8_t*/unsigned char *InitCode);

#endif
