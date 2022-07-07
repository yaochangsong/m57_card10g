/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

extern uint32_t config_get_disk_file_notifier_timeout(void);
int config_load_network(char *ifname);
static int config_fft_window_data_init(void);

static s_config config;

/**
 * Mutex for the configuration file, used by the related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Sets the default config parameters and initialises the configuration system */
void config_init(void)
{  
    char *path;
    printf_debug("config init\n");
    memset(&config, 0, sizeof(config));
    path = get_config_path();
    if( path == NULL)
        config.configfile = safe_strdup(DEFAULT_CONFIGFILE);
    else
        config.configfile = path;
    config.daemon = -1;
    for(int i= 0; i< MAX_RADIO_CHANNEL_NUM; i++)
        config.oal_config.channel[i].work_mode = OAL_NULL_MODE;
    #ifdef CONFIG_NET_10G
    config.oal_config.ctrl_para.wz_threshold_bandwidth = 1000000000; /* 万兆默认阀值; >=该值，用万兆传输 */
    #endif
    #if defined (CONFIG_DAO_XML)
    dao_read_create_config_file(config.configfile, &config);
    #elif defined(CONFIG_DAO_JSON)
    json_read_config_file(&config);
    #endif
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname){
            config_load_network(config.oal_config.network[i].ifname);
        }
    }
#ifdef CONFIG_SPM_FFT_WINDOWS
    config_fft_window_data_init();
#endif
}

int config_get_if_nametoindex(char *ifname)
{
    if(ifname == NULL || strlen(ifname) == 0)
        return -1;
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname){
            if(!strcmp(config.oal_config.network[i].ifname, ifname))
                return i;
        }
    }
    return -1;
}

char *config_get_if_indextoname(int index)
{
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname){
            if(index == i)
                return config.oal_config.network[i].ifname;
        }
    }
    return NULL;
}
bool config_match_gateway_addr(const char *ifname, uint32_t gateway)
{
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname){
            if(config.oal_config.network[i].addr.gateway == gateway)
                return true;
        }
    }
    return false;
}

bool config_match_ipaddr_addr(const char *ifname, uint32_t ipaddr)
{
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname && !strcmp(config.oal_config.network[i].ifname, ifname)){
            if(config.oal_config.network[i].addr.ipaddress == ipaddr)
                return true;
        }
    }
    return false;
}

bool config_match_netmask_addr(const char *ifname, uint32_t netmask)
{
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname && !strcmp(ifname, config.oal_config.network[i].ifname)){
            if(config.oal_config.network[i].addr.netmask == netmask)
                return true;
        }
    }
    return false;
}
int config_get_if_cmd_port(const char *ifname, uint16_t *port)
{
    if(ifname == NULL || strlen(ifname) == 0)
        return -1;
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
            if(config.oal_config.network[i].ifname && !strcmp(config.oal_config.network[i].ifname, ifname)){
                *port = config.oal_config.network[i].port;
                return 0;
            }
    }
    
    return -1;
}
int config_set_if_cmd_port(const char *ifname, uint16_t port)
{
    if(ifname == NULL || strlen(ifname) == 0)
        return -1;
    
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        if(config.oal_config.network[i].ifname){
            if(!strcmp(config.oal_config.network[i].ifname, ifname)){
                if(config.oal_config.network[i].port  == port)
                    break;
                config.oal_config.network[i].port  = port;
                config_save_all();
                return 0;
            }
        }
    }
    
    return -1;
}
char *config_get_ifname_by_addr(struct sockaddr_in *addr)
{
    for(int i = 0; i < ARRAY_SIZE(config.oal_config.network); i++){
        //printf_note("addr: %x, %x\n", addr->sin_addr.s_addr,  config.oal_config.network[i].addr.ipaddress);
        if(addr->sin_addr.s_addr == config.oal_config.network[i].addr.ipaddress)
            return config.oal_config.network[i].ifname;
    }
    return NULL;
}

bool config_get_work_enable(void)
{
    for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        if(config.oal_config.channel[i].enable.map.enable){
            return true;
        }
    }
    return false;
}

/** Accessor for the current configuration
@return:  A pointer to the current config.
 */
s_config *config_get_config(void)
{
    return &config;
}

