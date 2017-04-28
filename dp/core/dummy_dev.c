#include <stdio.h>

#include <ix/dummy_dev.h>
#include <ix/page.h>
#include <ix/errno.h>
#include <ix/cpu.h>

#include <assert.h>

#define STORAGE_PAGE_NR 8

static DEFINE_PERCPU(char *, dummy_dev);

int dummy_dev_init(void)
{
	char *arr;

	arr = (char *) page_alloc_contig(STORAGE_PAGE_NR);
	if (!arr)
		return -ENOMEM;
	percpu_get(dummy_dev) = arr;

	return 0;
}	

int dummy_dev_write(void *payload, uint64_t lba, uint64_t lba_count)
{
	memcpy(&percpu_get(dummy_dev)[lba * LBA_SIZE], payload, lba_count * LBA_SIZE);
	return 0;
}

struct mbuf *dummy_dev_read(uint64_t lba, uint64_t lba_count)
{
	assert(lba_count <= MBUF_DATA_LEN / LBA_SIZE);

	struct mbuf *read_mbuf = mbuf_alloc_local();
	char *data = mbuf_mtod(read_mbuf, char *);
	memcpy(data, &percpu_get(dummy_dev)[lba * LBA_SIZE], lba_count * LBA_SIZE);
	read_mbuf->len = lba_count * LBA_SIZE;

	return read_mbuf;
}

void dummy_dev_read_done(void *data)
{
	printf("Freeing mbuf\n");
	struct mbuf *b = (struct mbuf *)((uintptr_t) data - MBUF_HEADER_LEN);
	mbuf_free(b);
}
