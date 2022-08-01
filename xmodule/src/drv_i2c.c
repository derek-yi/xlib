
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>	/* for NAME_MAX */
#include <sys/ioctl.h>
#include <string.h>
#include <strings.h>	/* for strcasecmp() */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <sys/time.h>

#include "drv_i2c.h"

#ifndef VOS_OK
#define VOS_OK      0
#define VOS_ERR     (-1)
#endif

#ifdef APP_TEST

#define vos_msleep(x)   usleep((x)*1000)

#endif

#if 1 //from https://github.com/ev3dev/i2c-tools

int open_i2c_dev(int i2cbus, int quiet)
{
	int file;
    char filename[64];

	snprintf(filename, sizeof(filename), "/dev/i2c/%d", i2cbus);
	file = open(filename, O_RDWR);

	if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
		sprintf(filename, "/dev/i2c-%d", i2cbus);
		file = open(filename, O_RDWR);
	}

	if (file < 0 && !quiet) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
				i2cbus, i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
	}

	return file;
}

int set_slave_addr(int file, int address, int force)
{
	/* With force, let the user read from/write to the registers
	   even when a driver is also running */
	if (ioctl(file, force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		return -errno;
	}

	return 0;
}

static __s32 i2c_smbus_access(int file, char read_write, __u8 command,
                                     int size, void *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}

static __s32 i2c_smbus_read_byte(int file)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,0,I2C_SMBUS_BYTE,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static __s32 i2c_smbus_read_word_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_WORD_DATA,&data))
		return -1;
	else
		return 0x0FFFF & data.word;
}

static __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_BYTE_DATA,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

/* Returns the number of read bytes */ 
__s32 i2c_smbus_read_block_data(int file, __u8 command,
                                              __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_BLOCK_DATA,&data))
		return -1;
	else {
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

__s32 i2c_smbus_read_i2c_block_data(int file, __u8 command,
                                                __u8 length, __u8 *values)
{
  union i2c_smbus_data data;
  int i;

  if (length > 32)
      length = 32;
  data.block[0] = length;
  if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
                       length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
                        I2C_SMBUS_I2C_BLOCK_DATA,&data))
      return -1;
  else {
      for (i = 1; i <= data.block[0]; i++)
          values[i-1] = data.block[i];
      return data.block[0];
  }
}

static __s32 i2c_smbus_write_byte(int file, __u8 value)
{
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,value,
	                        I2C_SMBUS_BYTE,NULL);
}

static __s32 i2c_smbus_write_word_data(int file, __u8 command,
                                              __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                        I2C_SMBUS_WORD_DATA, &data);
}

static __s32 i2c_smbus_write_byte_data(int file, __u8 command,
                                            __u8 value)
{
  union i2c_smbus_data data;
  data.byte = value;
  return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
                          I2C_SMBUS_BYTE_DATA, &data);
}

static __s32 i2c_smbus_write_block_data(int file, __u8 command,
                                               __u8 length, const __u8 *values)
{
    union i2c_smbus_data data;
    int i;
    if (length > 32)
        length = 32;
    for (i = 1; i <= length; i++)
        data.block[i] = values[i-1];
    data.block[0] = length;
    return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
                            I2C_SMBUS_BLOCK_DATA, &data);
}