int config_load_network(char *ifname)
{
    struct in_addr ipaddr, netmask, gateway;
    struct network_addr_st network;
    uint8_t  mac[6];
    int index;

    if(ifname == NULL || strlen(ifname) == 0)
        return -1;

    index = config_get_if_nametoindex(ifname);
    if(index == -1)
        return -1;

    if(get_ipaddress(ifname, &ipaddr) != -1){
        network.ipaddress = ipaddr.s_addr;
        printf_note("ipaddress: %s\n", inet_ntoa(ipaddr));
    }
    if(get_netmask(ifname, &netmask) != -1){
        network.netmask = netmask.s_addr;
        printf_note("netmask: %s\n", inet_ntoa(netmask));
    }
    if(get_gateway(ifname, &gateway) != -1){
        network.gateway = gateway.s_addr;
        printf_note("gateway: %s\n", inet_ntoa(gateway));
    }
    if(get_mac(ifname, mac, sizeof(mac)) != -1){
        memcpy(network.mac, mac, sizeof(network.mac));
        printf_note("mac=%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    }
    memcpy(&config.oal_config.network[index].addr, &network, sizeof(struct network_addr_st));

    return 0;
}

int config_set_network(char *ifname, uint32_t ipaddr, uint32_t netmask, uint32_t gateway)
{
    // /etc/netset.sh set eth0 192.168.2.111 255.255.255.0 192.168.2.1
    char buffer[128] = {0};
    char *script = "/etc/netset.sh set";
    struct in_addr in_ipaddr, in_netmask, in_gateway;
    char s_ip[32], s_mask[32], s_gw[32];
    int ret = 0;
    
    if(ifname == NULL || if_nametoindex(ifname) == 0){
        printf_note("ifname: %s not found in device\n", ifname);
        return -1;
    }
    in_ipaddr.s_addr = ipaddr;
    strcpy(s_ip, inet_ntoa(in_ipaddr));
    in_netmask.s_addr = netmask;
    strcpy(s_mask, inet_ntoa(in_netmask));
    in_gateway.s_addr = gateway;
    strcpy(s_gw, inet_ntoa(in_gateway));
    snprintf(buffer, sizeof(buffer)-1, "%s %s %s %s %s", script, ifname, s_ip, s_mask, s_gw);
    printf_note("%s\n", buffer);
    ret = safe_system(buffer);
    config_load_network(ifname);

    return ret;
}

static int config_set_netaddr(char *ifname, uint32_t netaddr, char *script)
{
    // /etc/netset.sh set_xxx eth0 192.168.2.111
    static char buffer[128] = {0};
    struct in_addr in_ipaddr;
    char s_netaddr[32];
    int ret = 0;
    
    if(ifname == NULL || if_nametoindex(ifname) == 0){
        printf_note("ifname: %s not found in device\n", ifname);
        return -1;
    }
    in_ipaddr.s_addr = netaddr;
    strcpy(s_netaddr, inet_ntoa(in_ipaddr));
    snprintf(buffer, sizeof(buffer)-1, "%s %s %s", script, ifname, s_netaddr);
    printf_info("%s\n", buffer);
    ret = safe_system(buffer);
    config_load_network(ifname);
    
    return ret;
}

int config_set_ip(char *ifname, uint32_t ipaddr)
{
    int ret = 0;
    char *script = "/etc/netset.sh set_ip";
    ret=config_set_netaddr(ifname, ipaddr, script);
    return ret;
}


int config_set_netmask(char *ifname, uint32_t netmask)
{
    int ret = 0;
    char *script = "/etc/netset.sh set_netmask";
    ret = config_set_netaddr(ifname, netmask, script);
    return ret;
}

int config_set_gateway(char *ifname, uint32_t gateway)
{
    int ret = 0;
    char *script = "/etc/netset.sh set_gw";
    ret = config_set_netaddr(ifname, gateway, script);
    return ret;
}


/*本控 or 远控 查看接口*/
int config_get_control_mode(void)
{
#ifdef  CONFIG_LOCAL_CTRL_EN
    if(config.oal_config.ctrl_para.remote_local == CTRL_MODE_LOCAL){
        return CTRL_MODE_LOCAL;
    }else{
        return CTRL_MODE_REMOTE;
    }
#endif
    return CTRL_MODE_REMOTE;
}

void config_save_cache(int cmd, uint8_t type, int8_t ch, void *data)
{
    uint8_t status;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
     switch(cmd)
     {
        case EX_STATUS_CMD:
        {
            switch(type){
                case EX_CLK_STATUS:
                {
                    status = ((*(uint8_t *)data) == 0) ? 0 : 1;
                    printf_debug("clock status: %d, %d\n", *(uint8_t *)data, status);
                    poal_config->status_para.clkInfo.status = status;
                }
                    break;
                case EX_AD_STATUS:
                {
                    status = ((*(uint8_t *)data) == 0) ? 0 : 1;
                    printf_debug("ad status: %d, %d\n", *(uint8_t *)data, status);
                    poal_config->status_para.adInfo.status = status;
                }
                    break;
                case EX_DISK_STATUS:
                    status = ((*(uint8_t *)data) == 0) ? 0 : 1;
                    printf_debug("disk status: %d, %d\n", *(uint8_t *)data, status);
                    poal_config->status_para.diskInfo.diskNode.status = status;
                    break;
                    
            }
            break;
        }
     }
}

bool config_get_is_internal_clock(void)
{
    if(config.oal_config.ctrl_para.internal_clock == 0){
        /* External clock */
        return false;
    }
    return true;
}

uint32_t config_get_noise_level(void)
{
    return config.oal_config.ctrl_para.noise_level;
}

uint64_t config_get_disk_alert_threshold(void)
{
    return config.oal_config.status_para.diskInfo.alert.alert_threshold_byte;
}

void config_set_disk_alert_threshold(uint64_t val)
{
    config.oal_config.status_para.diskInfo.alert.alert_threshold_byte = val;
}

void config_set_split_file_threshold(uint64_t val)
{
    printf_note("set threshold: %"PRIu64"byte\n", val);
    config.oal_config.status_para.diskInfo.alert.split_file_threshold_byte = val;
}

uint64_t config_get_split_file_threshold(void)
{
    return config.oal_config.status_para.diskInfo.alert.split_file_threshold_byte;
}

char *config_get_devicecode(void)
{
    if(config.oal_config.ctrl_para.device_code && strlen(config.oal_config.ctrl_para.device_code) > 0)
        return config.oal_config.ctrl_para.device_code;
    return "0";
}

volatile bool auto_save_mode = false;
bool config_set_file_auto_save_mode(bool is_auto)
{
    auto_save_mode = is_auto;
    return auto_save_mode;
}

bool config_get_file_auto_save_mode(void)
{
    return auto_save_mode;
}


uint32_t config_get_disk_file_notifier_timeout(void)
{
    #define NOTIFIER_TIMEOUT 1000
    if(config.oal_config.ctrl_para.disk_file_notifier_timeout_ms == 0)
        return NOTIFIER_TIMEOUT;
    else 
        return config.oal_config.ctrl_para.disk_file_notifier_timeout_ms;
}

bool config_is_temperature_warning(int16_t temperature)
{
    #define DEFAILT_TEMP_WARN_THRESHOLD 85
    if(config.oal_config.ctrl_para.temperature_warn_threshold <= 0)
        config.oal_config.ctrl_para.temperature_warn_threshold = DEFAILT_TEMP_WARN_THRESHOLD;
    if(temperature > config.oal_config.ctrl_para.temperature_warn_threshold)
        return true;
    return false;
}


uint32_t  config_get_fft_size(uint8_t ch)
{
    uint32_t fftsize = 0;
    uint8_t mode; 
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    mode = poal_config->channel[ch].work_mode;
    if((mode == OAL_FAST_SCAN_MODE) || (mode == OAL_MULTI_ZONE_SCAN_MODE)){
        fftsize = poal_config->channel[ch].multi_freq_fregment_para.fregment[0].fft_size;
    }else{
        fftsize = poal_config->channel[ch].multi_freq_point_param.points[0].fft_size;
    }
    if(fftsize == 0){
        printf_warn("fftsize %u not set!!!\n",fftsize);
    }
    printf_note("fftsize:%u\n",fftsize);
    return fftsize;
}

static uint32_t config_get_resolution_by_fft(uint32_t fftsize, uint32_t bw_hz)
{
    uint32_t resolution;
    uint64_t sample_rate;

    sample_rate = io_get_raw_sample_rate(0,0,bw_hz);
    if(fftsize >= 0)
        resolution = sample_rate/fftsize;
    else{
        resolution = sample_rate/1024;
        printf_err("fftsize is 0,use default fftsize value: 1024, resolution:%u", resolution);
    }
    resolution = resolution/FFT_RESOLUTION_FACTOR;

    printf_debug("resolution=%uhz, fftsize=%u\n", resolution, fftsize);
    return resolution;
}

static uint32_t config_get_fft_by_resolution(uint32_t resolution, uint32_t bw_hz)
{
    uint32_t fftsize;
    uint64_t sample_rate;

    sample_rate = io_get_raw_sample_rate(0,0,bw_hz);
    if(resolution > 0)
        fftsize = sample_rate/resolution;
    else{
        printf_err("resolution is 0,use default fftsize value: 1024");
        fftsize = 1024;
    }
    fftsize = fftsize/FFT_RESOLUTION_FACTOR;

   // printf_note("resolution=%uhz, fftsize=%u\n", resolution, fftsize);
    return fftsize;
}

int config_get_fft_resolution(int mode, int ch, int index, uint32_t bw_hz,uint32_t *fft_size, uint32_t *resolution)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    if(mode == OAL_FIXED_FREQ_ANYS_MODE || mode == OAL_MULTI_POINT_SCAN_MODE){
        struct multi_freq_point_para_st *point;
        point = &poal_config->channel[ch].multi_freq_point_param;
#if defined(CONFIG_SPM_FFT_BY_RESOLUTION)
        *resolution = point->points[index].freq_resolution;
        *fft_size = config_get_fft_by_resolution(point->points[index].freq_resolution, bw_hz);
#else
        if(point->points[index].fft_size == 0){
            printf_warn("fft Size is NULL, default: 2K\n");
            point->points[index].fft_size = 2048;
        }
        *fft_size = point->points[index].fft_size;
        *resolution = config_get_resolution_by_fft(point->points[index].fft_size, bw_hz);
#endif
    } else{
        struct multi_freq_fregment_para_st *fregment;
        fregment = &poal_config->channel[ch].multi_freq_fregment_para;
#if defined(CONFIG_SPM_FFT_BY_RESOLUTION)
        *resolution = fregment->fregment[index].freq_resolution;
        *fft_size = config_get_fft_by_resolution(fregment->fregment[index].freq_resolution, bw_hz);
#else
        if(fregment->fregment[index].fft_size == 0){
            printf_warn("fft Size is NULL, default: 2K\n");
            fregment->fregment[index].fft_size = 2048;
        }
        *fft_size = fregment->fregment[index].fft_size;
        *resolution = config_get_resolution_by_fft(fregment->fregment[index].fft_size, bw_hz);
#endif
    }

    return 0;
}

int32_t config_get_analysis_calibration_value(uint64_t m_freq_hz)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;
    int i;

    for(i = 0; i< sizeof(poal_config->cal_level.analysis.start_freq_khz)/sizeof(uint32_t); i++){
        if((m_freq_hz >= (uint64_t)poal_config->cal_level.analysis.start_freq_khz[i]*1000) && (m_freq_hz < (uint64_t)poal_config->cal_level.analysis.end_freq_khz[i]*1000)){
            cal_value = poal_config->cal_level.analysis.power_level[i];
            found = 1;
            break;
        }
    }
    if(found){
        printf_debug("find the calibration level: %"PRIu64", %d\n", m_freq_hz, cal_value);
    }else{
        printf_note("Not find the calibration level, use default value: %d\n", cal_value);
    }
    cal_value += poal_config->cal_level.analysis.global_roughly_power_lever;

    return cal_value;
}

