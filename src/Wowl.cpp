#include "Wowl.h"

void Wowl::initMVVLVA(const Board& b, const Evaluation& e) {
	//Same value for knight and bishop
	int pieceValue[6] = { e.P_BASE_VAL, e.B_BASE_VAL, e.B_BASE_VAL, e.R_BASE_VAL, e.Q_BASE_VAL, e.K_BASE_VAL };
	for (int attacker = b.WP; attacker <= b.WK; attacker++) {
		for (int victim = b.WP; victim <= b.WK; victim++) {
			MVVLVAScores[attacker - 1][victim - 1] = pieceValue[victim - 1] * 100 - attacker;
		}
	}
}
int Wowl::SEE(Board& b, const Evaluation& e, int square, int color) const {
	int oldsqr;
	int val = 0;
	int piece = std::get<0>(b.getSmallestAttacker(square, color));
	int target = b.mailbox[square];
	int targetval = target;

	if (piece > 0) {
		if (!(target == -9 || target == 0)) {
			oldsqr = std::get<1>(b.getSmallestAttacker(square, color));
			b.move(oldsqr, square);
			if (b.inCheck(b.getTurn() * -1)) {
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
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo();
			continue;
		}
		if (!isCapture) { score = e.totalEvaluation(b, -b.getTurn()); }
		if (score > best && i > 0) {
			best = score;
			std::swap(lmV[0], lmV[i]);
		}
		b.undo();
	}

}
std::vector<int> Wowl::scoreMoves(Board& b, Evaluation& e, const std::vector<Move>& lmV, const int depth, const U64 poskey) {
	
	std::vector<int> scoreVec(lmV.size(), 0);
	
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		hashMove = hashTable.tt.at(poskey).hashBestMove;
	}
	else {
		hashMove = Move(NO_MOVE, NO_MOVE);
	}


	bool isCapture, isPassed, isPromotion;

	for (int i = 0; i < lmV.size(); ++i) {
		isCapture = (b.mailbox[lmV[i].to] != 0) ? true : false;
		isPassed = e.isPassed(b, lmV[i].from, b.getTurn());
		isPromotion = (b.mailbox[lmV[i].from] == b.WP * b.getTurn() && (lmV[i].to / 10 == 2 || lmV[i].to / 10 == 9)) ? true : false;

		//Hash table move
		if (lmV[i] == hashMove) {
			scoreVec[i] = 10000000;
			continue;
		}
		//Promotion
		if (isPromotion) {
			scoreVec[i] = 1000000;
			continue;
		}
		//Good captures
		if (isCapture) {
			int attacker = b.mailbox[lmV[i].from];
			int victim = b.mailbox[lmV[i].to];
			scoreVec[i] = MVVLVAScores[abs(attacker) - 1][abs(victim) - 1];
			continue;
		}
		//Passed pawn move
		if (isPassed) {
			scoreVec[i] = 25000;
			continue;
		}
		//Killer moves
		if (lmV[i] == killerMoves[0][depth]) {
			scoreVec[i] = 15000;
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
void Wowl::pickNextMove(std::vector<Move>& moveVec, std::vector<int>& scoreVec, int startpos) {
	int best = -WIN_SCORE;
	int bestMoveIndex = 0;
	for (int i = startpos; i < scoreVec.size(); ++i) {
		if (scoreVec[i] > best) {
			best = scoreVec[i];
			bestMoveIndex = i;
		}
	}

	std::swap(moveVec[startpos], moveVec[bestMoveIndex]);
	std::swap(scoreVec[startpos], scoreVec[bestMoveIndex]);
}
std::vector<int> Wowl::scoreCaptures(Board& b, Evaluation& e, const std::vector<Move>& cV, const int color) {
	std::vector<int> scoreVec(cV.size(), 0);
	for (int i = 0; i < cV.size(); ++i) {
		scoreVec[i] = SEE(b, e, b.captureVec[i].to, color >= 0);
	}
	return scoreVec;
}
void Wowl::orderCaptures(Board& b, std::vector<Move>& cV) {	int best = -WIN_SCORE;
	int score;
	for (int i = 0; i < cV.size(); ++i) {
		score = MVVLVAScores[abs(b.mailbox[cV[i].from]) - 1][abs(b.mailbox[cV[i].to]) - 1];
		if (score > best) {
			best = score;
			std::swap(cV[0], cV[i]);
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
	U64 key = hashTable.generatePosKey(b);

	int hashScore = probeHashTable(key, 0, -1, alpha, beta);
	if (hashScore != VAL_UNKWOWN) {
		stand_pat = hashScore;
	}
	else {
		stand_pat = qEval.totalEvaluation(b, color);
	}

	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}
	
	b.getCaptures();
	orderCaptures(b, b.captureVec);
	int pieceValues[5] = { e.P_BASE_VAL, e.N_BASE_VAL, e.B_BASE_VAL, e.R_BASE_VAL, e.Q_BASE_VAL };

	for (int i = 0; i < b.captureVec.size(); ++i) {
		//Delta pruning
		int piece = b.mailbox[b.captureVec[i].to] * color;
		if (stand_pat + pieceValues[piece - 1] + DELTA_MARGIN < alpha && e.phase > 0.25) { 
			continue; 
		}
		//Negative SEE
		if (SEE(b, e, b.captureVec[i].to, color) < 0) {
			continue;
		}
		qSearchNodes++;
		b.move(b.captureVec[i].from, b.captureVec[i].to);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo();
			continue;
		}
		score = -qSearch(b, e, -beta, -alpha, -color);
		b.undo();
		if (score >= beta) {
			if (i >= 0 && i < 4) {
				qfhf[i]++;
			}
			qfh++;
			return beta;
		}
		if (score > alpha) {
			alpha = score;
		}
	}

	return alpha;
}
int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta, bool can_null, bool is_pv) {

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);
	int tempHashFlag = hashTable.HASH_ALPHA;
	bool isInCheck = b.inCheck(color);

	if (checkThreefold(key)) { return DRAW_SCORE; }

	int hashScore = probeHashTable(key, depth, initial, alpha, beta);
	if (hashScore != VAL_UNKWOWN) {
		return hashScore;
	}

	if (depth == 0) {
		if (!isInCheck) {
			negaNodes++;
			int qScore = qSearch(b, WowlEval, alpha, beta, color);
			recordHash(key, depth, qScore, hashTable.HASH_EXACT);
			return qScore;
		}
		else {
			depth++;
		}
	}

	int score;

	//Null move pruning
	int R = NULL_MOVE_REDUCTION + depth / 6;
	if (depth - R - 1 >= 0 && can_null && !isInCheck && WowlEval.getPhase(b) > 0.25 && !is_pv) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - R, initial, -color, -beta, -beta + 1, false, false);
		b.undo();
		if (score >= beta) {
			return beta;
		}
	}

	//Futility pruning
	bool f_prune = 0;
	if (depth <= 3 && !isInCheck && WowlEval.getPhase(b) > 0.25 && !is_pv) {
		int futilityScore = WowlEval.totalEvaluation(b, color);
		if (futilityScore + futilityMargin[depth - 1] <= alpha && futilityScore + futilityMargin[depth - 1] < WIN_SCORE) {
			f_prune = 1;
		}
	};

	b.getLegalMoves();
	bool haveMove = false;
	bool raised_alpha = false;
	bool isCapture, isPassed, isPromotion, isKiller;
	int movesSearched = 0;

	std::vector<int> scoreVec = scoreMoves(b, WowlEval, b.legalMoveVec, depth, key);

	for (int i = 0; i < b.legalMoveVec.size(); i++) {

		pickNextMove(b.legalMoveVec, scoreVec, i);

		Move m = b.legalMoveVec[i];
		movesSearched++;
		isCapture = (b.mailbox[m.to] != 0) ? true : false;
		isPassed = WowlEval.isPassed(b, m.from, color);
		isPromotion = (b.mailbox[m.from] == b.WP * color && (m.to / 10 == 2 || m.to / 10 == 9)) ? true : false;
		isKiller = (killerMoves[0][depth] == m || killerMoves[1][depth] == m) ? true : false;

		if (f_prune == 1 && !isCapture && !isPromotion && !isKiller) { continue; }

		b.move(m.from, m.to);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo();
			continue;
		}
		else {

			negaNodes++;
			tempHashPosVec.emplace_back(key);
			haveMove = true;

			//Late move reduction
			if (movesSearched >= LMR_STARTING_MOVE && depth >= LMR_STARTING_DEPTH
				&& !b.inCheck(b.getTurn()) && !isCapture && !isPassed && !isPromotion && !isKiller) {
				int R = 1;
				if (depth >= LMR_STARTING_DEPTH + 1) {
					R += movesSearched / (LMR_STARTING_MOVE * 3);
					if (depth - R - 1 <= 0) {
						R = depth - 2;
					}
				}
				score = -negaSearch(b, depth - R - 1, initial, -color, -alpha - 1, -alpha, true, false);
			}
			else {
				score = alpha + 1;
			}
			if (score > alpha) {
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha, true, true);
			}

		}
		b.undo();
		tempHashPosVec.pop_back();
		if (score > alpha) {

			if (depth == initial && can_null) {
				bestMove.from = m.from;
				bestMove.to = m.to;
			}

			if (score >= beta) {
				if (i >= 0 && i < 4) {
					fhf[i]++;
				}
				fh++;
				if (can_null) {
					recordHash(key, depth, beta, hashTable.HASH_BETA);
					if (!isCapture && !isPromotion) {
						//Killer moves
						killerMoves[1][depth] = killerMoves[0][depth];
						killerMoves[0][depth] = m;
						//History heuristic
						historyMoves[(color == b.WHITE)][abs(b.mailbox[m.from]) - 1][m.to] += pow(depth, 2);
					}
				}
				return score;
			}

			raised_alpha = true;
			alpha = score;
			if (can_null) {
				hashTable.tt[key].hashBestMove = m;
				tempHashFlag = hashTable.HASH_EXACT;
			}
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

	if (can_null) {
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

	fh = 0, qfh = 0;
	for (int i = 0; i < 4; ++i) { 
		fhf[i] = 0;
		qfhf[i] = 0;
	}

	auto timeStart = clock();
	std::cout.precision(3);

	for (int idepth = 1; idepth <= MAX_SEARCH_DEPTH; ++idepth) {

		negaNodes = 0, qSearchNodes = 0;

		tempHashPosVec = hashPosVec;

		if ((clock() - timeStart) / CLOCKS_PER_SEC >= secs && idepth >= MIN_SEARCH_DEPTH) { break; }

		bestScore = negaSearch(b, idepth, idepth, color, id_alpha, id_beta, true, true);
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
		outputSearchInfo(b, idepth, prevNodes);
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
		f = negaSearch(b, depth, depth, color, beta - 1, beta, true, false);
		bound[f < beta] = f;
	} while (bound[0] < bound[1]);
	return f;
}

void Wowl::outputSearchInfo(const Board& b, const int depth, const int prevNodes) {
	std::cout << "Best score : " << double(bestScore) / 100 << std::endl;
	std::cout << "Best move : " << b.toNotation(finalBestMove.from) << b.toNotation(finalBestMove.to) << " at depth " << depth << std::endl;
	std::cout << "Nodes explored in negaSearch : " << negaNodes << std::endl;
	std::cout << "Nodes explored in qSearch : " << qSearchNodes << std::endl;
	if (negaNodes > 0) {
		std::cout << "Branching factor : " << double(negaNodes) / prevNodes << std::endl;
		std::cout << "NegaSearch cutoff percentage : ";
		for (int num = 0; num < 4; ++num) {
			std::cout << fhf[num] / fh * 100 << " ";
		}
		std::cout << std::endl;
		std::cout << "qSearch cutoff percentage : ";
		for (int num = 0; num < 4; ++num) {
			std::cout << qfhf[num] / qfh * 100 << " ";
		}
	}
	std::cout << std::endl << std::endl;
}
long Wowl::perft(Board b, int depth) {

	int nodes = 0;
	b.getLegalMoves();

	if (depth == 0) { return 1; }

	int size = b.legalMoveVec.size();

	for (const auto& i : b.legalMoveVec) {
		b.move(i.from, i.to);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo();
			continue;
		}
		nodes += perft(b, depth - 1);
		b.undo();
	}
	return nodes;
}