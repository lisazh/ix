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
  //KSTATS_VECTOR(bsys_io_read);
	printf("READ CALLED\n");
	dummy_dev_read(1, 1);
	return 0;

}
ssize_t bsys_io_write(char *key, void *val, size_t len){
  //KSTATS_VECTOR(bsys_io_write);
	dummy_dev_write(val, 1, 1);
	return 0;

}