static inline __s32 i2c_smbus_write_i2c_block_data(int file, __u8 command,
                                                  __u8 length,
                                                  const __u8 *values)
{
   union i2c_smbus_data data;
   int i;
   if (length > 32)
       length = 32;
   for (i = 1; i <= length; i++)
       data.block[i] = values[i-1];
   data.block[0] = length;
   return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
                           I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

#define MISSING_FUNC_FMT	"Error: Adapter does not have %s capability\n"

static int rd_check_funcs(int file, int size, int daddress, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus receive byte");
			return -1;
		}
		if (daddress >= 0
		 && !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read word");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

static int wr_check_funcs(int file, int size, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus write byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus write word");
			return -1;
		}
		break;

	case I2C_SMBUS_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus block write");
			return -1;
		}
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
			fprintf(stderr, MISSING_FUNC_FMT, "I2C block write");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

#endif

#if 1 //IOCTRL


#define DEFAULT_NUM_PAGES    8            /* we default to a 24C16 eeprom which has 8 pages */
#define BYTES_PER_PAGE       256          /* one eeprom page is 256 byte */
#define MAX_BYTES            8            /* max number of bytes to write in one chunk */
       /* ... note: 24C02 and 24C01 only allow 8 bytes to be written in one chunk.   *
        *  if you are going to write 24C04,8,16 you can change this to 16            */


int i2c_write_chunk(int i2c_bus, int dev_id, int offset, __u8 *data_p, int len)
{
    int ret;
    int file;
    int i;
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg             i2cmsg;
	char _buf[MAX_BYTES + 1];

    if (len > MAX_BYTES) {
        fprintf(stderr, "%d: Error \r\n", __LINE__);
        return VOS_ERR;
    }
        
    file = open_i2c_dev(i2c_bus, 0);
    if (file < 0) {
        fprintf(stderr, "%d: open_i2c_dev failed \r\n", __LINE__);
        return VOS_ERR;
    }

	_buf[0] = offset;    /* _buf[0] is the offset into the eeprom page! */
	for(i=0; i<len; i++) /* copy buf[0..n] -> _buf[1..n+1] */
	    _buf[1+i] = data_p[i];

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr  = dev_id;
	i2cmsg.flags = 0; //0-write, 1-read
	i2cmsg.len   = len + 1;
	i2cmsg.buf   = _buf;
 
    ret = ioctl(file, I2C_RDWR, (unsigned long)&msg_rdwr);
    if (ret < 0) {
        fprintf(stderr, "%d: ioctl failed \r\n", __LINE__);
        //return VOS_ERR;
    }

    close(file);
    return VOS_OK;
}

int i2c_read_chunk(int i2c_bus, int dev_id, int offset, __u8 *data_p, int len)
{
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg             i2cmsg;
    int file;
	int ret;

    ret = i2c_write_chunk(i2c_bus, dev_id, offset, NULL, 0);
    if (ret != VOS_OK) {
        fprintf(stderr, "%d: Error \r\n", __LINE__);
        return VOS_ERR;
    }

    file = open_i2c_dev(i2c_bus, 0);
    if (file < 0) {
        fprintf(stderr, "%d: open_i2c_dev failed \r\n", __LINE__);
        return VOS_ERR;
    }

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr  = dev_id;
	i2cmsg.flags = I2C_M_RD;
	i2cmsg.len   = len;
	i2cmsg.buf   = (char *)data_p;

    ret = ioctl(file, I2C_RDWR, (unsigned long)&msg_rdwr);
    if (ret < 0) {
        fprintf(stderr, "%d: ioctl failed \r\n", __LINE__);
        //return VOS_ERR;
    }

    close(file);
    return VOS_OK;    
}

#endif


#if 1 //SMBUS

int i2c_read_data(int i2c_bus, int mode, int dev_id, int offset)
{
    int file, ret;
    int pec = 0;

    //align to BYTES_PER_PAGE
    dev_id = dev_id + offset/BYTES_PER_PAGE;
    offset = offset%BYTES_PER_PAGE;
    
    file = open_i2c_dev(i2c_bus, 0);
	if (file < 0
	 || rd_check_funcs(file, mode, offset, pec)
	 || set_slave_addr(file, dev_id, 1)) {
	    fprintf(stderr, "Warning - open_i2c_dev failed\n");
        return -1;
    }

     switch (mode) {
     case I2C_SMBUS_BYTE:
         if (offset >= 0) {
             ret = i2c_smbus_write_byte(file, offset);
             if (ret < 0)
                 fprintf(stderr, "Warning - write failed\n");
         }
         ret = i2c_smbus_read_byte(file);
         break;
     case I2C_SMBUS_WORD_DATA:
         ret = i2c_smbus_read_word_data(file, offset);
         break;
     default: /* I2C_SMBUS_BYTE_DATA */
         ret = i2c_smbus_read_byte_data(file, offset);
         break;
     }
	close(file);

    //printf("read> 0x%0*x \r\n", mode == I2C_SMBUS_WORD_DATA ? 4 : 2, ret);
    return ret;
}


int i2c_read_buffer(int i2c_bus, int mode, int dev_id, int offset, __u8 *data_p, int len)
{
    int file, ret;
    int pec = 0;

    //align to BYTES_PER_PAGE
    dev_id = dev_id + offset/BYTES_PER_PAGE;
    offset = offset%BYTES_PER_PAGE;
    if ( offset/BYTES_PER_PAGE != (offset + len - 1)/BYTES_PER_PAGE ) {
        int fp_len = offset + len - BYTES_PER_PAGE; //first page len
	    ret = i2c_read_buffer(i2c_bus, mode, dev_id, offset, data_p, fp_len);
        ret |= i2c_read_buffer(i2c_bus, mode, dev_id + 1, 0, data_p + fp_len, len - fp_len);
        return ret;
    }

    file = open_i2c_dev(i2c_bus, 0);
	if (file < 0
	 || rd_check_funcs(file, mode, offset, pec)
	 || set_slave_addr(file, dev_id, 1)) {
	    fprintf(stderr, "Warning - open_i2c_dev failed\n");
        return -1;
    }

     switch (mode) {
     case I2C_SMBUS_BLOCK_DATA:
         //ret = i2c_smbus_read_block_data(file, daddress, data_p);
         ret = -1; //not work
         break;
     case I2C_SMBUS_I2C_BLOCK_DATA: //max block len is 32
         ret = i2c_smbus_read_i2c_block_data(file, offset, len, data_p);
         break;
     default: 
         ret = -1;
         break;
     }
	close(file);

    return ret;
}


int i2c_write_buffer(int i2c_bus, int mode, int dev_id, int offset, __u8 *data_p, int len)
{
    int file, ret;
    int pec = 0;
    int value;

    //align to BYTES_PER_PAGE
    dev_id = dev_id + offset/BYTES_PER_PAGE;
    offset = offset%BYTES_PER_PAGE;
    if ( offset/BYTES_PER_PAGE != (offset + len - 1)/BYTES_PER_PAGE ) {
        int fp_len = offset + len - BYTES_PER_PAGE; //first page len
        ret = i2c_write_buffer(i2c_bus, mode, dev_id, offset, data_p, fp_len);
        ret |= i2c_write_buffer(i2c_bus, mode, dev_id + 1, 0, data_p + fp_len, len - fp_len);
        return ret;
    }

    file = open_i2c_dev(i2c_bus, 0);
	if (file < 0
	 || wr_check_funcs(file, mode, pec)
	 || set_slave_addr(file, dev_id, 1)) {
	    fprintf(stderr, "Warning - open_i2c_dev failed\n");
        return -1;
    }

    switch (mode) {
    case I2C_SMBUS_BYTE:
        ret = i2c_smbus_write_byte(file, offset);
        break;
    case I2C_SMBUS_WORD_DATA:
        value = *(__u16 *)data_p;
        ret = i2c_smbus_write_word_data(file, offset, value);
        break;
    case I2C_SMBUS_BLOCK_DATA:
        ret = i2c_smbus_write_block_data(file, offset, len, data_p);
        break;
    case I2C_SMBUS_I2C_BLOCK_DATA: //offset align to 16, max block len is 16
        ret = i2c_smbus_write_i2c_block_data(file, offset, len, data_p);
        break;
    default: /* I2C_SMBUS_BYTE_DATA */
        value = *(__u8 *)data_p;
        ret = i2c_smbus_write_byte_data(file, offset, value);
        break;
    }

    if (ret < 0) {
        fprintf(stderr, "Error: Write failed\n");
    }

    close(file);
    //printf("write> daddress %d, len %d, ret %d \r\n", daddress, len, ret);
    return ret;
}

#endif

#ifdef APP_TEST

#define I2C_BUS		0x04
#define I2C_ADDR 	0x54
#define BUFF_SIZE	256

int i2c_byte_rw() 
{
    __u8 data_value[] = {0x40, 0x41};
    int ret;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
	
    //i2cset -f 6 0x50 0 0x40
    if ( i2c_write_buffer(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, 0, data_value, 1) != 0 ) {
       printf("%d: failed \r\n", __LINE__);
    }
    sleep(1); 

    ret = i2c_read_data(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, 0);
    if ( ret != 0x40 ) {
        printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
    }

    ret = i2c_read_data(I2C_BUS, I2C_SMBUS_BYTE, I2C_ADDR, 0);
    if ( ret != 0x40 ) {
        printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
    }

    return 0;
}

/*
root@analog:~# i2cdetect -l
i2c-3   i2c             i2c-0-mux (chan_id 2)                   I2C adapter
i2c-1   i2c             i2c-0-mux (chan_id 0)                   I2C adapter
i2c-8   i2c             i2c-0-mux (chan_id 7)                   I2C adapter
i2c-6   i2c             i2c-0-mux (chan_id 5)                   I2C adapter
i2c-4   i2c             i2c-0-mux (chan_id 3)                   I2C adapter
i2c-2   i2c             i2c-0-mux (chan_id 1)                   I2C adapter
i2c-0   i2c             Cadence I2C at e0004000                 I2C adapter
i2c-7   i2c             i2c-0-mux (chan_id 6)                   I2C adapter
i2c-5   i2c             i2c-0-mux (chan_id 4)                   I2C adapter

## DTS   
i2c_mux
    i2c@2
        eeprom@54
    i2c@3
        tmp75@48 
        tmp75@49 
    i2c@5
        tmp75@48
        tmp75@49
        tmp75@4A
        tmp75@4C 
        eeprom@50
    i2c@6 
        ina219@40
        ina219@41 
*/
int misc_rw_test() 
{
    int ret;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
	
    //tmp75@48 
    ret = i2c_read_data(6, I2C_SMBUS_BYTE_DATA, 0x48, 2);
    if ( ret != 0x4B ) {
       printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
    }

    //ina219@40
    ret = i2c_read_data(7, I2C_SMBUS_BYTE_DATA, 0x40, 0);
    if ( ret != 0x39 ) {
       printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
    }
	
    ret = i2c_read_data(7, I2C_SMBUS_BYTE_DATA, 0x40, 1);
    if ( ret != 0x07 ) {
       printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
    }

    return 0;
}

int i2c_buffer_rw1()
{
    __u8 mem_buff[64];
    __u8 rd_buff[64];
    int i, ret;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
    for(i = 0; i < sizeof(mem_buff); i++) {
        mem_buff[i] = 0x20 + i%60;
    }

    //i2c_write_block(6, I2C_SMBUS_BLOCK_DATA, 0x50, 0, mem_buff, sizeof(mem_buff)); //not work
    ret = i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 0, mem_buff, 16);
    vos_msleep(80);
    ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 16, mem_buff + 16, 16);
    vos_msleep(80);
    ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 32, mem_buff + 32, 16);
    vos_msleep(80);
    ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 48, mem_buff + 48, 16);
    vos_msleep(80);

    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
        return 0;
    }

    memset(rd_buff, 0, sizeof(rd_buff));
    for(i = 0, ret = 0; i < 64/32; i++) {
        ret |= i2c_read_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, i*32, rd_buff+i*32, 32); 
    }
    for(i = 0; i < 64; i++) {
        if (rd_buff[i] != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], rd_buff[i]);
        }
    }

    for(i = 0; i < 64; i++) {
        ret = i2c_read_data(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, i);
        if (ret != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], ret);
        }
    }

    return 0;
}


