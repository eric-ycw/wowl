#include "Board.h"

void Board::parseFEN(std::string fen) {
	if (fen.empty()) { return; }

	//Completely empty the board
	resetBoard(true);

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
				piece = NN;
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
			if (piece != NN) {
				int mailbox_coord = mailbox64[rank][file];
				assert(mailbox_coord >= 21 && mailbox_coord <= 98);
				mailbox[mailbox_coord] = piece;
				if (piece == WK) {
					kingSquare[0] = mailbox_coord;
				}
				else if (piece == BK) {
					kingSquare[1] = mailbox_coord;
				}
			}
			file++;
		}
	}

	assert(strip_pos > 0);
	assert(fen[strip_pos] == 'w' || fen[strip_pos] == 'b');
	fen.erase(0, strip_pos);
	turn = (fen[0] == 'w') ? WHITE : BLACK;

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
}

int Board::toCoord(char a, char b) {
	int x = int(a) - 97;
	int y = 7 - int(b) + 49;
	int pos = mailbox64[y][x];
	return pos;
}
std::string Board::toNotation(int square) const {
	std::string s = "  ";
	int coord = mailbox120[square];
	int x = coord % 8;
	int y = floor(coord / 8);
	s[0] = char(x + 97);
	s[1] = char(7 - y + 49);
	return s;
}

void Board::reserveVectors() {
	moveVec.reserve(150);
}

int Board::checkLegalPawn(int a, int b, int color) const {
	int target = mailbox[b];
	//Move forward one square
	if (a - color * 10 == b && !target) {
		return true;
	}
	//Diagonal capture
	if ((a - color * 11 == b || a - color * 9 == b) && target * color < 0) {
		return true;
	}
	//Second-rank pawns
	if (a - 20 * color == b && a <= 38 + (color == WHITE) * 50 && a >= 31 + (color == WHITE) * 50) {
		if (!target && !mailbox[b + 10 * color]) {
			return true;
		}
	}
	
	//En passant
	if (color == WHITE) {
		if (a <= 58 && a >= 51 && !target) {
			if (((mailbox[a - 1] == BP && b == a - 11) || (mailbox[a + 1] == BP && b == a - 11)) && b == epSquare) {
				return true;
			}
		}
	}
	else if (color == BLACK) {
		if (a <= 68 && a >= 61 && !target) {
			if (((mailbox[a - 1] == WP && b == a + 9) || (mailbox[a + 1] == WP && b == a + 11)) && b == epSquare) {
				return true;
			}
		}
	}
	return false;
}

