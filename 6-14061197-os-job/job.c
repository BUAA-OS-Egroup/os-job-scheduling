#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "job.h"

#define N 3

#define sch_mode 1

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int mutex=1;
struct waitqueue **head=NULL;
struct waitqueue *pq[N]={NULL,NULL,NULL};
const int slice[N]={5000,2000,1000};
const int duration=500, upgrade_time=10000;
struct waitqueue *next=NULL,*current =NULL;


void P(int *x)
{
	while ((*x)<=0);
	--(*x);
}

void V(int *x)
{
	++(*x);
}

/* 调度程序 */
void scheduler()
{
	struct jobinfo *newjob=NULL;
	struct jobcmd cmd;
	int  count = 0;
	bzero(&cmd,DATALEN);
	if((count = read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");
#ifdef DEBUG
	printf("Reading whether other process send command!\n");
	if(count){
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	}
	else
		printf("no data read\n");//different from PPT
#endif

#ifdef DEBUG
	printf("Update jobs in wait queue!\n");
#endif
updateall();

	switch(cmd.type){
	case ENQ:
#ifdef DEBUG
	printf("Execute enq!\n");
#endif
		do_enq(newjob,cmd);
		break;
	case DEQ:
#ifdef DEBUG
	printf("Execute deq!\n");
#endif
		do_deq(cmd);
		break;
	case STAT:
#ifdef DEBUG
	printf("Execute stat!\n");
#endif
		do_stat(cmd);
		break;
	default:
		break;
	}

	/* 更新等待队列中的作业 */
	if (sch_mode==0) updateall();
	if (sch_mode==1) updateall2();
	/* 选择高优先级作业 */
#ifdef DEBUG
	printf("Select which job to run next!\n");
#endif
	if (sch_mode==0) next = jobselect();
	if (sch_mode==1) next = jobselect2();
	/* 作业切换 */
#ifdef DEBUG
	printf("Switch to the next job!\n");
#endif
	if (sch_mode==0) jobswitch();
	if (sch_mode==1) jobswitch2();
}

int allocjid()
{
	return ++jobid;
}

void push_back(struct waitqueue**h, struct waitqueue* x)
{
	struct waitqueue* p;
	x->next=NULL;
	if ((*h)==NULL)
	{
		(*h)=x;
	}
	else 
	{
		for(p = (*h); p->next != NULL; p = p->next);
		p->next=x;
	}

}

void updateall2()
{
//#define DEBUG
#ifdef DEBUG
	struct jobcmd cmd1;
	do_stat(cmd1);
#endif///////////////////////////////////////task6

	struct waitqueue *p, *pre, *tmp, *o;
	int i;

	o=(struct waitqueue*)malloc(sizeof(struct waitqueue*));
	/* 更新作业运行时间 */
	if(current)
	{
		current->job->run_time += duration; /* 总运行时间加duration间隔 */
		current->job->s_time += duration; /* 时间片加duration间隔 */
	}

	/* 更新作业等待时间及优先级 */
	for (i=0;i<N;++i)
	for(pre=NULL, p = pq[i]; p != NULL;pre=p, p = p->next){
		p->job->wait_time += duration;
		if(p->job->wait_time >= upgrade_time && p->job->curpri < N){
			++(p->job->curpri);
			p->job->wait_time = 0;
		} 
		if ((p->job->curpri-1)>i) {
			tmp=p;
			if (pre!=NULL) {
				pre->next=p->next;
				p=pre;
			}
			else {
				pq[i]=p->next;
				o->next=p->next;
				p=o;
			}
			push_back(&pq[tmp->job->curpri-1],tmp);
		}
	}
	// printf("%s\n", pq[0]!=NULL?"YES":"NO");
	// printf("%s\n", pq[1]!=NULL?"YES":"NO");
	// printf("%s\n", pq[2]!=NULL?"YES":"NO");
#ifdef DEBUG
	struct jobcmd cmd2;
	do_stat(cmd2);
#endif///////////////////////////////////////task6
}

void updateall()
{
#ifdef DEBUG
	struct jobcmd cmd1;
	do_stat(cmd1);
#endif///////////////////////////////////////task6
	struct waitqueue *p;
	/* 更新作业运行时间 */
	if(current)
		current->job->run_time += 1; /* 加1代表1000ms */

	/* 更新作业等待时间及优先级 */
	for(p = (*head); p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 5000 && p->job->curpri < 3){
			p->job->curpri++;
			p->job->wait_time = 0;
		}
	}
#ifdef DEBUG
	struct jobcmd cmd2;
	do_stat(cmd2);
#endif///////////////////////////////////////task6
#undef DEBUG
}

struct waitqueue* jobselect()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;

	select = NULL;
	selectprev = NULL;
	if((*head)){
		/* 遍历等待队列中的作业，找到优先级最高的作业 */
		for(prev = NULL, p = (*head); p != NULL; prev = p,p = p->next)
			if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}
			if (selectprev!=NULL) selectprev->next = select->next;
			else (*head) = select->next;
			select->next = NULL;
	}
	return select;
}