int i2c_buffer_rw2() //if offset not align to 16, write failed
{
    __u8 mem_buff[64];
    __u8 rd_buff[64];
    int i, ret;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
    for(i = 0; i < sizeof(mem_buff); i++) {
        mem_buff[i] = 0x20 + i%32;
    }

    ret = i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 0, mem_buff, 8);
    vos_msleep(100);
    ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 8, mem_buff + 8, 16);
    vos_msleep(100);
    ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, 24, mem_buff + 32, 8);
    vos_msleep(100);

    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
    }

    memset(rd_buff, 0, sizeof(rd_buff));
    ret = i2c_read_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, i*32, rd_buff+i*32, 32);
    for(i = 0; i < 32; i++) {
        if (rd_buff[i] != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], rd_buff[i]);
        }
    }

    return 0;
}

int i2c_buffer_rw3()
{
    __u8 mem_buff[BUFF_SIZE];
    __u8 rd_buff[BUFF_SIZE];
    int i, ret = 0;
    struct timeval t_start, t_end;
    int t_used = 0;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
    for(i = 0; i < sizeof(mem_buff); i++) {
        mem_buff[i] = (i + 20)%200;
    }

    gettimeofday(&t_start, NULL);
    for(i = 0; i < BUFF_SIZE; i++) {
        ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, i, &mem_buff[i], 1); //byte mode
        vos_msleep(5);
    }
    gettimeofday(&t_end, NULL);

    t_used = (t_end.tv_sec - t_start.tv_sec)*1000000+(t_end.tv_usec - t_start.tv_usec);
    t_used = t_used/1000; //us to ms
    printf("%d: t_used %d ms \r\n", __LINE__, t_used);

    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
    }

    memset(rd_buff, 0, sizeof(rd_buff));
    for(i = 0, ret = 0; i < BUFF_SIZE/32; i++) {
        ret |= i2c_read_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, i*32, rd_buff+i*32, 32);
    }
    for(i = 0; i < 1024; i++) {
        if (rd_buff[i] != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], rd_buff[i]);
        }
    }

    return 0;
}

