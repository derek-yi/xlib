#include "vos.h"

#if 1

#include <linux/spi/spidev.h>

static int spi_debug = 0;

static int spi_retry_max = 3;

int vos_spi_read(int fd, int width, uint16_t addr)
{
	int ret;
	uint16_t cmd;
	uint8_t tx_buf[4];
	uint8_t rx_buf[4];
	struct spi_ioc_transfer xfer[2];
    int value = -1;
	
	memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
	memset(rx_buf, 0x0, sizeof(rx_buf));

	cmd = 0X8000 | addr;
	tx_buf[0] = cmd >> 8;
	tx_buf[1] = cmd & 0xFF;
	xfer[0].tx_buf	= (unsigned long)tx_buf;
	xfer[0].rx_buf	= (unsigned long)NULL;
	xfer[0].len 	= 2;

	xfer[1].tx_buf	= (unsigned long)NULL;
	xfer[1].rx_buf	= (unsigned long)rx_buf;
	xfer[1].len 	= width/8;

    for (int i = 0; i < spi_retry_max; i++) {
    	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &xfer);
    	if (ret < 1 ) {
    		if (spi_debug) printf("can't transfer, ret %d \n", ret);
        } else {
    	    if (spi_debug) printf("rx_buf: %x %x %x %x\r\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
            value = rx_buf[0];
            if (width > 8) value = (value << 8) | rx_buf[1];
            if (width > 16) {
                value = (value << 8) | rx_buf[2];
                value = (value << 8) | rx_buf[3];
            }
            break;
        }
    }

	return value;
}

int vos_spi_write(int fd, int width, uint16_t addr, uint32_t data)
{ 
	int ret;
	__u16 cmd, tx_len;
	__u8 tx_buf[8];
	struct spi_ioc_transfer xfer[2];

	cmd = addr;
	tx_buf[0] = cmd >> 8;
	tx_buf[1] = cmd & 0xFF;
    if (width > 16) {
    	tx_buf[2] = (data >> 24) & 0xFF;
    	tx_buf[3] = (data >> 16) & 0xFF;
    	tx_buf[4] = (data >> 8)  & 0xFF;
    	tx_buf[5] = data & 0xFF;
        tx_len = 6;
    } else if (width > 8) {
    	tx_buf[2] = (data >> 8)  & 0xFF;
    	tx_buf[3] = data & 0xFF;
        tx_len = 4;
    } else {
        tx_buf[2] = data & 0xFF;
        tx_len = 3;
    }

	memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
	xfer[0].len = tx_len;
	xfer[0].tx_buf = (unsigned long)tx_buf;

    for (int i = 0; i < spi_retry_max; i++) {
    	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer); 
    	if (ret < 1) {
    		if (spi_debug) printf("can't transfer, ret %d \n", ret);
        } else {
            return 0;
        }
    }
	
	return -1; //failed
}

#endif

#ifdef INCLUDE_AXI_SPI

#define TRX_LEN 11

int axi_spi_fd = -1;

int open_spi(char *dev_name)
{
	int fd;
	__u32	mode, speed;

	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

#if 1
	mode = 0; //SPI_CPHA | SPI_CPOL;
	if (ioctl(fd, SPI_IOC_WR_MODE32, &mode) < 0) {
		perror("SPI rd_mode");
		//return;
	}

	speed = 1000000;
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
		perror("SPI max_speed_hz");
		//return;
	}
#endif

	return fd;
}

int axi_spi_read(uint32_t address)
{
    int ret;
    __u8 tx_buf[TRX_LEN];
    __u8 rx_buf[TRX_LEN];
    struct spi_ioc_transfer xfer[2];
    int value = 0xFFFFFFFF;

    if (axi_spi_fd < 0) {
        axi_spi_fd = open_spi("/dev/spidev1.0");
        if (axi_spi_fd < 0) return value;
    }
    
    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0x0, sizeof(rx_buf));
    tx_buf[0] = 0x1; //read
    tx_buf[1] = (address >> 24) & 0xFF;
    tx_buf[2] = (address >> 16) & 0xFF;
    tx_buf[3] = (address >> 8) & 0xFF;
    tx_buf[4] = (address) & 0xFF;
    
    memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
    xfer[0].tx_buf  = (unsigned long)tx_buf;
    xfer[0].rx_buf  = (unsigned long)rx_buf;
    xfer[0].len     = TRX_LEN;
    ret = ioctl(axi_spi_fd, SPI_IOC_MESSAGE(1), &xfer);
    if (ret >= 0) {
        value = rx_buf[6];
        value = (value << 8) | rx_buf[7];
        value = (value << 8) | rx_buf[8];
        value = (value << 8) | rx_buf[9];
    }

    return value;
}

