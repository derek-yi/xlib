#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "vos.h"
#include "cJSON.h"
#include "tiny_cli.h" 
#include "drv_i2c.h"
#include "devm_fru.h" 

static int dbg_mode = 1;

#ifdef FRU_APP

#define vos_print               printf
#define vos_msleep(x)           usleep((x)*1000)
#define xlog(x, fmt, args...)   printf(fmt, ##args)

#else

#include "xlog.h"

#endif

typedef struct 
{
    int     i2c_bus;
    int     dev_id;
    int     chip_size;
}EEPROM_INFO;

int drv_get_eeprom_info(int fru_id, EEPROM_INFO *info)
{
    if (fru_id == 0) {
        info->i2c_bus       = 4;
        info->dev_id        = 0x54;
        info->chip_size     = 256;
    } else {
        return VOS_ERR;
    }
    
    return VOS_OK;
}

#if T_DESC("FRU_FUNC", 1)

FRU_ENTRY_ST fru_info[FRU_MAX_CNT];

static inline int hex2num(char c)
{
	if (c>='0' && c<='9') return c - '0';
	if (c>='a' && c<='z') return c - 'a' + 10;
	if (c>='A' && c<='Z') return c - 'A' + 10;
	return -1;
}
 
int parse_hex_string(char *hexStr, int max, uint8 *pData)  
{  
    int value;
    int i, j, k;
    
    for (i = 0, j = 0, k = 0; i < strlen(hexStr); i++)  
    {  
        value = hex2num(hexStr[i]);
        if (value < 0) continue;
        
        if (k == 0) {
            pData[j] = value;  
            k++;
        } else {
            pData[j] = pData[j]*16 + value;
            k = 0; j++;
        }

        if (j == max) break;
    }     

    if (j < max) return -1;
    
    return 0;
} 

int format_hex_string(char *hexStr, int max, uint8 *pData)  
{
    for (int i = 0; i < max; i++) {
        sprintf(hexStr + i*2, "%02x", pData[i]);
    }
    hexStr[max*2] = 0;
}

int devm_load_area_data(int fru_id, int start, char *area_data, uint32 area_len)
{
    int i, ret;
    EEPROM_INFO info;
    
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("%d: invalid fru_id %d \r\n", __LINE__, fru_id);
        return VOS_ERR;
    }

    //start = start*8;
    /*if (start + area_len > info.chip_size)  {
        vos_print("%d: oversize %d \r\n", __LINE__, start + area_len);
        return VOS_ERR;
    }*/

    for(i = 0; i < area_len; i++) {
        area_data[i] = i2c_read_data(info.i2c_bus, I2C_SMBUS_BYTE_DATA, info.dev_id, start + i);
    }    

    return VOS_OK;
}

int devm_store_area_data(int fru_id, int start, uint8 *area_data, uint32 area_len)
{
    int i, ret;
    EEPROM_INFO info;
    
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("%d: invalid fru_id %d \r\n", __LINE__, fru_id);
        return VOS_ERR;
    }
    
    //start = start*8;
    /*if (start + area_len > info.chip_size)  {
        vos_print("%d: oversize %d \r\n", __LINE__, start + area_len);
        return VOS_ERR;
    }*/

    //printf("%d: fru_id %d, i2c_bus %d, dev_id %d, start %d, len %d \r\n", __LINE__, fru_id, info.i2c_bus, info.dev_id, start, area_len);
    for(i = 0; i < area_len; i++) {
        ret |= i2c_write_buffer(info.i2c_bus, I2C_SMBUS_BYTE_DATA, info.dev_id, start + i, &area_data[i], 1); //byte mode
        vos_msleep(5);
    }
    
    if (ret != VOS_OK) {
        vos_print("%d: i2c_write_buffer failed \r\n", __LINE__);
        return VOS_ERR;
    }

    return VOS_OK;
}

