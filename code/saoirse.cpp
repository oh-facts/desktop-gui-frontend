#include "saoirse_platform.h"

internal void create_window(Window *win, Str8 title, f32 x, f32 y, Atlas *atlas, WindowKind kind)
{
	win->cxt = ui_alloc_cxt();
	win->title = title;
	win->kind = kind;
	ui_push_text_color(win->cxt, D_COLOR_WHITE);
	ui_push_bg_color(win->cxt, D_COLOR_WHITE);
	ui_push_pref_width(win->cxt, 0);
	ui_push_pref_height(win->cxt, 0);
	ui_push_fixed_pos(win->cxt, v2f{{0,0}});
	ui_push_size_kind(win->cxt, UI_SizeKind_Null);
	win->cxt->atlas = atlas;
	win->pos.x = x;
	win->pos.y = y;
	win->cxt->frames = 0;
	win->cur_row = 0;
	win->visible_rows = 10;
	win->max_rows = 100;
}

internal void update_window(S_Platform *pf, Input *input, Window *win, D_Bucket *draw)
{
	f32 aspect_ratio = pf->win_size.x * 1.f / pf->win_size.y;
	v2f screen_norm ;
	screen_norm.x = input->mpos.x * 1.f / pf->win_size.y * 2.f - aspect_ratio;
	screen_norm.y = 1 - input->mpos.y * 1.f / pf->win_size.y * 2.f;
	
	UI_Context *cxt = win->cxt;
	cxt->mpos = screen_norm;
	cxt->mheld = input_is_mouse_held(input, MOUSE_BUTTON_LEFT);
	cxt->mclick = input_is_click(input, MOUSE_BUTTON_LEFT);
	cxt->frames++;
	
	if(win->grabbed)
	{
		win->pos += screen_norm - win->mpos_last;
	}
	
	ui_begin(cxt);
	
	ui_push_fixed_pos(cxt, win->pos);
	
	ui_push_size_kind_y(win->cxt, UI_SizeKind_ChildrenSum);
	ui_push_size_kind_x(cxt, UI_SizeKind_Pixels);
	ui_push_pref_width(cxt, 0.8);
	
	ui_colf(cxt, "window")
	{
		ui_rowf(cxt, "titlebar")
		{
			ui_push_size_kind_x(cxt, UI_SizeKind_ChildrenSum);
			UI_Signal grabbed = ui_begin_rowf(cxt, "title part");
			{
				ui_pop_size_kind_x(cxt);
				ui_push_size_kind(cxt, UI_SizeKind_TextContent);
				ui_label(cxt, win->title);
				ui_pop_size_kind(cxt);
				if(cxt->mheld)
				{
					if(grabbed.hot)
					{
						win->grabbed = 1;
					}
				}
				else
				{
					win->grabbed = 0;
				}
				
				Arena_temp temp_arena = scratch_begin(0,0);
				Str8 hide_str = push_str8f(temp_arena.arena, "hide");
				f32 title_width = ui_text_spacing_stats(cxt->atlas->glyphs, win->title, 0.00007).br.x;
				title_width += ui_text_spacing_stats(cxt->atlas->glyphs, hide_str, 0.00007).br.x;
				scratch_end(&temp_arena);
				
				ui_push_size_kind_x(cxt, UI_SizeKind_Pixels);
				ui_push_pref_width(cxt, 0.8 - title_width);
				ui_spacerf(cxt, "this spspacerino");
				ui_pop_pref_width(cxt);
				ui_pop_size_kind_x(cxt);
			}
			ui_end_row(cxt);
			ui_push_size_kind(cxt, UI_SizeKind_TextContent);
			win->minimize = ui_labelf(cxt, "hide").active;
			ui_pop_size_kind(cxt);
		}
		
		if(!win->minimize)
		{
			UI_Signal body_sig = ui_begin_colf(cxt, "body");
			
			if(win->kind == WindowKind_Null)
			{
				if(body_sig.hot)
				{
					// scroll
					if(input->scroll < 0)
					{
						win->cur_row++;
					}
					else if(input->scroll > 0)
					{
						win->cur_row --;
					}
					
					// clamp
					if(win->cur_row + win->visible_rows > win->max_rows)
					{
						win->cur_row = win->max_rows - win->visible_rows;
					}
					else if(win->cur_row < 0)
					{
						win->cur_row = 0;
					}
				}
				{
					ui_push_size_kind(cxt, UI_SizeKind_TextContent);
					for(i32 i = win->cur_row; i < win->visible_rows + win->cur_row; i++)
					{
						ui_labelf(cxt, "content %d",i);
					}
					ui_pop_size_kind(cxt);
				}
			}
			else if(win->kind == WindowKind_Profiler)
			{
				ui_push_size_kind(cxt, UI_SizeKind_TextContent);
				
				Arena_temp temp_arena = scratch_begin(0,0);
				//* 1e-6
				Str8 cmt_mem = push_str8f(temp_arena.arena, "cmt: %llu M", (u64)(pf->cmt * 1e-6));
				Str8 res_mem = push_str8f(temp_arena.arena, "res: %llu G", (u64)(pf->res * 1e-9));
				
				Str8 clocks =  push_str8f(temp_arena.arena, "update & render: %llu", tcxt.counters_last[DEBUG_CYCLE_COUNTER_UPDATE_AND_RENDER].cycle_count);
				ui_label(cxt, clocks);
				ui_label(cxt, cmt_mem);
				ui_label(cxt, res_mem);
				
				scratch_end(&temp_arena);
				ui_pop_size_kind(cxt);
			}
			
			ui_end_col(cxt);
		}
	}
	ui_pop_size_kind(win->cxt);
	ui_pop_pref_width(win->cxt);
	ui_pop_fixed_pos(cxt);
	
	ui_layout(cxt->root);
	d_draw_ui(draw, cxt->root);
	ui_end(cxt);
	
	win->mpos_last = screen_norm;
}

