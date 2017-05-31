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

#define MAX_DATA 50;

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


int datalen;

/* 
 * dummy helper function to add to data 
 * for now, assume datum is null-terminated
 */ 
int append_data(char *buf, const char *datum, int currlen){
	int ret = strlen(datum);
	if (currlen + ret > MAX_DATA){
		fprintf(stderr, "Cannot copy, exceeded max data size");
		return 0;

	}
	strncpy((buf + currlen), datum, ret);
	return ret;
}


// TODO for later testing for kernel side event-firing..
static void get_handler(char *key, void *data, ssize_t len){
	ixev_get_done(data);
	if (len < MAX_DATA && strncmp(key, "testkey", 7) == 0){
		len += append_data(data, "\nmore data..", len);
		ixev_put(key, data, len);
	}
}

static void put_handler(char *key, void *val){

	if (len < MAX_DATA && strncmp(key, "testkey", 7) == 0){
		ixev_get(key);
	} else {
		free(val);
	}

}

struct ixev_io_ops io_ops = {
	.get_handler = &get_handler;
	.put_handler = &put_handler;
	.delete_handler = &delete_handler;
};


int main(int argc, char *argv[]){

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

	char key[] = "testkey";
	char *val = malloc(MAX_DATA);

	datalen = 0;
	datalen += append_data(val, "data", datalen);

	// since these are dummy calls, for now don't need callbacks/handlers
	ixev_put(ctx, key, val, datalen);

	//TODO: test delete...?
	//ixev_delete(ctx, key);


	while(1)
		ixev_wait();

	free(ctx);

}
