/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	: Theodosiadis Konstantinos AEM 8296
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

#define QUEUESIZE 10
#define LOOP 10000
#define   P 1 //Producer's threads//
#define   Q 1 //Consumer's threads//

//Functions for the fifo
void *random_fun_1(void* x);
void *random_fun_2(void* x);
void *random_fun_3(void* x);
void *random_fun_4(void* x);

//Random functions in workFunction
static void * (*random_array[4])(void *) = {&random_fun_1,&random_fun_2,&random_fun_3,&random_fun_4};

void *producer (void *args);
void *consumer (void *args);


typedef struct{
		void * (*work)(void *);
		void * arg;
		struct timeval insert_time; //For computing time
} workFunction;

typedef struct {
		workFunction buf[QUEUESIZE]; //Buffer now takes workFunction variables
		long head, tail;
		int full, empty;
		pthread_mutex_t *mut;
		pthread_cond_t *notFull, *notEmpty;
		double *results;

} queue;

queue *queueInit (double *intervals);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in); //queueAdd now adds workFunction variable
void queueDel (queue *q, workFunction *out); //queueDel now deletes workFunction variable

int resultsCounter;

int main (){

		resultsCounter = 0;

		double *intervals = (double *)malloc((LOOP*P)*sizeof(double));

		FILE *f;
		f = fopen("archive.csv","w");

		// pthread_t *pro = (pthread_t *) malloc(P * sizeof(pthread_t));
		// pthread_t *con = (pthread_t *) malloc(Q * sizeof(pthread_t));
		pthread_t pro[P], con[Q];

		queue *fifo;
		fifo = queueInit (intervals);

		if (fifo ==  NULL) {
				fprintf (stderr, "main: Queue Init failed.\n");
				exit (1);
		}

		for(int j=0; j<Q; j++){
				pthread_create (&con[j], NULL, consumer, (void *)fifo);
		}

		for(int i=0; i<P; i++){
				pthread_create (&pro[i], NULL, producer, (void *)fifo);
		}


		for(int i=0; i<P; i++){
				pthread_join (pro[i], NULL);
		}


		for(int j=0; j<Q; j++){
				pthread_join (con[j], NULL);
		}

		queueDelete (fifo);


		for(int i=0; i<LOOP*P; i++){
				fprintf(f,"%f \n",intervals[i]);
		}

		free(intervals);

		fclose(f);

		return 0;
}

void *producer (void *q){


		queue *fifo;
		fifo = (queue *)q;

		void * ptr;

		srand(time(NULL));

		for (int i = 0; i < LOOP; i++) {

				int j=rand()%4;
				ptr = &j;

				workFunction input;
				//Arguments to the workFunction
				input.arg = ptr;
				input.work = random_array[*((int *) input.arg)];

				pthread_mutex_lock (fifo->mut);

				while (fifo->full) {
					// printf ("producer: queue FULL.\n");
					pthread_cond_wait (fifo->notFull, fifo->mut);
				}

				//We add it to the fifo
				gettimeofday(&input.insert_time,NULL);
				queueAdd (fifo, input);

				pthread_mutex_unlock (fifo->mut);
				pthread_cond_signal (fifo->notEmpty);

		}


		return (NULL);
}

void *consumer (void *q){

		queue *fifo;
		fifo = (queue *)q;

		workFunction out;
		struct timeval pickup_time ;

		while(1){

			pthread_mutex_lock (fifo->mut);
			
			if(resultsCounter == LOOP*P-1){
					pthread_mutex_unlock (fifo->mut);
					break;
			}
			
			while (fifo->empty) {
					// printf ("consumer: queue EMPTY.\n");
					pthread_cond_wait (fifo->notEmpty, fifo->mut);
			}

			queueDel (fifo, &out);
			gettimeofday(&pickup_time,NULL);


			fifo->results[resultsCounter] = (double)((pickup_time.tv_usec-out.insert_time.tv_usec)/1.0e6+pickup_time.tv_sec-out.insert_time.tv_sec);
			resultsCounter++;

			pthread_mutex_unlock (fifo->mut);
			pthread_cond_signal (fifo->notFull);

			(*out.work)(out.arg);
		}
  	return (NULL);
}

queue *queueInit (double *intervals){

	queue *q;

	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	q->empty = 1;
	q->full = 0;
	q->head = 0;
	q->tail = 0;
	q->results = intervals;
	q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (q->mut, NULL);
	q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notFull, NULL);
	q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q){

	pthread_mutex_destroy (q->mut);
	free (q->mut);
	pthread_cond_destroy (q->notFull);
	free (q->notFull);
	pthread_cond_destroy (q->notEmpty);
	free (q->notEmpty);
	free (q);
}

void queueAdd (queue *q, workFunction in){

	q->buf[q->tail] = in; //buf now takes workFunction elements
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	q->empty = 0;

	return;
}

//make the out varible workFunction type//
void queueDel (queue *q, workFunction *out){

	*out = q->buf[q->head];

	q->head++;
	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	q->full = 0;

	return;
}

void* random_fun_1(void* arg){
	printf("Hello this is random_fun_1\n");
	return (NULL);
}

void* random_fun_2(void* arg){
	int i;
	printf("The sine of 10 random angles is:");
	for(i=0 ; i<10 ; i++){
		printf("%.2f ", sin(rand()%1000));
	}
	printf("\n");
	return (NULL);
}

void* random_fun_3(void* arg){
	int i;
	printf("The cosine of 10 random angels is:");
	for(i=0 ; i<10 ; i++){
		printf("%.2f ", cos(rand()%1000));
	}
	printf("\n");
	return (NULL);
}

void* random_fun_4(void* arg){
	int i;
	printf("The tangent of 10 random angels is:");
	for(i=0 ; i<10 ; i++){
		printf("%.2f ", tan(rand()%1000));;
	}
	printf("\n");
	return (NULL);
}