void Board::genPawnMoves(std::vector<Move>& legalMoves, int square) {
	int fromVal = mailbox[square];
	assert(fromVal == WHITE || fromVal == BLACK);

	for (const int i : pieceMoves[0]) {
		if (!i) { break; }
		int toVal = mailbox[square + i * fromVal];
		if (toVal * turn > 0 || toVal == NN || abs(toVal) == WK) {
			continue;
		}
		if (checkLegalPawn(square, square + i * fromVal, fromVal)) {
			legalMoves.emplace_back(Move(square, square + i * fromVal));
		}
	}
}
void Board::genKnightMoves(std::vector<Move>& legalMoves, int square) {
	for (const int i : pieceMoves[1]) {
		if (!i) { break; }
		int toVal = mailbox[square + i];
		if (toVal * turn > 0 || toVal == NN || abs(toVal) == WK) {
			continue;
		}
		legalMoves.emplace_back(Move(square, square + i));
	}
}
void Board::genSliderMoves(std::vector<Move>& legalMoves, int square, int piece) {
	assert(piece == WB || piece == WR || piece == WQ);
	for (const int i : pieceMoves[piece - 1]) {
		if (!i) { break; }
		for (int j = 1; j < 8; ++j) {
			int to = square + j * i;
			int toVal = mailbox[to];
			if (toVal * turn > 0 || toVal == NN || abs(toVal) == WK) {
				break;
			}
			else {
				legalMoves.emplace_back(Move(square, to));
				if (toVal * turn < 0) { break; }
			}
		}
	}
}
void Board::genKingMoves(std::vector<Move>& legalMoves, int square) {
	for (const int i : pieceMoves[5]) {
		if (!i) { break; }
		int toVal = mailbox[square + i];
		if (toVal * turn > 0 || toVal == NN || abs(toVal) == WK) {
			continue;
		}
		if (i == 2 || i == -2) { 
			checkCastling(); 
			if ((i == 2 && castling[0 + (turn == BLACK) * 2] == 1) || 
				(i == -2 && castling[1 + (turn == BLACK) * 2] == 1))
			{
				legalMoves.emplace_back(Move(square, square + i));
			}
		}
		else {
			legalMoves.emplace_back(Move(square, square + i));
		}
	}
}
void Board::genPawnCaptures(std::vector<Move>& captures, int square) {
	int fromVal = mailbox[square];
	assert(fromVal == WHITE || fromVal == BLACK);

	for (const int i : pieceMoves[0]) {
		if (!i) { break; }
		if (i == -10 || i == -20) { continue; }
		int toVal = mailbox[square + i * fromVal];
		if (toVal == NN || (toVal * turn >= 0 && square + i * fromVal != epSquare) || abs(toVal) == WK) {
			continue;
		}
		if (checkLegalPawn(square, square + i * fromVal, fromVal)) {
			captures.emplace_back(Move(square, square + i * fromVal));
		}
	}
}
void Board::genKnightCaptures(std::vector<Move>& captures, int square) {
	for (const int i : pieceMoves[1]) {
		if (!i) { break; }
		int toVal = mailbox[square + i];
		if (toVal * turn >= 0 || toVal == NN || abs(toVal) == WK) {
			continue;
		}
		captures.emplace_back(Move(square, square + i));
	}
}
void Board::genSliderCaptures(std::vector<Move>& captures, int square, int piece) {
	assert(piece == WB || piece == WR || piece == WQ);
	for (const int i : pieceMoves[piece - 1]) {
		if (!i) { break; }
		for (int j = 1; j < 8; ++j) {
			int to = square + j * i;
			int toVal = mailbox[to];
			if (toVal * turn > 0 || toVal == NN || abs(toVal) == WK) {
				break;
			}
			else if (toVal * turn < 0) {
				captures.emplace_back(Move(square, to));
				break;
			}
		}
	}
}
void Board::genKingCaptures(std::vector<Move>& captures, int square) {
	for (const int i : pieceMoves[5]) {
		if (!i) { break; }
		if (i == 2 || i == -2) { continue; }
		int toVal = mailbox[square + i];
		if (toVal * turn >= 0 || toVal == NN || abs(toVal) == WK) {
			continue;
		}
		captures.emplace_back(Move(square, square + i));
	}
}

//Get all pseudolegal moves
std::vector<Move> Board::getLegalMoves() {
	std::vector<Move> legalMoves;
	legalMoves.reserve(50);
	setEnPassantSquare();

	for (int i = 21; i < 99; ++i) {
		int piece = mailbox[i];
		int abs_piece = piece * turn;
		if (piece == NN || abs_piece <= 0) { continue; }
		switch (abs_piece) {
		case WP:
			genPawnMoves(legalMoves, i);
			break;
		case WN:
			genKnightMoves(legalMoves, i);
			break;
		case WB:
			genSliderMoves(legalMoves, i, WB);
			break;
		case WR:
			genSliderMoves(legalMoves, i, WR);
			break;
		case WQ:
			genSliderMoves(legalMoves, i, WQ);
			break;
		case WK:
			genKingMoves(legalMoves, i);
			break;
		}
	}
	return legalMoves;
}
std::vector<Move> Board::getCaptures() {
	std::vector<Move> captures;
	captures.reserve(20);
	setEnPassantSquare();

	for (int i = 21; i < 99; ++i) {
		int piece = mailbox[i];
		int abs_piece = piece * turn;
		if (piece == NN || abs_piece <= 0) { continue; }
		switch (abs_piece) {
		case WP:
			genPawnCaptures(captures, i);
			break;
		case WN:
			genKnightCaptures(captures, i);
			break;
		case WB:
			genSliderCaptures(captures, i, WB);
			break;
		case WR:
			genSliderCaptures(captures, i, WR);
			break;
		case WQ:
			genSliderCaptures(captures, i, WQ);
			break;
		case WK:
			genKingCaptures(captures, i);
			break;
		}
	}
	return captures;
}

