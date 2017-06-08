#pragma once

#include <ix/stddef.h>
#include <ix/mbuf.h>
#include <ix/syscall.h>

#define LBA_SIZE 512

typedef void (*io_cb_t)(void *);

int dummy_dev_init(void);
int dummy_dev_write(void *payload, uint64_t lba, uint64_t lba_count, 
		io_cb_t cb, void *arg);
int dummy_dev_writev(struct sg_entry *ents, unsigned int nents, uint64_t lba,
		uint64_t lba_count, io_cb_t cb, void *arg);
int dummy_dev_read(void *payload, uint64_t lba, uint64_t lba_count, io_cb_t cb,
		void *arg);
