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
#include <unistd.h>

#include <ixev.h>

//GLOBALS
struct ixev_io_ops io_ops;
char *iobuf;
char *keys[MAX_KEYS];
int curr_run;

//input params
char *iotype = "";
int batchsize = 1; //default val?
int num_runs = 0; 
int io_size = 0; //writes only
char *fname = "";


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
 * 
 */
static void generate_data(size_t datasize, char *buf){

	srand(time(0));
	int datum; 
	while (datasize){
		datum = rand();
		buf = datum;
		datasize -= sizeof(datum);
		buf += sizeof(datum);
	}
}


static void get_keys(){
	fopen(fname);

	for (int i = 0; i < num_runs*batchsize; i++){
		fgets()
	}

}

static void ro_get_handler(char *key, void *data, size_t datalen){

	ixev_get_done(data);

	ixev_get();
}

static void wo_put_handler(char *key, void *val){

	//get next data, key


	if (curr_run < num_runs){
		batch_put();
		curr_run++;
	} else {
		free(iobuf);
		exit(0);
	}

}

static void rw_get_handler(char *key, void *data, size_t datalen){

	ixev_get_done(data);

}

static rw_put_handler(char *key, void *val){

}

//dummy for now..
static void delete_handler(char *key){ }

void batch_put(){
	generate_data(batchsize * iosize, iobuf);

	for (int i=0; i < batchsize; i++){
		ixev_put(key[i*curr_run], (void *)(iobuf + (i*io_size)), io_size);
	}
}

void batch_get(){
	for (int i=1; i <= batchsize; i++){
		ixev_get(key[(i*curr_run - 1)]);
	}
}

void start_workload(){

	curr_run = 1;

	//to simplify things, every workload reads keys from a file
	get_keys();

	if (strcmp(iotype, "ro")){
		batch_get();

	} else if (strcmp(iotype, "wo") || strcmp(iotype, "rw")){
		assert(iosize > 0);
		iobuf = malloc(batchsize * iosize); //only allocate enough for one batch's data
		
		batch_put();
	}

	while(1)
		ixev_wait();

}


int main(int argc, char *argv[]){

	if (argc < 2){
		fprintf(stderror, )
	}

	while ((opt = getopt(argc, argv, "tbn:sf::")) != -1){
		switch(opt)
		{
			case 't':
				iotype = optarg;
				break;
			case 'b':
				batchsize = optarg;
				break;
			case 'n':
				numruns = optarg;
				break;
			case 's':
				io_size = optarg;
				break;
			case 'f':
				fname = optarg;
				break;

			default:
				printf("USAGE: [-t IO type] [-b batch size] [-n num_runs] [-s IO size] [-f filename]\n"); //batch size? also how to bound length of execution? time bound or IOPS
				return -1;
		}

	}

	if (strcmp(iotype, "ro") || strncmp(iotype, "wo", 2)){ //can set arbitrary handlers for other op b/c will never get called..
		io_ops.get_handler = &ro_get_handler;
		io_ops.put_handler = &wo_put_handler; 
		io_ops.delete_handler = &delete_handler;

	} else if (strcmp(iotype, "rw")){ //readwrite
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