int i2c_buffer_rw4()
{
    __u8 mem_buff[BUFF_SIZE];
    int i, ret = 0;
    struct timeval t_start, t_end;
    int t_used = 0;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
    for(i = 0; i < sizeof(mem_buff); i++) {
        mem_buff[i] = (i + 20)%200;
    }

    gettimeofday(&t_start, NULL);
    for(i = 0; i < BUFF_SIZE/16; i++) {
        ret |= i2c_write_buffer(I2C_BUS, I2C_SMBUS_I2C_BLOCK_DATA, I2C_ADDR, i*16, mem_buff+i*16, 16);
        vos_msleep(80);
    }
    gettimeofday(&t_end, NULL);

    t_used = (t_end.tv_sec - t_start.tv_sec)*1000000+(t_end.tv_usec - t_start.tv_usec);
    t_used = t_used/1000; //us to ms
    printf("%d: t_used %d ms \r\n", __LINE__, t_used);

    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
    }

    for(i = 0; i < BUFF_SIZE; i++) {
        ret = i2c_read_data(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, i);
        if (ret != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], ret);
        }
    }

    return 0;
}

int i2c_chunk_rw1(int page)
{
    __u8 mem_buff[BYTES_PER_PAGE];
    __u8 rd_buff[BYTES_PER_PAGE];
    int i, ret;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
    for(i = 0; i < sizeof(mem_buff); i++) {
        mem_buff[i] = 0x20 + i%60;
    }

    for(i = 0, ret = 0; i < BYTES_PER_PAGE/MAX_BYTES; i++) {
        ret |= i2c_write_chunk(I2C_BUS, I2C_ADDR + page, i*MAX_BYTES, mem_buff + i*MAX_BYTES, MAX_BYTES);
        vos_msleep(5);
    }
    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
    }


    memset(rd_buff, 0, sizeof(rd_buff));
    for(i = 0, ret = 0; i < BYTES_PER_PAGE/MAX_BYTES; i++) {
        ret |= i2c_read_chunk(I2C_BUS, I2C_ADDR + page, i*MAX_BYTES, rd_buff + i*MAX_BYTES, MAX_BYTES);
    }
    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
    }

    for(i = 0; i < BYTES_PER_PAGE; i++) {
        if (rd_buff[i] != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], rd_buff[i]);
        }
    }

    return 0;
}

