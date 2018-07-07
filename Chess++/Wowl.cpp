#include "stdafx.h"
#include "Wowl.h"

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV) {
	for (int i = 0; i < lmV.size(); i++) {
		//Captures
		if (b.mailbox[lmV[i].y] != 0) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
		}
		//IDD move
		if (lmV[i] == priorityMove) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
		}
	}
}

int Wowl::negaMax(Board b, int depth, int initial, int color, int alpha, int beta) {


	int score;
	Evaluation WowlEval;
	int max = -WIN_SCORE;
	int a = alpha;
	static std::vector<std::vector<sf::Vector2i>> tempVec(initial  + 1);


	if (tempVec.size() <= initial) {
		tempVec.resize(initial + 1);
	}

	//Terminate at end node
	if (depth == 0) {
		return color * WowlEval.totalEvaluation(b, WHITE);
	}

	//Get legal moves and order them
	b.getLegalMoves();
	int size = b.legalMoveVec.size();
	tempVec[depth] = b.legalMoveVec;
	orderMoves(b, b.legalMoveVec);

	//Check for end state
	if (size == 0) {
		return -color * WIN_SCORE;
	}

	for (int j = 0; j < size; j++) {
		b.move(b.legalMoveVec[j].x, b.legalMoveVec[j].y); 
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		else {
			score = -negaMax(b, depth - 1, depth, -color, -beta, -alpha);
		}
		b.undo();
		if (score > max) {
			max = score;
			if (depth == initial) {
				bestMove.x = b.legalMoveVec[j].x;
				bestMove.y = b.legalMoveVec[j].y;
				if (initial == SEARCH_DEPTH) {
					std::cout << "move : " << b.legalMoveVec[j].x << " " << b.legalMoveVec[j].y << std::endl;
					std::cout << "score : " << score << std::endl << std::endl;
				}
			}
		}
		b.legalMoveVec = tempVec[depth];
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			break;
		}
	}

	return max;
}

void Wowl::IID(Board b, std::vector<sf::Vector2i>& lmV, int depth, int color) {
	priorityMove.x = -1;
	priorityMove.y = -1;
	negaMax(b, depth - 2,  depth - 2, color, -WIN_SCORE, WIN_SCORE);
	priorityMove = bestMove;
	std::cout << "IDD best move is " << priorityMove.x << " " << priorityMove.y << std::endl;
	negaMax(b, depth, depth, color, -WIN_SCORE, WIN_SCORE);
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
		nodes += perft(b, depth - 1);
		b.undo();
		if (depth == SEARCH_DEPTH) {
			std::cout << nodes << std::endl;
		}
	}

	return nodes;
}