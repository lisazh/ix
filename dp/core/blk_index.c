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

//for profiling index construction
#include <sys/time.h>

// for CRC
#define CRC_POLYNOM 0x1021
#define CRC_WIDTH 16
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1)) 


static const uint32_t Crc32Lookup[256] =
{
  // note: the first number of every second row corresponds to the half-byte look-up table !
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
    0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
    0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
    0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
    0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
    0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
    0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
    0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
    0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
    0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
    0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
    0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
    0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
    0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
    0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
	
};

//#define SCAN_BATCH (MBUF_DATA_LEN/LBA_SIZE)

int blks_read; //used later in index_init
struct timeval timer1;
struct timeval timer2;

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

uint16_t new_crc_data1(char* msg, size_t len){
    long crc = ~0;
    crc ^= *msg++;
    
    while (len--)
        crc = crc & 1 ? (crc >> 1) ^ CRC_POLYNOM : crc >> 1;

    return ~crc;
}

uint32_t new_crc_data(const void* data, size_t length)
{
  uint32_t crc = ~0;
  unsigned char* current = (unsigned char*) data;
  while (length--)
    crc = (crc >> 8) ^ (int)Crc32Lookup[(crc & 0xFF) ^ *current++];
  return ~crc;
}


/*
 * Given key, return LBA mapping
 * returns NULL if key is not in the index..
 * TODO: look @ bloom filter to optimize membership testing..
 */
struct index_ent *get_index_ent(const char *key){

	gettimeofday(&timer1, NULL);
	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;
	gettimeofday(&timer2, NULL);

	printf("DEBUG: key hash for key %s took %d microseconds\n", key, (
                TIMETOMICROS(timer2.tv_sec, timer2.tv_usec) -
                TIMETOMICROS(timer1.tv_sec, timer1.tv_usec)));
	
	struct index_ent *ret = indx[hashval];

	//printf("DEBUG: looking for key %s with hash %lu\n", key, hashval);

	if (ret == NULL){ //no key found 
		//printf("No entry found..\n");
		return ret;

	} else if (strncmp(ret->key, key, strlen(key)) != 0) { //check chain
		//printf("DEBUG: traversing hash chain for key %s\n", ret->key);
		while (ret->next){
			//printf("DEBUG: looking at next entry in chain..\n"); 
			ret = ret->next;
			printf("DEBUG: current key is %s\n", ret->key);
			if (strncmp(ret->key, key, strlen(key)) == 0) {
				break;
			}
		}
		if (!ret->next && strncmp(ret->key, key, strlen(key)) != 0){
			return NULL;
		}
	}
	//gettimeofday(&timer2, NULL);
	//printf("DEBUG: index searching took %d microseconds\n", (
                //TIMETOMICROS(timer2.tv_sec, timer2.tv_usec) -
                //TIMETOMICROS(timer1.tv_sec, timer1.tv_usec)));

	return ret;		
}

uint16_t get_version(const char *key){
	struct index_ent *ent = get_index_ent(key);		
	//uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;

	if (ent == NULL)
		return 0;
	else 
		return ent->version;

}

uint16_t get_version2(const char *key){

	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;	
	struct index_ent *ret = indx[hashval];

	if (ret == NULL){ //no key found 
		//printf("No entry found..\n");
		return 0;

	} else if (strncmp(ret->key, key, strlen(key)) != 0) { //check chain
		//printf("DEBUG: traversing hash chain for key %s\n", ret->key);
		while (ret->next){
			//printf("DEBUG: looking at next entry in chain..\n"); 
			ret = ret->next;
			//printf("DEBUG: current key is %s\n", ret->key);
			if (strncmp(ret->key, key, strlen(key)) == 0) {
				break;
			}
		}
		if (!ret->next && strncmp(ret->key, key, strlen(key)) != 0){
			return 0;
		}
	}
	return ret->version;
}

/* 
 * helper function for determining how many blocks the data should occupy
 * assumes data_len is never zero..
 * TODO: MOVE TO MACRO 	
 */
lbasz_t calc_numblks(uint64_t data_len){
	//uint64_t ret = (data_len + DATA_SZ - 1) / DATA_SZ; //short way;
	lbasz_t ret = 1; //minimum one block 
	if (data_len > DATA_SZ)
		ret += ((data_len - DATA_SZ) + LBA_SIZE - 1) / LBA_SIZE;
	//printf("DEBUG: calc numblks returning %u for data of length %lu\n", ret, data_len);
	return ret;
}

/*  Helper function to get a new index structure
 * with metadata populated..
 *
 */
struct index_ent *new_index_ent(const char *key, const void *val, const uint64_t len){
	
	gettimeofday(&timer1, NULL);

	struct index_ent *ret = malloc(sizeof(struct index_ent));
	ret->magic = METAMAGIC;	
	memset(ret->key, '\0', MAX_KEY_LEN);
	strncpy(ret->key, key, strlen(key));
	//printf("DEBUG: key and entry key is %s and %s\n", key, ret->key);
	
	gettimeofday(&timer2, NULL);
	//printf("DEBUG: index entry setup (malloc & copy) took %d microseconds\n", (
                //TIMETOMICROS(timer2.tv_sec, timer2.tv_usec) -
                //TIMETOMICROS(timer1.tv_sec, timer1.tv_usec)));

