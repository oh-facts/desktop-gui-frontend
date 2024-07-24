#include "s_platform.h"

internal void create_window(Window *win, Str8 title, f32 x, f32 y)
{
	win->cxt = ui_alloc_cxt();
	win->title = title;
	ui_push_text_color(win->cxt, D_COLOR_WHITE);
	ui_push_bg_color(win->cxt, D_COLOR_WHITE);
	ui_push_pref_width(win->cxt, 0);
	ui_push_pref_height(win->cxt, 0);
	ui_push_fixed_pos(win->cxt, v2f{{0,0}});
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
	
	ui_push_pref_width(cxt, 0.2);
	ui_push_pref_height(cxt, 0.2);
	ui_push_fixed_pos(cxt, win->pos);
	// TODO(mizu): set size of parent to be equal to be size of children, or when
	// calculating row col offsets, check if prev sibling has a last child and if so, 
	// account for that instead of just prev sibling
	//ui_colf(cxt, "titlebar")
	{
		//ui_label(cxt, win->title);
		//ui_labelf(cxt, "ooga");
		//ui_labelf(cxt, "booga");
		//ui_labelf(cxt, "sooga");
		//ui_labelf(cxt, "rooga");
		/*
				ui_rowf(cxt, "titlebar buttons")
				{
					ui_push_pref_width(cxt, 0.8);
					if(ui_label(cxt, win->title).hot)
					{
						win->grabbed = input_is_mouse_held(input, MOUSE_BUTTON_LEFT);
					}
					ui_pop_pref_width(cxt);
					//ui_label(cxt,str8_lit("some"));
					win->minimize = ui_label(cxt,str8_lit("hide")).active;
				}
				*/
		if(!win->minimize)
		{
			ui_rowf(cxt, "row")
			{
				ui_labelf(cxt, "dooga");
				ui_labelf(cxt, "looga");
				for(i32 i = 0; i < 4; i++)
				{
					ui_labelf(cxt, "%d", i);
					ui_colf(cxt, "col %d", i)
					{
						for(i32 j = 0; j < 4; j++)
						{
							ui_labelf(cxt, "%d %d", i, j);
						}
					}
				}
			}
		}
		
	}
	
	ui_pop_fixed_pos(cxt);
	ui_pop_pref_width(cxt);
	ui_pop_pref_height(cxt);
	
	d_draw_ui(draw, cxt->root);
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
		Game *game = push_struct(arena, Game);
		pf->memory = (void*)game;
		game->arena = arena;
		game->trans = arena_create();
		game->temp = -1000;
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
		
		Arena_temp temp = arena_temp_begin(game->trans);
		Str8 font_path = push_str8f(game->trans,"%s../data/delius.ttf",pf->app_dir.c);
		Glyph *temp_font = make_bmp_font(font_path.c, codepoints, ARRAY_LEN(codepoints), game->trans);
		
		for(u32 i = 0; i < ARRAY_LEN(codepoints); i ++)
		{
			u32 c = codepoints[i];
			
			if(c != '\n' && c != ' ')
			{
				game->font[c].tex = r_alloc_texture(temp_font[i].bmp, temp_font[i].w, temp_font[i].h, 1, &font_params);
			}
			game->font[c].bearing = temp_font[i].bearing;
			game->font[c].advance = temp_font[i].advance;
			game->font[c].x0 = temp_font[i].x0;
			game->font[c].x1 = temp_font[i].x1;
			game->font[c].y0 = temp_font[i].y0;
			game->font[c].y1 = temp_font[i].y1;
		}
		u32 *white_square = push_struct(arena, u32);
		*white_square = 0xFFFFFFFF;
		game->white_square = r_alloc_texture(white_square, 1, 1, 4, &tiled_params);
		
		arena_temp_end(&temp);
		
		create_window(&game->win[0], str8_lit("Entity list"), -0.8, 0.8);
		//create_window(&game->win[1], str8_lit("Spritesheet"), -0.5f, -0.3f);
		//create_window(&game->win[2], str8_lit("Info Panel"),  .7f, -0.3f);
	}
	
	Game *game = (Game*)pf->memory;
	
	Arena *arena = game->arena;
	Arena *trans = game->trans;
	
	Arena_temp temp = arena_temp_begin(trans);
	
	game->draw = {};
	game->draw.arena = trans;
	game->draw.white_square = game->white_square;
	
	f32 aspect = (pf->win_size.x * 1.f)/ pf->win_size.y;
	d_push_proj_view(&game->draw, m4f_ortho(-aspect, aspect, -1, 1, -1.001, 1000).fwd);
	
	default_text_params =
	(D_Text_params){
		(v4f){{1,1,1,1}},
		0.00007,
		game->font
	};
	
	
	update_window(pf, input, &game->win[0], &game->draw);
	//update_window(pf, input, &game->win[1], &game->draw);
	//update_window(pf, input, &game->win[2], &game->draw);
	
	
	Str8 clocks = push_str8f(trans, "update and render: %llu", tcxt.counters_last[DEBUG_CYCLE_COUNTER_UPDATE_AND_RENDER].cycle_count);
	
	//d_draw_text(&game->draw, str8_lit("I love you :D"), v2f{{0,0.4}}, &default_text_params);
	//d_draw_rect(&game->draw, v2f{{0, 0}}, v2f{{0.6, 0.2}}, D_COLOR_THEME_1);
	//d_draw_text(&game->draw, clocks, v2f{{0,0}}, &default_text_params);
	
	//ui_end(cxt);
	
	
	d_pop_proj_view(&game->draw);
	
	r_submit(&game->draw.list, pf->win_size);
	
	arena_temp_end(&temp);
	
	END_TIMED_BLOCK(UPDATE_AND_RENDER);
	
	process_debug_counters();
}