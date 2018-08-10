#include "Hash.h"

U64 Hash::generateRand64() {
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<long long int> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));
	return dist(gen);
}
void Hash::initHashKeys() {
	for (int i = 0; i < 12; ++i) {
		for (int j = 0; j < 64; ++j) {
			pieceKeys[i][j] = generateRand64();
		}
	}
	sideKey = generateRand64();
	for (int i = 0; i < 4; ++i) {
		castlingKey[i] = generateRand64();
	}
}
U64 Hash::generatePosKey(Board& b) {

	int sqval, piece, coord;
	U64 finalKey = 0;

	//Pieces
	for (int sq = 21; sq < 99; ++sq) {
		piece = b.mailbox[sq];
		if (piece == 0 || piece == -9) {
			continue;
		}
		if (piece > 0) {
			piece -= 1;
		}
		else {
			piece = piece * -1 + 5;
		}
		coord = b.to64Coord(sq);
		finalKey ^= pieceKeys[piece][coord];
	}
	
	//Side to move
	if (b.getTurn() == b.WHITE) {
		finalKey ^= sideKey;
	}

	//En passant
	b.setEnPassantSquare();
	if (b.epSquare != -1) {
		finalKey ^= epKey;
	}

	//Castling
	std::tuple<bool, bool, bool, bool> castlingRights = b.checkCastling();
	if (std::get<0>(castlingRights)) {
		finalKey ^= castlingKey[0];
	}
	if (std::get<1>(castlingRights)) {
		finalKey ^= castlingKey[1];
	}
	if (std::get<2>(castlingRights)) {
		finalKey ^= castlingKey[2];
	}
	if (std::get<3>(castlingRights)) {
		finalKey ^= castlingKey[3];
	}

	return finalKey;
}