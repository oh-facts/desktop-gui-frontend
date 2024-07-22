/* date = July 18th 2024 0:47 pm */

#ifndef DRAW_H
#define DRAW_H

#define D_COLOR_BLACK (v4f){{0,0,0,1}}
#define D_COLOR_WHITE (v4f){{1,1,1,1}}
#define D_COLOR_RED (v4f){{1,0,0,1}}
#define D_COLOR_GREEN (v4f){{0,1,0,1}}
#define D_COLOR_BLUE (v4f){{0,0,1,1}}
#define D_COLOR_THEME_1 (v4f){{0.66519, 0.37321, 0.12030,1}}
#define D_COLOR_THEME_2 (v4f){{0.03, 0.02, 0.03,1}}
#define D_COLOR_THEME_3 (v4f){{0.21044,0.02368,0.06198,1}}

struct D_Proj_view_node
{
	D_Proj_view_node *next;
	m4f v;
};

struct R_Font
{
	R_Handle tex;
	v2i bearing;
	i32 x0, y0, x1, y1;
	i32 advance;
};

struct D_Text_params
{
	v4f color;
	f32 scale;
	R_Font *font;
};

struct D_Bucket
{
	Arena *arena;
	R_Pass_list list;
	
	R_Handle white_square;
	D_Proj_view_node *proj_view_top;
	
};

internal void d_push_proj_view(D_Bucket *bucket, m4f proj_view)
{
	D_Proj_view_node *node = push_struct(bucket->arena, D_Proj_view_node);
	node->v = proj_view;
	
	if(bucket->proj_view_top)
	{
		node->next = bucket->proj_view_top;
		bucket->proj_view_top = node;
	}
	else
	{
		bucket->proj_view_top = node;
	}
	
}

internal void d_pop_proj_view(D_Bucket *bucket)
{
	bucket->proj_view_top = bucket->proj_view_top->next;
}

internal R_Pass *d_pass_from_bucket(D_Bucket *bucket, R_PASS_KIND kind)
{
	R_Pass_node *node = bucket->list.last;
	R_Pass *pass = 0;
	if(!node || node->pass.kind != kind)
	{
		pass = r_push_pass_list(bucket->arena, &bucket->list, kind);
	}
	else
	{
		pass = &node->pass;
	}
	
	return pass;
}

internal void d_draw_rect(D_Bucket *bucket, v2f pos, v2f scale, v4f color)
{
	R_Pass *pass = d_pass_from_bucket(bucket, R_PASS_KIND_UI);
	R_Rect *rect = r_push_batch(bucket->arena, &pass->rect_pass.rects, R_Rect);
	
	rect->pos = pos;
	
	rect->scale = scale;
	rect->tex = bucket->white_square;
	rect->color = color;
	pass->rect_pass.proj_view = bucket->proj_view_top->v;
}

internal void d_draw_text(D_Bucket *bucket, Str8 text, v2f pos, D_Text_params *p)
{
	v2f text_pos = pos;
	f32 width = 0;
	for(u32 i = 0; i < text.len; i ++)
	{
		char c = text.c[i];
		
		R_Font *ch = p->font + (u32)c;
		f32 xpos = text_pos.x + ch->bearing.x * p->scale;
		f32 ypos = text_pos.y + ch->bearing.y * p->scale;
		f32 w = (ch->x1 - ch->x0) * p->scale;
		f32 h = (ch->y1 - ch->y0) * p->scale;
		
		width += ch->advance * p->scale;
		
		if(c == ' ')
		{
			text_pos.x += ch->advance * p->scale;
			continue;
		}
		i32 max_w = 10000;
		if(width + w > max_w)
		{
			width = 0;
			text_pos.x = pos.x;
			text_pos.y -= 0.08;
		}
		else
		{
			text_pos.x += ch->advance * p->scale;
		}
		
		R_Pass *pass = d_pass_from_bucket(bucket, R_PASS_KIND_UI);
		R_Rect *rect = r_push_batch(bucket->arena, &pass->rect_pass.rects, R_Rect);
		
		rect->pos.x = xpos;
		rect->pos.y = ypos;
		rect->scale.x = w;
		rect->scale.y = h;
		rect->tex = ch->tex;
		rect->color = p->color;
		pass->rect_pass.proj_view = bucket->proj_view_top->v;
		
		//r_add_ui(list, ch.bmp_handle , (v2f){{xpos, ypos}}, (v2f){{w,h}} , p->color);
		
	}
}

local_persist D_Text_params default_text_params;

void d_draw_ui(D_Bucket *draw, UI_Widget *root)
{
	// calculate sizes
	UI_Widget *stack[1024];
	int stack_size = 0;
	stack[stack_size++] = root;
	
	while (stack_size > 0)
	{
		UI_Widget *cur = stack[--stack_size];
		
		UI_Widget *child = cur->first;
		
		while (child)
		{
			for (i32 i = 0; i < Axis2_COUNT; i++)
			{
				if (child->prev)
				{
					if (cur->child_layout_axis == Axis2_X)
					{
						child->computed_rel_position[Axis2_X] = child->prev->computed_rel_position[Axis2_X] + child->prev->pref_size[Axis2_X].value;
					}
					else if (cur->child_layout_axis == Axis2_Y)
					{
						// down is -ve
						child->computed_rel_position[Axis2_Y] = child->prev->computed_rel_position[Axis2_Y] - child->prev->pref_size[Axis2_Y].value;
					}
				}
			}
			
			//child->pos.x = child->computed_rel_position[Axis2_X];
			//child->pos.y = child->computed_rel_position[Axis2_Y];
			
			child->pos.x = cur->computed_rel_position[Axis2_X] + child->computed_rel_position[Axis2_X];
			child->pos.y = cur->computed_rel_position[Axis2_Y] + child->computed_rel_position[Axis2_Y];
			
			
			child->size.x = child->pref_size[Axis2_X].value;
			child->size.y = child->pref_size[Axis2_Y].value;
			
			stack[stack_size++] = child;
			child = child->next;
		}
	}
	
	// draw
	stack_size = 0;
	stack[stack_size++] = root;
	
	while (stack_size > 0)
	{
		UI_Widget *cur = stack[--stack_size];
		
		UI_Widget *child = cur->first;
		
		
		while (child)
		{
			
			child->pos += child->fixed_position;
			
			if(child->flags & UI_Flags_has_text)
			{
				D_Text_params params = 
				{
					child->color,
					0.00008,
					default_text_params.font
				};
				
				d_draw_text(draw, child->text, child->pos, &params);
			}
			
			stack[stack_size++] = child;
			child = child->next;
		}
	}
}

#endif //DRAW_H
