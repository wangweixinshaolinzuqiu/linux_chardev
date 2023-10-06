/*************************************************************************
  > File Name: char_dev.c
  > Author: WangDengtao
  > Mail: 1799055460@qq.com 
  > Created Time: 2023年03月16日 星期四 16时40分29秒
 ************************************************************************/

#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/fs.h>

/*如果没有定义DEV_MAJOR就设置设备号为0,采用动态申请，如果有则使用宏定义的设备号*/
//#define DEV_MAJOR 88
#ifndef DEV_MAJOR
#define DEV_MAJOR 0
#endif

#define DEV_NAME  "chardev"       /*宏定义设备的名字*/
#define MIN(a,b)  (a < b ? a : b)

int dev_major = DEV_MAJOR;        /*主设备号*/
static struct cdev *chrtest_cdev; /*创建cdev结构体*/
static char kernel_buf[1024];     
static struct class *chrdev_class; /*定义一个class用于自动创建类*/

/*实现对应的open/read/write等函数，填入file_operations结构体 */
static ssize_t char_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/*将内核空间的数据复制到用户空间*/
	err = copy_to_user(buf, kernel_buf, MIN(1024, size));
	return MIN(1024, size);
}

static ssize_t char_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/*将buf中的数据复制到写缓冲区kernel_buf中，因为用户空间内存不能直接访问内核空间的内存*/
	err = copy_from_user(kernel_buf, buf, MIN(1024, size)); 
	return MIN(1024, size);
}

static int char_open (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int char_close (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

/*定义自己的file_operations结构体*/
static struct file_operations chrtest_fops = {
	.owner = THIS_MODULE,
	.open  = char_open,
	.read  = char_read,
	.write = char_write,
	.release = char_close,
};

/*注册驱动函数：写入口函数，安装驱动程序时就会调用这个入口函数 */
static int __init chardev_init(void)
{
	int result;
	/*
	   dev_t 定义在文件 include/linux/types.h
	   typedef __u32 __kernel_dev_t;
	   ......
	   typedef __kernel_dev_t dev_t;
	   可以看出 dev_t 是__u32 类型的，而__u32 定义在文件 include/uapi/asm-generic/int-ll64.h里面，定义如下：
	   typedef unsigned int __u32;
	   综上所述，dev_t 其实就是 unsigned int 类型，是一个 32 位的数据类型。
	   主设备号和次设备号两部分，其中高 12 位为主设备号，低 20 位为次设备号。因此 Linux系统中主设备号范围为 0~4095。
	 */
	dev_t devno;/*定义一个dev_t的变量表示设备号*/

	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	/*字符设备驱动注册的流程二：分配主次设备号，这里不仅支持静态指定，也支持动态申请*/
	/*静态申请主次设备号*/
	if(0 != dev_major)
	{
               //MKDEV 将主设备号和次设备号转换成dev_t类型的库函数
		devno = MKDEV(dev_major, 0);//将主设备号dev_major和从设备号0分配给devno变量，
		       //register_chrdev_region 静态注册一个设备号，（起始设备号，设备号长度，设备号名称）
        result = register_chrdev_region(devno, 1, DEV_NAME);//请求分配一个设备号，名字为DEV_NAME(chardev)，设备号是：88 0
	}
	else
	{
               //alloc_chrdev_region：动态申请一个设备号 （设备号（输出参数），次设备号，设备号数量，设备号名称）
		result = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);//求分配一个名字为chardev的设备号，从设备号为0,保存到devno变量中
		dev_major = MAJOR(devno);//获取设备号 获取主设备号
	}
	/*失败后的处理结果，总规上面只执行一次，所以直接在外面判断就可*/
	if(result < 0)
	{
		printk(KERN_ERR " %s chardev can't use major %d\n", DEV_NAME, dev_major);
		return -ENODEV;
	}

	printk(KERN_DEBUG " %s driver use major %d\n", DEV_NAME, dev_major);

	/*字符串设备驱动流程三：分配cdev（内核）结构体，使用动态申请的方式*/
	/*
	   内核在内部使用类型struct cdev的结构体来代表字符设备。在内核调用你的设备操作之前，你必须分配
	   一个这样的结构体并注册给linux内核，在这个结构体里有对于这个设备进行操作的函数，具体定义在
	   file_operation结构体中。
       struct cdev {
	                struct kobject kobj;
	                struct module *owner;
	                const struct file_operations *ops;
	                struct list_head list;
	                dev_t dev;
	               unsigned int count;
                   };
	 */
	if(NULL == (chrtest_cdev = cdev_alloc()))//cdev_alloc 在内核空间申请一个cdev的结构体
	{
		printk(KERN_ERR "%s driver can't alloc for the cdev\n", DEV_NAME);
		unregister_chrdev_region(devno, 1);//释放掉设备号
		return -ENOMEM;
	}

	/*字符设备驱动流程四：分配cdev结构体，绑定主次设备号,fops到cdev结构体中，并且注册到linux内核*/
	chrtest_cdev -> owner = THIS_MODULE; /*.owner这表示谁拥有这个驱动程序*/
	cdev_init(chrtest_cdev, &chrtest_fops);/*初始化设备  file_operations chrtest_fops*/
	result = cdev_add(chrtest_cdev, devno, 1); /*将字符设备注册进内核*/
	if(0 != result)//添加内核失败
	{
		printk(KERN_INFO "%s driver can't register cdev:result = %d\n", DEV_NAME, result);
		goto ERROR;
	}
	printk(KERN_INFO "%s driver can register cdev:result = %d\n", DEV_NAME, result);

	/*自动创建设备类型、/dev设备节点*/

	chrdev_class = class_create(THIS_MODULE, DEV_NAME); /*创建设备类型sys/class/chrdev*/
	if (IS_ERR(chrdev_class)) {
		result = PTR_ERR(chrdev_class);
		goto ERROR;
	}
	device_create(chrdev_class, NULL, MKDEV(dev_major, 0), NULL, DEV_NAME); 
	/*/dev/chrdev 注册这个设备节点*/

	return 0;

ERROR:
	printk(KERN_ERR" %s driver installed failure.\n", DEV_NAME);
	cdev_del(chrtest_cdev);
	unregister_chrdev_region(devno, 1);
	return result;

}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数*/
static void __exit chardev_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 注销设备类型、/dev设备节点*/

	device_destroy(chrdev_class, MKDEV(dev_major, 0)); /*注销这个设备节点*/
	class_destroy(chrdev_class); /*删除这个设备类型*/

	cdev_del(chrtest_cdev); /*注销字符设备*/
	unregister_chrdev_region(MKDEV(dev_major,0), 1); /*释放设备号*/
	printk(KERN_ERR" %s driver version 1.0.0 removed!\n", DEV_NAME);
	return;
}


/*调用函数 module_init 来声明 xxx_init 为驱动入口函数，当加载驱动的时候 xxx_init函数就会被调用.*/
module_init(chardev_init);
/*调用函数module_exit来声明xxx_exit为驱动出口函数，当卸载驱动的时候xxx_exit函数就会被调用.*/
module_exit(chardev_exit);

/*添加LICENSE和作者信息，是来告诉内核，该模块带有一个自由许可证；没有这样的说明，在加载模块的时内核会“抱怨”.*/
MODULE_LICENSE("Dual BSD/GPL");//许可 GPL、GPL v2、Dual MPL/GPL、Proprietary(专有)等，没有内核会提示.
MODULE_AUTHOR("WangDengtao");//作者
MODULE_VERSION("V1.0");//版本
