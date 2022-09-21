//任务二，由父进程创建一个管道，然后再创建 3 个子进程，并由这三个子进程利用管道与父进程之间进行通信 

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/wait.h>
#define MAXSIZE 70000


//创建管道两个端口和几个信号量 
int pipe_fd[2],cnt=0,pipe_content=65536,pipe_text = 0;
char SEM_NAME[]= "pipesem";
sem_t *mutex,*receive_signal;
/*
mutex:互斥访问管道
receive_signal： 控制父进程开始读进程信号量，
每个子进程写完管道后唤醒信号量，当值等于子进程个数时
表示子进程处理完毕，父进程开始读管道 
*/

//管道和信号量的初始化 
void init(){
    mutex = sem_open(SEM_NAME,O_CREAT,0666,1);
    receive_signal = sem_open("receive_signal", O_CREAT, 0666, 0);
    if(mutex == SEM_FAILED || receive_signal == SEM_FAILED)
    {
        perror("unable to create semaphore");
        sem_unlink(SEM_NAME);
        exit(-1);
    }

    //pipe创建管道 
    if (pipe(pipe_fd) < 0)
    {
        printf("pipe create error\n");
        return -1;
    }

}

//往管道写数据，将buf写入 
void PipeWrite(int fd[2],char *buf,int len)
{
	int nWirted;

	printf("Write Process :   Wanna input %d characters \n",len);

	close(fd[0]);//关闭管道读端 
	nWirted = write(fd[1],buf,len);//管道写函数，返回写入的字节数 
    pipe_text+=nWirted;
	printf("Write Process : Wrote in %d characters ...\n",nWirted);
}

//从管道读数据到buf中 
void PipeRead(int fd[2],char* buf,int len)
{
    int nRead = 0;
	printf("Read Process : Wanna read %d characters.\n",len );
	close(fd[1]);//关闭管道写端 
	nRead = read(fd[0],buf,len);
    pipe_text -= nRead;
	printf("Read Process : Read %d characters\n", nRead);
}

void InitStr(char * text,int len)
{
	int i;

	for(i = 0; i<len; i++){
		text[i] = '*';
	}
}

int main(){
    int i,pid;
    init();
    int process_n = 1;
    printf("Number of Progress: ");
    scanf("%d",&process_n);//输入子进程个数 
    for(i=0;i<process_n;i++){
        pid = fork();//创建子进程 
        cnt++;
        if(pid == 0) break;
    }

    if(pid == 0){//如果是子进程则写管道 
        sem_wait(mutex);
        char buf[MAXSIZE] = {0};
        // sprintf(buf,"hello %d",cnt);//创建写入字符串为'hello 子进程编号' 
        InitStr(buf,50000);
        PipeWrite(pipe_fd,buf,strlen(buf));
        sem_post(mutex);
        sem_post(receive_signal);
    }
    else{
        int t = -1;
        sem_getvalue(receive_signal, &t);
        while(t < process_n){
            while (1) {
                sem_getvalue(receive_signal, &t);
                printf("receive_signal: %d\n", t);
                printf("pipe_content: %d\n",pipe_text);
                if (t == process_n || pipe_text+5 >= pipe_content) break;
            }
            char buf[MAXSIZE] = {0};
            sem_wait(mutex);
            PipeRead(pipe_fd, buf, MAXSIZE);
            sem_post(mutex);
            // printf("father read: %s\n", buf);
        }
    }

    sem_close(mutex);
    sem_close(receive_signal);
    sem_unlink("pipesem");
    sem_unlink("receive_signal");
    return 0;
}