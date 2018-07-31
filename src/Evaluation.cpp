#include "Evaluation.h"

/*GAME PHASE*/
void Evaluation::setGamePhase(const Board& b) {
	if (gamePhase == OPENING) {
		int score = 0;
		//Sparse backrank --> more developed pieces
		for (int i = 21; i < 29; ++i) {
			if (b.mailbox[i] == 0) {
				score += 2;
			}
		}
		for (int i = 91; i < 99; ++i) {
			if (b.mailbox[i] == 0) {
				score += 2;
			}
		}

		//Castled
		for (int i = 0; i < 2; ++i) {
			if (b.castled[i]) {
				score += 2;
			}
		}

		if (score >= 18) {
			gamePhase = MIDGAME;
		}
	}
	else if (gamePhase == MIDGAME) {
		int count = 0;
		for (int i = 21; i < 99; ++i) {
			if (b.mailbox[i] != 0 && abs(b.mailbox[i]) != b.WP && abs(b.mailbox[i]) != b.WK) {
				count++;
			}
		}
		//Less than eight major/minor pieces
		if (count < 8) {
			gamePhase = ENDGAME;
		}
	}
}

/*PAWN STRUCTURE*/
int Evaluation::blockedPawns(const Board& b) {
	int bpcount = 0;
	for (int i = 21; i < 99; ++i) {
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
	for (int i = 21; i < 99; ++i) {
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
	for (int i = 21; i < 99; ++i) {
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
	bool passed;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			passed = true;
			file = i % 10;
			rank = (i - file) / 10;
			int sqr = i;
			while (sqr >= 31 && sqr <= 88) {
				if (b.mailbox[sqr] == b.WP * -color || b.mailbox[sqr - 1] == b.WP * -color || b.mailbox[sqr + 1] == b.WP * -color) {
					passed = false;
				}
				sqr += -color * 10;
			}
			if (passed) {
				int reverse_dis = color == b.WHITE ? 9 - rank : rank - 2;
				if (reverse_dis < 6) {
					pcount += reverse_dis * 2 * PASSED_P_BONUS;
				}
				else {
					pcount += pow(reverse_dis, 2) * PASSED_P_BONUS;
				}
			}
		}
	}
	return pcount * PASSED_P_BONUS;
}


/*PIECE VALUES*/
int Evaluation::baseMaterial(const Board& b, int color) {
	int mval = 0;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == -9) { continue; }
		if (color * b.mailbox[i] > 0) {
			switch (abs(b.mailbox[i])) {
			case b.WP:
				mval += P_BASE_VAL;
				break;
			case b.WN:
				mval += N_BASE_VAL;
				break;
			case b.WB:
				mval += B_BASE_VAL;
				break;
			case b.WR:
				mval += R_BASE_VAL;
				break;
			case b.WQ:
				mval += Q_BASE_VAL;
				break;
			}
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
				break;
			case b.WB:
				//Bishops are worse in closed positions
				mval -= blockedPawns(b) * OPEN_CLOSED_POS_PIECE_VALUE;
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
int Evaluation::bishopPair(const Board& b, int color) {
	int mval = 0;
	int bcount = 0;
	for (int i = 21; i < 99; ++i) {
		if (color * b.mailbox[i] > 0) {
			switch (abs(b.mailbox[i])) {
			case b.WB:
				bcount += 1;
				break;
			}
		}
	}
	//Bishop pair bonus
	if (bcount >= 2) {
		mval += 50;
	}
	return mval;
}

/*POSITION*/
int Evaluation::flipTableValue(int square) const {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = (11 - tenth) * 10 + unit;
	return rval;
}
int Evaluation::piecePosition(const Board& b, int color) {
	int pval = 0;
	for (int i = 21; i < 99; ++i) {
		if (color * b.mailbox[i] > 0) {
			switch (b.mailbox[i]) {
			case b.WP:
				pval += pawnTable[b.to64Coord(i)];
				break;
			case b.WN:
				pval += knightTable[b.to64Coord(i)];
				break;
			case b.WB:
				pval += bishopTable[b.to64Coord(i)];
				break;
			case b.WR:
				pval += rookTable[b.to64Coord(i)];
				break;
			case b.WQ:
				if (gamePhase == OPENING) {
					pval += queenOpeningTable[b.to64Coord(i)];
				}
				else if (gamePhase >= MIDGAME) {
					pval += queenNormalTable[b.to64Coord(i)];
				}
				break;
			case b.WK:
				if (gamePhase <= MIDGAME) {
					pval += kingNormalTable[b.to64Coord(i)];
				}
				else if (gamePhase == ENDGAME) {
					pval += kingEndTable[b.to64Coord(i)];
				}
				break;
			case b.BP:
				pval += pawnTable[b.to64Coord(flipTableValue(i))];
				break;
			case b.BN:
				pval += knightTable[b.to64Coord(flipTableValue(i))];
				break;
			case b.BB:
				pval += bishopTable[b.to64Coord(flipTableValue(i))];
				break;
			case b.BR:
				pval += rookTable[b.to64Coord(flipTableValue(i))];
				break;
			case b.BQ:
				if (gamePhase == OPENING) {
					pval += queenOpeningTable[b.to64Coord(flipTableValue(i))];
				}
				else if (gamePhase >= MIDGAME) {
					pval += queenNormalTable[b.to64Coord(flipTableValue(i))];
				}
				break;
			case b.BK:
				if (gamePhase <= MIDGAME) {
					pval += kingNormalTable[b.to64Coord(flipTableValue(i))];
				}
				else if (gamePhase == ENDGAME) {
					pval += kingEndTable[b.to64Coord(flipTableValue(i))];
				}
				break;
			}
		}
	}
	return pval;
}
int Evaluation::space(const Board&b, int color) {
	int file;
	int rank;
	int sval = 0;
	for (int i = 43; i < 77; ++i) {
		if (i % 10 < 3 || i % 10 > 6) {
			continue;
		}
		if (b.mailbox[i - color * 10 + 1] == -color * b.WP || b.mailbox[i - color * 10 - 1] == -color * b.WP) {
			continue;
		} 
		else {
			sval++;
		}
	}
	return sval * SPACE_BONUS;
}
int Evaluation::kingSafety(Board& b, int color) {
	b.setKingSquare(b.mailbox);
	int rval = 0;
	int pval = 0;
	int cval = 0;
	if (color == b.WHITE) {
		if (b.castled[0]) {
			cval++;
		}
		rval = isOpenFile(b, b.kingSquareWhite) + isOpenFile(b, b.kingSquareWhite + 1) + isOpenFile(b, b.kingSquareWhite - 1);
		if (gamePhase <= MIDGAME && b.castled[0]) {
			if (b.mailbox[b.kingSquareWhite - 10] != b.WP) {
				pval++;
			}
			if (b.mailbox[b.kingSquareWhite - 11] != b.WP) {
				pval++;
			}
			if (b.mailbox[b.kingSquareWhite - 9] != b.WP) {
				pval++;
			}
		}
	}
	else {
		if (b.castled[1]) {
			cval++;
		}
		rval = isOpenFile(b, b.kingSquareBlack) + isOpenFile(b, b.kingSquareBlack + 1) + isOpenFile(b, b.kingSquareBlack - 1);
		if (gamePhase <= MIDGAME && b.castled[1]) {
			if (b.mailbox[b.kingSquareBlack + 10] !=  b.BP) {
				pval++;
			}
			if (b.mailbox[b.kingSquareBlack + 11] !=  b.BP) {
				pval++;
			}
			if (b.mailbox[b.kingSquareBlack + 9] !=  b.BP) {
				pval++;
			}
		}
	}

	return rval * K_OPEN_FILE_PENALTY + pval * K_P_SHIELD_PENALTY + cval * K_CASTLED_BONUS;
}

/*CENTER*/
int Evaluation::pawnCenterControl(const Board& b, int color) {
	int pawnInExtendedCenter = 0;
	int pawnInCenter = 0;
	for (int i = 43; i < 77; ++i) {
		if (i % 10 >= 3 && i % 10 <= 6) {
			if (b.mailbox[i - color * 9] == b.WP * color || b.mailbox[i - color * 11] == b.WP * color) {
				pawnInExtendedCenter++;
			}
			if (i == 54 || i == 55 || i == 64 || i == 65) {
				if (b.mailbox[i - color * 9] == b.WP * color || b.mailbox[i - color * 11] == b.WP * color) {
					pawnInCenter++;
				}
			}
		}
	}
	return pawnInExtendedCenter * P_EXTENDED_CENTER_BONUS + pawnInCenter * P_CENTER_BONUS;
}
int Evaluation::pieceCenterControl(const Board& b, int color) {
	int controlledSquares = 0;
	for (int i = 21; i < 99; ++i) {
		if (b.mailbox[i] == b.WN * color || b.mailbox[i] == b.WB * color) {
			for (int j = 43; j < 77; ++j) {
				if (j % 10 >= 3 && j % 10 <= 6) {
					if (b.checkAttack(i, j, b.mailbox)) {
						controlledSquares++;
					}
				}
			}
		}
	}
	return controlledSquares * PIECE_CENTER_BONUS;
}


/*GETTERS*/
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
int Evaluation::getGamePhase() { return gamePhase; }

int Evaluation::totalEvaluation(Board& b, int color) {
	setGamePhase(b);
	int material = baseMaterial(b, color) + structureMaterial(b, color) - baseMaterial(b, -color) - structureMaterial(b, -color);
	int combos = bishopPair(b, color) - bishopPair(b, -color);
	int pawns = doubledAndIsolatedPawns(b, color) + connectedPawns(b, color) + backwardPawns(b, color) + passedPawns(b, color) 
			  - doubledAndIsolatedPawns(b, -color) - connectedPawns(b, -color) - backwardPawns(b, -color) - passedPawns(b, -color);
	int position = piecePosition(b, color) + space(b, color) + kingSafety(b, color) - piecePosition(b, -color) - space(b, -color) - kingSafety(b, -color);
	int center = pawnCenterControl(b, color) + pieceCenterControl(b, color) - pawnCenterControl(b, -color) - pieceCenterControl(b, -color);
	int sideToMove = (b.getTurn() == color) ? 1 : 0;
	int total = material + combos + position + center + pawns + sideToMove * SIDE_TO_MOVE_BONUS;
	return total;
}