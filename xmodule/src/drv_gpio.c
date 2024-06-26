#include "vos.h"

#if 1

int32_t gpio_set_value(int gpio_num, uint8_t value)
{
	char buf[100];
	int fd;
	int ret;

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_num);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		//printf("%s: Can't open gpio %d \n", __func__, gpio_num);
		return VOS_ERR;
	}

	if (value)
		ret = write(fd, "1", 2);
	else
		ret = write(fd, "0", 2);
	if (ret < 0) {
		//printf("%s: Can't write to file\n ", __func__);
		return VOS_ERR;
	}

	ret = close(fd);
	if (ret < 0) {
		//printf("%s: Can't close device \n", __func__);
		return VOS_ERR;
	}

	return VOS_OK;
}

int32_t gpio_direction_output(int gpio_num, uint8_t value)
{
	char buf[128];
	int fd;
	int ret;

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_num);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		//printf("%s: Can't open device %d \n", __func__, desc->number);
		vos_run_cmd("echo %d > /sys/class/gpio/export", gpio_num);
        fd = open(buf, O_WRONLY);
		if (fd < 0) return VOS_ERR;
	}

	ret = write(fd, "out", 4);
	if (ret < 0) {
		//printf("%s: Can't write to file \n", __func__);
		return VOS_ERR;
	}

	close(fd);
	ret = gpio_set_value(gpio_num, value);
	if (ret != VOS_OK) {
		//printf("%s: Can't set value \n", __func__);
		return VOS_ERR;
	}

	return VOS_OK;
}

#endif

