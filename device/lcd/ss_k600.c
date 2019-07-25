#include "config.h"

static SCREEN_BW_PARA_ST bw_data[] = {
    {SCREEN_BW_0, 600},
    {SCREEN_BW_1, 1500},
    {SCREEN_BW_2, 2400},
    {SCREEN_BW_3, 5000},
    {SCREEN_BW_4, 6000},
    {SCREEN_BW_5, 9000},
    {SCREEN_BW_6, 12000},
    {SCREEN_BW_7, 15000},
    {SCREEN_BW_8, 25000},
    {SCREEN_BW_9, 50000},
    {SCREEN_BW_10, 150000},
};

int8_t k600_decode_method_convert_to_standard(uint8_t method)
{
    uint8_t d_method;
    
    if(method == SCREEN_DECODE_AM){
        d_method = IO_DQ_MODE_AM;
    }else if(method == SCREEN_DECODE_FM || method == SCREEN_DECODE_WFM) {
        d_method = IO_DQ_MODE_FM;
    }else if(method == SCREEN_DECODE_LSB || method == SCREEN_DECODE_USB) {
        d_method = IO_DQ_MODE_LSB;
    }else if(method == SCREEN_DECODE_CW) {
        d_method = IO_DQ_MODE_CW;
    }else{
        printf_err("decode method not support:%d\n",method);
        return -1;
    }
    return d_method;
}

int8_t k600_decode_method_convert_from_standard(uint8_t method)
{
    uint8_t d_method;
    
    if(method == IO_DQ_MODE_AM){
        d_method = SCREEN_DECODE_AM;
    }else if(method == IO_DQ_MODE_FM || method == IO_DQ_MODE_WFM) {
        d_method = SCREEN_DECODE_FM;
    }else if(method == IO_DQ_MODE_LSB || method == IO_DQ_MODE_USB) {
        d_method = SCREEN_DECODE_LSB;
    }else if(method == IO_DQ_MODE_CW) {
        d_method = SCREEN_DECODE_CW;
    }else{
        printf_err("decode method not support:%d\n",method);
        return -1;
    }
    return d_method;
}



static int k600_receive_read_act(uint16_t addr, uint8_t short_len)
{
    SCREEN_SEND_CONTROL_ST *send_package;
    uint8_t send_buf[32] = {0};

    send_package = (SCREEN_SEND_CONTROL_ST *)send_buf;
    memset(send_buf, 0, sizeof(send_buf));

    send_package->start_flag = htons(SERIAL_SCREEN_STAT_FLAG);
    send_package->addr = htons(addr);
    send_package->type = READ_SCREEN_ADDR;
    send_package->payload.datanum = short_len;
    send_package->len = sizeof(SCREEN_SEND_CONTROL_ST) - sizeof(send_package->start_flag) - sizeof(SCREEN_SEND_V_DATA);

    printf_debug("send read data:\n");   
    for(int i = 0; i< send_package->len + 3; i++){
        printfd("0x%02x ", send_buf[i]);
    }
    printfd("\n");
    //send_data_by_serial(driver->serial_fd[SERIAL_SCREEN_INDEX], send_buf, send_package->len + 3);
    return 0;
}

static int k600_receive_write_act(uint16_t addr, void *data, uint8_t short_len)
{
    SCREEN_SEND_CONTROL_ST *send_package;
    uint8_t send_buf[32] = {0};

    if(data == NULL){
        printf_err("data null!\n");
        return -1;
    }
    send_package = (SCREEN_SEND_CONTROL_ST *)send_buf;
    memset(send_buf, 0, sizeof(send_buf));

    send_package->start_flag = htons(SERIAL_SCREEN_STAT_FLAG);
    send_package->addr = htons(addr);
    send_package->type = WRITE_SCREEN_ADDR;

    if(sizeof(send_package->payload.data) < short_len){
        printf_err("data is too long!\n");
        return -1;
    }
    memset(send_package->payload.data, 0, sizeof(short_len *2));

    send_package->len = sizeof(SCREEN_SEND_CONTROL_ST) - sizeof(send_package->start_flag) -sizeof(SCREEN_SEND_V_DATA) + short_len * 2 -1;
    memcpy((uint8_t *)send_package->payload.data, (uint8_t *)data, short_len * 2);	

    printf_debug("send write data:\n");   
    for(int i = 0; i< send_package->len + 3; i++){
        printfd("0x%02x ", send_buf[i]);
    }
    printfd("\n");


   // send_data_by_serial(driver->serial_fd[SERIAL_SCREEN_INDEX], send_buf, send_package->len + 3);
    return 0;
}