int axi_spi_write(uint32_t address, uint32_t data)
{ 
    int ret;
    __u8 tx_buf[TRX_LEN];
    __u8 rx_buf[TRX_LEN];
    struct spi_ioc_transfer xfer[2];

    if (axi_spi_fd < 0) {
        axi_spi_fd = open_spi("/dev/spidev1.0");
        if (axi_spi_fd < 0) return -1;
    }

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0x0, sizeof(rx_buf));
    tx_buf[1] = (address >> 24) & 0xFF;
    tx_buf[2] = (address >> 16) & 0xFF;
    tx_buf[3] = (address >> 8) & 0xFF;
    tx_buf[4] = (address) & 0xFF;
    tx_buf[5] = (data >> 24) & 0xFF;
    tx_buf[6] = (data >> 16) & 0xFF;
    tx_buf[7] = (data >> 8)  & 0xFF;
    tx_buf[8] = data & 0xFF;

    memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
    xfer[0].len = TRX_LEN;
    xfer[0].tx_buf = (unsigned long)tx_buf;
    xfer[0].rx_buf = (unsigned long)rx_buf;

    ret = ioctl(axi_spi_fd, SPI_IOC_MESSAGE(1), &xfer); 
    if (ret < 1) {
        return -1;
    }

    return 0; 
}


#endif

#if 1

int rw_reverse = 1;

static uint8_t bits = 8;
static uint32_t speed = 10000000; //read
static uint16_t delay = 0;

//one transfer
void transfer(int fd, uint8_t * tx, uint8_t * rx, uint8_t len)
{
    int ret;
    
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

	//gpio_set_value(66, 0);
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	//gpio_set_value(66, 1);

    if (ret < 1) {
        pabort("can't send spi message");
    }
}


__u8 SPI_ReadByte_LSB(int fd, __u16 addr)
{
	int ret;
	__u16 cmd;
	__u8 tx_buf[4];
	__u8 rx_buf[4];
	struct spi_ioc_transfer xfer[2];
	
	if (rw_reverse) cmd = addr;
    else cmd = 0X8000| addr;
	tx_buf[0] = cmd >> 8;
	tx_buf[1] = cmd & 0xFF;
	
	memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
	memset(rx_buf, 0x0, sizeof(rx_buf));
	
	xfer[0].tx_buf	= (unsigned long)tx_buf;
	xfer[0].rx_buf	= (unsigned long)NULL;
	xfer[0].len 	= 2;

	xfer[1].tx_buf	= (unsigned long)NULL;
	xfer[1].rx_buf	= (unsigned long)rx_buf;
	xfer[1].len 	= 1;

	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &xfer);
	if (ret < 1 )
		printf("can't transfer, %d \n", ret);

	printf("rx_buf:%x %x %x %x\r\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);

	return rx_buf[0];
}

int SPI_WriteByte_LSB(int fd, __u16 writeAddress, __u8 writedata)
{ 
	int ret;
	__u16 cmd;
	__u8 tx_buf[4];
	struct spi_ioc_transfer xfer[2];

	if (rw_reverse) cmd = 0X8000 | writeAddress;
    else cmd = 0X0000 | writeAddress;
	tx_buf[0] = cmd >> 8;
	tx_buf[1] = cmd & 0xFF;
	tx_buf[2] = writedata;

	memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
	xfer[0].len = 3;
	xfer[0].tx_buf = (unsigned long)tx_buf;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer); 
	if (ret < 1)
		printf("can't xfer spi message1");
	
	return 0; 
}

static void do_read(int fd, int len)
{
	unsigned char	buf[32], *bp;
	int		status;

	/* read at least 2 bytes, no more than 32 */
	if (len < 2)
		len = 2;
	else if (len > sizeof(buf))
		len = sizeof(buf);
	memset(buf, 0, sizeof buf);

	status = read(fd, buf, len);
	if (status < 0) {
		perror("read");
		return;
	}
	if (status != len) {
		fprintf(stderr, "short read\n");
		return;
	}

	printf("read(%2d, %2d): %02x %02x,", len, status,
		buf[0], buf[1]);
	status -= 2;
	bp = buf + 2;
	while (status-- > 0)
		printf(" %02x", *bp++);
	printf("\n");
}

