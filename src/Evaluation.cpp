#include "Evaluation.h"

double Evaluation::getPhase(const Board& b) {
	int baseMaterial = (pieceValues[b.WN - 1] + pieceValues[b.WB - 1] + pieceValues[b.WR - 1]) * 4 + pieceValues[b.WQ - 1] * 2;
	int nonPMaterial = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.OOB || piece == 0 || piece == b.WP || piece == b.BP || piece == b.WK || piece == b.BK) { continue; }
		nonPMaterial += pieceValues[abs(piece) - 1];
	}
	return double(nonPMaterial) / baseMaterial;
}

int Evaluation::blockedPawns(const Board& b) {
	int bpcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP && b.mailbox[i - 10] == b.BP 
			&& b.mailbox[i - 11] != b.BP && b.mailbox[i - 9] != b.BP
			&& b.mailbox[i - 1] != b.WP && b.mailbox[i + 1] != b.WP) {
			bpcount += 2;
		}
	}
	return bpcount;
}
int Evaluation::doubledAndIsolatedPawns(const Board& b, int color) {
	int filearray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int dpcount = 0;
	int ipcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			filearray[i % 10 - 1]++;
		}
	}
	for (int i = 0; i < 8; ++i) {
		if (filearray[i] == 2) {
			dpcount++;
		}
		else if (filearray[i] > 2) {
			//Additional penalty for tripled pawns
			dpcount += 2;
		}
		if (i >= 1 && i <= 6) {
			if (filearray[i] == b.WP * color && filearray[i - 1] == 0 && filearray[i + 1] == 0) {
				ipcount++;
			}
		}
		else if (i = 0) {
			if (filearray[0] == b.WP * color && filearray[1] == 0) {
				ipcount++;
			}
		}
		else if (i = 7) {
			if (filearray[7] == b.WP * color && filearray[6] == 0) {
				ipcount++;
			}
		}
	}
	return dpcount * DOUBLED_P_PENALTY + ipcount * ISOLATED_P_PENALTY;
}
int Evaluation::connectedPawns(const Board&b, int color) {
	int supported = 0;
	int phalanx = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			supported += b.mailbox[i + color * 10 + 1] == b.WP * color ? 1 : 0
				+ b.mailbox[i + color * 10 - 1] == b.WP * color ? 1 : 0;
			phalanx += b.mailbox[i + 1] == b.WP * color ? 1 : 0
				+ b.mailbox[i - 1] == b.WP * color ? 1 : 0;
		}
	}
	return supported * SUPPORTED_P_BONUS + phalanx * PHALANX_P_BONUS;
}
int Evaluation::backwardPawns(const Board& b, int color) {
	int bpcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] != b.WP * color) { continue; }
		if (b.mailbox[i - 10 * color] == b.WP * -color || !attackedByEnemyPawn(b, i - 10 * color, color)) { continue; }

		int j = i - 1;
		bool backward = true;
		while (b.mailbox[j] != b.OOB) {
			if (b.mailbox[j] == b.WP * color) {
				backward = false;
				break;
			}
			j += color * 10;
		}
		if (backward) {
			j = i + 1;
			while (b.mailbox[j] != b.OOB) {
				if (b.mailbox[j] == b.WP * color) {
					backward = false;
					break;
				}
				j += color * 10;
			}
		}
		if (backward) { bpcount++; }
	}
	return bpcount * BACKWARD_P_PENALTY;
}
int Evaluation::passedPawns(const Board& b, int color) {
	int pcount = 0;
	int rank, file;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			file = i % 10;
			rank = (i - file) / 10;
			if (isPassed(b, i, color)) {
				int reverse_dis = color == b.WHITE ? 9 - rank : rank - 2;
				//Reduce rank by one if blocked
				if (b.mailbox[i - color * 10] != 0) { reverse_dis -= 1; }
				if (reverse_dis < 5) {
					pcount += reverse_dis * 2 * PASSED_P_BONUS;
				}
				else if (reverse_dis == 5) {
					pcount += pow(reverse_dis, 2) * PASSED_P_BONUS;
				}
				else {
					pcount += pow(reverse_dis, 2.25) * PASSED_P_BONUS;
				}
			}
		}
	}
	return static_cast<int>(pcount * PASSED_P_BONUS);
}