int k600_receive_read_cmd_from_user(uint8_t data_type, uint8_t data_cmd)
{
    uint8_t data_len = 0;

    switch (data_type)
    {
        case SCREEN_COMMON_DATA1:
        {
            switch (data_cmd)
            {
                case SCREEN_CTRL_TYPE:
                case SCREEN_PORT:
                data_len = 1;
                break;
                case SCREEN_IPADDR:
                case SCREEN_NETMASK_ADDR:
                case SCREEN_GATEWAY_ADDR:
                {
                data_len = 2;
                }
                break;
            }
            k600_receive_read_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd),data_len);
            break;
        }
        case SCREEN_COMMON_DATA2:
        break;
        case SCREEN_CHANNEL1_DATA:
        case SCREEN_CHANNEL2_DATA:
        case SCREEN_CHANNEL3_DATA:
        case SCREEN_CHANNEL4_DATA:
        case SCREEN_CHANNEL5_DATA:
        case SCREEN_CHANNEL6_DATA:
        case SCREEN_CHANNEL7_DATA:
        case SCREEN_CHANNEL8_DATA:
        {
            printf_debug("channel[%x], action[%x]\n", data_type, data_cmd);
            switch (data_cmd){
            case SCREEN_CHANNEL_BW:
            case SCREEN_CHANNEL_NOISE_EN:
                case SCREEN_CHANNEL_MODE:
                case SCREEN_CHANNEL_DECODE_TYPE:
                case SCREEN_CHANNEL_VOLUME:
                case SCREEN_CHANNEL_MGC:
                case SCREEN_CHANNEL_AU_CTRL:
                case SCREEN_CHANNEL_AU_BUTTON_CTRL:
                    data_len = 1;
                    break;
                case SCREEN_CHANNEL_FRQ:
                    data_len = 2;
                break;
                    default:
                    printf_err("data type[%02x] error!!\n", data_type);
            }
            k600_receive_read_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd),data_len);
            break;
        }
        default:
            printf_err("data type[%02x] error!!\n", data_type);
            break;
    }
    return 0;
}