int32_t config_get_dc_offset_nshift_calibration_value(uint8_t ch, uint32_t fft_size, uint64_t m_freq)
{
    #define MSHIFT_MIN_RANGE  0x08
    #define MSHIFT_MAX_RANGE  0x14
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value=0,freq_cal_value=0, found = 0;
    uint32_t  _start_freq_khz = 0, _end_freq_khz = 0;
    int i;
    fft_size = fft_size;
    
    if(poal_config->cal_level.dc_offset.is_open == false)
        return -1;
    
    cal_value = poal_config->cal_level.dc_offset.global_mshift;
    if(m_freq > 0){
        for(i = 0; i< ARRAY_SIZE(poal_config->cal_level.dc_offset.mshift); i++){
            _start_freq_khz =poal_config->cal_level.dc_offset.start_freq_khz[i];
            _end_freq_khz = poal_config->cal_level.dc_offset.end_freq_khz[i];
            if(_start_freq_khz ==0 && _end_freq_khz == 0){
                break;
            }
            printf_debug("[%d],m_freq=%"PRIu64", _start_freq_khz=%u, _end_freq_khz=%u\n", i, m_freq,  _start_freq_khz, _end_freq_khz);
            if((m_freq >= (uint64_t)_start_freq_khz*1000) &&  (m_freq < (uint64_t)_end_freq_khz *1000)){
                freq_cal_value = poal_config->cal_level.dc_offset.mshift[i];
                cal_value += freq_cal_value;
                found ++;
                break;
            }
        }
    }
    if ((cal_value < MSHIFT_MIN_RANGE) ||(cal_value > MSHIFT_MAX_RANGE)){
        return -1;
    }
    return cal_value;
}


