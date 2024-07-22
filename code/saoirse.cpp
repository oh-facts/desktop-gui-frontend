#include "s_platform.h"

enum R_CAMERA_PROJ
{
  R_CAMERA_PROJ_UN,
  R_CAMERA_PROJ_PERS,
  R_CAMERA_PROJ_ORTHO,
  R_CAMERA_PROJ_COUNT
};

#define R_WORLD_UP (v3f){{0,1,0}}
#define R_WORLD_FRONT (v3f){{0,0,-1}}

union quat
{
  struct
  {
    f32 r,i,j,k;
  };
  
  struct
  {
    f32 x,y,z,w;
  };
  
  f32 e[4];
};

internal quat operator*(quat x, quat y)
{
  return (quat){
    .r = x.r * y.r - x.i * y.i - x.j * y.j - x.k * y.k,
    .i = x.r * y.i + x.i * y.r + x.j * y.k - x.k * y.j,
    .j = x.r * y.j - x.i * y.k + x.j * y.r + x.k * y.i,
    .k = x.r * y.k + x.i * y.j - x.j * y.i + x.k * y.r
  };
}

struct R_Camera
{
  v3f pos;
  v3f target;
  v3f up;
  f32 pitch;
  f32 yaw;
  f32 zoom;
  v3f mv;
  v3f input_rot;
  f32 speed;
  f32 aspect;
  R_CAMERA_PROJ proj;
};

internal m4f quat_to_matrix(quat q) 
{
  f32 xx = q.i * q.i;
  f32 yy = q.j * q.j;
  f32 zz = q.k * q.k;
  f32 xy = q.i * q.j;
  f32 xz = q.i * q.k;
  f32 yz = q.j * q.k;
  f32 wx = q.r * q.i;
  f32 wy = q.r * q.j;
  f32 wz = q.r * q.k;
  
  return (m4f) {
    {
      {1 - 2 * (yy + zz), 2 * (xy - wz),     2 * (xz + wy),     0},
      {2 * (xy + wz),     1 - 2 * (xx + zz), 2 * (yz - wx),     0},
      {2 * (xz - wy),     2 * (yz + wx),     1 - 2 * (xx + yy), 0},
      {0,                 0,                 0,                 1}
    }
  };
}

internal m4f m4f_make_perspective(f32 fov, f32 aspect, f32 near, f32 far) 
{
  f32 tanHalfFov = tan(fov / 2.0f);
  f32 range = near - far;
  
  return (m4f) {
    {
      {1.0f / (aspect * tanHalfFov), 0, 0, 0},
      {0, 1.0f / tanHalfFov, 0, 0},
      {0, 0, (near + far) / range, 2 * near * far / range},
      {0, 0, -1, 0}
    }
  };
}

internal m4f_ortho_proj r_cam_get_proj_inv(R_Camera *cam)
{
  f32 z = cam->zoom;
  f32 za = z * cam->aspect;
  
  m4f_ortho_proj out = m4f_ortho(-za, za, -z, z, 0.001, 1000);
  return out;
}

internal m4f r_cam_get_proj(R_Camera *cam)
{
  switch(cam->proj)
  {
    case R_CAMERA_PROJ_ORTHO:
    {
      f32 z = cam->zoom;
      f32 za = z * cam->aspect;
      
      m4f out = m4f_ortho(-za, za, -z, z, 0.001, 1000).fwd;
      return out;
      
    }break;
    case R_CAMERA_PROJ_PERS:
    {
      quat rot_x = {
        .r = cosf(cam->input_rot.x / 2), 
        .i = sinf(cam->input_rot.x / 2)
      };
      quat rot_y = {
        .r = cosf(cam->input_rot.y / 2),
        .j = sinf(cam->input_rot.y / 2),
      };
      
      
      quat rotation = rot_x * rot_y;
      
      
      m4f rotation_matrix = quat_to_matrix(rotation);
      
      m4f persp = m4f_make_perspective(90, 16.f/9, 0.001,1000);
      
      return persp * rotation_matrix;
      
    }break;
    default:
    {
      INVALID_CODE_PATH();
    }
  }
  
  INVALID_CODE_PATH();
  return m4f_identity();
}

internal m4f r_cam_get_view(R_Camera *cam)
{
  return m4f_look_at(cam->pos, cam->pos + cam->target, cam->up);
}

