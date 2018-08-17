#include "Evaluation.h"

double Evaluation::getPhase(const Board& b) {
	int baseMaterial = (pieceValues[b.WN - 1] + pieceValues[b.WB - 1] + pieceValues[b.WR - 1]) * 4 + pieceValues[b.WQ - 1] * 2;
	int nonPMaterial = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == -9 || piece == 0 || piece == b.WP || piece == b.BP || piece == b.WK || piece == b.BK ) { continue; }
		nonPMaterial += pieceValues[abs(piece) - 1];
	}
	return double(nonPMaterial) / baseMaterial;
}

int Evaluation::blockedPawns(const Board& b) {
	int bpcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP && b.mailbox[i - 10] ==  b.BP && b.mailbox[i - 11] >= 0 && b.mailbox[i - 9] >= 0) {
			bpcount++;
		}
		if (b.mailbox[i] ==  b.BP && b.mailbox[i + 10] == b.WP && b.mailbox[i + 11] <= 0 && b.mailbox[i + 9] <= 0) {
			bpcount++;
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
	int rankArray[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int squareArray[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int bpcount = 0;
	int rank, file;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			file = i % 10;
			rank = (i - file) / 10;
			assert(file - 1 >= 0 && file - 1 <= 7);
			if (rankArray[file - 1] == -1) {
				rankArray[file - 1] = rank;
				squareArray[file - 1] = i;
			}
			else {
			//Record rear pawn for doubled/tripled pawns
				if (rankArray[file - 1] * color < rank * color) {
					rankArray[file - i] = rank;
					squareArray[file - 1] = i;
				}
			}
		}
	}
	for (int i = 0; i < 8; ++i) {
		if (rankArray[i] != -1) {
			if (rankArray[std::max(0, i - 1)] * color < rankArray[i] * color && rankArray[std::min(7, i + 1)] * color < rankArray[i] * color) {
				if (b.mailbox[squareArray[i] - color * 20 + 1] == b.WP * -color || b.mailbox[squareArray[i] - color * 20 - 1] == b.WP * -color) {
					bpcount++;
				}
			}
		}
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
				else {
					pcount += pow(reverse_dis, 2.5) * PASSED_P_BONUS;
				}
			}
		}
	}
	return pcount * PASSED_P_BONUS;
}

int Evaluation::baseMaterial(const Board& b, int color) {
	int mval = 0;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == -9) { continue; }
		if (color * b.mailbox[i] > 0) {
			mval += pieceValues[abs(b.mailbox[i]) - 1];
		}
	}
	return mval;
}
int Evaluation::structureMaterial(const Board& b, int color) {
	int mval = 0;
	for (int i = 21; i < 99; ++i) {
		if (color * b.mailbox[i] > 0) {
			switch (abs(b.mailbox[i])) {
			case b.WN:
				//Knights are better in closed positions
				mval += blockedPawns(b) * OPEN_CLOSED_POS_PIECE_VALUE;
				if (b.mailbox[i - 10 * color] == b.WP * color) {
					mval += MINOR_BEHIND_PAWN_BONUS;
				}
				break;
			case b.WB:
				//Bishops are worse in closed positions
				mval -= blockedPawns(b) * OPEN_CLOSED_POS_PIECE_VALUE;
				if (b.mailbox[i - 10 * color] == b.WP * color) {
					mval += MINOR_BEHIND_PAWN_BONUS;
				}
				break;
			case b.WR:
				//Rooks are better on open and semi-open files
				mval += isOpenFile(b, i) * R_OPEN_FILE_BONUS;
				break;
			}
		}
	}
	return mval;
}

int Evaluation::knightOutpost(const Board& b, int color) {
	int support = 0;
	int val = 0;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == b.WN * color) {
			if (b.mailbox[i + 11 * color] == b.WP * color) {
				support++;
			}
			if (b.mailbox[i + 9 * color] == b.WP * color) {
				support++;
			}
			if (support > 0) {
				bool secure[2] = { true, true };
				for (int j = i - 1 - 10 * color; b.mailbox[j] != -9; j -= 10 * color) {
					if (b.mailbox[j] == b.WP * -color) { secure[0] = false; }
				}
				for (int j = i + 1 - 10 * color; b.mailbox[j] != -9; j -= 10 * color) {
					if (b.mailbox[j] == b.WP * -color) { secure[1] = false; }
				}
				if (secure[0] && secure[1]) {
					val += support * KNIGHT_OUTPOST_BONUS;
				}
			}
		}
	}
	return val;
}
int Evaluation::bishopPair(const Board& b, int color) {
	int mval = 0;
	int bcount = 0;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == b.WB * color) {
			bcount += 1;
		}
	}
	if (bcount >= 2) {
		mval += BISHOP_PAIR_BONUS;
	}
	return mval;
}
int Evaluation::rookBehindPassed(const Board& b, int color) {
	int rcount = 0;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == b.WR * color) {
			int sqr = i;
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
		}
	}
	return rcount * ROOK_BEHIND_PASSED_P_BONUS;
}
int Evaluation::trappedRook(const Board&b, int color) {
	int blocked = 0;
	int castle_id = (color == b.WHITE) ? 0 : 1;

	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == b.WR * color) {
			if (b.mailbox[i + 1] != 0) {
				blocked++;
			}
			if (b.mailbox[i - 1] != 0) {
				blocked++;
			}
			if (b.mailbox[i + 10] != 0) {
				blocked++;
			}
			if (b.mailbox[i - 10] != 0) {
				blocked++;
			}
		}
	}
	if (blocked > 1) {
		return blocked * TRAPPED_ROOK_PENALTY;
	}
	else {
		return 0;
	}
}

