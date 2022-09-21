#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 65535
#define FREE 0
#define ROOTBLOCKNUM 2
#define MAXOPENFILE 10
#define MAX_TEXT_SIZE 10000
#define FASTSIZE 42
typedef struct FCB
{
    char filename[8];
    char exname[3];
    unsigned char attribute; //�ļ�orĿ¼
    unsigned short time;
    unsigned short date;
    unsigned short first;  //��λ���
    unsigned short length; //����
    char free;
} fcb;
typedef struct FAT
{
    unsigned short id;
} fat;
typedef struct FAST
{
    int path;
    fcb* fcbptr;
    int times;
    int free;
} fast;
typedef struct USEROPEN
{
    char filename[8];
    char exname[3];
    unsigned char attribute; //�ļ�orĿ¼
    unsigned short time;
    unsigned short date;
    unsigned short first;
    unsigned short length;
    char free;             //�Ƿ�ռ��
    int dirno;             // ��Ŀ¼�ļ���ʼ�̿��
    int diroff;            // ���ļ���Ӧ�� fcb �ڸ�Ŀ¼�е��߼����
    char dir[MAXOPENFILE]; // ȫ·����Ϣ
    int count;
    char fcbstate;  // �Ƿ��޸� 1�� 0��
    char topenfile; // 0: �� openfile
    char chmod[2];  //��дȨ��
} useropen;
typedef struct BLOCK
{
    char magic_number[8];
    char information[200];
    unsigned short root;
    unsigned char* startblock;
} block0;

unsigned char* myvhard;
useropen openfilelist[MAXOPENFILE];
int currfd;
unsigned char* startp;
unsigned char buffer[SIZE];
void startsys();
void my_format();
void my_cd(char* dirname);
int do_read(int fd, int len, char* text);
int do_write(int fd, char* text, int len, char wstyle);
int my_read(int fd);
int my_write(int fd);
void exitsys();
void my_cd(char* dirname);
int my_open(char* filename);
int my_close(int fd);
void my_mkdir(char* dirname);
void my_rmdir(char* dirname);
int my_create(char* filename);
void my_rm(char* filename);
void my_ls();
void help();
unsigned int SDBMHash(char* str);
int get_free_openfilelist();
unsigned short int get_free_block();
fcb* find_from_fast(int hash);
void update_fast(int hash, fcb* fcbptr);
void rm_fast(int hash);
char* FILENAME = "myfilesys";
fast* fastptr;

void startsys()
{
    myvhard = (unsigned char*)malloc(SIZE);
    FILE* file;
    if ((file = fopen(FILENAME, "r")) != NULL)
    {
        fread(buffer, SIZE, 1, file);
        fclose(file);
        if (memcmp(buffer, "10101010", 8) == 0)
        {
            memcpy(myvhard, buffer, SIZE);
            printf("myfsys file read successfully\n");
        }
        else
        {
            printf("invalid myfsys magic num, create file system\n");
            my_format();
            memcpy(buffer, myvhard, SIZE);
        }
    }
    else
    {
        printf("myfsys not find, create file system\n");
        my_format();
        memcpy(buffer, myvhard, SIZE);
    }
    fcb* root;
    fastptr = (fast*)(myvhard + 5 * BLOCKSIZE);
    root = (fcb*)(myvhard + 6 * BLOCKSIZE);
    strcpy(openfilelist[0].filename, root->filename);
    strcpy(openfilelist[0].exname, root->exname);
    openfilelist[0].attribute = root->attribute;
    openfilelist[0].time = root->time;
    openfilelist[0].date = root->date;
    openfilelist[0].first = root->first;
    openfilelist[0].length = root->length;
    openfilelist[0].free = root->free;
    openfilelist[0].dirno = 6;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "/root/");
    openfilelist[0].count = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;
    startp = ((block0*)myvhard)->startblock;
    currfd = 0;
    return;
}

