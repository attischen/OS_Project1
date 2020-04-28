#include <linux/linkage.h>
#include <linux/kernel.h>
//334
asmlinkage void sys_print_process(int pid, long start, long end) {
    static const long BASE = 1000000000;
    printk(KERN_INFO "[Project1] %d %ld.%09ld %ld.%09ld\n", pid, start/ BASE, start % BASE, end / BASE, end % BASE);
}