int32_t config_get_gain_calibration_value(uint8_t ch, uint32_t fft_size, uint64_t m_freq)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value=0,freq_cal_value=0, found = 0;
    uint32_t  _start_freq_khz = 0, _end_freq_khz = 0;
    int i;
    fft_size = fft_size;
    cal_value = poal_config->cal_level.mgc.global_gain_val;
    if(m_freq > 0){
        for(i = 0; i< ARRAY_SIZE(poal_config->cal_level.mgc.gain_val); i++){
            _start_freq_khz =poal_config->cal_level.mgc.start_freq_khz[i];
            _end_freq_khz = poal_config->cal_level.mgc.end_freq_khz[i];
            if(_start_freq_khz ==0 && _end_freq_khz == 0){
                break;
            }
            printf_debug("[%d],m_freq=%"PRIu64", _start_freq_khz=%u, _end_freq_khz=%u\n", i, m_freq,  _start_freq_khz, _end_freq_khz);
            if((m_freq >= (uint64_t)_start_freq_khz*1000) &&  (m_freq < (uint64_t)_end_freq_khz *1000)){
                freq_cal_value = poal_config->cal_level.mgc.gain_val[i];
                cal_value += freq_cal_value;
                found ++;
                break;
            }
        }
    }
    cal_value += poal_config->channel[ch].rf_para.mgc_gain_value;

    return cal_value;
}
int32_t  config_get_fft_calibration_value(uint8_t ch, uint32_t fft_size, uint64_t m_freq)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int i;
    int32_t cal_value=0,freq_cal_value=0, freq_found = 0;
    uint32_t _fft = 0, _start_freq_khz = 0, _end_freq_khz = 0;
    int mode = 0;

    if(fft_size > 0){
        for(i=0;i<sizeof(poal_config->cal_level.cali_fft.fftsize)/sizeof(uint32_t);i++)
        {
            if(fft_size==poal_config->cal_level.cali_fft.fftsize[i])
            {
                cal_value=poal_config->cal_level.cali_fft.cali_value[i];
                break;
            }
        }
    }
    
    if(m_freq > 0){
        for(i = 0; i< ARRAY_SIZE(poal_config->cal_level.spm_level.cal_node); i++){
            _fft = poal_config->cal_level.spm_level.cal_node[i].fft;
            _start_freq_khz = poal_config->cal_level.spm_level.cal_node[i].start_freq_khz;
            _end_freq_khz = poal_config->cal_level.spm_level.cal_node[i].end_freq_khz;
            if(_fft == 0 && _start_freq_khz ==0 && _end_freq_khz == 0){
                break;
            }
            printf_debug("[%d], _fft=%u[%u],m_freq=%"PRIu64", _start_freq_khz=%u, _end_freq_khz=%u\n", i, _fft,fft_size, m_freq,  _start_freq_khz, _end_freq_khz);
            if(_fft == fft_size || _fft == 0){
                if((m_freq >= (uint64_t)_start_freq_khz*1000) &&  (m_freq < (uint64_t)_end_freq_khz *1000)){
                    freq_cal_value = poal_config->cal_level.spm_level.cal_node[i].power_level;
                    cal_value += freq_cal_value;
                    freq_found = 1;
                    break;
                }
            }
        }
    }
    
    if(freq_found){
        printf_debug("find the calibration level: %"PRIu64", %d\n", m_freq, cal_value);
    }

    cal_value += poal_config->cal_level.specturm.global_roughly_power_lever;