void exitsys()
{
    while (currfd)
    {
        my_close(currfd);
    }
    FILE* fp = fopen(FILENAME, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
}

void my_format()
{
    block0* boot = (block0*)myvhard;
    strcpy(boot->magic_number, "10101010");
    strcpy(boot->information, "fat file system");
    boot->root = 6;
    boot->startblock = myvhard + BLOCKSIZE * 6;
    fat* fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat* fat2 = (fat*)(myvhard + BLOCKSIZE * 3);
    fast* fasttabel = (fast*)(myvhard + BLOCKSIZE * 5);
    int i;
    for (i = 0; i < 7; i++)
    {
        fat1[i].id = END;
        fat2[i].id = END;
    }
    for (i = 7; i < 1000; i++)
    {
        fat1[i].id = FREE;
        fat2[i].id = FREE;
    }
    for (i = 0; i < (int)(BLOCKSIZE / sizeof(fast)); i++)
    {
        fasttabel->free = 0;
        fasttabel++;
    }
    fcb* root = (fcb*)(myvhard + BLOCKSIZE * 6);
    strcpy(root->filename, ".");
    strcpy(root->exname, "di");
    root->attribute = 0;
    time_t rawTime = time(NULL);
    struct tm* time = localtime(&rawTime);
    root->time = time->tm_hour * 2048 + time->tm_min * 32 + time->tm_sec / 2;
    root->date = (time->tm_year - 100) * 512 + (time->tm_mon + 1) * 32 + (time->tm_mday);
    root->first = 6;
    root->free = 1;
    root->length = 2 * sizeof(fcb);
    fcb* root2 = root + 1;
    memcpy(root2, root, sizeof(fcb));
    strcpy(root2->filename, "..");
    for (i = 2; i < (int)(BLOCKSIZE / sizeof(fcb)); i++)
    {
        root2++;
        strcpy(root2->filename, "");
        root2->free = 0;
    }
    FILE* fp = fopen(FILENAME, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
}

void my_ls()
{
    if (openfilelist[currfd].attribute == 1)
    {
        printf("data file\n");
        return;
    }
    char buf[MAX_TEXT_SIZE];
    int i;
    openfilelist[currfd].count = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    printf("Type  Filename\t\t\tDate\t Time\tSize\n");
    fcb* fcbptr = (fcb*)buf;
    for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++)
    {
        if (fcbptr->free == 1)
        {
            if (fcbptr->attribute == 0)
            {
                printf("<DIR> %-24s\t%d/%d/%d %d:%d\n", fcbptr->filename,
                    (fcbptr->date >> 9) + 2000, (fcbptr->date >> 5) & 0x000f,
                    (fcbptr->date) & 0x001f, (fcbptr->time >> 11),
                    (fcbptr->time >> 5) & 0x003f);
            }
            else
            {
                printf("<FILE> %-24s\t%d/%d/%d %d:%d\t%d\n", fcbptr->filename,
                    (fcbptr->date >> 9) + 2000, (fcbptr->date >> 5) & 0x000f,
                    (fcbptr->date) & 0x001f, (fcbptr->time >> 11),
                    (fcbptr->time >> 5) & 0x003f, fcbptr->length);
            }
        }
    }
}

void my_mkdir(char* dirname)
{
    int i = 0;
    char text[MAX_TEXT_SIZE];
    char* fname = strtok(dirname, ".");
    char* exname = strtok(NULL, ".");
    if (exname != NULL)
    {
        printf("you can not use extension\n");
        return;
    }
    openfilelist[currfd].count = 0;
    int filelen = do_read(currfd, openfilelist[currfd].length, text);
    fcb* fcbptr = (fcb*)text;
    for (i = 0; i < (int)(filelen / sizeof(fcb)); i++)
    {
        if (strcmp(dirname, fcbptr[i].filename) == 0 && fcbptr->attribute == 0)
        {
            printf("dir has existed\n");
            return;
        }
    }
    int fd = get_free_openfilelist();
    if (fd == -1)
    {
        printf("openfilelist is full\n");
        return;
    }
    unsigned short int block_num = get_free_block();
    if (block_num == END)
    {
        printf("blocks are full\n");
        openfilelist[fd].topenfile = 0;
        return;
    }
    fat* fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat* fat2 = (fat*)(myvhard + BLOCKSIZE * 3);
    fat1[block_num].id = END;
    fat2[block_num].id = END;
    for (i = 0; i < (int)(filelen / sizeof(fcb)); i++)
    {
        if (fcbptr[i].free == 0)
        {
            break;
        }
    }
    openfilelist[currfd].count = i * sizeof(fcb);
    openfilelist[currfd].fcbstate = 1;
    fcb* fcbtmp = (fcb*)malloc(sizeof(fcb));
    fcbtmp->attribute = 0;
    time_t rawtime = time(NULL);
    struct tm* time = localtime(&rawtime);
    fcbtmp->date = (time->tm_year - 100) * 512 + (time->tm_mon + 1) * 32 + (time->tm_mday);
    fcbtmp->time = (time->tm_hour) * 2048 + (time->tm_min) * 32 + (time->tm_sec) / 2;
    strcpy(fcbtmp->filename, dirname);
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = block_num;
    fcbtmp->length = 2 * sizeof(fcb);
    fcbtmp->free = 1;
    do_write(currfd, (char*)fcbtmp, sizeof(fcb), 2);
    openfilelist[fd].attribute = 0;
    openfilelist[fd].count = 0;
    openfilelist[fd].date = fcbtmp->date;
    openfilelist[fd].time = fcbtmp->time;
    openfilelist[fd].dirno = openfilelist[currfd].first;
    openfilelist[fd].diroff = i;
    strcpy(openfilelist[fd].exname, "di");
    strcpy(openfilelist[fd].filename, dirname);
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].first = fcbtmp->first;
    openfilelist[fd].free = fcbtmp->free;
    openfilelist[fd].length = fcbtmp->length;
    openfilelist[fd].topenfile = 1;
    strcat(
        strcat(strcpy(openfilelist[fd].dir, (char*)(openfilelist[currfd].dir)),
            dirname),
        "/");
    fcbtmp->attribute = 0;
    fcbtmp->date = fcbtmp->date;
    fcbtmp->time = fcbtmp->time;
    strcpy(fcbtmp->filename, ".");
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = block_num;
    fcbtmp->length = 2 * sizeof(fcb);
    do_write(fd, (char*)fcbtmp, sizeof(fcb), 2);
    fcb* fcbtmp2 = (fcb*)malloc(sizeof(fcb));
    memcpy(fcbtmp2, fcbtmp, sizeof(fcb));
    strcpy(fcbtmp2->filename, "..");
    fcbtmp2->first = openfilelist[currfd].first;
    fcbtmp2->length = openfilelist[currfd].length;
    fcbtmp2->date = openfilelist[currfd].date;
    fcbtmp2->time = openfilelist[currfd].time;
    do_write(fd, (char*)fcbtmp2, sizeof(fcb), 2);
    my_close(fd);
    free(fcbtmp);
    free(fcbtmp2);
    fcbptr = (fcb*)text;
    fcbptr->length = openfilelist[currfd].length;
    openfilelist[currfd].count = 0;
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[currfd].fcbstate = 1;
}

