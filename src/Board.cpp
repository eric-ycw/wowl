#include "Board.h"

void Board::parseFEN(std::string fen) {
	if (fen.empty()) { return; }

	//Completely empty the board
	resetBoard(true);

	int start = 21;
	int piece = 0;

	//The FEN string starts on the eight rank but startinng the counter at zero is more convenient
	int rank = 0;  
	int file = 0;
	int strip_pos = 0;

	for (int i = 0; i < fen.length(); ++i) {
		int empty = 1;
		if (rank == 8) { 
			strip_pos = i;
			break; 
		}

		switch (fen[i]) {
			case 'P': piece = WP; break;
			case 'N': piece = WN; break;
			case 'B': piece = WB; break;
			case 'R': piece = WR; break;
			case 'Q': piece = WQ; break;
			case 'K': piece = WK; break;
			case 'p': piece = BP; break;
			case 'n': piece = BN; break;
			case 'b': piece = BB; break;
			case 'r': piece = BR; break;
			case 'q': piece = BQ; break;
			case 'k': piece = BK; break;

			case '1': [[fallthrough]];
			case '2': [[fallthrough]];
			case '3': [[fallthrough]];
			case '4': [[fallthrough]];
			case '5': [[fallthrough]];
			case '6': [[fallthrough]];
			case '7': [[fallthrough]];
			case '8': 
				piece = OOB;
				empty = fen[i] - '0';
				break;

			case ' ': [[fallthrough]];
			case '/':
				rank++;
				file = 0;
				continue;

			default:
				std::cout << "Invalid FEN" << std::endl;
				break;
		}

		for (int i = 0; i < empty; ++i) {
			if (piece != OOB) {
				int mailbox_coord = mailbox64[rank][file];
				assert(mailbox_coord >= 21 && mailbox_coord <= 98);
				mailbox[mailbox_coord] = piece;
			}
			file++;
		}
	}

	assert(strip_pos > 0);
	assert(fen[strip_pos] == 'w' || fen[strip_pos] == 'b');
	fen.erase(0, strip_pos);
	turn = (fen[0] == 'w') ? WHITE : BLACK;
	//0123456
	//w - - 
	strip_pos = 2;
	for (int i = 2; i < 8; ++i) {
		strip_pos++;
		if (fen[i] == ' ') { break; }
		switch (fen[i]) {
			case 'K': castling[0] = 1; break;
			case 'Q': castling[1] = 1; break;
			case 'k': castling[2] = 1; break;
			case 'q': castling[3] = 1; break;
			default: break;
		}
	}

	assert(strip_pos > 2);
	fen.erase(0, strip_pos);
	if (fen[0] != '-') {
		file = fen[0] - 'a';
		rank = fen[1] = '8' - fen[1];
		assert(file >= 0 && file < 8);
		assert(rank >= 0 && rank < 8);
		epSquare = mailbox64[rank][file];
	}
	else {
		epSquare = -1;
	}
	outputBoard();
}

int Board::toCoord(char a, char b) {
	int x = int(a) - 97;
	int y = 7 - int(b) + 49;
	int pos = mailbox64[y][x];
	return pos;
}
//Converts mailbox coord to 64 coord
int Board::to64Coord(int square) const {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = square - 21 - 2 * (tenth - 2);
	return rval;
}
std::string Board::toNotation(int square) const {
	std::string s = "  ";
	int coord = to64Coord(square);
	int x = coord % 8;
	int y = floor(coord / 8);
	s[0] = char(x + 97);
	s[1] = char(7 - y + 49);
	return s;
}

void Board::reserveVectors() {
	moveVec.reserve(150);
	legalMoveVec.reserve(50);
	captureVec.reserve(30);
}