internal void r_cam_input(R_Camera *cam, Input *input)
{
  switch(cam->proj)
  {
    case R_CAMERA_PROJ_ORTHO:
    {
      cam->mv.x = 0;
      cam->mv.y = 0;
      
      if(input_is_key_held(input, 'W'))
      {
        cam->mv.y = 1;
      }
      if(input_is_key_held(input, 'S'))
      {
        cam->mv.y = -1;
      }
      if(input_is_key_held(input, 'D'))
      {
        cam->mv.x = 1;
      }
      if(input_is_key_held(input, 'A'))
      {
        cam->mv.x = -1;
      }
      
    }break;
    case R_CAMERA_PROJ_PERS:
    {
      cam->mv.x = 0;
      cam->mv.y = 0;
      cam->mv.z = 0;
      
      if(input_is_key_held(input, 'W'))
      {
        cam->mv.z = 1;
      }
      if(input_is_key_held(input, 'S'))
      {
        cam->mv.z = -1;
      }
      if(input_is_key_held(input, 'D'))
      {
        cam->mv.x = 1;
      }
      if(input_is_key_held(input, 'A'))
      {
        cam->mv.x = -1;
      }
      if(input_is_key_held(input, 'Q'))
      {
        cam->mv.y = 1;
      }
      if(input_is_key_held(input, 'E'))
      {
        cam->mv.y = -1;
      }
      
      v2i mv = input_get_mouse_mv(input);
      
      
      cam->input_rot.x += mv.y * 0.001;
      cam->input_rot.y += mv.x * 0.001;
      
    }break;
    default:
    {
      INVALID_CODE_PATH();
    }
  }
  
}

internal void r_cam_update(R_Camera *cam, f32 delta)
{
  
  switch(cam->proj)
  {
    case R_CAMERA_PROJ_ORTHO:
    {
      cam->pos += cam->mv * cam->speed * delta;
      
    }break;
    case R_CAMERA_PROJ_PERS:
    {
      quat rot_x = {
        .r = cosf(cam->input_rot.x / 2), 
        .i = sinf(cam->input_rot.x / 2)
      };
      quat rot_y = {
        .r = cosf(cam->input_rot.y / 2),
        .j = sinf(cam->input_rot.y / 2),
      };
      
      
      quat rotation = rot_x * rot_y;
      
      
      m4f rotation_matrix = quat_to_matrix(rotation);
      
      cam->pos += (rotation_matrix * (v4f){.xyz = cam->mv * cam->speed * delta}).xyz;
      
    }break;
    default:
    {
      INVALID_CODE_PATH();
    }
  }
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
		
		game->cxt = ui_alloc_cxt();
		ui_push_text_color(game->cxt, D_COLOR_WHITE);
		ui_push_bg_color(game->cxt, D_COLOR_WHITE);
		ui_push_pref_width(game->cxt, 0.2);
		ui_push_pref_height(game->cxt, 0.1);
		ui_push_fixed_pos(game->cxt, v2f{{0,0}});
		
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
		0.00008,
		game->font
	};
	
	Str8 clocks = push_str8f(trans, "update and render: %llu", tcxt.counters_last[DEBUG_CYCLE_COUNTER_UPDATE_AND_RENDER].cycle_count);
	
	//d_draw_text(&game->draw, str8_lit("I love you :D"), v2f{{0,0.4}}, &default_text_params);
	//d_draw_rect(&game->draw, v2f{{0, 0}}, v2f{{0.6, 0.2}}, D_COLOR_THEME_1);
	//d_draw_text(&game->draw, clocks, v2f{{0,0}}, &default_text_params);
	
	f32 aspect_ratio = pf->win_size.x * 1.f / pf->win_size.y;
	v2f screen_norm ;
	screen_norm.x = input->mpos.x * 1.f / pf->win_size.y * 2.f - aspect_ratio;
	screen_norm.y = 1 - input->mpos.y * 1.f / pf->win_size.y * 2.f;
	
	UI_Context *cxt = game->cxt;
	cxt->mpos = screen_norm;
	cxt->mheld = input_is_mouse_held(input, MOUSE_BUTTON_LEFT);
	cxt->mclick = input_is_click(input, MOUSE_BUTTON_LEFT);
	
	ui_push_fixed_pos(cxt, v2f{{0.1,0.2}});
	ui_rowf(cxt, "row")
	{
		if(ui_labelf(cxt, "ooga").active)
		{
			if(ui_labelf(cxt, "booga").active)
			{
				for(i32 i = 0; i < 4; i++)
				{
					ui_colf(cxt, "col %d", i)
					{
						for(i32 j = 0; j < 4; j++)
						{
							local_persist v4f color = D_COLOR_WHITE;
							ui_push_text_color(cxt,color);
							if(ui_labelf(cxt, "%d %d", i, j).hot)
							{
								color = D_COLOR_BLUE;
							}
							else
							{
								color = D_COLOR_WHITE;
							}
							ui_pop_text_color(cxt);
						}
					}
				}
				
			}
		}
	}
	ui_pop_fixed_pos(cxt);
	
	
	d_draw_ui(&game->draw, cxt->root);
	//ui_end(cxt);
	
	
	d_pop_proj_view(&game->draw);
	
	r_submit(&game->draw.list, pf->win_size);
	
	arena_temp_end(&temp);
	
	END_TIMED_BLOCK(UPDATE_AND_RENDER);
	
	process_debug_counters();
}