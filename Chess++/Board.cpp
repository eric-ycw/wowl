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
//Converts coord vector (without SIZE factor) to notation
std::string Board::toNotation(sf::Vector2f vec) {
	std::string s = "  ";
	s[0] = char(vec.x + 97);
	s[1] = char(7 - vec.y + 49);
	return s;
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
//Check move legality
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

	//Check if move puts king in check
	if (checkMoveCheck(a, b)) {
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
					if (moveVec.at(moveVec.size() - 1).x == a - 20 - 1 && moveVec.at(moveVec.size() - 1).y == a - 1) {
						legal = true;
						return legal;
					}
				}
				if (mailbox[a + 1] == -1 && newSquareVal == 0 && b == a - 10 + 1) {
					if (moveVec.at(moveVec.size() - 1).x == a - 20 + 1 && moveVec.at(moveVec.size() - 1).y == a + 1) {
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
					if (moveVec.at(moveVec.size() - 1).x == a + 20 - 1 && moveVec.at(moveVec.size() - 1).y == a - 1) {
						legal = true;
						return legal;
					}
				}
				if (mailbox[a + 1] == 1 && newSquareVal == 0 && b == a + 10 + 1) {
					if (moveVec.at(moveVec.size() - 1).x == a + 20 + 1 && moveVec.at(moveVec.size() - 1).y == a + 1) {
						legal = true;
						return legal;
					}
				}
			}
		}
	}
		break;
	case WN:
	{
		legal = false;
		if (a - 20 - 1 == b || a - 20 + 1 == b || a - 10 - 2 == b || a - 10 + 2 == b || a + 20 - 1 == b || a + 20 + 1 == b || a + 10 - 2 == b || a + 10 + 2 == b) {
			legal = true;
			return legal;
		}
	}
		break;
	case WB:
	{
		legal = false;
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
				//Blocked by piece
				if (mailbox[checkpos] * turn != 0) {
					//Check final time
					if (checkpos == b) {
						legal = true;
						return legal;
					}
					goto BISHOPNEXT;
				}
				if (checkpos == b) {
					legal = true;
					return legal;
				}
			}
		BISHOPNEXT:;
		}
	}
		break;
	case WR:
	{
		legal = false;
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
				if (mailbox[checkpos] * turn != 0) {
					if (checkpos == b) {
						legal = true;
						return legal;
					}
					goto ROOKNEXT;
				}
				if (checkpos == b) {
					legal = true;
					return legal;
				}
			}
		ROOKNEXT:;
		}
	}
		break;
	case WQ:
	{
		legal = false;
		int checkpos = 0;
		for (int type = 0; type < 8; type++) {
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
				case 4: checkpos = a - 11 * dis;
					break;
				case 5: checkpos = a - 9 * dis;
					break;
				case 6: checkpos = a + 11 * dis;
					break;
				case 7: checkpos = a + 9 * dis;
					break;
				}
				if (mailbox[checkpos] * turn != 0) {
					if (checkpos == b) {
						legal = true;
						return legal;
					}
					goto QUEENNEXT;
				}
				if (checkpos == b) {
					legal = true;
					return legal;
				}
			}
		QUEENNEXT:;
		}
	}
		break;
	case WK:
	{
		legal = true;
		//Eliminate impossible moves
		if (!(a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b || a + 2 == b || a - 2 == b)) {
			legal = false;
			return legal;
		}
		if (checkMoveCheck(a, b)) {
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
//Get all legal moves for current turn
void Board::getLegalMoves() {
	//Clear legal move vector
	if (legalMoveVec.size() > 0) {
		legalMoveVec.clear();
	}
	//Get all legal moves
	for (int m = 21; m < 99; m++) {
		for (int n = 21; n < 99; n++) {
			if (mailbox[m] * turn > 0 && mailbox[n] * turn <= 0 && mailbox[m] != -9) {
				if (checkLegal(m, n)) {
					legalMoveVec.emplace_back(sf::Vector2i(m, n));
				}

			}
		}
	}
}

/*ATTACKS*/
//Check if square can be attacked
bool Board::checkAttack(int a, int b, int arr[]) {
	bool canattack = true;

	int oldSquareVal = arr[a];
	int newSquareVal = arr[b];

	//Out of bounds
	if (newSquareVal == -9) {
		canattack = false;
		return canattack;
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
		canattack = false;
		if (oldSquareVal == 1) {
			if (b > a) {
				return canattack;
			}
			if (a - 10 - 1 == b || a - 10 + 1 == b) {
				if (newSquareVal <= 0) {
					canattack = true;
				}
			}
			return canattack;
		}
		else {
			if (b < a) {
				return canattack;
			}
			if (a + 10 - 1 == b || a + 10 + 1 == b) {
				if (newSquareVal >= 0) {
					canattack = true;
				}
			}
			return canattack;
		}
	}
		break;

	case WN:
	{
		canattack = false;
		if (a - 20 - 1 == b || a - 20 + 1 == b || a - 10 - 2 == b || a - 10 + 2 == b || a + 20 - 1 == b || a + 20 + 1 == b || a + 10 - 2 == b || a + 10 + 2 == b) {
			canattack = true;
		}
		return canattack;
	}
		break;

	case WB:
	{
		canattack = false;
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
				if (arr[checkpos] * turn != 0) {
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
		break;

	case WR:
	{
		canattack = false;
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
				if (arr[checkpos] * turn != 0) {
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
	break;

	case WQ:
	{
		canattack = false;
		int checkpos = 0;
		for (int type = 0; type < 8; type++) {
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
				case 4: checkpos = a - 11 * dis;
					break;
				case 5: checkpos = a - 9 * dis;
					break;
				case 6: checkpos = a + 11 * dis;
					break;
				case 7: checkpos = a + 9 * dis;
					break;
				}
				if (arr[checkpos] * turn != 0) {
					if (checkpos == b) {
						canattack = true;
						return canattack;
					}
					goto QUEENNEXT;
				}
				if (checkpos == b) {
					canattack = true;
					return canattack;
				}
			}
		QUEENNEXT:;
		}
		return canattack;
	}
		break;

	case WK:
	{
		canattack = true;
		//Eliminate impossible moves
		if (!(a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b)) {
			canattack = false;
			return canattack;
		}
		return canattack;
	}
		break;

	}
}

/*CHECK*/
//Check whether king is in check
bool Board::checkKing(int color, int arr[]) {
	int kingsquare;
	if (color == WHITE) {
		for (int m = 98; m > 20; m--) {
			if (arr[m] == WHITE * 6) {
				kingsquare = m;
				break;
			}
		}
	}
	else {
		for (int m = 21; m < 99; m++) {
			if (arr[m] == BLACK * 6) {
				kingsquare = m;
				break;
			}
		}
	}
	for (int n = 21; n < 99; n++) {
		if (arr[n] * color > 0 || arr[n] == -9) {
			continue;
		}
		else if (checkAttack(n, kingsquare, arr)) {
			return true;
		}
	}
	return false;
}
//Check whether a move would put own king in check
bool Board::checkMoveCheck(int a, int b) {
	for (int i = 21; i < 99; i++) {
		temp[i] = mailbox[i];
	}
	move(a, b, temp);
	if (checkKing(turn, temp)) {
		return true;
	}
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
		//White king has moved
		if (moveVec[c].x == 95) {
			std::get<0>(castling) = false;
			std::get<1>(castling) = false;
		}
		//Black king has moved
		if (moveVec[c].x == 25) {
			std::get<2>(castling) = false;
			std::get<3>(castling) = false;
		}
		//White h-file rook has moved
		if (moveVec[c].x == 98) {
			std::get<0>(castling) = false;
		}
		//White a-file rook has moved
		if (moveVec[c].x == 91) {
			std::get<1>(castling) = false;
		}
		//Black h-file rook has moved
		if (moveVec[c].x == 28) {
			std::get<2>(castling) = false;
		}
		//Black a-file rook has moved
		if (moveVec[c].x == 21) {
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
	if ((newpos - oldpos == -11 || newpos - oldpos == -9) && arr[newpos] == WP && moveVec.size() > 1) {
		if (moveVec[last - 1].y == newpos + 10) {
			arr[newpos + 10] = 0;
		}
	}
	//Black en passant
	else if ((newpos - oldpos == 11 || newpos - oldpos == 9) && arr[newpos] == BP && moveVec.size() > 1) {
		if (moveVec[last - 1].y == newpos - 10) {
			arr[newpos - 10] = 0;
		}
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(arr[newpos]) == WK) {
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
	else if (oldpos - newpos == 2 && abs(arr[newpos]) == WK) {
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
		if (lastnewpos == newpos + 10) {
			arr[newpos + 10] = 0;
		}
	}
	//Black en passant
	else if ((newpos - oldpos == 11 || newpos - oldpos == 9) && arr[newpos] == BP) {
		if (lastnewpos == newpos - 10) {
			arr[newpos - 10] = 0;
		}
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(arr[newpos]) == WK) {
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
	else if (oldpos - newpos == 2 && abs(arr[newpos]) == WK) {
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
	int size = moveVec.size();
	if (size > 1) {
		tempSpecialMoves(a, b, moveVec[size - 1].x, moveVec[size - 1].y, arr);
	}
}
void Board::undo() {
	if (moveVec.size() > 0) {
		moveVec.pop_back();
	}
	setPosition();
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
	//Reset mailbox
	for (int i = 21; i < 99; i++) {
		mailbox[i] = start[i];
	}
	//Reset castled array
	castled[0] = false;
	castled[1] = false;
	//Reset move vec
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

		//Special moves
		if (i > 0) {
			specialMoves(moveVec[i].x, moveVec[i].y, i, mailbox);
		}
	}

	//Check for turn
	if (pV.size() % 2 == 0) {
		turn = WHITE;
	}
	else {
		turn = BLACK;
	}
}
//Set board position (from internal move vector)
void Board::setPosition() {
	//Reset board without clearing movevec
	for (int i = 21; i < 99; i++) {
		mailbox[i] = start[i];
	}
	//Reset castled array
	castled[0] = false;
	castled[1] = false;
	int oldc, newc;

	for (int i = 0; i < moveVec.size(); i++) {
		oldc = moveVec[i].x;
		newc = moveVec[i].y;
		mailbox[newc] = mailbox[oldc];
		mailbox[oldc] = 0;
		//Special moves
		if (i > 0) {
			specialMoves(oldc, newc, i, mailbox);
		}
	}
	//Check for turn
	if (moveVec.size() % 2 == 0) {
		turn = WHITE;
	}
	else {
		turn = BLACK;
	}
}

/*GETTERS*/
int Board::getTurn() { return turn; }
int Board::getSquarePiece(int a) { return mailbox[a]; }