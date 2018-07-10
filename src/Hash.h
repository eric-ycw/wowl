#ifndef HASH_INCLUDED
#define HASH_INCLUDED

#include "Evaluation.h"
#include <random>
#include <unordered_map>

typedef unsigned long long int U64;

class Hash {

public:

	//Constructor
	Hash() { initHashKeys(); }

	U64 generateRand64();
	void initHashKeys();
	U64 generatePosKey(Board&);

	//Key is U64, best move is int
	std::unordered_map<U64, int> table;

private:

	U64 pieceKeys[12][64];
	U64 sideKey;
	U64 epKey;
	U64 castlingKey[4];

};

#endif