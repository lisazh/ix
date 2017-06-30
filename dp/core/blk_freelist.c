/* Freelist to manage LBA mappings 
 * TODO: a stack is probably the best way to manage this..
 *
 */

#include <stdlib.h>
#include <stdio.h> //For debug only..

#include <ix/blk.h>

/*
#define LBA_SIZE 512 //TODO: tune this depending on the device..?
#define MAX_LBA_NUM 100000 //TODO: THIS IS TEMPORARY...
//#define NUM_SZ_CLASS 16
#define META_SZ 32 //size of metadata, TODO: RECOMPUTE LATER..
#define DATA_SZ (LBA_SIZE - META_SZ)
*/

struct freelist_ent { //keep this internal to this file..?
	lba_t start_lba;
	lbasz_t lba_count;
	struct freelist_ent *next;	
};

static struct freelist_ent *freelist;


/* Mark specified LBA range as occupied in the freelist
 * Only used on start in index construction
 */ 
void alloc_block(lba_t lba, lbasz_t lba_count){
	struct freelist_ent *ent = freelist;

	while (ent->next){
		if (ent->start_lba <= lba && (ent->start_lba + ent->lba_count) > lba) //
			break;
		ent = ent->next;
	}

	assert((ent->start_lba + ent->lba_count) >= (lba + lba_count)); //allocation needs to be within the freelist entry

	// check ends
	if (ent->start_lba == lba){
		mark_used_blk(ent, lba_count);
	} else if ((ent->start_lba + ent->lba_count) == (lba + lba_count)){
		ent->lba_count -= lba_count;
	} else { //need to split the entry

		struct freelist_ent *newent = malloc(sizeof(struct freelist_ent));
		newent->start_lba = (lba + lba_count);
		newent->lba_count = ent->lba_count - lba_count - (lba - ent->start_lba);
		newent->next = ent->next;

		ent->lba_count = lba - ent->start_lba;
		ent->next = newent;

	}

}

void mark_used_blk(struct freelist_ent *ent, lbasz_t num_blks){

	if (num_blks < ent->lba_count){
		ent->start_lba += num_blks;
		ent->lba_count -= num_blks;

	} else { //remove this entry..
		if (ent == freelist){
			freelist = freelist->next;
		} else {
			struct freelist_ent *tmp = freelist;
			while (tmp->next != ent){ //TODO: maybe use doubly linked list if want to avoid this..
				tmp = tmp->next;
			}
			tmp->next = ent->next; 
		}
		free(ent);
	}

}

/*
* search for contiguous block large enough..
* first fit alg. b/c not likely to do *too* much reclamation (although this may change)
* also last ent guaranteed to be "rest of space" entry
*/ 
lba_t get_blk(lbasz_t num_blks){
	//printf("DEBUG: trying to find space for %lu blocks\n", num_blks);
	//print_freelist();
	struct freelist_ent *iter = freelist;
	//printf("DEBUG: entering loop\n");
	while (iter){
		if (iter->lba_count >= num_blks)
			break;
		iter = iter->next;
	} 
	//printf("DEBUG: exited loop\n");
	
	if (iter == NULL)
		printf("DEBUG: reached end without finding space.. \n");
	//TODO: currently assume will always have storage space..what if run out of blocks 
	uint64_t ret = iter->start_lba;
	mark_used_blk(iter, num_blks); //in case iter is modified

	return ret; //just return lba ?
}

/*
 * coalesce free block info w/ entry ent
 */
static void coalesce_blks(lba_t lba, lbasz_t count, struct freelist_ent *ent){
	//ent->start_lba = lba;
	ent->lba_count += count;
	if (lba < ent->start_lba)
		ent->start_lba = lba;

}

static void coalesce_entries(struct freelist_ent *first, struct freelist_ent *second){

	//assert(first->start_lba + first->lba_count == second->start_lba);
	
	assert(first->next == second); //consecutive entries
	first->lba_count += second->lba_count;
	first->next = second->next;

	free(second);
}

/*
 * Free lba range (assuming freelist organized in ASCENDING order)
 * NOTE: b/c we allocate from ahead, coalesce from opposite direction
 */ 
void free_blk(lba_t lba, lbasz_t num_blks){

	assert(lba < MAX_LBA_NUM);

	struct freelist_ent *prev = NULL; 
	struct freelist_ent *iter = freelist;
	while (iter){ //find where to insert free block
		if (lba < iter->start_lba)
			break;
		prev = iter;
		iter = iter->next;
	}
	
	
	if (prev && prev->start_lba + prev->lba_count == lba) //coalesce info instead of making a new block
		coalesce_blks(lba, num_blks, prev);
	else if (lba + num_blks == iter->start_lba)
		coalesce_blks(lba, num_blks, iter);
	else {
		struct freelist_ent *newent = malloc(sizeof(struct freelist_ent));
		newent->start_lba = lba;
		newent->lba_count = num_blks;
				
		if (prev && prev->start_lba < lba && lba < iter->start_lba){
				//newent->next = iter;
			prev->next = newent;
		}
		newent->next = iter;
		//prev->next = newent;
		
		if (iter == freelist){ //stopped at front
			freelist = newent;
			//newent->next = prev;
		}
			
	}
	if (prev && prev->start_lba + prev->lba_count == iter->start_lba) //double-ended coalescing
		coalesce_entries(prev, iter);

}

/*
 * Initialize freelist to entire space
 * TODO: will this be per-core or global..?
 */ 
void freelist_init(){
	freelist = malloc(sizeof(struct freelist_ent));

	freelist->start_lba = 0;
	freelist->lba_count = MAX_LBA_NUM;
	freelist->next = NULL;

}

//FOR DEBUGGING ONLY
void print_freelist(){

	printf("DEBUG: List of free blocks:\n");
	struct freelist_ent *tmp = freelist;
	while (tmp){
		printf("Block # %lu for %lu blocks\n", tmp->start_lba, tmp->lba_count);
		tmp = tmp->next;
	}
}



