#include <linux/module.h>   // Included for all kernel modules
#include <linux/kernel.h>   // Included for KERN_INFO
#include <linux/init.h>     // Included for __init and __exit macros
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
#define ALS_INTERRUPTS          0b00000001
#define POWER_ON                0b00000100
#define TRIM                    0b00100000

#define SENSORS_OFF             0b00000000
#define SENSORS_G_IR            0b00000100
#define SENSORS_G               0b00001000
#define SENSORS_IR              0b00001100

#define ALS_ENABLE_INTERRUPTS   0b00000001

#define ALS_CONVERSION_TIME_100 0b00000000 // 14 bits resolution conversion for 100ms calculation time
#define ALS_CONVERSION_TIME_25  0b00000100 // 12 bits resolution conversion for 25ms calculation time
#define ALS_CONVERSION_TIME_6   0b00001000 // 10 bits resolution conversion for 6.25ms calculation time
#define ALS_CONVERSION_TIME_1   0b00001100 // 8 bits resolution conversion for 1.5625ms calculation time

#define ALS_GAIN_1              0b00000000
#define ALS_GAIN_4              0b00000001
#define ALS_GAIN_16             0b00000010
#define ALS_GAIN_128            0b00000011

#define OVERFLOW                0b01000000

#define ALS_PERSIST_TIMER_1     0b00000000
#define ALS_PERSIST_TIMER_2     0b00000001
#define ALS_PERSIST_TIMER_4     0b00000010
#define ALS_PERSIST_TIMER_16    0b00000011

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peekayy <peekayy.dev@gmail.com>");
MODULE_DESCRIPTION("Digital ambient light sensor MAX44004 driver");

static int max44004_get_lux_value(struct i2c_client *client){
	return 0;
}

static ssize_t lux_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct i2c_client *client = to_i2c_client(dev);
	return sprintf(buf, "%d\n", max44004_get_lux_value(client));
}

static DEVICE_ATTR(lux, S_IRUGO, lux_show, NULL);

static struct attribute *max44004_attributes[] = {
		/*&dev_attr_range.attr,
	&dev_attr_resolution.attr,
	&dev_attr_mode.attr,
	&dev_attr_power_state.attr,*/
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
