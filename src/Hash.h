#ifndef HASH_INCLUDED
#define HASH_INCLUDED

#include "Evaluation.h"
#include <random>
#include <unordered_map>

typedef unsigned long long int U64;

struct HashVal {
	HashVal() {}
	HashVal(int depth, int score, int flag, int age) : 
		hashDepth(depth), hashScore(score), hashFlag(flag), hashAge(age) {}

	int hashDepth, hashScore, hashFlag, hashAge;
	Move hashBestMove = Move(-9, -9);
};

class Hash {

public:

	Hash() { initHashKeys(); }

	enum hashFlags { HASH_EXACT = 0, HASH_BETA = 1, HASH_ALPHA = 2 };

	U64 generateRand64();
	void initHashKeys();
	U64 generatePosKey(Board&);
	U64 updatePosKey(Board&, const U64, const Move, const int);

	std::unordered_map<U64, HashVal> tt;

private:

	U64 pieceKeys[12][64];
	U64 sideKey;
	U64 epKey;
	U64 castlingKey[4];

};

#endif