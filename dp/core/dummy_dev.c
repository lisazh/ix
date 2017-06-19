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
#include <ix/timer.h>

#include <assert.h>

static struct mempool_datastore timer_datastore;
static DEFINE_PERCPU(struct mempool, timer_mempool);
static DEFINE_PERCPU(char *, dummy_dev);

#define MAX_PENDING_TIMERS 1024
#define WRITE_DELAY 50
#define READ_DELAY 10

struct io_timer {
	struct timer t;
	void (*cb)(void *);
	void *arg;
};

/*
 * This is the timer handle - Hack to implement timers
 */
void generic_io_handler(struct timer *t, struct eth_fg *unused)
{
	struct io_timer *iot = container_of(t, struct io_timer, t);
	iot->cb(iot->arg);
	mempool_free(&percpu_get(timer_mempool), iot);
}

int dummy_dev_init(void)
{
	int ret;
	ret = mempool_create_datastore(&timer_datastore, 32*MAX_PENDING_TIMERS,
				       sizeof(struct io_timer), 0, MEMPOOL_DEFAULT_CHUNKSIZE, "io_timer");
	return ret;
}

int dummy_dev_init_cpu(void)
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

	ret = mempool_create(&percpu_get(timer_mempool),
			&timer_datastore, MEMPOOL_SANITY_PERCPU, percpu_get(cpu_id));
	if (ret)
		return ret;

	return 0;
}

int dummy_dev_write(void *payload, uint64_t lba, uint64_t lba_count,
		io_cb_t cb, void *arg)
{
	struct io_timer * iot;
	memcpy(&percpu_get(dummy_dev)[lba * LBA_SIZE], payload, lba_count * LBA_SIZE);

	iot = mempool_alloc(&percpu_get(timer_mempool));
	assert(iot);
	iot->cb = cb;
	iot->arg = arg;

	timer_init_entry(&iot->t, generic_io_handler);
	timer_add(&iot->t, NULL, WRITE_DELAY);
	return 0;
}

int dummy_dev_read(void *payload, uint64_t lba, uint64_t lba_count, io_cb_t cb, 
		void *arg)
{
	//printf("DEBUG: about to issue read to lba %lu into %p\n", lba, payload);
	struct io_timer *iot;
	memcpy(payload, &percpu_get(dummy_dev)[lba * LBA_SIZE], lba_count * LBA_SIZE);

	iot = mempool_alloc(&percpu_get(timer_mempool));
	assert(iot);
	iot->cb = cb;
	iot->arg = arg;
	
	timer_init_entry(&iot->t, generic_io_handler);
	timer_add(&iot->t, NULL, READ_DELAY);

	return 0;
}
	
int dummy_dev_writev(struct sg_entry *ents, unsigned int nents, uint64_t lba,
		uint64_t lba_count, io_cb_t cb, void *arg)
{
	struct io_timer *iot;
	int i, bytes_written = 0;

	for (i=0;i<nents;i++) {
		memcpy(&percpu_get(dummy_dev)[lba * LBA_SIZE + bytes_written],
				ents[i].base, ents[i].len);
		bytes_written += ents[i].len;
		printf("DEBUG: bytes written so far is %d\n", bytes_written);
		assert(bytes_written <= lba_count * LBA_SIZE);
	}
	iot = mempool_alloc(&percpu_get(timer_mempool));
	assert(iot);
	iot->cb = cb;
	iot->arg = arg;

	timer_init_entry(&iot->t, generic_io_handler);
	timer_add(&iot->t, NULL, WRITE_DELAY);
	
	printf("DEBUG: added callback to timer..\n");
	return 0;
}
