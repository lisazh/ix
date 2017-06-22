/* 
 * Parametrizable I/O "Benchmark"
 * 
 */ 

// copy & pasted not sure if need all of them...
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <ixev.h>

//#define MAX_KEYS 10
#define MAX_KEY_LEN 5 //overwrite the internal value for benchmarking..
#define DEF_IO_SIZE 128

enum {
		READ_ONLY = 0, 
		WRITE_ONLY, 
		READ_WRITE
};


//GLOBALS
struct ixev_io_ops io_ops;
char *iobuf;
char **keys;
struct timeval **timers;
struct timeval glob_timer;

int curr_iter = 0;
int resp_iter = 0; //

//input params -- default values for all
uint8_t iotype;
int batchsize = 1;
int io_size = DEF_IO_SIZE; //writes only
char *fname = "keys.ix";
int max_iter = 4;

// dummy functions for ixev_conn_ops 
static struct ixev_ctx *io_dummyaccept(struct ip_tuple *id)
{
	return NULL;
}

static void io_dummyrelease(struct ixev_ctx *ctx) { }

static void io_dummydialed(struct ixev_ctx *ctx, long ret) { }

struct ixev_conn_ops conn_ops = {
	.accept = &io_dummyaccept,
	.release = &io_dummyrelease, 
	.dialed = &io_dummydialed,
};


static void start_timer(char *key){
	int ind = atoi(key) - 1;
	printf("DEBUG: timer index is %s (str) %d (int) \n", key, ind);
	//int ind = (atoi(key) * (curr_iter/batchsize)) - 1;
	struct timeval *timer = timers[ind];
	if (gettimeofday(timer, NULL)){
		fprintf(stderr, "Timer issue. \n");
		exit(1);
	}
}

static void end_timer(char * key){
	int ind = atoi(key) - 1;

	struct timeval *timer = timers[ind];
	time_t old_secs = timer->tv_sec;
	suseconds_t old_usecs = timer->tv_usec;

	if (gettimeofday(timer, NULL)){
		fprintf(stderr, "Timer issue.\n");
		exit(1);
	}	

	//struct timeval *timer = &(timers[atoi(key)]);
	timer->tv_sec = timer->tv_sec - old_secs;
	timer->tv_usec = timer->tv_usec - old_usecs;
	
	printf("DEBUG: finishing timer for key %s with times %ld:%ld\n", key, timer->tv_sec, timer->tv_usec);
}


/*
static void start_timer(int ind){
	struct timeval *timer = timers[ind];
	if (gettimeofday(timer, NULL)){
		fprintf(stderr, "Timer issue. \n");
		exit(1);
	}
}

static void end_timer(int ind){

	struct timeval *timer = timers[ind];
	time_t old_secs = timer->tv_sec;
	suseconds_t old_usecs = timer->tv_usec;

	if (gettimeofday(timer, NULL)){
		fprintf(stderr, "Timer issue.\n");
		exit(1);
	}

	timer->tv_sec = timer->tv_sec - old_secs;
	timer->tv_usec = timer->tv_usec - old_usecs;
	
	printf("DEBUG: finishing timer for key %s with times %ld:%ld\n", key, timer->tv_sec, timer->tv_usec);
}
*/

/*
static void flush_timers(){
	FILE *res;

	//TODO uniquely identify results file on every run..
	if ((res = fopen("results.ix", "w")) == NULL){
		fprintf(stderr, "Unable to open file %s\n", fname);
		exit(1);
	}

	for (int i = 0; i < max_iter; i++){ //TODO double check format..
		//struct timeval *timer = (timers + (i * sizeof(struct timeval)));
		fprintf(res, "%d,%ld:%ld\n", i+1, timer->tv_sec, timer->tv_usec);
	}	 

	fclose(res);

}
*/
/* Generates dummy data according to size 
 * super dumb version for now...improve later.
 */