int Evaluation::material(const Board& b, int square, int color) {
	return pieceValues[abs(b.mailbox[square]) - 1];
}

int Evaluation::knightOutpost(const Board& b, int square, int color) {
	int support = 0;
	int val = 0;
	if (b.mailbox[square + 11 * color] == b.WP * color) {
		support++;
	}
	if (b.mailbox[square + 9 * color] == b.WP * color) {
		support++;
	}
	if (support > 0) {
		bool secure[2] = { true, true };
		for (int j = square - 1 - 10 * color; b.mailbox[j] != b.OOB; j -= 10 * color) {
			if (b.mailbox[j] == b.WP * -color) { secure[0] = false; }
		}
		for (int j = square + 1 - 10 * color; b.mailbox[j] != b.OOB; j -= 10 * color) {
			if (b.mailbox[j] == b.WP * -color) { secure[1] = false; }
		}
		if (secure[0] && secure[1]) {
			val += support * KNIGHT_OUTPOST_BONUS;
		}
	}
	return val;
}
int Evaluation::rookBehindPassed(const Board& b, int square, int color) {
	int rcount = 0;
	int sqr = square;
	while (sqr >= 31 && sqr <= 88) {
		//Supporting friendly passed pawn or blocking enemy passed pawn
		sqr += -color * 10;
		if (isPassed(b, sqr, color) || isPassed(b, sqr, -color)) {
			rcount += 1;
			break;
		}
	}
	while (sqr >= 31 && sqr <= 88) {
		//Behind enemy passed pawn
		sqr += color * 10;
		if (isPassed(b, sqr, -color)) {
			rcount += 1;
			break;
		}
	}
	return rcount * ROOK_BEHIND_PASSED_P_BONUS;
}
int Evaluation::trappedRook(const Board&b, int square, int color) {
	int blocked = 0;
	int castle_id = (color == b.WHITE) ? 0 : 1;
	if (b.mailbox[square + 1] != 0) {
		blocked++;
	}
	if (b.mailbox[square - 1] != 0) {
		blocked++;
	}
	if (b.mailbox[square + 10] != 0) {
		blocked++;
	}
	if (b.mailbox[square - 10] != 0) {
		blocked++;
	}
	if (blocked > 1) {
		return blocked * TRAPPED_ROOK_PENALTY;
	}
	else {
		return 0;
	}
}

int Evaluation::flipTableValue(int square) const {
	return (11 - square / 10) * 10 + square % 10;
}
int Evaluation::PST(const Board& b, int square, int color) {
	int val = 0;
	int coord = b.to64Coord((color == b.WHITE) ? square : flipTableValue(square));
	switch (abs(b.mailbox[square])) {
	case b.WP:
		val = pawnTable[coord] * phase;
		break;
	case b.WN:
		val = knightTable[coord];
		break;
	case b.WB:
		val = bishopTable[coord];
		break;
	case b.WR:
		val = rookTable[coord];
		break;
	case b.WQ:
		val = queenTable[coord];
		break;
	case b.WK:
		val = static_cast<int>((kingTable[coord] * phase + kingEndTable[coord] * (1 - phase)));
		break;
	}
	return val;
}

int Evaluation::mobilityKnight(const Board&b, int square, int color) {
	int mobility = 0;
	for (const int i : b.pieceMoves[1]) {
		if (!i) { break; }
		int toVal = b.mailbox[square + i];
		if (toVal * color > 0 || toVal == b.OOB || !attackedByEnemyPawn(b, square + i, color)) {
			continue;
		}
		mobility++;
	}
	return mobility;
}
int Evaluation::mobilitySlider(const Board&b, int square, int color, int piece) {
	int mobility = 0;
	assert(piece == b.WB || piece == b.WR || piece == b.WQ);
	for (const int i : b.pieceMoves[piece - 1]) {
		if (!i) { break; }
		for (int j = 0; j < 8; ++j) {
			int to = square + (j + 1) * i;
			int toVal = b.mailbox[to];
			if (toVal * color > 0 || toVal == b.OOB || !attackedByEnemyPawn(b, square + i, color)) {
				break;
			}
			else {
				mobility++;
				if (toVal * color < 0) { break; }
			}
		}
	}
	return mobility;
}

