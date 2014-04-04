#include <linux/module.h>   // Included for all kernel modules
#include <linux/init.h>     // Included for __init and __exit macros
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>

// Registers
#define MAX44004_INTERRUPT_STATUS            0x00
#define MAX44004_MAIN_CONFIG                 0x01
#define MAX44004_RECEIVER_CONFIG             0x02

#define MAX44004_DATA_HIGH_BYTE              0x04
#define MAX44004_DATA_LOW_BYTE               0x05

#define MAX44004_UPPER_THRESHOLD_HIGH_BYTE   0x06
#define MAX44004_UPPER_THRESHOLD_LOW_BYTE    0x07
#define MAX44004_LOWER_THRESHOLD_HIGH_BYTE   0x08
#define MAX44004_LOWER_THRESHOLD_LOW_BYTE    0x09
#define MAX44004_THRESHOLD_PERSIST_TIMER     0x0A
#define MAX44004_DIGITAL_GAIN_TRIM_HIGH_BYTE 0x0F
#define MAX44004_DIGITAL_GAIN_TRIM_LOW_BYTE  0x10

// Flags
#define MAX44004_ALS_INTERRUPTS           0b00000001
#define MAX44004_POWER_ON                 0b00000100
#define MAX44004_TRIM                     0b00100000

#define MAX44004_SENSORS_OFF              0b00000000
#define MAX44004_SENSORS_G_IR             0b00000100
#define MAX44004_SENSORS_G                0b00001000
#define MAX44004_SENSORS_IR               0b00001100

#define MAX44004_ALS_ENABLE_INTERRUPTS    0b00000001

#define MAX44004_ALS_CONVERSION_TIME_100  0b00000000 // 14 bits resolution conversion for 100ms calculation time
#define MAX44004_ALS_CONVERSION_TIME_25   0b00000100 // 12 bits resolution conversion for 25ms calculation time
#define MAX44004_ALS_CONVERSION_TIME_6    0b00001000 // 10 bits resolution conversion for 6.25ms calculation time
#define MAX44004_ALS_CONVERSION_TIME_1    0b00001100 // 8 bits resolution conversion for 1.5625ms calculation time

#define MAX44004_ALS_GAIN_1               0b00000000
#define MAX44004_ALS_GAIN_4               0b00000001
#define MAX44004_ALS_GAIN_16              0b00000010
#define MAX44004_ALS_GAIN_128             0b00000011

#define MAX44004_OVERFLOW                 0b01000000

#define MAX44004_ALS_PERSIST_TIMER_1      0b00000000
#define MAX44004_ALS_PERSIST_TIMER_2      0b00000001
#define MAX44004_ALS_PERSIST_TIMER_4      0b00000010
#define MAX44004_ALS_PERSIST_TIMER_16     0b00000011

#define MAX44004_OVERFLOW_BIT_MASK        0b00111111
#define MAX44004_ALS_MODE_MASK            0b00001100
#define MAX44004_ALS_CONVERSION_TIME_MASK 0b00001100
#define MAX44004_ALS_GAIN_MASK            0b00000011

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peekayy <peekayy.dev@gmail.com>");
MODULE_DESCRIPTION("Digital ambient light sensor MAX44004 driver");

struct max44004_data {
	struct i2c_client *client;
	struct mutex lock;
};

static int max44004_get_lux_value(struct i2c_client *client){
	struct max44004_data *data = i2c_get_clientdata(client);
	int lsb, msb, range, bitdepth;

	mutex_lock(&data->lock);
	lsb = i2c_smbus_read_byte_data(client, MAX44004_DATA_LOW_BYTE);

	if (lsb < 0) {
		mutex_unlock(&data->lock);
		return lsb;
	}

	msb = i2c_smbus_read_byte_data(client, MAX44004_DATA_HIGH_BYTE);
	mutex_unlock(&data->lock);

	if (msb < 0)
		return msb;

	//range = isl29003_get_range(client);
	//bitdepth = (4 - isl29003_get_resolution(client)) * 4;
	msb = msb & MAX44004_OVERFLOW_BIT_MASK;
	return (msb << 8) | lsb;
}

static ssize_t lux_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct i2c_client *client = to_i2c_client(dev);
	return sprintf(buf, "%d\n", max44004_get_lux_value(client));
}

