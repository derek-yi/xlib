
#ifndef _DEV_FRU_H_
#define _DEV_FRU_H_


/*************************************************************************
 * fru data structure
 *************************************************************************/
#define MAX_FRU_NUM             1
#define FRU_MAX_CNT             128
#define FRU_MAGIC_BYTE          0x5A

#define FRU_TYPE_BOARD_SN       0x01    //ascii < 30B
#define FRU_TYPE_CPU_ID         0x02    //binary 16B
#define FRU_TYPE_MMC_ID         0x03    //binary 16B
#define FRU_TYPE_RECORD         0x10    //ascii < 30B
#define FRU_TYPE_MAC            0x20    //binary 6B

typedef struct 
{
    uint8   magic;  //FRU_MAGIC_BYTE
    uint8   type;
    uint8   data[30];
}FRU_ENTRY_ST;


#endif
