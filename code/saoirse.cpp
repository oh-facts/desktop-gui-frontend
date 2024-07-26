#include "saoirse_platform.h"
internal void ui_begin(UI_Context *cxt)
{
	UI_Widget *root = ui_make_widget(cxt, str8_lit("rootere"));
	ui_push_parent(cxt, root);
	cxt->root = root;
}

internal void ui_end(UI_Context *cxt)
{
	ui_pop_parent(cxt);
}

internal void create_window(Window *win, Str8 title, f32 x, f32 y)
{
	win->cxt = ui_alloc_cxt();
	win->title = title;
	ui_push_text_color(win->cxt, D_COLOR_WHITE);
	ui_push_bg_color(win->cxt, D_COLOR_WHITE);
	ui_push_pref_width(win->cxt, 0);
	ui_push_pref_height(win->cxt, 0);
	ui_push_fixed_pos(win->cxt, v2f{{0,0}});
	ui_push_size_kind(win->cxt, UI_SizeKind_Null);
	
	win->pos.x = x;
	win->pos.y = y;
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
	
	if(win->grabbed)
	{
		win->pos += screen_norm - win->mpos_last;
	}
	
	ui_begin(cxt);
	ui_push_pref_width(cxt, 0.3);
	ui_push_pref_height(cxt, 0.2);
	ui_push_fixed_pos(cxt, win->pos);
	
	ui_push_size_kind(win->cxt, UI_SizeKind_ChildrenSum);
	ui_colf(cxt, "window")
	{
		ui_rowf(cxt, "titlebar")
		{
			ui_push_size_kind(cxt, UI_SizeKind_Pixels);
			ui_label(cxt, win->title);
			win->minimize = ui_labelf(cxt, "hide").active;
			ui_pop_size_kind(cxt);
		}
		if(!win->minimize)
		{
			ui_rowf(cxt, "body")
			{
				ui_colf(cxt, "col1")
				{
					ui_push_size_kind(cxt, UI_SizeKind_Pixels);
					ui_labelf(cxt, "content1");
					ui_pop_size_kind(cxt);
					
					ui_rowf(cxt, "cw")
					{
						ui_push_size_kind(cxt, UI_SizeKind_Pixels);
						ui_labelf(cxt, "cw1");
						ui_labelf(cxt, "cw11");
						ui_labelf(cxt, "cw111");
						ui_pop_size_kind(cxt);
					}
					
					ui_push_size_kind(cxt, UI_SizeKind_Pixels);
					ui_labelf(cxt, "content111");
					ui_pop_size_kind(cxt);
				}
				
				ui_push_size_kind(cxt, UI_SizeKind_Pixels);
				ui_labelf(cxt, "content2");
				ui_pop_size_kind(cxt);
				
				ui_colf(cxt, "col3")
				{
					ui_push_size_kind(cxt, UI_SizeKind_Pixels);
					ui_labelf(cxt, "content3");
					ui_labelf(cxt, "content33");
					ui_labelf(cxt, "content333");
					ui_pop_size_kind(cxt);
				}
				
				ui_push_size_kind(cxt, UI_SizeKind_Pixels);
				ui_labelf(cxt, "content4");
				ui_labelf(cxt, "content5");
				ui_pop_size_kind(cxt);
				
			}
		}
	}
	ui_pop_size_kind(win->cxt);
	
	ui_pop_fixed_pos(cxt);
	ui_pop_pref_width(cxt);
	ui_pop_pref_height(cxt);
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
		
		char codepoints[] =
		{
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y','z',
			
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
			'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
			
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			
			'.', '?', ',', '-', ':', '!',
			
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
				state->font[c].tex = r_alloc_texture(temp_font[i].bmp, temp_font[i].w, temp_font[i].h, 1, &font_params);
			}
			state->font[c].bearing = temp_font[i].bearing;
			state->font[c].advance = temp_font[i].advance;
			state->font[c].x0 = temp_font[i].x0;
			state->font[c].x1 = temp_font[i].x1;
			state->font[c].y0 = temp_font[i].y0;
			state->font[c].y1 = temp_font[i].y1;
		}
		u32 *white_square = push_struct(arena, u32);
		*white_square = 0xFFFFFFFF;
		state->white_square = r_alloc_texture(white_square, 1, 1, 4, &tiled_params);
		
		arena_temp_end(&temp);
		
		create_window(&state->win[0], str8_lit("Entity list"), -0.8, 0.8);
		//create_window(&state->win[1], str8_lit("Spritesheet"), -0.5f, -0.3f);
		//create_window(&state->win[2], str8_lit("Info Panel"),  .7f, -0.3f);
	}
	
	State *state = (State*)pf->memory;
	
	Arena *arena = state->arena;
	Arena *trans = state->trans;
	
	Arena_temp temp = arena_temp_begin(trans);
	
	state->draw = {};
	state->draw.arena = trans;
	state->draw.white_square = state->white_square;
	state->draw.default_text_params =
	(D_Text_params){
		(v4f){{1,1,1,1}},
		0.00007,
		state->font
	};
	
	f32 aspect = (pf->win_size.x * 1.f)/ pf->win_size.y;
	d_push_proj_view(&state->draw, m4f_ortho(-aspect, aspect, -1, 1, -1.001, 1000).fwd);
	
	update_window(pf, input, &state->win[0], &state->draw);
	//update_window(pf, input, &state->win[1], &state->draw);
	//update_window(pf, input, &state->win[2], &state->draw);
	
	
	Str8 clocks = push_str8f(trans, "update and render: %llu", tcxt.counters_last[DEBUG_CYCLE_COUNTER_UPDATE_AND_RENDER].cycle_count);
	
	//d_draw_text(&state->draw, str8_lit("I love you :D"), v2f{{0,0.4}}, &default_text_params);
	//d_draw_rect(&state->draw, v2f{{0, 0}}, v2f{{0.6, 0.2}}, D_COLOR_THEME_1);
	//d_draw_text(&state->draw, clocks, v2f{{0,0}}, &default_text_params);
	
	//ui_end(cxt);
	
	
	d_pop_proj_view(&state->draw);
	
	r_submit(&state->draw.list, pf->win_size);
	
	arena_temp_end(&temp);
	
	END_TIMED_BLOCK(UPDATE_AND_RENDER);
	
	process_debug_counters();
}