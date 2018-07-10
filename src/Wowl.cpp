#include "stdafx.h"
#include <assert.h>
#include "Wowl.h"

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV) {
	for (int i = 0; i < lmV.size(); i++) {
		//ID move
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
	for (int idepth = 1; idepth < SEARCH_DEPTH; idepth++) {
		priorityMove.x = -1;
		priorityMove.y = -1;
		estimate = negaSearch(b, idepth, idepth, color, -WIN_SCORE, WIN_SCORE);
		priorityMove = bestMove;
		std::cout << "ID best move is " << priorityMove.x << " " << priorityMove.y << std::endl;
	}
	int nodes = 0;
	std::cout << estimate - ASPIRATION_WINDOW << " " << estimate + ASPIRATION_WINDOW << std::endl;
	negaSearch(b, depth, depth, color, estimate - ASPIRATION_WINDOW, estimate + ASPIRATION_WINDOW);
}

void Wowl::findBestMove(Board b, int depth, int color) {
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