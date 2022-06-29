#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include "../../dao/json/cJSON.h"

typedef struct{
    uint8_t *bits;
    uint8_t *name;
    uint8_t *describe;
    uint8_t *type;
    uint8_t *default_val; 
}bit_info_st;

typedef struct{
    uint8_t *short_name;
    uint32_t offset;
    uint32_t port_size;  //端口偏移大小
    uint32_t port_max;   //最大端口数
    uint8_t bitrange_num;
    bit_info_st *p_bits;
    uint32_t real_val;
}reg_info_st;

typedef struct{
    char *chip_name;
    uint8_t regs_num;
    reg_info_st *p_regs;
}chip_config;

static chip_config g_chip;
static FILE *ret_fp;

static uint32_t read_reg(uint32_t *addr)
{
    uint32_t val = 0;
    char buf[100] = {'\0'};
    char rbuf[20] = {'\0'};

    sprintf(buf, "val=`nrs1800_tool -a 0x%x`; reg_val=`echo ${val##*:}`; echo $reg_val >> ret_val.txt", *addr);
    system(buf);
    fflush(ret_fp);
    fread(rbuf, sizeof(char), 10, ret_fp);
    sscanf(rbuf, "0x%08x", &val);
    fclose(ret_fp); 
    return val;
}

static void get_reg_bit_val(uint32_t *addr, uint32_t *retval)
{
    uint32_t val, ret0, ret1, ret3 = 0;

//    val = read_reg(addr);
    val = 0x06400000;
#if 1
    ret0 = val & 0xffff0000;
    ret1 = val & 0x0000ffff;
    for(int i = 0; i < 16; i ++)
    {
        ret3 |= ((ret1 >> i) & 0x1) << (31 - i);
        ret3 |= ((ret0 >> (31-i)) & 0x1) << i;
    }
#else
    ret3 = ((val & 0x000000ff) << 24) |
            ((val & 0x0000ff00) << 8) |
            ((val & 0x00ff0000) >> 8) |
            ((val & 0xff000000) >> 24);
#endif

#if 1
    char *bitrangs[] = {
            "07:00","15:08", "23:16","31:24"
         };
    printf("val = 0x%08x\n", val);
    printf("source val:\n");
    for(int i = 0; i < 4; i++)
    {
        printf("%s ", bitrangs[i]);
        for(int j = 0; j < 8; j++)
        {
            printf("%c ", (val >> (i * 8 + (7-j))) & 0x1 == 0x1 ? '1' : '0');
        }
        printf("\n");
    }
    printf("change val:0x%08x\n", ret3);
    printf("changebit val:\n");

    for(int i = 0; i < 4; i++)
    {
        printf("%s ", bitrangs[i]);
        for(int j = 0; j < 8; j++)
        {
            printf("%c ", (ret3 >> (i * 8 + (7-j))) & 0x1 == 0x1 ? '1' : '0');
        }
        printf("\n");
    }
#endif

    *retval = ret3;
}


static bool get_bit_range(char *bits, uint32_t *start, uint32_t *end)
{
    uint8_t *paddr = NULL;

    *start = atoi(bits);
    uint8_t* p = strstr(bits, ":");
    if(p)
    {    
        p ++;
        *end = atoi(p);
        return true;
    }
    return false;
}

static void bit_printf(uint32_t *addrval, char *bitrange)
{
    uint32_t start, end = 0;

    if(get_bit_range(bitrange, &start, &end) == true)
    {
        for(int i = start; i <= end; i++)
        {
            printf("%c", (*addrval >> i) & 0x1 == 1 ? '1' : '0');
        }

        if(end - start <= 4)
            printf("\t\t|");
        else
            printf("\t|");
    }
    else
    {
        printf("%c", *addrval >> start & 0x1 == 1 ? '1' : '0');
        printf("\t\t|");
    }    
}   

