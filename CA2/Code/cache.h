#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
};

struct Stat
{
	int missL1; 
	int missL2; 
	int accL1;
	int accL2;
	int accVictim;
	int missVictim;

	// add more stat if needed. Don't forget to initialize!
	int hitL1;
	int hitL2;
	int hitVictim;

};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	cacheBlock Victim[VICTIM_SIZE];
	
	Stat myStat;
	// add more things here
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	void load(int* data, int adr, int* myMem);
	void store(int* data, int adr, int* myMem);

	int accessL1(int adr, unsigned int& index, unsigned int& offset);
	int accessVictim(int adr, unsigned int& index, unsigned int& offset);
	int accessL2(int adr, unsigned int& index, unsigned int& line, unsigned int& offset);

	cacheBlock insertL1(int* data, int adr);
	cacheBlock insertVictim(cacheBlock cb, unsigned index);
	cacheBlock insertL2(cacheBlock cb, unsigned index, unsigned line, unsigned adr);

	double get_L1_miss_rate();
	double get_L2_miss_rate();
	double get_Victim_miss_rate();


	// add more functions here ...	
};


