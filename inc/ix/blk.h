/* 
 *  Header file for in-memory storage index & LBA management
 * 
 */ 

#include <stdint.h> //TODO: replace w/ <ix/types.h> or smthg..?
#include <assert.h> //TODO: to be replaced with appropriate error checking..
#include <ix/dummy_dev.h>
#include <ix/compiler.h>

#define MAX_ENTRIES 8 //dummy for now, eventually will need to decide how big we want index to be..
 						// NOTE also what is this number in relation to # of (per-core) LBAs?

//#define LBA_SIZE 512 //TODO: this is defined in dummy_dev.h
#define MAX_LBA_NUM 100000 //TODO: THIS IS TEMPORARY...
#define MAX_KEY_LEN 110
// ideally want key length + rest of metadata no more than 128 bytes (1/4 of a block)
#define META_SZ 144/8 //size of metadata in bytes LTODO: find less hardcode-y way to determine
#define DATA_SZ (LBA_SIZE - META_SZ)

/*
 struct lba_meta{
 	int64_t lba;
 	uint64_t lba_count; // TODO: see if can use bitmasks to figure this out
 	uint16_t crc;
 };
 */

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



