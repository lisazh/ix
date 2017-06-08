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


#include <stdio.h>

//#define NVME_AWUNPF 2048 //LTODO: will need to get this from "device" config or whatever
#define DEVBLK_SIZE 1000000 // reasonable approximation of an erase block..
//#define MAX_BATCH (DEVBLK_SIZE/LBA_SIZE) //LTODO move LBA_SIZE def from somewhere else 
#define MAX_BATCH 8 //FOR TESTING FIX LATER
#define SG_MULT 3 //number of sg entries needed per write (LTODO change to 3 eventually)
// LTODO: for each write need 3 entries: meta, data, zeros ()
									 
void io_read_cb(char *key, void *addr, size_t len);
void io_write_cb();



struct ibuf{
	struct sg_entry buf[MAX_BATCH*SG_MULT]; 
	int32_t numblks;
	struct index_ent *currbatch[MAX_BATCH];
	char *usrkeys[MAX_BATCH];
	int32_t currind;
};

static DEFINE_PERCPU(struct ibuf, batch_buf);
static char zerobuf[LBA_SIZE] = {0};

int blkio_init(void) {

	struct ibuf *iobuf = &percpu_get(batch_buf);
	freelist_init(); //LTODO: this involves malloc
	index_init();

	iobuf->numblks = 0;
	iobuf->currind = 0;
	for (int i = 0; i < MAX_BATCH; i++){
		iobuf->currbatch[i] = NULL; //initialize pointers..
		iobuf->usrkeys[i] = NULL;
	}
	return 0;
}

ssize_t bsys_io_read(char *key){
	struct index_ent *ent = get_index_ent(key);
	if (!ent)
		return -1;

	//printf("DEBUG: about to read lba %ld for %lu blocks\n", ent->lba, ent->lba_count);
	
	uint64_t numblks = calc_numblks(ent->val_len);

	//Add this in a timer event
	struct mbuf *buff = dummy_dev_read(ent->lba, numblks);
	void *iomap_addr = mbuf_to_iomap(buff, mbuf_mtod(buff, void *));

	//LTODO: add delays 
	//LTODO: need to package params to callback into one structure..?
	io_read_cb(key, (iomap_addr + META_SZ), ent->val_len);
	return 0;
}

ssize_t bsys_io_write(char *key, void *val, size_t len){

	printf("DEBUG: batching write to key %s at %p with length %ld\n", key, val, len);

	struct ibuf *iobuf = &percpu_get(batch_buf);
	struct index_ent *newdata = new_index_ent(key);
	newdata->val_len = len;
	newdata->crc = crc_data((uint8_t *)val, len);
	
	//printf("DEBUG: new metadata entry at %p with key %s at %p\n", (void *)newdata, newdata->key, (void *) newdata->key);
	//printf("DEBUG: size of metadata structure is %lu\n", sizeof(struct index_ent));
	//printf("DEBUG: size of next pointer is %lu\n", sizeof(struct index_ent *));
	//printf("DEBUG: sanity check metadata entry for key %s and val length %lu\n", newdata->key, newdata->val_len);

	int currind = iobuf->currind;
	iobuf->currbatch[currind] = newdata; //keep for later to allocate blocks 
	iobuf->numblks += calc_numblks(len);
	iobuf->buf[currind*SG_MULT].base = newdata;
	iobuf->buf[currind*SG_MULT].len = META_SZ;

	iobuf->buf[currind*SG_MULT + 1].base = val;
	iobuf->buf[currind*SG_MULT + 1].len = len;

	int numzeros = (len <= DATA_SZ) ? (DATA_SZ - len) : LBA_SIZE - ((len - DATA_SZ) % LBA_SIZE);

	iobuf->buf[currind*SG_MULT + 2].base = zerobuf;
	iobuf->buf[currind*SG_MULT + 2].len = numzeros;
		
	//printf("DEBUG: writing %d (meta) + %d (data) + %d (zeros) bytes\n", META_SZ, len, numzeros);
	//TODO: zeros here ()

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
ssize_t bsys_io_write_flush(){
	struct ibuf *iobuf = &percpu_get(batch_buf);

	if (!iobuf->numblks){ //check if there's anything to write
		printf("DEBUG: no batched writes to issue\n");
		return 0;
	}
	//debugprint_sg();
	int startlba = get_blk(iobuf->numblks);
	//uint64_t ret = iobuf->numblks;

	//update lba values..
	for (int i = 0; i < iobuf->currind; i++){
		if ((iobuf->currbatch[i])->key != NULL){		
			if (i == 0){
				(iobuf->currbatch[i])->lba = startlba;
			} else {
				(iobuf->currbatch[i])->lba = startlba + calc_numblks((iobuf->currbatch[i-1])->val_len);
			}
		}
	}
	dummy_dev_writev(iobuf->buf, (iobuf->currind)*SG_MULT, startlba, iobuf->numblks);
	printf("DEBUG: Wrote %d entries starting at %d for %d blocks\n", iobuf->currind, startlba, iobuf->numblks);
	//LTODO: add delays..
	io_write_cb();

	return 0;
}

/* Indicates read is completed and buffer can be reclaimed
 *
 */ 
ssize_t bsys_io_read_done(void *addr)
{
	//printf("DEBUG: read done at address %p\n", addr);
	void *kaddr = iomap_to_mbuf(&percpu_get(mbuf_mempool), addr);
	dummy_dev_read_done(kaddr);
	return 0;
}

/* Callback for write completion
 * Updates in-memory indexes 
 * LTODO: what to pass in?
 */
void io_write_cb(){

	struct ibuf *iobuf = &percpu_get(batch_buf);
	//TODO: check status/error codes on completion..? or just assume always returns ok

	// update metadata & free all interim metadata

	uint64_t ind = iobuf->currind;
	//printf("DEBUG: updating %ld index entries\n", ind);

	//struct index_ent *meta = insert_key(key);
	for (int i = 0; i < ind; i++){
		update_index(iobuf->currbatch[i]);
		void *uaddr = iobuf->buf[i*SG_MULT + 1].base;
		usys_io_wrote(iobuf->usrkeys[i], uaddr);

		printf("DEBUG: updated index entry for %s\n", iobuf->currbatch[i]->key);
		iobuf->currbatch[i] = NULL; //update the pointer

		//update sg_entries
		iobuf->buf[i*SG_MULT].base = NULL;
		iobuf->buf[i*SG_MULT].len = 0;
		iobuf->buf[i*SG_MULT + 1].base = NULL;		
		iobuf->buf[i*SG_MULT + 1].len = 0;
	}
	
	//reset variables
	iobuf->currind = 0;
	iobuf->numblks = 0;
	
}

// TODO: unpack key, address and length from param
void io_read_cb(char *key, void *addr, size_t len){
	usys_io_read(key, addr, len);
}

	