void my_rmdir(char* dirname)
{
    int i, tag = 0;
    char buf[MAX_TEXT_SIZE];
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
    {
        printf("can not remove . and .. special dir\n");
        return;
    }
    fcb* fcbptr;
    char str[800];
    strcpy(str, openfilelist[currfd].dir);
    int hash = SDBMHash(strcat(str, dirname));
    tag = -1;
    if (tag != 1)
    {
        openfilelist[currfd].count = 0;
        do_read(currfd, openfilelist[currfd].length, buf);
        fcbptr = (fcb*)buf;
        for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++)
        {
            if (fcbptr->free == 0)
                continue;
            if (strcmp(fcbptr->filename, dirname) == 0 && fcbptr->attribute == 0)
            {
                tag = 1;
                break;
            }
        }
    }
    if (tag != 1)
    {
        printf("no such dir\n");
        return;
    }
    if (fcbptr->length > 2 * sizeof(fcb))
    {
        printf("can not remove a non empty dir\n");
        return;
    }
    int block_num = fcbptr->first;
    fat* fat1 = (fat*)(myvhard + BLOCKSIZE);
    int nxt_num = 0;
    while (1)
    {
        nxt_num = fat1[block_num].id;
        fat1[block_num].id = FREE;
        if (nxt_num != END)
        {
            block_num = nxt_num;
        }
        else
        {
            break;
        }
    }
    fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat* fat2 = (fat*)(myvhard + BLOCKSIZE * 3);
    memcpy(fat2, fat1, BLOCKSIZE * 2);
    fcbptr->date = 0;
    fcbptr->time = 0;
    fcbptr->exname[0] = '\0';
    fcbptr->filename[0] = '\0';
    fcbptr->first = 0;
    fcbptr->free = 0;
    fcbptr->length = 0;
    openfilelist[currfd].count = i * sizeof(fcb);
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    int lognum = i;
    if ((lognum + 1) * sizeof(fcb) == openfilelist[currfd].length)
    {
        openfilelist[currfd].length -= sizeof(fcb);
        lognum--;
        fcbptr = (fcb*)buf + lognum;
        while (fcbptr->free == 0)
        {
            fcbptr--;
            openfilelist[currfd].length -= sizeof(fcb);
        }
    }
    fcbptr = (fcb*)buf;
    fcbptr->length = openfilelist[currfd].length;
    openfilelist[currfd].count = 0;
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    rm_fast(hash);
    openfilelist[currfd].fcbstate = 1;
}

