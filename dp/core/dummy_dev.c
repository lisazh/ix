#include <stdio.h>

#include <ix/dummy_dev.h>
#include <ix/page.h>
#include <ix/errno.h>
#include <ix/cpu.h>

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
	printf("wrote successfully: %s\n", (char *) payload);
	return 0;
}

void dummy_dev_read(uint64_t lba, uint64_t lba_count)
{
	char tmp[512];
	memcpy(tmp, &percpu_get(dummy_dev)[lba * LBA_SIZE], lba_count * LBA_SIZE);
	printf("I read %s\n", tmp);
}
