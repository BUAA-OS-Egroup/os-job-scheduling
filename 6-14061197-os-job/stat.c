#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"

/* 
 * 命令语法格式
 *     stat
 */

char fifobuf[1024];

void usage()
{
	printf("Usage: stat\n");		
}

int main(int argc,char *argv[])
{
	struct jobcmd statcmd;
	struct stat statbuf;	
	int fd,cnt;
	char c;

	if(argc!=1)
	{
		usage();
		return 1;
	}

	statcmd.type=STAT;
	statcmd.defpri=0;
	statcmd.owner=getuid();
	statcmd.argnum=0;

    #ifdef DEBUG
		printf("enqcmd cmdtype\t%d\n"
			"enqcmd owner\t%d\n"
			"enqcmd argc\t%d\n"
			"enqcmd data\t%s\n",
			enqcmd.type,enqcmd.owner,enqcmd.argnum,enqcmd.data);

    #endif 
	

	if(stat(sFIFO,&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove(sFIFO)<0)
			error_sys("remove failed");
	}

	if(mkfifo(sFIFO,FILE_MODE)<0)
		error_sys("mkfifo failed");


	if((fd=open(mFIFO,O_WRONLY))<0)
		error_sys("stat: open fifo failed");

	if(write(fd,&statcmd,DATALEN)<0)
		error_sys("stat: write failed");

	close(fd);

	if((fd=open(sFIFO,O_RDONLY))<0)
		error_sys("open fifo failed");

	cnt=0;

	while (1)
	{
		if((read(fd,&c,1))<0)
		{
			error_sys("read fifo failed");		
			break;
		}
		else 
		{
			if (c=='$') break;
			fifobuf[cnt++]=c;
		}
	}

	fifobuf[cnt]='\0';
	printf("%s\n", fifobuf);

	if(remove(sFIFO)<0)
		error_sys("remove failed");

	return 0;
}
