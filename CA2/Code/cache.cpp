#include "cache.h"
cache::cache()
{

	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 

	// Do the same for Victim Cache ...
	for (int i=0; i<VICTIM_SIZE; i++) 
		Victim[i].valid = false;

	myStat.missL1 = 0;
	myStat.missL2 = 0;
	myStat.accL1 = 0;
	myStat.accL2 = 0;

	// Add stat for Victim cache ... 
	myStat.accVictim = 0;
	myStat.missVictim = 0;

	myStat.hitL1 = 0;
	myStat.hitL2 = 0;
	myStat.hitVictim = 0;

	
}

void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	// add your code here
	// Either complete load or store algorithm
	if (MemR) {
		load(data, adr, myMem);
	} else if (MemW) {
		store(data, adr, myMem);
	}
}

void cache::load(int* data, int adr, int* myMem) {

	//cout << "Handling load for " << adr << endl;
	int result;

	unsigned int index;
	unsigned int offset;

	// Perform Access to L1 Cache
	myStat.accL1 += 1;
	result = accessL1(adr, index, offset);
	if (result > 0) {
		//cout << "Hit in L1, ";
		cacheBlock cb = L1[index];
		*data = cb.data >> (offset * 8) & 0xFF;
		return;
	}
	//cout << "Miss in L1, ";
	myStat.missL1 += 1;

	// Perform Access to Victim (if miss previously)
	myStat.accVictim += 1;
	result = accessVictim(adr, index, offset);
	if (result > 0) {
		//cout << "Hit in Victim, ";
		*data = Victim[index].data >> (offset * 8) & 0xFF;

		// Copy correct block into L1, and replace into Victim
		cacheBlock cb_L1 = insertL1(data, adr);
		cacheBlock cb_Victim = insertVictim(cb_L1, index);
		return;
	}
	//cout << "Miss in Victim, ";
	myStat.missVictim += 1;

	// Perform Access to L2 (if miss previously)
	unsigned int line;
	myStat.accL2 += 1;
	result = accessL2(adr, index, line, offset);
	if (result > 0) {
		//cout << "Hit in L2, ";
		cacheBlock cb = L2[index][line];
		*data = cb.data >> (offset * 8) & 0xFF;

		// Copy correct block into L1, and replace into Victim, and replace into L2
		cacheBlock cb_L1 = insertL1(data, adr);
		cacheBlock cb_Victim = insertVictim(cb_L1, -1);
		cacheBlock cb_L2 = insertL2(cb_Victim, index, line, adr);
		return;
	}
	//cout << "Miss in L2, ";
	myStat.missL2 += 1;

	// Access to memory (if miss previously)
	*data = myMem[adr] >> (offset * 8) & 0xFF;

	// Insert correct block into L1, and replace into
	cacheBlock cb_L1 = insertL1(data, adr);

	cacheBlock cb_Victim = insertVictim(cb_L1, -1);

	cacheBlock cb_L2 = insertL2(cb_Victim, -1, -1, adr);



}

void cache::store(int* data, int adr, int* myMem) {
	int result;
	unsigned int index;
	unsigned int offset;

	// Update data if exist in L1, Victim, or L2 cache
	unsigned int line;
	if (accessL1(adr, index, offset) > 0) {
		L1[index].data = *data;
	} else if (accessVictim(adr, index, offset) > 0) {
		Victim[index].data = *data;

		for (int i = 0; i < VICTIM_SIZE; i++) {
			if (Victim[index].lru_position > Victim[index].lru_position)
				Victim[index].lru_position -= 1;
		}

		Victim[index].lru_position = VICTIM_SIZE - 1;

	} else if (accessL2(adr, index, line, offset) > 0) {
		L2[index][line].data = *data;

		for (int i = 0; i < VICTIM_SIZE; i++) {
			if (L2[index][line].lru_position > Victim[index].lru_position)
				L2[index][line].lru_position -= 1;
		}

		L2[index][line].lru_position = L2_CACHE_WAYS - 1;
	}

	// Always perform update to myMem
	myMem[adr] = *data;
}

int cache::accessL1 (int adr, unsigned int& index, unsigned int& offset) {

	offset = adr & 0x3; // 0x3 is 0b11
	index = (adr >> 2) & 0xF; // 0xF is 0b1111
	unsigned int tag = adr >> 2;

	// Check if block is valid and matches the tag
	if (L1[index].valid && L1[index].tag == tag) {
		return 1;
	}
	
	return -1;
}

int cache::accessVictim (int adr, unsigned int& index, unsigned int& offset) {
	// Update Stats

	offset = adr & 0x3; // 0x3 is 0b11
	unsigned int tag = adr >> 2;

	// Loop through victim and check if a valid block matches tag
	for (unsigned int i = 0; i < VICTIM_SIZE; i++) {
		if (Victim[i].valid && Victim[i].tag == tag) {
			index = i;
			return 1;
		}
	}
	
	return -1;
}

