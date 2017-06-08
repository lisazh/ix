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

static void ro_get_handler(char *key, void *data, size_t datalen){

}

static void wo_put_handler(char *key, void *val){

}

static void rw_get_handler(char *key, void *data, size_t datalen){
	//call put

}

static rw_put_handler(char *key, void *val){
	//call get

}

//dummy for now..
static void delete_handler(char *key){ }

struct ixev_io_ops io_ops;

void process_workload(char *type, int size){

	if (strcmp(type, "ro") || strcmp(type, "rw")){ //start w/ get
		ixev_get();

	} else if (strcmp(type, "wo")){
		ixev_put();
	}

	while(1)
		ixev_wait();

}


int main(int argc, char *argv[]){

	if (argc < 3){
		printf("USAGE: <IO type> <IO size>\n"); //batch size? also how to bound length of execution? time bound or IOPS
		return -1;
	}

	if (strcmp(argv[1], "ro") || strncmp(argv[1], "wo", 2)){ //can set arbitrary handlers for other op b/c will never get called..
		io_ops.get_handler = &ro_get_handler;
		io_ops.put_handler = &wo_put_handler; 
		io_ops.delete_handler = &delete_handler;

	} else if (strcmp(argv[1], "rw")){ //readwrite
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

	process_workload(argv[1], atoi(argv[2]));

	free(ctx);

}
