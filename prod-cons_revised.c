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

#define QUEUESIZE 1000
#define LOOP 700
#define   P 1 //Producer's threads//
#define   Q 1 //Consumer's threads//

//Functions for the fifo
void *random_fun_1(void* x);
void *random_fun_2(void* x);
void *random_fun_3(void* x);
void *random_fun_4(void* x);

//Random functions in workFunction
static void * (*random_array[4])(void *) = {&random_fun_1,&random_fun_2,&random_fun_3,&random_fun_4};

pthread_mutex_t lock; //mutex so we dont have overlaps

void *producer (void *args);
void *consumer (void *args);

static int sum=0;

typedef struct{
  void * (*work)(void *);
  void * arg;
  struct timeval insert_time,receive_time; //For computing time
  double elapsed_time;
} workFunction;

typedef struct {
  workFunction buf[QUEUESIZE]; //Buffer now takes workFunction variables
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut,*prod_mutex;
  pthread_cond_t *notFull, *notEmpty;

  int prod_counter;//We count the number of producers that are finished
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in); //queueAdd now adds workFunction variable
void queueDel (queue *q, workFunction *out); //queueDel now deletes workFunction variable

FILE *f; //To keep the results

int main ()
{
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("\n mutex init has failed\n");
    return 1;
  }
  queue *fifo;
  pthread_t pro[P], con[Q];

  fifo = queueInit ();

  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  f = fopen("archive.csv","w");

  for(int i=0; i<P; i++){
    pthread_create (&pro[i], NULL, producer, (void *)fifo);
  }

  for(int j=0; j<Q; j++){
    pthread_create (&con[j], NULL, consumer, (void *)fifo);
  }

  for(int i=0; i<P; i++){
    pthread_join (pro[i], NULL);
  }

  for(int j=0; j<Q; j++){
    pthread_join (con[j], NULL);
  }
  //This will not run
  queueDelete (fifo);
  pthread_mutex_destroy(&lock);
  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;
  void * ptr;
  srand(time(NULL)); //Random selection of function

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }

    workFunction funcs; //Element that we 'll put on fifo
 
    int j=rand()%4;
    ptr = &j;

    //Arguments to the workFunction
    funcs.arg = ptr;
    funcs.work = random_array[*((int *) funcs.arg)];

    //We add it to the fifo
    gettimeofday(&funcs.insert_time,NULL);
    queueAdd (fifo, funcs);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    //We dont use sleep
  }
  pthread_mutex_lock (fifo->prod_mutex);
  fifo->prod_counter++;
  pthread_mutex_unlock (fifo->prod_mutex);
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction d; //Parameter d now has workFunction argument
  fifo = (queue *)q;

  while(1){
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      sum++;
      if(sum == Q){
        fclose(f);
        exit(0);
      }
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &d);
    gettimeofday(&d.receive_time,NULL);
    d.elapsed_time = (double)((d.receive_time.tv_usec-d.insert_time.tv_usec)/1.0e6+d.receive_time.tv_sec-d.insert_time.tv_sec);
    fprintf(f,"%f \n",d.elapsed_time);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    (*d.work)(d.arg);
    //We dont use sleep
  }
  return (NULL);
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->prod_counter = 0;
  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->prod_mutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->prod_mutex, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);
  pthread_mutex_destroy (q->prod_mutex);
  free (q->prod_mutex);
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
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
void queueDel (queue *q, workFunction *out)
{
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
  pthread_mutex_lock(&lock);
  printf("Hello this is random_fun_1\n");
  pthread_mutex_unlock(&lock);
  return (NULL);
}

void* random_fun_2(void* arg){
  pthread_mutex_lock(&lock);
  int i;
  printf("The sine of 10 random angles is:");
  for(i=0 ; i<10 ; i++){
    printf("%.2f ", sin(rand()%1000));
  }
  printf("\n");
  pthread_mutex_unlock(&lock);
  return (NULL);
}

void* random_fun_3(void* arg){
  pthread_mutex_lock(&lock);
  int i;
  printf("The cosine of 10 random angels is:");
  for(i=0 ; i<10 ; i++){
    printf("%.2f ", cos(rand()%1000));
  }
  printf("\n");
  pthread_mutex_unlock(&lock);
  return (NULL);
}

void* random_fun_4(void* arg){
  pthread_mutex_lock(&lock);
  int i;
  printf("The tangent of 10 random angels is:");
  for(i=0 ; i<10 ; i++){
    printf("%.2f ", tan(rand()%1000));;
  }
  printf("\n");
  pthread_mutex_unlock(&lock);
  return (NULL);
}
