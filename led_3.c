#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/device.h> // 内核源码里创建/移除设备，产生热插拔事件的函数class
 
 
#define MYMA  200
#define MYMI  3333
#define COUNT    1
 
#define LED_IO   GPIOB(5)//注意修改! 对应开发板上的led灯引脚
 
dev_t devid; //用于存放设备号
struct cdev mycdev; 
static struct class *mycls;
 
 
 
//打开设备文件时开启中断
static int myopen(struct inode *ind,struct file *fl)
{
    int ret;
    ret = gpio_request(LED_IO, "led_0"); //请求gpio口,tldev则是为其取一个名字。
    if(ret<0)
        goto err0;
    gpio_direction_output(LED_IO, 0); //配置gpio口为输出，先输出低电平
    return 0;
 
err0:
    return ret;
}
 
static ssize_t mywrite(struct file *fl,const char *__user buf,size_t len,loff_t *off)
{
    char stat;
 
    copy_from_user(&stat,buf,sizeof(char));
    if(stat)
        gpio_set_value(LED_IO,1);
    else
        gpio_set_value(LED_IO,0);
 
    return 0;
}
 
//关闭设备文件时释放中断
static int myclose(struct inode *inode,struct file *fl)
{
    gpio_set_value(LED_IO,0);
    gpio_free(LED_IO);
 
    return 0;
}
 
static struct file_operations fops= {
    .owner = THIS_MODULE,
    .open  = myopen,
    .write = mywrite,
    .release= myclose,
};
 
 
 
static int __init test_init(void)
{
    int ret;
 
    devid = MKDEV(MYMA, MYMI); //生成一个设备号
    ret = register_chrdev_region(devid, COUNT, DEVNAME); //name设备名(用于查看用, 长度不能超过64字节   )
    if (ret < 0)
        goto err0;
 
    cdev_init(&mycdev, &fops);
    mycdev.owner = THIS_MODULE;
    ret = cdev_add(&mycdev, devid, COUNT);
    if (ret < 0)
        goto err1;  
  
    mycls= class_create(THIS_MODULE,"mykeycls");
    if(mycls== NULL)
        goto err2;
    device_create(mycls,NULL,devid,NULL,DEVNAME);
 
 
    return 0;
 
err2:
    cdev_del(&mycdev);
err1:
    unregister_chrdev_region(devid, COUNT);
err0:
    return ret;
}
 
static void __exit test_exit(void)
{
    device_destroy(mycls,devid);
    class_destroy(mycls);
    unregister_chrdev_region(devid, COUNT);
    cdev_del(&mycdev);
    gpio_free(LED_IO); //释放gpio
}
 
module_init(test_init);
module_exit(test_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("led");
MODULE_VERSION("v0.0");