void generate_data(size_t datasize, char *buf){
	//printf("DEBUG: iobuf starting at %p\n", buf);
	//srand(time(0));
	//int datum; 
	//while (datasize){
		//datum = rand();
//		printf("DEBUG: size of rand thing %lu\n", sizeof(datum));
		//*((int *)buf) = datum;
		//datasize -= sizeof(datum);
		//buf += sizeof(datum);
	//}
	for (int i = 0; i < datasize; i++){
		buf[i] = 'a';		
	}
	buf[datasize] = '\0';
	printf("DEBUG: data generated is %s at %p\n", buf, buf);
}

void get_keys(){
	
	if (fname == NULL){
		fprintf(stderr, "No file specified\n");
		exit(1);
	}

	FILE *fkeys; 
	
	if ((fkeys = fopen(fname, "r")) == NULL){
		fprintf(stderr, "Unable to open file %s\n", fname);
		exit(1);
	}

	keys = malloc(sizeof(char *) * max_iter);

	for (int i = 0; i < max_iter; i++){
		char *key = malloc(MAX_KEY_LEN);
		if (fgets(key, MAX_KEY_LEN, fkeys) == NULL){
			fprintf(stderr, "Unable to read key from %s\n", fname);
			fclose(fkeys);
			exit(1);
		}
		key[strcspn(key, "\n")] = '\0'; //remove trailing \n
		keys[i] = key;
		//printf("DEBUG: alloc'd and read key %s at %p\n", keys[i], keys[i]); 
	}
	fclose(fkeys);

}

// allow specification of data file too...
void get_data(char *fdataname){
	FILE *fdata;
	fdata = fopen(fdataname, "r");
	
	for (int i = 0; i < max_iter*batchsize; i++){ //read everything..
		if (fgets((iobuf + (i*io_size)), io_size, fdata) == NULL){
			fprintf(stderr, "Unable to read data from %s\n", fname);
			fclose(fdata);
			exit(1);
		}
	}
	fclose(fdata);
}
	

void free_keys(){
	for (int i = 0; i < max_iter; i++){
		free(keys[i]);
	}
	free(keys);
}


/* Called only once at the beginning of the workload..
 * in order to get a large number of concurrent in-flight requests
 */
void batch_put(){
	generate_data(batchsize * io_size, iobuf);

	for (int i=0; i < batchsize; i++){
		ixev_put(keys[i], (void *)(iobuf + (i*io_size)), io_size);
		start_timer(keys[i]);	
		curr_iter++;
	}
}


/* Called only once at the beginning of the workload..
 * in order to get a large number of concurrent in-flight requests
 */
void batch_get(){
	for (int i = 0; i < batchsize; i++){
		ixev_get(keys[i]);
		start_timer(keys[i]);
		curr_iter++;
	}
}

void init_timers(){

	timers = malloc(sizeof(struct timeval *) * max_iter);
	for (int i = 0; i < max_iter; i++){
		timers[i] = malloc(sizeof (struct timeval));
	}
}

void flushandfree_timers(){

	time_t gsecs = glob_timer.tv_sec;
	suseconds_t gusecs = glob_timer.tv_usec;

	if (gettimeofday(&glob_timer, NULL)){
		fprintf(stderr, "Timer issue.\n");
		exit(1);
	}	

	//struct timeval *timer = &(timers[atoi(key)]);
	glob_timer.tv_sec = glob_timer.tv_sec - gsecs;
	glob_timer.tv_usec = glob_timer.tv_usec - gusecs;
	

	FILE *res;

	//TODO uniquely identify results file on every run..
	char fname[20];
	strcpy(fname, "results_");
	sprintf((fname + 8), "%d_%d", batchsize, max_iter/batchsize);
	strcat(fname, ".ix\0");
	if ((res = fopen(fname, "w")) == NULL){
		fprintf(stderr, "Unable to open file %s\n", fname);
		exit(1);
	}

	fprintf(res, "Overall time: %ld:%ld\n", glob_timer.gsecs, glob_timer.gusecs);

	for (int i = 0; i < max_iter; i++){
		fprintf(res, "%d,%ld:%ld\n", i+1, timers[i]->tv_sec, timers[i]->tv_usec);
		free(timers[i]);
	}	 

	fclose(res);
	free(timers);
}

void cleanup(){
	//flush_timers();
	//free(timers);
	flushandfree_timers();
	free_keys();

	if (iotype == WRITE_ONLY || iotype == READ_WRITE)
		free(iobuf);

	printf("DEBUG: clean up complete\n");
}


