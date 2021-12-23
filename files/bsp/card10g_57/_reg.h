#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>

#define FPGA_REG_DEV "/dev/xdma0_user"

#include <stdint.h>
#include "../../utils/utils.h"
#include "../../log/log.h"
#include "../../protocol/oal/poal.h"
#include "../../bsp/io.h"


#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif

#define FPGA_REG_BASE           0x00000000
#define FPGA_SYSETM_BASE        FPGA_REG_BASE
#define FPGA_STAUS_OFFSET 	    (0x1000)
#define FPGA_SPI_REG_OFFSET 	(0x3010)

#define VALID_MAX_CARD_SLOTS_NUM 4
#define START_CARD_SLOTS_NUM 2

#define CONFG_REG_LEN 0x100

#ifndef MAX_SCAN_FREQ_HZ
#define MAX_SCAN_FREQ_HZ (6000000000)
#endif


#include "platform.h"

typedef struct _SYSTEM_CONFG_REG_
{
    uint32_t version;
    uint32_t system_reset;
    uint32_t reserve1[0x2];
    uint32_t fpga_status;
    uint32_t reserve2[0x7];
    uint32_t channel_sel;
}SYSTEM_CONFG_REG;

typedef struct _STATUS_REG_
{
    uint32_t c0_load;       /* 芯片0加载结果  ，0 = 成功，其他 = 错误代码 */
    uint32_t c0_unload;     /* 芯片0卸载结果，0 = 成功，其他 = 错误代码 */
    uint32_t c1_load;       /* 芯片1加载结果  ，0 = 成功，其他 = 错误代码 */
    uint32_t c1_unload;     /* 芯片1卸载结果，0 = 成功，其他 = 错误代码 */
    uint32_t board_type;    /* 0:未知, 1: 威风，2：非威风*/
    uint32_t board_status;  /* 0 = 故障 1 = 正常 */
    uint32_t link_status;   /* link状态   0 = 故障，1 = 正常 */
    uint32_t recv;
    uint32_t version;       /* 版本号 */
    uint32_t fmc_status;    /* FMC子卡状态 */
    uint32_t board_addr_status;    /* 0: 拨码开关错误 1：拨码开关正确 */
}STATUS_REG;

typedef struct _SPI_REG_
{
    uint32_t data;          /* 数据寄存器 */
    uint32_t cmd;           /* 执行寄存器 */
    uint32_t read;          /* 回读寄存器 */
    uint32_t status;        /* SPI状态寄存器 */
}SPI_REG;



typedef struct _FPGA_CONFIG_REG_
{
    SYSTEM_CONFG_REG *system;
    STATUS_REG *status[MAX_FPGA_CARD_SLOT_NUM];
    SPI_REG *rf_reg;
}FPGA_CONFIG_REG;


/*****system*****/
/*GET*/
#define GET_SYS_FPGA_VER(reg)				(reg->system->version)
#define GET_SYS_FPGA_STATUS(reg)			(reg->system->fpga_status)
#define GET_SYS_FPGA_BOARD_VI(reg)			
#define GET_CHANNEL_SEL(reg)				(reg->system->channel_sel)

/*SET*/
#define SET_SYS_RESET(reg,v) 				(reg->system->system_reset=v)
#define SET_SYS_IF_CH(reg,v) 				
#define SET_CHANNEL_SEL(reg, v)				(reg->system->channel_sel = v)
//#define SET_SYS_SSD_MODE(reg,v) 			(reg->system->ssd_mode=v)

/* RF */
#define SET_RF_BYTE_DATA(reg, v)             (reg->rf_reg->data=v)
#define SET_RF_BYTE_CMD(reg, v)              (reg->rf_reg->cmd=v)
#define GET_RF_STATUS(reg)                   (reg->rf_reg->status)
#define GET_RF_BYTE_READ(reg)                (reg->rf_reg->read)


