#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"

/* 
 * 命令语法格式
 *     deq jid
 */
void usage()
{
	printf("Usage: deq jid\n"
		"\tjid\t\t the job id\n");
}

int main(int argc,char *argv[])
{
	struct jobcmd deqcmd;
	int fd;
	
	if(argc!=2)
	{
		usage();
		return 1;
	}

	deqcmd.type=DEQ;
	deqcmd.defpri=0;
	deqcmd.owner=getuid();
	deqcmd.argnum=1;

	strcpy(deqcmd.data,*++argv);
    #ifdef DEBUG
		printf("enqcmd cmdtype\t%d\n"
			"enqcmd owner\t%d\n"
			"enqcmd argc\t%d\n"
			"enqcmd data\t%s\n",
			enqcmd.type,enqcmd.owner,enqcmd.argnum,enqcmd.data);

    #endif 

	if((fd=open(mFIFO,O_WRONLY))<0)
		error_sys("deq: open fifo failed");

	if(write(fd,&deqcmd,DATALEN)<0)
		error_sys("deq: write failed");

	close(fd);
	return 0;
}
