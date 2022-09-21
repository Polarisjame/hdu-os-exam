SYSCALL_DEFINE3(myfile,char __user*,str1,char __user*,str2,int,flag){
	struct file* fp1,* fp2;
	char buf1[100]="",buf2[100]="",fname1[100]="",fname2[100]="";
	loff_t pos;
	pos = 0;
	printk("Enter Copy\n");
	copy_from_user(fname1,str1,100);
	copy_from_user(fname2,str2,100);
	printk("FILE_PATH_1:%s\n",fname1);
	printk("FILE_PATH_2:%s\n",fname2);
	fp2 = filp_open(fname2,O_RDWR | O_CREAT,0644);
	kernel_read(fp2,buf2,sizeof(buf2),&pos);
	filp_close(fp2,NULL);
	printk("Read the src File: %s\n",buf2);
	fp1 = filp_open(fname1,O_RDWR | O_CREAT,0644);
	pos=0;
	kernel_read(fp1,buf1,sizeof(buf1),&pos);
	if(strlen(buf1)>0){
		if(flag == 0){
			filp_close(fp1,NULL);
			return 1;
		}
	}
	pos = 0;
	kernel_write(fp1,buf2,strlen(buf2),&pos);
	filp_close(fp1,NULL);
	printk("Write the object File\n");
	return 0;
}