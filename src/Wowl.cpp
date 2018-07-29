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
					targetval = e.N_BASE_VAL;
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
				assert(targetval > 0);
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

void Wowl::orderMoves(Board& b, Evaluation& e, std::vector<sf::Vector2i>& lmV, int depth, U64 poskey) {
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		hashMove = hashMoveToVec(poskey);
	}
	int orderArray[3] = { -1, -1, -1 };
	for (int i = 0; i < lmV.size(); i++) {
		//Hash table move
		if (lmV[i] == hashMove) {
			orderArray[2] = i;
			continue;
		}
		//Killer moves
		if (lmV[i].x == (killerMoves[0][depth] - killerMoves[0][depth] % 100) / 100 && lmV[i].y == killerMoves[0][depth] % 100) {
			orderArray[1] = i;
			continue;
		}
		if (lmV[i].x == (killerMoves[1][depth] - killerMoves[1][depth] % 100) / 100 && lmV[i].y == killerMoves[1][depth] % 100) {
			orderArray[0] = i;
			continue;
		}
	}
	for (int i = 0; i < 3; i++) {
		if (orderArray[i] >= 0) {
			lmV.insert(lmV.begin(), lmV[orderArray[i]]);
			lmV.erase(lmV.begin() + orderArray[i] + 1);
		}
	}
}
void Wowl::resetKillerMoves() {
	for (int i = 0; i < MAX_SEARCH_DEPTH; i++) {
		killerMoves[0][i] = 0;
		killerMoves[1][i] = 0;
	}
}

sf::Vector2i Wowl::hashMoveToVec(U64 key) {
	sf::Vector2i move;
	int val = hashTable.tt.at(key).hashBestMove;
	move.x = (val - val % 100) / 100;
	move.y = val % 100;
	return move;
}
int Wowl::probeHashTable(U64 key, int depth, int initial, int alpha, int beta) {
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
				if (depth == initial) { bestMove = hashMoveToVec(key); }
				return hashTable.tt.at(key).hashScore;
			}
		}
		return VAL_UNKWOWN;
	}
	return VAL_UNKWOWN;
}
void Wowl::recordHash(U64 key, int depth, int score, int flag) {
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

	b.getQMoves();
	for (const auto& j : b.qMoveVec) {
		if (SEE(b, e, j.y, color) > 0) {
			qSearchNodes++;
			b.move(j.x, j.y);
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
int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta) {

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);
	int tempHashFlag = hashTable.HASH_ALPHA;

	if (depth == 0) {
		negaNodes++;
		int qScore = qSearch(b, WowlEval, alpha, beta, color);
		recordHash(key, depth, qScore, hashTable.HASH_EXACT);
		return qScore;
	}

	if (probeHashTable(key, depth, initial, alpha, beta) != VAL_UNKWOWN) { return probeHashTable(key, depth, initial, alpha, beta); }

	int score;

	//Null move pruning
	if (depth - 1 - NULL_MOVE_REDUCTION >= 0 && !b.inCheck(color, b.mailbox) && WowlEval.getGamePhase() != WowlEval.ENDGAME) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - NULL_MOVE_REDUCTION, initial, -color, -beta, -beta + 1);
		b.nullMove();
		if (score >= beta) {
			return beta;
		}
	}

	b.getLegalMoves();
	bool haveMove = false;
	for (const auto& i : b.legalMoveVec) {
		if (!b.checkMoveCheck(i.x, i.y)) {
			haveMove = true;
			break;
		}
	}
	if (!haveMove) {
		if (depth == initial) {
			bestMove.x = NO_MOVE;
			bestMove.y = NO_MOVE;
		}
		if (b.inCheck(color, b.mailbox)) {
			return -WIN_SCORE + depth;  //Checkmate
		}
		else {
			return DRAW_SCORE;  //Stalemate
		}
	}

	orderMoves(b, WowlEval, b.legalMoveVec, depth, key);

	bool foundPV = false;

	for (const auto& i : b.legalMoveVec) {
		b.move(i.x, i.y);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		else {
			negaNodes++;
			//PVS	
			if (foundPV) {
				score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha);
				if (score > alpha && score < beta) {
					score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha);
				}
			}
			else {
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha);
			}
		}
		b.undo();
		if (score > alpha) {
			alpha = score;
			tempHashFlag = hashTable.HASH_EXACT;
			foundPV = true;
			hashTable.tt[key].hashBestMove = i.x * 100 + i.y;

			if (depth == initial) {
				bestMove.x = i.x;
				bestMove.y = i.y;
			}
		}
		if (score >= beta) {
			recordHash(key, depth, beta, hashTable.HASH_BETA);
			killerMoves[1][depth] = killerMoves[0][depth];
			killerMoves[0][depth] = i.x * 100 + i.y;
			return beta;
		}
	}

	recordHash(key, depth, alpha, tempHashFlag);
	return alpha;
}
void Wowl::ID(Board& b, int depth, int color, int secs) {

	negaNodes = 0;
	qSearchNodes = 0;
	b.setEnPassantSquare();
	resetKillerMoves();
	bestMove.x = NO_MOVE;
	bestMove.y = NO_MOVE;
	finalBestMove = bestMove;

	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	bool researched_once = false;

	auto timeStart = clock();

	for (int idepth = 1; idepth <= MAX_SEARCH_DEPTH; idepth++) {

		if ((clock() - timeStart) / CLOCKS_PER_SEC >= secs) { break; }

		estimate = negaSearch(b, idepth, idepth, color, id_alpha, id_beta);

		if ((estimate <= id_alpha) || (estimate >= id_beta)) {
			if (researched_once) {
				id_alpha = -WIN_SCORE;
				id_beta = WIN_SCORE;
			}
			else {
				id_alpha = estimate - ASPIRATION_WINDOW * 2;
				id_beta = estimate + ASPIRATION_WINDOW * 2;
			}
			idepth--;
			researched_once = true;
			continue;
		}

		id_alpha = estimate - ASPIRATION_WINDOW;
		id_beta = estimate + ASPIRATION_WINDOW;
		researched_once = false;

		finalBestMove = bestMove;
		std::cout << "Best move : " << b.toNotation(finalBestMove.x) << b.toNotation(finalBestMove.y) << " at depth " << idepth << std::endl << std::endl;
	}

	ageHash();

	std::cout << "Transposition table size : " << hashTable.tt.size() << std::endl;
	std::cout << "Nodes explored in negaSearch : " << negaNodes << std::endl;
	std::cout << "Nodes explored in qSearch : " << qSearchNodes << std::endl << std::endl;
}
int Wowl::MTDf(Board b, int f, int depth, int color) {
	int bound[2] = { -WIN_SCORE, WIN_SCORE };
	int beta;
	do {
		beta = f + (f == bound[0]);
		f = negaSearch(b, depth, depth, color, beta - 1, beta);
		bound[f < beta] = f;
	} while (bound[0] < bound[1]);
	return f;
}
long Wowl::perft(Board b, int depth) {

	int nodes = 0;

	b.getQMoves();
	for (int i = 0; i < b.qMoveVec.size(); i++) {
		b.move(b.qMoveVec[i].x, b.qMoveVec[i].y);
		if (b.inCheck(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		captures++;
		b.undo();
	}

	if (depth == 0) { return 1; }

	b.getLegalMoves();
	int size = b.legalMoveVec.size();

	for (int i = 0; i < size; i++) {
		b.move(b.legalMoveVec[i].x, b.legalMoveVec[i].y);
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