int my_create(char* filename)
{
    if (strcmp(filename, "") == 0)
    {
        printf("please input filename\n");
        return -1;
    }
    if (openfilelist[currfd].attribute == 1)
    {
        printf("you are in data file now\n");
        return -1;
    }
    openfilelist[currfd].count = 0;
    char buf[MAX_TEXT_SIZE];
    do_read(currfd, openfilelist[currfd].length, buf);
    int i;
    fcb* fcbptr = (fcb*)buf;
    for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++)
    {
        if (fcbptr->free == 0)
        {
            continue;
        }
        if (strcmp(fcbptr->filename, filename) == 0 && fcbptr->attribute == 1)
        {
            printf("the same filename error\n");
            return -1;
        }
    }
    fcbptr = (fcb*)buf;
    for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++)
    {
        if (fcbptr->free == 0)
            break;
    }
    int block_num = get_free_block();
    if (block_num == -1)
    {
        return -1;
    }
    fat* fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat* fat2 = (fat*)(myvhard + BLOCKSIZE * 3);
    fat1[block_num].id = END;
    memcpy(fat2, fat1, BLOCKSIZE * 2);
    strcpy(fcbptr->filename, filename);
    time_t rawtime = time(NULL);
    struct tm* time = localtime(&rawtime);
    fcbptr->date = (time->tm_year - 100) * 512 + (time->tm_mon + 1) * 32 + (time->tm_mday);
    fcbptr->time = (time->tm_hour) * 2048 + (time->tm_min) * 32 + (time->tm_sec) / 2;
    fcbptr->first = block_num;
    fcbptr->free = 1;
    fcbptr->attribute = 1;
    fcbptr->length = 0;
    openfilelist[currfd].count = i * sizeof(fcb);
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    fcbptr = (fcb*)buf;
    fcbptr->length = openfilelist[currfd].length;
    openfilelist[currfd].count = 0;
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[currfd].fcbstate = 1;
    return 0;
}

void my_rm(char* filename)
{
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].count = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    int i, flag = 0;
    fcb* fcbptr = (fcb*)buf;
    for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++)
    {
        if (strcmp(fcbptr->filename, filename) == 0 && fcbptr->attribute == 1)
        {
            flag = 1;
            break;
        }
    }
    if (flag != 1)
    {
        printf("no such file\n");
        return;
    }
    int block_num = fcbptr->first;
    fat* fat1 = (fat*)(myvhard + BLOCKSIZE);
    int nxt_num = 0;
    while (1)
    {
        nxt_num = fat1[block_num].id;
        fat1[block_num].id = FREE;
        if (nxt_num != END)
            block_num = nxt_num;
        else
            break;
    }
    fat1 = (fat*)(myvhard + BLOCKSIZE);
    fat* fat2 = (fat*)(myvhard + BLOCKSIZE * 3);
    memcpy(fat2, fat1, BLOCKSIZE * 2);
    fcbptr->date = 0;
    fcbptr->time = 0;
    fcbptr->exname[0] = '\0';
    fcbptr->filename[0] = '\0';
    fcbptr->first = 0;
    fcbptr->free = 0;
    fcbptr->length = 0;
    openfilelist[currfd].count = i * sizeof(fcb);
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    int lognum = i;
    if ((lognum + 1) * sizeof(fcb) == openfilelist[currfd].length)
    {
        openfilelist[currfd].length -= sizeof(fcb);
        lognum--;
        fcbptr = (fcb*)buf + lognum;
        while (fcbptr->free == 0)
        {
            fcbptr--;
            openfilelist[currfd].length -= sizeof(fcb);
        }
    }
    fcbptr = (fcb*)buf;
    fcbptr->length = openfilelist[currfd].length;
    openfilelist[currfd].count = 0;
    do_write(currfd, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[currfd].fcbstate = 1;
}