int devm_read_fru_info(int fru_id)
{
    int ret;
    
    if (fru_id >= MAX_FRU_NUM) {
		vos_print("invalid fru_id %d \r\n", fru_id);
        return VOS_ERR;
    }

    memset(&fru_info[0], 0, sizeof(FRU_ENTRY_ST)*FRU_MAX_CNT);
    for (int i = 0; i < FRU_MAX_CNT; i++) {
        ret = devm_load_area_data(fru_id, i*sizeof(FRU_ENTRY_ST), (char *)&fru_info[i], sizeof(FRU_ENTRY_ST));
        if (ret != VOS_OK) {
    		vos_print("devm_load_area_data failed, ret %d \r\n", ret);
            return VOS_ERR;
        }
    }

    return VOS_OK;
}

int devm_show_fru_info(int fru_id)
{
    uint8 fmt_str[64];

    for (int i = 0; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) continue;

        if (fru_info[i].type == FRU_TYPE_BOARD_SN) {
            vos_print("BOARD SN    : %s\r\n", fru_info[i].data);
        } 
        else  if (fru_info[i].type == FRU_TYPE_MMC_ID) {
            format_hex_string(fmt_str, 16, fru_info[i].data);
            vos_print("BOARD MMC   : %s\r\n", fmt_str);
        } 
    }

    for (int i = 0; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) continue;

        if (fru_info[i].type == FRU_TYPE_RECORD) {
            vos_print(">> %s\r\n", fru_info[i].data);
        } 
    }
    
    return VOS_OK;
}

