all: process scheduler
scheduler:
	gcc scheduler.c -o scheduler
process:
	gcc process.c -o process
clean:
	rm process scheduler