#ifdef CONFIG_CALIBRATION_GAIN
    int found = 0;
    /* ##NOTE: [重要]在常规模式下，0增益校准## */
    if(poal_config->channel[ch].rf_para.gain_ctrl_method != POAL_AGC_MODE && 
        poal_config->cal_level.specturm.gain_calibration_onoff == true){
        #if 0
        found = 0;
         for(i = 0; i< ARRAY_SIZE(poal_config->cal_level.rf_mode.mag); i++){
            if(poal_config->cal_level.rf_mode.mag[i].mode == POAL_NORMAL){
                cal_value += poal_config->cal_level.rf_mode.mag[i].magification*10;
                found = 1;
                break;
            }
            }
         if(found == 0){
            printf_err("Not find magification in rf normal mode!!!\n");
        }
        #endif
        /* 增益模式校准 */
        found = 0;
        mode = poal_config->channel[ch].rf_para.rf_mode_code;
        for(i = 0; i< ARRAY_SIZE(poal_config->channel[ch].rf_para.rf_mode.mag); i++){
            if(poal_config->channel[ch].rf_para.rf_mode.mode[i] == mode){
                cal_value += _get_rf_magnification(ch, mode, poal_config, m_freq)*10;
                printf_debug("after rf mode magification,mode:%d magification:%d, cal_value=%d, m_freq=%"PRIu64"\n", mode, 
                        _get_rf_magnification(ch, mode, poal_config, m_freq), cal_value, m_freq);
                found = 1;
                break;
            }
        }
        if(found == 0){
            printf_debug("RF mode Error: %d\n", mode);
                    }
        /* 衰减模式校准 */
        /* attenuation */
        found = 0;
        //struct rf_distortion_range range;
        struct db_rang_st range;
        int8_t attenuation = 0;
        attenuation = poal_config->channel[ch].rf_para.attenuation;
        for(i = 0; i< ARRAY_SIZE(poal_config->channel[ch].rf_para.rf_mode.rf_attenuation); i++){
            range.start = poal_config->channel[ch].rf_para.rf_mode.rf_attenuation[i].start;
            range.end = poal_config->channel[ch].rf_para.rf_mode.rf_attenuation[i].end;
            if(poal_config->channel[ch].rf_para.rf_mode.mode[i] == mode){
                if(attenuation >= range.start && attenuation <= range.end){
                    if(cal_value > attenuation*10)
                        cal_value -= attenuation*10;
                }
                printf_debug("after rf attenuation,mode:%d attenuation:%d, cal_value=%d\n", mode, attenuation, cal_value);
                found = 1;
                break;
            }
        }
        
        found = 0;
        attenuation = poal_config->channel[ch].rf_para.mgc_gain_value;
        for(i = 0; i< ARRAY_SIZE(poal_config->channel[ch].rf_para.rf_mode.mgc_attenuation); i++){
            range.start = poal_config->channel[ch].rf_para.rf_mode.mgc_attenuation[i].start;
            range.end = poal_config->channel[ch].rf_para.rf_mode.mgc_attenuation[i].end;
            if(poal_config->channel[ch].rf_para.rf_mode.mode[i] == mode){
                if(attenuation >= range.start && attenuation <= range.end){
                    if(cal_value > attenuation*10)
                        cal_value -= attenuation*10;
                }
                printf_debug("after rf attenuation,mode:%d attenuation:%d, cal_value=%d\n", mode, attenuation, cal_value);
                found = 1;
                break;
            }
        }
    }
#endif
    return cal_value;
}

/******************************************************************************
* FUNCTION:
*     config_save_batch
*
* DESCRIPTION:
*     根据命令批量（单个）保存对应参数
* PARAMETERS
*     cmd:  保存参数对应类型命令; 见executor.h定义
*    type:  子类型 见executor.h定义
*     data: 对应数据结构；默认专递全局配置config结构体指针
* RETURNS
*       -1:  失败
*        0：成功
******************************************************************************/

int8_t config_save_batch(exec_cmd cmd, uint8_t type,s_config *config)
{
    printf_info(" config_save_batch\n");

#ifdef CONFIG_DAO_XML
     dao_conf_save_batch(cmd,type,config);
        
#elif defined CONFIG_DAO_JSON
    json_write_config_file(config);
#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return 0;
}

int8_t config_save_all(void)
{
#ifdef CONFIG_DAO_JSON
    json_write_config_file(&config);
#endif
    return 0;
}


/******************************************************************************
* FUNCTION:
*     config_write_data
*
* DESCRIPTION:
*     根据命令和参数保存数据到config结构体; 中频参数默认保存到频点为1的参数
* PARAMETERS
*     cmd:  保存参数对应类型命令; 见executor.h定义
*     type:  子类型 见executor.h定义
*     data:   数据
* RETURNS
*       -1:  失败
*        0：成功
******************************************************************************/