static bool reg_printf(uint32_t *addrval, bit_info_st *regbit)
{
    if(!regbit)
    {
        printf("error\t: regbit is null!\n");
        return false;
    }

    printf("%s\t|", regbit->bits);
    bit_printf(addrval, regbit->bits);
    printf("%-25s|%s\t|%-10s\t|\n%s", regbit->name, regbit->type,
        regbit->default_val, regbit->describe);
    printf("——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————\n");  

    return true; 
}

static bool ensure_which_reg(uint32_t addr, uint32_t *baseaddr, uint32_t *reg_num)
{   
    bool ret = false;
    int which, port = 0;
    int port_size = 0;

    if(addr >= 0x140 && addr < 0x300)
    {
        port_size = 0x20;
        port = (addr - 0x140) / 0x20;   
        which = (addr - 0x140) % 0x20;  
        *baseaddr = 0x140 + which;      
        // printf("port\t\t= 0x%x\n", port);
        // printf("which\t\t= 0x%x\n", which);
        // printf("baseaddr\t= 0x%x\n", *baseaddr);
    }
    else if(addr >= 0x1040 && addr < 0x1480)
    {

    }
    else if (addr >= 0x2000 && addr < 0x10070)
    {
        
    }
    else if (addr >= 0x10070 && addr < 0x1f000)
    {
        
    }
    else if (addr >= 0x31044 && addr < 0x031484)
    {
        
    }
    else
    {

    }

    for(int i = 0; i < g_chip.regs_num; i++)
    {
        if(g_chip.p_regs[i].offset == *baseaddr)
        {
            *reg_num = i;
            return true;
        }
    }

    printf("warn\t: can't find addr_reg!\n");
    return false;
}


static bool parse_reg_json_with_config(cJSON* root)
{
    cJSON *value;
    cJSON *info;
    cJSON *node;
    cJSON *regs, *reg;
    uint32_t int_val = 0;

    regs = cJSON_GetObjectItem(root, "regs");
    if(!regs)
    {
        printf("error\t: can't get regs json node!\n");
        cJSON_Delete(root);
        return false;
    }

    g_chip.regs_num = cJSON_GetArraySize(regs);
    g_chip.p_regs = (reg_info_st *)malloc(sizeof(reg_info_st) * g_chip.regs_num);

    for(int j = 0; j < g_chip.regs_num; j++)
    {
        reg = cJSON_GetArrayItem(regs, j);
        value = cJSON_GetObjectItem(reg, "short_name");
        if(cJSON_IsString(value))      
            g_chip.p_regs[j].short_name = strdup(value->valuestring);
        value = cJSON_GetObjectItem(reg, "offset");
        if(cJSON_IsString(value))   
            sscanf(value->valuestring, "0x%x", &g_chip.p_regs[j].offset);
        value = cJSON_GetObjectItem(reg, "port_size");
        if(cJSON_IsString(value))      
            sscanf(value->valuestring, "0x%x", &g_chip.p_regs[j].port_size);
        value = cJSON_GetObjectItem(reg, "port_max");
        if(cJSON_IsString(value))     
            sscanf(value->valuestring, "%d", &g_chip.p_regs[j].port_max);

        info = cJSON_GetObjectItem(reg, "info");
        g_chip.p_regs[j].bitrange_num = cJSON_GetArraySize(info);
        g_chip.p_regs[j].p_bits = (bit_info_st *)malloc(sizeof(bit_info_st) * g_chip.p_regs[j].bitrange_num);

        for(int i = 0; i < g_chip.p_regs[j].bitrange_num; i++)
        {
            node = cJSON_GetArrayItem(info, i);
            if(node){
                value = cJSON_GetObjectItem(node, "bits");
                if(cJSON_IsString(value))
                    g_chip.p_regs[j].p_bits[i].bits = strdup(value->valuestring);
                value = cJSON_GetObjectItem(node, "name");
                if(cJSON_IsString(value))
                    g_chip.p_regs[j].p_bits[i].name = strdup(value->valuestring);
                value = cJSON_GetObjectItem(node, "type");
                if(cJSON_IsString(value))
                    g_chip.p_regs[j].p_bits[i].type = strdup(value->valuestring);
                value = cJSON_GetObjectItem(node, "default_val");
                if(cJSON_IsString(value))
                    g_chip.p_regs[j].p_bits[i].default_val = strdup(value->valuestring);
                value = cJSON_GetObjectItem(node, "describe");
                if(cJSON_IsString(value))
                    g_chip.p_regs[j].p_bits[i].describe = strdup(value->valuestring);
            }
        }
    }
    cJSON_Delete(root);
    return true;
}


