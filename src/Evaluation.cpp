#include "Evaluation.h"

double Evaluation::getPhase(const Board& b) {
	int baseMaterial = (pieceValues[b.WN - 1] + pieceValues[b.WB - 1] + pieceValues[b.WR - 1]) * 4 + pieceValues[b.WQ - 1] * 2;
	int nonPMaterial = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN || piece == 0 || piece == b.WP || piece == b.BP || piece == b.WK || piece == b.BK) { continue; }
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
		dpcount += (filearray[i] >= 2) ? filearray[i] - 1 : 0;
		if (i >= 1 && i <= 6) {
			if (filearray[i] == b.WP * color && filearray[i - 1] == 0 && filearray[i + 1] == 0) {
				ipcount++;
			}
		}
		else if (i == 0) {
			if (filearray[0] == b.WP * color && filearray[1] == 0) {
				ipcount++;
			}
		}
		else if (i == 7) {
			if (filearray[7] == b.WP * color && filearray[6] == 0) {
				ipcount++;
			}
		}
	}
	return dpcount * doubledPawnPenalty + ipcount * isolatedPawnPenalty;
}
int Evaluation::backwardPawns(const Board& b, int color) {
	int bpcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] != b.WP * color) { continue; }
		if (b.mailbox[i - 10 * color] == b.WP * -color || !attackedByEnemyPawn(b, i - 10 * color, color)) { continue; }

		int j = i - 1;
		bool backward = true;
		while (b.mailbox[j] != b.NN) {
			if (b.mailbox[j] == b.WP * color) {
				backward = false;
				break;
			}
			j += color * 10;
		}
		if (backward) {
			j = i + 1;
			while (b.mailbox[j] != b.NN) {
				if (b.mailbox[j] == b.WP * color) {
					backward = false;
					break;
				}
				j += color * 10;
			}
		}
		if (backward) { bpcount++; }
	}
	return bpcount * backwardPawnPenalty;
}

int Evaluation::pawnEval(const Board& b) {
	int val = 0;
	for (int i = 31; i < 89; ++i) {
		int piece = b.mailbox[i];
		if (piece != b.WP && piece != b.BP) { continue; }

		// Connected pawns
		val += ((b.mailbox[i + piece * 11] == piece) + (b.mailbox[i + piece * 9] == piece)) * supportedPawnBonus * piece;
		val += ((b.mailbox[i + 1] == piece) + (b.mailbox[i - 1] == piece)) * phalanxPawnBonus * piece;

		// Passed pawns
		if (!isPassed(b, i, piece)) { continue; }
		int rank = i / 10;
		rank = (piece == b.WP) ? 9 - rank : rank - 2;
		if (b.mailbox[i - piece * 10] != 0) { rank -= 1; }  // Reduce bonus if blocked
		val += passedPawnBonus[rank] * piece;
	}
	return val;
}

int Evaluation::knightOutpost(const Board& b, int square, int color) {
	int support = (b.mailbox[square + 11 * color] == b.WP * color) + (b.mailbox[square + 9 * color] == b.WP * color);
	int val = 0;
	if (support > 0) {
		bool secure[2] = { true, true };
		for (int j = square - 1 - 10 * color; b.mailbox[j] != b.NN; j -= 10 * color) {
			if (b.mailbox[j] == b.WP * -color) { secure[0] = false; }
		}
		for (int j = square + 1 - 10 * color; b.mailbox[j] != b.NN; j -= 10 * color) {
			if (b.mailbox[j] == b.WP * -color) { secure[1] = false; }
		}
		if (secure[0] && secure[1]) {
			val += support * knightOutpostBonus;
		}
	}
	return val;
}
int Evaluation::rookBehindPassed(const Board& b, int square, int color) {
	int rcount = 0;
	int sqr = square;
	while (sqr >= 31 && sqr <= 88) {
		// Supporting friendly passed pawn or blocking enemy passed pawn
		sqr += -color * 10;
		if (isPassed(b, sqr, color) || isPassed(b, sqr, -color)) {
			rcount += 1;
			break;
		}
	}
	while (sqr >= 31 && sqr <= 88) {
		// Behind enemy passed pawn
		sqr += color * 10;
		if (isPassed(b, sqr, -color)) {
			rcount += 1;
			break;
		}
	}
	return rcount * rookBehindPassedBonus;
}
int Evaluation::trappedRook(const Board& b, int square, int color) {
	int blocked = 0;
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
	return (blocked > 1) * blocked * trappedRookPenalty;
}

