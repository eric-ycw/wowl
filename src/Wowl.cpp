#include "stdafx.h"
#include "Wowl.h"

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV, int depth, int initial, U64 poskey) {
	if (depth != SEARCH_DEPTH) {
		if (hashTable.table.find(poskey) != hashTable.table.end()) {
			int val = hashTable.table.at(poskey);
			hashMove.x = (val - val % 100) / 100;
			hashMove.y = val % 100;
		}
	}
	for (int i = 0; i < lmV.size(); i++) {
		if (depth == SEARCH_DEPTH) {
			//Move from iterative deepening
			if (lmV[i] == priorityMove) {
				lmV.insert(lmV.begin(), lmV[i]);
				lmV.erase(lmV.begin() + i + 1);
			}
		}
		else {
			//Move stored in hash table
			if (lmV[i] == hashMove) {
				lmV.insert(lmV.begin(), lmV[i]);
				lmV.erase(lmV.begin() + i + 1);
			}
		}
	}
}

int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta) {

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);

	if (depth == 0) {
		return color * WowlEval.totalEvaluation(b, WHITE);
	}

	int score;
	int max = -WIN_SCORE;

	b.getLegalMoves();
	int size = b.legalMoveVec.size();

	if (size == 0) {
		return -WIN_SCORE;
	}

	orderMoves(b, b.legalMoveVec, depth, initial, key);

	for (int j = 0; j < size; j++) {
		b.move(b.legalMoveVec[j].x, b.legalMoveVec[j].y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		else {
			//NegaScout
			if (j == 0) {
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha);
			}
			else {
				score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha);
				if (alpha < score && score < beta && depth > 1) {
					int scoutScore = -negaSearch(b, depth - 1, initial, -color, -beta, -score);
					if (scoutScore > score) {
						score = scoutScore;
					}
				}
			}
		}
		b.undo();
		if (score > max) {
			max = score;
			//Store move in hash table
			hashTable.table[key] = b.legalMoveVec[j].x * 100 + b.legalMoveVec[j].y;
			//Update best move
			if (depth == initial) {
				bestMove.x = b.legalMoveVec[j].x;
				bestMove.y = b.legalMoveVec[j].y;
			}
			if (depth == SEARCH_DEPTH) {
				std::cout << "best move : " << bestMove.x << " " << bestMove.y << " at depth " << depth << " by " << color << std::endl;
				std::cout << "best score : " << max << std::endl;
			}
		}
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			break;
		}
	}

	return max;
}

void Wowl::ID(Board b, std::vector<sf::Vector2i> lmV, int depth, int color) {
	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	for (int idepth = 1; idepth < SEARCH_DEPTH; idepth++) {
		priorityMove.x = -1;
		priorityMove.y = -1;
		estimate = negaSearch(b, idepth, idepth, color, id_alpha, id_beta);
		priorityMove = bestMove;
		std::cout << "ID best move is " << priorityMove.x << " " << priorityMove.y << " at depth " << idepth << std::endl;
		std::cout << id_alpha << " " << id_beta << " at depth " << idepth << std::endl << std::endl;
		if (estimate <= id_alpha || estimate >= id_beta) {
			id_alpha = -WIN_SCORE;
			id_beta = WIN_SCORE;
			idepth--;
			continue;
		}
		id_alpha = estimate - ASPIRATION_WINDOW;
		id_beta = estimate + ASPIRATION_WINDOW;
	}
	int nodes = 0;
	negaSearch(b, depth, depth, color, id_alpha, id_beta);
}

void Wowl::findBestMove(Board b, int depth, int color) {
	b.setEnPassantSquare();
	ID(b, b.legalMoveVec, depth, color);
}

long Wowl::perft(Board b, int depth) {

	int nodes = 0;

	if (depth == 0) { return 1; }

	b.getLegalMoves();
	int size = b.legalMoveVec.size();

	for (int i = 0; i < size; i++) {
		b.move(b.legalMoveVec[i].x, b.legalMoveVec[i].y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		nodes += perft(b, depth - 1);
		b.undo();
		if (depth == SEARCH_DEPTH) {
			std::cout << nodes << std::endl;
		}
	}

	return nodes;
}