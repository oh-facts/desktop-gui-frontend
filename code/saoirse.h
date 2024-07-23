/* date = July 18th 2024 6:07 am */

#ifndef SAOIRSE_H
#define SAOIRSE_H

struct Window
{
	UI_Context *cxt;
	v2f pos;
	v2f size;
	b32 minimize;
};

struct Game
{
	Arena *arena;
	Arena *trans;
	
	i32 temp;
	Window win;
	D_Bucket draw;
	R_Font font[256];
	R_Handle white_square;
};


#endif //SAOIRSE_H