/*****broad band*****/
/*GET*/
#define GET_BROAD_AGC_THRESH(reg, ch) 			
/*SET*/
#define SET_BROAD_SIGNAL_CARRIER(reg,v, ch) 	
#define SET_BROAD_ENABLE(reg,v, ch) 			
#define SET_BROAD_BAND(reg,v, ch) 				
#define SET_BROAD_FIR_COEFF(reg,v, ch) 			
/*fft smooth */
#define SET_FFT_SMOOTH_TYPE(reg,v, ch) 			
#define SET_FFT_MEAN_TIME(reg,v, ch) 			
#define SET_FFT_CALIB(reg,v, ch) 				
#define SET_FFT_FFT_LEN(reg,v, ch) 				

/*****narrow band*****/
/*GET*/
#define GET_NARROW_SIGNAL_VAL(reg,id) 		
/*SET*/
#define SET_NARROW_BAND(reg,id,v) 			
#define SET_NARROW_FIR_COEFF(reg,id,v) 		
#define SET_NARROW_DECODE_TYPE(reg,id,v) 	
//#define SET_NARROW_ENABLE(reg,id,v) 		(reg->narrow_band[id]->enable=v)
#define SET_NARROW_SIGNAL_CARRIER(reg,id,v) 
#define SET_NARROW_NOISE_LEVEL(reg,id,v) 	

/* others */
#define SET_CURRENT_TIME(reg,v)			    
#define SET_DATA_RESET(reg,v)			    
#define SET_CURRENT_COUNT(reg,v)		    
#define SET_TRIG_TIME(reg,v)			   
#define SET_TRIG_COUNT(reg,v)			    

#define GET_CURRENT_TIME(reg)			0
#define GET_CURRENT_COUNT(reg)			0


#define AUDIO_REG(reg)                      (void*)0

static inline void _set_xdma_channel(FPGA_CONFIG_REG *reg, int ch,  int enable)
{
    uint32_t ch_map = GET_CHANNEL_SEL(reg);
    if(enable){
        ch_map |= (1 << ch);
    } else {
        ch_map &= ~(1 << ch);
    }
   // printf_note("ch:%d, ch_map:0x%x\n", ch, ch_map);
    SET_CHANNEL_SEL(reg, ch_map);
}


static inline void _set_narrow_channel(FPGA_CONFIG_REG *reg, int ch, int subch, int enable)
{
}

static inline uint64_t _get_middle_freq_by_channel(int ch, void *args)
{
    return 0;
}

static inline uint64_t _get_middle_freq(int ch, uint64_t rang_freq, void *args)
{
    return 0;
}


static inline void _set_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
{}

/*
    @back:1 playback  0: normal
*/
static inline void _set_ssd_mode(FPGA_CONFIG_REG *reg, int ch,int back)
{}

static inline void set_rf_safe(int ch, uint32_t *reg, uint32_t val)
{}

static inline uint32_t get_rf_safe(int ch, uint32_t *reg)
{
    return 0;
}

static inline int _get_channel_by_mfreq(uint64_t rang_freq, void *args)
{
    return 0;
}
static inline void _set_biq_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
{}



/* RF */
/*GET*/
/* 
@ch: rf control channel
@index: a channel may have multiple RF controls
*/
static inline int32_t _reg_get_rf_temperature(int ch, int index, FPGA_CONFIG_REG *reg)
{
    return 0;
}


/* 获取射频不同工作模式下放大倍数 
    @ch: 工作通道
    @index: 射频模式
    @args: 通道参数指针
*/
static inline int32_t _get_rf_magnification(int ch, int index,void *args, uint64_t mid_freq)
{
    return 0;
}


static inline bool _reg_get_rf_ext_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    return false;
}

static inline bool _reg_get_rf_lock_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    return false;
}

/* RF */
/*SET*/
static inline void _reg_set_rf_frequency(int ch, int index, uint64_t freq_hz, FPGA_CONFIG_REG *reg)
{}

