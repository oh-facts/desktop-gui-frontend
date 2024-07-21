/* date = July 18th 2024 6:07 am */

#ifndef SAOIRSE_H
#define SAOIRSE_H

struct Game
{
	Arena *arena;
	Arena *trans;
	
	i32 temp;
	
	D_Bucket draw;
	R_Font font[256];
	R_Handle white_square;
};

#endif //SAOIRSE_H
