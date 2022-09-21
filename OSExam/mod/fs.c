#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/string.h>


//实现拼接文件的内核模块，将内容拼接到目标文件之前
static char buf1[100];
static char* buftemp;
static char buf2[100];
static char* fname1;
static char* fname2;
module_param(fname1,charp,0644);
module_param(fname2,charp,0644);

static int __init modin(void){
		struct file* fp;
		loff_t pos;
		printk(KERN_ALERT"ENTER CONNECTING\n");
		fp = filp_open(fname1,O_RDWR | O_CREAT,0644);
		if(IS_ERR(fp)){
				printk(KERN_ALERT"File1 Open ERROR\n");
				return -1;
		}
		//fs = get_fs();
		//set_fs(KERNEL_DS);
		//vfs_read(fp,buf2,sizeof(buf2),&pos);
		pos=0;
		kernel_read(fp,buf2,sizeof(buf2),&pos);
		filp_close(fp,NULL);

		fp = filp_open(fname2,O_RDWR | O_CREAT,0644);
		if(IS_ERR(fp)){
				printk(KERN_ALERT"File1 Open ERROR\n");
				return -1;
		}
		pos=0;
		kernel_read(fp,buf1,sizeof(buf1),&pos);
		filp_close(fp,NULL);
		
		buftemp = strncat(buf1,buf2,strlen(buf2));
		printk(KERN_ALERT"CAT_STR: %s",buf1);


		fp = filp_open(fname1,O_RDWR | O_CREAT,0644);
		if(IS_ERR(fp)){
				printk(KERN_ALERT"File1 Open ERROR\n");
				return -1;
		}
		pos=0;
		kernel_write(fp,buf1,strlen(buf1),&pos);
		filp_close(fp,NULL);

		printk(KERN_ALERT"CONEECT&WRITE DONE\n");
		return 0;

}


static void modout(void){

		printk(KERN_ALERT"MOD OUT\n");
}

module_init(modin);
module_exit(modout);
MODULE_LICENSE("GPL");