//Check if move is pseudolegal
bool Board::checkLegal(int a, int b) {
	if (a < 0 || b < 0 || a > 119 || b > 119) { return false; }

	int oldSquareVal = mailbox[a];
	int newSquareVal = mailbox[b];

	//Turn + own piece
	if (oldSquareVal * turn <= 0 || newSquareVal * turn > 0) {
		return false;
	}

	//Out of bounds
	if (newSquareVal == OOB) {
		return false;
	}

	//Destination cannot be king
	if (abs(newSquareVal) == 6) {
		return false;
	}

	int sval;
	if (oldSquareVal < 0) {
		sval = oldSquareVal * -1;
	}
	else {
		sval = oldSquareVal;
	}
	switch (sval) {
	case WP:
		setEnPassantSquare();
		return checkLegalPawn(a, b, oldSquareVal);
		break;
	case WN:
		return checkAttackKnight(a, b);
		break;
	case WB:
		return checkAttackBishop(a, b);
		break;
	case WR:
		return checkAttackRook(a, b);
		break;
	case WQ:
		return checkAttackQueen(a, b);
		break;
	case WK:
		return checkLegalKing(a, b, turn);
		break;
	}
	return true;
}
bool Board::checkLegalPawn(int a, int b, int color) const {

	//Pawns cannot move backwards
	if (b * color > a) {
		return false;
	}

	int target = mailbox[b];
	//Move forward one square
	if (a - color * 10 == b) {
		if (target == 0) {
			return true;
		}
	}
	//Diagonal capture
	if (a - color * 10 - 1 == b || a - color * 10 + 1 == b) {
		if (target * color < 0) {
			return true;
		}
	}

	if (color == WHITE) {
		//Check for second-rank pawns
		if (a <= 88 && a >= 81) {
			if (a - 20 == b) {
				if (target == 0 && mailbox[b + 10] == 0) {
					return true;
				}
			}
		}
		//En passant
		if (a <= 58 && a >= 51 && target == 0) {
			if ((mailbox[a - 1] == BP && b == a - 10 - 1) || (mailbox[a + 1] == BP && b == a - 10 + 1) && b == epSquare) {
				return true;
			}
		}
	}
	else if (color == BLACK) {
		if (a <= 38 && a >= 31) {
			if (a + 20 == b) {
				if (target == 0 && mailbox[b - 10] == 0) {
					return true;
				}
			}
		}
		if (a <= 68 && a >= 61 && target == 0) {
			if ((mailbox[a - 1] == WP && b == a + 10 - 1) || (mailbox[a + 1] == WP && b == a + 10 + 1) && b == epSquare) {
				return true;
			}
		}
	}

	return false;
}
bool Board::checkLegalKing(int a, int b, int color) {
	if (a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b) {
		return true;
	}
	if (a + 2 == b || a - 2 == b) {
		checkCastling();
	}
	if (color == WHITE) {
		if ((a + 2 == b && castling[0] == 1) || (a - 2 == b && castling[1] == 1)) {
			return true;
		}
	}
	else if (color == BLACK) {
		if ((a + 2 == b && castling[2] == 1) || (a - 2 == b && castling[3] == 1)) {
			return true;
		}
	}
	return false;
}
//Get all pseudolegal moves
void Board::getLegalMoves() {
	legalMoveVec.clear();
	for (int m = 21; m < 99; m++) {
		int piece = mailbox[m];
		int abs_piece = piece * turn;
		int tmp;
		if (piece == OOB || piece == 0 || abs_piece < 0) { continue; }
		if (abs_piece == WP || abs_piece == WN || abs_piece == WK) {
			if (abs_piece == WP) {
				tmp = 0;
			}
			else if (abs_piece == WN) {
				tmp = 1;
			}
			else if (abs_piece == WK) {
				tmp = 2;
			}
			for (int n : pieceMoves[tmp]) {
				if (tmp == 0) { n *= turn; }
				if (n == 0 || mailbox[m + n] == OOB) {
					continue;
				}
				if (checkLegal(m, m + n)) {
					legalMoveVec.emplace_back(Move(m, m + n));
				}
			}
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == OOB) {
					continue;
				}
				if (abs_piece == WB) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0) { continue; }
				}
				else if (abs_piece == WR) {
					if ((m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) { continue; }
				}
				else if (abs_piece == WQ) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0 && (m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) {
						continue; 
					}
				}
				if (checkLegal(m, n)) {
					legalMoveVec.emplace_back(Move(m, n));
				}
			}
		}
	}
}
void Board::getCaptures() {
	captureVec.clear();
	for (int m = 21; m < 99; m++) {
		int piece = mailbox[m];
		int abs_piece = piece * turn;
		int tmp;
		if (piece == OOB || piece == 0 || abs_piece < 0) { continue; }
		if (abs_piece == WP || abs_piece == WN || abs_piece == WK) {
			if (abs_piece == WP) {
				tmp = 0;
			}
			else if (abs_piece == WN) {
				tmp = 1;
			}
			else if (abs_piece == WK) {
				tmp = 2;
			}
			for (int n : pieceMoves[tmp]) {
				if (tmp == 0) { n *= turn; }
				if (n == 0 || mailbox[m + n] == OOB || mailbox[m + n] * piece >= 0) {
					continue;
				}
				if (abs_piece == WP && (n == -10 || n == -20)) { continue; }  //Pawn forward moves can't be captures
				if (checkLegal(m, m + n)) {
					captureVec.emplace_back(Move(m, m + n));
				}
			}
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == OOB || mailbox[n] * piece >= 0) {
					continue;
				}
				if (abs_piece == WB) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0) { continue; }
				}
				else if (abs_piece == WR) {
					if ((m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) { continue; }
				}
				else if (abs_piece == WQ) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0 && (m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) {
						continue;
					}
				}
				if (checkLegal(m, n)) {
					captureVec.emplace_back(Move(m, n));
				}
			}
		}
	}
}

