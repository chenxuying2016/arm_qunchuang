#include "pubPwrMain.h"
#include "comStruct.h"
#include "ini/iniparser.h"

int read_pwr_config(char *pwrName,s1103PwrCfgDb *pPower_info)
{
    char pwrpath[256];
    char section[256];
    dictionary	*ini;
    sprintf(pwrpath,"%s/%s",POWER_INI_FILE_PATH,pwrName);
    ini = iniparser_load(pwrpath);
    if (ini==NULL)
    {
        printf("cannot parse pwr.ini\n");
        return -1;
    }
    sprintf(section,"power:vdd");
    pPower_info->VDD= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddFlyTime");
    pPower_info->VDDFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddOverLimit");
    pPower_info->VDDOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddUnderLimit");
    pPower_info->VDDUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddCurrentOverLimit");
    pPower_info->VDDCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddCurrentUnderLimit");
    pPower_info->VDDCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:vddCurrentShortLimit");
    //pPower_info->vddCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddOpenDelay");
    pPower_info->VDDOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vddCloseDelay");
    pPower_info->VDDCloseDelay= iniparser_getint(ini, section, 0);


    sprintf(section,"power:vbl");
    pPower_info->VBL= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblFlyTime");
    pPower_info->VBLFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblOverLimit");
    pPower_info->VBLOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblUnderLimit");
    pPower_info->VBLUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblCurrentOverLimit");
    pPower_info->VBLCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblCurrentUnderLimit");
    pPower_info->VBLCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:vblCurrentShortLimit");
    //pPower_info->VBLCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblOpenDelay");
    pPower_info->VBLOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vblCloseDelay");
    pPower_info->VBLCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:vif");
    pPower_info->MTP= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifFlyTime");
    pPower_info->MTPFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifOverLimit");
    pPower_info->MTPOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifUnderLimit");
    pPower_info->MTPUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifCurrentOverLimit");
    pPower_info->MTPCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifCurrentUnderLimit");
    pPower_info->MTPCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:vifCurrentShortLimit");
    //pPower_info->MTPCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifOpenDelay");
    pPower_info->MTPOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vifCloseDelay");
    pPower_info->MTPCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:avdd");
    pPower_info->VDDIO= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddFlyTime");
    pPower_info->VDDIOFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddOverLimit");
    pPower_info->VDDIOOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddUnderLimit");
    pPower_info->VDDIOUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddCurrentOverLimit");
    pPower_info->VDDIOCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddCurrentUnderLimit");
    pPower_info->VDDIOCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:avddCurrentShortLimit");
    //pPower_info->VDDIOCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddOpenDelay");
    pPower_info->VDDIOOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:avddCloseDelay");
    pPower_info->VDDIOCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:vsp");
    pPower_info->VSP= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspFlyTime");
    pPower_info->VSPFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspOverLimit");
    pPower_info->VSPOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspUnderLimit");
    pPower_info->VSPUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspCurrentOverLimit");
    pPower_info->VSPCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspCurrentUnderLimit");
    pPower_info->VSPCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:vspCurrentShortLimit");
    //pPower_info->VSPCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspOpenDelay");
    pPower_info->VSPOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vspCloseDelay");
    pPower_info->VSPCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:vsn");
    pPower_info->VSN= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnFlyTime");
    pPower_info->VSNFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnOverLimit");
    pPower_info->VSNOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnUnderLimit");
    pPower_info->VSNUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnCurrentOverLimit");
    pPower_info->VSNCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnCurrentUnderLimit");
    pPower_info->VSNCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:vsnCurrentShortLimit");
    //pPower_info->VSNCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnOpenDelay");
    pPower_info->VSNOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vsnCloseDelay");
    pPower_info->VSNCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:elvdd");
    pPower_info->ELVDD= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddFlyTime");
    pPower_info->ELVDDFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddOverLimit");
    pPower_info->ELVDDOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddUnderLimit");
    pPower_info->ELVDDUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddCurrentOverLimit");
    pPower_info->ELVDDCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddCurrentUnderLimit");
    pPower_info->ELVDDCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:elvddCurrentShortLimit");
    //pPower_info->VSNCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddOpenDelay");
    pPower_info->ELVDDOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvddCloseDelay");
    pPower_info->ELVDDCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:elvss");
    pPower_info->ELVSS= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssFlyTime");
    pPower_info->ELVSSFlyTime= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssOverLimit");
    pPower_info->ELVSSOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssUnderLimit");
    pPower_info->ELVSSUnderLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssCurrentOverLimit");
    pPower_info->ELVSSCurrentOverLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssCurrentUnderLimit");
    pPower_info->ELVSSCurrentUnderLimit= iniparser_getint(ini, section, 0);
    //sprintf(section,"power:elvssCurrentShortLimit");
    //pPower_info->ELVSSCurrentShortLimit= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssOpenDelay");
    pPower_info->ELVSSOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:elvssCloseDelay");
    pPower_info->ELVSSCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:signalOpenDelay");
    pPower_info->signalOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:signalCloseDelay");
    pPower_info->signalCloseDelay= iniparser_getint(ini, section, 0);

    sprintf(section,"power:vdimOpenDelay");
    pPower_info->vdimOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vdimCloseDelay");
    pPower_info->vdimCloseDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:vdim");
    pPower_info->vdim= iniparser_getint(ini, section, 0);

    sprintf(section,"power:pwmOpenDelay");
    pPower_info->pwmOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:pwmCloseDelay");
    pPower_info->pwmCloseDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:pwm");
    pPower_info->pwm= iniparser_getint(ini, section, 0);
    sprintf(section,"power:pwmFreq");
    pPower_info->pwmFreq= iniparser_getint(ini, section, 0);
    sprintf(section,"power:pwmDuty");
    pPower_info->pwmDuty= iniparser_getint(ini, section, 0);

    sprintf(section,"power:invertOpenDelay");
    pPower_info->invertOpenDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:invertCloseDelay");
    pPower_info->invertCloseDelay= iniparser_getint(ini, section, 0);
    sprintf(section,"power:invert");
    pPower_info->invert= iniparser_getint(ini, section, 0);

    sprintf(section,"power:LEDEnable");
    pPower_info->LEDEnable= iniparser_getint(ini, section, 0);
    sprintf(section,"power:LEDchannel");
    pPower_info->LEDChannel= iniparser_getint(ini, section, 0);
    sprintf(section,"power:LEDdriverCurrent");
    pPower_info->LEDCurrent= iniparser_getint(ini, section, 0);
    sprintf(section,"power:LEDnumber");
    pPower_info->LEDNumber= iniparser_getint(ini, section, 0);
    sprintf(section,"power:LEDdriverOVP");
    pPower_info->LEDOVP= iniparser_getint(ini, section, 0);
    sprintf(section,"power:LEDdriverUVP");
    pPower_info->LEDUVP= iniparser_getint(ini, section, 0);

    iniparser_freedict(ini);
    return 0;
}

int check_power_min_volt_is_v1_8(s1103PwrCfgDb *p_power_cfg)
{
	if (p_power_cfg == NULL)
	{
		printf("check_power_min_volt_is_v1_8 error: Invalid param.\n");
		return 1;
	}

	if (p_power_cfg->VDD > 0 && p_power_cfg->VDD == 1800)
	{
		printf("check_power_min_volt_is_v1_8: vdd is 1800 mv.\n");
		return 1;
	}

	if (p_power_cfg->VDDIO > 0 && p_power_cfg->VDDIO == 1800)
	{
		printf("check_power_min_volt_is_v1_8: vddio is 1800 mv.\n");
		return 1;
	}

	if (p_power_cfg->VSP > 0 && p_power_cfg->VSP == 1800)
	{
		printf("check_power_min_volt_is_v1_8: vsp is 1800 mv.\n");
		return 1;
	}

	if (p_power_cfg->ELVDD > 0 && p_power_cfg->ELVDD == 1800)
	{
		printf("check_power_min_volt_is_v1_8: VGH is 1800 mv.\n");
		return 1;
	}

	printf("check_power_min_volt_is_v1_8: min power volt is not 1800 mv.\n");
	
	return 0;
}

