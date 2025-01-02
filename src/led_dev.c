#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define GPIO_LED_PIN 17  

static dev_t dev_num;
static struct cdev cdev;
static int gpio_value = 0; 

static int gpio_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "GPIO driver opened\n");
    return 0;
}

static int gpio_close(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "GPIO driver closed\n");
    return 0;
}

static ssize_t gpio_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > sizeof(gpio_value)) {
        printk(KERN_ERR "Input data too large\n");
        return -EINVAL;
    }

    if (copy_from_user(&gpio_value, buf, len)) {
        return -EFAULT;
    }

    if (gpio_value == 0) {
        gpio_set_value(GPIO_LED_PIN, 0); 
    } else if (gpio_value == 1) {
        gpio_set_value(GPIO_LED_PIN, 1); 
    }

    return len;
}

static const struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .open = gpio_open,
    .release = gpio_close,
    .write = gpio_write,
};

static int __init gpio_driver_init(void)
{
    int ret;

    if (!gpio_is_valid(GPIO_LED_PIN)) {
        printk(KERN_ERR "Invalid GPIO pin\n");
        return -ENODEV;
    }

    ret = gpio_request(GPIO_LED_PIN, "LED");
    if (ret) {
        printk(KERN_ERR "Unable to request GPIO pin %d\n", GPIO_LED_PIN);
        return ret;
    }

    gpio_direction_output(GPIO_LED_PIN, 0);

    ret = alloc_chrdev_region(&dev_num, 0, 1, "gpio_driver");
    if (ret) {
        printk(KERN_ERR "Failed to allocate device number\n");
        gpio_free(GPIO_LED_PIN);
        return ret;
    }

    cdev_init(&cdev, &gpio_fops);
    ret = cdev_add(&cdev, dev_num, 1);
    if (ret) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        gpio_free(GPIO_LED_PIN);
        return ret;
    }

    printk(KERN_INFO "GPIO driver initialized\n");
    return 0;
}

static void __exit gpio_driver_exit(void)
{
    cdev_del(&cdev);
    unregister_chrdev_region(dev_num, 1);
    gpio_free(GPIO_LED_PIN);
    printk(KERN_INFO "GPIO driver unloaded\n");
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple GPIO driver to control an LED");

