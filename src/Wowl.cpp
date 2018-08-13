#include "Wowl.h"

void Wowl::initMVVLVA(const Board& b, const Evaluation& e) {
	int pieceValue[6] = { e.P_BASE_VAL, e.N_BASE_VAL, e.B_BASE_VAL, e.R_BASE_VAL, e.Q_BASE_VAL, e.K_BASE_VAL };
	for (int attacker = b.WP; attacker <= b.WK; attacker++) {
		for (int victim = b.WP; victim <= b.WK; victim++) {
			MVVLVAScores[attacker - 1][victim - 1] = pieceValue[victim - 1] - attacker;
		}
	}
}
int Wowl::SEE(Board b, Evaluation& e, int square, int color) const {
	int oldsqr;
	int val = 0;
	int piece = std::get<0>(b.getSmallestAttacker(square, color));
	int target = b.mailbox[square];
	int targetval = target;

	if (piece > 0) {
		if (!(target == -9 || target == 0)) {
			oldsqr = std::get<1>(b.getSmallestAttacker(square, color));
			b.move(oldsqr, square);
			if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
				b.undo();
			}
			else {
				switch (abs(target)) {
				case b.WP:
					targetval = e.P_BASE_VAL;
					break;
				case b.WN:
					targetval = e.B_BASE_VAL;  //We use the same value for bishop and knight
					break;
				case b.WB:
					targetval = e.B_BASE_VAL;
					break;
				case b.WR:
					targetval = e.R_BASE_VAL;
					break;
				case b.WQ:
					targetval = e.Q_BASE_VAL;
					break;
				case b.WK:
					targetval = e.K_BASE_VAL;
					break;
				}
				int netMaterial = targetval - SEE(b, e, square, -color);
				val = netMaterial;
				b.undo();
			}
		}
	}
	return val;
}
bool Wowl::checkThreefold(const U64 key) const {
	int poscount = 0;
	for (const auto& i : tempHashPosVec) {
		if (key == i) { poscount++; }
	}
	return (poscount >= 3) ? true : false;
}

void Wowl::staticEvalOrdering(Board& b, Evaluation& e, std::vector<Move>& lmV, int depth, int alpha, int beta) {
	int best = -WIN_SCORE;
	int score = -WIN_SCORE;
	bool isCapture;

	for (int i = 0; i < lmV.size(); ++i) {
		isCapture = (b.mailbox[lmV[i].to] == 0) ? false : true;
		b.move(lmV[i].from, lmV[i].to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		score = -qSearch(b, e, -beta, -alpha, b.getTurn());
		if (score > best && i > 0) {
			best = score;
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
		}
		b.undo();
	}

}
std::vector<int> Wowl::scoreMoves(Board& b, const std::vector<Move>& lmV, const int depth, const U64 poskey) {
	
	std::vector<int> scoreVec(lmV.size(), 0);
	
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		hashMove = hashTable.tt.at(poskey).hashBestMove;
	}
	else {
		hashMove = Move(NO_MOVE, NO_MOVE);
	}

	for (int i = 0; i < lmV.size(); ++i) {
		//Hash table move
		if (lmV[i] == hashMove) {
			scoreVec[i] = 10000000;
			continue;
		}
		//Good captures
		int victim = b.mailbox[lmV[i].to];
		if (victim != 0) {
			int attacker = b.mailbox[lmV[i].from];
			scoreVec[i] = MVVLVAScores[abs(attacker) - 1][abs(victim) - 1] * 100;
			continue;
		}
		//Killer moves
		if (lmV[i] == killerMoves[0][depth]) {
			scoreVec[i] = 20000;
			continue;
		}
		if (lmV[i] == killerMoves[1][depth]) {
			scoreVec[i] = 12000;
			continue;
		}
		//History heuristic
		scoreVec[i] = historyMoves[(b.getTurn() == b.WHITE)][abs(b.mailbox[lmV[i].from]) - 1][lmV[i].to];
	}
	return scoreVec;
}
void Wowl::pickNextMove(std::vector<Move>& lmV, const std::vector<int>& scoreVec, int startpos) {
	int best = 0;
	int bestMoveIndex = 0;
	for (int i = startpos; i < scoreVec.size(); ++i) {
		if (scoreVec[i] > best) {
			best = scoreVec[i];
			bestMoveIndex = i;
		}
	}
	std::swap(lmV[0], lmV[bestMoveIndex]);
}
void Wowl::orderCaptures(Board& b, std::vector<Move>& cV) {	int best = -WIN_SCORE;
	int score;
	for (int i = 0; i < cV.size(); ++i) {
		score = MVVLVAScores[abs(b.mailbox[cV[i].from]) - 1][abs(b.mailbox[cV[i].to]) - 1];
		if (score > best) {
			best = score;
			cV.insert(cV.begin(), cV[i]);
			cV.erase(cV.begin() + i + 1);
		}
	}
}
void Wowl::resetMoveHeuristics() {
	for (int i = 0; i < MAX_SEARCH_DEPTH; ++i) {
		killerMoves[0][i] = Move(NO_MOVE, NO_MOVE);
		killerMoves[1][i] = Move(NO_MOVE, NO_MOVE);
	}
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 6; ++j) {
			for (int k = 0; k < 120; k++) {
				historyMoves[i][j][k] = 0;
			}
		}
	}
}

