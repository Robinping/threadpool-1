#include <threadpool.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define DEBUG

#ifdef __cplusplus
extern "C" {
#endif

struct task
{
	void *(*callback)(void *arg);
	void *arg;
};


struct threadpool_t
{
	int nthread;
	int queue_size;
	
	pthread_t *tid;

	struct task *tasks;
	int    head,tail;
	
	pthread_mutex_t mutex;
	pthread_cond_t  queue_not_empty;
	pthread_cond_t  queue_not_full;
};

#ifdef DEBUG
struct take_task_param
{
	threadpool_t *pool;
	int          id;
	int          ntask;
};
#endif

void *take_task(void *arg)
{
	threadpool_t *pool=(threadpool_t*)arg;
	struct task task;
#ifdef DEBUG
	struct take_task_param *param=(struct take_task_param*)arg;
	pool=param->pool;
#endif
	
	while(1)
	{
		pthread_mutex_lock(&pool->mutex);
		while ( pool->head==pool->tail ) /* task queue is empty */
		{
			pthread_cond_wait(&pool->queue_not_empty,&pool->mutex);	
		}
		task= pool->tasks[pool->head];
		pool->head=(pool->head+1)%pool->queue_size;

		pthread_mutex_unlock(&pool->mutex);
		pthread_cond_signal(&pool->queue_not_full);

		if (task.callback) /* normal task */
		{
#ifdef DEBUG
			param->ntask++;
			printf("id-->%d start ntask:%d\n",param->id,param->ntask);
#endif			
			task.callback( task.arg );
		}
		else /* abnormal: posion pill */
		{
#ifdef DEBUG			
			printf("id-->%d quit\n",param->id);
#endif				
			pthread_exit(NULL);
		}
	}
	return NULL;
}

threadpool_t *threadpool_init(int nthread,int queue_size)
{
	threadpool_t *pool;
	int i,ret;
#ifdef DEBUG	
	struct take_task_param *param;
#endif	
	/* Allocate Threadpool Structure & initalize the mutex & cond */
	pool=(threadpool_t*)malloc(sizeof(threadpool_t));
	if (pool==NULL)
	{
		fprintf(stderr,"Error: malloc error\n");
		exit(0);
	}

	pool->nthread=nthread;
	pool->tid=(pthread_t*)malloc(sizeof(pthread_t)*nthread);
	if (pool->tid==NULL)
	{
		fprintf(stderr,"Error: malloc error\n");
		exit(0);
	}
	
	/* Allocate the Task Queue */
	pool->queue_size=queue_size;
	pool->tasks=(struct task*)malloc( sizeof(struct task)*queue_size );
	if (pool->tasks==NULL)
	{
		fprintf(stderr,"Error: malloc error\n");
		exit(0);
	}
	
	if ((ret=pthread_mutex_init(&pool->mutex,NULL))!=0)
	{
		fprintf(stderr,"Error: pthread_mutex_init error\n");
		fprintf(stderr,"       %s\n",strerror(errno));
		exit(0);
	}
	if ((ret=pthread_cond_init(&pool->queue_not_empty,NULL))!=0)
	{
		fprintf(stderr,"Error: pthread_cond_init error\n");
		fprintf(stderr,"       %s\n",strerror(errno));
		exit(0);
	}
	if ((ret=pthread_cond_init(&pool->queue_not_full,NULL))!=0)
	{
		fprintf(stderr,"Error: pthread_cond_init error\n");
		fprintf(stderr,"       %s\n",strerror(errno));
		exit(0);
	}

#ifdef DEBUG
	param=(struct take_task_param*)malloc(sizeof(struct take_task_param)*nthread);
#endif	
	
	pool->head=0;
	pool->tail=0; 
	for (i=0;i<nthread;++i)
	{
#ifdef DEBUG		
		param[i].pool=pool;
		param[i].id  =i;
		param[i].ntask=0;
		pthread_create(&((pool->tid)[i]),NULL,take_task,&param[i]);
#else		
		pthread_create(&((pool->tid)[i]),NULL,take_task,pool);
#endif	
	}
	return pool;
}

void threadpool_addtask(threadpool_t *pool,void *(*callback)(void *),void *arg)
{
	pthread_mutex_lock(&pool->mutex);
	while ( pool->head==(pool->tail+1)%pool->queue_size ) /* task is full */
	{
		pthread_cond_wait(&pool->queue_not_full,&pool->mutex);			
	}
	
	pool->tasks[pool->tail].callback=callback;
	pool->tasks[pool->tail].arg     =arg;
	
	pool->tail=(pool->tail+1)%pool->queue_size;
	
	pthread_mutex_unlock(&pool->mutex);
	
	pthread_cond_signal(&pool->queue_not_empty);
}

void threadpool_destroy(threadpool_t *pool)
{
	int i;
	
	/* add posion pill */
	for (i=0;i<pool->nthread;++i)
	{
		threadpool_addtask(pool,NULL,NULL);
	}
	for (i=0;i<pool->nthread;++i)
	{
		pthread_join(pool->tid[i],NULL);
	}
	
	/* release resource */
	pthread_mutex_destroy(&pool->mutex);
	pthread_cond_destroy(&pool->queue_not_empty);
	pthread_cond_destroy(&pool->queue_not_full);
	
	free(pool->tasks);
	free(pool->tid);
	free(pool);
}

#ifdef __cplusplus
}
#endif

