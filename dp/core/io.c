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
#define MAX_BATCH (DEVBLK_SIZE/LBA_SIZE) //LTODO move LBA_SIZE def from somewhere else 
#define SG_MULT 2 //number of sg entries needed per write (LTODO change to 3 eventually)
// LTODO: for each write need 3 entries: meta, data, zeros ()
									 

struct ibuf{
	struct sg_entry buf[MAX_BATCH*SG_MULT]; 
	int32_t numblks;
	struct index_ent *currbatch[MAX_BATCH];
	int32_t currind; 
	
};

static struct ibuf *iobuf; //LTODO: initialize somewhere...

int blkio_init(void) {

	freelist_init(); //LTODO: this involves malloc
	index_init();

	iobuf = malloc(sizeof(struct ibuf)); //LTODO: use mempools
	iobuf->numblks = 0;
	iobuf->currind = 0;
	for (int i = 0; i < MAX_BATCH; i++)
		iobuf->currbatch[i] = NULL; //initialize pointers..
	return 0;
}

ssize_t bsys_io_read(char *key){
	//FIXME: map key to LBAs
	struct index_ent *ent = get_key_to_lba(key);
	if (!ent)
		return -1;

	printf("DEBUG: about to read lba %d for %u blocks\n", ent->lba, ent->lba_count);
	
	//Add this in a timer event
	struct mbuf *buff = dummy_dev_read(ent->lba, ent->lba_count);
	void * iomap_addr = mbuf_to_iomap(buff, mbuf_mtod(buff, void *));
	usys_io_read(key, iomap_addr, buff->len);
	return 0;
}

ssize_t bsys_io_write(char *key, void *val, size_t len){
	//FIXME: decide where to write
	
	//struct index_ent *meta = insert_key(key, len);
	struct index_ent *newdata = malloc(sizeof(struct index_ent)); //LTODO: replace with appropriate mem mgmt
	newdata->key = malloc(strlen(key) + 1);
	strncpy(newdata->key, key, strlen(key));
	newdata->key[strlen(key)] = '\0';
	newdata->lba_count = calc_numblks(len);
	newdata->crc = crc_data((uint8_t *)val, len);

	int currind = iobuf->currind;
	iobuf->currbatch[iobuf->currind++] = newdata; //keep for later to allocate blocks 
	iobuf->numblks += newdata->lba_count;
	iobuf->buf[currind*SG_MULT].base = newdata;
	iobuf->buf[currind*SG_MULT].len = META_SZ;

	iobuf->buf[currind*SG_MULT + 1].base = val;
	iobuf->buf[currind*SG_MULT + 1].len = len;
	
	/*
	if (iobuf->numblks >= MAX_BATCH){ 
		bsys_io_write_flush();
	}
	*/

	return 0;
}


//flush batched writes to device..
ssize_t bsys_io_write_flush(){

	printf("DEBUG: about to issue writes\n");
	uint64_t startlba = get_blk(iobuf->numblks);
	//uint64_t ret = iobuf->numblks;

	//update lba values..
	for (int i = 0; i < iobuf->currind; i++){
		if ((iobuf->currbatch[i])->key != NULL){		
			if (i == 0){
				(iobuf->currbatch[i])->lba = startlba;
			} else {
				(iobuf->currbatch[i])->lba = startlba + (iobuf->currbatch[i-1])->lba_count;
			}
		}
	}

	//LTODO: issue the write to device...
	dummy_dev_writev(iobuf->buf, (iobuf->currind)*SG_MULT, startlba, iobuf->numblks);

	//LTODO: add delays..
	io_write_cb();

	return 0;
}

/* Indicates read is completed and buffer can be reclaimed
 *
 */ 
ssize_t bsys_io_read_done(void *addr)
{
	void *kaddr = iomap_to_mbuf(&percpu_get(mbuf_mempool), addr);
	dummy_dev_read_done(kaddr);
	return 0;
}

/* Callback for write completion
 * Updates in-memory indexes 
 * LTODO: what to pass in?
 */
void io_write_cb(){

	//TODO: check status/error codes on completion..? or just assume always returns ok

	// update metadata & free all interim metadata

	uint64_t ind = iobuf->currind;

	//struct index_ent *meta = insert_key(key);
	for (int i = 0; i < ind; i++){
		update_index(iobuf->currbatch[i]);
		usys_io_wrote(iobuf->currbatch[i]->key);

		printf("DEBUG: updated index entry for %s\n", iobuf->currbatch[i]->key);
		iobuf->currbatch[i] = NULL; //update the pointer
	}

	//reset variables
	iobuf->currind = 0;
	iobuf->numblks = 0;

}

// TODO: unpack key, address and length from param
void io_read_cb(){
	 //usys_io_read();

}

	
