#include "Evaluation.h"

double Evaluation::getPhase(const Board& b) {
	int pieceValues[6] = { 0, N_BASE_VAL, B_BASE_VAL, R_BASE_VAL, Q_BASE_VAL, 0 };
	int baseMaterial = (N_BASE_VAL + B_BASE_VAL + R_BASE_VAL) * 4 + Q_BASE_VAL * 2;
	int nonPMaterial = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == -9 || piece == 0) { continue; }
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
					pcount += pow(reverse_dis, 2) * PASSED_P_BONUS;
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
			if (color == b.WHITE) {
				coord = i;
			}
			else {
				coord = flipTableValue(i);
			}
			switch (abs(b.mailbox[i])) {
			case b.WP:
				val += static_cast<int>(pawnTable[b.to64Coord(coord)] * phase);
				break;
			case b.WN:
				val += knightTable[b.to64Coord(coord)];
				break;
			case b.WB:
				val += bishopTable[b.to64Coord(coord)];
				break;
			case b.WR:
				val += static_cast<int>(rookTable[b.to64Coord(coord)] * phase);
				break;
			case b.WQ:
				val += queenTable[b.to64Coord(coord)];
				break;
			case b.WK:
				val += static_cast<int>((kingNormalTable[b.to64Coord(coord)] * phase + kingEndTable[b.to64Coord(coord)] * (1 - phase)));
				break;
			}
		}
	}
	return val;
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
	int kingsqr = (color == b.WHITE) ? b.kingSquareWhite : b.kingSquareBlack;
	int castle_id = (color == b.WHITE) ? 0 : 1;

	//Check if castled
	if (b.castled[castle_id]) {
		cval++;
	}
	//Open files
	rval = isOpenFile(b, kingsqr) + isOpenFile(b, kingsqr + 1) + isOpenFile(b, kingsqr - 1);
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

	return static_cast<int>((rval * K_OPEN_FILE_PENALTY + pval * K_P_SHIELD_PENALTY  + cval * K_CASTLED_BONUS) * phase);
}

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
	int pieces = bishopPair(b, color) + rookBehindPassed(b, color) + trappedRook(b, color) - bishopPair(b, -color) - rookBehindPassed(b, -color) - trappedRook(b, -color);
	int pawns = doubledAndIsolatedPawns(b, color) + connectedPawns(b, color) + backwardPawns(b, color) + passedPawns(b, color) 
			  - doubledAndIsolatedPawns(b, -color) - connectedPawns(b, -color) - backwardPawns(b, -color) - passedPawns(b, -color);
	int position = PST(b, color) + space(b, color) + kingSafety(b, color) - PST(b, -color) - space(b, -color) - kingSafety(b, -color);
	int center = pawnCenterControl(b, color) - pawnCenterControl(b, -color); // + pieceCenterControl(b, color) - pieceCenterControl(b, -color);
	int sideToMove = (b.getTurn() == color) ? 1 : 0;
	int total = material + pieces + position + center + pawns + sideToMove * SIDE_TO_MOVE_BONUS;
	return total;
}
void Evaluation::outputEvalInfo(Board& b, int color) {
	phase = getPhase(b);
	std::cout << "<<Material>> " << baseMaterial(b, color) + structureMaterial(b, color) << " | " << baseMaterial(b, -color) + structureMaterial(b, -color) << std::endl;
	std::cout << "<<Pieces>> " << bishopPair(b, color) + rookBehindPassed(b, color) + trappedRook(b, color) << " | " << bishopPair(b, -color) + rookBehindPassed(b, -color) + trappedRook(b, -color) << std::endl;
	std::cout << "<<Pawns>> " << doubledAndIsolatedPawns(b, color) + connectedPawns(b, color) + backwardPawns(b, color) + passedPawns(b, color) 
			  << " - " << doubledAndIsolatedPawns(b, -color) + connectedPawns(b, -color) + backwardPawns(b, -color) + passedPawns(b, -color) << std::endl;
	std::cout << "<<Position>> " << PST(b, color) + space(b, color) + kingSafety(b, color) << " | " << PST(b, -color) + space(b, -color) + kingSafety(b, -color) << std::endl;
	std::cout << "<<Center>> " << pawnCenterControl(b, color) << " | " << pawnCenterControl(b, -color) << std::endl;
	std::cout.precision(3);
	std::cout << "<<Phase>> " << phase << std::endl << std::endl;
}