static void do_msg(int fd, int len)
{
	struct spi_ioc_transfer	xfer[2];
	unsigned char buf[32], *bp;
	int	status;

	memset(xfer, 0, sizeof xfer);
	memset(buf, 0, sizeof buf);

	if (len > sizeof buf)
		len = sizeof buf;

	buf[0] = 0x0;
	buf[1] = 0x0;
	buf[2] = 0x3c;
	xfer[0].tx_buf = (unsigned long)buf;
	xfer[0].len = 3;

	xfer[1].rx_buf = (unsigned long) buf;
	xfer[1].len = len;

	status = ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
	if (status < 0) {
		perror("SPI_IOC_MESSAGE");
		return;
	}

	printf("response(%2d, %2d): ", len, status);
	for (bp = buf; len; len--)
		printf(" %02x", *bp++);
	printf("\n");
}

static void dumpstat(const char *name, int fd)
{
	__u8	lsb, bits;
	__u32	mode, speed;

	if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
		perror("SPI rd_mode");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
		perror("SPI rd_lsb_fist");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
		perror("SPI bits_per_word");
		return;
	}
	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
		perror("SPI max_speed_hz");
		return;
	}

	printf("%s: spi mode 0x%x, %d bits %sper word, %d Hz max\n",
		name, mode, bits, lsb ? "(LSB) " : "(MSB) ", speed);
}

int open_spi(char *dev_name)
{
	int fd;
	//__u32	mode, speed;

	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

#if 0
	mode = SPI_CPHA | SPI_CPOL;
	if (ioctl(fd, SPI_IOC_WR_MODE32, &mode) < 0) {
		perror("SPI rd_mode");
		//return;
	}

	speed = 10000;
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
		perror("SPI max_speed_hz");
		//return;
	}
#endif

	return fd;
}


#endif

#ifndef MAKE_XLIB


int main(int argc, char **argv)
{
	int		c;
	int		readcount = 0;
	int		msglen = 0;
	int		fd;
	int 	reg_addr, reg_val;
	int		rw = 0;
	char	*name;

	printf("spidev start\n");
	while ((c = getopt(argc, argv, "hm:r:g:d:p:v")) != EOF) {
		switch (c) {
		case 'm':
			msglen = atoi(optarg);
			if (msglen < 0)
				goto usage;
			continue;
		case 'r':
			readcount = atoi(optarg);
			if (readcount < 0)
				goto usage;
			continue;
		case 'g':
			rw = 1;
			reg_addr = strtoul(optarg, NULL, 0);
			if (reg_addr < 0)
				goto usage;
			continue;
		case 'p':
			rw = 2;
			reg_addr = strtoul(optarg, NULL, 0);
			if (reg_addr < 0)
				goto usage;
			continue;
		case 'd':
			reg_val = strtoul(optarg, NULL, 0);
			if (reg_val < 0)
				goto usage;
			continue;
		case 'v':
			rw_reverse = 0; //ad9544
			continue;
		case 'h':
		case '?':
		usage:
			fprintf(stderr, "usage: %s [-h] [-m N] [-r N] /dev/spidevB.D\n", argv[0]);
			fprintf(stderr, "       %s <-g|-p addr> [-d data] /dev/spidevB.D\n", argv[0]);
			return 1;
		}
	}

	if ((optind + 1) != argc)
		goto usage;
	name = argv[optind];

	fd = open_spi(name);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	dumpstat(name, fd);

	if (msglen)
		do_msg(fd, msglen);

	if (rw == 1) {
		__u8 data = SPI_ReadByte_LSB(fd, reg_addr);
		printf("reg[0x%x] = 0x%x \n", reg_addr, data);
	} else if (rw == 2) {
		printf("set reg[0x%x] to 0x%x \n", reg_addr, reg_val);
		SPI_WriteByte_LSB(fd, reg_addr, (__u8)reg_val);
	} else if (readcount) {
		do_read(fd, readcount);
	}

	close(fd);
	return 0;
}

#endif

