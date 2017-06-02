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

#define MAX_DATA 50

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


static int datalen;

/* 
 * dummy helper function to add to data 
 * for now, assume datum is null-terminated
 */ 
int append_data(char *buf, const char *datum, int currlen){
	int ret = strlen(datum);
	if (currlen + ret > MAX_DATA){
		fprintf(stderr, "Cannot copy, exceeded max data size\n");
		return 0;

	}
	strncpy((buf + currlen), datum, ret);
	return ret;
}


// TODO for later testing for kernel side event-firing..
static void get_handler(char *key, void *data, size_t len){

	char *val = malloc(len + 1);
	memcpy(val, data, len);
	val[len] = '\0'
	printf("Value read was %s\n", val);
	free(val);
	ixev_get_done(data);
	//assert(len==datalen);
	if (datalen < MAX_DATA && strncmp(key, "testkey", 7) == 0){
		len += append_data(data, "\nmore data..", len);
		ixev_put(key, data, len);
	}
}

static void put_handler(char *key, void *val){

	if (datalen < MAX_DATA && strncmp(key, "testkey", 7) == 0){
		ixev_get(key);
	} else {
		free(val);
	}

}

//dummy for now..
static void delete_handler(char *key){ }

struct ixev_io_ops io_ops = {
	.get_handler = &get_handler,
	.put_handler = &put_handler,
	.delete_handler = &delete_handler,
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
	ixev_put(key, val, datalen);

	//TODO: test delete...?
	//ixev_delete(ctx, key);


	while(1)
		ixev_wait();

	free(ctx);

}