int8_t config_write_data(int cmd, uint8_t type, uint8_t ch, void *data, ...)
{
    printf_info(" config_load_from_data\n");

    struct poal_config *poal_config = &(config_get_config()->oal_config);

    if(data ==NULL)
        return -1;
    
     va_list argp;
     va_start (argp, data);
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            switch(type)
            {
                case EX_MUTE_SW:
                    if(*(uint8_t *)data > 1){
                        return -1;
                    }
                    poal_config->channel[ch].multi_freq_point_param.points[0].noise_en = *(int8_t *)data;
                    break;
                case EX_MUTE_THRE:
                    poal_config->channel[ch].multi_freq_point_param.points[0].noise_thrh = *(int8_t *)data;
                    break;
                case EX_DEC_METHOD:
                    poal_config->channel[ch].multi_freq_point_param.points[0].d_method = *(int8_t *)data;
                    break;
                case EX_DEC_BW:
                    poal_config->channel[ch].multi_freq_point_param.points[0].d_bandwith = *(int32_t *)data;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                    poal_config->channel[ch].multi_freq_point_param.audio_sample_rate = *(float *)data;
                    break;
                case EX_MID_FREQ:
                    poal_config->channel[ch].multi_freq_point_param.points[0].center_freq = *(uint64_t *)data;
                    break;
                case EX_BANDWITH:
                    poal_config->channel[ch].multi_freq_point_param.points[0].bandwidth = *(uint64_t *)data;
                    break;
                case EX_AUDIO_VOL_CTRL:
                    poal_config->channel[ch].multi_freq_point_param.points[0].audio_volume = *(int16_t *)data;
                    break;
                case EX_FFT_SIZE:
                    poal_config->channel[ch].multi_freq_point_param.points[0].fft_size = *(int32_t *)data;
                    poal_config->channel[ch].multi_freq_fregment_para.fregment[0].fft_size = *(int32_t *)data;
                    /* 根据FFT设置分辨率 */
                    //poal_config->channel[ch].multi_freq_point_param.points[0].freq_resolution = config_get_resolution_by_fft(*(int32_t *)data);
                    break;
                case EX_RESOLUTION:
                    poal_config->channel[ch].multi_freq_point_param.points[0].freq_resolution = *(int32_t *)data;
                   // poal_config->channel[ch].multi_freq_point_param.points[0].fft_size = config_get_fft_by_resolution(*(int32_t *)data);
                    poal_config->channel[ch].multi_freq_fregment_para.fregment[0].freq_resolution = *(int32_t *)data;
                   // poal_config->channel[ch].multi_freq_fregment_para.fregment[0].fft_size = config_get_fft_by_resolution(*(int32_t *)data);
                    break;
                case EX_SMOOTH_TIME:
                    poal_config->channel[ch].multi_freq_point_param.smooth_time = *(int16_t *)data;
                    printf_note("smooth_time: %d\n", poal_config->channel[ch].multi_freq_point_param.smooth_time);
                    break;
                case EX_SMOOTH_TYPE:
                    poal_config->channel[ch].multi_freq_point_param.smooth_mode = *(int16_t *)data;
                    printf_note("smooth_mode: %d\n", poal_config->channel[ch].multi_freq_point_param.smooth_mode);
                    break;
                case EX_SMOOTH_THRE:
                    poal_config->channel[ch].multi_freq_point_param.smooth_threshold = *(int16_t *)data;
                    printf_note("smooth_threshold: %d\n", poal_config->channel[ch].multi_freq_point_param.smooth_threshold);
                    break;
                case EX_WINDOW_TYPE:
                    poal_config->channel[ch].multi_freq_point_param.window_type = *(int8_t *)data;
                    printf_note("window_type: %d\n", poal_config->channel[ch].multi_freq_point_param.window_type);
                    break;
                default:
                    printf_err("not surpport type\n");
                    break;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    poal_config->channel[ch].rf_para.mid_freq = *(int64_t *)data;
                    break;
                case EX_RF_MID_BW:
                    poal_config->channel[ch].rf_para.mid_bw = *(uint32_t *)data;
                    printf_note("mid_bw=%u\n", poal_config->channel[ch].rf_para.mid_bw);
                    break;
                case EX_RF_MID_FREQ_FILTER:
                    poal_config->channel[ch].rf_para.board_mid_freq = *(uint64_t *)data;
                    printf_note("board_mid_freq=%"PRIu64"\n", poal_config->channel[ch].rf_para.board_mid_freq);
                    break;
                case EX_RF_MODE_CODE:
                    poal_config->channel[ch].rf_para.rf_mode_code = *(int8_t *)data;
                    printf_note("rf_mode_code=%d\n", poal_config->channel[ch].rf_para.rf_mode_code);
                    break;
                case EX_RF_GAIN_MODE:
                    poal_config->channel[ch].rf_para.gain_ctrl_method = *(int8_t *)data;
                    printf_note("gain_ctrl_method=%d\n", poal_config->channel[ch].rf_para.gain_ctrl_method);
                    break;
                case EX_RF_AGC_CTRL_TIME:
                    poal_config->channel[ch].rf_para.agc_ctrl_time = *(uint32_t *)data;
                    printf_note("agc_ctrl_time=%u\n", poal_config->channel[ch].rf_para.agc_ctrl_time);
                    break;
                case EX_RF_AGC_OUTPUT_AMP:
                    poal_config->channel[ch].rf_para.agc_mid_freq_out_level = *(int8_t *)data;
                    printf_note("agc_mid_freq_out_level=%d\n", poal_config->channel[ch].rf_para.agc_mid_freq_out_level);
                    break;
                case EX_RF_ATTENUATION:
                    poal_config->channel[ch].rf_para.attenuation = *(int8_t *)data;
                    printf_note("attenuation=%d\n", poal_config->channel[ch].rf_para.attenuation);
                    break;
                case EX_RF_MGC_GAIN:
                    poal_config->channel[ch].rf_para.mgc_gain_value = *(int8_t *)data;
                    printf_note("mgc_gain_value=%d\n", poal_config->channel[ch].rf_para.mgc_gain_value);
                    break;
                case EX_RF_ANTENNA_SELECT:
                    break;
                default:
                    printf_err("not surpport type\n");
                    break;
            }
            break;
        }
        case EX_NETWORK_CMD:
        {
            uint32_t index = va_arg(argp, uint32_t);
            switch(type)
            {
                case EX_NETWORK_IP:
                    poal_config->network[index].addr.ipaddress = *(int32_t *)data;
                    break;
                case EX_NETWORK_MASK:
                    poal_config->network[index].addr.netmask = *(int32_t *)data;
                    break;
                case EX_NETWORK_GW:
                    poal_config->network[index].addr.gateway = *(int32_t *)data;
                    break;
                case EX_NETWORK_PORT:
                    poal_config->network[index].port = *(int16_t *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    break;
            }
            break;

        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    poal_config->ctrl_para.remote_local = *(uint8_t *)data;
                    printf_note("set ctrl mode:%s\n", (poal_config->ctrl_para.remote_local==0?"local":"remote"));
                    break;
                default:
                    printf_err("not surpport type\n");
                    break;
             }
        }
            break;
        default:
            printf_err("invalid set data[%d]\n", cmd);
            break;
     }
    va_end(argp);
    return 0;
}