struct waitqueue* jobselect2()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest, i, lowest;
	if (current!=NULL&&current->job->s_time<slice[get_pri(current->job->curpri)]) highest=current->job->curpri;
	else highest=-1;

	lowest=current!=NULL?get_pri(current->job->curpri):0;

	select = NULL;
	selectprev = NULL;

	for (i=N-1;i>=lowest;--i)
	if(pq[i]){
		/* 遍历等待队列中的作业，找到优先级最高的作业 */
		for(prev = NULL, p = pq[i]; p != NULL; prev = p,p = p->next)
			if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}
		if (select!=NULL)
		{
			if (selectprev!=NULL) selectprev->next = select->next;
			else pq[i] = select->next;
			select->next = NULL;
			break;
		}
	}
	return select;
}

int get_pri(int x)
{
	return (x-1>=0)?(x-1):0;
}

void jobswitch2()
{
	struct waitqueue *p;
	int i;

	if(current && current->job->state == DONE){ /* 当前作业完成 */
		/* 作业完成，删除它 */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* 释放空间 */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	//printf("%s\n", next?"havenxt":"nonxt");

	if(next == NULL && current == NULL) /* 没有作业要运行 */

		return;
	else if (next != NULL && current == NULL){ /* 开始新的作业 */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->s_time = 0;
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else if (next != NULL && current != NULL) { /* 切换作业 */
		// if (current->job->curpri>next->job->curpri||current->job->curpri==next->job->curpri&&current->job->s_time<slice[get_pri(current->job->curpri)])
		// {
		// 	push_back(&pq[get_pri(next->job->curpri)],next);			
		// 	return;
		// }

		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		//current->job->curpri = current->job->defpri;
		--current->job->curpri;
		if (current->job->curpri<current->job->defpri) current->job->curpri=current->job->defpri;
		current->job->wait_time = 0;
		current->job->state = READY;

		/* 放回等待队列 */
		push_back(&pq[get_pri(current->job->curpri)],current);

		current = next;
		next = NULL;
		current->job->s_time = 0;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}else{ /* next == NULL且current != NULL，不切换 */
		return;
	}
}

void jobswitch()
{
	struct waitqueue *p;
	int i;

	if(current && current->job->state == DONE){ /* 当前作业完成 */
		/* 作业完成，删除它 */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* 释放空间 */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	if(next == NULL && current == NULL) /* 没有作业要运行 */

		return;
	else if (next != NULL && current == NULL){ /* 开始新的作业 */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else if (next != NULL && current != NULL){ /* 切换作业 */

		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		//current->job->curpri = current->job->defpri;
		--current->job->curpri;
		if (current->job->curpri<current->job->defpri) current->job->curpri=current->job->defpri;

		current->job->wait_time = 0;
		current->job->state = READY;

		/* 放回等待队列 */
		if((*head)){
			for(p = (*head); p->next != NULL; p = p->next);
			p->next = current;
		}else{
			(*head) = current;
		}
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}else{ /* next == NULL且current != NULL，不切换 */
		return;
	}
}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;
//	P(&mutex);
	switch (sig) {
case SIGVTALRM: /* 到达计时器所设置的计时间隔 */
	scheduler();
#ifdef DEBUG
	printf("SIGVTALRM RECEIVED!.\n");
#endif
//	V(&mutex);
	return;
case SIGCHLD: /* 子进程结束时传送给父进程的信号 */
	ret = waitpid(-1,&status,WNOHANG);
	if (ret == 0)
	{
//		V(&mutex);
		return;
	}
	if(WIFEXITED(status)){
		current->job->state = DONE;
		printf("normal termation, exit status = %d\n",WEXITSTATUS(status));
	}else if (WIFSIGNALED(status)){
		printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
	}else if (WIFSTOPPED(status)){
		printf("child stopped, signal number = %d\n",WSTOPSIG(status));
	}
//	V(&mutex);
	return;
	default:
//		V(&mutex);
		return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;


	sigemptyset(&zeromask);

	/* 封装jobinfo数据结构 */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;
	newjob->run_time = 0;
	newjob->s_time = 0;
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		}else
			offset++;
	}

	arglist[i] = NULL;

#ifdef DEBUG

	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for(i = 0;i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);

#endif

	/*向等待队列中增加新的作业*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job = newjob;

	if((*head))
	{
		for(p = (*head);p->next != NULL; p = p->next);
		p->next = newnode;
	}else
		(*head) = newnode;

	/*为作业创建进程*/
	if((pid = fork())<0)
		error_sys("enq fork failed");

	if(pid==0){
		newjob->pid = getpid();
		/*阻塞子进程,等等执行*/
		raise(SIGSTOP);
#ifdef DEBUG

		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
#endif

		/*复制文件描述符到标准输出*/
		dup2(globalfd,1);
		/* 执行命令 */
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
//		V(&mutex);
		sleep(1000);
//		P(&mutex);
		newjob->pid = pid;
	}

}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;


	deqid = atoi(deqcmd.data);

#ifdef DEBUG
	printf("deq jid %d\n",deqid);
#endif

	/*current jodid==deqid,终止当前作业*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else{ /* 或者在等待队列中查找deqid */
		select=NULL;
		selectprev=NULL;
		for (i=N-1;i>=0;--i) {
			for(prev = NULL,p = pq[i];p!=NULL;prev = p,p = p->next)
				if(p->job->jid==deqid){
					select = p;
					selectprev = prev;
					break;
				}
				if (select!=NULL)
				{
					if (selectprev!=NULL) selectprev->next = select->next;
					else pq[i]=select->next;
					printf("%s\n", "del_succ");
					break;
				}
		}
		if(select){
			for(i=0;(select->job->cmdarg)[i]!=NULL;i++){
				free((select->job->cmdarg)[i]);
				(select->job->cmdarg)[i]=NULL;
			}
			free(select->job->cmdarg);
			free(select->job);
			free(select);
			select=NULL;
		}
	}

}

