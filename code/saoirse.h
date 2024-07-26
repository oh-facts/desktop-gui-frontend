/* date = July 18th 2024 6:07 am */

#ifndef SAOIRSE_H
#define SAOIRSE_H

struct Window
{
	UI_Context *cxt;
	Str8 title;
	v2f pos;
	v2f size;
	b32 minimize;
	b32 grabbed;
	v2f mpos_last;
};

struct State
{
	Arena *arena;
	Arena *trans;
	i32 temp;
	
	Window win[3];
	D_Bucket draw;
	
	Glyph atlas[256];
	R_Font font[256];
	R_Handle white_square;
};


#endif //SAOIRSE_H