int8_t config_write_save_data(exec_cmd cmd, uint8_t type, uint8_t ch, void *data)
{
    config_write_data(cmd, type, ch, data);
    config_save_batch(cmd, type, config_get_config());
    return 0;
}

int8_t config_read_by_cmd(exec_cmd cmd, uint8_t type, uint8_t ch, void *data, ...)
{
    #define DEFAULT_BW_HZ (160000000)
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ret=  -1;
    va_list argp;
    va_start (argp, data);
    printf_debug("config_read_by_cmd[%d]\n", cmd);
    if(data == NULL){
        goto exit;
    }
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            uint32_t index = va_arg(argp, uint32_t);
            switch(type)
            {
                case EX_MUTE_SW:
                    *(uint8_t *)data = poal_config->channel[ch].multi_freq_point_param.points[index].noise_en;
                    break;
                case EX_MUTE_THRE:
                     *(int8_t *)data = poal_config->channel[ch].multi_freq_point_param.points[index].noise_thrh;
                    break;
                case EX_DEC_METHOD:
                     *(uint8_t *)data = poal_config->channel[ch].multi_freq_point_param.points[index].d_method;
                    break;
                case EX_DEC_BW:
                     *(uint32_t *)data = poal_config->channel[ch].multi_freq_point_param.points[index].d_bandwith;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                     *(float *)data = poal_config->channel[ch].multi_freq_point_param.audio_sample_rate;
                    break;
                case EX_MID_FREQ:
                    *(uint64_t *)data = poal_config->channel[ch].multi_freq_point_param.points[index].center_freq;
                    break;
                case EX_BANDWITH:
                {
                    *(uint32_t *)data = poal_config->channel[ch].multi_freq_point_param.points[index].bandwidth;
                    break;
                }
                case EX_SUB_CH_MID_FREQ:
                {
                    *(uint64_t *)data = poal_config->channel[ch].sub_channel_para.sub_ch[index].center_freq;
                    break;
                }
                case EX_SMOOTH_TYPE:
                    *(uint16_t *)data = poal_config->channel[ch].multi_freq_point_param.smooth_mode;
                    break;
                default:
                    printf_err("not surpport type\n");
                    goto exit;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    *(uint64_t *)data = poal_config->channel[ch].rf_para.mid_freq;
                    break;
                case EX_RF_MID_BW:{
                    
                    struct scan_bindwidth_info *scanbw;
                    int i, found = 0;
                    scanbw = &poal_config->ctrl_para.scan_bw; 
                    for(i = 0; i < ARRAY_SIZE(scanbw->fixed_bindwidth_flag); i++){
                        if(scanbw->fixed_bindwidth_flag[i] == true){
                            *(int32_t *)data = scanbw->bindwidth_hz[i];
                            found ++;
                            break;
                    }
                    }
                    if(found == 0){
                        if(poal_config->channel[ch].rf_para.mid_bw > 0)
                            *(int32_t *)data = poal_config->channel[ch].rf_para.mid_bw;
                        else
                            *(int32_t *)data = DEFAULT_BW_HZ;
                    }
                    printf_debug("ch=%d, rf middle bw=%u, %u\n",ch, *(int32_t *)data, poal_config->channel[ch].rf_para.mid_bw);
                }
                    break;
                case EX_RF_MODE_CODE:
                    *(uint8_t *)data = poal_config->channel[ch].rf_para.rf_mode_code;
                    break;
                case EX_RF_GAIN_MODE:
                    *(uint8_t *)data = poal_config->channel[ch].rf_para.gain_ctrl_method;
                    break;
                case EX_RF_MGC_GAIN:
                    *(int8_t *)data = poal_config->channel[ch].rf_para.mgc_gain_value;
                    break;
                case EX_RF_ATTENUATION:
                    *(int8_t *)data = poal_config->channel[ch].rf_para.attenuation;
                    break;
                case EX_RF_MID_FREQ_FILTER:
                    *(uint64_t *)data = poal_config->channel[ch].rf_para.board_mid_freq;
                    break;
                
                default:
                    printf_err("not surpport type\n");
                    goto exit;
            }
            break;
        }
        case EX_NETWORK_CMD:
        {
            char *ifname = NULL;
            void *ptr = (void *)va_arg(argp,long);
            int index = 0;
            if(ptr == NULL)
                break;
            for(int i = 0; i < ARRAY_SIZE(poal_config->network); i++){
                ifname = poal_config->network[i].ifname;
                if(!strcmp((char *)ptr, ifname)){
                    index = i;
                    break;
                }
            }
            switch(type)
            {
                case EX_NETWORK_IP:
                    *(uint32_t *)data = poal_config->network[index].addr.ipaddress;
                    break;
                case EX_NETWORK_MASK:
                    *(uint32_t *)data = poal_config->network[index].addr.netmask;
                    break;
                case EX_NETWORK_GW:
                    *(uint32_t *)data = poal_config->network[index].addr.gateway;
                    break;
                case EX_NETWORK_PORT:
                    *(uint16_t *)data = poal_config->network[index].port;
                    break;
                default:
                    printf_err("not surpport type\n");
                    goto exit;
            }
            break;
        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    break;
                case EX_CTRL_SIDEBAND:
                {
                    #define DEFAULT_SIDEBAND 1.28
                    struct scan_bindwidth_info *scanbw;
                    scanbw = &poal_config->ctrl_para.scan_bw; 
                    uint32_t bw = va_arg(argp, uint32_t);
                    int found = 0;
                     if(bw == 0)
                        bw = DEFAULT_BW_HZ;
                     for(int i = 0; i<sizeof(scanbw->bindwidth_hz)/sizeof(uint32_t); i++){
                        if(scanbw->bindwidth_hz[i] == bw){
                            if(f_sgn(scanbw->sideband_rate[i]) > 0){
                                *(float *)data = scanbw->sideband_rate[i];
                                scanbw->work_sideband_rate = scanbw->sideband_rate[i];
                                found = 1;
                            }
                            break;
                        }
                    }
                    if(found == 1){
                        ret = 0;
                        printf_debug("find side rate:%f, bw=%u\n",*(float *)data,  bw);
                    }else{
                        *(float *)data = DEFAULT_SIDEBAND;
                        printf_info("not find side rate, bw=%u, use default sideband=%f\n",  bw, *(float *)data);
                        goto exit;
                    }
                    break;
                }
                case EX_CTRL_CALI_SIGAL_THRE:
                    *(int32_t *)data = poal_config->ctrl_para.signal.threshold;
                    break;
                case EX_CTRL_IQ_DATA_LENGTH:
                    *(uint32_t *)data = poal_config->ctrl_para.iq_data_length;
                    break;
                default:
                    printf_err("not surpport type\n");
                    goto exit;
             }
             break;
        }
        case EX_STATUS_CMD:
        {
            switch(type){
                case EX_CLK_STATUS:
                    *(uint8_t *)data = poal_config->status_para.clkInfo.status;
                    break;
                case EX_AD_STATUS:
                    *(uint8_t *)data = poal_config->status_para.adInfo.status;
                    break;
                case EX_DISK_STATUS:
                     *(uint8_t *)data = poal_config->status_para.diskInfo.diskNode.status;
                    break;
            }
            break;
        }
        default:
            printf_err("invalid set data[%d]\n", cmd);
            goto exit;
     }
     ret = 0;