static cJSON* json_read_file(const char *filename, cJSON* root)
{
    long len = 0;
    char *JSON_content;

    FILE* fp = fopen(filename, "rb+");
    if(!fp)
    {
        printf("open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if(0 == len)
    {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    JSON_content = (char*) malloc(sizeof(char) * len);
    fread(JSON_content, 1, len, fp);
    fclose(fp);
    root = cJSON_Parse(JSON_content);
    if (root == NULL)
    {
        const char *Error_ptr = cJSON_GetErrorPtr();
        if (Error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", Error_ptr);
        }
        return NULL;
    }
    free(JSON_content);

    return root;
}

static void printf_reg_info(uint32_t reg_num)
{
    bool ret = false;

    printf("reg offset\t:0x%x\n", g_chip.p_regs[reg_num].offset);
    if(g_chip.p_regs[reg_num].port_size != 0)
        printf("reg port_size\t:0x%x\n", g_chip.p_regs[reg_num].port_size);
    if(g_chip.p_regs[reg_num].port_max != 0)
        printf("reg port_max\t:%d\n", g_chip.p_regs[reg_num].port_max);
    printf("reg short_name\t:%s\n", g_chip.p_regs[reg_num].short_name);
    printf("reg real val\t:0x%08x\n", g_chip.p_regs[reg_num].real_val);
    printf("——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————\n");
    printf("bits\t|real_val\t|name\t\t\t  |type\t|def_val\t|describe\n");
    for(int j = 0; j < g_chip.p_regs[reg_num].bitrange_num; j++)
    {
        ret = reg_printf(&g_chip.p_regs[reg_num].real_val, &g_chip.p_regs[reg_num].p_bits[j]);
        if(ret == false)
            exit(0);
    }
}

static void usage(const char *prog)
{
    printf("Usage: %s [option]\n"
        "       -c config the chip's reg json file"
        "       -r read reg, such as: -r 0x15c\n", prog);
    exit(1);
}

int main(int argc, char *argv[])
{
    uint32_t read_reg;
    bool ret = false;
    int opt;
    if(argc < 2)
        usage(argv[0]);

    int fileflag = 0;
    cJSON *root = NULL;

    while((opt = getopt(argc, argv, "c:r:")) != -1)
    {
        switch (opt){
            case 'c':
                printf("json file:\t= %s\n", optarg);
                root = json_read_file(optarg, root);
                fileflag = 1;
            break;

            case 'r':
                printf("read regs\t= %s\n", optarg);
                if(!strstr(optarg, "x"))
                {
                    usage(argv[0]);
                    return -1;
                }
                sscanf(optarg, "0x%x", &read_reg);
            break;

            
            default:
                usage(argv[0]);
            break;
        }
    }
    ret_fp = fopen("ret_val.txt", "w+");
    if(!ret_fp)
    {
        printf("error: open ret_val.txt failed!\n");
        return -1;
    }
    
    if(fileflag == 0)
        root = json_read_file("/etc/nrs1800_desc.json", root);
    
    ret = parse_reg_json_with_config(root);
    if(ret == false)
        return -1;

    int baseaddr, reg_num = 10;
    ret = ensure_which_reg(read_reg, &baseaddr, &reg_num);
    if(ret == true)
    {    
        printf("baseaddr\t= 0x%x\nreg_num\t\t= %d\n", baseaddr, reg_num);
        get_reg_bit_val(NULL, &g_chip.p_regs[reg_num].real_val);
        printf_reg_info(reg_num);
        return 0;
    }

    printf("error\t: 0x%x reg find failed!\n", read_reg);
    return -1;
}