int my_open(char* filename1)
{
    const char sep[2] = "-";
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].count = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    char* filename = strtok(filename1, sep);
    char* mode = strtok(NULL, sep);
    char* res = strtok(NULL, sep);
    int i, flag = 0;
    fcb* fcbptr = (fcb*)buf;
    for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++)
    {
        if (strcmp(fcbptr->filename, filename) == 0 && fcbptr->attribute == 1)
        {
            flag = 1;
            break;
        }
    }
    if (flag != 1)
    {
        printf("no such file\n");
        return -1;
    }
    int fd = get_free_openfilelist();
    if (fd == -1)
    {
        printf("my_open: full openfilelist\n");
        return -1;
    }
    if (mode != NULL && !strcmp(mode, "l"))
    {
        printf("%4s %8s %5s %6s \t%5s\n", "FD", "Filename", "Time", "Size", "Path");
        printf("%4d %8s %2d:%2d %6d \t%s%s \n", fd, fcbptr->filename, fcbptr->time >> 11, (fcbptr->time >> 5) & 0x003f, fcbptr->length, (char*)(openfilelist[currfd].dir), filename);
    }
    strcpy(openfilelist[fd].chmod, "11");
    if (mode != NULL && !strcmp(mode, "rd") || res != NULL && !strcmp(res, "rd"))
    {
        strcpy(openfilelist[fd].chmod, "10");
    }
    if (mode != NULL && !strcmp(mode, "wd") || res != NULL && !strcmp(res, "wd"))
    {
        strcpy(openfilelist[fd].chmod, "01");
    }
    openfilelist[fd].attribute = 1;
    openfilelist[fd].count = 0;
    openfilelist[fd].date = fcbptr->date;
    openfilelist[fd].time = fcbptr->time;
    openfilelist[fd].length = fcbptr->length;
    openfilelist[fd].first = fcbptr->first;
    openfilelist[fd].free = 1;
    strcpy(openfilelist[fd].filename, fcbptr->filename);
    strcat(strcpy(openfilelist[fd].dir, (char*)(openfilelist[currfd].dir)), filename);
    openfilelist[fd].dirno = openfilelist[currfd].first;
    openfilelist[fd].diroff = i;
    openfilelist[fd].topenfile = 1;
    openfilelist[fd].fcbstate = 0;
    currfd = fd;
    return 1;
}

void my_cd(char* dirname)
{
    int i = 0;
    int tag = -1;
    int fd;
    if (openfilelist[currfd].attribute == 1)
    {
        printf("you are in a data file, you could use 'close' to exit this file\n");
        return;
    }
    char str[800];
    strcpy(str, openfilelist[currfd].dir);
    int hash = SDBMHash(strcat(str, dirname)); //ƴ�ӳ�ȫ·����Ϣ����hashֵ
    fcb* fcbptr = find_from_fast(hash);        //�ӿ���и���hashֵ���Ҷ�Ӧfcbָ��
    if (fcbptr != NULL)
    {
        tag = 1;
    }
    if (tag != 1)
    {
        char* buf = (char*)malloc(10000);
        openfilelist[currfd].count = 0;                    //��дָ��������λ
        do_read(currfd, openfilelist[currfd].length, buf); //�ӿ�ʼ��ſ�ʼ��length���ȣ������ļ����������ݶ���buf��
        fcbptr = (fcb*)buf;

        for (i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++, fcbptr++) //����Ŀ¼��fcb�ҵ���Ӧ�ļ���fcb
        {
            if (strcmp(fcbptr->filename, dirname) == 0 && fcbptr->attribute == 0)
            {
                tag = 1;
                break;
            }
        }
    }
    if (tag != 1)
    {
        printf("my_cd: no such dir\n");
        return;
    }
    else
    {
        if (strcmp(fcbptr->filename, ".") == 0) //��ʾcd���Լ�
        {
            return;
        }
        else if (strcmp(fcbptr->filename, "..") == 0) //��ʾcd����Ŀ¼
        {
            if (currfd == 0)
            {
                return;
            }
            else
            {
                currfd = my_close(currfd);
                return;
            }
        }
        else
        {
            update_fast(hash, fcbptr);
            fd = get_free_openfilelist();
            if (fd == -1)
            {
                return;
            }
            openfilelist[fd].attribute = fcbptr->attribute;
            openfilelist[fd].count = 0;
            openfilelist[fd].date = fcbptr->date;
            openfilelist[fd].time = fcbptr->time;
            strcpy(openfilelist[fd].filename, fcbptr->filename);
            strcpy(openfilelist[fd].exname, fcbptr->exname);
            openfilelist[fd].first = fcbptr->first;
            openfilelist[fd].free = fcbptr->free;
            openfilelist[fd].fcbstate = 0;
            openfilelist[fd].length = fcbptr->length;
            strcat(strcat(strcpy(openfilelist[fd].dir, (char*)(openfilelist[currfd].dir)), dirname), "/");
            openfilelist[fd].topenfile = 1;
            openfilelist[fd].dirno = openfilelist[currfd].first;
            openfilelist[fd].diroff = i;
            currfd = fd;
        }
    }
}

