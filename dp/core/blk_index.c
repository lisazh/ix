/* 
 * NEW IMPLEMENTATION for In memory index for NVM KV store
 * DO NOT actually do any LBA assignment in this case...leave for later..
 */ 

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h> //for testing only..
#include <ix/city.h>
#include <ix/blk.h>
#include <ix/mbuf.h>

// for CRC
#define CRC_POLYNOM 0x1021
#define CRC_WIDTH 16
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1)) 


//#define SCAN_BATCH (MBUF_DATA_LEN/LBA_SIZE)

int blks_read; //used later in index_init

// wrapper function, in case we decide to change the hash...
uint64_t hashkey(const char *key, size_t keylen){
	return CityHash64(key, keylen);
}


uint16_t crc_data(uint8_t msg[], size_t len){
 	uint16_t crc = 0;
 	for (int i = 0; i < len; i++){
 		crc ^= (msg[i] << (CRC_WIDTH - 8));

 		for (uint8_t bit = 0; bit < 8; bit++){
 			if (crc & CRC_TOPBIT){
 				crc = (crc << 1) ^ CRC_POLYNOM; 
 			} else {
 				crc = (crc << 1);
 			}
 		}
 	}
 	return crc;
}

/*
 * Given key, return LBA mapping
 * returns NULL if key is not in the index..
 * TODO: look @ bloom filter to optimize membership testing..
 */
struct index_ent *get_index_ent(const char *key){

	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;
	struct index_ent *ret = indx[hashval];

	printf("DEBUG: looking for key %s with hash %lu\n", key, hashval);

	if (ret == NULL){ //no key found 
		return ret;

	} else if (strncmp(ret->key, key, strlen(key)) != 0) { //check chain
		while (ret->next){
			ret = ret->next;
			if (strncmp(ret->key, key, strlen(key)) == 0) {
				break;
			}
		}
		if (!ret->next && strncmp(ret->key, key, strlen(key)) != 0){
			return NULL;
		}
	}
	return ret;		
}

uint16_t get_version(const char *key){
	struct index_ent *ent = get_index_ent(key);
	if (ent == NULL)
		return 0;
	else 
		return ent->version;

}

/* 
 * helper function for determining how many blocks the data should occupy
 * assumes data_len is never zero..	
 */
uint64_t calc_numblks(ssize_t data_len){
	//uint64_t ret = (data_len + DATA_SZ - 1) / DATA_SZ; //short way;
	uint64_t ret = 1; //minimum one block 
	if (data_len > DATA_SZ)
		ret += ((data_len - DATA_SZ) + LBA_SIZE - 1) / LBA_SIZE;
	return ret;
}

/*  Helper function to get a new index structure
 * with metadata populated..
 *
 */
struct index_ent *new_index_ent(const char *key, const void *val, const uint64_t len){
	struct index_ent *ret = malloc(sizeof(struct index_ent));
	ret->magic = METAMAGIC;	
	memset(ret->key, '\0', MAX_KEY_LEN);
	strncpy(ret->key, key, strlen(key));

	//update metadata
	ret->val_len = len;
	ret->crc = crc_data((uint8_t *)val, len);
	ret->version = get_version(key) + 1;
	printf("DEBUG: metadata for key %s with magic value %hu, val_len %lu, crc %d and version %d\n", key, ret->magic, ret->val_len, ret->crc, ret->version); 
	return ret;
}

/*
 * Update index w/ new entry
 * Either replace (and free) old entry corresponding to the same key or insert it anew.
 */
void update_index(struct index_ent *meta){
	char *key = meta->key;

	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;
	struct index_ent *oldent = indx[hashval];
	if (oldent != NULL){
		if (strncmp(oldent->key, key, strlen(key)) != 0){ //walk chain..
			struct index_ent *prev = oldent;
			oldent = oldent->next;
			while(oldent){
				if (strncmp(oldent->key, key, strlen(key)) == 0){
					break;
				}
				prev = oldent;
				oldent = oldent->next;
			}
			prev->next = meta; //add to end of the chain/swap out for oldent
			
			if (!oldent)
				return;

		} else { //swap
			indx[hashval] = meta;
		}
		// clean up oldent
		free_blk(oldent->lba, calc_numblks(oldent->val_len));
		//printf("DEBUG: freeing LBA %d for %lu blocks\n", oldent->lba, calc_numblks(oldent->val_len));
		//print_freelist();
		meta->next = oldent->next;
		free(oldent);

	} else { //key not already in index
		printf("DEBUG: inserting key %s with hash %lu\n", key, hashval);
		indx[hashval] = meta;
	}
}

/*
 * Remove key from index
 * NOTE: this removes references to the specified key in our index entirely
 * Do not use for updates
 * TODO: possible refactoring needed bottom half bit untidy
*/

void delete_key(char *key){

	int ind = hashkey(key, strlen(key)) % MAX_ENTRIES;

	struct index_ent *oldent = indx[ind];

	if (strncmp(oldent->key, key, strlen(key)) != 0){ //check the chain
		struct index_ent *prev = oldent;
		oldent = oldent->next;
		while(oldent){
			if (strncmp(oldent->key, key, strlen(key)) == 0)
				break;

			prev = oldent;
			oldent = oldent->next;
		}

		if (!oldent){ //end of bucket and no matching entry found
			//TODO: better error handling
			fprintf(stderr, "Key to be deleted %s not found\n", key);
			return;
		}
		prev->next = oldent->next; //remove from chain
	}

	free_blk(oldent->lba, calc_numblks(oldent->val_len));
	free(oldent);
	indx[ind] = NULL; //safety

}
void dummy_cb(void *unused){}