static inline void _reg_set_rf_bandwidth(int ch, int index, uint32_t bw_hz, FPGA_CONFIG_REG *reg)
{}

static inline void _reg_set_rf_mode_code(int ch, int index, uint8_t code, FPGA_CONFIG_REG *reg)
{}

static inline void _reg_set_rf_mgc_gain(int ch, int index, uint8_t gain, FPGA_CONFIG_REG *reg)
{}


static inline void _reg_set_rf_attenuation(int ch, int index, uint8_t atten, FPGA_CONFIG_REG *reg)
{}

static inline void _reg_set_rf_cali_source_attenuation(int ch, int index, uint8_t level, FPGA_CONFIG_REG *reg)
{}

static inline void _reg_set_rf_direct_sample_ctrl(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{}

static inline void _reg_set_rf_cali_source_choise(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{}

//#平台信息查询回复命令(0x17)
static inline int _reg_get_fpga_info(FPGA_CONFIG_REG *reg,int id, void **args)
{
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00};
    uint8_t *ptr;
    ptr = malloc(sizeof(data));
    if(ptr == NULL){
        printf_err("malloc err\n");
        return -1;
    }
    memcpy(ptr, data, sizeof(data));
    cards_status_set(5);
    *args = (void *)ptr;
    return sizeof(data);
}

/* 获取处理卡版本 */
static inline uint32_t _reg_get_fpga_version(FPGA_CONFIG_REG *reg, int slot_id, void **args)
{
    if(slot_id < START_CARD_SLOTS_NUM || slot_id > START_CARD_SLOTS_NUM+VALID_MAX_CARD_SLOTS_NUM)
        return false;

    return reg->status[slot_id]->version;
}


/* 获取FMC子卡状态 */
static inline int _reg_get_fpga_fmc_status(FPGA_CONFIG_REG *reg, int slot_id, void **args)
{
    if(slot_id < START_CARD_SLOTS_NUM || slot_id > START_CARD_SLOTS_NUM+VALID_MAX_CARD_SLOTS_NUM)
        return false;

    return (reg->status[slot_id]->fmc_status);
}


/* 获取处理卡拨码开关状态 */
static inline bool _reg_is_fpga_addr_ok(FPGA_CONFIG_REG *reg, int slot_id, void **args)
{
    if(slot_id < START_CARD_SLOTS_NUM || slot_id > START_CARD_SLOTS_NUM+VALID_MAX_CARD_SLOTS_NUM)
        return false;

    return (reg->status[slot_id]->board_addr_status == 0 ? false : true);
}

/* 获取处理卡类型和槽位状态 */
static inline int _reg_get_fpga_info_(FPGA_CONFIG_REG *reg, int id, void **args)
{
    uint8_t *ptr, *pstatus;

    pstatus = calloc(1, MAX_FPGA_CARD_SLOT_NUM*2);
    if(pstatus == NULL){
        printf_err("malloc err\n");
        return -1;
    }
    ptr = pstatus;
    ptr += START_CARD_SLOTS_NUM;
    for(int i = START_CARD_SLOTS_NUM; i < START_CARD_SLOTS_NUM+VALID_MAX_CARD_SLOTS_NUM; i++){
        //printfi("[%d]%p, %p, %x\n", i, reg->status[i], &reg->status[i]->board_type, reg->status[i]->board_type);
        *ptr++ = reg->status[i]->board_type;
    }
    ptr = pstatus;
    ptr += MAX_FPGA_CARD_SLOT_NUM;
    ptr += START_CARD_SLOTS_NUM;
    for(int i = START_CARD_SLOTS_NUM; i < START_CARD_SLOTS_NUM+VALID_MAX_CARD_SLOTS_NUM; i++){
        *ptr++ = reg->status[i]->board_status;
        //printfi("[%d]%p, %p, %x\n", i, reg->status[i], &reg->status[i]->board_status, reg->status[i]->board_status);
        if(reg->status[i]->board_status)
            cards_status_set(i);
    }
    ptr = pstatus;
    for(int i = 0; i <MAX_FPGA_CARD_SLOT_NUM*2; i++){
        printfi("%02x ", *ptr++);
    }
    printfi("\n");
    *args = (void *)pstatus;
    return (MAX_FPGA_CARD_SLOT_NUM*2);
}

//加载命令结果查询回复
static inline int _reg_get_load_result(FPGA_CONFIG_REG *reg, int id, void **args)
{
    int slot_id = 0, chip_id = 0;
    int ret = -1, try_count = 0;
    
    chip_id = id & 0x0ff;
    slot_id = (id >> 8) & 0x0f;
    if(chip_id > 0)
        chip_id -= 1;
    
    if(chip_id >= MAX_FPGA_CHIPID_NUM){
        printf_err("chip id[%d] is bigger than max[%d]\n", chip_id, MAX_FPGA_CHIPID_NUM);
        return ret;
    }
    if(slot_id >= MAX_FPGA_CARD_SLOT_NUM){
        printf_err("slot id[%d] is bigger than max[%d]\n", slot_id, MAX_FPGA_CARD_SLOT_NUM);
        return ret;
    }
    printf_info("id=0x%x, chip_id=0x%x, slot_id=0x%x\n", id, chip_id, slot_id);
    do{
        usleep(100000);
        if(chip_id == 0){
            ret = reg->status[slot_id]->c0_load;
        } else {
            ret = reg->status[slot_id]->c1_load;
        }
        printf_note("result bit: %p, %p, %p\n", reg->status[slot_id], &reg->status[slot_id]->c0_load, &reg->status[slot_id]->c1_load);
        printf_note("load result: 0x%x[slot_id:0x%x, chip_id:0x%x], try_count:%d\n", ret, slot_id, chip_id, try_count);
    }while(ret != 1 && try_count++ < 8);
    printf_warn(">>>>>>CARD[0x%04x] LOAD %s!!<<<<<\n", id, ret == 1 ? "OK" : "Faild");
    return (ret == 1 ? 0 : -1);
}

static inline int _reg_get_unload_result(FPGA_CONFIG_REG *reg, int id, void **args)
{
    int slot_id = 0, chip_id = 0;
    int ret = -1, try_count = 0;
    
    chip_id = id & 0x0ff;
    slot_id = (id >> 8) & 0x0f;
    if(chip_id > 0)
        chip_id -= 1;
    
    if(chip_id >= MAX_FPGA_CHIPID_NUM){
        printf_err("chip id[%d] is bigger than max[%d]\n", chip_id, MAX_FPGA_CHIPID_NUM);
        return ret;
    }
    if(slot_id >= MAX_FPGA_CARD_SLOT_NUM){
        printf_err("slot id[%d] is bigger than max[%d]\n", slot_id, MAX_FPGA_CARD_SLOT_NUM);
        return ret;
    }
    printf_info("id=0x%x, chip_id=0x%x, slot_id=0x%x\n", id, chip_id, slot_id);
    do{
        usleep(100000);
        if(chip_id == 0){
            ret = reg->status[slot_id]->c0_load;
        } else {
            ret = reg->status[slot_id]->c1_load;
        }
        printf_note("result bit: %p, %p, %p\n", reg->status[slot_id], &reg->status[slot_id]->c0_load, &reg->status[slot_id]->c1_load);
        printf_note("unload result: 0x%x[slot_id:0x%x, chip_id:0x%x], try_count:%d\n", ret, slot_id, chip_id, try_count);
    }while(ret == 1 && try_count++ < 8);
    printf_warn(">>>>>>CARD[0x%04x] UNLOAD %s!!<<<<<\n", id, ret != 1 ? "OK" : "Faild");
    return (ret != 1 ? 0 : -1);
}

//link状态查询
static inline int _reg_get_link_result(FPGA_CONFIG_REG *reg, int id, void **args)
{
    int slot_id = 0, chip_id = 0;
    int ret = -1, try_count = 0;
    
    chip_id = id & 0x0ff;
    slot_id = (id >> 8) & 0x0f;
    if(chip_id > 0)
        chip_id -= 1;
    
    if(chip_id >= MAX_FPGA_CHIPID_NUM){
        printf_err("chip id[%d] is bigger than max[%d]\n", chip_id, MAX_FPGA_CHIPID_NUM);
        return ret;
    }
    if(slot_id >= MAX_FPGA_CARD_SLOT_NUM){
        printf_err("slot id[%d] is bigger than max[%d]\n", slot_id, MAX_FPGA_CARD_SLOT_NUM);
        return ret;
    }
    printf_info("id=0x%x, chip_id=0x%x, slot_id=0x%x\n", id, chip_id, slot_id);
    do{
        usleep(1000);
        ret = reg->status[slot_id]->link_status;
    }while(ret != 1 && try_count++ < 8);
    printf_warn(">>>>>>CARD[0x%04x] Link %s!!<<<<<\n", id, ret == 1 ? "OK" : "Faild");
    return (ret == 1 ? 0 : -1);
}


//卸载命令结果查询回复
static inline int _reg_get_unload_result_(FPGA_CONFIG_REG *reg, int id, void **args)
{
    int slot_id = 0, chip_id = 0;
    int ret = -1, try_count = 0;
    
    chip_id = id & 0x0ff;
    slot_id = (id >> 8) & 0x0f;
    printf_note("id=0x%x, chip_id=0x%x, slot_id=0x%x\n", id, chip_id, slot_id);
    if(chip_id > 0)
        chip_id -= 1;
    
    if(chip_id >= MAX_FPGA_CHIPID_NUM){
        printf_err("chip id[%d] is bigger than max[%d]\n", chip_id, MAX_FPGA_CHIPID_NUM);
        return ret;
    }
    if(slot_id >= MAX_FPGA_CARD_SLOT_NUM){
        printf_err("slot id[%d] is bigger than max[%d]\n", slot_id, MAX_FPGA_CARD_SLOT_NUM);
        return ret;
    }
    printf_note("id=0x%x, chip_id=0x%x, slot_id=0x%x\n", id, chip_id, slot_id);
    do{
        usleep(100);
        if(chip_id == 0){
            ret = reg->status[slot_id]->c0_unload;
        } else {
            ret = reg->status[slot_id]->c1_unload;
        }
        printf_note("load result: %d[id:%d,slot_id:%d], try_count:%d\n", ret, chip_id, slot_id, try_count);
    }while(ret < 0 && try_count++ < 3);
    
    return (ret == 1 ? 0 : -1);;
}




//读取板卡信息回复(0x13)
static inline int _reg_get_board_info(int id, void **args)
{
    //0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    uint8_t data[] = {0x0f, 0x77, 0xba, 
        0xf6, 0x45, 0x1a, 0x81, 0x5e, 0x6d, 0xa5, 0x32, 0x9b, 0x52, 0xfa, 0x2d, 0xe3, 0x05, 
        0x02, 0xd9, 0xe3, 0x26, 0x9e, 0x28, 0x7f, 0xb0, 0x26, 0x30, 0x46, 0xd4, 0xb1, 0x45,
        0x07, 0x43, 0x62, 0x0b, 0x5c, 0x9d, 0x95, 0xe1, 0x35, 0xc2, 0x9c, 0x7d, 0xde, 0xcb, 
        0xf9, 0x45, 0x12, 0x06, 0x5b, 0xe8, 0x11, 0xcc, 0x4f, 0x85, 0x1c, 0x43, 0x23, 0x00, 
        0x87, 0xf4, 0xaa, 0x09, 0x3b, 0x2d, 0xae, 0x4e, 0x5f, 0xdd, 0x20, 0x64, 0xcd, 0xbe,
        0xc5, 0x25, 0x89, 0x08, 0xfa, 0x4a, 0x82, 0xc7, 0x0c, 0x2c, 0x2b, 0x6d, 0xbb, 0x3c,
        0x91, 0xec, 0xa3, 0xd1, 0x1b, 0x14, 0x8d, 0xbc, 0xc3, 0x51, 0xaa, 0x58, 0x6b, 0xb5, 
        0x6d, 0x67, 0xb5, 0xf9, 0x54, 0xd4, 0x24, 0xb9, 0x77, 0x84, 0xce, 0xb9, 0xda, 0x2f,
        0xc2, 0xef, 0x8b, 0x6d, 0x66, 0xd9, 0x99, 0xbc, 0x68, 0x6b, 0x17, 0x85, 0xb7, 0x53, 
        0x88, 0xef, 0xaa, 0x53, 0x6f, 0xab, 0xc9, 0x57, 0xf0, 0x33, 0x92, 0x9f, 0xb5, 0xe0, 
        0xbc, 0x6f, 0x76, 0x99, 0xfc, 0xe5, 0x1f, 0x6c, 0xf6, 0xb1, 0xda, 0x93, 0x73, 0xc1, 
        0xe9, 0xf7, 0x15, 0x4e, 0xf1, 0xe0, 0xdd, 0x89, 0x1e, 0x10, 0x62, 0x94, 0x19, 0x76, 
        0xa9, 0x68, 0xc7, 0xc1, 0x77, 0x56, 0x69, 0x23, 0x24, 0x20, 0x7e, 0x3f, 0x6f, 0x67, 
        0x30, 0x23, 0x5e, 0xb9, 0x12, 0x65, 0x0d, 0x57, 0x3b, 0x9a, 0x53, 0xea, 0xb3, 0xba, 
        0xe5, 0x97, 0x4d, 0xb7, 0x3a, 0xff, 0x35, 0x6b, 0x59, 0x66, 0xe5, 0x70, 0x7f, 0xc3, 
        0x8e, 0x5a, 0x11, 0xb0, 0xef, 0x0d, 0xb5, 0x44, 0xb5, 0xda, 0x21, 0xc0, 0x91, 0x40, 
        0xd5, 0x9e, 0x9a, 0x92, 0x77, 0x1f, 0x58, 0x7b, 0xad, 0xad, 0x4c, 0xf6, 0x81, 0x3e, 
        0xb6, 0xda, 0xbf, 0x8b, 0x8f, 0xa9, 0x4c, 0xbc, 0x65, 0x88, 0x5f, 0x88, 0xc9, 0x42, 
        0x2d, 0x47, 0x76, 0xc5, 0xde, 0xc3, 0x42, 0x6b, 0xb9, 0xef, 0xad, 0x59, 0x1c, 0x67, 
        0x09, 0xc4, 0x69, 0x18, 0xdf, 0xc5, 0x9d, 0x71, 0x56, 0xb6, 0xe5, 0xd4, 0xc0, 0x8f, 
        0x68, 0xc9, 0x52, 0x11, 0x27, 0x57, 0x15, 0xdc, 0xc2, 0xf5, 0x1e, 0x97, 0x60, 0xb7, 
        0x53, 0x51, 0x6d, 0x0b, 0xc2, 0xd7, 0xe7, 0xd3, 0xa6, 0xe8, 0x5c, 0xf4, 0xfb, 0x2c, 
        0xac, 0xbd, 0xf1, 0x6a, 0xbc, 0xc4, 0xd8, 0x53, 0x1d};
    
    uint8_t *ptr;
    ptr = malloc(sizeof(data));
    if(ptr == NULL){
        printf_err("malloc err\n");
        return -1;
    }
    memcpy(ptr, data, sizeof(data));

    *args = (void *)ptr;
    return sizeof(data);
    
}

static inline int _get_set_load(void)
{
    return 0;
}

extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif

