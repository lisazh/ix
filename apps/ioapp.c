/* 
 * DUMMY "I/O" APP
 * FOR TESTING PURPOSES ONLY
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

/*
struct io_conn {
	struct ixev_ctx ctx
};
*/

// dummy functions for ixev_conn_ops 
static struct ixev_ctx *io_dummyaccept(struct ip_tuple *id)
{
	return NULL;
}

static void io_dummyrelease(struct ixev_ctx *ctx) { }

static void io_dummydialed(struct ixev_ctx *ctx, long ret) { }

struct ixev_conn_ops io_ops = {
	.accept = &io_dummyaccept,
	.release = &io_dummyrelease, 
	.dialed = &io_dummydialed,
};

// TODO for later testing for kernel side event-firing..
static void get_handler(struct ixev_ctx *ctx, int reason){

}

static void put_handler(struct ixev_ctx *ctx, int reason){

}

int main(int argc, char *argv[]){

	int ret;
	struct ixev_ctx *ctx;
	ctx = malloc(sizeof(struct ixev_ctx));

	//Call ixev_init
	ixev_init(&io_ops);
	ixev_ctx_init(ctx); //is this always necessary..? 

	ret = ixev_init_thread();
	if (ret) {
		fprintf(stderr, "unable to init IXEV\n");
		exit(ret);
	}

	char key[] = "testkey";
	char val[] = "test_value";
	size_t len = strlen(val);


	// since these are dummy calls, for now don't need callbacks/handlers
	ixev_put(ctx, key, val, len);
	ixev_get(ctx, key);

	//TODO: test delete...?
	//ixev_delete(ctx, key);


	while(1)
		ixev_wait();

	free(ctx);

}
