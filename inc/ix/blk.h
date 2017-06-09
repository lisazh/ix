/* 
 *  Header file for in-memory storage index & LBA management
 * 
 */ 

#include <stdint.h> //TODO: replace w/ <ix/types.h> or smthg..?
#include <assert.h> //TODO: to be replaced with appropriate error checking..
#include <ix/dummy_dev.h>
#include <ix/compiler.h>

#define MAX_ENTRIES 1024 //dummy for now, eventually will need to decide how big we want index to be..
 						// NOTE also what is this number in relation to # of (per-core) LBAs?

//#define LBA_SIZE 512 //this is defined in dummy_dev.h
#define MAX_LBA_NUM (STORAGE_SIZE/LBA_SIZE/32) //TODO: THIS IS TEMPORARY...need to determine real value..

// ideally want key length + rest of metadata no more than 128 bytes (1/4 of a block)
#define MAX_KEY_LEN 110
#define META_SZ ((144/8) + MAX_KEY_LEN) //in bytes LTODO: find less hardcode-y way to determine
#define DATA_SZ (LBA_SIZE - META_SZ)

struct index_ent {
	char key[MAX_KEY_LEN];
	int64_t lba;
	uint64_t val_len; //length of value in BYTES
	//uint64_t lba_count;
	uint16_t crc;
	//LTODO: version?
	struct index_ent *next; //for hash chaining..
} __packed;



static struct index_ent *indx[MAX_ENTRIES];

/* free list management */

void alloc_block(uint64_t lba, uint64_t lba_count);

uint64_t get_blk(uint64_t num_blks);

void free_blk(uint64_t lba, uint64_t num_blks);

void freelist_init();

void print_freelist(); //for debugging..

/* index management */
uint16_t crc_data(uint8_t msg[], size_t len);

uint64_t calc_numblks(ssize_t data_len);

struct index_ent *get_index_ent(const char *key);

struct index_ent *new_index_ent(const char *key);

void update_index(struct index_ent *meta);

//struct index_ent *insert_key(char *key, ssize_t val_len);

void delete_key(char *key);

void index_init();



