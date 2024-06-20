#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>	/* for NAME_MAX */
#include <sys/ioctl.h>
#include <string.h>
#include <strings.h>	/* for strcasecmp() */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <sys/time.h>
#include <pthread.h>

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
//https://github.com/costad2/i2c-tools/blob/master/tools/i2ctransfer.c

#define DEFAULT_NUM_PAGES    8            /* we default to a 24C16 eeprom which has 8 pages */
#define BYTES_PER_PAGE       256          /* one eeprom page is 256 byte */
#define MAX_BYTES            8            /* max number of bytes to write in one chunk */
       /* ... note: 24C02 and 24C01 only allow 8 bytes to be written in one chunk.   *
        *  if you are going to write 24C04,8,16 you can change this to 16            */

#define PRINT_STDERR	(1 << 0)
#define PRINT_READ_BUF	(1 << 1)
#define PRINT_WRITE_BUF	(1 << 2)
#define PRINT_HEADER	(1 << 3)

static pthread_mutex_t fpga_rw_mutex = PTHREAD_MUTEX_INITIALIZER;

int verbose = 0;

static void print_msgs(struct i2c_msg *msgs, __u32 nmsgs, unsigned flags)
{
	__u32 i, j;
	FILE *output = flags & PRINT_STDERR ? stderr : stdout;

	for (i = 0; i < nmsgs; i++) {
		int read = !!(msgs[i].flags & I2C_M_RD);
		int newline = !!(flags & PRINT_HEADER);

		if (flags & PRINT_HEADER)
			fprintf(output, "Msg %u: addr 0x%02x, %s, len %u",
				i, msgs[i].addr, read ? "read" : "write", msgs[i].len);
		if (msgs[i].len &&
		   (read == !!(flags & PRINT_READ_BUF) ||
		   !read == !!(flags & PRINT_WRITE_BUF))) {
			if (flags & PRINT_HEADER)
				fprintf(output, ", buf ");
			for (j = 0; j < msgs[i].len; j++)
				fprintf(output, "0x%02x ", msgs[i].buf[j]);
			newline = 1;
		}
		if (newline)
			fprintf(output, "\n");
	}
}

int i2c_write_chunk(int i2c_bus, int dev_id, int offset, char *data_p, int len)
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

	_buf[0] = (offset >> 8) & 0xFF;
	_buf[1] = (offset) & 0xFF;
	for(i=0; i<len; i++) /* copy buf[0..n] -> _buf[1..n+1] */
	    _buf[2+i] = data_p[i];

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;
	i2cmsg.addr  = dev_id;
	i2cmsg.flags = 0; //0-write, 1-read
	i2cmsg.len   = len + 2;
	i2cmsg.buf   = _buf;
    ret = ioctl(file, I2C_RDWR, (unsigned long)&msg_rdwr);
    if (ret < 0) {
        fprintf(stderr, "%d: ioctl failed \r\n", __LINE__);
        return VOS_ERR;
    }

    if (verbose)
    print_msgs(&i2cmsg, ret, PRINT_READ_BUF | (verbose ? PRINT_HEADER | PRINT_WRITE_BUF : 0));

    close(file);
    return VOS_OK;
}

int i2c_read_chunk(int i2c_bus, int dev_id, int offset, char *data_p, int len)
{
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg i2cmsg[2];
	char _buf[MAX_BYTES + 1];
    int file;
	int ret;

    file = open_i2c_dev(i2c_bus, 0);
    if (file < 0) {
        fprintf(stderr, "%d: open_i2c_dev failed \r\n", __LINE__);
        return VOS_ERR;
    }

	_buf[0] = (offset >> 8) & 0xFF;
	_buf[1] = (offset) & 0xFF;
	i2cmsg[0].addr  = dev_id;
	i2cmsg[0].flags = 0; //0-write, 1-read
	i2cmsg[0].len   = 2;
	i2cmsg[0].buf   = _buf;

	i2cmsg[1].addr  = dev_id;
	i2cmsg[1].flags = I2C_M_RD | I2C_M_NOSTART;
	i2cmsg[1].len   = len;
	i2cmsg[1].buf   = (char *)data_p;

	msg_rdwr.msgs = i2cmsg;
	msg_rdwr.nmsgs = 2;
    ret = ioctl(file, I2C_RDWR, (unsigned long)&msg_rdwr);
    if (ret < 0) {
        fprintf(stderr, "%d: ioctl failed \r\n", __LINE__);
        return VOS_ERR;
    }

    if (verbose)
    print_msgs(i2cmsg, ret, PRINT_READ_BUF | (verbose ? PRINT_HEADER | PRINT_WRITE_BUF : 0));

    close(file);
    return VOS_OK;    
}

