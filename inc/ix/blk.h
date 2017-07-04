/* 
 *  Header file for in-memory storage index & LBA management
 * 
 */ 

#include <stdint.h> //TODO: replace w/ <ix/types.h> or smthg..?
#include <assert.h> //TODO: to be replaced with appropriate error checking..
#include <ix/dummy_dev.h>
#include <ix/compiler.h>

#define MAX_ENTRIES 4096 //dummy for now, eventually will need to decide how big we want index to be..
 						// NOTE also what is this number in relation to # of (per-core) LBAs?

//#define LBA_SIZE 512 //this is defined in dummy_dev.h
#define MAX_LBA_NUM (STORAGE_SIZE/LBA_SIZE) //TODO: Make this a per-core value...

// ideally want key length + rest of metadata no more than 128 bytes (1/4 of a block)
#define MAX_KEY_LEN 108
#define META_SZ ((160/8) + MAX_KEY_LEN) //in bytes LTODO: find less hardcode-y way to determine
#define DATA_SZ (LBA_SIZE - META_SZ)

#define METAMAGIC 0xd006

//random
#define CALC_NUMBLKS(b) ((b > DATA_SZ) ? (1 + (((b - DATA_SZ) + LBA_SIZE - 1) / LBA_SIZE)) : 1)


typedef int32_t lba_t; //typedef'd in case we change the size...used to be 64bit but truncated for now..
typedef uint32_t lbasz_t;

struct index_ent {
	uint16_t magic; //magic value for ease checking..
	char key[MAX_KEY_LEN];
	lba_t lba;
	uint64_t val_len; //length of value in BYTES
	//uint64_t lba_count;
	//uint16_t crc;
	uint32_t crc;
	uint16_t version;
	struct index_ent *next; //for hash chaining..
} __packed;

static struct index_ent *indx[MAX_ENTRIES];

/* free list management */
lba_t get_blk(lbasz_t num_blks);
void alloc_block(lba_t lba, lbasz_t lba_count);
void free_blk(lba_t lba, lbasz_t num_blks);
void freelist_init();
void print_freelist(); //for debugging..

/* index management */
void index_init();
//lbasz_t calc_numblks(uint64_t data_len);
struct index_ent *get_index_ent(const char *key);
struct index_ent *new_index_ent(const char *key, const void *val, uint64_t len);
void update_index(struct index_ent *meta);
//uint16_t get_version(const char *key);
//struct index_ent *insert_key(char *key, ssize_t val_len);
void print_index();
void delete_key(char *key); //unused for now...




