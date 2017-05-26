#include <stdio.h>
#include <threadpool.h>

void *task1(void *arg)
{
	printf("1->%d\n",(int)(arg));
//	sleep(1);
	
	return NULL;
}

void *task2(void *arg)
{
	printf("2->%d\n",(int)(arg));
//	sleep(1);
	
	return NULL;
}

int main(int argc,char *argv[])
{
	threadpool_t *pool;
	int i;
	
	pool=threadpool_init(10,20);
	
	for (i=0;i<100;++i)
	{
		threadpool_addtask(pool,task1,(void*)i);
		threadpool_addtask(pool,task2,(void*)i);
	}
	
	threadpool_destroy(pool);
	
	return 0;
}