int fpga_read(uint32_t address) 
{
    int ret, data;
    char buff[8];

    pthread_mutex_lock(&fpga_rw_mutex);
    ret = i2c_read_chunk(3, 0x50, address, buff, 4);
    data = buff[0];
    data = (data << 8) | buff[1];
    data = (data << 8) | buff[2];
    data = (data << 8) | buff[3];
    pthread_mutex_unlock(&fpga_rw_mutex);

    return ret == VOS_OK ? data : 0xFFFFFFFF;
}

int fpga_write(uint32_t address, uint32_t value) 
{
    int ret;
    char buff[8];

    pthread_mutex_lock(&fpga_rw_mutex);
    buff[0] = (value >> 24) & 0xFF;
    buff[1] = (value >> 16) & 0xFF;
    buff[2] = (value >> 8) & 0xFF;
    buff[3] = value & 0xFF;
    ret = i2c_write_chunk(3, 0x50, address, buff, 4);
    pthread_mutex_unlock(&fpga_rw_mutex);

    return ret;
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

int i2c_write_data(int i2c_bus, int mode, int dev_id, int offset, __u16 data_w)
{
    int file, ret;
    int pec = 0;
    int value;

    //align to BYTES_PER_PAGE
    dev_id = dev_id + offset/BYTES_PER_PAGE;
    offset = offset%BYTES_PER_PAGE;

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
        value = (__u16)data_w;
        ret = i2c_smbus_write_word_data(file, offset, value);
        break;
    default: /* I2C_SMBUS_BYTE_DATA */
        value = (__u8)data_w;
        ret = i2c_smbus_write_byte_data(file, offset, value);
        break;
    }

    if (ret < 0) {
        fprintf(stderr, "Error: Write failed\n");
    }

    close(file);
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

#define I2C_BUS		0x08
#define I2C_ADDR 	0x54
#define BUFF_SIZE	256

int i2c_byte_rw() 
{
    int ret;

    //eeprom
    if ( i2c_write_data(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, 0x40, 0x30) != 0 ) {
       printf("%d: failed \r\n", __LINE__);
       return 0;
    }

    ret = i2c_read_data(I2C_BUS, I2C_SMBUS_BYTE_DATA, I2C_ADDR, 0x40);
    if ( ret != 0x30 ) {
        printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
        return 0;
    }

    //tmp75@48 
    ret = i2c_read_data(8, I2C_SMBUS_BYTE_DATA, 0x48, 2);
    if ( ret != 0x4B ) {
       printf("%d: failed, read 0x%x \r\n", __LINE__, ret);
       return 0;
    }

    printf("i2c_byte_rw pass \n");
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int i2c_bus, dev_id, addr, data;
    char buff[8];
    
    if (argc < 2) {
        fprintf(stderr, "usage: %s <addr> <32> <data> \n", argv[0]);
        fprintf(stderr, "       %s <addr> [<32>] \n", argv[0]);
        return 0;
    }

    i2c_bus = 3;
    dev_id  = 0x50;
    addr    = strtoul(argv[1], 0, 0);
    if (argc < 4) {
        printf("0x%08X \n", fpga_read(addr));
    } else {
        data = (uint32_t)strtoul(argv[3], 0, 0);
        ret = fpga_write(addr, data);
        if (ret != VOS_OK) {
            printf("fpga_write failed \r\n");
            return 0;
        }
    }

    return 0;
}

#endif