int my_close(int fd)
{
    if (fd > MAXOPENFILE || fd < 0)
    {
        printf("my_close: fd error\n");
        return -1;
    }
    int i;
    char buf[MAX_TEXT_SIZE];
    int father_fd = -1;
    fcb* fcbptr;
    for (i = 0; i < MAXOPENFILE; i++)
    {
        if (openfilelist[i].first == openfilelist[fd].dirno && i != fd)
        {
            father_fd = i;
            break;
        }
    }
    if (father_fd == -1)
    {
        printf("my_close: no father dir\n");
        return -1;
    }
    if (openfilelist[fd].fcbstate == 1)
    {
        do_read(father_fd, openfilelist[father_fd].length, buf);
        fcbptr = (fcb*)(buf + sizeof(fcb) * openfilelist[fd].diroff);
        strcpy(fcbptr->exname, openfilelist[fd].exname);
        strcpy(fcbptr->filename, openfilelist[fd].filename);
        fcbptr->first = openfilelist[fd].first;
        fcbptr->free = openfilelist[fd].free;
        fcbptr->length = openfilelist[fd].length;
        fcbptr->time = openfilelist[fd].time;
        fcbptr->date = openfilelist[fd].date;
        fcbptr->attribute = openfilelist[fd].attribute;
        openfilelist[father_fd].count = openfilelist[fd].diroff * sizeof(fcb);
        do_write(father_fd, (char*)fcbptr, sizeof(fcb), 2);
    }
    memset(&openfilelist[fd], 0, sizeof(useropen));
    currfd = father_fd;
    return father_fd;
}

int my_read(int fd)
{
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("no such file\n");
        return -1;
    }
    if (openfilelist[fd].chmod[0] != (int)'1')
    {
        printf("Permission denied: Read forbidden\n");
        return -1;
    }
    openfilelist[fd].count = 0;
    char text[MAX_TEXT_SIZE] = "\0";
    do_read(fd, openfilelist[fd].length, text);
    printf("%s\n", text);
    return 1;
}
int my_write(int fd)
{
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("my_write: no such file\n");
        return -1;
    }
    int wstyle;
    if (openfilelist[fd].chmod[1] != (int)'1')
    {
        printf("Permission denied: Write forbidden\n");
        return -1;
    }
    while (1)
    {
        printf("1:Truncation  2:Coverage  3:Addition\n");
        scanf("%d", &wstyle);
        if (wstyle > 3 || wstyle < 1)
        {
            printf("input error\n");
        }
        else
        {
            break;
        }
    }
    char text[MAX_TEXT_SIZE] = "\0";
    char texttmp[MAX_TEXT_SIZE] = "\0";
    printf("please input data, line feed + $$ to end file\n");
    getchar();
    while (gets(texttmp))
    {
        if (strcmp(texttmp, "$$") == 0)
        {
            break;
        }
        texttmp[strlen(texttmp)] = '\n';
        strcat(text, texttmp);
    }
    text[strlen(text)] = '\0';
    do_write(fd, text, strlen(text) + 1, wstyle);
    openfilelist[fd].fcbstate = 1;
    return 1;
}

int do_read(int fd, int len, char* text)
{
    int len_tmp = len;
    char* textptr = text;
    unsigned char* buf = (unsigned char*)malloc(1024);
    if (buf == NULL)
    {
        printf("do_read reg mem error\n");
        return -1;
    }
    int off = openfilelist[fd].count;
    int block_num = openfilelist[fd].first;
    fat* fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
    while (off >= BLOCKSIZE)
    {
        off -= BLOCKSIZE;
        block_num = fatptr->id;
        if (block_num == END)
        {
            printf("do_read: block not exist\n");
            return -1;
        }
        fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
    }
    unsigned char* blockptr = myvhard + BLOCKSIZE * block_num;
    memcpy(buf, blockptr, BLOCKSIZE);
    while (len > 0)
    {
        if (BLOCKSIZE - off > len)
        {
            memcpy(textptr, buf + off, len);
            textptr += len;
            off += len;
            openfilelist[fd].count += len;
            len = 0;
        }
        else
        {
            memcpy(textptr, buf + off, BLOCKSIZE - off);
            textptr += BLOCKSIZE - off;
            len -= BLOCKSIZE - off;
            block_num = fatptr->id;
            if (block_num == END)
            {
                printf("do_read: len is lager then file\n");
                break;
            }
            fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
            blockptr = myvhard + BLOCKSIZE * block_num;
            memcpy(buf, blockptr, BLOCKSIZE);
        }
    }
    free(buf);
    return len_tmp - len;
}

