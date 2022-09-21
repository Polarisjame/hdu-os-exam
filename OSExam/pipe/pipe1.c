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
#define MAXSIZE 6000

//test max size

int pipe_fd[2],cnt=0;
char SEM_NAME[]= "pipesem";
char write_name[] = "writesem";
sem_t *mutex,*write_sig,*receive_signal;


void init(){
    mutex = sem_open(SEM_NAME,O_CREAT,0666,1);
    write_sig = sem_open(write_name,O_CREAT,0666,1); 
    receive_signal = sem_open("/receive_signal", O_CREAT, 0666, 0);
    if(mutex == SEM_FAILED)
    {
        perror("unable to create semaphore");
        sem_unlink(SEM_NAME);
        exit(-1);
    }
    if (pipe(pipe_fd) < 0)
    {
        printf("pipe create error\n");
        return -1;
    }

}

void InitStr(char * text,int len)
{
	int i;

	for(i = 0; i<len; i++){
		text[i] = '*';
	}
}

void PipeWrite(int fd[2],char *buf,int len)
{
	int nWirted;

	// printf("Write Process :   Wanna input %d characters \n",len);

	// close(fd[0]);
	nWirted = write(fd[1],buf,len);
	// printf("Write Process : Wrote in %d characters ...\n",nWirted);
}

void PipeRead(int fd[2],char* buf,int len)
{
    int nRead = 0;
	printf("Read Process : Wanna read %d characters.\n",len );
	close(fd[1]);
	nRead = read(fd[0],buf,len);
	printf("Read Process : Read %d characters\n", nRead);
}

int main(){
    int i,pid;
    init();
    int cnt = 0;
    char buf[10] = "";
    while(1){
        InitStr(buf,1);
        PipeWrite(pipe_fd,buf,strlen(buf));
        cnt++;
        printf("NUM Has Writen to rhe pipe is: %d\n",cnt);
    }

    sem_close(mutex);
    sem_close(write_sig);
    sem_close(receive_signal);
    sem_unlink("pipesem");
    sem_unlink("writesem");
    sem_unlink("/receive_signal");
    return 0;
}