static int max44004_read_config_register(struct i2c_client *client, int register_addr){
	struct max44004_data *data = i2c_get_clientdata(client);
	int status;
	mutex_lock(&data->lock);
	status = i2c_smbus_read_byte_data(client, register_addr);
	mutex_unlock(&data->lock);

	return status;
}

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct i2c_client *client = to_i2c_client(dev);
	int status = max44004_read_config_register(client, MAX44004_INTERRUPT_STATUS);
	return sprintf(buf, "Status register : 0x%X\nFlags :\n\tPWRON : %s\n\tALSINTS : %s", status, (status & 0x4)?"ON":"OFF", (status & 0x1)?"ON":"OFF");
}

static ssize_t config_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct i2c_client *client = to_i2c_client(dev);
	int main = max44004_read_config_register(client, MAX44004_MAIN_CONFIG);
	int receiver = max44004_read_config_register(client, MAX44004_RECEIVER_CONFIG);
	char *mode, *alstim, *alspga;

	switch(main & MAX44004_ALS_MODE_MASK){
	case MAX44004_SENSORS_OFF:
		mode = "SENSORS OFF";
		break;
	case MAX44004_SENSORS_G_IR:
		mode = "SENSORS GREEN + IR";
		break;
	case MAX44004_SENSORS_G:
		mode = "SENSOR GREEN ONLY";
		break;
	case MAX44004_SENSORS_IR:
		mode = "SENSOR IR ONLY";
		break;
	}

	switch(receiver & MAX44004_ALS_CONVERSION_TIME_MASK){
	case MAX44004_ALS_CONVERSION_TIME_100:
		alstim = "100ms";
		break;
	case MAX44004_ALS_CONVERSION_TIME_25:
		alstim = "25ms";
		break;
	case MAX44004_ALS_CONVERSION_TIME_6:
		alstim = "6.25ms";
		break;
	case MAX44004_ALS_CONVERSION_TIME_1:
		alstim = "1.5625ms";
		break;
	}

	switch(receiver & MAX44004_ALS_GAIN_MASK){
	case MAX44004_ALS_GAIN_128:
		alspga = "128x";
		break;
	case MAX44004_ALS_GAIN_16:
		alspga = "16x";
		break;
	case MAX44004_ALS_GAIN_4:
		alspga = "4x";
		break;
	case MAX44004_ALS_GAIN_1:
		alspga = "1x";
		break;
	}


	return sprintf(buf, "Main config register : 0x%X\nFlags :\n\tTRIM : %s\n\tMODE : %s\n\tALSINTE : %s\nReceiver config register: 0x%X\nFlags:\n\tALSTIM : %s\n\tALSPGA : %s\n", main, (main & 0x10)?"ON":"OFF", mode, (main & 0x1)?"ON":"OFF",receiver,alstim,alspga);
}


static DEVICE_ATTR(lux, S_IRUGO, lux_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, status_show, NULL);
static DEVICE_ATTR(config, S_IRUGO, config_show, NULL);

static struct attribute *max44004_attributes[] = {
		&dev_attr_status.attr,
		&dev_attr_config.attr,
		&dev_attr_lux.attr,
		NULL
};

static const struct attribute_group max44004_attr_group = {
		.attrs = max44004_attributes,
};

static struct i2c_device_id max44004_id[] = {
		{"max44004", 0},
		{}
};

MODULE_DEVICE_TABLE(i2c, max44004_id);

static int max44004_probe(struct i2c_client *client, const struct i2c_device_id *id){
	int err = 0;
	struct max44004_data *data;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	data = kzalloc(sizeof(struct max44004_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	i2c_set_clientdata(client, data);


	/* register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &max44004_attr_group);
	if (err){
		printk(KERN_ERR "Error while creating sysfs hooks for max44004");
	}

	printk(KERN_INFO "Probed max44004 module.\n");
	return 0;
}

static int max44004_remove(struct i2c_client *client){
	sysfs_remove_group(&client->dev.kobj, &max44004_attr_group);
	kfree(i2c_get_clientdata(client));
	printk(KERN_INFO "Removed max44004 module.\n");
	return 0;
}

static struct i2c_driver max44004_driver = {
		.driver = {
				.name = "max44004",
				.owner	= THIS_MODULE,
		},
		.probe = max44004_probe,
		.remove = max44004_remove,
		.id_table = max44004_id,
};

module_i2c_driver(max44004_driver);