int do_write(int fd, char* text, int len, char wstyle)
{
    int block_num = openfilelist[fd].first;
    int i, tmp_num;
    int lentmp = 0;
    char* textptr = text;
    char buf[BLOCKSIZE];
    fat* fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
    unsigned char* blockptr;
    if (wstyle == 1)
    {
        openfilelist[fd].count = 0;
        openfilelist[fd].length = 0;
    }
    else if (wstyle == 3)
    {
        openfilelist[fd].count = openfilelist[fd].length;
        if (openfilelist[fd].attribute == 1)
        {
            if (openfilelist[fd].length != 0)
            {
                openfilelist[fd].count = openfilelist[fd].length - 1;
            }
        }
    }
    int off = openfilelist[fd].count;
    while (off >= BLOCKSIZE)
    {
        block_num = fatptr->id;
        if (block_num == END)
        {
            printf("do_write: off error\n");
            return -1;
        }
        fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
        off -= BLOCKSIZE;
    }
    blockptr = (unsigned char*)(myvhard + BLOCKSIZE * block_num);
    while (len > lentmp)
    {
        memcpy(buf, blockptr, BLOCKSIZE);
        for (; off < BLOCKSIZE; off++)
        {
            *(buf + off) = *textptr;
            textptr++;
            lentmp++;
            if (len == lentmp)
                break;
        }
        memcpy(blockptr, buf, BLOCKSIZE);
        if (off == BLOCKSIZE && len != lentmp)
        {
            off = 0;
            block_num = fatptr->id;
            if (block_num == END)
            {
                block_num = get_free_block();
                if (block_num == END)
                {
                    printf("do_write: block full\n");
                    return -1;
                }
                blockptr = (unsigned char*)(myvhard + BLOCKSIZE * block_num);
                fatptr->id = block_num;
                fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
                fatptr->id = END;
            }
            else
            {
                blockptr = (unsigned char*)(myvhard + BLOCKSIZE * block_num);
                fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
            }
        }
    }
    openfilelist[fd].count += len;
    if (openfilelist[fd].count > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].count;
    if (wstyle == 1 || (wstyle == 2 && openfilelist[fd].attribute == 0))
    {
        off = openfilelist[fd].length;
        fatptr = (fat*)(myvhard + BLOCKSIZE) + openfilelist[fd].first;
        while (off >= BLOCKSIZE)
        {
            block_num = fatptr->id;
            off -= BLOCKSIZE;
            fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
        }
        while (1)
        {
            if (fatptr->id != END)
            {
                i = fatptr->id;
                fatptr->id = FREE;
                fatptr = (fat*)(myvhard + BLOCKSIZE) + i;
            }
            else
            {
                fatptr->id = FREE;
                break;
            }
        }
        fatptr = (fat*)(myvhard + BLOCKSIZE) + block_num;
        fatptr->id = END;
    }
    memcpy((fat*)(myvhard + BLOCKSIZE * 3), (fat*)(myvhard + BLOCKSIZE), BLOCKSIZE * 2);
    return len;
}

int get_free_openfilelist()
{
    int i;
    for (i = 0; i < MAXOPENFILE; i++)
    {
        if (openfilelist[i].topenfile == 0)
        {
            openfilelist[i].topenfile = 1;
            return i;
        }
    }
    return -1;
}

unsigned short int get_free_block()
{
    int i;
    fat* fat1 = (fat*)(myvhard + BLOCKSIZE);
    for (i = 0; i < (int)(SIZE / BLOCKSIZE); i++)
    {
        if (fat1[i].id == FREE)
        {
            return i;
        }
    }
    return END;
}

unsigned int SDBMHash(char* str)
{
    unsigned int hash = 0;
    while (*str)
    {

        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    }
    return (hash & 0x7FFFFFFF);
}

fcb* find_from_fast(int hash)
{
    fcb* fcbptr = NULL;
    for (int i = 0; i < FASTSIZE; i++)
    {
        fast* ptr = fastptr + i;
        if (ptr->free == 0)
        {
            break;
        }
        if (ptr->path == hash)
        {
            fcbptr = ptr->fcbptr;
            ptr->times++;
            break;
        }
    }
    return fcbptr;
}

