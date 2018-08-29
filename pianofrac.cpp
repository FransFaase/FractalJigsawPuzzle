/* PianoFrac     Copyright (C) 2016 Frans Faase

   Program for generating Piano Fractal jigsaw puzzles.
	  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

GNU General Public License:
   http://www.iwriteiam.nl/GNU.txt

Details:
   http://www.iwriteiam.nl/D1612.html#11

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "insertonlymap.h" // http://www.iwriteiam.nl/insertonlymap3_h.txt

// Hexagonal transformation matrices

int transf[12][4] = {
	{  1,  0,  0,  1 },   {  0,  1,  1,  0 },
	{  1,  1, -1,  0 },   { -1,  0,  1,  1 },
	{  0,  1, -1, -1 },   { -1, -1,  0,  1 },
	{ -1,  0,  0, -1 },   {  0, -1, -1,  0 },
	{ -1, -1,  1,  0 },   {  1,  0, -1, -1 },
	{  0, -1,  1,  1 },   {  1,  1,  0, -1 },
};

// Definition of the field

#define FIELD_SIZE		9
#define FIELD_SIZE_2	(FIELD_SIZE+2)
const char *field[FIELD_SIZE_2] = {
	"           ",
	" abX       ",
	" bXabX     ",
	" XabXabX   ",
	"  bXabXab  ",
	"  XabXabXa ",
	"   bXabXab ",
	"   XabXabX ",
	"    bXabX  ",
	"     abX   ",
	"           ",
};

char fieldAt(int i, int j)
{
	if (i < -1 || i >= FIELD_SIZE+1 || j < -1 || j >= FIELD_SIZE+1)
		return ' ';
	return field[i+1][j+1];
}

#define POSITIONS 52								// number of x and X in field
#define BLACK_POSITIONS 19							// numbor of X in field
#define WHITE_POSITIONS (POSITIONS-BLACK_POSITIONS)	// number of x in field

// Unique numbers to the fields in the field
int field_numbers[FIELD_SIZE_2][FIELD_SIZE_2];
int num_x[POSITIONS];
int num_y[POSITIONS];
int white_x[WHITE_POSITIONS];
int white_y[WHITE_POSITIONS];
int trans[6][POSITIONS];

int fieldNumberAt(int i, int j)
{
	if (i < -1 || i >= FIELD_SIZE+1 || j < -1 || j >= FIELD_SIZE+1)
		return -1;
	return field_numbers[i+1][j+1];
}

void init_field_numbers()
{
	int num = 0;
	int num_white = 0;
	for (int i = 0; i < FIELD_SIZE_2; i++)
		for (int j = 0; j < FIELD_SIZE_2; j++)
			if (field[i][j] != ' ')
			{
				num_x[num] = i;
				num_y[num] = j;
				field_numbers[i][j] = num++;
				if (field[i][j] != 'X')
				{
					white_x[num_white] = j;
					white_y[num_white] = i;
					num_white++;
				}
			}
			else
				field_numbers[i][j] = -1;
		
	if (num != POSITIONS)
	{
		fprintf(stderr, "fatal: POSITIONS should be %d\n", num);
		exit(1);
	}
	if (num_white != WHITE_POSITIONS)
	{
		fprintf(stderr, "fatal: WHITE_POSITIONS should be %d\n", num_white);
		exit(1);
	}

	for (int i = 0, k = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			int field_number = fieldNumberAt(i, j);
			if (field_number != -1)
				trans[0][k++] = field_number;
		}

	for (int i = 0, k = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			int field_number = fieldNumberAt(j, i);
			if (field_number != -1)
				trans[1][k++] = field_number;
		}

	for (int i = 0, k = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			int field_number = fieldNumberAt(4-i+j, 8-i);
			if (field_number != -1)
				trans[2][k++] = field_number;
		}

	for (int i = 0, k = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			int field_number = fieldNumberAt(8-i, 4-i+j);
			if (field_number != -1)
				trans[3][k++] = field_number;
		}

	for (int i = 0, k = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			int field_number = fieldNumberAt(4-j+i, 8-j);
			if (field_number != -1)
				trans[4][k++] = field_number;
		}

	for (int i = 0, k = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			int field_number = fieldNumberAt(8-j, 4-j+i);
			if (field_number != -1)
				trans[5][k++] = field_number;
		}
}


// Classes for printing the field or a solution

class PrintBoard
{
	// Abstract class for printing a board.
	// Derived classes, need to define methods 'line' and 'name'.
public:
	virtual bool line(int x1, int y1, int x2, int y2) = 0;
	virtual char name(int x, int y) = 0;
	void print(FILE *f)
	{
		int xstart = 1;
		int ystart = -4;
		for (int i = 0; i < 18; i++)
		{
			if (i % 2 == 0)
			{
				fprintf(f, "  ");
				ystart++;
			}
			else
			{
				xstart++;
			}
			int x = xstart;
			int y = ystart;
			for (int j = 0; j < 8; j++, x--, y++)
				printf("%c%c%c%c",
						line(x+1, y, x, y) ? '\\' : ' ',
						line(x+1, y+1, x, y) ? '_' : ' ',
						line(x, y+1, x, y) ? '/' : ' ',
						name(x, y+1));
			fprintf(f, "\n");
		}
	}
};

class PrintField : public PrintBoard
{
public:
	PrintField(const char *(&field)[FIELD_SIZE_2]) : _field(field) {}
	virtual bool line(int x1, int y1, int x2, int y2)
	{
		if (0 <= x1 && x1 < FIELD_SIZE && 0 <= y1 && y1 < FIELD_SIZE && _field[x1+1][y1+1] != ' ')
			return true;
		if (0 <= x2 && x2 < FIELD_SIZE && 0 <= y2 && y2 < FIELD_SIZE && _field[x2+1][y2+1] != ' ')
			return true;
		return false;
	}
	virtual char name(int x, int y)
	{
		if (0 <= x && x < FIELD_SIZE && 0 <= y && y < FIELD_SIZE)
			return _field[x+1][y+1];
		return ' ';
	}
	
private:
	const char *(&_field)[FIELD_SIZE_2];
};


// Generate to Exact Cover with hard coded pieces

const int HC_PIECES_FIELD_SIZE = 16;
const char *hc_pieces_field[HC_PIECES_FIELD_SIZE] = {
	"A  b cCc  d     ",
	"  bB     dD     ",
	"           d    ",
	"  e   Ff        ",
	" eE fFf         ",
	"eE            j ",
	"   hH   iI    J ",
	"  hH h  I i jJj ",
	"         i      ",
	"  gG            ",
	"  G g           ",
	"  g             ",
	"                ",
	"                ",
	"                ",
	"                ",
};


const int NR_HC_PIECES = 10;
const int MAX_NR_POS_PER_PIECE = 7; 

struct HardcodedPiece
{
	HardcodedPiece() { size = 0; min_x = 40; min_y = 40; }
	int size;
	int min_x;
	int min_y;
	int x[MAX_NR_POS_PER_PIECE];
	int y[MAX_NR_POS_PER_PIECE];
	bool dark[MAX_NR_POS_PER_PIECE];
};
HardcodedPiece hardcodedPieces[NR_HC_PIECES];

void init_hardcoded_pieces()
{
	for (int i = 0; i < HC_PIECES_FIELD_SIZE; i++)
		for (int j = 0; j < HC_PIECES_FIELD_SIZE; j++)
			if (hc_pieces_field[i][j] != ' ')
			{
				bool dark = 'A' <= hc_pieces_field[i][j] && hc_pieces_field[i][j] <= 'Z'; 
				int p = hc_pieces_field[i][j] - (dark ? 'A' : 'a');
				if (p < 0 || p >= NR_HC_PIECES)
				{
					fprintf(stderr, "fatal: Char '%c' at hardcodedPieces[%d][%d] is wrong\n", hc_pieces_field[i][j], i, j);
					exit(1);
				}
				HardcodedPiece &hardcodedPiece = hardcodedPieces[p];
				hardcodedPiece.size++;
				if (i < hardcodedPiece.min_x)
					hardcodedPiece.min_x = i;
				if (j < hardcodedPiece.min_y)
					hardcodedPiece.min_y = j;
			}

	for (int p = 0; p < NR_HC_PIECES; p++)
	{
		if (hardcodedPieces[p].size > MAX_NR_POS_PER_PIECE)
		{
			printf("Piece %c has %d positions\n", p+'a', hardcodedPieces[p].size);
			return;
		}
		hardcodedPieces[p].size = 0;
	}

	for (int i = 0; i < HC_PIECES_FIELD_SIZE; i++)
		for (int j = 0; j < HC_PIECES_FIELD_SIZE; j++)
			if (hc_pieces_field[i][j] != ' ')
			{
				bool dark = 'A' <= hc_pieces_field[i][j] && hc_pieces_field[i][j] <= 'Z'; 
				int p = hc_pieces_field[i][j] - (dark ? 'A' : 'a');
				HardcodedPiece &hardcodedPiece = hardcodedPieces[p];
				hardcodedPiece.x[hardcodedPiece.size] = i - hardcodedPiece.min_x;
				hardcodedPiece.y[hardcodedPiece.size] = j - hardcodedPiece.min_y;
				hardcodedPiece.dark[hardcodedPiece.size] = dark;
				hardcodedPiece.size++;
			}
}

class BoolVector
{
public:
	BoolVector()
	{
		for (int i = 0; i < POSITIONS; i++)
			vector[i] = 0;
	}
	int compare(const BoolVector& rhs) const
	{
		for (int i = 0; i < POSITIONS; i++)
			if (vector[i])
			{
				if (!rhs.vector[i])
					return -1;
			}
			else
			{
				if (rhs.vector[i])
					return 1;
			}
		return 0;
	}
	bool& operator[](int i) { return vector[i]; }
	bool operator[](int i) const { return vector[i]; }
private:
	bool vector[POSITIONS];
};

class Number
{
public:
	Number(const BoolVector& vector) {}
	int nr;
};

class Placements : public InsertOnlyMap<BoolVector, Number> {};

void generate_ec_from_hardcode(bool with_piece_number)
{
	init_hardcoded_pieces();
			
	Placements all_placements;

	for (int p = 0; p < NR_HC_PIECES; p++)
	{
		HardcodedPiece &hardcodedPiece = hardcodedPieces[p];
		
		for (int x = -20; x < 40; x++)
			for (int y = -20; y < 40; y++)
				for (int t = 0; t < 12; t++)
				{
					if (p == 0 && t != 0) continue;
					
					BoolVector vector;
					
					bool fit = true;
					for (int i = 0; i < hardcodedPiece.size; i++)
					{
						int px = x + transf[t][0] * hardcodedPiece.x[i] + transf[t][2] * hardcodedPiece.y[i];
						int py = y + transf[t][1] * hardcodedPiece.x[i] + transf[t][3] * hardcodedPiece.y[i];
						char c = fieldAt(px, py);
						if (hardcodedPiece.dark[i] ? c == 'X' : (c == 'a' || c == 'b'))
							vector[field_numbers[px+1][py+1]] = true;
						else
						{
							fit = false;
							break;
						}
					}
										
					if (fit)
					{
						Number *number = all_placements.findOrCreate(vector);
						number->nr = p+1;
					}
				}
	}
	
	for (Placements::iterator it(all_placements); it.more(); it.next())
	{
		// generate vector
		for (int i = 0; i < POSITIONS; i++)
			printf("%d", it.key()[i]);
		
		if (with_piece_number)
			printf(" %d on", it.value().nr);
		
		char c = ' ';
		for (int i = 0; i < POSITIONS; i++)
			if (it.key()[i])
			{
				printf("%c%d", c, i);
				c = ',';
			}
		printf("\n");
	}
}

// Generated to Exact Cover (no hard coded)

class PieceKey
{
public:
	PieceKey()
	{
		for (int i = 0; i < FIELD_SIZE*FIELD_SIZE; i++)
			vector[i] = 0;
	}
	int compare(const PieceKey& rhs) const
	{
		for (int i = 0; i < FIELD_SIZE*FIELD_SIZE; i++)
			if (vector[i])
			{
				if (!rhs.vector[i])
					return -1;
			}
			else
			{
				if (rhs.vector[i])
					return 1;
			}
		return 0;
	}
	bool& operator[](int i) { return vector[i]; }
	bool operator[](int i) const { return vector[i]; }
private:
	bool vector[FIELD_SIZE*FIELD_SIZE];
};
class PieceNumber
{
public:
	PieceNumber(const PieceKey& key) : nr(-1) {}
	int nr;
};

class Pieces : public InsertOnlyMap<PieceKey, PieceNumber> {};
Pieces all_pieces;
int next_piece_nr = 2;

void addHardcodedPiece(HardcodedPiece &hardcodedPiece)
{
	PieceKey smallestKey;
	
	for (int t = 0; t < 12; t++)
	{
		int min_x = 1000;
		int min_y = 1000;
		for (int i = 0; i < hardcodedPiece.size; i++)
		{
			int px = transf[t][0] * hardcodedPiece.x[i] + transf[t][2] * hardcodedPiece.y[i];
			if (px < min_x)
				min_x = px;
			int py = transf[t][1] * hardcodedPiece.x[i] + transf[t][3] * hardcodedPiece.y[i];
			if (py < min_y)
				min_y = py;
		}
		
		PieceKey pieceKey;
		for (int i = 0; i < hardcodedPiece.size; i++)
		{
			int px = transf[t][0] * hardcodedPiece.x[i] + transf[t][2] * hardcodedPiece.y[i] - min_x;
			int py = transf[t][1] * hardcodedPiece.x[i] + transf[t][3] * hardcodedPiece.y[i] - min_y;
			if (px < 0 || px >= FIELD_SIZE || py < 0 || py >= FIELD_SIZE)
			{
				fprintf(stderr, "fatal: %d %d\n", px, py);
				exit(1);
			}
			pieceKey[px + FIELD_SIZE*py] = true;
		}
		if (t == 0 || pieceKey.compare(smallestKey) < 0)
			smallestKey = pieceKey;
	}
	PieceNumber* pieceNumber = all_pieces.findOrCreate(smallestKey);
	if (pieceNumber->nr != -1)
	{
		fprintf(stderr, "double piece\n");
		exit(1);
	}
	fprintf(stderr, "piece %d\n", next_piece_nr);
	for (int y = 0; y < FIELD_SIZE; y++)
	{
		for (int x = 0; x < FIELD_SIZE; x++)
				fprintf(stderr, "%d", smallestKey[x + FIELD_SIZE*y]);
			fprintf(stderr, "\n");
	}
	pieceNumber->nr = next_piece_nr++;
}

bool vector[POSITIONS];
int group[FIELD_SIZE_2][FIELD_SIZE_2];
int groups[POSITIONS];
int next_group = 0;

bool range[WHITE_POSITIONS+1];
int max_set;

int pieceNumberForCurrent()
{
	PieceKey smallestKey;
	
	for (int t = 0; t < 12; t++)
	{
		int min_x = 1000;
		int min_y = 1000;
		for (int x = 1; x <= FIELD_SIZE; x++)
			for (int y = 1; y <= FIELD_SIZE; y++)
				if (field_numbers[x][y] != -1 && vector[field_numbers[x][y]])
				{
					int px = transf[t][0] * x + transf[t][2] * y;
					if (px < min_x)
						min_x = px;
					int py = transf[t][1] * x + transf[t][3] * y;
					if (py < min_y)
						min_y = py;
				}
		
		PieceKey pieceKey;
		for (int x = 1; x <= FIELD_SIZE; x++)
			for (int y = 1; y <= FIELD_SIZE; y++)
				if (field_numbers[x][y] != -1 && vector[field_numbers[x][y]])
				{
					int px = transf[t][0] * x + transf[t][2] * y - min_x;
					int py = transf[t][1] * x + transf[t][3] * y - min_y;
					if (px < 0 || px >= FIELD_SIZE || py < 0 || py >= FIELD_SIZE)
					{
						fprintf(stderr, "fatal: %d %d\n", px, py);
						exit(1);
					}
					pieceKey[px + FIELD_SIZE*py] = true;
				}
		if (t == 0 || pieceKey.compare(smallestKey) < 0)
			smallestKey = pieceKey;
	}
	PieceNumber* pieceNumber = all_pieces.findOrCreate(smallestKey);
	if (pieceNumber->nr == -1)
	{
		fprintf(stderr, "piece %d\n", next_piece_nr);
		for (int y = 0; y < FIELD_SIZE; y++)
		{
			for (int x = 0; x < FIELD_SIZE; x++)
				fprintf(stderr, "%d", smallestKey[x + FIELD_SIZE*y]);
			fprintf(stderr, "\n");
		}
		pieceNumber->nr = next_piece_nr++;
	}
	return pieceNumber->nr;
}


int groupFor(int i)
{
	if (i == -1)
		return -1;
	while (groups[i] != -1)
		i = groups[i];
	return i;
}

void fill(int white_n, int nr_set, int nr_groups, bool with_piece_number)
{
	if (white_n == WHITE_POSITIONS)
	{
		if (range[nr_set] && nr_groups == 1)
		{
			for (int i = 0; i < POSITIONS; i++)
				printf("%c", vector[i] ? '1' : '0');
			if (with_piece_number)
				printf(" %d on", pieceNumberForCurrent());
			char sep = ' ';
			for (int i = 0; i < POSITIONS; i++)
				if (vector[i])
				{
					printf("%c%d", sep, i);
					sep = ',';
				}
			printf("\n");
		}
		return;
	}
	
	fill(white_n + 1, nr_set, nr_groups, with_piece_number);
	
	if (nr_set >= max_set)
		return;

	int x = white_x[white_n];
	int y = white_y[white_n];

	vector[field_numbers[x][y]] = true;

	if (   field[x][y] == 'a' 
		&& (field[x-1][y-1] == ' ' || !vector[field_numbers[x-1][y-1]]))
	{
		int old_group_x = group[x-1][y];
		int old_group_y = group[x][y-1];
		int new_group_x = -1;
		int new_group_y = -1;
	
		if (field[x-1][y] == 'X')
		{
			new_group_x = groupFor(old_group_x);
			if (new_group_x == -1) new_group_x = groupFor(group[x-2][y]);
			if (new_group_x == -1) new_group_x = groupFor(group[x-2][y-1]);
		}
		if (field[x][y-1] == 'X')
		{
			new_group_y = groupFor(old_group_y);
			if (new_group_y == -1) new_group_y = groupFor(group[x-1][y-2]);
			if (new_group_y == -1) new_group_y = groupFor(group[x][y-2]);
			if (new_group_y == -1) new_group_y = groupFor(group[x+1][y-1]);
		}
		
		if (new_group_x != -1)
		{
			if (old_group_x == -1)
			{
				group[x-1][y] = new_group_x;
				vector[field_numbers[x-1][y]] = true;
			}
			
			if (new_group_y != -1)
			{
				if (old_group_y == -1)
				{
					group[x][y-1] = new_group_y;
					vector[field_numbers[x][y-1]] = true;
				}

				if (new_group_x < new_group_y)
				{
			 		groups[new_group_x] = new_group_y;
			 		group[x][y] = new_group_y;
			 		fill(white_n + 1, nr_set+1, nr_groups-1, with_piece_number);
			 		group[x][y] = -1;
			 		groups[new_group_x] = -1;
			 	}
			 	else if (new_group_y < new_group_x)
				{
			 		groups[new_group_y] = new_group_x;
			 		group[x][y] = new_group_x;
			 		fill(white_n + 1, nr_set+1, nr_groups-1, with_piece_number);
			 		group[x][y] = -1;
			 		groups[new_group_y] = -1;
			 	}

				if (old_group_y == -1)
				{
					group[x][y-1] = -1;
					vector[field_numbers[x][y-1]] = false;
				}
			}
			
			if (old_group_y == -1)
			{
				group[x][y] = new_group_x;
				fill(white_n + 1, nr_set+1, nr_groups, with_piece_number);
				group[x][y] = -1;
			}

			if (old_group_x == -1)
			{
				group[x-1][y] = -1;
				vector[field_numbers[x-1][y]] = false;
			}
		}
		if (old_group_x == -1)
		{
			if (new_group_y != -1)
			{
				if (old_group_y == -1)
				{
					group[x][y-1] = new_group_y;
					vector[field_numbers[x][y-1]] = true;
				}

				group[x][y] = new_group_y;
				fill(white_n + 1, nr_set+1, nr_groups, with_piece_number);
				group[x][y] = -1;

				if (old_group_y == -1)
				{
					group[x][y-1] = -1;
					vector[field_numbers[x][y-1]] = false;
				}
			}
			if (old_group_y == -1)
			{
				group[x][y] = next_group++;
				fill(white_n + 1, nr_set+1, nr_groups+1, with_piece_number);
				group[x][y] = -1;
				next_group--;
			}
		}
	}
	
	if (   field[x][y] == 'b' 
		&& (field[x-1][y] == ' ' || !vector[field_numbers[x-1][y]])
		&& (field[x][y-1] == ' ' || !vector[field_numbers[x][y-1]]))
	{
		int old_group_xy = group[x-1][y-1];
		int old_group_f = group[x+1][y];
		int new_group_xy = -1;
		int new_group_f = -1;
	
		if (field[x-1][y-1] == 'X')
		{
			new_group_xy = groupFor(old_group_xy);
			if (new_group_xy == -1) new_group_xy = groupFor(group[x-2][y-1]);
			if (new_group_xy == -1) new_group_xy = groupFor(group[x-2][y-2]);
			if (new_group_xy == -1) new_group_xy = groupFor(group[x-1][y-2]);
		}
		if (field[x+1][y] == 'X')
		{
			new_group_f = groupFor(old_group_f);
			if (new_group_f == -1) new_group_f = groupFor(group[x+1][y-1]);
		}

		if (new_group_xy != -1)
		{
			if (old_group_xy == -1)
			{
				group[x-1][y-1] = new_group_xy;
				vector[field_numbers[x-1][y-1]] = true;
			}
			
			if (new_group_f != -1)
			{
				if (old_group_f == -1)
				{
					group[x+1][y] = new_group_f;
					vector[field_numbers[x+1][y]] = true;
				}

				if (new_group_xy < new_group_f)
				{
			 		groups[new_group_xy] = new_group_f;
			 		group[x][y] = new_group_f;
			 		fill(white_n + 1, nr_set+1, nr_groups-1, with_piece_number);
			 		group[x][y] = -1;
			 		groups[new_group_xy] = -1;
			 	}
			 	else if (new_group_f < new_group_xy)
				{
			 		groups[new_group_f] = new_group_xy;
			 		group[x][y] = new_group_xy;
			 		fill(white_n + 1, nr_set+1, nr_groups-1, with_piece_number);
			 		group[x][y] = -1;
			 		groups[new_group_f] = -1;
			 	}

				if (old_group_f == -1)
				{
					group[x+1][y] = -1;
					vector[field_numbers[x+1][y]] = false;
				}
			}
			if (old_group_f == -1)
			{
				group[x][y] = new_group_xy;
				fill(white_n + 1, nr_set+1, nr_groups, with_piece_number);
				group[x][y] = -1;
			}

			if (old_group_xy == -1)
			{
				group[x-1][y-1] = -1;
				vector[field_numbers[x-1][y-1]] = false;
			}
		}
		if (old_group_xy == -1)
		{
			if (new_group_f != -1)
			{
				if (old_group_f == -1)
				{
					group[x+1][y] = new_group_f;
					vector[field_numbers[x+1][y]] = true;
				}

				group[x][y] = new_group_f;
				fill(white_n + 1, nr_set+1, nr_groups, with_piece_number);
				group[x][y] = -1;

				if (old_group_f == -1)
				{
					group[x+1][y] = -1;
					vector[field_numbers[x+1][y]] = false;
				}
			}

			if (old_group_f == -1)
			{
				group[x][y] = next_group++;
				fill(white_n + 1, nr_set+1, nr_groups+1, with_piece_number);
				group[x][y] = -1;
				next_group--;
			}
		}
	}
	vector[field_numbers[x][y]] = false;
}

void generate_ec(bool with_piece_number)
{
	for (int i = 0; i < POSITIONS; i++)
	{
		vector[i] = false;
		groups[i] = -1;
	}
	for (int y = 0; y < FIELD_SIZE_2; y++)
		for (int x = 0; x < FIELD_SIZE_2; x++)
			group[x][y] = -1;
			
	if (with_piece_number)
	{
		init_hardcoded_pieces();
		
		for (int p = 1; p < NR_HC_PIECES; p++)
		{
			HardcodedPiece &hardcodedPiece = hardcodedPieces[p];
			addHardcodedPiece(hardcodedPiece);
		}
	}
		
	fill(0, 0, 0, with_piece_number);
}

// Iterator for Exact Cover output

#define MAX_NR_PIECES_IN_SOL POSITIONS

class Solution
{
public:
	Solution() : minimal(false), normalized(false), nr_pieces(0) {}
	int piece_numbers[POSITIONS];
	bool minimal;
	bool normalized;
	int nr_pieces;
	struct
	{
		int nr;
		int nr_white;
	} pieces[MAX_NR_PIECES_IN_SOL];
	
	int numberAt(int i, int j)
	{
		int field_number = fieldNumberAt(i, j);
		return field_number >= 0 ? piece_numbers[field_number] : -1;
	}
	
	void normalize(Solution &result)
	{
		normalized = true;
		minimal = true;
		for (int t = 0; t < 6; t++)
		{
			Solution candidate;
			
			int mapping[MAX_NR_PIECES_IN_SOL];
			for (int i = 0; i < nr_pieces; i++)
				mapping[i] = -1;
			
			for (int i = 0; i < POSITIONS; i++)
			{
				int piece_nr = piece_numbers[trans[t][i]];
				if (mapping[piece_nr] == -1)
				{
					mapping[piece_nr] = candidate.nr_pieces++;
					candidate.pieces[mapping[piece_nr]] = pieces[piece_nr];
				}
				candidate.piece_numbers[i] = mapping[piece_nr];
			}
			if (t == 0)
				result = candidate;
			else
			{
				for (int i = 0; i < POSITIONS; i++)
					if (candidate.piece_numbers[i] != result.piece_numbers[i])
					{
						if (candidate.piece_numbers[i] < result.piece_numbers[i])
						{
							result = candidate;
							minimal = false;
						}
						break;
					}
			}
		}
	}
	
	void print(FILE *f)
	{
		for (int p = 0; p < nr_pieces; p++)
		{
			if (pieces[p].nr > 0)
				fprintf(f, "%d on", pieces[p].nr);
			char ch = ' ';
			for (int i = 0; i < POSITIONS; i++)
				if (piece_numbers[i] == p)
				{
					fprintf(f, "%c%d", ch, i);
					ch = ',';
				}
			fprintf(f, "|");
		}
		fprintf(f, "\n");
	}
};

class SolutionIterator : public Solution
{
public:
	SolutionIterator(FILE *f) : _f(f) { next(); }
	bool more() { return _more; }
	void next()
	{
		char buffer[600];
		if (!(_more = fgets(buffer, 600, _f))) return;
		
		for (int i = 0; i < POSITIONS; i++)
				piece_numbers[i] = -1;
				
		nr_pieces = 0;
		char *s = buffer;
		while (*s != '\n' && *s != '\n' && *s != '\0')
		{
			int n;
			if (!parse_number(s, n))
				break;
			if (strncmp(s, " on ", 4) == 0)
			{
				s += 4;
				pieces[nr_pieces].nr = n;
				if (!parse_number(s, n))
					break;
			}
			else
				pieces[nr_pieces].nr = 0;
			for(;;)
			{
				if (n < 0 || n >= POSITIONS)
					break;
				piece_numbers[n] = nr_pieces;
				if (*s != ',') break;
				s++;
				if (!parse_number(s, n))
					break;
			}
			if (*s == '|')
				s++;
			nr_pieces++;
		}
	}
private:
	bool parse_number(char *&s, int &number)
	{
		number = 0;
		bool result = false;
		for (; '0' <= *s && *s <= '9'; s++)
		{
			result = true;
			number = 10 * number + *s-'0';
		}
		return result;
	}
	bool _more; 
	FILE *_f;
};

class PieceOccurances
{
public:
	PieceOccurances(const Solution &solution) 
	{
		nr_used_pieces = 0;
		max_occ = 0;
		for (int i = 0; i < solution.nr_pieces; i++)
		{
			int piece_nr = solution.pieces[i].nr;
			int j;
			for (j = 0; j < nr_used_pieces; j++)
				if (used_pieces[j].piece_nr >= piece_nr)
					break;
			if (j < nr_used_pieces && used_pieces[j].piece_nr == piece_nr)
			{
				used_pieces[j].occurances++;
				if (used_pieces[j].occurances > max_occ)
					max_occ = used_pieces[j].occurances;
			}
			else
			{
				for (int k = nr_used_pieces; k > j; k--)
					used_pieces[k] = used_pieces[k-1];
				used_pieces[j].piece_nr = piece_nr;
				used_pieces[j].occurances = 1;
				nr_used_pieces++;
			}
		}
	}

	void add(int piece_nr)
	{
		int j;
		for (j = 0; j < nr_used_pieces; j++)
			if (used_pieces[j].piece_nr >= piece_nr)
				break;
		if (j < nr_used_pieces && used_pieces[j].piece_nr == piece_nr)
		{
			used_pieces[j].occurances++;
			if (used_pieces[j].occurances > max_occ)
				max_occ = used_pieces[j].occurances;
		}
		else
		{
			for (int k = nr_used_pieces; k > j; k--)
				used_pieces[k] = used_pieces[k-1];
			used_pieces[j].piece_nr = piece_nr;
			used_pieces[j].occurances = 1;
			nr_used_pieces++;
		}
	}	
	int compare(const PieceOccurances &rhs) const
	{
		if (nr_used_pieces < rhs.nr_used_pieces)
			return -1;
		if (nr_used_pieces > rhs.nr_used_pieces)
			return 1;
		for (int i = 0; i < nr_used_pieces; i++)
		{
			if (used_pieces[i].piece_nr < rhs.used_pieces[i].piece_nr)
				return -1;
			if (used_pieces[i].piece_nr > rhs.used_pieces[i].piece_nr)
				return 1;
			if (used_pieces[i].occurances < rhs.used_pieces[i].occurances)
				return -1;
			if (used_pieces[i].occurances > rhs.used_pieces[i].occurances)
				return 1;
		}
	}
	void printSignature(FILE* f)
	{
		const char *sep = "";
		for (int i = 0; i < nr_used_pieces; i++)
		{
			int occ = used_pieces[i].occurances;
			for (int j = 0; j < occ; j++)
			{
				fprintf(f,"%s%d", sep, used_pieces[i].piece_nr);
				sep = ",";
			}
		}
		printf("\n");
	}
	void getSignature(char *buffer)
	{
		buffer[0] = '\0';
		char *s = buffer;
		const char *sep = "";
		for (int i = 0; i < nr_used_pieces; i++)
		{
			int occ = used_pieces[i].occurances;
			for (int j = 0; j < occ; j++)
			{
				sprintf(s, "%s%d", sep, used_pieces[i].piece_nr);
				while (*s != '\0')
					s++;
				sep = ",";
			}
		}
	}

	int nr_used_pieces;
	struct
	{
		int piece_nr;
		int occurances;
	} used_pieces[MAX_NR_PIECES_IN_SOL];
	int max_occ;
};

	
// Print solution

class PrintSol : public PrintBoard
{
public:
	PrintSol(Solution &solution) : _solution(solution) {}
	virtual bool line(int x1, int y1, int x2, int y2)
	{
		return _solution.numberAt(x1, y1) != _solution.numberAt(x2, y2);
	}
	virtual char name(int x, int y)
	{
		//if (0 <= x && x < FIELD_SIZE && 0 <= y && y < FIELD_SIZE
		//	&& _vector[_numbers[x+1][y+1]] == 0)
		//	return _field[x+1][y+1];
		return ' ';
	}
	
private:
	Solution _solution;
};


// Generate SVG

class GenerateSVG
{
public:
	GenerateSVG() : sidelength(100), color("red"), stroke_width(2), border_radius(1.5), border_d(1.7), bottom(false),
		target_height(-1), target_width(-1), margin(10)
	{
		sqrt32 = sqrt(0.75);
		dir_dx[0] =  0.0;     dir_dy[0] = -1;
		dir_dx[1] =  sqrt32;  dir_dy[1] = -0.5;
		dir_dx[2] =  sqrt32;  dir_dy[2] =  0.5;
		dir_dx[3] =  0.0;     dir_dy[3] =  1;
		dir_dx[4] = -sqrt32;  dir_dy[4] =  0.5;
		dir_dx[5] = -sqrt32;  dir_dy[5] = -0.5;
	 }
	
	double sidelength;
	double stroke_width;
	const char *color;
	double border_radius;
	double border_d;
	bool bottom;
	double target_height;
	double target_width;
	double margin;

	void draw(FILE *f, int depth, Solution &solution)
	{
		double total_width  = 2 * 1.3           + 2 * sqrt32 * 1.7;
		double border_width = 2 * border_radius + 2 * sqrt32 * border_d;
		if (border_width > total_width)
			total_width = border_width;
		x_center = margin + total_width * 0.5 * sidelength;
		
		double total_height  = 2 * 1.3           + 1.5 * 1.7;
		double border_height = 2 * border_radius + 1.5 * border_d;
		if (border_height > total_height)
		{
			total_height = border_height;
			y_center = margin + (border_radius + border_d) * sidelength;
		}
		else
			y_center = margin + 3.0 * sidelength;

		double extra_bottom_width = 0;
		if (bottom)
			extra_bottom_width = (2 * border_radius / sqrt32 + sqrt32 * border_d) * sidelength;

		double x_bottom_center = x_center + extra_bottom_width;
		double y_bottom_center = y_center - 0.5 * border_d - sqrt32 * sidelength;

		if (bottom && target_height > 2 * margin && target_width > 2 * margin)
		{
			double f = 3/sqrt(3);
			double w = target_width - 2 * margin;
			double h = target_height - 2 * margin;
			double w_a = (2 + 2/sqrt32) * border_radius + 3 * sqrt32 * border_d;
			double h_a =           	2.0 * border_radius +        1.5 * border_d;
			double y_offset = (h * w_a - w * h_a) / (h/f + w);
			if (y_offset < 0)
			{
				y_offset = 0;
				sidelength = h / h_a;
				fprintf(stderr, "h: %lf, w: %lf\n", h / h_a, w / w_a);
			}
			else
			{
			 	if (y_offset > f*(2/sqrt32 * border_radius + sqrt32 * border_d))
					y_offset = f*(2/sqrt32 * border_radius + sqrt32 * border_d);
				sidelength = w / (w_a - y_offset/f);
			}
			total_width = w_a - y_offset/f;
			total_height = h_a + y_offset;
			target_width = 2 * margin + sidelength * total_width;
			target_height = 2 * margin + sidelength * total_height;
			fprintf(stderr, "%lf %lf %lf %lf\n", w_a, h_a, y_offset, sidelength);
			fprintf(stderr, "%lf %lf\n", sidelength * ( w_a - y_offset/f ) + 2 * margin, target_width);
			fprintf(stderr, "%lf %lf\n", sidelength * ( h_a + y_offset ) + 2 * margin, target_height);
			x_center = margin + sidelength * ( border_radius + sqrt32 * border_d); 
			y_center = target_height - margin - sidelength * ( border_radius + 0.5 * border_d );
			fprintf(stderr, "%lf %lf\n", x_center, y_center);
			x_bottom_center = target_width - x_center;
			y_bottom_center = target_height - y_center;
			fprintf(stderr, "%lf %lf\n", x_bottom_center, y_bottom_center);
			fprintf(stderr, "%lf %lf\n",
							x_center + sidelength * (sqrt32 * border_d + 2 * border_radius / sqrt32 - y_offset / f),
							y_center - sidelength * (0.5 * border_d + y_offset));
			extra_bottom_width = 0;
		}
		fprintf(f, "<svg width=\"%.0f\" height=\"%.0f\" xmlns=\"http://www.w3.org/2000/svg\">\n",
				2*margin + total_width * sidelength + extra_bottom_width, 2*margin + total_height * sidelength);
		
		// Calculate direction of line with given depth
		x = 0;
		y = 0;
		radius = 1;
		rcosa = 1.0;
		rsina = 0.0;
		dir = 5;
		drawline(0, depth, "");
		double r = sqrt(x*x + y*y);
		radius = sidelength / r;
		rcosa = radius *  x / r;
		rsina = radius * -y / r;
		
		for (int j = 1; j <= 6; j++)
		{
			x = x_center - j * sidelength * 0.5;
			y = y_center + (j-4) * sidelength * sqrt32;
			
			printf("\n");
			
			for (int i = 0; i <= j; i++)
			{
				int p = j - 2 + i;
				int q = 2*j - 3 - i;
				char center = solution.numberAt(p, q+1);
				char o[6];
				o[0] = solution.numberAt(p-1, q+1  );
				o[1] = solution.numberAt(p-1, q+1-1);
				o[2] = solution.numberAt(p  , q+1-1 );
				o[3] = solution.numberAt(p+1, q+1  );
				o[4] = solution.numberAt(p+1, q+1+1);
				o[5] = solution.numberAt(p  , q+1+1);
				bool isolated = true;
				for (int k = 0; k < 6 && isolated; k++)
					isolated = center != o[k];
				if (isolated)
					center = o[0];
				for (int k = 0; k < 6; k++)
					if (center != o[k]) 
						drawcap(f, k);
				
				if (i < j)
				{
					dir = 5;
					bool drawit = fieldAt(p, q) != ' ' || fieldAt(p+1, q+1) != ' ';
					drawline(drawit ? f : 0, depth, color);
				}			
			}
			for (int i = j-1; i >= 0; i--)
			{
				int p = j - 2 + i;
				int q = 2*j - 3 - i;
				dir = 3;
				bool drawit = fieldAt(p,q) != ' ' || fieldAt(p, q-1) != ' ';
				drawline(drawit ? f : 0, depth, color);
				dir = 1;
				drawit = fieldAt(p,q) != ' ' || fieldAt(p-1, q) != ' ';
				drawline(drawit ? f : 0, depth, color);
			}
		}

		{
			double r = sidelength * border_radius;
			double d = sidelength * border_d;
			double x, y;
			x = x_center      - sqrt32 * r;
			y = y_center - d  -    0.5 * r;
			fprintf(f, "<path d=\"M%.2lf %.2lf\n", x, y);
			double x_t = x_center      + sqrt32 * r;
			double y_t = y_center - d  -    0.5 * r;
			fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", r, r, x_t, y_t);
			double x_b = x_center + sqrt32 * d + sqrt32 * r;
			double y_b = y_center +    0.5 * d -    0.5 * r;
			fprintf(f, "L %.2lf %.2lf\n", x_b, y_b);
			x = x_center + sqrt32 * d ;
			y = y_center +    0.5 * d + r;
			fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", r, r, x, y);
			x = x_center - sqrt32 * d ;
			y = y_center +    0.5 * d + r;
			fprintf(f, "L %.2lf %.2lf\n", x, y);
			x = x_center - sqrt32 * d - sqrt32 * r;
			y = y_center +    0.5 * d -    0.5 * r;
			fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", r, r, x, y);
			x = x_center      - sqrt32 * r;
			y = y_center - d  -    0.5 * r;
			fprintf(f, "L %.2lf %.2lf\n", x, y);
			fprintf(f, "\" stroke=\"%s\" stroke-width=\"%.2lf\" fill-opacity=\"0.0\"/>\n", color, stroke_width);
			
			if (bottom)
			{
				double x_bb = x_bottom_center	  - sqrt32 * r;
				double y_bb = y_bottom_center + d  +	0.5 * r;
				x = x_bottom_center - sqrt32 * d - sqrt32 * r;
				y = y_bottom_center -	0.5 * d +	0.5 * r;
				fprintf(stderr, "y %lf %lf %lf\n", y, y_t, y_bb);
				if (y < y_t)
				{
					if (y_bb < y_t)
						fprintf(f, "<path d=\"M%.2lf %.2lf\n", x_bb, y_bb);
					else
						fprintf(f, "<path d=\"M%.2lf %.2lf\n", x_t, y_t);
					fprintf(f, "L %.2lf %.2lf\n", x, y);
				}
				else
					fprintf(f, "<path d=\"M%.2lf %.2lf\n", x, y);
				x = x_bottom_center - sqrt32 * d ;
				y = y_bottom_center -    0.5 * d - r;
				fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", r, r, x, y);

				x = x_bottom_center + sqrt32 * d ;
				y = y_bottom_center -    0.5 * d - r;
				fprintf(f, "L %.2lf %.2lf\n", x, y);
				x = x_bottom_center + sqrt32 * d + sqrt32 * r;
				y = y_bottom_center -    0.5 * d +    0.5 * r;
				fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", r, r, x, y);

				x = x_bottom_center      + sqrt32 * r;
				y = y_bottom_center + d  +    0.5 * r;
				fprintf(f, "L %.2lf %.2lf\n", x, y);
				fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", r, r, x_bb, y_bb);
				
/* Turn this off and simply use the Z command to close the SVG path which is the bottom of the puzzle.
				if (y_bb > y_b)
					fprintf(f, "L %.2lf %.2lf\n", x_b, y_b);
				fprintf(f, "\" stroke=\"%s\" stroke-width=\"%.2lf\" fill-opacity=\"0.0\"/>\n", color, stroke_width);
*/
				fprintf(f, " Z\" stroke=\"%s\" stroke-width=\"%.2lf\" fill-opacity=\"0.0\"/>\n", color, stroke_width);
			}
		}

		fprintf(f, "</svg>\n");
	}