int Evaluation::flipTableValue(int square) const {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = (11 - tenth) * 10 + unit;
	return rval;
}
int Evaluation::PST(const Board& b, int color) {
	int val = 0;
	int coord;
	for (int i = 21; i < 99; ++i) {
		if (color * b.mailbox[i] > 0) {
			coord = (color == b.WHITE) ? i : flipTableValue(i);
			if (color == b.WHITE) {
				coord = i;
			}
			else {
				coord = flipTableValue(i);
			}
			switch (abs(b.mailbox[i])) {
			case b.WP:
				val += pawnTable[b.to64Coord(coord)] * phase;
				break;
			case b.WN:
				val += knightTable[b.to64Coord(coord)];
				break;
			case b.WB:
				val += bishopTable[b.to64Coord(coord)];
				break;
			case b.WR:
				val += rookTable[b.to64Coord(coord)];
				break;
			case b.WQ:
				val += queenTable[b.to64Coord(coord)];
				break;
			case b.WK:
				val += static_cast<int>((kingTable[b.to64Coord(coord)] * phase + kingEndTable[b.to64Coord(coord)] * (1 - phase)));
				break;
			}
		}
	}
	return val;
}

int Evaluation::mobilityKnight(const Board&b, int square, int color) {
	int mobility = 0;
	int knightSquares[8] = { -21, -19, -12, -8, 21, 19, 12, 8 };
	for (int i = 0; i < 8; i++) {
		int sqr = square + knightSquares[i];
		if (!attackedByEnemyPawn(b, sqr, color) && b.mailbox[sqr] == 0) {
			mobility++;
		}
	}
	return mobility;
}
int Evaluation::mobilityBishop(const Board&b, int square, int color) {
	int mobility = 0;
	int bishopMoves[4] = { 11, 9, -11, -9 };
	for (int dir = 0; dir < 4; ++dir) {
		for (int i = 0; i < 8; ++i) {
			int sqr = square + (i + 1) * bishopMoves[dir];
			if (b.mailbox[sqr] == 0) {
				if (!attackedByEnemyPawn(b, sqr, color)) {
					mobility++;
				}
			}
			else {
				break;
			}
		}
	}
	return mobility;
}
int Evaluation::mobilityRook(const Board&b, int square, int color) {
	int mobility = 0;
	int rookMoves[4] = { 1, -1, 10, -10 };
	for (int dir = 0; dir < 4; ++dir) {
		for (int i = 0; i < 8; ++i) {
			int sqr = square + (i + 1) * rookMoves[dir];
			if (b.mailbox[sqr] == 0) {
				if (!attackedByEnemyPawn(b, sqr, color)) {
					mobility++;
				}
			}
			else {
				break;
			}
		}
	}
	return mobility;
}
int Evaluation::totalMobility(const Board&b, int color) {
	int total = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece * color == b.WN) {
			total += knightMobilityTable[mobilityKnight(b, i, color)];
		}
		else if (piece * color == b.WB) {
			total += bishopMobilityTable[mobilityBishop(b, i, color)];
		}
		else if (piece * color == b.WR) {
			total += rookMobilityTable[mobilityRook(b, i, color)];
		}
		else if (piece * color == b.WQ) {
			total += queenMobilityTable[mobilityRook(b, i, color) + mobilityBishop(b, i, color)];
		}
	}
	return total;
}