// callback for ixev_put in READ_ONLY workloads
static void ro_get_handler(char *key, void *data, size_t datalen){
	//TODO: print result...or output to file..
	resp_iter++;
	end_timer(key);
	ixev_get_done(data);

	if (curr_iter < max_iter){
		int i = curr_iter++;
		ixev_get(keys[i]);
		start_timer(keys[i]);
	} else if (resp_iter >= max_iter){
		cleanup();
		exit(0);
	}

}


// callback for ixev_put in WRITE_ONLY workloads
static void wo_put_handler(char *key, void *val){
	
	printf("DEBUG: callback reached for key %s at %p\n", key, key);
	resp_iter++;
	end_timer(key);
	if (curr_iter < max_iter){
		int i = curr_iter++; 
		//printf("DEBUG: issuing put for index %d, curr_iter is %d\n", i, curr_iter);
		ixev_put(keys[i], (void *)(iobuf + ((i % batchsize)*io_size)) , io_size);
		//printf("DEBUG: about to start timer for key %s..\n", keys[i]);
		start_timer(keys[i]);
	} else if (resp_iter >= max_iter){
		printf("DEBUG: end reached on callback for key %s\n", key);
		cleanup();
		exit(0);
	}

	if ((resp_iter % batchsize) == 0){
		generate_data(batchsize * io_size, iobuf);
	}
	printf("DEBUG: reached end of key %s callback no issue\n", key);

}

static void rw_get_handler(char *key, void *data, size_t datalen){

	ixev_get_done(data);
	if (curr_iter < max_iter){



	} else {
		cleanup();
		exit(0);
	}


}

static void rw_put_handler(char *key, void *val){

	if (curr_iter < max_iter){



	} else {
		cleanup();
		exit(0);
	}

}

//dummy
static void delete_handler(char *key){ }

void start_workload(){
	printf("DEBUG: starting workload\n");

	get_keys();

	init_timers();

	if (gettimeofday(&glob_timer, NULL)){ //start global timer..
		fprintf(stderr, "Timer issue.\n");
		exit(1);
	}	

	if (iotype == READ_ONLY){
		batch_get();
	} else if (iotype == WRITE_ONLY || iotype == READ_WRITE){
		iobuf = malloc(batchsize * io_size);
		batch_put();
		generate_data(batchsize*io_size, iobuf);
	}

	while(1)
		ixev_wait();

}


int main(int argc, char *argv[]){
	
	if (argc < 4){
		fprintf(stderr, "USAGE: <IO type> <batch size> <# runs> <io_size>\n");
		exit(1);
	}
	int numruns = 0;

	iotype = atoi(argv[1]);
	batchsize = atoi(argv[2]);
	numruns = atoi(argv[3]);
	io_size = (argc >= 4) ? atoi(argv[4]) : io_size;

	max_iter = numruns * batchsize;

	printf("DEBUG: params are: %i (iotype), %d (batchsize), %d (io_size), %d (max iterations), %s (key file)\n", iotype, batchsize, io_size, max_iter, fname);

	if (iotype == READ_ONLY || iotype == WRITE_ONLY){
		io_ops.get_handler = &ro_get_handler;
		io_ops.put_handler = &wo_put_handler; 
		io_ops.delete_handler = &delete_handler;
	} else if (iotype == READ_WRITE){
		io_ops.get_handler = &rw_get_handler;
		io_ops.put_handler = &rw_put_handler;
		io_ops.delete_handler = &delete_handler;
	} else {
		fprintf(stderr, "Invalid argument, no workload type specified.\n");
		exit(1);
	}

	int ret;
	struct ixev_ctx *ctx;
	ctx = malloc(sizeof(struct ixev_ctx));

	ixev_init(&conn_ops);
	ixev_init_io(&io_ops);
	ixev_ctx_init(ctx); //is this always necessary..? 

	ret = ixev_init_thread();
	if (ret) {
		fprintf(stderr, "unable to init IXEV\n");
		exit(ret);
	}

	start_workload();

	free(ctx);

}
