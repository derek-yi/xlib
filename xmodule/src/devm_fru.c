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

static int dbg_mode = 0;

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

FRU_COMMON_HEADER   fru_common_info[MAX_FRU_NUM];
FRU_CHASSIS_INFO    fru_chassis_info[MAX_FRU_NUM];
FRU_BOARD_INFO      fru_board_info[MAX_FRU_NUM];
FRU_PRODUCT_INFO    fru_product_info[MAX_FRU_NUM];

static inline int hex2num(char c)
{
	if (c>='0' && c<='9') return c - '0';
	if (c>='a' && c<='z') return c - 'a' + 10;//这里+10的原因是:比如16进制的a值为10
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

int devm_load_area_data(int fru_id, int start, char *area_data, uint32 area_len)
{
    int i, ret;
    EEPROM_INFO info;
    
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("%d: invalid fru_id %d \r\n", __LINE__, fru_id);
        return VOS_ERR;
    }

    start = start*8;
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
    
    start = start*8;
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

static uint8 devm_get_area_checksum(uint8 *area_data, uint32 data_len)
{
	uint32 i;
	uint8 tmp = 0;

	for (i = 0; i < data_len; i++)
		tmp += area_data[i];

	return tmp;
}

int devm_read_fru_info(int fru_id)
{
    int ret;
    
    if (fru_id >= MAX_FRU_NUM) {
		vos_print("invalid fru_id %d \r\n", fru_id);
        return VOS_ERR;
    }
    
    ret = devm_load_area_data(fru_id, 0, (char *)&fru_common_info[fru_id], sizeof(FRU_COMMON_HEADER));
    if (ret != VOS_OK) {
		vos_print("devm_load_area_data failed, ret %d \r\n", ret);
        return VOS_ERR;
    }

    if (dbg_mode) { 
        vos_print("fru_id %d: chassis_info_start %d, board_info_start %d, product_info_start %d \r\n", 
            fru_id, fru_common_info[fru_id].chassis_info_start, fru_common_info[fru_id].board_info_start,
            fru_common_info[fru_id].product_info_start);
    }

    memset(&fru_chassis_info[fru_id], 0, sizeof(FRU_CHASSIS_INFO));
    if (fru_common_info[fru_id].chassis_info_start > 0) {
        if (dbg_mode) vos_print("%d: load fru_common_info \r\n", __LINE__);
        ret = devm_load_area_data(fru_id, fru_common_info[fru_id].chassis_info_start, (char *)&fru_chassis_info[fru_id], sizeof(FRU_CHASSIS_INFO));
        if (ret != VOS_OK) {
    		xlog(XLOG_WARN, "%d: devm_load_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
        if ( fru_chassis_info[fru_id].area_checksum != devm_get_area_checksum((uint8 *)&fru_chassis_info[fru_id], sizeof(FRU_CHASSIS_INFO) - 1) ) {
            memset(&fru_chassis_info[fru_id], 0, sizeof(FRU_CHASSIS_INFO));  //invalid area
            xlog(XLOG_WARN, "%d: fru_chassis_info crc error \r\n", __LINE__);
            return VOS_ERR;
        }
    }

    memset(&fru_board_info[fru_id], 0, sizeof(FRU_BOARD_INFO));
    if (fru_common_info[fru_id].board_info_start > 0) {
        if (dbg_mode)vos_print("%d: load fru_board_info \r\n", __LINE__);
        ret = devm_load_area_data(fru_id, fru_common_info[fru_id].board_info_start, (char *)&fru_board_info[fru_id], sizeof(FRU_BOARD_INFO));
        if (ret != VOS_OK) {
    		xlog(XLOG_WARN, "%d: devm_load_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
        if ( fru_board_info[fru_id].area_checksum != devm_get_area_checksum((uint8 *)&fru_board_info[fru_id], sizeof(FRU_BOARD_INFO) - 1) ) {
            memset(&fru_board_info[fru_id], 0, sizeof(FRU_BOARD_INFO));  //invalid area
            xlog(XLOG_WARN, "%d: fru_board_info crc error \r\n", __LINE__);
            return VOS_ERR;
        }
    }

    memset(&fru_product_info[fru_id], 0, sizeof(FRU_PRODUCT_INFO));
    if (fru_common_info[fru_id].product_info_start > 0) {
        if (dbg_mode)vos_print("%d: load fru_product_info \r\n", __LINE__);
        ret = devm_load_area_data(fru_id, fru_common_info[fru_id].product_info_start, (char *)&fru_product_info[fru_id], sizeof(FRU_PRODUCT_INFO));
        if (ret != VOS_OK) {
    		xlog(XLOG_WARN, "%d: devm_load_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
        if ( fru_product_info[fru_id].area_checksum != devm_get_area_checksum((uint8 *)&fru_product_info[fru_id], sizeof(FRU_PRODUCT_INFO) - 1) ) {
            memset(&fru_product_info[fru_id], 0, sizeof(FRU_PRODUCT_INFO));  //invalid area
            xlog(XLOG_WARN, "%d: fru_product_info crc error \r\n", __LINE__);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

#define FOMRT_FIELD_STR(buffer, field_data, len)    \
    do {    \
        memcpy(buffer, field_data, len);    \
        buffer[len] = 0;    \
    } while(0)

int devm_show_custom_record(int rec_len, uint8 *record)
{
    if (!record) return VOS_ERR;
    
    if (record[0] == 0x1) {
        vos_print("  Format Version: 0x%02x\r\n", record[1]);
        vos_print("  Mac Address   : %02x%02x-%02x%02x-%02x%02x\r\n", 
                    record[2], record[3], record[4], record[5], record[6], record[7]);
    }
    else if (record[0] == 0x2) { //2a5d7f1f-48174dea-9a70b8ff-7ae4710a
        vos_print("  Format Version: 0x%02x\r\n", record[1]);
        vos_print("  UUID          : %02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x\r\n", 
                    record[2], record[3], record[4], record[5], record[6], record[7], record[8], record[9], 
                    record[10], record[11], record[12], record[13], record[14], record[15], record[16], record[17]);
    }
    else if (record[0] == 0x20) {
        vos_print("  Format Version: 0x%02x\r\n", record[1]);
        vos_print("  SKU ID        : 0x%02x\r\n", record[2]);
    }
    else if (record[0] == 0x40) {
        vos_print("  Format Version    : 0x%02x\r\n", record[1]);
        vos_print("  Channel Quantity  : 0x%02x\r\n", record[2]);
        vos_print("  Channel.1 Tx Power: 0x%02x\r\n", record[3]);
        vos_print("  Channel.2 Tx Power: 0x%02x\r\n", record[4]);
    }
#ifndef DAEMON_RELEASE
    else if (rec_len > 0) {
        int i;
        vos_print("  RAW Data: ");
        for(i = 0; i < rec_len; i++ ) vos_print("0x%02x ", record[i]);
        vos_print("\r\n");    
    }
#endif

    return VOS_OK;
}

int devm_show_fru_info(int fru_id)
{
    uint8 fmt_str[64];

    if (fru_chassis_info[fru_id].area_len > 0) {
        vos_print("Chassis Area Format Version  : 0x%02x \r\n", fru_chassis_info[fru_id].fmt_version);
        vos_print("Chassis Info Area Length     : 0x%02x \r\n", fru_chassis_info[fru_id].area_len);
        vos_print("Chassis Type                 : 0x%02x \r\n", fru_chassis_info[fru_id].chassis_type);
        vos_print("Chassis Part Num type        : 0x%02x \r\n", fru_chassis_info[fru_id].chassis_part_num_type);
        FOMRT_FIELD_STR(fmt_str, fru_chassis_info[fru_id].chassis_part_num, 20);
        vos_print("Chassis Part Num bytes       : %s \r\n", fmt_str);
        vos_print("Chassis Serial Num type      : 0x%02x \r\n", fru_chassis_info[fru_id].chassis_serial_num_type);
        FOMRT_FIELD_STR(fmt_str, fru_chassis_info[fru_id].chassis_serial_num, 20);
        vos_print("Chassis Serial Num bytes     : %s \r\n", fmt_str);
        vos_print("Custom Chassis Info No.1 type: 0x%02x \r\n", fru_chassis_info[fru_id].chassis_info_n1_type);
        devm_show_custom_record(fru_chassis_info[fru_id].chassis_info_n1_type, fru_chassis_info[fru_id].chassis_info_n1);
        vos_print(" \r\n");
    }
    
    if (fru_board_info[fru_id].area_len > 0) {
        vos_print("Board Area Format Version    : 0x%02x \r\n", fru_board_info[fru_id].fmt_version);
        vos_print("Board Info Area Length       : 0x%02x \r\n", fru_board_info[fru_id].area_len);
        vos_print("Board Info Language Code     : 0x%02x \r\n", fru_board_info[fru_id].language_code);
        vos_print("Board Manufacturing Date     : 0x%02x 0x%02x 0x%02x\r\n", fru_board_info[fru_id].Manufacturing_time[0],
                fru_board_info[fru_id].Manufacturing_time[1], fru_board_info[fru_id].Manufacturing_time[2]);
        vos_print("Board Manufacturer type      : 0x%02x \r\n", fru_board_info[fru_id].manufacturer_type);
        FOMRT_FIELD_STR(fmt_str, fru_board_info[fru_id].manufacturer_name, 16);
        vos_print("Board Manufacturer bytes     : %s \r\n", fmt_str);
        vos_print("Board Name type              : 0x%02x \r\n", fru_board_info[fru_id].board_name_type);
        FOMRT_FIELD_STR(fmt_str, fru_board_info[fru_id].board_name, 16);
        vos_print("Board Name bytes             : %s \r\n", fmt_str);
        vos_print("Board Serial Number type     : 0x%02x \r\n", fru_board_info[fru_id].board_serial_num_type);
        FOMRT_FIELD_STR(fmt_str, fru_board_info[fru_id].board_serial_num, 20);
        vos_print("Board Serial Number bytes    : %s \r\n", fmt_str);
        vos_print("Board Part Number type       : 0x%02x \r\n", fru_board_info[fru_id].board_part_num_type);
        FOMRT_FIELD_STR(fmt_str, fru_board_info[fru_id].board_part_num, 20);
        vos_print("Board Part Number bytes      : %s \r\n", fmt_str);
        vos_print("Board FRU File ID type       : 0x%02x \r\n", fru_board_info[fru_id].board_fru_fileid_type);
        vos_print("Board FRU File ID bytes      : 0x%02x \r\n", fru_board_info[fru_id].board_fru_fileid);
        vos_print("Custom Board Info No.1 type  : 0x%02x \r\n", fru_board_info[fru_id].custom_board_info_n1_type);
        devm_show_custom_record(fru_board_info[fru_id].custom_board_info_n1_type, fru_board_info[fru_id].custom_board_info_n1);
        vos_print("Custom Board Info No.2 type  : 0x%02x \r\n", fru_board_info[fru_id].custom_board_info_n2_type);
        devm_show_custom_record(fru_board_info[fru_id].custom_board_info_n2_type, fru_board_info[fru_id].custom_board_info_n2);
        vos_print("Custom Board Info No.3 type  : 0x%02x \r\n", fru_board_info[fru_id].custom_board_info_n3_type);
        devm_show_custom_record(fru_board_info[fru_id].custom_board_info_n3_type, fru_board_info[fru_id].custom_board_info_n3);
        vos_print("Custom Board Info No.4 type  : 0x%02x \r\n", fru_board_info[fru_id].custom_board_info_n4_type);
        devm_show_custom_record(fru_board_info[fru_id].custom_board_info_n4_type, fru_board_info[fru_id].custom_board_info_n4);
        vos_print(" \r\n");
    }

    if (fru_product_info[fru_id].area_len > 0) {
        vos_print("Product Area Format Version  : 0x%02x \r\n", fru_product_info[fru_id].fmt_version);
        vos_print("Product Info Area Length     : 0x%02x \r\n", fru_product_info[fru_id].area_len);
        vos_print("Product Info Language Code   : 0x%02x \r\n", fru_product_info[fru_id].language_code);
        vos_print("Product Manufacturer type    : 0x%02x \r\n", fru_product_info[fru_id].manufacturer_type);
        FOMRT_FIELD_STR(fmt_str, fru_product_info[fru_id].manufacturer_name, 16);
        vos_print("Product Manufacturer bytes   : %s \r\n", fmt_str);
        vos_print("Product Name type            : 0x%02x \r\n", fru_product_info[fru_id].product_name_type);
        FOMRT_FIELD_STR(fmt_str, fru_product_info[fru_id].product_name, 16);
        vos_print("Product Name bytes           : %s \r\n", fmt_str);
        vos_print("Product Part Number type     : 0x%02x \r\n", fru_product_info[fru_id].product_part_num_type);
        FOMRT_FIELD_STR(fmt_str, fru_product_info[fru_id].product_part_num, 20);
        vos_print("Product Part Number bytes    : %s \r\n", fmt_str);
        vos_print("Product Version type         : 0x%02x \r\n", fru_product_info[fru_id].product_version_type);
        FOMRT_FIELD_STR(fmt_str, fru_product_info[fru_id].product_version, 8);
        vos_print("Product Version bytes        : %s \r\n", fmt_str);
        vos_print("Product Serial Number type   : 0x%02x \r\n", fru_product_info[fru_id].product_serial_num_type);
        FOMRT_FIELD_STR(fmt_str, fru_product_info[fru_id].product_serial_num, 20);
        vos_print("Product Serial Number bytes  : %s \r\n", fmt_str);
        vos_print("Product Asset Tag type       : 0x%02x \r\n", fru_product_info[fru_id].product_asset_tag_type);
        FOMRT_FIELD_STR(fmt_str, fru_product_info[fru_id].product_asset_tag, 8);
        vos_print("Product Asset Tag bytes      : %s \r\n", fmt_str);
        vos_print("Product FRU File ID type     : 0x%02x \r\n", fru_product_info[fru_id].product_fru_fileid_type);
        vos_print("Product FRU File ID bytes    : 0x%02x \r\n", fru_product_info[fru_id].product_fru_fileid);
        vos_print("Custom Product Info No.1 type: 0x%02x \r\n", fru_product_info[fru_id].custom_product_info_n1_type);
        devm_show_custom_record(fru_product_info[fru_id].custom_product_info_n1_type, fru_product_info[fru_id].custom_product_info_n1);
        vos_print("Custom Product Info No.2 type: 0x%02x \r\n", fru_product_info[fru_id].custom_product_info_n2_type);
        devm_show_custom_record(fru_product_info[fru_id].custom_product_info_n2_type, fru_product_info[fru_id].custom_product_info_n2);
        vos_print("Custom Product Info No.3 type: 0x%02x \r\n", fru_product_info[fru_id].custom_product_info_n3_type);
        devm_show_custom_record(fru_product_info[fru_id].custom_product_info_n3_type, fru_product_info[fru_id].custom_product_info_n3);
        vos_print(" \r\n");
    }
    
    return VOS_OK;
}

int devm_fru_set_mac(int store, uint8 mac[6])
{
    int ret;
    FRU_BOARD_INFO *pInfo;

    pInfo = &fru_board_info[0];
    pInfo->custom_board_info_n1_type = 0x8;
    pInfo->custom_board_info_n1[0] = 0x1;
    pInfo->custom_board_info_n1[1] = 0x1;
    memcpy(&pInfo->custom_board_info_n1[2], mac, 6);
    pInfo->area_checksum = devm_get_area_checksum((uint8 *)pInfo, sizeof(FRU_BOARD_INFO) - 1);

    if (store) {
        ret = devm_store_area_data(0, fru_common_info[0].board_info_start, (uint8 *)pInfo, sizeof(FRU_BOARD_INFO));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int devm_fru_set_uuid(int store, uint8 uuid[16])
{
    int ret;
    FRU_BOARD_INFO *pInfo;

    pInfo = &fru_board_info[0];
    pInfo->custom_board_info_n2_type = 0x18;
    pInfo->custom_board_info_n2[0] = 0x2;
    pInfo->custom_board_info_n2[1] = 0x1;
    memcpy(&pInfo->custom_board_info_n2[2], uuid, 16);
    pInfo->area_checksum = devm_get_area_checksum((uint8 *)pInfo, sizeof(FRU_BOARD_INFO) - 1);

    if (store) {
        ret = devm_store_area_data(0, fru_common_info[0].board_info_start, (uint8 *)pInfo, sizeof(FRU_BOARD_INFO));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int devm_fru_get_uuid(char *uuid_str, int max)
{
    if (fru_common_info[0].board_info_start > 0 && fru_board_info[0].custom_board_info_n2_type > 0) {
        uint8 *ptr = fru_board_info[0].custom_board_info_n2;
        snprintf(uuid_str, max, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                    ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], 
                    ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15], ptr[16], ptr[17]);
		return VOS_OK;
    } 
    
    return VOS_ERR;
}

int devm_fru_get_skuid(uint8 *skuid)
{
    FRU_PRODUCT_INFO *pInfo;

    if (!skuid) return VOS_ERR;
    
    pInfo = &fru_product_info[0];
    if ( pInfo->custom_product_info_n1_type == 0x8 && pInfo->custom_product_info_n1[0] == 0x20 ) {
        *skuid = pInfo->custom_product_info_n1[2];
        return VOS_OK;
    }
    
    return VOS_ERR;
}

int devm_fru_set_skuid(int store, uint8 skuid)
{
    int ret;
    FRU_PRODUCT_INFO *pInfo;

    pInfo = &fru_product_info[0];
    pInfo->custom_product_info_n1_type = 0x8;
    pInfo->custom_product_info_n1[0] = 0x20;
    pInfo->custom_product_info_n1[1] = 0x1;
    pInfo->custom_product_info_n1[2] = skuid;
    pInfo->area_checksum = devm_get_area_checksum((uint8 *)pInfo, sizeof(FRU_PRODUCT_INFO) - 1);

    if (store) {
        ret = devm_store_area_data(0, fru_common_info[0].product_info_start, (uint8 *)pInfo, sizeof(FRU_PRODUCT_INFO));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }
    
    return VOS_OK;
}

int devm_fru_set_sn(int store, int fru_id, char *buffer, int buf_len)
{
    int ret;
    FRU_BOARD_INFO *pInfo;
    
    pInfo = &fru_board_info[fru_id];
    pInfo->board_serial_num_type = 0xD4;
    memset(pInfo->board_serial_num, 0, 20);
    memcpy(pInfo->board_serial_num, buffer, buf_len);

    if (store) {
        pInfo->area_checksum = devm_get_area_checksum((uint8 *)pInfo, sizeof(FRU_BOARD_INFO) - 1);
        ret = devm_store_area_data(fru_id, fru_common_info[fru_id].board_info_start, (uint8 *)pInfo, sizeof(FRU_BOARD_INFO));
        if (ret != VOS_OK) {
            vos_print("%d: devm_store_area_data failed, ret %d \r\n", __LINE__, ret);
            return VOS_ERR;
        }
    }

    return VOS_OK;
}

int devm_fru_get_sn(char *sn_str, int max)
{
    FRU_BOARD_INFO *pInfo;

    if (!sn_str) return VOS_ERR;
    
    pInfo = &fru_board_info[0];
    if ( fru_common_info[0].board_info_start > 0 && pInfo->board_serial_num_type == 0xD4 ) {
        snprintf(sn_str, max, "%s", pInfo->board_serial_num);
        return VOS_OK;
    }
    
    return VOS_ERR;
}

static int _get_json_val(uint8 *dst, int max_len, cJSON* root)
{
    int sub_cnt;
    int i, j;
    int mode = 0; //invalid
    
    sub_cnt = cJSON_GetArraySize(root);
    for (i = 0; i < sub_cnt; i++) {
        cJSON* sub_node = cJSON_GetArrayItem(root, i); 

        //vos_print("%d: sub_node->string %s \r\n", __LINE__, sub_node->string);
        if ( !strcmp(sub_node->string, "Data Format") )  {
            if ( !strcmp(sub_node->valuestring, "Binary") ) mode = 1;
            else if ( !strcmp(sub_node->valuestring, "ASCII") ) mode = 2;
            else return -1;
        }

        if ( !strcmp(sub_node->string, "MaxLen") )  {
            if (max_len != sub_node->valueint) return -2;
        }

        if ( !strcmp(sub_node->string, "value") && (mode == 1) )  {
            int p_size = cJSON_GetArraySize(sub_node);
            if (p_size > max_len) return -3;
            
            for (j = 0; j < p_size; j++) {
                cJSON* tmp_param = cJSON_GetArrayItem(sub_node, j);
                dst[j] = (uint8)tmp_param->valueint;
                //vos_print("%d: mode 1, value %d \r\n", __LINE__, tmp_param->valueint);
            }
            return VOS_OK;
        }
        else if ( !strcmp(sub_node->string, "value") && (mode == 2) )  {
            if (strlen(sub_node->valuestring) > max_len) return -4;
            memcpy((char *)dst, sub_node->valuestring, strlen(sub_node->valuestring));
            //vos_print("%d: mode 2, value %s \r\n", __LINE__, sub_node->valuestring);
            return VOS_OK;
        }
    }

    return VOS_ERR;
}

#define GET_JSON_VALUE(dst_ptr, max_len, js_node)    \
do {    \
    ret = _get_json_val((uint8 *)(dst_ptr), max_len, js_node);  \
    if (ret != VOS_OK) break;   \
    node_cnt++; \
} while(0)
    
static int drv_fru_json_parse1(FRU_COMMON_HEADER* area, cJSON* root_tree)
{
    int ret = VOS_OK;
    int node_cnt = 0;
    int i, sub_cnt;
    
    sub_cnt = cJSON_GetArraySize(root_tree);
    //vos_print("%d: sub_cnt %d \r\n", __LINE__, sub_cnt);
    for (i = 0; i < sub_cnt; ++i) {
        cJSON* sub_node = cJSON_GetArrayItem(root_tree, i); //each field

        //vos_print("%d: sub_node->string %s \r\n", __LINE__, sub_node->string);
        if ( !strcmp(sub_node->string, "Common Header Format Version") ) 
            GET_JSON_VALUE(&area->fmt_version, 1, sub_node);
        else if ( !strcmp(sub_node->string, "Internal Use Area Starting Offset") ) 
            GET_JSON_VALUE(&area->internal_use_start, 1, sub_node);
        else if ( !strcmp(sub_node->string, "Chassis Info Area Starting Offset") ) 
            GET_JSON_VALUE(&area->chassis_info_start, 1, sub_node);
        else if ( !strcmp(sub_node->string, "Board Info Area Starting Offset") ) 
            GET_JSON_VALUE(&area->board_info_start, 1, sub_node);
        else if ( !strcmp(sub_node->string, "Product Info Area Starting Offset") ) 
            GET_JSON_VALUE(&area->product_info_start, 1, sub_node);
        else if ( !strcmp(sub_node->string, "MultiRecord Area Starting Offset") ) 
            GET_JSON_VALUE(&area->multi_record_start, 1, sub_node);
    }

    if (ret != VOS_OK || node_cnt != 6) {
        vos_print("%d: json parse error(%d)\r\n", __LINE__, ret);
    }
    
    return ret;
}

static int drv_fru_json_parse2(FRU_CHASSIS_INFO* area, cJSON* root_tree)
{
    int ret = VOS_OK;
    int node_cnt = 0;
    int i, sub_cnt;
    
    sub_cnt = cJSON_GetArraySize(root_tree);
    for (i = 0; i < sub_cnt; ++i) {
        cJSON* sub_node = cJSON_GetArrayItem(root_tree, i); //each field

        //vos_print("%d: sub_node->string %s \r\n", __LINE__, sub_node->string);
        if ( !strcmp(sub_node->string, "Chassis Info Area Format Version") ) {
            GET_JSON_VALUE(&area->fmt_version, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Chassis Info Area Length") ) {
            GET_JSON_VALUE(&area->area_len, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Chassis Type") )  {
            GET_JSON_VALUE(&area->chassis_type, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Chassis Part Number bytes") )  {
            GET_JSON_VALUE(&area->chassis_part_num, 20, sub_node);
            area->chassis_part_num_type = 0xD4;
        } else if ( !strcmp(sub_node->string, "Chassis Serial Number bytes") )  {
            GET_JSON_VALUE(&area->chassis_serial_num, 20, sub_node);
            area->chassis_serial_num_type = 0xD4;
        } else if ( !strcmp(sub_node->string, "Custom Chassis Info No.1 bytes") )  {
            GET_JSON_VALUE(&area->chassis_info_n1, 16, sub_node);
            area->chassis_info_n1_type = 0x10;
        } else if ( !strcmp(sub_node->string, "End of Fields marker") )  {
            GET_JSON_VALUE((uint8 *)&area->end_marker, 1, sub_node);
        }
    }

    if (ret != VOS_OK || node_cnt != 7) {
        vos_print("%d: json parse error(%d)\r\n", __LINE__, ret);
    }
    
    return ret;
}

static int drv_fru_json_parse3(FRU_BOARD_INFO* area, cJSON* root_tree)
{
    int ret = VOS_OK;
    int node_cnt = 0;
    int i, sub_cnt;
    
    sub_cnt = cJSON_GetArraySize(root_tree);
    for (i = 0; i < sub_cnt; ++i) {
        cJSON* sub_node = cJSON_GetArrayItem(root_tree, i); //each field

        //vos_print("%d: sub_node->string %s \r\n", __LINE__, sub_node->string);
        if ( !strcmp(sub_node->string, "Board Info Area Format Version") )  { 
            GET_JSON_VALUE(&area->fmt_version, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Board Info Area Length") )  {
            GET_JSON_VALUE(&area->area_len, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Board Info Language Code") )  {
            GET_JSON_VALUE(&area->language_code, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Board Manufacturing Date") )  {
            GET_JSON_VALUE(&area->Manufacturing_time, 3, sub_node);
        } else if ( !strcmp(sub_node->string, "Board Manufacturer bytes") )  {
            GET_JSON_VALUE(&area->manufacturer_name, 16, sub_node);
            area->manufacturer_type = 0xD0;
        } else if ( !strcmp(sub_node->string, "Board Name bytes") )  {
            GET_JSON_VALUE(&area->board_name, 16, sub_node);
            area->board_name_type = 0xD0;
        } else if ( !strcmp(sub_node->string, "Board Serial Number bytes") )  {
            GET_JSON_VALUE(&area->board_serial_num, 20, sub_node);
            area->board_serial_num_type = 0xD4;
        } else if ( !strcmp(sub_node->string, "Board Part Number bytes") )  {
            GET_JSON_VALUE(&area->board_part_num, 20, sub_node);
            area->board_part_num_type = 0xD4;
        } else if ( !strcmp(sub_node->string, "Board FRU File ID bytes") )  {
            GET_JSON_VALUE(&area->board_fru_fileid, 1, sub_node);
            area->board_fru_fileid_type = 0x01;
        } else if ( !strcmp(sub_node->string, "Custom Board Info No.1 bytes") )  {
            GET_JSON_VALUE(&area->custom_board_info_n1, 8, sub_node);
            area->custom_board_info_n1_type = 0x08;
        } else if ( !strcmp(sub_node->string, "Custom Board Info No.2 bytes") )  {
            GET_JSON_VALUE(&area->custom_board_info_n2, 24, sub_node);
            area->custom_board_info_n2_type = 0x18;
        } else if ( !strcmp(sub_node->string, "Custom Board Info No.3 bytes") )  {
            GET_JSON_VALUE(&area->custom_board_info_n3, 63, sub_node);
            area->custom_board_info_n3_type = 0x3F;
        } else if ( !strcmp(sub_node->string, "Custom Board Info No.4 bytes") )  {
            GET_JSON_VALUE(&area->custom_board_info_n4, 63, sub_node);
            area->custom_board_info_n4_type = 0x3F;
        } else if ( !strcmp(sub_node->string, "End of Fields marker") )  {
            GET_JSON_VALUE(&area->end_marker, 1, sub_node);
        }
    }

    if (ret != VOS_OK || node_cnt != 14) {
        vos_print("%d: json parse error(%d)\r\n", __LINE__, ret);
    }
    
    return ret;
}

static int drv_fru_json_parse4(FRU_PRODUCT_INFO* area, cJSON* root_tree)
{
    int ret = VOS_OK;
    int node_cnt = 0;
    int i, sub_cnt;
    
    sub_cnt = cJSON_GetArraySize(root_tree);
    for (i = 0; i < sub_cnt; ++i) {
        cJSON* sub_node = cJSON_GetArrayItem(root_tree, i); //each field

        //vos_print("%d: sub_node->string %s \r\n", __LINE__, sub_node->string);
        if ( !strcmp(sub_node->string, "Product Info Area Format Version") )   {
            GET_JSON_VALUE(&area->fmt_version, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Product Info Area Length") )   {
            GET_JSON_VALUE(&area->area_len, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Product Info Language Code") )   {
            GET_JSON_VALUE(&area->language_code, 1, sub_node);
        } else if ( !strcmp(sub_node->string, "Product Manufacturer bytes") )   {
            GET_JSON_VALUE(&area->manufacturer_name, 16, sub_node);
            area->manufacturer_type = 0xD0;
        } else if ( !strcmp(sub_node->string, "Product Name bytes") )   {
            GET_JSON_VALUE(&area->product_name, 16, sub_node);
            area->product_name_type = 0xD0;
        } else if ( !strcmp(sub_node->string, "Product Part Number bytes") )   {
            GET_JSON_VALUE(&area->product_part_num, 20, sub_node);
            area->product_part_num_type = 0xD4;
        } else if ( !strcmp(sub_node->string, "Product Version bytes") )   {
            GET_JSON_VALUE(&area->product_version, 8, sub_node);
            area->product_version_type = 0xC8;
        } else if ( !strcmp(sub_node->string, "Product Serial Number bytes") )   {
            GET_JSON_VALUE(&area->product_serial_num, 20, sub_node);
            area->product_serial_num_type = 0xD4;
        } else if ( !strcmp(sub_node->string, "Product Asset Tag bytes") )   {
            GET_JSON_VALUE(&area->product_asset_tag, 8, sub_node);
            area->product_asset_tag_type = 0xC8;
        } else if ( !strcmp(sub_node->string, "Product FRU File ID bytes") )   {
            GET_JSON_VALUE(&area->product_fru_fileid, 1, sub_node);
            area->product_fru_fileid_type = 0x01;
        } else if ( !strcmp(sub_node->string, "Custom Product Info No.1 bytes") )   {
            GET_JSON_VALUE(&area->custom_product_info_n1, 8, sub_node);
            area->custom_product_info_n1_type = 0x08;
        } else if ( !strcmp(sub_node->string, "Custom Product Info No.2 bytes") )   {
            GET_JSON_VALUE(&area->custom_product_info_n2, 24, sub_node);
            area->custom_product_info_n2_type = 0x18;
        } else if ( !strcmp(sub_node->string, "Custom Product Info No.3 bytes") )   {
            GET_JSON_VALUE(&area->custom_product_info_n3, 56, sub_node);
            area->custom_product_info_n3_type = 0x38;
        } else if ( !strcmp(sub_node->string, "End of Fields marker") )   {
            GET_JSON_VALUE(&area->end_marker, 1, sub_node);
        }
    }

    if (ret != VOS_OK || node_cnt != 14) {
        vos_print("%d: json parse error(%d)\r\n", __LINE__, ret);
    }
    
    return ret;
}

int devm_fru_load_json(int fru_id, char *json_file)
{
    int ret = VOS_OK;
    char *json = NULL;
    cJSON* root_tree;
    cJSON* sub_tree;
    FRU_COMMON_HEADER   common_info;
    FRU_CHASSIS_INFO    chassis_info;
    FRU_BOARD_INFO      board_info;
    FRU_PRODUCT_INFO    product_info;

    json = json_read_file(json_file);
    if ((json == NULL) || (json[0] == '\0') || (json[1] == '\0')) {
        vos_print("file %s is invalid \r\n", json_file);
        return VOS_ERR;
    }
 
    root_tree = cJSON_Parse(json);
    if (root_tree == NULL) {
        vos_print("parse json file fail \r\n");
        goto FUNC_EXIT;
    }

    memset(&common_info, 0, sizeof(common_info));
    sub_tree = cJSON_GetObjectItem(root_tree, "Common Header");
    if (sub_tree == NULL) {
        vos_print("no common info \r\n");
        goto FUNC_EXIT;
    } else {
        ret = drv_fru_json_parse1(&common_info, sub_tree);
    }

    xlog(XLOG_WARN, "load %s to fru %d \r\n", json_file, fru_id);
    if (dbg_mode) {
        vos_print("common_info.fmt_version %d\r\n", common_info.fmt_version);
        vos_print("common_info.internal_use_start %d\r\n", common_info.internal_use_start);
        vos_print("common_info.chassis_info_start %d\r\n", common_info.chassis_info_start);
        vos_print("common_info.board_info_start %d\r\n", common_info.board_info_start);
        vos_print("common_info.product_info_start %d\r\n", common_info.product_info_start);
        vos_print("common_info.multi_record_start %d\r\n", common_info.multi_record_start);
    }

    memset(&chassis_info, 0, sizeof(chassis_info));
    sub_tree = cJSON_GetObjectItem(root_tree, "Chassis Info");
    if (sub_tree != NULL) {
        ret |= drv_fru_json_parse2(&chassis_info, sub_tree);
    }

    memset(&board_info, 0, sizeof(board_info));
    sub_tree = cJSON_GetObjectItem(root_tree, "Board Info");
    if (sub_tree != NULL) {
        ret |= drv_fru_json_parse3(&board_info, sub_tree);
    }

    memset(&product_info, 0, sizeof(product_info));
    sub_tree = cJSON_GetObjectItem(root_tree, "Product Info");
    if (sub_tree != NULL) {
        ret |= drv_fru_json_parse4(&product_info, sub_tree);
    }    

    if (ret != VOS_OK) {
        vos_print("parse json file fail \r\n");
        goto FUNC_EXIT;
    }

    if (common_info.fmt_version > 0) {
        common_info.area_checksum = devm_get_area_checksum((uint8 *)&common_info, sizeof(common_info) - 1);
        ret = devm_store_area_data(fru_id, 0, (uint8 *)&common_info, sizeof(common_info));
        vos_print("saving common info %s \r\n", (ret == VOS_OK) ? "OK":"Failed");
    }
    
    if (chassis_info.fmt_version > 0) {
        chassis_info.area_checksum = devm_get_area_checksum((uint8 *)&chassis_info, sizeof(chassis_info) - 1);
        ret = devm_store_area_data(fru_id, common_info.chassis_info_start, (uint8 *)&chassis_info, sizeof(chassis_info));
        vos_print("saving chassis info %s \r\n", (ret == VOS_OK) ? "OK":"Failed");
    }
    
    if (board_info.fmt_version > 0) {
        board_info.area_checksum = devm_get_area_checksum((uint8 *)&board_info, sizeof(board_info) - 1);
        ret = devm_store_area_data(fru_id, common_info.board_info_start, (uint8 *)&board_info, sizeof(board_info));
        vos_print("saving board info %s \r\n", (ret == VOS_OK) ? "OK":"Failed");
    }
    
    if (product_info.fmt_version > 0) {
        product_info.area_checksum = devm_get_area_checksum((uint8 *)&product_info, sizeof(product_info) - 1);
        ret = devm_store_area_data(fru_id, common_info.product_info_start, (uint8 *)&product_info, sizeof(product_info));
        vos_print("saving product info %s \r\n", (ret == VOS_OK) ? "OK":"Failed");
    }    

FUNC_EXIT:
    if (root_tree != NULL) cJSON_Delete(root_tree);
    if (json != NULL) free(json);

    return VOS_OK;
}

int fru_info_test()
{
    devm_read_fru_info(0);
    devm_read_fru_info(1);

    devm_show_fru_info(0);
    devm_show_fru_info(1);
    return 0;
}

#endif

#if T_DESC("CLI", 1)

int cli_show_fru_info(int argc, char **argv)
{
    int ret;
    EEPROM_INFO info;
    int fru_id;

    if (argc < 2) {
        vos_print("Usage: %s <fru-id> \r\n", argv[0]);
        return CMD_ERR_PARAM;
    }
    fru_id = atoi(argv[1]);
	
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("invalid FRU-ID %d \r\n", fru_id);
        return VOS_ERR;
    }

    devm_show_fru_info(fru_id);
    return VOS_OK;
}    

int cli_fru_set_mac(int argc, char **argv)
{
    uint8 mac_addr[6];

    if (argc < 2) {
        vos_print("Usage: %s <mac-addr> \r\n", argv[0]);
        vos_print("       <mac-addr> mac in hex, such as 000c-293e-ce9d \r\n");
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
    uint8 hex_uuid[16];

    if (argc < 2) {
        vos_print("Usage: %s <uuid> \r\n", argv[0]);
        vos_print("       <uuid> uuid in hex string \r\n");
        return CMD_ERR_PARAM;
    }

    if ( parse_hex_string(argv[1], 16, hex_uuid) != VOS_OK) {
        vos_print("Invalid UUID \r\n");
        return VOS_OK;
    }
    
    devm_fru_set_uuid(TRUE, hex_uuid);
    return VOS_OK;
}    

int cli_fru_set_skuid(int argc, char **argv)
{
    uint8 sku_id;

    if (argc < 2) {
        vos_print("Usage: %s <skuid> \r\n", argv[0]);
        return CMD_ERR_PARAM;
    }

    sku_id = strtoul(argv[1], 0, 0);             
    devm_fru_set_skuid(TRUE, sku_id);
    return VOS_OK;
}  

int cli_fru_load_json(int argc, char **argv)
{
    int ret;
    int fru_id;
    EEPROM_INFO info;
    
    if (argc < 3) {
        vos_print("Usage: %s <fru-id> <jsonfile>\r\n", argv[0]);
        return CMD_ERR_PARAM;
    }

    fru_id = atoi(argv[1]);
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("invalid fru-id %d \r\n", fru_id);
        return VOS_ERR;
    }

    ret = devm_fru_load_json(fru_id, argv[2]);
    if (ret != VOS_OK) {
        vos_print("Failed to write fru info\r\n");
        return VOS_ERR;
    }
    
    return VOS_OK;
}  

int cli_fru_set_sn(int argc, char **argv)
{
    int ret;
    int fru_id;
    EEPROM_INFO info;

    if (argc < 3) {
        vos_print("Usage: %s <fru-id> <sn-str>\r\n", argv[0]);
        return CMD_ERR_PARAM;
    }

    fru_id = atoi(argv[1]);
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("invalid fru-id %d \r\n", fru_id);
        return VOS_ERR;
    }

    if (strlen(argv[2]) > 20) {
        vos_print("\r\n serial number too long \r\n");
        return VOS_OK;
    }

    vos_print("Save board serial num ... ");
    ret = devm_fru_set_sn(TRUE, fru_id, argv[2], strlen(argv[2]));
    if (ret != VOS_OK) vos_print("Failed \r\n");
    else vos_print("OK \r\n");
    
    return VOS_OK;
}  

#endif

#ifdef FRU_APP

int fru_set_sn(int argc, char **argv)
{
    int ret;
    int buf_len;
    char buffer[128];
    int fru_id;
    EEPROM_INFO info;

    if (argc < 2) {
        vos_print("Usage: %s <fru-id> \r\n", argv[0]);
        return VOS_OK;
    }

    fru_id = atoi(argv[1]);
    ret = drv_get_eeprom_info(fru_id, &info);
    if (ret != VOS_OK) {
        vos_print("invalid fru-id %d \r\n", fru_id);
        return VOS_ERR;
    }

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

    vos_print("Step <2> Input board serial number: ");
    fgets(buffer, sizeof(buffer), stdin);
    buf_len = strlen(buffer);
    if (buf_len > 0) buffer[buf_len - 1] = 0;
    //vos_print("\r\n fgets: %d, %s \r\n", strlen(buffer), buffer);

    if (strlen(buffer) > 20) {
        vos_print("\r\n serial number too long \r\n");
        return VOS_OK;
    }

    vos_print("Step <3> Save board serial num ... ");
    ret = devm_fru_set_sn(TRUE, fru_id, buffer, strlen(buffer));
    if (ret != VOS_OK) vos_print("Failed \r\n");
    else vos_print("OK \r\n");
    
    return VOS_OK;
}  

/*
## compile
aarch64-linux-gnu-gcc -DFRU_APP -o fru.bin drv_i2c.c devm_fru.c vos.c  cJSON.c -I../include -lrt -lpthread
*/
int main(int argc, char **argv)
{
    if (argc < 2) {
        vos_print("Usage:\r\n");
        vos_print("  show <fru-id>              -- show fru info \r\n");
        vos_print("  load <fru-id> <jsonfile>   -- load json fru \r\n");
        vos_print("  set_mac <mac-addr>         -- set board mac addr \r\n");
        vos_print("  set_sku <skuid>            -- set board SKU ID \r\n");
        vos_print("  set_uuid <uuid>            -- set board UUID \r\n");
        vos_print("  set_sn <fru-id>            -- set board serial number \r\n");
        return VOS_OK;
    }

    devm_read_fru_info(0);

    if (!strcmp(argv[1], "show")) 
        cli_show_fru_info(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "load")) 
        cli_fru_load_json(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_mac")) 
        cli_fru_set_mac(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_sku")) 
        cli_fru_set_skuid(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_uuid")) 
        cli_fru_set_uuid(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "set_sn")) 
        fru_set_sn(argc - 1, &argv[1]);
    else 
        vos_print("unknown cmd \r\n");

    return VOS_OK;
}

#else

int devm_load_fru_info(void)
{   
    int ret;
    char temp_buf[128];
    char uuid_str[128];
    char sn_str[32];
    uint8 uuid[16];

    ret = devm_read_fru_info(0);
    if (ret != VOS_OK) {
		devm_fru_load_json(0, "/home/app/dft/g70_fru_demo.json");
		ret = devm_read_fru_info(0); //try again
        if (ret != VOS_OK) return VOS_ERR;
    }

    //show board_serial_num
	if(fru_common_info[0].board_info_start > 0 && fru_board_info[0].board_serial_num_type > 0) {
        snprintf(temp_buf, 20, "%s", fru_board_info[0].board_serial_num);
        temp_buf[20] = 0;
	    xlog(XLOG_WARN, "devm_read_fru_info: Board SN %s", temp_buf);
    } 

#if 0 //mac address
    if (fru_board_info[0].custom_board_info_n1_type > 0 && fru_board_info[0].custom_board_info_n1[0] > 0) {
        char mac_addr[32];
        uint8 *ptr = fru_board_info[0].custom_board_info_n1;
        sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
        xlog(XLOG_WARN, "Board Mac: %s", mac_addr);
        sprintf(temp_buf, "ifconfig %s hw ether %s up", "eth0", mac_addr);
        shell_run_cmd("ifconfig eth0 down");
        shell_run_cmd(temp_buf);
    } 
#endif

    //show uuid
    if(fru_board_info[0].custom_board_info_n2_type > 0 && fru_board_info[0].custom_board_info_n2[0] > 0) {
        uint8 *ptr = fru_board_info[0].custom_board_info_n2;
        sprintf(uuid_str, "%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x", 
                    ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], 
                    ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15], ptr[16], ptr[17]);
    } else {
        sys_node_readstr("/sys/class/mmc_host/mmc0/mmc0:0001/cid", uuid_str, sizeof(uuid_str));
        xlog(XLOG_WARN, "mmc cid to uuid_str: %s", uuid_str);
        parse_hex_string(uuid_str, 16, uuid);
        devm_fru_set_uuid(TRUE, uuid);
    }
	xlog(XLOG_WARN, "devm_read_fru_info: Product UUID %s", uuid_str);

    return VOS_OK;
}

int devm_fru_init(void)
{
	devm_load_fru_info();

	cli_cmd_reg("fru_show",     	"show fru info",       		&cli_show_fru_info);
	cli_cmd_reg("fru_load",    		"load fru from json",       &cli_fru_load_json);
	cli_cmd_reg("fru_set_sn",    	"set board sn",       		&cli_fru_set_sn);
	cli_cmd_reg("fru_set_uuid",    	"set board uuid",      		&cli_fru_set_uuid);

	return VOS_OK;
}

#endif