int Evaluation::pieceEval(const Board& b) {
	int val = 0;
	int bcount[2] = { 0, 0 };
	int attack[2] = { 0, 0 };
	int attackers[2] = { 0, 0 };
	int blocked = blockedPawns(b) * closedPositionBonus;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN || piece == 0) { continue; }
		int color = (piece > 0) ? b.WHITE : b.BLACK;
		int index = !(color == b.WHITE);
		int absPiece = piece * color;
		if (absPiece == b.WP) { continue; }
		switch (absPiece) {
		case b.WN:
			val += (blocked + minorBehindPawnBonus * (b.mailbox[i - 10 * color] == b.WP * color) +
					knightOutpost(b, i, color) + knightMobilityTable[mobilityKnight(b, i, color)]) * color;
			for (int j = 0; j < 9; ++j) {
				int square = b.kingSquare[!index] + kingRing[j];
				if (b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackKnight(i, square)) {
					attack[index] += knightKingAttacker;
					attackers[index]++;
					break;
				}
			}
			break;
		case b.WB:
			bcount[index]++;
			val += (-blocked + minorBehindPawnBonus * (b.mailbox[i - 10 * color] == b.WP * color) +
					bishopMobilityTable[mobilitySlider(b, i, color, b.WB)]) * color;
			for (int j = 0; j < 9; ++j) {
				int square = b.kingSquare[!index] + kingRing[j];
				if (b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackBishop(i, square)) {
					attack[index] += bishopKingAttacker;
					attackers[index]++;
					break;
				}
			}
			break;
		case b.WR:
			val += (-blocked + isOpenFile(b, i) * rookOpenFileBonus +
					rookBehindPassed(b, i, color) + trappedRook(b, i, color) +
					rookMobilityTable[mobilitySlider(b, i, color, b.WR)]) * color;
			for (int j = 0; j < 9; ++j) {
				int square = b.kingSquare[!index] + kingRing[j];
				if (b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackRook(i, square)) {
					attack[index] += rookKingAttacker;
					attackers[index]++;
					break;
				}
			}
			break;
		case b.WQ:
			val += queenMobilityTable[mobilitySlider(b, i, color, b.WQ)] * color;
			for (int j = 0; j < 9; ++j) {
				int square = b.kingSquare[!index] + kingRing[j];
				if (b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackQueen(i, square)) {
					attack[index] += queenKingAttacker;
					attackers[index]++;
					break;
				}
			}
			break;
		}
		val += (pawnPushThreat(b, i, color) + pawnAttackThreat(b, i, color)) * color;
	}
	val += bishopPairBonus * ((bcount[0] >= 2) - (bcount[1] >= 2));
	val += attack[0] * (attackers[0] > 1) - attack[1] * (attackers[1] > 1);
	return val;
}

int Evaluation::flipTableValue(int square) const {
	return (11 - square / 10) * 10 + square % 10;
}
int Evaluation::PST(const Board& b, int square, int color) {
	int val = 0;
	int coord = b.mailbox120[(color == b.WHITE) ? square : flipTableValue(square)];
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
		val = kingTable[coord] * phase + kingEndTable[coord] * (1 - phase);
		break;
	}
	return val;
}

int Evaluation::mobilityKnight(const Board& b, int square, int color) {
	int mobility = 0;
	for (const int& i : b.pieceMoves[1]) {
		if (!i) { break; }
		int toVal = b.mailbox[square + i];
		if (toVal == b.NN || toVal * color > 0 || attackedByEnemyPawn(b, square + i, color)) {
			continue;
		}
		mobility++;
	}
	return mobility;
}
int Evaluation::mobilitySlider(const Board& b, int square, int color, int piece) {
	int mobility = 0;
	for (const int& i : b.pieceMoves[piece - 1]) {
		if (!i) { break; }
		for (int j = 1; j < 8; ++j) {
			int target = b.mailbox[square + j * i];
			if (target == b.NN || target * color > 0) {
				break;
			}
			if (!attackedByEnemyPawn(b, square + j * i, color)) {
				mobility++;
			}
			if (target * color < 0) { break; }
		}
	}
	return mobility;
}

int Evaluation::spaceArea(const Board& b , int color) {
	int sval = 0;
	for (int i = 33 + (color == b.WHITE) * 30; i <= 56 + (color == b.WHITE) * 30; ++i) {
		int file = i % 10;
		if (file < 3 || file > 6) { continue;; }
		int piece = b.mailbox[i];
		if (piece == b.NN || piece == b.WP * color) { continue; }
		if (attackedByEnemyPawn(b, i, color)) { continue; }
		sval++;
		// Increase space value if square is behind own pawn by 1-3 squares
		for (int j = 1; j <= 3; ++j) {
			if (b.mailbox[i - color * 10 * j] == b.WP * color) {
				sval++;
			}
		}
	}
	return static_cast<int>(sval * phase * 8);
}