int cache::accessL2(int adr, unsigned int& index, unsigned int& line, unsigned int& offset) {

	offset = adr & 0x3; // 0x3 is 0b11
	index = (adr >> 2) & 0xF; // 0xF is 0b1111
	unsigned int tag = adr >> 2;

	// After calculating proper index, loop through the set and check if valid block matches the tag
	for (unsigned l = 0; l < L2_CACHE_WAYS; l++) {
		if (L2[index][l].valid && L2[index][l].tag == tag) {
			line = l;
			return 1;
		}
	}

	return -1;
}

cacheBlock cache::insertL1(int* data, int adr) {

	unsigned int index = (adr >> 2) & 0xF; // 0xF is 0b1111
	unsigned int tag = adr >> 2;

	// Find the removed cacheBlock
	cacheBlock remove_cb = L1[index];

	// Insert new cacheBlock
	L1[index].tag = tag;
	L1[index].data = *data;
	L1[index].valid = true;



	return remove_cb;
}

cacheBlock cache::insertVictim(cacheBlock cb, unsigned index) {
	// Shouldn't insert if cacheBlock is not valid
	if (!cb.valid) return cb;

	// Need to find invalid block or LRU block if not copying block out of Victim
	if (index == -1) {
		for (unsigned int i = 0; i < VICTIM_SIZE; i++) {
			if (!Victim[i].valid) {
				index = i;
				break;
			}
		}
	}
	if (index == -1) {
		unsigned int lru_i = -1;
		for (unsigned int i = 0; i < VICTIM_SIZE; i++) {
			if (!Victim[i].valid)  continue;
			if (lru_i == -1 || Victim[i].lru_position < Victim[lru_i].lru_position) lru_i = i;
		}
		index = lru_i;
	}

	// Get the removed block
	cacheBlock remove_cb = Victim[index];

	// Update the LRU positions
	if (!remove_cb.valid) {
		for (int i = 0; i < VICTIM_SIZE; i++) {
			if (Victim[i].lru_position > remove_cb.lru_position)
				Victim[i].lru_position -= 1;
		}
	}

	// Insert new block into Victim
	cb.lru_position = VICTIM_SIZE - 1;
	Victim[index] = cb;

	return remove_cb;
}

cacheBlock cache::insertL2(cacheBlock cb, unsigned index, unsigned line, unsigned adr) {
	// Shouldn't insert if cacheBlock is not valid
	if (!cb.valid) return cb;
	
	// Need to find invalid block or LRU block if not copying block out of L2
	if (index == -1) {
		unsigned tag = cb.tag;
		index = tag & 0xF;
	}

	if (line == -1) {
		for (unsigned int l = 0; l < L2_CACHE_WAYS; l++) {
			if (!L2[index][l].valid) {
				line = l;
				break;
			}
		}
	}

	if (line == -1) {
		unsigned int lru_l = -1;
		for (unsigned int l = 0; l < L2_CACHE_WAYS; l++) {
			if (!L2[index][l].valid) continue;
			if (lru_l == -1 || L2[index][l].lru_position < L2[index][lru_l].lru_position) lru_l = l;
		}
		line = lru_l;
	}

	// Get the removed block
	cacheBlock remove_cb = L2[index][line];

	// Update all LRU positions
	if (remove_cb.valid) {
		for (int l = 0; l < L2_CACHE_WAYS; l++) {
			if (L2[index][l].valid && L2[index][l].lru_position > remove_cb.lru_position)
				L2[index][l].lru_position -= 1;
		}
	}

	// Set this place that hold removed block to be invalid now
	L2[index][line].valid = false;

	// Find the spot [new_index, new_line] to insert the new cache block
	cb.lru_position = L2_CACHE_WAYS - 1;
	unsigned new_index = cb.tag & 0xF;
	unsigned new_line = -1;

	if (new_line == -1) {
		for (unsigned int l = 0; l < L2_CACHE_WAYS; l++) {
			if (!L2[new_index][l].valid) {
				new_line = l;
				break;
			}
		}
	}

	if (new_line == -1) {
		unsigned int lru_l = -1;
		for (unsigned int l = 0; l < L2_CACHE_WAYS; l++) {
			if (!L2[new_index][l].valid) continue;
			if (lru_l == -1 || L2[new_index][l].lru_position < L2[new_index][lru_l].lru_position) lru_l = l;
		}
		new_line = lru_l;
	}

	// Insert the new cacheBlock
	L2[new_index][new_line] = cb;

	return remove_cb;
}

double cache::get_L1_miss_rate() {
	return (double) myStat.missL1 / myStat.accL1;
}

double cache::get_L2_miss_rate() {
	return (double) myStat.missL2 / myStat.accL2;
}

double cache::get_Victim_miss_rate() {
	return (double) myStat.missVictim / myStat.accVictim;
}
