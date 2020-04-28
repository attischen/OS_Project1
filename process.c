#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

//333 time_now
//334 print_process
#define CHILD_CPU 1
void restrict_cpu(int pid , int cpuNum){
	if (cpuNum >= get_nprocs() ) {
		fprintf(stderr, "cpuNum invalid: %d only %d cpu available\n",cpuNum, get_nprocs());
		exit(0);
	}
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpuNum, &mask);
	if (sched_setaffinity(pid, sizeof(mask), &mask) < 0){
		perror("sched_setaffinity err\n");
	}
}

void run_unit_time(){ volatile unsigned long i; for(i=0;i<1000000UL;i++); } 
int main(int argc, char const *argv[])
{
	restrict_cpu(getpid() , CHILD_CPU);	
	int pid = getpid();

	if (argc != 3){
		printf("usage:./process process_name exec_t\n");
		exit(0);
	}

	char *process_name = argv[1];
	int exec_t = atoi(argv[2]);
	printf("%s %d %d\n",process_name,pid,exec_t);

	//long start = syscall(333);
	for(int t = 0; t < exec_t ; t++)
		run_unit_time();
	//long end = syscall(333);

	//syscall(334,pid,start,end);
	return 0;
}