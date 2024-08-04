/* date = July 18th 2024 6:07 am */

#ifndef SAOIRSE_H
#define SAOIRSE_H

enum WindowKind
{
	WindowKind_Null,
	WindowKind_Profiler,
	WindowKind_COUNT
};

struct Window
{
	UI_Context *cxt;
	Str8 title;
	v2f pos;
	v2f size;
	b32 minimize;
	b32 grabbed;
	v2f mpos_last;
	i32 cur_row;
	i32 visible_rows;
	i32 max_rows;
	
	WindowKind kind;
	
	union o
	{
		struct Window_profiler
		{
			i32 temp;
		};
	};
	
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
	R_Handle face;
};


#endif //SAOIRSE_H