int Wowl::probeHashTable(const U64 key, int depth, int initial, int alpha, int beta) {
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		if (hashTable.tt.at(key).hashDepth >= depth) {
			if (hashTable.tt.at(key).hashFlag == hashTable.HASH_ALPHA) {
				if (hashTable.tt.at(key).hashScore <= alpha) {
					return alpha;
				}
			}
			else if (hashTable.tt.at(key).hashFlag == hashTable.HASH_BETA) {
				if (hashTable.tt.at(key).hashScore >= beta) {
					return beta;
				}
			} else if (hashTable.tt.at(key).hashFlag == hashTable.HASH_EXACT) {
				if (depth == initial) { bestMove = hashTable.tt.at(key).hashBestMove; }
				return hashTable.tt.at(key).hashScore;
			}
		}
	}
	return VAL_UNKWOWN;
}
void Wowl::recordHash(const U64 key, int depth, int score, int flag) {
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		if (hashTable.tt[key].hashDepth <= depth) {
			hashTable.tt[key].hashDepth = depth;
			hashTable.tt[key].hashScore = score;
			hashTable.tt[key].hashFlag = flag;
		}
	}
	else {
		hashTable.tt[key].hashDepth = depth;
		hashTable.tt[key].hashScore = score;
		hashTable.tt[key].hashFlag = flag;
		hashTable.tt[key].hashAge = 0;
	}
}
void Wowl::ageHash() {
	for (auto it = hashTable.tt.begin(); it != hashTable.tt.end();) {
		if (it->second.hashAge < TT_CLEAR_AGE) {
			(*it).second.hashAge++;
			it++;
		}
		else {
			it = hashTable.tt.erase(it);
		}
	}
}

