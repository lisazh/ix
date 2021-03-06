/*
 * io_api.c - plumbing between the IO and userspace
 */

#include <assert.h>
#include <ix/stddef.h>
#include <ix/errno.h>
#include <ix/syscall.h>
#include <ix/log.h>
#include <ix/uaccess.h>
#include <ix/ethdev.h>
#include <ix/kstats.h>
#include <ix/cfg.h>
#include <ix/dummy_dev.h>
#include <ix/blk.h>
#include <ix/mempool.h>

#include <stdio.h>
#include <sys/time.h>



//#define NVME_AWUNPF 2048 //LTODO: will need to get this from "device" config or whatever
//#define DEVBLK_SIZE 1000000 // reasonable approximation of an erase block..
//#define MAX_BATCH (DEVBLK_SIZE/LBA_SIZE) //LTODO move LBA_SIZE def from somewhere else 
#define MAX_BATCH 100000 //FOR TESTING FIX LATER
#define SG_MULT 3 //number of sg entries needed per write (LTODO change to 3 eventually)
// LTODO: for each write need 3 entries: meta, data, zeros ()

#define READ_BATCH 8 // number of buffers per read - 2KB per buffer
#define MAX_PENDING_REQ 100000

void io_read_cb(void *arg);
void io_write_cb(void *unused);

struct ibuf{
	struct sg_entry buf[MAX_BATCH*SG_MULT]; 
	lbasz_t numblks;
	struct index_ent *currbatch[MAX_BATCH];
	char *usrkeys[MAX_BATCH];
	int32_t currind;
};

struct pending_req {
	char *key;
	size_t len;
	struct sg_entry ents[READ_BATCH];
};

static DEFINE_PERCPU(struct ibuf, batch_buf);
static DEFINE_PERCPU(struct mempool, pending_req_mempool);
static struct mempool_datastore pending_req_datastore;
static char zerobuf[LBA_SIZE] = {0};

struct timeval timer; //for debugging...

int blkio_init(void)
{
	
	//printf("DEBUG: initializing block stuff..\n");
	int ret;
	ret = mempool_create_datastore(&pending_req_datastore, 32*MAX_PENDING_REQ,
				       sizeof(struct pending_req), 0, MEMPOOL_DEFAULT_CHUNKSIZE, "pending_req");
	return ret;
}

int blkio_init_cpu(void)
{
	//printf("DEBUG: initializing per cpu block stuff..\n");
	struct ibuf *iobuf = &percpu_get(batch_buf);
	freelist_init(); //LTODO: this involves malloc
	index_init();

	iobuf->numblks = 0;
	iobuf->currind = 0;

	for (int i = 0; i < MAX_BATCH; i++){
		iobuf->currbatch[i] = NULL; //initialize pointers, for safety..
		iobuf->usrkeys[i] = NULL;
	}
	int ret;
	ret = mempool_create(&percpu_get(pending_req_mempool), 
			&pending_req_datastore, MEMPOOL_SANITY_PERCPU, percpu_get(cpu_id));
	if (ret)
		return ret;

	return 0;
}

ssize_t bsys_io_read(char *key){

	gettimeofday(&timer, NULL);
	//printf("DEBUG: entering read syscall handler at %ld microseconds\n", timer.tv_usec);

	struct pending_req *pr;
	struct index_ent *ent = get_index_ent(key);
	if (!ent)
		return -1;

	//printf("DEBUG: about to read lba %ld for %lu blocks\n", ent->lba, ent->lba_count);
	
	uint64_t numblks = CALC_NUMBLKS(ent->val_len);

	// Allocate buffer for read
	assert(numblks <= MBUF_DATA_LEN / LBA_SIZE);
	struct mbuf *read_mbuf = mbuf_alloc_local();
	char *data = mbuf_mtod(read_mbuf, char *);
	read_mbuf->len = numblks * LBA_SIZE;

	pr = mempool_alloc(&percpu_get(pending_req_mempool));
	pr->key = key;
	pr->len = ent->val_len;
	pr->ents[0].base = mbuf_to_iomap(read_mbuf, mbuf_mtod_off(read_mbuf, void *, META_SZ));

	gettimeofday(&timer, NULL);
	//printf("DEBUG: about to issue \"device\" read at %ld microseconds\n", timer.tv_usec);

	dummy_dev_read(data, ent->lba, numblks, io_read_cb, pr);

	gettimeofday(&timer, NULL);
	//printf("DEBUG: finished issuing device call at %ld microseconds\n", timer.tv_usec);

	return 0;
}

