#include "stdafx.h"
#include "Board.h"

/*CONVERTERS*/
//Returns coordinates without SIZE factor
sf::Vector2f Board::toCoord(char a, char b) {
	int x = int(a) - 97;
	int y = 7 - int(b) + 49;
	return sf::Vector2f(x, y);
}
//Converts coord vector to mailbox coord
int Board::convertCoord(sf::Vector2f vec) {
	int pos = mailbox64[int(vec.y)][int(vec.x)];
	return pos;
}
//Converts mailbox coord to coord vector (without SIZE factor)
sf::Vector2i Board::convertCoord(int pos) {
	int coord_x = pos % 10 - 1;
	int coord_y = (pos - coord_x) / 10 - 2;
	return sf::Vector2i(int(coord_x), int(coord_y));
}
//Converts mailbox coord to 64 coord
int Board::to64Coord(int square) {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = square - 21 - 2 * (tenth - 2);
	return rval;
}

/*VECTORS*/
//Reserve memory for vectors
void Board::reserveVectors() {
	moveVec.reserve(200);
	legalMoveVec.reserve(999);
	attackMoveVec.reserve(999);
}

/*LEGALITY*/
//Check move legality (pseudo-legal moves)
bool Board::checkLegal(int a, int b) {
	bool legal = true;
	int oldSquareVal = mailbox[a];
	int newSquareVal = mailbox[b];

	//Turn + own piece
	if (turn == WHITE) {
		if (oldSquareVal < 0 || newSquareVal > 0) {
			legal = false;
			return legal;
		}
	}
	else if (turn == BLACK) {
		if (oldSquareVal > 0 || newSquareVal < 0) {
			legal = false;
			return legal;
		}
	}

	//Out of bounds
	if (newSquareVal == -9) {
		legal = false;
		return legal;
	}

	//Destination cannot be king
	if (abs(newSquareVal) == 6) {
		legal = false;
		return legal;
	}

	/*MOVE PATTERNS*/
	int sval;
	if (oldSquareVal < 0) {
		sval = oldSquareVal * -1;
	}
	else {
		sval = oldSquareVal;
	}
	switch (sval) {
	case WP:
	{
		legal = false;
		if (oldSquareVal == WP) {
			//Pawns cannot move backwards
			if (b > a) {
				return legal;
			}
			//Check for second-rank pawns
			if (a <= 88 && a >= 81) {
				if (a - 20 == b) {
					if (newSquareVal == 0 && mailbox[b + 10] == 0) {
						legal = true;
						return legal;
					}
				}
			}
			//Move forward one square
			if (a - 10 == b) {
				if (newSquareVal == 0) {
					legal = true;
					return legal;
				}
			}
			//Diagonal capture
			if (a - 10 - 1 == b || a - 10 + 1 == b) {
				if (newSquareVal < 0) {
					legal = true;
					return legal;
				}
			}
			//En passant
			//Check if on fifth rank
			if (a <= 58 && a >= 51) {
				//Black pawn adjacency
				if (mailbox[a - 1] == -1 && newSquareVal == 0 && b == a - 10 - 1) {
					//Check last move
					if (moveVec.back().x == a - 20 - 1 && moveVec.back().y == a - 1) {
						legal = true;
						return legal;
					}
				}
				if (mailbox[a + 1] == -1 && newSquareVal == 0 && b == a - 10 + 1) {
					if (moveVec.back().x == a - 20 + 1 && moveVec.back().y == a + 1) {
						legal = true;
						return legal;
					}
				}
			}
		}
		else if (oldSquareVal == BP) {
			if (b < a) {
				return legal;
			}
			if (a <= 38 && a >= 31) {
				if (a + 20 == b) {
					if (newSquareVal == 0 && mailbox[b - 10] == 0) {
						legal = true;
						return legal;
					}
				}
			}
			if (a + 10 == b) {
				if (newSquareVal == 0) {
					legal = true;
					return legal;
				}
			}
			if (a + 10 - 1 == b || a + 10 + 1 == b) {
				if (newSquareVal > 0) {
					legal = true;
					return legal;
				}
			}
			if (a <= 68 && a >= 61) {
				if (mailbox[a - 1] == 1 && newSquareVal == 0 && b == a + 10 - 1) {
					if (moveVec.back().x == a + 20 - 1 && moveVec.back().y == a - 1) {
						legal = true;
						return legal;
					}
				}
				if (mailbox[a + 1] == 1 && newSquareVal == 0 && b == a + 10 + 1) {
					if (moveVec.back().x == a + 20 + 1 && moveVec.back().y == a + 1) {
						legal = true;
						return legal;
					}
				}
			}
		}
	}
		break;
	case WN:
		return checkAttackKnight(a, b, mailbox);
		break;
	case WB:
		return checkAttackBishop(a, b, mailbox);
		break;
	case WR:
		return checkAttackRook(a, b, mailbox);
		break;
	case WQ:
		return checkAttackQueen(a, b, mailbox);
		break;
	case WK:
	{
		legal = true;
		if (!(a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b || a + 2 == b || a - 2 == b)) {
			legal = false;
			return legal;
		}
		if (turn == WHITE) {
			//Short castling
			if (a + 2 == b && std::get<0>(checkCastling()) == false) {
				legal = false;
				return legal;
			}
			//Long castling
			if (a - 2 == b && std::get<1>(checkCastling()) == false) {
				legal = false;
				return legal;
			}
		}
		else if (turn == BLACK) {
			//Short castling
			if (a + 2 == b && std::get<2>(checkCastling()) == false) {
				legal = false;
				return legal;
			}
			//Long castling
			if (a - 2 == b && std::get<3>(checkCastling()) == false) {
				legal = false;
				return legal;
			}
		}
		return legal;
	}
		break;
	}
	return legal;
}
//Get all pseudo legal moves for current turn
void Board::getLegalMoves() {
	//Clear legal move vector
	if (!legalMoveVec.empty()) {
		legalMoveVec.clear();
	}
	//Get all legal moves
	for (int m = 21; m < 99; m++) {
		if (mailbox[m] == -9) {
			continue;
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == -9) {
					continue;
				}
				else {
					if (mailbox[m] * turn > 0 && mailbox[n] * turn <= 0) {
						if (checkLegal(m, n)) {
							legalMoveVec.emplace_back(sf::Vector2i(m, n));
						}

					}
				}
			}
		}
	}
}