	//update metadata
	ret->val_len = len;
	//ret->crc = new_crc_data((uint8_t *)val, len);
	ret->crc = 0;		

	gettimeofday(&timer1, NULL);
	//printf("DEBUG: index entry CRC took %d microseconds\n", (
               // TIMETOMICROS(timer1.tv_sec, timer1.tv_usec) -
               // TIMETOMICROS(timer2.tv_sec, timer2.tv_usec)));

	ret->version = get_version2(key) + 1;
	//printf("DEBUG: metadata for key %s with magic value %hu, val_len %lu, crc %d and version %d\n", key, ret->magic, ret->val_len, ret->crc, ret->version); 
	ret->next = NULL;

	gettimeofday(&timer2, NULL);
	//printf("DEBUG: index entry creation (rest) took %d microseconds\n", (
		//TIMETOMICROS(timer2.tv_sec, timer2.tv_usec) - 
		//TIMETOMICROS(timer1.tv_sec, timer1.tv_usec)));

	return ret;
}

/*
 * Update index w/ new entry
 * Either replace (and free) old entry corresponding to the same key or insert it anew.
 */
void update_index(struct index_ent *meta){

	gettimeofday(&timer1, NULL);

	char *key = meta->key;

	uint64_t hashval = hashkey(key, strlen(key)) % MAX_ENTRIES;
	struct index_ent *oldent = indx[hashval];
	if (oldent != NULL){
		//printf("DEBUG: examining existing index for update..\n");
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
		//printf("DEBUG: inserting key %s with hash %lu\n", key, hashval);
		indx[hashval] = meta;
	}

	gettimeofday(&timer2, NULL);
	//printf("DEBUG: index update took %d microseconds\n", (
		//TIMETOMICROS(timer2.tv_sec, timer2.tv_usec) - 
		//TIMETOMICROS(timer1.tv_sec, timer1.tv_usec)));

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

/*
 * Callback function for the initialization process.	
 * TODO: actually need to make this read asynchronously..
 */
void init_cb(void *arg){
	struct index_ent *ent = malloc(sizeof(struct index_ent));

	//printf("DEBUG: reading entries from device..key is %s, value length is %lu\n", ent->key, ent->val_len);
	uint16_t *tmp = (uint16_t *)arg;
	if (*tmp  == METAMAGIC){
		memcpy(ent, arg, META_SZ);
		ent->next = NULL;
		//printf("DEBUG: reading entries from device..key is %s, value length is %lu\n", ent->key, ent->val_len);
		assert(ent->val_len > 0);
	
		uint64_t blks = calc_numblks(ent->val_len);
		struct index_ent *old = get_index_ent(ent->key);
		
		if ((old && old->version < ent->version)|| old == NULL){
			uint16_t tocheck_crc;

			if (ent->val_len > DATA_SZ){

				char *buf = malloc(LBA_SIZE * blks);
				dummy_dev_read(buf, blks_read, blks, dummy_cb, NULL);
				tocheck_crc = new_crc_data((buf + META_SZ), ent->val_len);
		
				free(buf);

			} else {
				tocheck_crc = new_crc_data((arg + META_SZ), ent->val_len);
			}

			//assert(ent->crc == tocheck_crc); //TODO: remove assert, do check instead
			update_index(ent);
			alloc_block(ent->lba, blks);

		} else {
			free(ent);
		}
		blks_read += blks; //even if we didn't keep the value, count it as read 
		//printf("DEBUG: Read %lu, blocks so far is %d\n", blks, blks_read);
	
	} else { //if garbage data found just skip to next block..
		free(ent);
		blks_read++;
		//printf("DEBUG:Skipped, blocks so far is %d\n", blks_read);
	}
	//print_index();
	//print_freelist();
}


/* 
 * TODO: scan (per cpu partition?) of storage structure and populate indexes
 */
void index_init(){

	for (int i = 0; i < MAX_ENTRIES; i++){
		indx[i] = NULL; 
	}

	blks_read = 0; //this is both the number of blks scanned so far and the current LBA..
	
	//printf("DEBUG: attempting to read from device..\n"); 
	char *buf = malloc(LBA_SIZE);
	
	//#TIMER STUFF
	if (gettimeofday(&timer1, NULL)){
		fprintf(stderr, "Timer issue.\n");
		exit(1);
	}
	//#


	while(blks_read < MAX_LBA_NUM){
		//printf("DEBUG: about to issue read..\n");
		dummy_dev_read(buf, blks_read, 1, dummy_cb, NULL);		
		init_cb(buf);
	}

	free(buf);

	//#END TIMER STUFF
	if (gettimeofday(&timer2, NULL)){
		fprintf(stderr, "Timer issue.\n");
		exit(1);
	}	
	printf("DEBUG: index building took %ld:%ld\n", 
		timer2.tv_sec - timer1.tv_sec, timer2.tv_usec - timer1.tv_usec);
	//#
	
	print_index();
	print_freelist();	
}

/* 
 * Callback for handling getting rest of the data..
 * arg is metadata + a known number of blocks
 * 
 */
/*

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
*/

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