int devm_fru_set_mac(int store, uint8 mac[6])
{
    int ret, i, j;
    
    for (i = 0, j = -1; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) {
            if (j < 0) j = i;
            continue;
        }

        if (fru_info[i].type == FRU_TYPE_MAC) {
            memcpy(fru_info[i].data, mac, 6);
            break;
        }
    }

    if (i == FRU_MAX_CNT) {
        if (j < 0) {
            vos_print("%d: full fru info \r\n", __LINE__);
            return VOS_ERR;
        }
        i = j;
        fru_info[i].magic = FRU_MAGIC_BYTE;
        fru_info[i].type = FRU_TYPE_MAC;
        memcpy(fru_info[i].data, mac, 6);
    }

    if (store) {
        ret = devm_store_area_data(0, i*sizeof(FRU_ENTRY_ST), (uint8 *)&fru_info[i], sizeof(FRU_ENTRY_ST));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int devm_fru_set_uuid(int store, char *uuid_str)
{
    int ret, i, j;
    uint8 hex_uuid[16];

    if ( parse_hex_string(uuid_str, 16, hex_uuid) != VOS_OK) {
        return VOS_ERR;
    }
    
    for (i = 0, j = -1; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) {
            if (j < 0) j = i;
            continue;
        }

        if (fru_info[i].type == FRU_TYPE_MMC_ID) {
            memcpy(fru_info[i].data, hex_uuid, 16);
            break;
        }
    }

    if (i == FRU_MAX_CNT) {
        if (j < 0) {
            vos_print("%d: full fru info \r\n", __LINE__);
            return VOS_ERR;
        }
        i = j;
        fru_info[i].magic = FRU_MAGIC_BYTE;
        fru_info[i].type = FRU_TYPE_MMC_ID;
        memcpy(fru_info[i].data, hex_uuid, 16);
    }

    if (store) {
        ret = devm_store_area_data(0, i*sizeof(FRU_ENTRY_ST), (uint8 *)&fru_info[i], sizeof(FRU_ENTRY_ST));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int devm_fru_get_uuid(char *uuid_str, int max)
{
    int ret;

    if ( (!uuid_str) || (max < 33) ) return VOS_ERR;
    
    for (int i = 0; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) continue;

        if (fru_info[i].type == FRU_TYPE_MMC_ID) {
            format_hex_string(uuid_str, 16, fru_info[i].data);
            return VOS_OK;
        }
    }
    
    return VOS_ERR;
}

int devm_fru_set_sn(int store, char *buffer, int buf_len)
{
    int ret, i, j;
    
    for (i = 0, j = -1; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) {
            if (j < 0) j = i;
            continue;
        }

        if (fru_info[i].type == FRU_TYPE_BOARD_SN) {
            snprintf(fru_info[i].data, 30, "%s", buffer);
            break;
        }
    }

    if (i == FRU_MAX_CNT) {
        if (j < 0) {
            vos_print("%d: full fru info \r\n", __LINE__);
            return VOS_ERR;
        }
        i = j;
        fru_info[i].magic = FRU_MAGIC_BYTE;
        fru_info[i].type = FRU_TYPE_BOARD_SN;
        snprintf(fru_info[i].data, 30, "%s", buffer);
    }

    if (store) {
        ret = devm_store_area_data(0, i*sizeof(FRU_ENTRY_ST), (uint8 *)&fru_info[i], sizeof(FRU_ENTRY_ST));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int devm_fru_get_sn(char *sn_str, int max)
{
    int ret;

    if (!sn_str) return VOS_ERR;
    
    for (int i = 0; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) continue;

        if (fru_info[i].type == FRU_TYPE_BOARD_SN) {
            snprintf(sn_str, max, "%s", fru_info[i].data);
            return VOS_OK;
        }
    }
    
    return VOS_ERR;
}

int devm_fru_add_log(int store, char *buffer)
{
    int ret, i, j;
    
    for (i = 0, j = -1; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) {
            if (j < 0) j = i;
            continue;
        }
    }

    if (i == FRU_MAX_CNT) {
        if (j < 0) {
            vos_print("%d: full fru info \r\n", __LINE__);
            return VOS_ERR;
        }
        i = j;
        fru_info[i].magic = FRU_MAGIC_BYTE;
        fru_info[i].type = FRU_TYPE_RECORD;
        snprintf(fru_info[i].data, 30, "%s", buffer);
    }

    if (store) {
        ret = devm_store_area_data(0, i*sizeof(FRU_ENTRY_ST), (uint8 *)&fru_info[i], sizeof(FRU_ENTRY_ST));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int fru_info_test()
{
    devm_read_fru_info(0);
    devm_show_fru_info(0);
    return 0;
}

#endif

#if T_DESC("CLI", 1)

int cli_show_fru_info(int argc, char **argv)
{
    devm_show_fru_info(0);
    return VOS_OK;
}    

int cli_fru_set_mac(int argc, char **argv)
{
    uint8 mac_addr[6];

    if (argc < 2) {
        vos_print("Usage: %s <mac_addr> \r\n", argv[0]);
        vos_print("       <mac_addr> mac in hex, such as 000c-293e-ce9d \r\n");
        return CMD_ERR_PARAM;
    }

    if ( parse_hex_string(argv[1], 6, mac_addr) != VOS_OK) {
        vos_print("Invalid Mac Address \r\n");
        return VOS_OK;
    }
    
    devm_fru_set_mac(TRUE, mac_addr);
    return VOS_OK;
}    

int cli_fru_set_uuid(int argc, char **argv)
{
    int ret;

    if (argc < 2) {
        vos_print("Usage: %s <uuid> \r\n", argv[0]);
        vos_print("       <uuid> uuid in 32B hex string \r\n");
        return CMD_ERR_PARAM;
    }

    vos_print("Save board UUID ... ");
    ret = devm_fru_set_uuid(TRUE, argv[1]);
    if (ret != VOS_OK) vos_print("Failed \r\n");
    else vos_print("OK \r\n");

    return VOS_OK;
}    

int cli_fru_set_sn(int argc, char **argv)
{
    int ret;

    if (argc < 2) {
        vos_print("Usage: %s <sn_str>\r\n", argv[0]);
        return CMD_ERR_PARAM;
    }

    if (strlen(argv[1]) > 28) {
        vos_print("serial number too long \r\n");
        return VOS_OK;
    }

    vos_print("Save board serial num ... ");
    ret = devm_fru_set_sn(TRUE, argv[1], strlen(argv[1]));
    if (ret != VOS_OK) vos_print("Failed \r\n");
    else vos_print("OK \r\n");
    
    return VOS_OK;
}  

int cli_reset_fru_info(int argc, char **argv)
{   
    int ret;

    xlog(XLOG_WARN, "WARN: all fru info will clean up\n");
    for (int i = 0; i < FRU_MAX_CNT; i++) {
        if (fru_info[i].magic != FRU_MAGIC_BYTE) continue;
        if (fru_info[i].type < FRU_TYPE_RECORD) continue;

        ret = devm_store_area_data(0, i*sizeof(FRU_ENTRY_ST), (uint8 *)&fru_info[i], sizeof(FRU_ENTRY_ST));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
	
    return VOS_OK;
}

int cli_fru_add_log(int argc, char **argv)
{
    uint8 log_str[128];
    int offset = 0;

    if (argc < 2) {
        vos_print("Usage: %s <string> ... \r\n", argv[0]);
        vos_print("  less than 30B, example: 20220909 do sth \r\n");
        return CMD_ERR_PARAM;
    }

    memset(log_str, 0, sizeof(log_str));
    for (int i = 0; i < argc - 1; i++) {
        offset += snprintf(log_str + offset, 32 - offset, "%s ", argv[i + 1]);
    }

    if ( devm_fru_add_log(TRUE, log_str) != VOS_OK) {
        vos_print("%d: devm_fru_add_log failed \r\n", __LINE__);
    }
    return VOS_OK;
}    

#endif

#ifdef FRU_APP

int fru_set_sn(int argc, char **argv)
{
    int ret;
    int buf_len;
    char buffer[128];

#if 0 //
    vos_print("Step <1> Input board part number: ");
    fgets(buffer, sizeof(buffer), stdin);
    buf_len = strlen(buffer);
    if (buf_len > 0) buffer[buf_len - 1] = 0;
    //vos_print("\r\n fgets: %d, %s \r\n", strlen(buffer), buffer);

    buf_len = strlen(fru_board_info[fru_id].board_part_num);
    if (buf_len > 20) buf_len = 20;
    if (memcmp(buffer, fru_board_info[fru_id].board_part_num, buf_len) ) {
        vos_print("\r\n Part Number Mismatch, Board Part Number is %s \r\n", fru_board_info[fru_id].board_part_num);
        return VOS_OK;
    }
#endif

    vos_print("Step <1> Input board serial number: ");
    fgets(buffer, sizeof(buffer), stdin);
    buf_len = strlen(buffer);
    if (buf_len > 0) buffer[buf_len - 1] = 0;
    //vos_print("\r\n fgets: %d, %s \r\n", strlen(buffer), buffer);

    if (strlen(buffer) > 28) {
        vos_print("\r\n serial number too long \r\n");
        return VOS_OK;
    }

    vos_print("Step <2> Save board serial num ... ");
    ret = devm_fru_set_sn(TRUE, buffer, strlen(buffer));
    if (ret != VOS_OK) vos_print("Failed \r\n");
    else vos_print("OK \r\n");
    
    return VOS_OK;
}  

/*
## compile
aarch64-linux-gnu-gcc -DFRU_APP -o fru.bin drv_i2c.c devm_fru.c vos.c -I../include -lrt -lpthread
*/
int main(int argc, char **argv)
{
    if (argc < 2) {
        vos_print("Usage:\r\n");
        vos_print("  show                       -- show fru info \r\n");
        vos_print("  reset                      -- reset fru info \r\n");
        vos_print("  set_mmc <uuid>             -- set board mmc ID \r\n");
        vos_print("  set_sn                     -- set board serial number \r\n");
        vos_print("  add_log <string> ...       -- add repair log \r\n");
        return VOS_OK;
    }

    devm_read_fru_info(0);

    if (!strcmp(argv[1], "show")) 
        cli_show_fru_info(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "reset")) 
        cli_reset_fru_info(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_mac")) 
        cli_fru_set_mac(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_mmc")) 
        cli_fru_set_uuid(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_sn")) 
        fru_set_sn(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "add_log")) 
        cli_fru_add_log(argc - 1, &argv[1]);
    else 
        vos_print("unknown cmd \r\n");

    return VOS_OK;
}

#else

int devm_fru_init(void)
{
	devm_read_fru_info(0);

	cli_cmd_reg("fru_show",     	"show fru info",       		&cli_show_fru_info);
	cli_cmd_reg("fru_reset",    	"reset fru info",           &cli_reset_fru_info);
	cli_cmd_reg("fru_set_sn",    	"set board sn",       		&cli_fru_set_sn);
	cli_cmd_reg("fru_set_mmc",    	"set mmc id",      		    &cli_fru_set_uuid);
	cli_cmd_reg("fru_add_log",    	"add repair log",      		&cli_fru_add_log);

	return VOS_OK;
}

#endif

