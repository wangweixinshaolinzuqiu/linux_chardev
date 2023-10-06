#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#define DEV_MAJOR	200		/* 主设备号 */
#define DEV_NAME "myled"    /* 设备名  */
#define PB_CFG0_REG 0x01C20800 + 1 * 0x24 //PB配置寄存器 A:0 B:1 C:2 ....
#define PB_DATA_REG 0x01C20800 + 1 * 0x34 //PB数据寄存器 A:0 B:1 C:2 ....
#define PIN_N 5 //第5个引脚
#define N (PIN_N % 8 * 4)   //引脚x : x % 8 * 4
volatile unsigned int *gpio_con = NULL;
volatile unsigned int *gpio_dat = NULL;
/*打开设备*/
static int led_open (struct inode *node, struct file *filp)
{
   if (gpio_con) {
    /*打开成功*/
    }
    else {
   		return -1;
    }
    return 0;
}

/*写设备*/
static ssize_t led_write (struct file *filp, const char __user *buf, size_t size, loff_t *off)
{
    unsigned char val;        
    copy_from_user(&val, buf, 1);
 
    if (val)
    {
   		*gpio_dat |= (1 << PIN_N);//引脚5设置1
    }
    else
    {
    	*gpio_dat &= ~(1 << PIN_N);//引脚5设置0
    }
 
    return 1; 
}

/*关闭设备*/
static int led_release (struct inode *node, struct file *filp)
{
    return 0;
}

/*
 * 设备操作函数结构体
 */
static struct file_operations myled_oprs = {
    .owner = THIS_MODULE,
    .open  = led_open,
    .write = led_write,
    .release = led_release,
};

/* 设备初始化 */ 
static int myled_init(void)
{
    int ret;
    ret = register_chrdev(DEV_MAJOR, DEV_NAME, &myled_oprs);
	if (ret < 0) {
		printk(KERN_ERR "fail\n");
		return -1;
	}  
	printk(KERN_ERR "init \n");

    gpio_con = (volatile unsigned int *)ioremap(PB_CFG0_REG, 1);
    //选择1
    gpio_dat = (volatile unsigned int *)ioremap(PB_DATA_REG, 1);
    //选择2
    //gpio_dat = gpio_con + 4;  //数据寄存器 指针+4是移动了4*4=16个字节 原来是0x24 现在是0x34

    *gpio_con &= ~(7 << N);  //7=111 取反000 20：22设置000 默认是0x7=111 失能
    *gpio_con |=  (1 << N);  //设置输出 20：22设置001

    *gpio_dat &= ~(1 << PIN_N);  //第5个引脚初始化设置0
    return 0;
}


static void __exit myled_exit(void)
{
	/* 注销字符设备驱动 */ 
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
	printk("exit!\r\n");
}
//注册模块加载函数
module_init(myled_init);
//卸载模块加载函数
module_exit(myled_exit);
//开源信息
MODULE_LICENSE("GPL");
//作者
MODULE_AUTHOR("Lv129");