int8_t k600_send_data_to_user(uint8_t *pdata, int32_t total_len)
{
    SCREEN_READ_CONTROL_ST data, *ptr;
    uint8_t data_type,data_cmd;
    struct in_addr ip;
    char *ipstr=NULL;

    if(pdata == NULL){
        return -1;
    }

    memcpy((uint8_t *)&data, pdata, sizeof(SCREEN_READ_CONTROL_ST));

    ptr = (SCREEN_READ_CONTROL_ST *)pdata;

    ptr->start_flag = htons(ptr->start_flag);
    ptr->addr = htons(ptr->addr);

    printf_debug("start flag[%04x] addr[%04x] len=%d, total_len=%d\n", ptr->start_flag, ptr->addr, ptr->len, total_len);

    if(ptr->start_flag != SERIAL_SCREEN_STAT_FLAG){
        printf_err("start flag[%04x] error!!\n", ptr->start_flag);
        return -1;
    }

    if(ptr->type != READ_SCREEN_ADDR){
        printf_err("read type[%02x] error!!\n", ptr->type);
        return -1;
    }
    printf_debug("len=%d, total_len=%d\n", ptr->len, total_len);
    if(ptr->len + SERIAL_SCREEN_HEAD_LEN != total_len){
        printf_err("received data length [%02x] error!!\n", total_len);
        return -1;
    }

    if(ptr->datanum * 2 > sizeof(ptr->data)){
        printf_err("datanum[%02x] is too long!!\n", ptr->datanum);
        return -1;
    }
    for(int i = 0; i < ptr->datanum; i++){
        ptr->data[i] = htons(ptr->data[i]);
    }
    data_type = GET_HIGH_8_BIT_16BIT(ptr->addr);
    data_cmd =  GET_LOW_8_BIT_16BIT(ptr->addr);

    printf_debug("received  data_type[%x] cmd[%x]\n", data_type, data_cmd);
    //if(REMOTE_CONTROL_MODE() && data_type != SCREEN_COMMON_DATA1 && data_cmd != SCREEN_CTRL_TYPE){
    //     printf_err("Remote Control Mode, Ignore Command\n"); 
    //     return 1;
    // }
    switch (data_type){
    case SCREEN_COMMON_DATA1:
    {
        switch (data_cmd)
        {
            case SCREEN_CTRL_TYPE:
            {
                uint8_t ctrl_data;
                ctrl_data = ptr->data[0];
            }
            break;
            case SCREEN_IPADDR1:
            case SCREEN_IPADDR2:
            case SCREEN_IPADDR3:
            case SCREEN_IPADDR4:
            {
                uint32_t ipaddr;
                if(config_read_by_cmd(EX_NETWORK_CMD, EX_NETWORK_IP, 0, &ipaddr) != 0){
                    return -1;
                }
                printf_debug("data_cmd=%x, datanum=%d, pdata=%d[%x], pdata2=%d\n", data_cmd,ptr->datanum,  ptr->data[0],ptr->data[0], ptr->data[1]);
                memcpy((uint8_t *)(&ipaddr) + data_cmd - SCREEN_IPADDR1, &(ptr->data[0]), ptr->datanum);
                ip.s_addr = ipaddr;
                ipstr= inet_ntoa(ip);
                printf_info("ipstr=%s ipaddr=%x\n", ipstr,  ip.s_addr);
                config_refresh_data(EX_NETWORK_CMD, EX_NETWORK_IP, 0, &ipaddr);
                executor_set_command(EX_NETWORK_CMD, EX_NETWORK_IP, 0, NULL);
            }
            break;
            case SCREEN_NETMASK_ADDR1:
            case SCREEN_NETMASK_ADDR2:
            case SCREEN_NETMASK_ADDR3:
            case SCREEN_NETMASK_ADDR4:
            {
                uint32_t subnetmask;
                if(config_read_by_cmd(EX_NETWORK_CMD, EX_NETWORK_MASK, 0, &subnetmask) != 0){
                    return -1;
                }
                printf_debug("data_cmd=%x, datanum=%d, pdata=%d[%x], pdata2=%d\n", data_cmd,ptr->datanum,  ptr->data[0],ptr->data[0], ptr->data[1]);
                memcpy((uint8_t *)(&subnetmask) + data_cmd - SCREEN_NETMASK_ADDR1, &(ptr->data[0]), ptr->datanum);
                ip.s_addr = subnetmask;
                ipstr= inet_ntoa(ip);
                printf_info("subnetmaskstr=%s subnetmask=%x\n", ipstr,  ip.s_addr);
                config_refresh_data(EX_NETWORK_CMD, EX_NETWORK_MASK, 0, &subnetmask);
                executor_set_command(EX_NETWORK_CMD, EX_NETWORK_MASK, 0, NULL);

            }
            break;
            case SCREEN_GATEWAY_ADDR1:
            case SCREEN_GATEWAY_ADDR2:
            case SCREEN_GATEWAY_ADDR3:
            case SCREEN_GATEWAY_ADDR4:
            {
                uint32_t gateway;
                if(config_read_by_cmd(EX_NETWORK_CMD,  EX_NETWORK_GW, 0, &gateway) != 0){
                    return -1;
                }
                printf_debug("data_cmd=%x, datanum=%d, pdata=%d[%x], pdata2=%d\n", data_cmd,ptr->datanum,  ptr->data[0],ptr->data[0], ptr->data[1]);
                memcpy((uint8_t *)(&gateway) + data_cmd - SCREEN_GATEWAY_ADDR1, &(ptr->data[0]), ptr->datanum);
                ip.s_addr = gateway;
                ipstr= inet_ntoa(ip);
                printf_info("gatewaystr=%s gateway=%x\n", ipstr,  ip.s_addr);
                config_refresh_data(EX_NETWORK_CMD,  EX_NETWORK_GW, 0, &gateway);
                executor_set_command(EX_NETWORK_CMD,  EX_NETWORK_GW, 0, NULL);

            }
            break;
            case SCREEN_PORT:
            {   
                uint32_t PORTT;
                if(config_read_by_cmd(EX_NETWORK_CMD,  EX_NETWORK_PORT, 0, &PORTT) != 0){
                    return -1;
                }
                printf_debug("data_cmd=%x, datanum=%d, pdata=%d[%x], pdata2=%d\n", data_cmd,ptr->datanum,  ptr->data[0],ptr->data[0], ptr->data[1]);
                ip.s_addr = PORTT;
                ipstr= inet_ntoa(ip);
                printf_info("PORTTstr=%s PORTT=%x\n", ipstr,  ip.s_addr);
                config_refresh_data(EX_NETWORK_CMD,  EX_NETWORK_PORT, 0, &PORTT);
                executor_set_command(EX_NETWORK_CMD,  EX_NETWORK_PORT, 0, NULL);
            }
        }
    }
        break;
    case SCREEN_COMMON_DATA2:
        printf_warn("invalid received cmd[%x]\n", data_type);
        break;
    case SCREEN_CHANNEL1_DATA:
    case SCREEN_CHANNEL2_DATA:
    case SCREEN_CHANNEL3_DATA:
    case SCREEN_CHANNEL4_DATA:
    case SCREEN_CHANNEL5_DATA:
    case SCREEN_CHANNEL6_DATA:
    case SCREEN_CHANNEL7_DATA:
    case SCREEN_CHANNEL8_DATA:
    {
        uint16_t i_data;
        uint8_t channel = data_type;
        printf_debug("channel[%x], action[%x]\n", data_type, data_cmd);
        switch (data_cmd){
            case SCREEN_CHANNEL_BW:
            {   
                uint32_t bw;
                i_data = ptr->data[0];
                printf_debug("ch = %d, SCREEN_CHANNEL_BW[%d]\n",  channel, i_data);
                for(uint32_t i = 0; i<sizeof(bw_data)/sizeof(SCREEN_BW_PARA_ST); i++){
                    if(bw_data[i].type == (uint8_t)i_data){
                        printf_debug("rec bw data %uHz\n", bw_data[i].idata);
                        bw = (uint32_t)(bw_data[i].idata);
                        break;
                    }
                }
            }
                break;
            
            case SCREEN_CHANNEL_MODE:
            case SCREEN_CHANNEL_NOISE_EN:
            case SCREEN_CHANNEL_DECODE_TYPE:
            {   
                i_data = ptr->data[0];
                printf_debug("ch = %d, [%d]\n",  channel, i_data);
                /* the spi noise mode setting is different with screen protocal 
                    so need to be transfer.
                */
                if(data_cmd == SCREEN_CHANNEL_MODE){
                    /*
                        1: low distortion mode of operation
                        2: Normal working mode
                        3: Low noise mode of operation
                    */
                    i_data = i_data + 1;
                }else if(data_cmd == SCREEN_CHANNEL_DECODE_TYPE){
                    if(i_data   == SCREEN_DECODE_AM){
                        i_data  = DQ_MODE_AM;
                    }else if(i_data  == SCREEN_DECODE_FM || i_data  == SCREEN_DECODE_WFM){
                        i_data  = DQ_MODE_FM;
                    }else if(i_data  == SCREEN_DECODE_CW){
                        i_data  = DQ_MODE_CW;
                    }else if(i_data  == SCREEN_DECODE_LSB || i_data == SCREEN_DECODE_USB){
                        i_data  = DQ_MODE_USB;
                    }else{
                        printf_err("error decode type!!\n");
                    }
                }
            }
                break;
            case SCREEN_CHANNEL_FRQ:
            {   
                uint32_t i_freq_khz;
                uint64_t i_freq_hz;
                
                if(ptr->datanum != 2){
                    printf_err("datanum error[%d]\n", ptr->datanum);
                    return -1;
                }
                printf_debug("fre data[%02x %02x]\n",  ptr->data[0],ptr->data[1]);
                i_freq_khz = COMPOSE_32BYTE_BY_16BIT(ptr->data[0],ptr->data[1]);
                printf_debug("ch = %d, SCREEN_CHANNEL_FRQ[%u %x]\n",  channel, i_freq_khz, i_freq_khz);
                break;
            }
            case SCREEN_CHANNEL_VOLUME:
            case SCREEN_CHANNEL_MGC:
            {   
                i_data = ptr->data[0];
                printf_debug("ch = %d, [%d]\n", channel, i_data);
            }
                break;
            case SCREEN_CHANNEL_AU_CTRL:
            {   
                uint8_t setdata, clear_data = 0;
                i_data = ptr->data[0];
                printf_debug("ch = %d,rec SCREEN_CHANNEL_AU_CTRL[%d]\n", channel, i_data);
                setdata = (~i_data)&0x01;
                k600_receive_write_data_from_user(channel, SCREEN_CHANNEL_AU_CTRL, &setdata);
                k600_receive_write_data_from_user(channel, SCREEN_CHANNEL_AU_BUTTON_CTRL, &clear_data);
            }
                break;
            case SCREEN_CHANNEL_AU_BUTTON_CTRL:
            {   
                i_data = ptr->data[0];
                printf_debug("ch = %d, SCREEN_CHANNEL_AU_BUTTON_CTRL[%d]\n", channel, i_data);
                if(i_data == SCREEN_BUTTON_PRESS_DOWN){
                    printf_debug("serial_screen_data_send_read\n");
                    k600_receive_read_cmd_from_user(channel, SCREEN_CHANNEL_AU_CTRL);
                }
            }
                break;
        }
        break;
    }
    default:
        printf_err("data type[%02x] error!!\n", data_type);
        break;
    }
    return 0;
}

