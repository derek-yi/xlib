
#ifndef _MESH_NODE_H_
#define _MESH_NODE_H_

#ifndef uint32
typedef unsigned int uint32;
#endif

#define VOS_OK          0
#define VOS_ERR         1

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif


#define MNODE_MAGIC_NUM         0x5AA51001
#define MNODE_CMD_HELLO         100
#define MNODE_CMD_ECHO          101
#define MNODE_CMD_USER_CMD      102


typedef struct _mnode_msg
{
    uint32 magic_num;
    uint32 cmd;
    uint32 ack;
    
    uint32 data_len;
    char data[4];
}mnode_msg;



#endif
