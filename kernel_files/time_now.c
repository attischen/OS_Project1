#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/timer.h>
asmlinkage long sys_time_now(){
	struct timespec t;
	getnstimeofday(&t);
	return(t.tv_sec*1000000000+t.tv_nsec);
}