void update_fast(int hash, fcb* fcbptr)
{

    int min = 1000000;
    int mini = 0;
    int i;
    for (i = 0; i < FASTSIZE; i++)
    {
        fast* ptr = fastptr + i;
        if (ptr->free == 0)
        {
            mini = i;
            break;
        }
        ptr->times--;
        if (min > ptr->times)
        {
            min = ptr->times;
            mini = i;
        }
    }
    fast* ptr = fastptr + mini;
    ptr->fcbptr = fcbptr;
    ptr->path = hash;
    ptr->times = 1;
    ptr->free = 1;
}
void print_fast()
{
    printf("%-20s %-20s\n", "path", "fcbptr");

    for (int i = 0; i < FASTSIZE; i++)
    {
        fast* ptr = fastptr + i;
        if (ptr->free == 1)
        {
            printf("%-20ld %-20s\n", ptr->path, ((fcb*)ptr->fcbptr)->filename);
        }
    }
}
void rm_fast(int hash)
{
    for (int i = 0; i < FASTSIZE; i++)
    {
        fast* ptr = fastptr + i;
        if (ptr->path == hash)
        {
            ptr->fcbptr = NULL;
            ptr->free = 0;
            ptr->times = 0;
            ptr->path = 0;
        }
    }
}

void test()
{
    int openfile_num = 0;
    int i;
    printf("debug area ############\n");

    for (i = 0; i < MAXOPENFILE; i++)
    {
        if (openfilelist[i].topenfile == 1)
        {
            openfile_num++;
        }
    }
    printf("  open file number: %d\n", openfile_num);
    printf("  curr file name: %s\n", openfilelist[currfd].filename);
    printf("  curr file length: %d\n", openfilelist[currfd].length);
    printf("debug end  ############\n");
}

int main(void)
{
    char cmd[13][10] = { "mkdir", "rmdir", "ls", "cd", "create", "rm",
                        "open", "close", "write", "read", "exit", "format","fast" };
    char command[30], * sp;
    int cmd_idx, i;
    printf("************* file system ***************\n");
    startsys();
    while (1)
    {
        printf("%s $ ", openfilelist[currfd].dir);
        gets(command);
        cmd_idx = -1;
        if (strcmp(command, "") == 0)
        {
            printf("\n");
            continue;
        }
        sp = strtok(command, " ");
        for (i = 0; i < 13; i++)
        {
            if (strcmp(sp, cmd[i]) == 0)
            {
                cmd_idx = i;
                break;
            }
        }
        switch (cmd_idx)
        {
        case 0:
            sp = strtok(NULL, " ");
            if (sp != NULL)
                my_mkdir(sp);
            else
                printf("mkdir error\n");
            break;
        case 1:
            sp = strtok(NULL, " ");
            if (sp != NULL)
                my_rmdir(sp);
            else
                printf("rmdir error\n");
            break;
        case 2:
            my_ls();
            break;
        case 3:
            sp = strtok(NULL, " ");
            if (sp != NULL)
                my_cd(sp);
            else
                printf("cd error\n");
            break;
        case 4:
            sp = strtok(NULL, " ");
            if (sp != NULL)
                my_create(sp);
            else
                printf("create error\n");
            break;
        case 5:
            sp = strtok(NULL, " ");
            if (sp != NULL)
                my_rm(sp);
            else
                printf("rm error\n");
            break;
        case 6:
            sp = strtok(NULL, " ");
            if (sp != NULL)
                my_open(sp);
            else
                printf("open error\n");
            break;
        case 7:
            if (openfilelist[currfd].attribute == 1)
                my_close(currfd);
            else
                printf("there is not openning file\n");
            break;
        case 8:
            if (openfilelist[currfd].attribute == 1)
                my_write(currfd);
            else
                printf("please open file first, then write\n");
            break;
        case 9:
            if (openfilelist[currfd].attribute == 1)
                my_read(currfd);
            else
                printf("please open file first, then read\n");
            break;
        case 10:
            exitsys();
            printf("**************** exit file system ****************\n");
            return 0;
            break;
        case 11:
            my_format();
            printf("the file system is formated!\n");
            break;
        case 12:
            print_fast();
            break;
        default:
            printf("wrong command: %s\n", command);
            break;
        }
    }
    return 0;
}