exit:
    va_end(argp);
    return ret;
}


struct fft_windows_type_info{
    uint16_t *fptr;
    size_t f_size;
};

static struct fft_windows_type_info fwt[WINDOW_TYPE_MAX];

uint16_t *config_get_fft_window_data(int type, size_t *fsize)
{
    uint32_t *ptr = NULL;
    if(type >= WINDOW_TYPE_MAX)
        return NULL;
    *fsize = fwt[type].f_size;
    
    return fwt[type].fptr;
}


static int config_fft_window_data_init(void)
{
    char *filepath[WINDOW_TYPE_MAX];
    ssize_t ret = 0;
    struct stat fstat;
    int result = 0;
    
    filepath[WINDOW_TYPE_BLACKMAN] = "/etc/win/blackman.bin"; 
    filepath[WINDOW_TYPE_HAMMING] = "/etc/win/hamming.bin";
    filepath[WINDOW_TYPE_HANNING] = "/etc/win/hanning.bin"; 
    filepath[WINDOW_TYPE_RECT] = "/etc/win/rectwin.bin"; 
    
    for(int i = 0; i< WINDOW_TYPE_MAX; i++){
        result = stat(filepath[i], &fstat);
        if(result != 0){
            perror("Faild!");
            continue;
        }
        printfn("file:%s, size = %zd\n", filepath[i], fstat.st_size);
        fwt[i].fptr = safe_malloc(fstat.st_size + 16);
        ret = read_file_whole(fwt[i].fptr, filepath[i]);
        if(ret == -1)
            continue;
        fwt[i].f_size = ret;
        #if 0
        for(int j = 0; j < 8; j++){
            printfn("%u ", fwt[i].fptr[j]);
        }
        printfn("\n");
        #endif
    }
    return 0;
}