bool Board::checkAttack(int a, int b) const {
	if (a < 0 || b < 0 || a > 119 || b > 119) { return false; }

	int oldSquareVal = mailbox[a];
	int newSquareVal = mailbox[b];

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
			return checkAttackPawn(a, b, WHITE);
		}
		else {
			return checkAttackPawn(a, b, BLACK);
		}
		break;

	case WN:
		return checkAttackKnight(a, b);
		break;

	case WB:
		return checkAttackBishop(a, b);
		break;

	case WR:
		return checkAttackRook(a, b);
		break;

	case WQ:
		return checkAttackQueen(a, b);
		break;

	case WK:
		return checkAttackKing(a, b);
		break;

	}
}
bool Board::checkAttackPawn(int a, int b, int color) const {
	if ((a - color * 11 == b || a - color * 9 == b) && mailbox[b] * mailbox[a] <= 0) {
		return true;
	}
	return false;
}
bool Board::checkAttackKnight(int a, int b) const {
	if ((a - 20 - 1 == b || a - 20 + 1 == b || a - 10 - 2 == b || a - 10 + 2 == b || a + 20 - 1 == b || a + 20 + 1 == b || a + 10 - 2 == b || a + 10 + 2 == b) && mailbox[b] * mailbox[a] <= 0) {
		return true;
	}
	return false;
}
bool Board::checkAttackBishop(int a, int b) const {
	if ((a - b) % 9 != 0 && (a - b) % 11 != 0) { return false; }
	int checkpos = 0;
	int move_dirs[4] = { 11, 9, -11, -9 };
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			checkpos = a + move_dirs[type] * dis;
			if (mailbox[checkpos] * mailbox[a] != 0) {
				if (checkpos == b) {
					return true;
				}
				break;
			}
			if (checkpos == b) {
				return true;
			}
		}
	}
	return false;
}
bool Board::checkAttackRook(int a, int b) const {
	if ((a - b) % 10 != 0 && (a - b < - 7 || a - b > 7)) { return false; }
	int checkpos = 0;
	int move_dirs[4] = { 1, 10, -1, -10 };
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			checkpos = a + move_dirs[type] * dis;
			if (mailbox[checkpos] * mailbox[a] != 0) {
				if (checkpos == b) { return true; }
				break;
			}
			if (checkpos == b) { return true; }
		}
	}
	return false;
}
bool Board::checkAttackQueen(int a, int b) const {
	if (checkAttackBishop(a, b) || checkAttackRook(a, b)) {
		return true;
	}
	return false;
}
bool Board::checkAttackKing(int a, int b) const {
	if (a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b) {
		return true;
	}
	return false;
}
std::tuple<int, int> Board::getSmallestAttacker(int square, int color) {

	std::tuple<int, int> attackerArray[5];
	for (int i = 0; i < 5; ++i) {
		std::get<0>(attackerArray[i]) = 0;
		std::get<1>(attackerArray[i]) = 0;
	}

	int sqval;
	if (mailbox[square] * color >= 0) {
		assert(std::get<0>(attackerArray[0]) == 0);
		assert(std::get<1>(attackerArray[0]) == 0);
		return attackerArray[0];
	}

	for (int i = 21; i < 99; ++i) {
		sqval = mailbox[i];
		if (sqval == OOB || sqval * color <= 0) {
			continue;
		}
		if (sqval < 0) {
			sqval *= -1;
		}
		switch (sqval) {
		case WP:
			if (checkAttackPawn(i, square, mailbox[i])) {
				std::get<0>(attackerArray[0])++;
				std::get<1>(attackerArray[0]) = i;
			}
			break;
		case WN:
			if (checkAttackKnight(i, square)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WB:
			if (checkAttackBishop(i, square)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WR:
			if (checkAttackRook(i, square)) {
				std::get<0>(attackerArray[2])++;
				std::get<1>(attackerArray[2]) = i;
			}
			break;
		case WQ:
			if (checkAttackQueen(i, square)) {
				std::get<0>(attackerArray[3])++;
				std::get<1>(attackerArray[3]) = i;
			}
			break;
		case WK:
			if (checkAttackKing(i, square)) {
				std::get<0>(attackerArray[4])++;
			}
			break;
		}
	}

	for (int i = 0; i < 5; ++i) {
		if (std::get<0>(attackerArray[i]) > 0) {
			std::get<0>(attackerArray[i]) = i + 1;
			return attackerArray[i];
		}
	}
	assert(std::get<0>(attackerArray[0]) == 0);
	assert(std::get<1>(attackerArray[0]) == 0);
	return attackerArray[0];
}

void Board::setKingSquare() {
	for (int k = 21; k < 99; ++k) {
		if (mailbox[k] == WHITE * 6) {
			kingSquareWhite = k;
		}
		if (mailbox[k] == BLACK * 6) {
			kingSquareBlack = k;
		}
	}
}
bool Board::inCheck(int color) {
	setKingSquare();	
	int kingsqr = (color == WHITE) ? kingSquareWhite : kingSquareBlack;
	for (int n = 21; n < 99; n++) {
		if (n % 10 == 0 || n % 10 == 9 || mailbox[n] * color > 0) {
			continue;
		}
		if (checkAttack(n, kingsqr)) {
			return true;
		}
	}
	return false;
}
bool Board::checkMoveCheck(int a, int b) {
	int mailbox_copy[120];
	int castling_copy[4];
	memcpy(mailbox_copy, mailbox, sizeof(mailbox));
	memcpy(castling_copy, castling, sizeof(castling));
	move(a, b);
	if (inCheck(turn * -1)) {
		undo(mailbox_copy, castling_copy);
		return true;
	}
	undo(mailbox_copy, castling_copy);
	return false;
}

//Check castling rights
void Board::checkCastling() {
	//No one can castle
	if (!castling[0] && !castling[1] && !castling[2] && !castling[3]) { return; }

	//Temporarily reset castling permissions if castling rights were not forfeited
	for (int i = 0; i < 4; i++) {
		if (castling[i] == -1) {
			castling[i] = 1;
		}
	}

	//Blocked by pieces
	if (castling[0] == 1 && (mailbox[96] != 0 || mailbox[97] != 0)) {
		castling[0] = -1;
	}
	if (castling[1] == 1 && (mailbox[92] != 0 || mailbox[93] != 0 || mailbox[94] != 0)) {
		castling[1] = -1;
	}
	if (castling[2] == 1 && (mailbox[26] != 0 || mailbox[27] != 0)) {
		castling[2] = -1;
	}
	if (castling[3] == 1 && (mailbox[22] != 0 || mailbox[23] != 0 || mailbox[24] != 0)) {
		castling[3] = -1;
	}
	for (int c = 0; c < moveVec.size(); ++c) {
		int oldpos = moveVec[c].from;
		//White king has moved
		if (oldpos == 95 && (castling[0] != 0 || castling[1] != 0)) {
			castling[0] = 0;
			castling[1] = 0;;
		}
		//Black king has moved
		if (oldpos == 25 && (castling[2] != 0 || castling[3] != 0)) {
			castling[2] = 0;
			castling[3] = 0;;
		}
		//White h-file rook has moved
		if (oldpos == 98 && castling[0] != 0) {
			castling[0] = 0;
		}
		//White a-file rook has moved
		if (oldpos == 91 && castling[1] != 0) {
			castling[1] = 0;
		}
		//Black h-file rook has moved
		if (oldpos == 28 && castling[2] != 0) {
			castling[2] = 0;
		}
		//Black a-file rook has moved
		if (oldpos == 21 && castling[3] != 0) {
			castling[3] = 0;
		}
	}
	//King is under check
	if ((castling[0] == 1 || castling[1] == 1) && inCheck(WHITE)) {
		castling[0] = -1;
		castling[1] = -1;
	}
	if ((castling[2] == 1 || castling[3] == 1) && inCheck(BLACK)) {
		castling[2] = -1;
		castling[3] = -1;
	}
	//Squares are attacked
	for (int n = 21; n < 99; n++) {
		if (mailbox[n] != OOB || mailbox[n] == 0) {
			if (mailbox[n] < 0) {
				if (castling[0] == 1 && (checkAttack(n, 96) || checkAttack(n, 97))) {
					castling[0] = -1;
				}
				if (castling[1] == 1 && (checkAttack(n, 93) || checkAttack(n, 94))) {
					castling[1] = -1;
				}
			}
			else {
				if (castling[2] == 1 && (checkAttack(n, 26) || checkAttack(n, 27))) {
					castling[2] = -1;
				}
				if (castling[3] == 1 && (checkAttack(n, 23) || checkAttack(n, 24))) {
					castling[3] = -1;
				}
			}
		}
	}
}

//Defines special behavior for en passant, castling and promotion
void Board::specialMoves(int oldpos, int newpos) {
	//While en passant
	if (mailbox[newpos] == WP && newpos == epSquare) {
		mailbox[newpos + 10] = 0;
	}
	//Black en passant
	else if (mailbox[newpos] == BP && newpos == epSquare) {
		mailbox[newpos - 10] = 0;
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(mailbox[newpos]) == WK && abs(mailbox[newpos + 1]) == WR) {
		mailbox[newpos + 1] = 0;
		//White
		if (mailbox[newpos] > 0) {
			mailbox[oldpos + 1] = WR;
		}
		//Black
		else {
			mailbox[oldpos + 1] = BR;
		}
	}
	//Long castling
	else if (oldpos - newpos == 2 && abs(mailbox[newpos]) == WK && abs(mailbox[newpos - 2]) == WR) {
		mailbox[newpos - 2] = 0;
		//White
		if (mailbox[newpos] > 0) {
			mailbox[oldpos - 1] = WR;
		}
		//Black
		else {
			mailbox[oldpos - 1] = BR;
		}
	}
	//White promotion (to queen)
	if (mailbox[newpos] == WP && newpos >= 21 && newpos <= 28) {
		mailbox[newpos] = WQ;
	}
	else if (mailbox[newpos] == BP && newpos >= 91 && newpos <= 98) {
		mailbox[newpos] = BQ;
	}
}
void Board::move(int a, int b) {
	assert(a >= 0 && a < 120);
	assert(b >= 0 && b < 120);
	setEnPassantSquare();
	mailbox[b] = mailbox[a];
	mailbox[a] = 0;
	moveVec.emplace_back(Move(a, b));
	specialMoves(a, b);

	turn *= -1;
}
void Board::undo(int mailbox_copy[], int castling_copy[]) {
	memcpy(mailbox, mailbox_copy, sizeof(mailbox));
	memcpy(castling, castling_copy, sizeof(castling));
	if (!moveVec.empty()) {
		moveVec.pop_back();
	}
	turn *= -1;
}
void Board::nullMove() {
	moveVec.emplace_back(Move(0, 0));
	turn *= -1;
}
void Board::undoNullMove() {
	if (!moveVec.empty()) {
		moveVec.pop_back();
	}
	turn *= -1;
}
void Board::setEnPassantSquare() {
	if (moveVec.empty()) { return; }

	int oldsq = moveVec.back().from;
	int newsq = moveVec.back().to;
	int color = mailbox[newsq];

	if (mailbox[newsq] != WP && mailbox[newsq] != BP) {
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
	epSquare = newsq + color * 10;
}

void Board::outputBoard() const {
	std::cout << std::endl;
	for (int i = 0; i < 120; ++i) {
		switch (mailbox[i]) {
		case 0:
			std::cout << "* ";
			break;
		case WP:
			std::cout << "P ";
			break;
		case WN:
			std::cout << "N ";
			break;
		case WB:
			std::cout << "B ";
			break;
		case WR:
			std::cout << "R ";
			break;
		case WQ:
			std::cout << "Q ";
			break;
		case WK:
			std::cout << "K ";
			break;
		case BP:
			std::cout << "p ";
			break;
		case BN:
			std::cout << "n ";
			break;
		case BB:
			std::cout << "b ";
			break;
		case BR:
			std::cout << "r ";
			break;
		case BQ:
			std::cout << "q ";
			break;
		case BK:
			std::cout << "k ";
			break;
		}
		if ((i + 1) % 10 == 0) {
			std::cout << std::endl;
		}
	}
}
//Reset board to starting position or empty board
void Board::resetBoard(bool empty) {
	for (int i = 21; i < 99; ++i) {
		if (mailbox[i] == OOB) { continue; }
		mailbox[i] = (empty) ? 0 : start[i];
	}
	for (int i = 0; i < 4; ++i) {
		castling[i] = !empty;
	}
	epSquare = -1;
	moveVec.clear();
}

//Set board position from move vector
void Board::setPosition() {
	for (int i = 21; i < 99; ++i) {
		mailbox[i] = start[i];
	}

	int oldc, newc;
	int size = moveVec.size();

	for (int i = 0; i < size; ++i) {
		oldc = moveVec[i].from;
		newc = moveVec[i].to;
		mailbox[newc] = mailbox[oldc];
		mailbox[oldc] = 0;
		if (i > 0) {
			specialMoves(oldc, newc);
		}
	}
	turn = (size % 2 == 0) ? WHITE : BLACK;
}

int Board::getTurn() const { return turn; }
int Board::getSquarePiece(int a) const { return mailbox[a]; }