int Evaluation::kingShelter(Board& b, int color) {
	int rval = 0;
	int pval = 0;
	int kingsqr = (color == b.WHITE) ? b.kingSquare[0] : b.kingSquare[1];

	// Open files
	rval = isOpenFile(b, kingsqr) + isOpenFile(b, kingsqr + 1) * 0.75 + isOpenFile(b, kingsqr - 1) * 0.75;
	// Pawn shield in front of king
	if (b.mailbox[kingsqr - 10 * color] != b.WP * color) {
		pval++;
	}
	if (b.mailbox[kingsqr - 11 * color] != b.WP * color) {
		pval++;
	}
	if (b.mailbox[kingsqr - 9 * color] != b.WP * color) {
		pval++;
	}

	return static_cast<int>((rval * kingOpenFilePenalty + pval * kingWeakPawnShieldPenalty) * phase);
}

int Evaluation::pawnPushThreat(const Board& b, int square, int color) {
	if (b.mailbox[square - 19 * color] == b.WP * -color && attackedByEnemyPawn(b, square - 9 * color, color)) {
		return pawnPushThreatPenalty;
	}
	if (b.mailbox[square - 21 * color] == b.WP * -color && attackedByEnemyPawn(b, square - 11 * color, color)) {
		return pawnPushThreatPenalty;
	}
	return 0;
}
int Evaluation::pawnAttackThreat(const Board& b, int square, int color) {
	if (!attackedByEnemyPawn(b, square, color)) { return 0; }
	if (attackedByEnemyPawn(b, square - 11 * color, color) ||
		attackedByEnemyPawn(b, square - 9 * color, color)) {
		return pawnAttackThreatPenalty;
	}
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN || piece * color >= b.BP) { continue; }
		switch (piece * color) {
		case b.BN:
			if (b.checkAttackKnight(i, square)) { return pawnAttackThreatPenalty; }
			break;
		case b.BB:
			if (b.checkAttackBishop(i, square)) { return pawnAttackThreatPenalty; }
			break;
		case b.BR:
			if (b.checkAttackRook(i, square)) { return pawnAttackThreatPenalty; }
			break;
		case b.BQ:
			if (b.checkAttackQueen(i, square)) { return pawnAttackThreatPenalty; }
			break;
		case b.BK:
			if (b.checkAttackKing(i, square)) { return pawnAttackThreatPenalty; }
			break;
		}
	}
	return 0;
}

bool Evaluation::attackedByEnemyPawn(const Board& b, int square, int color) {
	return (b.mailbox[square - 11 * color] == b.WP * -color) || (b.mailbox[square - 9 * color] == b.WP * -color);
}
int Evaluation::isOpenFile(const Board& b, int square) {
	int file = square % 10;
	int pcount = 0;
	if (b.mailbox[square] != b.NN) {
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
int Evaluation::isPassed(const Board& b, int square, int color) {
	if (b.mailbox[square] != b.WP * color) { return false; }
	bool passed = true;
	int sqr = square;
	while (sqr >= 31 && sqr <= 88) {
		sqr -= color * 10;
		if (b.mailbox[sqr] == b.WP * -color || b.mailbox[sqr - 1] == b.WP * -color || b.mailbox[sqr + 1] == b.WP * -color) {
			passed = false;
			break;
		}
	}
	return passed;
}

int Evaluation::totalEvaluation(Board& b, int color, int lazyScore[]) {
	phase = getPhase(b);
	int lazyIndex = !(color == b.WHITE);
	int lazy = lazyScore[lazyIndex] - lazyScore[!lazyIndex];
	int pieces = pieceEval(b) * color;
	int pawns = pawnEval(b) * color + doubledAndIsolatedPawns(b, color) - doubledAndIsolatedPawns(b, -color) +
		backwardPawns(b, color) - backwardPawns(b, -color);
	int space = spaceArea(b, color) - spaceArea(b, -color);
	int king = kingShelter(b, color) - kingShelter(b, -color);
	return lazy + pieces + pawns + space + king;
}
int Evaluation::lazyEvaluation(const Board& b, int color) {
	phase = getPhase(b);
	int val = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		int absPiece = color * piece;
		if (piece == b.NN || absPiece <= 0) { continue; }
		val += pieceValues[abs(piece) - 1] + PST(b, i, color);
	}
	return val;
}