void do_stat(struct jobcmd statcmd)
{
	struct waitqueue *p;
	char timebuf[BUFLEN];
	int cnt=0, i;

	/*
	*打印所有作业的统计信息:
	*1.作业ID
	*2.进程ID
	*3.作业所有者
	*4.作业运行时间
	*5.作业等待时间
	*6.作业创建时间
	*7.作业状态
	*/

	/* 打印信息头部 */
	printf("JID\tPID\tOID\tRTIME\tWTIME\tPRI\tCTIME\t\t\t\tSTATUS\n");
	if(current){
		++cnt;
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			current->job->curpri,
			timebuf,"RUNNING");
	}

	for (i=N-1;i>=0;--i)
	for(p = pq[i];p!=NULL;p = p->next){
		++cnt;
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			p->job->curpri,
			timebuf,
			"READY");
	}
	printf("-Total: %d jobs\n", cnt);
}

void dispose()
{
	siginfo=0;
}

int main()
{
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;

#ifdef DEBUG
	printf("DEBUG IS OPEN!\n");
#endif

	if(stat(mFIFO,&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove(mFIFO)<0)
			error_sys("remove failed");
	}

	if(mkfifo(mFIFO,FILE_MODE)<0)
		error_sys("mkfifo failed");
	/* 在非阻塞模式下打开FIFO */
	if((fifo=open(mFIFO,O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	head=&pq[0];
	/* 建立信号处理函数 */
	newact.sa_sigaction=sig_handler;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGVTALRM,&newact,&oldact2);
	signal(SIGINT,dispose);

	/* 设置时间间隔为1000毫秒 */
	if (sch_mode==1)
	{
		interval.tv_sec=0;
		interval.tv_usec=duration*1000;
	}
	if (sch_mode==0)
	{
		interval.tv_sec=1;
		interval.tv_usec=0;
	}

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(globalfd);
	puts("");
	if(remove(mFIFO)<0)
		error_sys("remove failed");
	printf("%s\n", "bye");
	return 0;
}
