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

#include <stdio.h>



ssize_t bsys_io_read(char *key){
	printf("READ CALLED\n");
	return 0;

}
ssize_t bsys_io_write(char *key, void *val, size_t len){
	printf("WRITE CALLED\n");
	return 0;

}