int Evaluation::spaceArea(const Board&b, int color) {
	int file;
	int rank;
	int sval = 0;
	int pieces = 0;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.OOB) { continue; }
		if (piece * color > 0) {
			pieces++;
		}
	}
	for (int i = 43; i < 77; ++i) {
		if (i % 10 < 3 || i % 10 > 6) {
			continue;
		}
		if (b.mailbox[i - color * 10 + 1] == -color * b.WP || b.mailbox[i - color * 10 - 1] == -color * b.WP) {
			continue;
		}
		sval++;
		//Increase space value if square is behind own pawn by 1-3 squares
		for (int j = 0; j < 2; ++j) {
			if (b.mailbox[i - color * 10 * j] == color * b.WP) {
				sval++;
				break;
			}
		}
	}
	return static_cast<int>(sval * pieces * 1.5);
}

int Evaluation::kingShelter(Board& b, int color) {
	int rval = 0;
	int pval = 0;
	int cval = 0;
	int kingsqr = (color == b.WHITE) ? b.kingSquare[0] : b.kingSquare[1];

	//Open files
	rval = isOpenFile(b, kingsqr) + isOpenFile(b, kingsqr + 1) * 0.75 + isOpenFile(b, kingsqr - 1) * 0.75;
	//Pawn shield in front of king
	if (b.mailbox[kingsqr- 10 * color] != b.WP * color) {
		pval++;
	}
	if (b.mailbox[kingsqr - 11 * color] != b.WP * color) {
		pval++;
	}
	if (b.mailbox[kingsqr - 9 * color] != b.WP * color) {
		pval++;
	}

	return static_cast<int>((rval * K_OPEN_FILE_PENALTY + pval * K_P_SHIELD_PENALTY) * phase);
}
int Evaluation::kingDangerProximity(Board& b, int color) {
	int kingsqr = (color == b.WHITE) ? b.kingSquare[0] : b.kingSquare[1];
	int kingRing[24] = {
		1, -1, 10, -10, 11, 9, -11, 9,
		2, -2, 20, -20, 22, 18, -22, 18,
		12, 8, -12, -8, 21, -21, 19, -19
	};
	int attack = 0;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.OOB || piece * color >= 0) { continue; }
		switch (piece * color) {
		case b.BN:
			for (int j = 0; j < 24; ++j) {
				if (b.checkAttackKnight(i, i + kingRing[j])) {
					attack += KNIGHT_KING_ATTACKER;
					break;
				}
			}
			break;
		case b.BB:
			for (int j = 0; j < 24; ++j) {
				if (b.checkAttackSlider(i, i + kingRing[j], b.WB)) {
					attack += BISHOP_KING_ATTACKER;
					break;
				}
			}
			break;
		case b.BR:
			for (int j = 0; j < 24; ++j) {
				if (b.checkAttackSlider(i, i + kingRing[j], b.WR)) {
					attack += ROOK_KING_ATTACKER;
					break;
				}
			}
			break;
		case b.BQ:
			for (int j = 0; j < 24; ++j) {
				if (b.checkAttackSlider(i, i + kingRing[j], b.WQ)) {
					attack += QUEEN_KING_ATTACKER;
					break;
				}
			}
			break;
		}
	}
	return attack;
}

int Evaluation::pawnPushThreat(const Board& b, int square, int color) {
	if (b.mailbox[square - 19 * color] == b.WP * -color && attackedByEnemyPawn(b, square - 9 * color, color)) {
		return PAWN_PUSH_THREAT_PENALTY;
	}
	if (b.mailbox[square - 21 * color] == b.WP * -color && attackedByEnemyPawn(b, square - 11 * color, color)) {
		return PAWN_PUSH_THREAT_PENALTY;
	}
	return 0;
}

