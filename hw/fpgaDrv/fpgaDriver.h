

#ifndef FPGA_DRV_H_
#define FPGA_DRV_H_

int fpga_drv_init();
void fpga_drv_term();

int  fpga_reg_dev_ioctl(unsigned int cmd, unsigned long arg);
int  fpga_trans_pic_data(const char *pName);


#endif