ssize_t bsys_io_write(char *key, void *val, size_t len){

	//printf("DEBUG: batching write to key %s at %p with length %ld\n", key, val, len);

	struct ibuf *iobuf = &percpu_get(batch_buf);
	KSTATS_PUSH(indx_add, NULL);
	struct index_ent *newdata = new_index_ent(key, val, len);
	KSTATS_POP(NULL);	

	assert(newdata->magic == METAMAGIC);
	
	//printf("DEBUG: new metadata entry at %p with key %s at %p\n", (void *)newdata, newdata->key, (void *) newdata->key);
	//printf("DEBUG: size of metadata structure is %lu\n", sizeof(struct index_ent));
	//printf("DEBUG: size of next pointer is %lu\n", sizeof(struct index_ent *));
	//printf("DEBUG: sanity check metadata entry for key %s and val length %lu\n", newdata->key, newdata->val_len);

	int currind = iobuf->currind;
	iobuf->currbatch[currind] = newdata; //keep for later to allocate blocks 
	int blks = CALC_NUMBLKS(len);
	//printf("DEBUG: adding %d blocks\n", blks);
	iobuf->numblks = iobuf->numblks + blks;
	//printf("DEBUG: number of blocks in batch is now %d\n", iobuf->numblks);
	//printf("DEBUG: function - %u ; macro - %u\n", calc_numblks(len), CALC_NUMBLKS(len));

	iobuf->buf[currind*SG_MULT].base = newdata;
	iobuf->buf[currind*SG_MULT].len = META_SZ;

	iobuf->buf[currind*SG_MULT + 1].base = val;
	iobuf->buf[currind*SG_MULT + 1].len = len;

	int numzeros = (len <= DATA_SZ) ? (DATA_SZ - len) : LBA_SIZE - ((len - DATA_SZ) % LBA_SIZE);

	//note that there might not be any zeros but for continuity/simplicity include this entry in the scatter gather list anyway..
	iobuf->buf[currind*SG_MULT + 2].base = zerobuf;
	iobuf->buf[currind*SG_MULT + 2].len = numzeros;
		
	//printf("DEBUG: writing %d (meta) + %d (data) + %d (zeros) bytes\n", META_SZ, len, numzeros);
	
	iobuf->usrkeys[currind] = key;
	iobuf->currind++;

	/*
	if (iobuf->numblks >= MAX_BATCH){ 
		bsys_io_write_flush();
	}
	*/

	return 0;
}

void debugprint_sg(){
	struct ibuf *iobuf = &percpu_get(batch_buf);
	for (int i = 0; i < iobuf->currind; i++){

		printf("DEBUG: metadata at %p\n", iobuf->currbatch[i]);
		printf("DEBUG: SG entry metadata at %p with length %ld\n", iobuf->buf[i*SG_MULT].base, iobuf->buf[i*SG_MULT].len);	
		printf("DEBUG: SG entry data at %p with length %ld\n", iobuf->buf[i*SG_MULT + 1].base, iobuf->buf[i*SG_MULT + 1].len);

	}
}

