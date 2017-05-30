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


//#include <stdio.h>

#define NVME_AWUNPF 2048 //LTODO: will need to get this from "device" config or whatever
#define MAX_BATCH (NVME_AWUNPF/LBA_SIZE) //LTODO move LBA_SIZE def from somewhere else 

struct ibuf{
	void *buf;// mempools? mempool datastore? use smthg figure out init
				// also make sure this is properly zero'd when init'ing
	int32_t numblks;
	struct index_ent *currbatch[MAX_BATCH];
	int32_t currind; 
	
};

static struct ibuf *iobuf; //LTODO: initialize somewhere...

int blkio_init(void) {
	return 0;
}

ssize_t bsys_io_read(char *key){
	//FIXME: map key to LBAs
	struct index_ent *ent = get_key_to_lba(key);
	
	//Add this in a timer event
	struct mbuf *buff = dummy_dev_read(1, 1);
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

	iobuf->currbatch[iobuf->currind++] = &newdata; //keep for later to allocate blocks 

	//dummy_dev_write(val, 1, 1);
	
	/*
	if (iobuf->numblks > MAX_BATCH){ 
		bsys_io_write_flush();
	}
	*/

	//LTODO: replace with appropriate bufferin gmechanism
	//memcpy(&(iobuf->buf[LBA_SIZE*iobuf->numblks]), meta, sizeof(struct index_ent)); 
	//memcpy(&(iobuf->buf[LBA_SIZE*iobuf->numblks + sizeof(struct index_ent)]), val, len); 

	iobuf->numblks += newdata->lba_count;

	return 0;
}


//flush batched writes to device..
ssize_t bsys_io_write_flush(){

	uint64_t startlba = get_blk(iobuf->numblks);
	uint64_t ret = iobuf->numblks;

	//update lba values..
	for (int i = 0; i < MAX_BATCH; i++){
		if ((iobuf->currbatch[i])->key != NULL){		
			if (i == 0){
				(iobuf->currbatch[i])->lba = startlba;
			} else {
				(iobuf->currbatch[i])->lba = startlba + (iobuf->currbatch[i-1])->lba_count;
			}
		}
	}

	//LTODO: issue the write to device...
	return ret;
}

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
	