int Evaluation::spaceArea(const Board&b, int color) {
	phase = getPhase(b);
	int file;
	int rank;
	int sval = 0;
	int pieces = 0;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == -9) { continue; }
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
	b.setKingSquare();
	int rval = 0;
	int pval = 0;
	int cval = 0;
	int kingsqr = (color == b.WHITE) ? b.kingSquareWhite : b.kingSquareBlack;
	int castle_id = (color == b.WHITE) ? 0 : 1;

	//Check for forfeit of castling rights
	if (!b.castled[castle_id]) {
		int rank = (color == b.WHITE) ? 9 : 2;
		for (int c = 0; c < b.moveVec.size(); ++c) {
			int oldpos = b.moveVec[c].from;
			if (oldpos == rank * 10 + 5) {
				cval += 2;
				break;
			}
			if (oldpos == rank * 10 + 8 || oldpos == rank * 10 + 1) {
				cval += 1;
			}
		}
		if (cval > 2) cval = 2;
	}
	//Open files
	rval = isOpenFile(b, kingsqr) + isOpenFile(b, kingsqr + 1) * 0.75 + isOpenFile(b, kingsqr - 1) * 0.75;
	//Pawn shield in front of king
	if (b.castled[castle_id]) {
		if (b.mailbox[b.kingSquareWhite - 10 * color] != b.WP * color) {
			pval++;
		}
		if (b.mailbox[b.kingSquareWhite - 11 * color] != b.WP * color) {
			pval++;
		}
		if (b.mailbox[b.kingSquareWhite - 9 * color] != b.WP * color) {
			pval++;
		}
	}

	return static_cast<int>((rval * K_OPEN_FILE_PENALTY + pval * K_P_SHIELD_PENALTY  + cval * CASTLING_RIGHTS_LOSS_PENALTY) * phase);
}
int Evaluation::bishopKingAttack(Board& b, int square, int color) {
	int attack = 0;
	int bishopMoves[4] = { 11, 9, -11, -9 };
	for (int dir = 0; dir < 4; ++dir) {
		for (int i = 0; i < 8; ++i) {
			int sqr = square + (i + 1) * bishopMoves[dir];
			if (b.mailbox[sqr] == 0) {
				if (inKingRing(b, sqr, color)) { attack++; }
			}
			else {
				break;
			}
		}
	}
	return attack;
}
int Evaluation::rookKingAttack(Board& b, int square, int color) {
	int attack = 0;
	int rookMoves[4] = { 1, -1, 10, -10 };
	for (int dir = 0; dir < 4; ++dir) {
		for (int i = 0; i < 8; ++i) {
			int sqr = square + (i + 1) * rookMoves[dir];
			if (b.mailbox[sqr] == 0) {
				if (inKingRing(b, sqr, color)) { attack++; }
			}
			else {
				break;
			}
		}
	}
	return attack;
}
int Evaluation::kingDangerProximity(Board& b, int color) {
	phase = getPhase(b);
	b.setKingSquare();
	int kingsqr = (color == b.WHITE) ? b.kingSquareWhite : b.kingSquareBlack;
	int knightSquares[8] = { -21, -19, -12, -8, 21, 19, 12, 8 };
	int attack = 0;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == -9 || piece == 0) { continue; }
		switch (piece * color) {
		case b.BN:
			for (int j = 0; j < 8; j++) {
				int sqr = i + knightSquares[j];
				if (inKingRing(b, sqr, color)) { 
					attack += KNIGHT_KING_ATTACK;
				}
			}
			break;

		case b.BB:
			attack += bishopKingAttack(b, i, color) * BISHOP_KING_ATTACK;
			break;
		case b.BR:
			attack += rookKingAttack(b, i, color) * ROOK_KING_ATTACK;
			break;
		case b.BQ:
			attack += bishopKingAttack(b, i, color) + rookKingAttack(b, i, color) * QUEEN_KING_ATTACK;
			break;

		}
	}
	return static_cast<int>(attack * -phase);
}
bool Evaluation::inKingRing(Board& b, int square, int color) {
	int kingsqr = (color == b.WHITE) ? b.kingSquareWhite : b.kingSquareBlack;
	int kingRing[24] = {
		1, -1, 10, -10, 11, 9, -11, 9,
		2, -2, 20, -20, 22, 18, -22, 18,
		12, 8, -12, -8, 21, -21, 19, -19
	};
	if (kingsqr - square > 22 || kingsqr - square < -22) { return false; }
	for (int i = 0; i < 24; ++i) {
		int sqr = kingsqr + kingRing[i];
		if (square == sqr) { return true; }
	}
	return false;
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
	if (b.mailbox[square] != -9) {
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

int Evaluation::totalEvaluation(Board& b, int color) {
	phase = getPhase(b);
	int material = baseMaterial(b, color) + structureMaterial(b, color) - baseMaterial(b, -color) - structureMaterial(b, -color);
	int pieces = knightOutpost(b, color) + bishopPair(b, color) + rookBehindPassed(b, color) + trappedRook(b, color) 
				- knightOutpost(b, -color) - bishopPair(b, -color) - rookBehindPassed(b, -color) - trappedRook(b, -color);
	int pawns = doubledAndIsolatedPawns(b, color) + connectedPawns(b, color) + backwardPawns(b, color) + passedPawns(b, color) 
			  - doubledAndIsolatedPawns(b, -color) - connectedPawns(b, -color) - backwardPawns(b, -color) - passedPawns(b, -color);
	int pst = PST(b, color) - PST(b, -color);
	int mobility = totalMobility(b, color) - totalMobility(b, -color);
	int space = spaceArea(b, color) - spaceArea(b, -color);
	int king = kingShelter(b, color) + kingDangerProximity(b, color) - kingShelter(b, -color) - kingDangerProximity(b, -color);
	int total = material + pieces + pawns + pst + mobility + space + king;
	return total;
}