/*ATTACKS*/
//Check if square can be attacked
bool Board::checkAttack(int a, int b, const int arr[]) {
	bool canattack = true;

	int oldSquareVal = arr[a];
	int newSquareVal = arr[b];

	/*MOVE PATTERNS*/
	int sval;
	if (oldSquareVal < 0) {
		sval = oldSquareVal * -1;
	}
	else {
		sval = oldSquareVal;
	}
	switch (sval) {
	case WP:
		if (oldSquareVal == 1) {
			return checkAttackPawn(a, b, arr, WHITE);
		}
		else {
			return checkAttackPawn(a, b, arr, BLACK);
		}
		break;

	case WN:
		return checkAttackKnight(a, b, arr);
		break;

	case WB:
		return checkAttackBishop(a, b, arr);
		break;

	case WR:
		return checkAttackRook(a, b, arr);
		break;

	case WQ:
		return checkAttackQueen(a, b, arr);
		break;

	case WK:
		return checkAttackKing(a, b, arr);
		break;

	}
}
bool Board::checkAttackPawn(int a, int b, const int arr[], int color) {
	bool canattack = false;
	if ((a - color * 11 == b || a - color * 9 == b) && arr[b] * arr[a] <= 0) {
		canattack = true;
	}
	return canattack;
}
bool Board::checkAttackKnight(int a, int b, const int arr[]) {
	bool canattack = false;
	if ((a - 20 - 1 == b || a - 20 + 1 == b || a - 10 - 2 == b || a - 10 + 2 == b || a + 20 - 1 == b || a + 20 + 1 == b || a + 10 - 2 == b || a + 10 + 2 == b) && arr[b] * arr[a] <= 0) {
		canattack = true;
	}
	return canattack;
}
bool Board::checkAttackBishop(int a, int b, const int arr[]) {
	bool canattack = false;
	int checkpos = 0;
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			switch (type) {
			case 0: checkpos = a - 11 * dis;
				break;
			case 1: checkpos = a - 9 * dis;
				break;
			case 2: checkpos = a + 11 * dis;
				break;
			case 3: checkpos = a + 9 * dis;
				break;
			}
			if (arr[checkpos] * arr[a] != 0) {
				if (checkpos == b) {
					canattack = true;
					return canattack;
				}
				goto BISHOPNEXT;
			}
			if (checkpos == b) {
				canattack = true;
				return canattack;
			}
		}
	BISHOPNEXT:;
	}
	return canattack;
}
bool Board::checkAttackRook(int a, int b, const int arr[]) {
	bool canattack = false;
	int checkpos = 0;
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			switch (type) {
			case 0: checkpos = a - 10 * dis;
				break;
			case 1: checkpos = a + 10 * dis;
				break;
			case 2: checkpos = a - dis;
				break;
			case 3: checkpos = a + dis;
				break;
			}
			if (arr[checkpos] * arr[a] != 0) {
				if (checkpos == b) {
					canattack = true;
					return canattack;
				}
				goto ROOKNEXT;
			}
			if (checkpos == b) {
				canattack = true;
				return canattack;
			}
		}
	ROOKNEXT:;
	}
	return canattack;
	
}
bool Board::checkAttackQueen(int a, int b, const int arr[]) {
	bool canattack = false;
	if (checkAttackBishop(a, b, arr)) {
		canattack = true;
		return canattack;
	}
	if (checkAttackRook(a, b, arr)) {
		canattack = true;
		return canattack;
	}
	return canattack;
}
bool Board::checkAttackKing(int a, int b, const int arr[]) {
	bool canattack = false;
	if (a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b) {
		canattack = true;
		return canattack;
	}
	return canattack;
}
std::tuple<int, int> Board::getSmallestAttacker(int square, int color) {
	int sqval;
	std::tuple<int, int> attackerArray[5];
	for (int i = 0; i < 5; i++) {
		std::get<0>(attackerArray[i]) = 0;
		std::get<1>(attackerArray[i]) = 0;
	}

	for (int i = 21; i < 99; i++) {
		sqval = mailbox[i];
		if (sqval == -9 || sqval * color <= 0) {
			continue;
		}
		if (sqval < 0) {
			sqval *= -1;
		}
		switch (sqval) {
		case WP:
			if (checkAttackPawn(i, square, mailbox, mailbox[i])) {
				std::get<0>(attackerArray[0])++;
				std::get<1>(attackerArray[0]) = i;
			}
			break;
		case WN:
			if (checkAttackKnight(i, square, mailbox)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WB:
			if (checkAttackBishop(i, square, mailbox)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WR:
			if (checkAttackRook(i, square, mailbox)) {
				std::get<0>(attackerArray[2])++;
				std::get<1>(attackerArray[2]) = i;
			}
			break;
		case WQ:
			if (checkAttackQueen(i, square, mailbox)) {
				std::get<0>(attackerArray[3])++;
				std::get<1>(attackerArray[3]) = i;
			}
			break;
		case WK:
			if (checkAttackKing(i, square, mailbox)) {
				std::get<0>(attackerArray[4])++;
			}
			break;
		}
	}

	for (int i = 0; i < 5; i++) {
		if (std::get<0>(attackerArray[i]) > 0) {
			std::get<0>(attackerArray[i]) = i + 1;
			return attackerArray[i];
		}
	}
	assert (std::get<0>(attackerArray[0]) == 0);
	return attackerArray[0];
}

/*KING*/
void Board::setKingSquare(int arr[]) {
	for (int k = 98; k > 20; k--) {
		if (arr[k] == WHITE * 6) {
			kingSquareWhite = k;
		}
		if (arr[k] == BLACK * 6) {
			kingSquareBlack = k;
		}
	}
}
//Check whether king is in check
bool Board::checkKing(int color, int arr[]) {
	setKingSquare(arr);
	
	if (color == WHITE) {
		for (int n = 21; n < 99; n++) {
			if (arr[n] * color > 0 || arr[n] == -9) {
				continue;
			}
			else if (checkAttack(n, kingSquareWhite, arr)) {
				return true;
			}
		}
	}
	else {
		for (int n = 21; n < 99; n++) {
			if (arr[n] * color > 0 || arr[n] == -9) {
				continue;
			}
			else if (checkAttack(n, kingSquareBlack, arr)) {
				return true;
			}
		}
	}
	return false;
}
bool Board::checkMoveCheck(int a, int b) {
	move(a, b);
	if (checkKing(turn * -1, mailbox)) {
		undo();
		return true;
	}
	undo();
	return false;
}

/*CASTLING*/
//Check castling rights
std::tuple<bool, bool, bool, bool> Board::checkCastling() {
	std::tuple<bool, bool, bool, bool> castling(true, true, true, true);
	//King is under check
	if (checkKing(WHITE, mailbox)) {
		std::get<0>(castling) = false;
		std::get<1>(castling) = false;
	}
	if (checkKing(BLACK, mailbox)) {
		std::get<2>(castling) = false;
		std::get<3>(castling) = false;
	}
	//Blocked by pieces
	if (mailbox[96] != 0 || mailbox[97] != 0) {
		std::get<0>(castling) = false;
	}
	if (mailbox[92] != 0 || mailbox[93] != 0 || mailbox[94] != 0) {
		std::get<1>(castling) = false;
	}
	if (mailbox[26] != 0 || mailbox[27] != 0) {
		std::get<2>(castling) = false;
	}
	if (mailbox[22] != 0 || mailbox[23] != 0 || mailbox[24] != 0) {
		std::get<3>(castling) = false;
	}
	for (int c = 0; c < moveVec.size(); c++) {
		int oldpos = moveVec[c].x;
		//White king has moved
		if (oldpos == 95) {
			std::get<0>(castling) = false;
			std::get<1>(castling) = false;
		}
		//Black king has moved
		if (oldpos == 25) {
			std::get<2>(castling) = false;
			std::get<3>(castling) = false;
		}
		//White h-file rook has moved
		if (oldpos == 98) {
			std::get<0>(castling) = false;
		}
		//White a-file rook has moved
		if (oldpos == 91) {
			std::get<1>(castling) = false;
		}
		//Black h-file rook has moved
		if (oldpos == 28) {
			std::get<2>(castling) = false;
		}
		//Black a-file rook has moved
		if (oldpos == 21) {
			std::get<3>(castling) = false;
		}
	}
	//Squares are attacked
	for (int n = 21; n < 99; n++) {
		if (mailbox[n] != -9) {
			if (mailbox[n] < 0) {
				if (checkAttack(n, 96, mailbox) || checkAttack(n, 97, mailbox)) {
					std::get<0>(castling) = false;
				}
				if (checkAttack(n, 92, mailbox) || checkAttack(n, 93, mailbox) || checkAttack(n, 94, mailbox)) {
					std::get<1>(castling) = false;
				}
			}
			else {
				if (checkAttack(n, 26, mailbox) || checkAttack(n, 27, mailbox)) {
					std::get<2>(castling) = false;
				}
				if (checkAttack(n, 22, mailbox) || checkAttack(n, 23, mailbox) || checkAttack(n, 24, mailbox)) {
					std::get<3>(castling) = false;
				}
			}
		}
	}
	return castling;
}

/*MOVES*/
//Defines special behavior for en passant, castling and promotion
void Board::specialMoves(int oldpos, int newpos, int last, int arr[]) {
	//White en passant
	if ((newpos - oldpos == -11 || newpos - oldpos == -9) && arr[newpos] == WP && !moveVec.empty()) {
		if (moveVec[last - 1].y == newpos + 10 && arr[newpos + 10] == BP && (moveVec[last - 1].x == oldpos + 1 || moveVec[last - 1].x == oldpos - 1)) {
			arr[newpos + 10] = 0;
		}
	}
	//Black en passant
	else if ((newpos - oldpos == 11 || newpos - oldpos == 9) && arr[newpos] == BP && !moveVec.empty()) {
		if (moveVec[last - 1].y == newpos - 10 && arr[newpos - 10] == WP && (moveVec[last - 1].x == oldpos + 1 || moveVec[last - 1].x == oldpos - 1)) {
			arr[newpos - 10] = 0;
		}
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(arr[newpos]) == WK && abs(arr[newpos + 1]) == WR) {
		arr[newpos + 1] = 0;
		//White
		if (arr[newpos] > 0) {
			arr[oldpos + 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			arr[oldpos + 1] = BR;
			castled[1] = true;
		}
	}
	//Long castling
	else if (oldpos - newpos == 2 && abs(arr[newpos]) == WK && abs(arr[newpos - 1]) == WR) {
		arr[newpos - 2] = 0;
		//White
		if (arr[newpos] > 0) {
			arr[oldpos - 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			arr[oldpos - 1] = BR;
			castled[1] = true;
		}
	}
	//White promotion (to queen)
	if (arr[newpos] == WP && newpos >= 21 && newpos <= 28) {
		arr[newpos] = WQ;
	}
	else if (arr[newpos] == BP && newpos >= 91 && newpos <= 98) {
		arr[newpos] = BQ;
	}
}
//Special move behaviour without moveVec
void Board::tempSpecialMoves(int oldpos, int newpos, int lastoldpos, int lastnewpos, int arr[]) {
	//White en passant
	if ((newpos - oldpos == -11 || newpos - oldpos == -9) && arr[newpos] == WP) {
		if (lastnewpos == newpos + 10 && arr[newpos + 10] == BP && (lastoldpos == oldpos + 1 || lastoldpos == oldpos - 1)) {
			arr[newpos + 10] = 0;
		}
	}
	//Black en passant
	else if ((newpos - oldpos == 11 || newpos - oldpos == 9) && arr[newpos] == BP) {
		if (lastnewpos == newpos - 10 && arr[newpos - 10] == WP && (lastoldpos == oldpos + 1 || lastoldpos == oldpos - 1)) {
			arr[newpos - 10] = 0;
		}
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(arr[newpos]) == WK && abs(arr[newpos + 1]) == WR) {
		arr[newpos + 1] = 0;
		//White
		if (arr[newpos] > 0) {
			arr[oldpos + 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			arr[oldpos + 1] = BR;
			castled[1] = true;
		}
	}
	//Long castling
	else if (oldpos - newpos == 2 && abs(arr[newpos]) == WK && abs(arr[newpos - 1]) == WR) {
		arr[newpos - 2] = 0;
		//White
		if (arr[newpos] > 0) {
			arr[oldpos - 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			arr[oldpos - 1] = BR;
			castled[1] = true;
		}
	}
	//White promotion (to queen)
	if (arr[newpos] == WP && newpos >= 21 && newpos <= 28) {
		arr[newpos] = WQ;
	}
	else if (arr[newpos] == BP && newpos >= 91 && newpos <= 98) {
		arr[newpos] = BQ;
	}
}
//moveVec move
void Board::move(int a, int b) {
	mailbox[b] = mailbox[a];
	mailbox[a] = 0;
	//Update move vec
	moveVec.emplace_back(sf::Vector2i(a, b));

	//Special moves
	specialMoves(a, b, moveVec.size() - 1, mailbox);

	//Check for turn
	if (moveVec.size() % 2 == 0) {
		turn = WHITE;
	}
	else {
		turn = BLACK;
	}
}
//Temp array move
void Board::move(int a, int b, int arr[]) {
	arr[b] = arr[a];
	arr[a] = 0;

	//Special moves
	if (!moveVec.empty()) {
		tempSpecialMoves(a, b, moveVec.back().x, moveVec.back().y, arr);
	}

}
void Board::undo() {
	if (!moveVec.empty()) {
		moveVec.pop_back();
	}
	setPosition();
}
void Board::setEnPassantSquare() {
	if (moveVec.empty()) {
		epSquare = -1;
		return;
	}

	int oldsq = moveVec.back().x;
	int newsq = moveVec.back().y;
	int color = mailbox[newsq];

	if (mailbox[newsq] != WP || mailbox[newsq] != BP) {
		epSquare = -1;
		return;
	}
	if (abs(oldsq - newsq) != 20) {
		epSquare = -1;
		return;
	}
	if (mailbox[newsq + 1] != WP * -color && mailbox[newsq - 1] != WP * -color) {
		epSquare = -1;
		return;
	}
	if (mailbox[newsq + color * 10] != 0) {
		epSquare = -1;
		return;
	}
	epSquare = newsq + color * 10;
}
void Board::getCaptureMoves() {
	//Clear capture move vector
	if (!captureVec.empty()) {
		captureVec.clear();
	}
	//Get all capture moves
	for (int m = 21; m < 99; m++) {
		if (mailbox[m] == -9) {
			continue;
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == -9 || mailbox[n] * mailbox[m] >= 0) {
					continue;
				}
				else {
					if (checkLegal(m, n)) {
						captureVec.emplace_back(sf::Vector2i(m, n));
					}
				}
			}
		}
	}
}

/*BOARD*/
void Board::outputBoard() {
	std::cout << std::endl;
	for (int i = 0; i < 120; i++) {
		if (mailbox[i] >= 0) {
			std::cout << " ";
		}
		std::cout << mailbox[i] << " ";
		if ((i + 1) % 10 == 0) {
			std::cout << std::endl;
		}
	}
	std::cout << "Turn : " << turn << std::endl;
}
void Board::resetBoard() {
	for (int i = 21; i < 99; i++) {
		mailbox[i] = start[i];
	}
	castled[0] = false;
	castled[1] = false;
	moveVec.clear();
}

/*SETTERS*/
//Set board position (from position vector)
void Board::setPosition(std::vector<std::string> pV) {
	//Reset board
	resetBoard();
	for (int i = 0; i < pV.size(); i++) {
		int oldpos = convertCoord(toCoord(pV[i][0], pV[i][1]));
		int newpos = convertCoord(toCoord(pV[i][2], pV[i][3]));
		move(oldpos, newpos);

		if (i > 0) {
			specialMoves(moveVec[i].x, moveVec[i].y, i, mailbox);
		}
	}

	if (pV.size() % 2 == 0) {
		turn = WHITE;
	}
	else {
		turn = BLACK;
	}
}
//Set board position (from internal move vector)
void Board::setPosition() {
	for (int i = 21; i < 99; i++) {
		mailbox[i] = start[i];
	}
	//Reset castled array
	castled[0] = false;
	castled[1] = false;
	int oldc, newc;
	int size = moveVec.size();

	for (int i = 0; i < size; i++) {
		oldc = moveVec[i].x;
		newc = moveVec[i].y;
		mailbox[newc] = mailbox[oldc];
		mailbox[oldc] = 0;
		if (i > 0) {
			specialMoves(oldc, newc, i, mailbox);
		}
	}

	if (size % 2 == 0) {
		turn = WHITE;
	}
	else {
		turn = BLACK;
	}
}

/*GETTERS*/
int Board::getTurn() { return turn; }
int Board::getSquarePiece(int a) { return mailbox[a]; }