int8_t k600_receive_write_data_from_user(uint8_t data_type, uint8_t data_cmd, void *pdata)
{
    if(pdata == NULL){
        return -1;
    }

    switch (data_type)
    {
        case SCREEN_COMMON_DATA1:
        {
            switch (data_cmd)
            {
                case SCREEN_CTRL_TYPE:
               {
                    uint16_t mode;
                    mode = htons(*(uint16_t *)pdata);
                    k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, SCREEN_CTRL_TYPE), &mode,1);
                }
                    break;
                case SCREEN_IPADDR:
                {
                    uint8_t ipbuf[4], addr;
                    uint16_t _ip;
                    INT_TO_CHAR(*(uint32_t *) pdata, ipbuf[3], ipbuf[2], ipbuf[1],ipbuf[0]);

                    addr = SCREEN_IPADDR1;
                    for(int j = 3; j >= 0; j--){
                        _ip = COMPOSE_16BYTE_BY_8BIT(ipbuf[j], 0);
                        k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, addr), &_ip,1);
                        addr++;
                    }
                }
                    break;
                case SCREEN_NETMASK_ADDR:
                {
                    uint8_t netmaskbuf[4], addr; 
                    uint16_t _netmask;
                     
                    INT_TO_CHAR(*(uint32_t *) pdata, netmaskbuf[3], netmaskbuf[2], netmaskbuf[1],netmaskbuf[0]);

                    addr = SCREEN_NETMASK_ADDR1;
                    for(int j = 3; j >= 0; j--){
                        _netmask = COMPOSE_16BYTE_BY_8BIT(netmaskbuf[j], 0);
                        k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, addr), &_netmask,1);
                        addr++;
                    }
                }
                    break;
                case SCREEN_GATEWAY_ADDR:
                {
                    uint8_t gatewaybuf[4], addr; 
                    uint16_t _gateway;
                    INT_TO_CHAR(*(uint32_t *) pdata, gatewaybuf[3], gatewaybuf[2], gatewaybuf[1],gatewaybuf[0]);

                    addr = SCREEN_GATEWAY_ADDR1;
                    for(int j = 3; j >= 0; j--){
                        _gateway = COMPOSE_16BYTE_BY_8BIT(gatewaybuf[j], 0);
                        k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, addr), &_gateway,1);
                        addr++;
                    }
                }
                    break;
                case SCREEN_PORT:
                {
                    uint16_t port;
                    port = htons(*(uint16_t *)pdata);
                    k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, SCREEN_PORT), &port,1);
                }
                    break;
                    
            }
                break;
        }
        case SCREEN_COMMON_DATA2:
        {
            uint16_t _data = 0;
            
            printf_debug("received cmd[%x]\n", data_type);
            switch (data_cmd){
                case SCREEN_LED1_STATUS:
                case SCREEN_LED2_STATUS:
                case SCREEN_LED3_STATUS:
                case SCREEN_LED4_STATUS:
                case SCREEN_LED5_STATUS:
                case SCREEN_LED6_STATUS:
                case SCREEN_LED7_STATUS:
                case SCREEN_LED8_STATUS:
                case SCREEN_LED_CLK_STATUS:
                case SCREEN_LED_AD_STATUS:
                {
                     if( *(uint8_t *)pdata != 0 && *(uint8_t *)pdata != 1 ) {                         
                        printf_err("error data\n");
                        return -1;
                     }
                     break;
                }
                case SCREEN_CPU_STATUS:
                case SCREEN_FPGA_STATUS:
                case SCREEN_CPU_DATA_STATUS:
                case SCREEN_FPGA_DATA_STATUS:
                {
                     if( *(uint8_t *)pdata < 0 && *(uint8_t *)pdata > 100 ) {                         
                        printf_err("error data\n");
                        return -1;
                     }
                     break;
                }
                    break;
            }              
            printf_debug("%d\n", *(uint8_t *)pdata);
            _data = COMPOSE_16BYTE_BY_8BIT(*(uint8_t *)pdata, 0);
            k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd), &_data, 1);     
        }
            break;
        case SCREEN_CHANNEL1_DATA:
        case SCREEN_CHANNEL2_DATA:
        case SCREEN_CHANNEL3_DATA:
        case SCREEN_CHANNEL4_DATA:
        case SCREEN_CHANNEL5_DATA:
        case SCREEN_CHANNEL6_DATA:
        case SCREEN_CHANNEL7_DATA:
        case SCREEN_CHANNEL8_DATA:
        {
            switch (data_cmd){
                case SCREEN_CHANNEL_BW:
                {
                    uint16_t _bw = 0;
                    for(uint32_t i = 0; i<sizeof(bw_data)/sizeof(SCREEN_BW_PARA_ST); i++){
                        if(bw_data[i].idata == *(uint32_t *)pdata){
                            _bw = COMPOSE_16BYTE_BY_8BIT(bw_data[i].type, 0);
                            printf_debug("set cw %d %x\n", bw_data[i].type, _bw);
                            k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd), &_bw, 1);
                            break;
                        }
                    }
                }
                    break;
                case SCREEN_CHANNEL_NOISE_EN:
                {
                    uint16_t _noise = 0;
                    if(*(uint8_t *)pdata != SCREEN_NOISE_OFF && *(uint8_t *)pdata != SCREEN_NOISE_ON){
                        printf_err("error data\n");
                        return -1;
                    }
                    _noise = COMPOSE_16BYTE_BY_8BIT(*(uint8_t *)pdata, 0);
                    k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd), &_noise, 1);
                }
                    break;
                case SCREEN_CHANNEL_MODE:
                {   
                    uint16_t _data = 0;
                    printf_debug("fdata[%u]\n", _data);
                    if( *(uint8_t *)pdata != SCREEN_MODE_LOWDIS && 
                        *(uint8_t *)pdata != SCREEN_MODE_NORMAL &&
                        *(uint8_t *)pdata != SCREEN_MODE_RECEIVE){
                        
                        printf_err("error data\n");
                        return -1;
                    }
                    _data = COMPOSE_16BYTE_BY_8BIT(*(uint8_t *)pdata, 0);
                    k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd), &_data, 1);
                }
                    break;
                case SCREEN_CHANNEL_AU_CTRL:
                case SCREEN_CHANNEL_AU_BUTTON_CTRL:
                {
                     if( *(uint8_t *)pdata != 0 && 
                         *(uint8_t *)pdata != 1){
                        printf_err("error data\n");
                        return -1;
                    }
                }
                case SCREEN_CHANNEL_DECODE_TYPE:
                case SCREEN_CHANNEL_VOLUME:
                case SCREEN_CHANNEL_MGC:
                {   
                    uint16_t _data = 0;
                    printf_debug("%d\n",*(uint8_t *)pdata);
                    _data = COMPOSE_16BYTE_BY_8BIT(*(uint8_t *)pdata, 0);
                    if(data_cmd == SCREEN_CHANNEL_DECODE_TYPE){
                        _data = k600_decode_method_convert_from_standard(_data);
                    }
                    k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd), &_data, 1);
                }
                    break;
                case SCREEN_CHANNEL_FRQ:
                {
                    //float fdata;
                    uint32_t idata;
                    idata = *(uint32_t *)pdata;
                    if(idata > 9999999){ /* KHz */
                         printf_err("error data\n");
                         return -1;
                    }
                    printf_debug("fdata[%u]\n", idata);
                    idata = htonl(idata);
                    k600_receive_write_act(COMPOSE_16BYTE_BY_8BIT(data_type, data_cmd), &idata, 2);
                }
                    break;
            }
            break;

        }
        default:
            printf_err("data type[%02x] error!!\n", data_type);
            break;
    }
    return 0;
}