private:
	double sqrt32;
	double x;
	double y;
	double radius;
	double rsina;
	double rcosa;
	double x_center;
	double y_center;

	int dir;

	double dir_dx[6];
	double dir_dy[6];

	void move(int dir)
	{
		x -= rcosa * dir_dx[dir] - rsina * dir_dy[dir];
		y -= rsina * dir_dx[dir] + rcosa * dir_dy[dir];
	}

	void left(FILE* f, int t)
	{
		for (int i = 0; i < t; i++)
		{
			move(dir);
			dir = (dir + 1)%6;
		}
		if (f != 0) fprintf(f, "A %.2lf %.2lf 0 0 1 %.2lf %.2lf ", radius, radius, x, y);
	}

	void right(FILE* f, int t)
	{
		for (int i = 0; i < t; i++)
		{
			dir = (dir + 5)%6;
			move(dir);
		}
		if (f != 0) fprintf(f, "A %.2lf %.2lf 0 0 0 %.2lf %.2lf ", radius, radius, x, y);
	}


	void drawseg(FILE* f, int depth)
	{
		if (depth < 0)
			return;
		drawseg(f, depth-1);
		right(f, 2);
		drawseg(f, depth-1);
		left(f, 2);
		drawseg(f, depth-1);
	}

	void drawline(FILE* f, int depth, const char* color)
	{
		move((dir+4)%6);
		right(0, 1);
		if (f != 0) fprintf(f, "<path d=\"M%.2lf %.2lf\n", x, y);
		drawseg(f, depth);
		left(f, 2);
		drawseg(f, depth);
		right(f, 2);
		drawseg(f, depth);
		right(0, 1);
		move((dir+1)%6);
		if (f != 0) fprintf(f, "\" stroke=\"%s\" stroke-width=\"%.2lf\" fill-opacity=\"0.0\"/>\n", color, stroke_width);
	}

	void drawcap(FILE* f, int capdir)
	{
		double k_x = x;
		double k_y = y;
		
		move(capdir);
		move((capdir+1)%6);
		dir = (capdir+4)%6;
		if (f != 0) fprintf(f, "<path d=\"M%.2lf %.2lf ", x, y);
		right(f, 2);
		if (f != 0) fprintf(f, "\" stroke=\"%s\" stroke-width=\"%.2lf\" fill-opacity=\"0.0\"/>\n", color, stroke_width);
		move((capdir+2)%6);
		move((capdir+3)%6);
		
		x = k_x;
		y = k_y;
	}	
};

