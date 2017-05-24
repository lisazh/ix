#pragma once

#include <ix/stddef.h>
#include <ix/mbuf.h>
#include <ix/syscall.h>

#define LBA_SIZE 512

int dummy_dev_init(void);
int dummy_dev_write(void *payload, uint64_t lba, uint64_t lba_count);
int dummy_dev_writev(struct sg_entry *ents, unsigned int nents, uint64_t lba,
		uint64_t lba_count);
struct mbuf *dummy_dev_read(uint64_t lba, uint64_t lba_count);
void dummy_dev_read_done(void *data);
