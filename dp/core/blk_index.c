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

// for CRC
#define CRC_POLYNOM 0x1021
#define CRC_WIDTH 16
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1)) 

// wrapper function, in case we decide to change the hash...
uint64_t hashkey(char *key, size_t keylen){
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
 * TODO: look @ bloom filter to optimize membership testing..
 */
struct index_ent *get_key_to_lba(char *key){

	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;
	struct index_ent *ret = indx[hashval];

	printf("DEBUG: looking for key %s with hash %lu\n", key, hashval);

	if (ret == NULL){ //no key found 
		//TODO: error handling
		return ret;

	} else if (strncmp(ret->key, key, strlen(key)) != 0) { //check chain
		while (ret->next){
			ret = ret->next;
			if (strncmp(ret->key, key, strlen(key)) == 0) {
				break;
			}
		}
		if (!ret->next && strncmp(ret->key, key, strlen(key)) != 0){
			//TODO: error handling, key not found
			return NULL;
		}
	}
	return ret;		
}

/* 
 * helper function for determining how many blocks the data should occupy
 * assumes data_len is never zero..	
 */
//LTODO: this may change depending on data layout..
uint64_t calc_numblks(ssize_t data_len){
	//uint64_t ret = (data_len + DATA_SZ - 1) / DATA_SZ; //short way;
	uint64_t ret = 1; //minimum one block 
	if (data_len > DATA_SZ)
		ret += ((data_len - DATA_SZ) + LBA_SIZE - 1) / LBA_SIZE;
	return ret;
}

/*  Helper function to get a new index structure
 *
 */
struct index_ent *new_ent(const char *key){
	struct index_ent *ret = malloc(sizeof(struct index_ent));
	memset(ret->key, '\0', MAX_KEY_LEN);
	//ret->key = {0};
	strncpy(ret->key, key, strlen(key));

	return ret;
}


char *update_index(struct index_ent *meta){
	char *key = meta->key;

	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;
	struct index_ent *oldent = indx[hashval];
	if (oldent != NULL){ //need to assume that pointer is either null or a valid entry..
		if (strncmp(oldent->key, key, strlen(key)) == 0){
			//simple swap
			meta->next = oldent->next;
			indx[hashval] = meta;
			//free(oldent->key);
			free(oldent);
		} else { //traverse hash chain..
			struct index_ent *tmp = oldent->next;
			while (tmp){
				if (strncmp(tmp->key, key, strlen(key)) == 0){
					break;	
				}
				oldent = tmp;
				tmp = tmp->next;
			}
			oldent->next = meta;
			if (tmp){
				meta->next = tmp->next;
				free(tmp); //remove old index entry

			}
		}
	} else { //key not already in index
		printf("DEBUG: inserting key %s with hash %lu\n", key, hashval);
		indx[hashval] = meta;
	}
	return key;
}

/* DEPRECATED
 * Handles insertion of a key
 * TODO: replace all calls to malloc with appropriate memory management mech...
 * ALSO: consider separating cases of updating old entry & insert new entry, depending on performance
 
struct index_ent *insert_key(char *key, ssize_t val_len){

	// compute index metadata..
	//uint64_t hashval = hashkey(key, strlen(key)); //TODO: assume keys are null-terminated??

	uint64_t lba_ct = calc_numblks(val_len);

	//struct index_ent *insert = &(indx[hashval % MAX_ENTRIES]);
	struct index_ent *insert = get_key_to_lba(key);

	if (insert){ //key already exists, just update metadata
		printf("DEBUG: key already exists\n");
		free_blk(insert->lba, insert->lba_count);

	} else { //insert key appropriately
		uint64_t hashval = hashkey(key, strlen(key)); //TODO: assume keys are null-terminated??
		insert = &(indx[hashval % MAX_ENTRIES]);

		if (insert->key){ 
			//handle collision w chaining
			while (insert->next != NULL){
				insert = insert->next;
			}

			struct index_ent *tmp = insert;
			insert = malloc(sizeof(struct index_ent));
			tmp->next = insert;
			insert->next = NULL; //not sure if this needs to be done explicitly...			
		}

		insert->key = malloc(strlen(key) + 1); //+1 depending if key is null-terminated...
		strncpy(insert->key, key, strlen(key));
		insert->key[strlen(key)] = '\0'; //safety
	}
	
	//insert->lba_count = lba_ct;

	//printf("DEBUG: Inserted key %s at lba %ld\n", insert->key, insert->lba);
	printf("DEBUG: Inserted key %s for %lu blocks\n", insert->key, insert->lba_count);

	return insert;
}
*/

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
		struct index_ent *tmp = oldent->next;
		while(tmp){
			if (strncmp(tmp->key, key, strlen(key)) == 0)
				break;

			oldent = tmp;
			tmp = tmp->next;
		}

		if (!tmp){ //end of bucket and no matching entry found
			//TODO: better error handling
			printf("ERROR: key not found");
			return;
		}

		oldent->next = tmp->next;
		char *oldkey = tmp->key;
		free_blk(tmp->lba, tmp->lba_count);
		free(oldkey);
		free(tmp);

	} else {
		char *oldkey = oldent->key;
		free_blk(oldent->lba, oldent->lba_count);

		//oldent->key = NULL;
		oldent->lba = -1;
		oldent->lba_count = 0;
		oldent->crc = 0;
		oldent->next = NULL;

		//free(oldkey);

	}

}

