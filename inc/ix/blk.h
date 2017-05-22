/* 
 *  Header file for in-memory storage index & LBA management
 * 
 */ 

#include <stdint.h> //TODO: replace w/ <ix/types.h> or smthg..?
#include <assert.h> //TODO: to be replaced with appropriate error checking..



#define MAX_ENTRIES 8 //dummy for now, eventually will need to decide how big we want index to be..
 						// NOTE also what is this number in relation to # of (per-core) LBAs?

#define LBA_SIZE 512 //TODO: tune this depending on the device..?
#define MAX_LBA_NUM 100000 //TODO: THIS IS TEMPORARY...

#define META_SZ 32 //size of metadata, TODO: RECOMPUTE LATER..
#define DATA_SZ (LBA_SIZE - META_SZ)

/*
 struct lba_metadata{
 	uint64_t lba;
 	uint64_t blk_no; // TODO: see if can use bitmasks to figure this out
 	uint64_t total_blks;
 };
*/

struct index_ent {
	char *key;
	int64_t lba;
	uint64_t lba_count;
	uint16_t crc;
	//TODO: version?
	struct index_ent *next; //for hash chaining..
};

static struct index_ent indx[MAX_ENTRIES];

/* free list management */
uint64_t get_blk(uint64_t num_blks);

void free_blk(uint64_t lba, uint64_t num_blks);

void freelist_init();

void print_freelist();

/* index management */

struct index_ent *get_key_to_lba(char *key);

struct index_ent *insert_key(char *key, ssize_t val_len);

void delete_key(char *key);

void index_init();