// flush batched writes to device..
// TODO: return value..? what should it be..
ssize_t bsys_io_write_flush()
{
	struct ibuf *iobuf = &percpu_get(batch_buf);

	if (!iobuf->numblks){ //check if there's anything to write
		//printf("DEBUG: no batched writes to issue\n");
		return 0;
	}
	//debugprint_sg();
	KSTATS_PUSH(indx_alloc, NULL);
	int startlba = get_blk(iobuf->numblks); //allocate entire batch in the index
	KSTATS_POP(NULL);
	//uint64_t ret = iobuf->numblks;
	
	//update lba values..
	for (int i = 0; i < iobuf->currind; i++){
		if ((iobuf->currbatch[i])->key != NULL){		
			if (i == 0){
				(iobuf->currbatch[i])->lba = startlba;
			} else {
				int blks = CALC_NUMBLKS((iobuf->currbatch[i-1])->val_len);
				(iobuf->currbatch[i])->lba = startlba + blks;
				startlba += blks;
			}
		}
	}

	//gettimeofday(&timer, NULL);
	//printf("DEBUG: about to issue \"device\" write at %ld microseconds\n", timer.tv_usec);

	KSTATS_PUSH(dev_write, NULL);
	dummy_dev_writev(iobuf->buf, (iobuf->currind)*SG_MULT, startlba,
			iobuf->numblks, io_write_cb, NULL);
	KSTATS_POP(NULL);
	//gettimeofday(&timer, NULL);
	//printf("DEBUG: issued write at %ld microseconds\n", timer.tv_usec);

	//printf("DEBUG: Wrote %d entries starting at %d for %d blocks\n", iobuf->currind, startlba, iobuf->numblks);
	
	//reset variables...
	return 0;
}

/* Indicates read is completed and buffer can be reclaimed
 *
 */ 
ssize_t bsys_io_read_done(void *addr)
{
	//printf("DEBUG: read done at address %p\n", addr);
	void *kaddr = iomap_to_mbuf(&percpu_get(mbuf_mempool), addr);
	struct mbuf *b = (struct mbuf *)((uintptr_t) kaddr - MBUF_HEADER_LEN);
	mbuf_free(b);
	return 0;
}

/* Callback for write completion
 * Updates in-memory indexes 
 * LTODO: what to pass in?
 */
void io_write_cb(void *unused){
	
	//gettimeofday(&timer, NULL);
	//printf("DEBUG: reached write callback at %ld microseconds\n", timer.tv_usec);

	struct ibuf *iobuf = &percpu_get(batch_buf);
	//TODO: check status/error codes on completion..? or just assume always returns ok

	// update metadata & free all interim metadata

	uint64_t ind = iobuf->currind;
	//printf("DEBUG: updating %ld index entries\n", ind);

	//struct index_ent *meta = insert_key(key);
	for (int i = 0; i < ind; i++){
		KSTATS_PUSH(indx_upd, NULL);
		update_index(iobuf->currbatch[i]);
		KSTATS_POP(NULL);
		void *uaddr = iobuf->buf[i*SG_MULT + 1].base;
		usys_io_wrote(iobuf->usrkeys[i], uaddr);

		//printf("DEBUG: updated index entry for %s at %p\n", iobuf->usrkeys[i], uaddr);
		iobuf->currbatch[i] = NULL; //update the pointer
		iobuf->usrkeys[i] = NULL;

		//update sg_entries
		iobuf->buf[i*SG_MULT].base = NULL;
		iobuf->buf[i*SG_MULT].len = 0;
		iobuf->buf[i*SG_MULT + 1].base = NULL;		
		iobuf->buf[i*SG_MULT + 1].len = 0;
		iobuf->buf[i*SG_MULT + 2].base = NULL;
		iobuf->buf[i*SG_MULT + 2].len = 0;
	}
	
	//reset variables
	iobuf->currind = 0;
	iobuf->numblks = 0;
}

// TODO: unpack key, address and length from param
void io_read_cb(void *arg)
{
	gettimeofday(&timer, NULL);
	//printf("DEBUG: reached read callback at %ld microseconds\n", timer.tv_usec);

	struct pending_req *rq = (struct pending_req *)arg;

	usys_io_read(rq->key, rq->ents[0].base, rq->len); //FIXME what if val longer..? 
	mempool_free(&percpu_get(pending_req_mempool), rq);
}
