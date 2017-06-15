/* 
 * Parametrizable I/O "Benchmark"
 * 
 */ 

// copy & pasted not sure if need all of them...
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <ixev.h>

#define MAX_KEYS 1000000
#define MAX_KEY_LEN 10 //overwrite our own value for benchmarking..
 						// can test longer keys but whatever..
#define DEF_IO_SIZE 128
//GLOBALS
struct ixev_io_ops io_ops;
char *iobuf;
char *keys[MAX_KEYS];
//int curr_run;
int curr_iter; //within each batch...

//input params -- default values for all
char iotype[3] = "ro\0";
int batchsize = 1; //default val?
//int num_runs = 0; 
int io_size = DEF_IO_SIZE; //writes only, use default value
char fname[100] = "keys.ix";
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



//static void start_timer(){}
//static void end_timer(){}

/* Generates dummy data according to size 
 * super dumb version for now...improve later.
 */
void generate_data(size_t datasize, char *buf){

	srand(time(0));
	int datum; 
	while (datasize){
		datum = rand();
		*((int *)buf) = datum;
		datasize -= sizeof(datum);
		buf += sizeof(datum);
	}
}

void get_keys(){
	
	if (fname == NULL){
		fprintf(stderr, "No file specified\n");
		exit(1);

	}
	FILE *fkeys; 
	
	fkeys = fopen(fname, "r");

	for (int i = 0; i < MAX_KEYS; i++){
		keys[i] = malloc(MAX_KEY_LEN);
	}

	for (int i = 0; i < max_iter; i++){
		if (fgets(keys[i], MAX_KEY_LEN, fkeys) == NULL){
			fprintf(stderr, "Unable to read key from %s\n", fname);
			fclose(fkeys);
			exit(1);
		}
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
	for (int i = 0; i < MAX_KEYS; i++){
		free(keys[i]);
	}
}

void batch_put(){
	generate_data(batchsize * io_size, iobuf);

	for (int i=0; i < batchsize; i++){
		ixev_put(keys[i], (void *)(iobuf + (i*io_size)), io_size);
	}
}

void batch_get(){
	for (int i=1; i <= batchsize; i++){
		ixev_get(keys[i]);
	}
}


static void ro_get_handler(char *key, void *data, size_t datalen){
	//TODO: print result...or output to file..
	ixev_get_done(data);

	if (curr_iter < max_iter){
		//batch_get();
		ixev_get(keys[curr_iter++]);
	} else {
		free(iobuf);
		free_keys();
		exit(0);
	}

}

static void wo_put_handler(char *key, void *val){

	if (curr_iter < max_iter){
		//batch_put();
		int i = curr_iter++;
		ixev_put(keys[i], (void *)(iobuf + (i*io_size)) , io_size);
	} else {
		free(iobuf);
		free_keys();
		exit(0);
	}
	if ((curr_iter % batchsize) == 0){
		generate_data(batchsize * io_size, iobuf);
	
	}

}

static void rw_get_handler(char *key, void *data, size_t datalen){

	ixev_get_done(data);

}

static void rw_put_handler(char *key, void *val){

}

//dummy for now..
static void delete_handler(char *key){ }

void start_workload(){
	printf("DEBUG: starting workload\n");
	//to simplify things, every workload reads keys from a file
	get_keys();
	
	if (strcmp(iotype, "ro") == 0){
		printf("DEBUG: read-only mode\n");
		batch_get();

	} else if (strcmp(iotype, "wo") == 0 || strcmp(iotype, "rw") == 0){
		printf("DEBUG: iotype is %s\n", iotype);
		iobuf = malloc(batchsize * io_size); //only allocate enough for one batch of data
		batch_put();
		generate_data(batchsize*io_size, iobuf); //next batch of data for writes..
	}

	curr_iter += batchsize;

	while(1)
		ixev_wait();

}


int main(int argc, char *argv[]){
	
	/*
	if (argc < 2){
		fprintf(stderr, "USAGE: [-t IO type] [-b batch size] [-n # runs] [-s IO size] [-f filename]\n");
		exit(1);
	}
	int opt, numruns = 0;
	*/
	//strncpy(iotype, argv[1], 2);
	//batchsize = atoi(argv[2]);
	//strncpy(fname, argv[3], strlen(argv[3]));
		
	/*
	while ((opt = getopt(argc, argv, "tbnf:s::")) != -1){
		switch(opt)
		{
			case 't':
				iotype = optarg;
				break;
			case 'b':
				batchsize = atoi(optarg);
				break;
			case 'n':
				numruns  = atoi(optarg);
				break;
			case 's':
				io_size = atoi(optarg);
				break;
			case 'f':
				fname = optarg;
				break;

			default:
				printf("USAGE: [-t IO type] [-b batch size] [-n num_runs] [-s IO size] [-f filename]\n"); //batch size? also how to bound length of execution? time bound or IOPS
				exit(1);
		}

	}
	*/

	//max_iter = numruns * batchsize;

	printf("DEBUG: default values are: %s (iotype), %d (batchsize), %d (io_size), %d (max iterations), %s (key file)\n", iotype, batchsize, io_size, max_iter, fname);	

	if (strcmp(iotype, "ro") == 0 || strncmp(iotype, "wo", 2) == 0){ //can set arbitrary handlers for other op b/c will never get called..
		io_ops.get_handler = &ro_get_handler;
		io_ops.put_handler = &wo_put_handler; 
		io_ops.delete_handler = &delete_handler;

	} else if (strcmp(iotype, "rw") == 0){ //readwrite
		io_ops.get_handler = &rw_get_handler;
		io_ops.put_handler = &rw_put_handler;
		io_ops.delete_handler = &delete_handler;

	} else {
		fprintf(stderr, "Invalid argument, no workload type specified.\n");
		return -1;
	}

	int ret;
	struct ixev_ctx *ctx;
	ctx = malloc(sizeof(struct ixev_ctx));

	//Call ixev_init
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