//TODO: actually need to make this read asynchronously..
void init_cb(void *arg){
	struct index_ent *ent = malloc(sizeof(struct index_ent));
	//memset(ent, METAMAGIC, 2);
	//memcpy(ent, arg, META_SZ);

	//printf("DEBUG: reading entries from device..key is %s, value length is %lu\n", ent->key, ent->val_len);
	uint16_t *tmp = (uint16_t *)arg;
	//assert(ent->magic == METAMAGIC);
	if (*tmp  == METAMAGIC){
		memcpy(ent, arg, META_SZ);
		printf("DEBUG: reading entries from device..key is %s, value length is %lu\n", ent->key, ent->val_len);
		assert(ent->val_len > 0);
	
		uint64_t blks = calc_numblks(ent->val_len);
		struct index_ent *tmp = get_index_ent(ent->key);
		
		if ((tmp && tmp->version < ent->version)|| tmp == NULL){
			uint16_t tocheck_crc;

			if (ent->val_len > DATA_SZ){

				char *buf = malloc(LBA_SIZE * blks);
				dummy_dev_read(buf, blks_read, blks, dummy_cb, NULL);
				tocheck_crc = crc_data((buf + META_SZ), ent->val_len);
		
				free(buf);

			} else {
				tocheck_crc = crc_data((arg + META_SZ), ent->val_len);
			}

			assert(ent->crc == tocheck_crc);
			update_index(ent);
			alloc_block(ent->lba, blks);
			print_index();
			print_freelist();
		} else {
			free(ent);
		}
		blks_read += blks; //even if we didn't keep the value, count it as read 
		printf("DEBUG: Read %lu, blocks so far is %d\n", blks, blks_read);
	
	} else { //not sure how to handle the case w/ garbage data or smthg..
		free(ent);
		blks_read++; //skip to next block..
		printf("DEBUG:Skipped, blocks so far is %d\n", blks_read);
	}

}


/* 
 * TODO: scan (per cpu partition?) of storage structure and populate indexes
 */
void index_init(){

	for (int i = 0; i < MAX_ENTRIES; i++){
		indx[i] = NULL; 
	}

	blks_read = 0; //this is both the number of blks scanned so far and the current LBA..
	
	printf("DEBUG: attempting to read from device..\n"); 
	char *buf = malloc(LBA_SIZE);
	
	while(blks_read < MAX_LBA_NUM){
		//printf("DEBUG: about to issue read..\n");
		dummy_dev_read(buf, blks_read, 1, dummy_cb, NULL);		
		init_cb(buf);
	}

	//dummy_dev_read(buf, blks_read, 1, init_cb_oneblk, buf); 
	free(buf);
	
}


/* 
 * Callback for handling getting rest of the data..
 * arg is metadata + a known number of blocks
 * 
 */
void init_cb_multiblk(void *arg){

	//allocate a new index entry
	struct index_ent *ent = malloc(sizeof(struct index_ent));

	free(arg);
}

///alternate version for asynchronous-ness
void init_cb_oneblk(void *arg){

	struct index_ent *ent = malloc(sizeof(struct index_ent));

	uint16_t *tmp = (uint16_t *)arg;
	//assert(ent->magic == METAMAGIC);
	if (*tmp  == METAMAGIC){
		memcpy(ent, arg, META_SZ);
		printf("DEBUG: reading entries from device..key is %s, value length is %lu\n", ent->key, ent->val_len);
		assert(ent->val_len > 0);
	
		uint64_t blks = calc_numblks(ent->val_len);
		struct index_ent *tmp = get_index_ent(ent->key);
		
		if ((tmp && tmp->version < ent->version)|| tmp == NULL){
			uint16_t tocheck_crc;

			if (ent->val_len > DATA_SZ){

				char *buf = malloc(LBA_SIZE * blks);
				dummy_dev_read(buf, blks_read, blks, dummy_cb, NULL);
				tocheck_crc = crc_data((buf + META_SZ), ent->val_len);
		
				//free(buf);
				free(ent);

			} else {
				tocheck_crc = crc_data((arg + META_SZ), ent->val_len);
			}

			assert(ent->crc == tocheck_crc);
			update_index(ent);
			alloc_block(ent->lba, blks);
			print_index();
			print_freelist();
		} else {
			free(ent);
		}
		blks_read += blks; //even if we didn't keep the value, count it as read 
		printf("DEBUG: Read %lu, blocks so far is %d\n", blks, blks_read);
	
	} else { //garbage data, skip to next block
		free(ent);
		blks_read++;
		printf("DEBUG:Skipped, blocks so far is %d\n", blks_read);
	}

	if (blks_read < MAX_LBA_NUM)
		dummy_dev_read(buf, blks_read, 1, init_cb_oneblk, buf);
	else
		free(buf);
}

/* FOR DEBUGGING ONLY
 * 
 */
void print_index(){
	for (int i = 0; i < MAX_ENTRIES; i++){
		if (indx[i] && indx[i]->key){
			printf("Index entry for key %s, with value length %lu, at lba %lu\n", indx[i]->key, indx[i]->val_len, indx[i]->lba);
		}
	}
}
