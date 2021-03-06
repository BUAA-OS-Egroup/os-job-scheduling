#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"
#define DEBUG
/* 
 * 命令语法格式
 *     enq [-p num] e_file args
 */
void usage()
{
	printf("Usage: enq[-p num] e_file args\n"
		"\t-p num\t\t specify the job priority\n"
		"\te_file\t\t the absolute path of the exefile\n"
		"\targs\t\t the args passed to the e_file\n");
}

int main(int argc,char *argv[])
{
	int p=1,flag=0;
	int fd;
	char c,*offset;
	struct jobcmd enqcmd;

	if(argc==1)
	{
		usage();
		return 1;
	}

	while(--argc>0 && (*++argv)[0]=='-')
	{
		//printf("%s\n", *argv);
		while(c=*++argv[0])
		{
			switch(c)
			{
				case 'p':
					if (argc<2){
						printf("%s\n", "-p: priority required");	
						return 1;		
					}
					p=atoi(*(++argv));
					argc--;
					flag=1;
					break;
				default:
					printf("enq: Illegal option %c\n",c);
					return 1;
			}
			if (flag) 
			{
				flag=0;
				break;
			}
		}
	}

	if(p<1||p>3)
	{
		printf("enq: invalid priority:must between 1 and 3\n");
		return 1;
	}

	enqcmd.type=ENQ;
	enqcmd.defpri=p;
	enqcmd.owner=getuid();
	enqcmd.argnum=argc;
	offset=enqcmd.data;

	if (argc<=0)
	{
		printf("%s\n", "enq: command required");	
		return 1;
	}

	while (argc-->0)
	{
		strcpy(offset,*argv);
		strcat(offset,":");
		offset=offset+strlen(*argv)+1;
		argv++;
	}

    #ifdef DEBUG
		printf("enqcmd cmdtype\t%d\n"
			"enqcmd owner\t%d\n"
			"enqcmd defpri\t%d\n"
			"enqcmd argc\t%d\n"
			"enqcmd data\t%s\n",
			enqcmd.type,enqcmd.owner,enqcmd.defpri,enqcmd.argnum,enqcmd.data);

    #endif 

		if((fd=open(mFIFO,O_WRONLY))<0)
//			if (mkfifo(mFIFO,FILE_MODE)<0||(fd=open(mFIFO,O_WRONLY))<0)
				error_sys("enq: open fifo failed");

		if(write(fd,&enqcmd,DATALEN)<0)
			error_sys("enq: write failed");

		close(fd);
		return 0;
}

