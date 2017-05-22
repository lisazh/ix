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

#define NVME_AWUNPF 2048 //TODO: will need to get this from "device" config or whatever
#define MAX_BATCH (NVME_AWUNPF/LBA_SIZE) //TODO move LBA_SIZE def from somewhere else 

struct ibuf{
	void *buf;// mempools? mempool datastore? use smthg figure out init
				// also make sure this is properly zero'd when init'ing
	int32_t numblks;
	struct index_ent currbatch[MAX_BATCH];
	int32_t currind; 
	
}

static struct ibuf iobuf;

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
	
	// do index magic..

	struct index_ent *meta = insert_key(key, len);

	currbatch[currind++] = meta; //keep for later to allocate blocks 
	//TODO: checksum?

	//dummy_dev_write(val, 1, 1);
	
	if (iobuf->numblks > MAX_BATCH){ 
		bsys_io_write_flush();
	}

//!!TODO pointer math to find offset in buf to write to....
	memcpy(&(iobuf->buf[LBA_SIZE*iobuf->numblks]), meta, sizeof(struct index_ent)); 
	memcpy(&(iobuf->buf[LBA_SIZE*iobuf->numblks + sizeof(struct index_ent)]), val, len); 

	iobuf->numblks += meta->lba_count;
	usys_io_wrote(key);

	return 0;
}


//flush batched writes to device..
ssize_t bsys_io_write_flush(){

	//update metadata
	uint64_t startlba = get_blk(iobuf->numblks);
	for (int i = 0; i < MAX_BATCH; i++){
		if (currbatch[i]){		
			if (i == 0){
				currbatch[i]->lba = startlba;
			} else {
				currbatch[i]->lba = startlba + currbatch[i-1]->lba_count;
			}
		}
	}

	//TODO: issue the write to device...


	iobuf->numblks = 0;
	iobuf->keyind = 0;
}

ssize_t bsys_io_read_done(void *addr)
{
	void *kaddr = iomap_to_mbuf(&percpu_get(mbuf_mempool), addr);
	dummy_dev_read_done(kaddr);
	return 0;
}	
