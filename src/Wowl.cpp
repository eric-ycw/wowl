#include "Wowl.h"

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
				if (netMaterial > 0) {
					val = netMaterial;
				}
				else {
					val = 0;
				}
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

std::vector<Move> Wowl::IIDmoveOrdering(Board& b, Evaluation& e, std::vector<Move> lmV, int depth, int alpha, int beta) {
	std::vector<int> scoreVec(lmV.size());
	bool isCapture;

	for (int i = 0; i < lmV.size(); ++i) {
		isCapture = (b.mailbox[lmV[i].to] != 0) ? true : false;
		b.move(lmV[i].from, lmV[i].to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			scoreVec[i] = -WIN_SCORE;
			b.undo();
			continue;
		}
		if (isCapture) {
			scoreVec[i] = qSearch(b, e, alpha, beta, b.getTurn() * -1);
		}
		else {
			scoreVec[i] = e.totalEvaluation(b, b.getTurn() * -1);
		}
		b.undo();
	}
	int i = 1;
	int j;
	while (i < lmV.size()) {
		j = i;
		while (j > 0 && scoreVec[j - 1] < scoreVec[j]) {
			int dummy_score = scoreVec[j];
			scoreVec[j] = scoreVec[j - 1];
			scoreVec[j - 1] = dummy_score;

			Move dummy_move = lmV[j];
			lmV[j] = lmV[j - 1];
			lmV[j - 1] = dummy_move;

			j -= 1;
		}
		i += 1;
	}
	return lmV;
}
void Wowl::orderMoves(Board& b, Evaluation& e, std::vector<Move>& lmV, int depth, U64 poskey, int alpha, int beta) {
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		hashMove = hashTable.tt.at(poskey).hashBestMove;
	}
	int orderArray[4] = { -1 };
	int history_max = 0;
	bool ordered = false;
	
	if (depth > 1) {
		lmV = IIDmoveOrdering(b, e, b.legalMoveVec, depth, alpha, beta);
	}

	for (int i = 0; i < lmV.size(); ++i) {
		//Hash table move
		if (lmV[i] == hashMove) {
			orderArray[0] = i;
			continue;
		}
		//Good captures
		if (b.mailbox[lmV[i].to] != 0) {
			if (SEE(b, e, lmV[i].to, b.getTurn()) > 0 && std::get<1>(b.getSmallestAttacker(lmV[i].to, b.getTurn())) == lmV[i].from) {
				orderArray[1] = i;
				continue;
			}
		}
		//Killer moves
		if (lmV[i] == killerMoves[0][depth]) {
			orderArray[2] = i;
			continue;
		}
		if (lmV[i] == killerMoves[1][depth]) {
			orderArray[3] = i;
			continue;
		}
		//History heuristic
		/*int history_score = historyMoves[(b.getTurn() == b.WHITE)][lmV[i].from][lmV[i].to];
		if (history_score > history_max) {
			orderArray[4] = i;
			history_max = history_score;
			continue;
		}*/
	}
	for (int i = 4; i >= 0; --i) {
		if (orderArray[i] >= 0) {
			ordered = true;
			lmV.insert(lmV.begin(), lmV[orderArray[i]]);
			lmV.erase(lmV.begin() + orderArray[i] + 1);
		}
	}
}
void Wowl::resetMoveHeuristics() {
	for (int i = 0; i < MAX_SEARCH_DEPTH; ++i) {
		killerMoves[0][i] = Move(NO_MOVE, NO_MOVE);
		killerMoves[1][i] = Move(NO_MOVE, NO_MOVE);
	}
	int historyMoves[2][120][120] = {{{0}}};
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

	int score;
	Evaluation qEval;

	int stand_pat = qEval.totalEvaluation(b, color);

	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}

	for (const auto& j : b.captureVec) {
		if (SEE(b, e, j.to, color) > 0) {
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

	negaNodes++;

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
		if (b.inCheck(color, b.mailbox)) {
			return -WIN_SCORE;  //Checkmate
		}
		else {
			return DRAW_SCORE;  //Stalemate
		}
	}

	if ((b.inCheck(color, b.mailbox) || b.inCheck(color * -1, b.mailbox)) && depth == 0) {
		depth++;
	}

	if (depth == 0) {
		int qScore = qSearch(b, WowlEval, alpha, beta, color);
		recordHash(key, depth, qScore, hashTable.HASH_EXACT);
		return qScore;
	}

	if (probeHashTable(key, depth, initial, alpha, beta) != VAL_UNKWOWN) {
		return probeHashTable(key, depth, initial, alpha, beta); 
	}

	int score;

	//Null move pruning
	if (depth - 1 - NULL_MOVE_REDUCTION >= 0 && !b.inCheck(color, b.mailbox) && !b.inCheck(-color, b.mailbox) && WowlEval.getGamePhase() != WowlEval.ENDGAME) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - NULL_MOVE_REDUCTION, initial, -color, -beta, -beta + 1, false);
		b.nullMove();
		if (score >= beta) {
			return beta;
		}
	}

	orderMoves(b, WowlEval, b.legalMoveVec, depth, key, alpha, beta);
	
	bool foundPV = false;
	bool isCapture;
	int movesSearched = 0;

	for (const auto& i : b.legalMoveVec) {
		movesSearched++;
		isCapture = (b.mailbox[i.to] != 0) ? true : false;
		b.move(i.from, i.to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		else {
			tempHashPosVec.emplace_back(key);

			//Late move reductions
			if (movesSearched >= LMR_STARTING_MOVE && depth >= LMR_STARTING_DEPTH 
				&& !b.inCheck(b.getTurn(), b.mailbox) && !isCapture) {
				if (depth >= LMR_STARTING_DEPTH + 1 && movesSearched >= LMR_STARTING_MOVE * 3) {
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
				if (initial > 1 && foundPV) {
					score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha, true);
					if (score > alpha && score < beta) {
						score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha, true);
					}
					foundPV = true;
				}
				else {
					score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha, true);
				}
			}
		}
		b.undo();
		tempHashPosVec.pop_back();
		if (score > alpha) {
			alpha = score;
			foundPV = true;
			if (record) {
				tempHashFlag = hashTable.HASH_EXACT;
				hashTable.tt[key].hashBestMove = Move(i.from, i.to);
			}

			if (depth == initial && record) {
				bestMove.from = i.from;
				bestMove.to = i.to;
			}
		}
		if (score >= beta) {
			if (record) {
				recordHash(key, depth, beta, hashTable.HASH_BETA);
				//Killer moves
				killerMoves[1][depth] = killerMoves[0][depth];
				killerMoves[0][depth] = i;
				//History heuristic
				if (!isCapture) {
					historyMoves[(color == b.WHITE)][i.from][i.to] += pow(depth, 2);
				}
			}
			return beta;
		}
	}

	if (record) {
		recordHash(key, depth, alpha, tempHashFlag);
	}
	return alpha;
}
void Wowl::ID(Board& b, int depth, int color, int secs) {

	b.setEnPassantSquare();
	resetMoveHeuristics();
	bestMove = Move(NO_MOVE, NO_MOVE);
	finalBestMove = bestMove;

	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	int delta = ASPIRATION_WINDOW;
	bool first_search = true;
	int prevNodes = 0;
	negaNodes = 0;
	qSearchNodes = 0;

	auto timeStart = clock();

	for (int idepth = 1; idepth <= MAX_SEARCH_DEPTH; ++idepth) {

		if ((clock() - timeStart) / CLOCKS_PER_SEC >= secs && idepth >= MIN_SEARCH_DEPTH) { break; }


		negaNodes = 0;
		qSearchNodes = 0;

		tempHashPosVec = hashPosVec;

		bestScore = negaSearch(b, idepth, idepth, color, id_alpha, id_beta, true);

		//Break when mate is found
		if (bestScore == WIN_SCORE || bestScore == -WIN_SCORE) {
			finalBestMove = bestMove;
			std::cout << "Best move : " << b.toNotation(finalBestMove.from) << b.toNotation(finalBestMove.to) << " at depth " << idepth << std::endl;
			break;
		}

		if (bestScore <= id_alpha || bestScore >= id_beta) {
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
		std::cout << "Best move : " << b.toNotation(finalBestMove.from) << b.toNotation(finalBestMove.to) << " at depth " << idepth << std::endl;
		std::cout << "Nodes explored in negaSearch : " << negaNodes << std::endl;
		std::cout << "Nodes explored in qSearch : " << qSearchNodes << std::endl;
		std::cout.precision(3);
		std::cout << "Branching factor : " << double(negaNodes) / prevNodes << std::endl << std::endl;
		prevNodes = negaNodes;
	}

	ageHash();
	std::cout << "Transposition table size : " << hashTable.tt.size() << std::endl;
	
	if (bestScore >= 0) {
		std::cout << "Wowl thinks it is winning by " << bestScore << " centipanws" << std::endl << std::endl;
	}
	else {
		std::cout << "Wowl thinks it is losing by " << bestScore * -1 << " centipanws" << std::endl << std::endl;
	}
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

	for (int i = 0; i < b.captureVec.size(); ++i) {
		b.move(b.captureVec[i].from, b.captureVec[i].to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		captures++;
		b.undo();
	}

	if (depth == 0) { return 1; }

	int size = b.legalMoveVec.size();

	for (int i = 0; i < size; ++i) {
		b.move(b.legalMoveVec[i].from, b.legalMoveVec[i].to);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		nodes += perft(b, depth - 1);
		b.undo();
		std::cout << "Nodes : " << nodes << std::endl;
		std::cout << "Captures : " << captures << std::endl;
	}
	return nodes;
}