int Wowl::qSearch(Board b, Evaluation& e, int alpha, int beta, int color) {

	int score, stand_pat;
	Evaluation qEval;

	stand_pat = qEval.totalEvaluation(b, color);

	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}
	
	b.getCaptures();
	orderCaptures(b, b.captureVec);

	for (const auto& j : b.captureVec) {
		if (SEE(b, e, j.to, color) >= 0) {
			qSearchNodes++;
			b.move(j.from, j.to);
			score = -qSearch(b, e, -beta, -alpha, -color);
			b.undo();
			if (score >= beta) {
				return beta;
			}
			if (score > alpha) {
				alpha = score;
			}
		}
	}

	return alpha;
}
int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta, bool record) {

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);
	int tempHashFlag = hashTable.HASH_ALPHA;
	bool isInCheck = b.inCheck(color, b.mailbox);

	if (checkThreefold(key)) { return DRAW_SCORE; }

	b.getLegalMoves();
	bool haveMove = false;
	for (const auto& i : b.legalMoveVec) {
		if (!b.checkMoveCheck(i.from, i.to)) {
			haveMove = true;
			break;
		}
	}
	if (!haveMove) {
		if (depth == initial) {
			bestMove.from = NO_MOVE;
			bestMove.to = NO_MOVE;
		}
		if (isInCheck) {
			return -WIN_SCORE;  //Checkmate
		}
		else {
			return DRAW_SCORE;  //Stalemate
		}
	}

	if (depth == 0 && isInCheck) {
		depth++;
	}

	if (depth == 0) {
		int qScore = qSearch(b, WowlEval, alpha, beta, color);
		recordHash(key, depth, qScore, hashTable.HASH_EXACT);
		return qScore;
	}

	int hashScore = probeHashTable(key, depth, initial, alpha, beta);
	if (hashScore != VAL_UNKWOWN) {
		return hashScore;
	}

	int score;

	//Null move pruning
	if (depth - 1 - NULL_MOVE_REDUCTION >= 0 && !isInCheck && WowlEval.getPhase(b) > 0.25) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - NULL_MOVE_REDUCTION, initial, -color, -beta, -beta + 1, false);
		b.nullMove();
		if (score >= beta) {
			return beta;
		}
	}
	
	bool foundPV = false;
	bool isCapture;
	int movesSearched = 0;
	std::vector<int> scoreVec = scoreMoves(b, b.legalMoveVec, depth, key);

	for (int i = 0; i < b.legalMoveVec.size(); i++) {

		pickNextMove(b.legalMoveVec, scoreVec, i);

		Move m = b.legalMoveVec[i];
		movesSearched++;
		isCapture = (b.mailbox[m.to] != 0) ? true : false;

		//Futility pruning
		if (depth <= 2 && !isInCheck && !isCapture) {
			int futilityScore = WowlEval.totalEvaluation(b, color);
			if (futilityScore + futilityMargin[depth - 1] <= alpha && futilityScore + futilityMargin[depth - 1] < WIN_SCORE) { continue; }
		};

		b.move(m.from, m.to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();	
			continue;
		}
		else {

			negaNodes++;
			tempHashPosVec.emplace_back(key);

			//Late move reduction
			if (movesSearched >= LMR_STARTING_MOVE && depth >= LMR_STARTING_DEPTH 
				&& !b.inCheck(b.getTurn(), b.mailbox) && !isCapture) {
				if (depth >= LMR_STARTING_DEPTH + 1 && movesSearched >= LMR_STARTING_MOVE * 4) {
					score = -negaSearch(b, depth - 3, initial, -color, -alpha - 1, -alpha, true);
				}
				else {
					score = -negaSearch(b, depth - 2, initial, -color, -alpha - 1, -alpha, true);
				}
			}
			else {
				score = alpha + 1;
			}
			if (score > alpha) {
				//PVS
				if (foundPV) {
					score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha, true);
					if (score > alpha && score < beta) {
						score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha, true);
					}
				}
				else {
					score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha, true);
				}
					
			}
		}
		b.undo();
		tempHashPosVec.pop_back();
		if (score > alpha) {
			foundPV = true;
			alpha = score;
			if (record) {
				tempHashFlag = hashTable.HASH_EXACT;
				hashTable.tt[key].hashBestMove = Move(m.from, m.to);
			}

			if (depth == initial && record) {
				bestMove.from = m.from;
				bestMove.to = m.to;
			}
		}
		if (score >= beta) {
			if (m == b.legalMoveVec[0]) { fhf++; }
			fh++;
			if (record) {
				recordHash(key, depth, beta, hashTable.HASH_BETA);
				if (!isCapture) {
					//Killer moves
					killerMoves[1][depth] = killerMoves[0][depth];
					killerMoves[0][depth] = m;
					//History heuristic
					historyMoves[(color == b.WHITE)][abs(b.mailbox[m.from]) - 1][m.to] += pow(depth, 2);
				}
			}
			return score;
		}
	}

	if (record) {
		recordHash(key, depth, alpha, tempHashFlag);
	}
	return alpha;
}
void Wowl::ID(Board& b, const Evaluation& e, int depth, int color, int secs) {

	initMVVLVA(b, e);
	b.setEnPassantSquare();
	resetMoveHeuristics();
	bestMove = Move(NO_MOVE, NO_MOVE);
	finalBestMove = bestMove;

	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	int delta = ASPIRATION_WINDOW;
	bool first_search = true;

	int prevNodes = 0;
	int totalNodes = 0;

	auto timeStart = clock();
	std::cout.precision(3);

	for (int idepth = 1; idepth <= MAX_SEARCH_DEPTH; ++idepth) {

		if ((clock() - timeStart) / CLOCKS_PER_SEC >= secs && idepth >= MIN_SEARCH_DEPTH && first_search == true) { break; }

		negaNodes = 0, qSearchNodes = 0;
		fh = 0, fhf = 0;

		tempHashPosVec = hashPosVec;

		bestScore = negaSearch(b, idepth, idepth, color, id_alpha, id_beta, true);

		totalNodes += negaNodes + qSearchNodes;

		//Break when mate is found
		if (bestScore == WIN_SCORE || bestScore == -WIN_SCORE) {
			finalBestMove = bestMove;
			std::cout << "Best move : " << b.toNotation(finalBestMove.from) << b.toNotation(finalBestMove.to) << " at depth " << idepth << std::endl;
			break;
		}

		if ((bestScore <= id_alpha) || (bestScore >= id_beta)) {
			id_beta += delta;
			id_alpha -= delta;
			delta += delta / 2;
			idepth--;
			first_search = false;
			std::cout << "Research" << std::endl;
			continue;
		}

		delta = ASPIRATION_WINDOW;
		id_alpha = bestScore - delta;
		id_beta = bestScore + delta;
		first_search = true;

		finalBestMove = bestMove;
		std::cout << "Best score : " << double(bestScore) / 100 << std::endl;
		std::cout << "Best move : " << b.toNotation(finalBestMove.from) << b.toNotation(finalBestMove.to) << " at depth " << idepth << std::endl;
		std::cout << "Nodes explored in negaSearch : " << negaNodes << std::endl;
		std::cout << "Nodes explored in qSearch : " << qSearchNodes << std::endl;
		std::cout << "Branching factor : " << double(negaNodes) / prevNodes << std::endl;
		std::cout << "Cutoff percentage : " << fhf / fh * 100 << std::endl << std::endl;
		prevNodes = negaNodes;
	}

	ageHash();
	std::cout << "Time : " << (double((clock() - timeStart)) / CLOCKS_PER_SEC) << " s" << std::endl;
	std::cout << double(totalNodes) / 1000 / (double(clock() - timeStart) / CLOCKS_PER_SEC) << " kn/s" << std::endl << std::endl;
}

int Wowl::MTDf(Board b, int f, int depth, int color) {
	int bound[2] = { -WIN_SCORE, WIN_SCORE };
	int beta;
	do {
		beta = f + (f == bound[0]);
		f = negaSearch(b, depth, depth, color, beta - 1, beta, true);
		bound[f < beta] = f;
	} while (bound[0] < bound[1]);
	return f;
}
long Wowl::perft(Board b, int depth) {

	int nodes = 0;
	b.getLegalMoves();

	if (depth == 0) { return 1; }

	int size = b.legalMoveVec.size();

	for (const auto& i : b.legalMoveVec) {
		b.move(i.from, i.to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		nodes += perft(b, depth - 1);
		b.undo();
	}
	return nodes;
}