int i2c_chunk_rw2(int page) //if offset not align to MAX_BYTES, write failed
{
    __u8 mem_buff[BYTES_PER_PAGE];
    __u8 rd_buff[BYTES_PER_PAGE];
    int i, ret;

	printf("================= %d: %s ============== \r\n", __LINE__, __func__);
    for(i = 0; i < sizeof(mem_buff); i++) {
        mem_buff[i] = 0x20 + i%60;
    }

    ret = i2c_write_chunk(I2C_BUS, I2C_ADDR + page, 11, mem_buff, MAX_BYTES);
    vos_msleep(5);
    if ( ret != 0 ) {
        printf("%d: failed, write failed \r\n", __LINE__);
    }

    memset(rd_buff, 0, sizeof(rd_buff));
    ret = i2c_read_chunk(I2C_BUS, I2C_ADDR + page, 11, rd_buff, MAX_BYTES);
    for(i = 0; i < MAX_BYTES; i++) {
        if (rd_buff[i] != mem_buff[i]) {
            printf("%d: index %d, write 0x%x, read 0x%x \r\n", __LINE__, i, mem_buff[i], rd_buff[i]);
        }
    }

    return 0;
}

int main()
{
    i2c_byte_rw();
    //misc_rw_test();
    
    i2c_buffer_rw1();
    //i2c_buffer_rw2();
    //i2c_buffer_rw3(); //fail
    i2c_buffer_rw4();

    i2c_chunk_rw1(0);
	i2c_chunk_rw1(1);
	
    i2c_chunk_rw2(0);
    
    //fru_info_test();

    return 0;
}

#endif