extern "C"
{
	YK_API void update_and_render(S_Platform *, Input *);
}

void update_and_render(S_Platform * pf, Input *input)
{
	BEGIN_TIMED_BLOCK(UPDATE_AND_RENDER);
	
	if(!pf->initialized)
	{
		pf->initialized = 1;
		
		s_global_platform_api_init(&pf->p_api);
		s_global_render_api_init(&pf->r_api);
		Arena *arena = arena_create(Megabytes(1), Gigabytes(1));
		State *state = push_struct(arena, State);
		pf->memory = (void*)state;
		state->arena = arena;
		state->trans = arena_create();
		state->temp = -1000;
		tcxt_init();
		
		R_Texture_params font_params = {
			R_TEXTURE_FILTER_LINEAR,
			R_TEXTURE_FILTER_LINEAR,
			R_TEXTURE_WRAP_CLAMP_TO_BORDER
		};
		
		R_Texture_params tiled_params = {
			R_TEXTURE_FILTER_LINEAR,
			R_TEXTURE_FILTER_LINEAR,
			R_TEXTURE_WRAP_REPEAT
		};
		
		R_Texture_params pixel_art_params = {
			R_TEXTURE_FILTER_NEAREST,
			R_TEXTURE_FILTER_NEAREST,
			R_TEXTURE_WRAP_CLAMP_TO_BORDER
		};
		
		char codepoints[] =
		{
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y','z',
			
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
			'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
			
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			
			'&', '.', '?', ',', '-', ':', '!',
			
			' ', '\n'
		};
		
		Arena_temp temp = arena_temp_begin(state->trans);
		Str8 font_path = push_str8f(state->trans,"%s../data/delius.ttf",pf->app_dir.c);
		Glyph *temp_font = make_bmp_font(font_path.c, codepoints, ARRAY_LEN(codepoints), state->trans);
		
		for(u32 i = 0; i < ARRAY_LEN(codepoints); i ++)
		{
			u32 c = codepoints[i];
			
			if(c != '\n' && c != ' ')
			{
				state->atlas_tex[c] = r_alloc_texture(temp_font[i].bmp, temp_font[i].w, temp_font[i].h, 1, &font_params);
			}
			
			state->atlas.glyphs[c].bearing = temp_font[i].bearing;
			state->atlas.glyphs[c].advance = temp_font[i].advance;
			state->atlas.glyphs[c].x0 = temp_font[i].x0;
			state->atlas.glyphs[c].x1 = temp_font[i].x1;
			state->atlas.glyphs[c].y0 = temp_font[i].y0;
			state->atlas.glyphs[c].y1 = temp_font[i].y1;
			
		}
		
		u32 *white_square = push_struct(arena, u32);
		*white_square = 0xFFFFFFFF;
		state->white_square = r_alloc_texture(white_square, 1, 1, 4, &tiled_params);
		
		Bitmap face = bitmap(str8_lit("C:/dev/saoirse/data/face.png"));
		state->face = r_alloc_texture(face.data, face.w, face.h, face.n, &pixel_art_params);
		
		arena_temp_end(&temp);
		
		create_window(&state->win[0], str8_lit("Entity list"), -0.8, 0.8, &state->atlas, WindowKind_Null);
		create_window(&state->win[1], str8_lit("Spritesheet"), -0.5f, -0.3f, &state->atlas, WindowKind_Null);
		create_window(&state->win[2], str8_lit("Info Panel"),  .7f, -0.3f, &state->atlas, WindowKind_Profiler);
	}
	
	State *state = (State*)pf->memory;
	
	//Arena *arena = state->arena;
	Arena *trans = state->trans;
	
	Arena_temp temp = arena_temp_begin(trans);
	
	state->draw = {};
	state->draw.arena = trans;
	state->draw.white_square = state->white_square;
	state->draw.default_text_params =
	(D_Text_params){
		(v4f){{1,1,1,1}},
		0.00007,
		&state->atlas,
		state->atlas_tex
	};
	
	f32 zoom = 1;
	f32 aspect = (pf->win_size.x * 1.f)/ pf->win_size.y;
	d_push_proj_view(&state->draw, m4f_ortho(-aspect * zoom, aspect * zoom, -zoom, zoom, -1.001, 1000).fwd);
	
	for(i32 i = 0; i < 3; i++)
	{
		//printf("%d \n", i);
		update_window(pf, input, state->win + i, &state->draw);
	}
	
	d_draw_img(&state->draw, v2f{{0,0}}, v2f{{1,1}}, D_COLOR_WHITE, state->face);
	
	d_pop_proj_view(&state->draw);
	
	r_submit(&state->draw.list, pf->win_size);
	
	//printf("%d\n",awa);
	
	arena_temp_end(&temp);
	
	END_TIMED_BLOCK(UPDATE_AND_RENDER);
	
	
	process_debug_counters();
}