bool Board::checkAttack(int a, int b) const {
	int fromVal = mailbox[a];
	int toVal = mailbox[b];

	if (toVal == NN || fromVal * toVal > 0) { return false; }

	int sval = (fromVal > 0) ? fromVal : fromVal * -1;
	switch (sval) {
	case WP:
		return checkAttackPawn(a, b, fromVal);
		break;

	case WN:
		return checkAttackKnight(a, b);
		break;

	case WB:
		return checkAttackSlider(a, b, WB);
		break;

	case WR:
		return checkAttackSlider(a, b, WR);
		break;

	case WQ:
		return checkAttackSlider(a, b, WQ);
		break;

	case WK:
		return checkAttackKing(a, b);
		break;
	}
}
bool Board::checkAttackPawn(int a, int b, int color) const {
	return (a - color * 11 == b || a - color * 9 == b);
}
bool Board::checkAttackKnight(int a, int b) const {
	return (a - 21 == b || a - 19 == b || a - 12 == b || a - 8 == b || a + 19 == b || a + 21 == b || a + 8 == b || a + 12 == b);
}
bool Board::checkAttackSlider(int a, int b, int piece) const {
	bool notRook, notBishop;
	if (piece != WB) {
		notRook = (a - b) % 10 != 0 && abs(a - b) > 7;
		if (piece == WR && notRook) { return false; }
	}
	if (piece != WR) {
		notBishop = (a - b) % 9 != 0 && (a - b) % 11 != 0;
		if (piece == WB && notBishop) { return false; }
	}
	if (piece == WQ && notBishop && notRook) { return false; }
	for (const int i : pieceMoves[piece - 1]) {
		if (!i) { break; }
		for (int j = 1; j < 8; ++j) {
			int to = a + j * i;
			if (to == b) { return true; }
			int toVal = mailbox[to];
			if (toVal * turn != 0) { break; }
		}
	}
	return false;
}
bool Board::checkAttackKing(int a, int b) const {
	return (a + 10 == b || a - 10 == b || a + 11 == b || a - 9 == b || a + 9 == b || a - 11 == b || a + 1 == b || a - 1 == b);
}
std::tuple<int, int> Board::getSmallestAttacker(int square, int color) {

	std::tuple<int, int> attackerArray[5];
	for (int i = 0; i < 5; ++i) {
		std::get<0>(attackerArray[i]) = 0;
		std::get<1>(attackerArray[i]) = 0;
	}

	if (mailbox[square] * color >= 0) {
		assert(std::get<0>(attackerArray[0]) == 0);
		assert(std::get<1>(attackerArray[0]) == 0);
		return attackerArray[0];
	}

	int smallest = WK;

	for (int i = 21; i < 99; ++i) {
		int piece = mailbox[i];
		if (piece == NN || piece * color <= 0) {
			continue;
		}
		piece = (piece > 0) ? piece : -piece;
		if (piece >= smallest) {
			continue;
		}
		switch (piece) {
		case WP:
			if (checkAttackPawn(i, square, mailbox[i])) {
				std::get<0>(attackerArray[0])++;
				std::get<1>(attackerArray[0]) = i;
			}
			smallest = WP;
			break;
		case WN:
			if (checkAttackKnight(i, square)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			smallest = WN;
			break;
		case WB:
			if (checkAttackSlider(i, square, WB)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			smallest = WN;  //Same value for knight and bishop
			break;
		case WR:
			if (checkAttackSlider(i, square, WR)) {
				std::get<0>(attackerArray[2])++;
				std::get<1>(attackerArray[2]) = i;
			}
			smallest = WR;
			break;
		case WQ:
			if (checkAttackSlider(i, square, WQ)) {
				std::get<0>(attackerArray[3])++;
				std::get<1>(attackerArray[3]) = i;
			}
			smallest = WQ;
			break;
		case WK:
			if (checkAttackKing(i, square)) {
				std::get<0>(attackerArray[4])++;
			}
			smallest = WK;
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

bool Board::inCheck(int color) {
	int kingsqr = kingSquare[!(color == WHITE)];
	for (int i = 21; i < 99; ++i) {
		int piece = mailbox[i];
		if (piece == NN || piece * color >= 0) {
			continue;
		}
		if (checkAttack(i, kingsqr)) {
			return true;
		}
	}
	return false;
}
bool Board::wouldCheck(Move m, int color) {
	int mailboxCopy[120], castlingCopy[4], kingCopy[2];
	memcpy(mailboxCopy, mailbox, sizeof(mailbox));
	memcpy(castlingCopy, castling, sizeof(castling));
	memcpy(kingCopy, kingSquare, sizeof(kingSquare));

	move(m.from, m.to);
	bool isinCheck = inCheck(-color);
	undo(mailboxCopy, castlingCopy, kingCopy, lazyScore);
	return isinCheck;
}

//Updates castling rights
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

	for (int i = 0; i < moveVec.size(); ++i) {
		int from = moveVec[i].from;
		//White king has moved
		if (from == 95 && (castling[0] != 0 || castling[1] != 0)) {
			castling[0] = 0;
			castling[1] = 0;;
		}
		//Black king has moved
		if (from == 25 && (castling[2] != 0 || castling[3] != 0)) {
			castling[2] = 0;
			castling[3] = 0;;
		}
		//White h-file rook has moved
		if (from == 98 && castling[0] != 0) {
			castling[0] = 0;
		}
		//White a-file rook has moved
		if (from == 91 && castling[1] != 0) {
			castling[1] = 0;
		}
		//Black h-file rook has moved
		if (from == 28 && castling[2] != 0) {
			castling[2] = 0;
		}
		//Black a-file rook has moved
		if (from == 21 && castling[3] != 0) {
			castling[3] = 0;
		}
	}

	for (int n = 21; n < 99; n++) {
		int piece = mailbox[n];
		if (piece == NN || piece == 0) { continue; }
		if (piece < 0) {
			//In check
			if ((castling[0] == 1 || castling[1] == 1) && checkAttack(n, kingSquare[0])) {
				castling[0] = -1;
				castling[1] = -1;
			}
			//Squares are attacked
			if (castling[0] == 1 && (checkAttack(n, 96) || checkAttack(n, 97))) {
				castling[0] = -1;
			}
			if (castling[1] == 1 && (checkAttack(n, 93) || checkAttack(n, 94))) {
				castling[1] = -1;
			}
		}
		else {
			if ((castling[2] == 1 || castling[3] == 1) && checkAttack(n, kingSquare[1])) {
				castling[2] = -1;
				castling[3] = -1;
			}
			if (castling[2] == 1 && (checkAttack(n, 26) || checkAttack(n, 27))) {
				castling[2] = -1;
			}
			if (castling[3] == 1 && (checkAttack(n, 23) || checkAttack(n, 24))) {
				castling[3] = -1;
			}
		}
	}
}
//Return an int representing which castling rights have been forfeited
int Board::checkCastlingForfeit() {
	int forfeit[4];
	for (int i = 0; i < 4; ++i) {
		forfeit[i] = abs(castling[i]);
	}
	for (int i = 0; i < moveVec.size(); ++i) {
		int from = moveVec[i].from;
		if (from == 95) {
			forfeit[0] = 1;
			forfeit[1] = 1;;
		}
		if (from == 25) {
			forfeit[2] = 1;
			forfeit[3] = 1;;
		}
		if (from == 98) {
			forfeit[0] = 1;
		}
		if (from == 91) {
			forfeit[1] = 0;
		}
		if (from == 28) {
			forfeit[2] = 1;
		}
		if (from == 21) {
			forfeit[3] = 1;
		}
	}
	return 0 ^ (forfeit[0]) ^ (forfeit[1] * 2) ^ (forfeit[2] * 4) ^ (forfeit[3] * 8);
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
	setEnPassantSquare();
	mailbox[b] = mailbox[a];
	mailbox[a] = 0;
	if (mailbox[b] == WK) {
		kingSquare[0] = b;
	}
	else if (mailbox[b] == BK) {
		kingSquare[1] = b;
	}
	moveVec.emplace_back(Move(a, b));
	specialMoves(a, b);
	turn *= -1;
}
void Board::undo(int mailboxCopy[], int castlingCopy[], int kingCopy[], int lazyCopy[]) {
	memcpy(mailbox, mailboxCopy, sizeof(mailbox));
	memcpy(castling, castlingCopy, sizeof(castling));
	memcpy(kingSquare, kingCopy, sizeof(kingSquare));
	memcpy(lazyScore, lazyCopy, sizeof(lazyScore));
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
		if (mailbox[i] == NN) { continue; }
		mailbox[i] = (empty) ? 0 : start[i];
	}
	for (int i = 0; i < 4; ++i) {
		castling[i] = !empty;
	}
	epSquare = -1;
	//Note : if the board resets to starting position lazyScore is not updated
	if (empty) {
		lazyScore[0] = 0;
		lazyScore[1] = 0;
	}
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