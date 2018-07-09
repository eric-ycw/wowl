#include "stdafx.h"
#include <assert.h>
#include "Wowl.h"

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV) {
	for (int i = 0; i < lmV.size(); i++) {
		//IDD move
		if (lmV[i] == priorityMove) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
		}
	}
}

int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta) {

	Evaluation WowlEval;

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

	if (depth == SEARCH_DEPTH) {
		orderMoves(b, b.legalMoveVec);
	}

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
			if (depth == initial) {
				bestMove.x = b.legalMoveVec[j].x;
				bestMove.y = b.legalMoveVec[j].y;
				if (depth == SEARCH_DEPTH) {
					std::cout << "best move : " << bestMove.x << " " << bestMove.y << " at depth " << depth << " by " << color << std::endl;
					std::cout << "best score : " << max << std::endl;
				}
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

void Wowl::IID(Board b, std::vector<sf::Vector2i> lmV, int depth, int color) {
	priorityMove.x = -1;
	priorityMove.y = -1;
	negaSearch(b, IDD_SEARCH_DEPTH, IDD_SEARCH_DEPTH, color, -WIN_SCORE, WIN_SCORE);
	priorityMove = bestMove;
	std::cout << "IDD best move is " << priorityMove.x << " " << priorityMove.y << std::endl;
	negaSearch(b, depth, depth, color, -WIN_SCORE, WIN_SCORE);
}

void Wowl::findBestMove(Board b, int depth, int color) {
	IID(b, b.legalMoveVec, depth, color);
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