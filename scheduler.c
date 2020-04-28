#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#define max_Process 1000
#define max_process_name 100
#define CHILD_CPU 1
#define PARENT_CPU 0
#define time_quantum 500

int running = -1;     // CPU user
int next_run;

struct process{
	char name[max_process_name];
	int ready_t;
	int exec_t;
	int pid;
};
struct process P[max_Process];

void read_input(struct process * P,int N){
		for (int i = 0; i < N; i++){
			scanf("%s%d%d", P[i].name,&(P[i].ready_t), &(P[i].exec_t));
		}
}

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

void suspend(int pid){
	//printf("suspend pid %d\n",pid);
	struct sched_param s;
	s.sched_priority = 20;
	if(sched_setscheduler(pid, SCHED_FIFO, &s) < 0){
		perror("sched_setscheduler suspend \n");
	}
}

void giveCPU(int pid){
	//printf("giveCPU pid %d\n",pid);
	struct sched_param s;
	s.sched_priority = 80;
	if(sched_setscheduler(pid, SCHED_FIFO, &s) < 0){
		perror("sched_setscheduler giveCPU \n");
	}
}


void create_process(int ready){
	int p;
	if ((p = fork()) == 0){
		restrict_cpu(getpid() , CHILD_CPU);	
		char  exec_t[20];
        sprintf(exec_t,"%d", P[ready].exec_t );
        execl("./process", "./process", P[ready].name , exec_t, (char *)0);
	}else if(p < 0){
		perror("fork err\n");	
	}else{
		suspend(p);
		P[ready].pid = p;
	}
}

void run_unit_time(){ volatile unsigned long i; for(i=0;i<1000000UL;i++); } 

int cmp(const void *a, const void *b) {
	return ((struct process *)a)->ready_t - ((struct process *)b)->ready_t;
}


int FIFO_running_index = -1;
int RR_turn = -1;
int last_time = 0;
int quantum_start = -1;
int t;

int next_CPU_user(int type,int running ,int N_process){
	switch(type){
		
		case(0):{//FIFO
			if(running != -1)
				return running;
			if(P[FIFO_running_index + 1].ready_t <= t){
				FIFO_running_index++;
				return FIFO_running_index;
			}
			return -1;
			break;
		}

		case(1):{//RR
			if (running == -1 || t - last_time > time_quantum){
				int i;
				for(i = (RR_turn + 1) % N_process ; i != RR_turn ;i = (i + 1) % N_process){
					if(P[i].exec_t != -1 && P[i].ready_t <= t){
						RR_turn = i;
						last_time = t;
						return i;
					}		
					// no process need CPU
					if(RR_turn == -1 && i == N_process - 1){
						return -1;
					}
				}
				// same process exec again
				if(P[i].exec_t != -1 && P[i].ready_t <= t){
					last_time = t;
					return i;
				}
				//idle
				return -1;
			}else
				return running;
			break;
		}

		case(2):{//SJF
			if(running != -1)
				return running;
			int ret = -1;
			for(int i = 0; i < N_process;i++){
				if(P[i].exec_t != -1 && P[i].ready_t <= t){
					if(ret == -1 || P[i].exec_t < P[ret].exec_t){
						ret = i;
					}
				}
			}
			return ret;
			break;
		}

		case(3):{//PSJF
			int ret = running;
			for(int i = 0; i < N_process;i++){
				if(P[i].exec_t != -1 && P[i].ready_t <= t){
					if(ret == -1 || P[i].exec_t < P[ret].exec_t){
						ret = i;
					}
				}
			}
			return ret;
			break;
		}
	}
}

int main(){
	char schedule_str[5];
	int schedule_type,N_process;
	scanf("%s%d",schedule_str,&N_process);
	//printf("N %d\n",N_process);
	char alltypes[4][5]={"FIFO","RR","SJF","PSJF"};
	for (schedule_type = 0; schedule_type < 5 ; schedule_type++){
		if(schedule_type == 4){
			printf("unsupported schedule_type:%s\n",schedule_str);
			exit(0);
		}
		if (strcmp(schedule_str,alltypes[schedule_type]) == 0)
			break;
	}
	if(N_process == 0)
		exit(0);
	
	long start_time[max_Process];
	long end_time[max_Process];
	read_input(P,N_process);

	//sort by ready_t,exec_t if ready_t is same
	qsort(P,N_process,sizeof(struct process),cmp);

	//run on different CPU then child
	restrict_cpu(getpid(), PARENT_CPU);

	int lastPrepare = -1; // prepare is the next cadidate of CPU user

	//next ready is the process with min ready_t not yet created
	int next_ready = 0,complete_process = 0;
	for( t = 0; ; t ++){
		/*if(t%100 == 0){
			printf("%d\n",t );
			fflush(stdout);
		}*/
		//check process termination
		if (running != -1){
			int status;
			if(waitpid(P[running].pid,&status,WNOHANG) != 0  && WIFEXITED(status) ){
#ifdef DEBUG
				printf("%s terminated\n",P[running].name);	
#endif
				end_time[running] = syscall(333); //time_now()
				//print_process()
				syscall(334,P[running].pid,start_time[running],end_time[running]);
				complete_process++;
				P[running].exec_t = -1;
				running = -1;

				if ( complete_process == N_process){
					//printf("everone finished\n");
					exit(0);
				}
				
			}			
		}

		//enter new process
		while(next_ready < N_process && t == P[next_ready].ready_t){
#ifdef DEBUG 
			printf("%s created \n", P[next_ready].name);
#endif
			create_process(next_ready);		
			start_time[next_ready] = syscall(333); //time_now()
	      	next_ready++;
		}

		//switch CPU 
		next_run = next_CPU_user(schedule_type,running,N_process);
		//-1 : CPU idle 
#ifdef DEBUG 
		if( next_run != running){
			printf("%d %d\n",running,next_run);
			//if(running != -1 && next_run != -1)
			//	printf("CPU:%s -> %s\n",P[running].name,P[next_run].name);
		}
#endif
		if (running != -1 && running != next_run){
			suspend(P[running].pid);		
		}
		if (next_run != -1 && running != next_run){
			giveCPU(P[next_run].pid);	
		}
		running = next_run;

		

		run_unit_time();
		if(running != -1 && P[running].exec_t > 0){
			P[running].exec_t --;
		}
	}
}