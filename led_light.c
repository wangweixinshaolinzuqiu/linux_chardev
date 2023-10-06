#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define GPIO_REG_BASE   0x01C20800      //GPIO物理基地址 (小页4kb)
#define MAP_SIZE        0x400 //MMU页大小
#define GPIO_BASE_OFFSET (GPIO_REG_BASE & 0X00000FFF) //GPIO基地址偏移计算
#define GPIO_PAGE_OFFSET (GPIO_REG_BASE & 0XFFFFF000) //获得页偏移
/**********************修改的************************/
#define rPB_CFG0 0X24  //PB_CFG0寄存器地址偏移
#define rPB_DAT 0X34    //PB_DAT寄存器地址偏移
/***************************************************/
int led_on(unsigned char *MAP_BASE);
int led_off(unsigned char *MAP_BASE);
int main(int argc, char **argv)
{
    static int dev_fd;
    unsigned char *map_base;

    if(argc!=2 || (strcmp(argv[1],"on") && strcmp(argv[1],"off"))){
        printf("argv_error!please input 'on' or 'off'!\n");
        exit (0);
    }
 //    /dev/mem:内核所有物理地址空间的全映像
    dev_fd = open("/dev/mem", O_RDWR );
    if (dev_fd < 0){
        printf("open(/dev/mem) failed.\n");
        return 0;
    }
    map_base = (unsigned char *)mmap(NULL, 0x400,PROT_READ | PROT_WRITE, MAP_SHARED,dev_fd, GPIO_PAGE_OFFSET); //把物理地址映射到虚拟地址
    if(!strcmp(argv[1],"on")) led_on(map_base); //点亮LED
    if(!strcmp(argv[1],"off")) led_off(map_base);//关闭LED
    if(dev_fd) close(dev_fd);
    munmap(map_base,MAP_SIZE);//解除映射关系
    return 0;
}


//led_on
int  led_on(unsigned char *MAP_BASE)
{
    unsigned int PB_CFG0,PB_DAT;
    PB_CFG0=*(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_CFG0);
    PB_DAT=*(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_DAT);
    *(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_CFG0)=((PB_CFG0 & 0XFF0FFFFF)|0X00100000);//PB5 第6个引脚
    *(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_DAT)=((PB_DAT & 0XFFFFFFDF)|0X00000020);
}
//led_off
int  led_off(unsigned char *MAP_BASE)
{
    unsigned int PB_CFG0,PB_DAT;
    PB_CFG0=*(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_CFG0);
    PB_DAT=*(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_DAT);
    *(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_CFG0)=((PB_CFG0 & 0XFF0FFFFF)|0X00100000);//PB5 第6个引脚
    *(volatile unsigned int *)(MAP_BASE+GPIO_BASE_OFFSET+rPB_DAT)=((PB_DAT & 0XFFFFFFDF));
}