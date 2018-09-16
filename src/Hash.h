#ifndef HASH_INCLUDED
#define HASH_INCLUDED

#include "Board.h"
#include <random>
#include <unordered_map>

typedef unsigned long long int U64;

struct ttInfo {
	ttInfo() {}
	ttInfo(int a, int b, int c, int d) : 
		depth(a), score(b), flag(c), age(d) {}

	int depth, score, flag, age;
	int eval = 100000;
	Move hashBestMove = Move(-9, -9);
};

class Hash {

public:

	Hash() { initHashKeys(); }

	enum hashFlags { HASH_EXACT = 0, HASH_BETA = 1, HASH_ALPHA = 2 };

	U64 generatePosKey(Board&);
	U64 updatePosKey(Board&, const U64, const Move, const int);

	std::unordered_map<U64, ttInfo> hashTable;

private:

	U64 generateRand64();
	void initHashKeys();

	U64 pieceKeys[12][64];
	U64 sideKey;
	U64 epKey;
	U64 castlingKey[4];

};

#endif