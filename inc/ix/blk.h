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
#define MAX_KEY_LEN 128

#define META_SZ (MAX_KEY_LEN + 144) //size of metadata, TODO: RECOMPUTE LATER..
#define DATA_SZ (LBA_SIZE - META_SZ)

/*
 struct lba_meta{
 	int64_t lba;
 	uint64_t lba_count; // TODO: see if can use bitmasks to figure this out
 	uint16_t crc;
 };
 */

struct index_ent {
	char *key;
	int64_t lba;
	uint64_t lba_count;
	uint16_t crc;
	//LTODO: version?
	//struct lba_meta *metadata; //wrap metadata for asynchronous replacement..
	struct index_ent *next; //for hash chaining..
};



static struct index_ent indx[MAX_ENTRIES];

/* free list management */
uint64_t get_blk(uint64_t num_blks);

void free_blk(uint64_t lba, uint64_t num_blks);

void freelist_init();

void print_freelist(); //for debugging..

/* index management */

uint16_t crc_data(uint8_t msg[], size_t len);

uint64_t calc_numblks(ssize_t data_len);

struct index_ent *get_key_to_lba(char *key);

void update_index(struct index_ent *meta);

//struct index_ent *insert_key(char *key, ssize_t val_len);

void delete_key(char *key);

void index_init();



