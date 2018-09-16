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
	U64 finalKey = 0;

	// Pieces
	for (int sq = 21; sq < 99; ++sq) {
		int piece = b.mailbox[sq];
		if (piece == 0 || piece == b.NN) {
			continue;
		}
		piece = (piece > 0) ? piece - 1 : -piece + 5;
		finalKey ^= pieceKeys[piece][b.mailbox120[sq]];
	}
	
	// Side to move
	if (b.getTurn() == b.WHITE) {
		finalKey ^= sideKey;
	}

	// En passant
	b.setEnPassantSquare();
	if (b.epSquare != -1) {
		finalKey ^= epKey;
	}

	// Castling
	int forfeit = b.checkCastlingForfeit();
	for (int bit = 0; bit < 4; ++bit, forfeit >>= 1) {
		if ((forfeit & 1) == 1) {
			finalKey ^= castlingKey[bit];
		}
	}

	return finalKey;
}
U64 Hash::updatePosKey(Board&b, const U64 key, const Move m, const int captured_piece) {
	U64 finalKey = key;

	finalKey ^= sideKey;

	int piece = b.mailbox[m.to];
	piece = (piece > 0) ? piece - 1 : -piece + 5;
	finalKey ^= pieceKeys[piece][b.mailbox120[m.from]];
	if (captured_piece != 0) {
		piece = (captured_piece > 0) ? captured_piece - 1 : -captured_piece + 5;
		finalKey ^= pieceKeys[captured_piece][b.mailbox120[m.to]];
	}

	b.setEnPassantSquare();
	if (b.epSquare != -1) {
		finalKey ^= epKey;
	}

	int forfeit = b.checkCastlingForfeit();
	for (int bit = 0; bit < 4; ++bit, forfeit >>= 1) {
		if ((forfeit & 1) == 1) {
			finalKey ^= castlingKey[bit];
		}
	}

	return finalKey;
}