bool Evaluation::attackedByEnemyPawn(const Board& b, int square, int color) {
	if ((b.mailbox[square - 11 * color] == b.WP * -color) || (b.mailbox[square - 9 * color] == b.WP * -color)) {
		return true;
	}
	return false;
}
int Evaluation::isOpenFile(const Board& b, int square) {
	int file = square % 10;
	int pcount = 0;
	if (b.mailbox[square] != b.OOB) {
		for (int i = 2; i <= 9; ++i) {
			pcount = 0;
			if (abs(b.mailbox[i * 10 + file] == b.WP)) {
				pcount++;
			}
		}
	}
	if (pcount > 1) {
		return 0;
	}
	else if (pcount == 1) {
		return 1;
	}
	else if (pcount == 0) {
		return 2;
	}
}
int Evaluation::isPassed(const Board&b, int square, int color) {
	bool passed;
	int rank, file;
	if (b.mailbox[square] == b.WP * color) {
		passed = true;
		file = square % 10;
		rank = (square - file) / 10;
		int sqr = square;
		while (sqr >= 31 && sqr <= 88) {
			sqr += -color * 10;
			if (b.mailbox[sqr] == b.WP * -color || b.mailbox[sqr - 1] == b.WP * -color || b.mailbox[sqr + 1] == b.WP * -color) {
				passed = false;
			}
		}
		if (passed) {
			return true;
		}
	}
	return false;
}

int Evaluation::piecesAndMobility(const Board&b, int color) {
	//Mobility and various piece bonuses
	int val = 0;
	int bcount = 0;
	int blocked = blockedPawns(b);
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		int abs_piece = color * piece;
		if (piece == b.OOB || abs_piece <= 0) { continue; }
		if (abs_piece != b.WP) { val += pawnPushThreat(b, i, color); }
		if (abs_piece == b.WN) {
			val += blocked * OPEN_CLOSED_POS_PIECE_VALUE;
			val += MINOR_BEHIND_PAWN_BONUS * (b.mailbox[i - 10 * color] == b.WP * color);
			val += knightOutpost(b, i, color);
			val += knightMobilityTable[mobilityKnight(b, i, color)];
		}
		else if (abs_piece == b.WB) {
			val -= blocked * OPEN_CLOSED_POS_PIECE_VALUE;
			val += MINOR_BEHIND_PAWN_BONUS * (b.mailbox[i - 10 * color] == b.WP * color);
			val += bishopMobilityTable[mobilitySlider(b, i, color, b.WB)];
			bcount++;
		}
		else if (abs_piece == b.WR) {
			val += isOpenFile(b, i) * R_OPEN_FILE_BONUS;
			val += rookBehindPassed(b, i, color) + trappedRook(b, i, color);
			val += rookMobilityTable[mobilitySlider(b, i, color, b.WR)];
		}
		else if (abs_piece == b.WQ) {
			val += queenMobilityTable[mobilitySlider(b, i, color, b.WQ)];
		}
		
	}
	if (bcount >= 2) {
		val += BISHOP_PAIR_BONUS;
	}
	return val;
}

int Evaluation::totalEvaluation(Board& b, int color, int lazyScore[]) {
	phase = getPhase(b);
	int lazyIndex = !(color == b.WHITE);
	int lazy = lazyScore[lazyIndex] - lazyScore[!lazyIndex];
	int pieces = piecesAndMobility(b, color) - piecesAndMobility(b, -color);
	int pawns = doubledAndIsolatedPawns(b, color) + connectedPawns(b, color) + backwardPawns(b, color) + passedPawns(b, color)
		- doubledAndIsolatedPawns(b, -color) - connectedPawns(b, -color) - backwardPawns(b, -color) - passedPawns(b, -color);
	int space = spaceArea(b, color) - spaceArea(b, -color);
	int king = kingShelter(b, color) - kingShelter(b, -color) + kingDangerProximity(b, color) - kingDangerProximity(b, -color);
	return lazy + pieces + pawns + space + king;
}
int Evaluation::lazyEvaluation(const Board& b, int color) {
	phase = getPhase(b);
	int val = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		int abs_piece = color * piece;
		if (piece == b.OOB || abs_piece <= 0 || abs_piece == b.WK) { continue; }
		val += material(b, i, color) + PST(b, i, color);
	}
	return val;
}