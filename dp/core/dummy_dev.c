#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ix/dummy_dev.h>
#include <ix/page.h>
#include <ix/errno.h>
#include <ix/cpu.h>

#include <assert.h>

#define STORAGE_SIZE 8*1024*1024

static DEFINE_PERCPU(char *, dummy_dev);

int dummy_dev_init(void)
{
	int fd, ret;
	void *vaddr;

	fd = shm_open("/dummy-ssd", O_RDWR | O_CREAT, 0660);
	if (fd == -1)
		return 1;

	ret = ftruncate(fd, STORAGE_SIZE);
	if (ret)
		return ret;

	vaddr = mmap(NULL, STORAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (vaddr == MAP_FAILED)
		return 1;

	percpu_get(dummy_dev) = vaddr;

	return 0;
}

int dummy_dev_write(void *payload, uint64_t lba, uint64_t lba_count,
		io_cb_t cb, void *arg)
{
	memcpy(&percpu_get(dummy_dev)[lba * LBA_SIZE], payload, lba_count * LBA_SIZE);
	return 0;
}

struct mbuf *dummy_dev_read(uint64_t lba, uint64_t lba_count, io_cb_t cb, 
		void *arg)
{
	assert(lba_count <= MBUF_DATA_LEN / LBA_SIZE);

	struct mbuf *read_mbuf = mbuf_alloc_local();
	char *data = mbuf_mtod(read_mbuf, char *);
	memcpy(data, &percpu_get(dummy_dev)[lba * LBA_SIZE], lba_count * LBA_SIZE);
	read_mbuf->len = lba_count * LBA_SIZE;

	return read_mbuf;
}

int dummy_dev_writev(struct sg_entry *ents, unsigned int nents, uint64_t lba,
		uint64_t lba_count, io_cb_t cb, void *arg)
{
	int i, bytes_written = 0;

	for (i=0;i<nents;i++) {
		memcpy(&percpu_get(dummy_dev)[lba * LBA_SIZE + bytes_written],
				ents[i].base, ents[i].len);
		bytes_written += ents[i].len;
		printf("DEBUG: bytes written so far is %d\n", bytes_written);
		assert(bytes_written <= lba_count * LBA_SIZE);
	}

	return 0;
}

void dummy_dev_read_done(void *data)
{
	printf("Freeing mbuf\n");
	struct mbuf *b = (struct mbuf *)((uintptr_t) data - MBUF_HEADER_LEN);
	mbuf_free(b);
}
