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

#define MAX_DATA 108

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



char key1[] = "testkey1";
char key2[] = "testkeya";
char val1[] = "datadatadata";
char *val2;
static int len = 12;
static int len2 = 108;
static char *val;
static int numruns;
static int run = 0;
static int maxruns = 1000000;
struct timeval timer1;
struct timeval timer2;

/* 
 * dummy helper function to add to data 
 * for now, assume datum is null-terminated
 */ 
int append_data(char *buf, const char *datum, int currlen){
	
	printf("DEBUG: current data length is %d\n", currlen);
	int ret = strlen(datum);
	strncpy((buf + currlen), datum, ret);
	return ret;
}


// TODO for later testing for kernel side event-firing..
static void get_handler(char *key, void *data, size_t datalen){
	ixev_get_done(data);
	//assert(len==datalen);
	if (len < (MAX_DATA/2) && strncmp(key, "testkey1", 8) == 0){
		len += append_data(val, " & more data", datalen);
		ixev_put(key, val, len);
	} else if (len < MAX_DATA && strncmp(key, "testkey1", 8) == 0){
		len += append_data(val, " & more data", datalen);
		ixev_put(key1, val, len);
		ixev_put(key2, val, len/2);
	} else {
		return;
	}

}

static void put_handler(char *key, void *val){

/*
	if (len < MAX_DATA){
		ixev_get(key);
	} else if (strncmp(key, "testkeya", 8) == 0){
		printf("DEBUG: finished writing, exiting..\n");
		free(val);
	}

	if (run < maxruns){
		ixev_put(key1, val2, len2);
		run++;
	} else {
		//free(val);
		gettimeofday(&timer2, NULL);
		printf("DEBUG: finished in %ld seconds, %ld microseconds\n",(timer2.tv_sec - timer1.tv_sec), (timer2.tv_usec - timer1.tv_usec));
	
	}
*/
	//printf("DEBUG: put handler\n");
	ixev_put(key1, val2, len2);
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

	
	//numruns = atoi(argv[1]);
	//Call ixev_init
	ixev_init(&conn_ops);
	ixev_init_io(&io_ops);
	ixev_ctx_init(ctx); //is this always necessary..? 

	ret = ixev_init_thread();
	if (ret) {
		fprintf(stderr, "unable to init IXEV\n");
		exit(ret);
	}

	val2 = malloc(MAX_DATA);
	for (int i = 0; i < 9; i++){
		strncpy((val2 + i*12), val1, 12);
	}
	printf("DEBUG: data to insert is %s\n", val2);

	//len = 0;
	
	//len += append_data(val, "data data data", len);

	gettimeofday(&timer1, NULL);
	// since these are dummy calls, for now don't need callbacks/handlers
	ixev_put(key1, val2, len2);
	//ixev_get(key1);

	//TODO: test delete...?
	//ixev_delete(ctx, key);


	while(1)
		ixev_wait();

	free(ctx);

}