/* 
 * TODO: scan (per cpu partition?) of storage structure and populate indexes
 */
void index_init(){
	//indx = malloc((sizeof(struct index_ent) * MAX_ENTRIES)); //TODO: ???use appropriate "memory" allocation
	/*
	for (int i = 0; i < MAX_ENTRIES; i++){
		indx[i].key = NULL;
		indx[i].metadata = NULL;
		indx[i].next = NULL;
	}
	*/
	for (int i = 0; i < MAX_ENTRIES; i++){
		indx[i] = NULL; //initialize to null-pointers..
	}
	//LTODO: scan through device and update freelist appropriately..
	//LTODO: remember to add crc checks while scanning from device..
	
}

/*
 * BELOW FOR TESTING PURPOSES ONLY

int main(int argc, char *argv[]){
	index_init();
	printf("DEBUG: initialized index\n");
	freelist_init();
	printf("DEBUG: initialized freelist\n");

	printf("DEBUG: Block size: %d, Metadata size: %d, Data size: %d\n", LBA_SIZE, META_SZ, DATA_SZ);

	char key1[] = "test1";
	char key2[] = "test2";
	char key3[] = "asasdftest3";
	char key4[] = "fjjff";


	insert_key(key1, 10);
	insert_key(key2, 20);
	insert_key(key3, 3000);
	insert_key(key4, 11);
	print_freelist();
	/*
	insert_key(key1, 15);
	print_freelist();
	insert_key(key4, 11); //an "in-place update"


	struct index_ent *ent1 = get_key_to_lba(key1);
	struct index_ent *ent2 = get_key_to_lba(key2);
	struct index_ent *ent3 = get_key_to_lba(key3);
	struct index_ent *ent4 = get_key_to_lba(key4);

	printf("Key %s inserted at block no %lu for %lu blocks\n", ent1->key, ent1->lba, ent1->lba_count);
	printf("Key %s inserted at block no %lu for %lu blocks\n", ent2->key, ent2->lba, ent2->lba_count);
	printf("Key %s inserted at block no %lu for %lu blocks\n", ent3->key, ent3->lba, ent3->lba_count);
	printf("Key %s inserted at block no %lu for %lu blocks\n", ent4->key, ent4->lba, ent4->lba_count);
	print_freelist();

	delete_key(key1);
	delete_key(key2);
	delete_key(key3);
	delete_key(key4);

}

*/