// Main body of program

void print_usage(const char *program_name)
{
	fprintf(stderr,
			"Usage:\n"
			"  %s gen_ec_hc [-with_name]\n"
			"  %s gen_ec [-con] [-range=n,n-n,n-] [-with_name]\n"
			"  %s normalize [-minimal]\n"
			"  %s used_pieces [-max_occ=n] [-sup_occ=n] [-max=n] [-min=n]\n"
			"  %s filter (<pieces>)\n"
			"  %s print\n" 
			"  %s svg [-border_rad=r] [-border_d=r] [-depth=n] [-colour=c] [-stroke_width=r] [-side_length=r]\n"
			"         [-bottom] [-width=r] [-height=r] [-margin=r]",
			program_name, program_name, program_name, program_name,
			program_name, program_name, program_name);
}

int main(int argc, char *argv[])
{
	init_field_numbers();	
	
	if (argc == 1) { print_usage(argv[0]); return 1; }
	
	if (strcmp(argv[1], "gen_ec_hc") == 0)
	{
		bool with_name = false;
		if (argc > 3) { print_usage(argv[0]); return 1; }
		if (argc == 3)
		{
			if (strcmp(argv[2], "-with_name") != 0) { print_usage(argv[0]); return 1; }
			with_name = true;
		}
		generate_ec_from_hardcode(with_name);
	}
	else if (strcmp(argv[1], "gen_ec") == 0)
	{
		for (int i = 0; i <= WHITE_POSITIONS; i++)
			range[i] = false;
		max_set = 0;
		bool include_con = false;
		bool with_name = false;
			
		for (int i = 2; i < argc; i++)
		{
			if (strcmp(argv[i], "-con") == 0)
				include_con = true;
			else if (strncmp(argv[i], "-range=", 7) == 0)
			{
				const char *s = argv[i]+7;
				while (*s != '\0')
				{
					if (*s < '0' || '9' < *s)
					{
						fprintf(stderr, "error: expecting digit at '%s' in range\n", s);
						exit(1);
					}
					int sv = 0;
					for (; '0' <= *s && *s <= '9'; s++)
						sv = 10*sv + *s - '0';
					if (sv < 1)
					{
						fprintf(stderr, "error: range value should be larger than 0\n");
						exit(1);
					}
					if (sv > WHITE_POSITIONS)
					{
						fprintf(stderr, "error: range value should be smaller or equal to %d\n", WHITE_POSITIONS);
						exit(1);
					}
					if (*s == '-')
					{
						s++;
						int ev = 0;
						if (*s == '\0')
							ev = WHITE_POSITIONS;
						else
							for (; '0' <= *s && *s <= '9'; s++)
								ev = 10*ev + *s - '0';
						if (ev > WHITE_POSITIONS)
						{
							fprintf(stderr, "error: range value should be smaller or equal to %d\n", WHITE_POSITIONS);
							exit(1);
						}
						if (ev < sv)
						{
							fprintf(stderr, "error : range %d-%d is empty\n", sv, ev);
							exit(1);
						}
						for (int v = sv; v <= ev; v++)
							range[v] = true;
						if (ev > max_set)
							max_set = ev;
					}
					else
					{
						range[sv] = true;
						if (sv > max_set)
							max_set = sv;
					}						
					if (*s == ',')
						s++;
				}
			}
			else if (strcmp(argv[i], "-with_name") == 0)
				with_name = true;
			else
			{ 
				fprintf(stderr, "error: unknown option %s\n", argv[i]);
				exit(1);
			}
		}
		if (max_set == 0)
		{
			for (int v = 1; v <= WHITE_POSITIONS; v++)
				range[v] = true;
			max_set = WHITE_POSITIONS;
		}

		if (include_con)
		{
			for (int i = 1; i <= FIELD_SIZE; i++)
				for (int j = 1; j <= FIELD_SIZE; j++)
					if (field[i][j] == 'X')
					{
						int num = field_numbers[i][j];
						for (int i = 0; i < POSITIONS; i++)
							printf("%c", i == num ? '1' : '0');
						if (with_name)
							printf(" 1 on");
						printf(" %d\n", num);
					}
		} 
		generate_ec(with_name);
	}
	else if (strcmp(argv[1], "normalize") == 0)
	{
		if (argc > 3) { print_usage(argv[0]); return 1; }
		bool filter_minimal = false;
		if (argc == 3)
		{
			if (strcmp(argv[2], "-minimal") != 0) { print_usage(argv[0]); return 1; }
			filter_minimal = true;
		}
		
		for (SolutionIterator sol_it(stdin); sol_it.more(); sol_it.next())
		{
			Solution normalized;
			sol_it.normalize(normalized);
			if (!filter_minimal || sol_it.minimal)
				normalized.print(stdout);
		}
	}
	else if (strcmp(argv[1], "used_pieces") == 0)
	{
		int max_occ = 1000;
		int sup_occ = 1000;
		int max = 1000;
		int min = 0;
		for (int i = 2; i < argc; i++)
		{
			if (strncmp(argv[i], "-max_occ=", 9) == 0)
			{
				const char *s = argv[i]+9;
				int sv = 0;
				for (; '0' <= *s && *s <= '9'; s++)
					sv = 10*sv + *s - '0';
				if (sv > 0)
					max_occ = sv;
			}
			else if (strncmp(argv[i], "-sup_occ=", 9) == 0)
			{
				const char *s = argv[i]+9;
				int sv = 0;
				for (; '0' <= *s && *s <= '9'; s++)
					sv = 10*sv + *s - '0';
				if (sv > 0)
					sup_occ = sv;
			}
			else if (strncmp(argv[i], "-max=", 5) == 0)
			{
				const char *s = argv[i]+5;
				int sv = 0;
				for (; '0' <= *s && *s <= '9'; s++)
					sv = 10*sv + *s - '0';
				if (sv > 0)
					max = sv;
			}
			else if (strncmp(argv[i], "-min=", 5) == 0)
			{
				const char *s = argv[i]+5;
				int sv = 0;
				for (; '0' <= *s && *s <= '9'; s++)
					sv = 10*sv + *s - '0';
				if (sv > 0)
					min = sv;
			}
			else
			{ 
				fprintf(stderr, "error: unknown option %s\n", argv[i]);
				exit(1);
			}
		}
		bool found_solutions = false;
		int min_max_occ = 1000;
		int min_sup_occ = 1000;
		int min_max = 1000;
		int max_min = 0;
		for (SolutionIterator sol_it(stdin); sol_it.more(); sol_it.next())
		{
			PieceOccurances pieceOccurances(sol_it);
			
			if (   pieceOccurances.max_occ <= max_occ
				&& sol_it.nr_pieces - pieceOccurances.nr_used_pieces <= sup_occ
				&& sol_it.nr_pieces <= max 
				&& sol_it.nr_pieces >= min)
			{
				found_solutions = true;
				pieceOccurances.printSignature(stdout);
			}
			else
			{
				if (pieceOccurances.max_occ < min_max_occ)
					min_max_occ = pieceOccurances.max_occ;
				if (sol_it.nr_pieces - pieceOccurances.nr_used_pieces < min_sup_occ)
					min_sup_occ = sol_it.nr_pieces - pieceOccurances.nr_used_pieces;
				if (sol_it.nr_pieces < min_max)
					min_max = sol_it.nr_pieces;
				if (sol_it.nr_pieces > max_min)
					max_min = sol_it.nr_pieces;
			}
		}
		if (!found_solutions)
			fprintf(stderr, "No solutions found: -max_occ >= %d, -sup_occ= >= %d, -max >= %d -min <= %d\n", min_max_occ, min_sup_occ, min_max, max_min);
	}		
	else if (strcmp(argv[1], "filter") == 0)
	{
		for (SolutionIterator sol_it(stdin); sol_it.more(); sol_it.next())
		{
			PieceOccurances pieceOccurances(sol_it);

			char buffer[100];
			pieceOccurances.getSignature(buffer);

			bool selected = false;
			for (int i = 2; i < argc; i++)
				if (strcmp(buffer, argv[i]) == 0)
				{
					selected = true;
					break;
				}
			if (selected)
				sol_it.print(stdout);
		}
	}
	else if (strcmp(argv[1], "print") == 0)
	{
		for (SolutionIterator sol_it(stdin); sol_it.more(); sol_it.next())
		{
			PrintSol printSol(sol_it);
			printSol.print(stdout);
		}
	}
	else if (strcmp(argv[1], "svg") == 0)
	{
		GenerateSVG generateSVG;
		int depth = 2;
		for (int i = 2; i < argc; i++)
		{
			if (strncmp(argv[i], "-border_rad=", 12) == 0)
			{
				double value;
				if (sscanf(argv[i]+12, "%lf", &value) > 0)
					generateSVG.border_radius = value;
			}
			else if (strncmp(argv[i], "-border_d=", 10) == 0)
			{
				double value;
				if (sscanf(argv[i]+10, "%lf", &value) > 0)
					generateSVG.border_d = value;
			}
			else if (strncmp(argv[i], "-depth=", 7) == 0)
			{
				char ch = argv[i][7];
				if ('1' <= ch && ch <= '4' && argv[i][8] == '\0')
					depth = ch - '0';
			}
			else if (strncmp(argv[i], "-colour=", 8) == 0)
			{
				generateSVG.color = argv[i]+8;
			}
			else if (strncmp(argv[i], "-stroke_width=", 14) == 0)
			{
				double value;
				if (sscanf(argv[i]+14, "%lf", &value) > 0)
					generateSVG.stroke_width = value;
			}
			else if (strncmp(argv[i], "-side_length=", 13) == 0)
			{
				double value;
				if (sscanf(argv[i]+13, "%lf", &value) > 0)
				{
					//fprintf(stderr, "border_d = %lf\n", value);
					generateSVG.sidelength = value;
				}
			}
			else if (strcmp(argv[i], "-bottom") == 0)
			{
				generateSVG.bottom = true;
			}
			else if (strncmp(argv[i], "-height=", 8) == 0)
			{
				double value;
				if (sscanf(argv[i]+8, "%lf", &value) > 0 && value > 0.0)
				{
					fprintf(stderr, "height = %lf\n", value);
					generateSVG.target_height = value;
				}
			}
			else if (strncmp(argv[i], "-width=", 7) == 0)
			{
				double value;
				if (sscanf(argv[i]+7, "%lf", &value) > 0 && value > 0.0)
				{
					fprintf(stderr, "width = %lf\n", value);
					generateSVG.target_width = value;
				}
			}
			else if (strncmp(argv[i], "-margin=", 8) == 0)
			{
				double value;
				if (sscanf(argv[i]+8, "%lf", &value) > 0 && value >= 0.0)
				{
					fprintf(stderr, "margin = %lf\n", value);
					generateSVG.margin = value;
				}
			}
			else
			{ 
				fprintf(stderr, "error: unknown option %s\n", argv[i]);
				exit(1);
			}
		}
		
		SolutionIterator sol_it(stdin);
		if (sol_it.more())
		{
			
			generateSVG.draw(stdout, depth, sol_it);
		}
	}
	else { print_usage(argv[0]); return 1; }
	
	return 0;
}
1171c1171
< 		double y_bottom_center = y_center - 0.5 * border_d;
---
> 		double y_bottom_center = y_center - 0.5 * border_d - sqrt32 * sidelength;
1265c1265
< 				int q = 2*j - 3 - i;
---
> 				_int q = 2*j - 3 - i;
