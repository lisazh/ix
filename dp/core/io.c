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

#include <stdio.h>



ssize_t bsys_io_read(char *key){
	//FIXME: map key to LBAs
	struct mbuf *buff = dummy_dev_read(1, 1);
	
	//Add this in a timer event
	void * iomap_addr = mbuf_to_iomap(buff, mbuf_mtod(buff, void *));
	usys_io_read(key, iomap_addr, buff->len);
	return 0;
}

ssize_t bsys_io_write(char *key, void *val, size_t len){
	//FIXME: decide where to write
	dummy_dev_write(val, 1, 1);
	usys_io_wrote(key);

	return 0;
}
