/* date = July 20th 2024 5:33 am */

#ifndef UI_H
#define UI_H

#define UI_DeferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

enum Axis2
{
  Axis2_X,
  Axis2_Y,
  Axis2_COUNT
};

enum UI_SizeKind
{
  UI_SizeKind_Null,
  UI_SizeKind_Pixels,
  UI_SizeKind_TextContent,
  UI_SizeKind_PercentOfParent,
  UI_SizeKind_ChildrenSum,
};

struct UI_Size
{
  UI_SizeKind kind;
  f32 value;
  f32 strictness;
};

struct UI_Signal
{
  b32 hot;
  b32 active;
	b32 toggle;
};

enum UI_Flags
{
  UI_Flags_has_text = 1 << 0,
  UI_Flags_has_bg = 1 << 1,
  UI_Flags_clickable = 1 << 2,
};

struct UI_Widget
{
  UI_Widget *first;
  UI_Widget *last;
  UI_Widget *next;
  UI_Widget *prev;
  
	u32 id;
	
  UI_Flags flags;
	Str8 text;
	UI_Size pref_size[Axis2_COUNT];
	v4f color;
	v4f bg_color;
	Axis2 child_layout_axis;
	v2f fixed_position;
	
	// calculated after hierearchy pass
	f32 computed_rel_position[Axis2_COUNT];
	f32 computed_size[Axis2_COUNT];
	v2f pos;
	v2f size;
	
  b32 hot;
	b32 active;
};

struct UI_Parent_node
{
	UI_Parent_node *next;
	UI_Widget *parent;
};

struct UI_Color_node
{
  UI_Color_node *next;
  v4f color;
};

struct UI_Pref_width_node
{
  UI_Pref_width_node *next;
  f32 w;
};

struct UI_Pref_height_node
{
  UI_Pref_height_node *next;
  f32 h;
};

struct UI_Fixed_pos_node
{
	UI_Fixed_pos_node *next;
	v2f pos;
};

struct UI_Axis2_node
{
	UI_Axis2_node *next;
	b32 auto_pop;
	Axis2 axis;
};

struct UI_Context
{
  Arena *arena;
  v2f mpos;
  b32 mdown;
  b32 mdown_last_frame;
  b32 mclick;
  
  UI_Widget *root;
  
  UI_Parent_node *parent_stack;
  UI_Color_node *text_color_stack;
  UI_Color_node *bg_color_stack;
  UI_Pref_width_node *pref_width_stack;
	UI_Pref_height_node *pref_height_stack;
	UI_Fixed_pos_node *fixed_pos_stack;
	UI_Axis2_node *child_layout_axis_stack;
	
	u32 num;
};

internal void ui_push_parent(UI_Context *cxt, UI_Widget *widget)
{
	UI_Parent_node *node = push_struct(cxt->arena, UI_Parent_node);
	node->parent = widget;
	if(!cxt->parent_stack)
	{
		cxt->parent_stack = node;
		cxt->root = widget;
	}
	else
	{
		node->next = cxt->parent_stack;
		cxt->parent_stack = node;
	}
}

internal void ui_pop_parent(UI_Context *cxt)
{
	cxt->parent_stack = cxt->parent_stack->next;
}

internal void ui_push_text_color(UI_Context *cxt, v4f color)
{
	UI_Color_node *node = push_struct(cxt->arena, UI_Color_node);
	node->color = color;
	if(!cxt->text_color_stack)
	{
		cxt->text_color_stack = node;
	}
	else
	{
		node->next = cxt->text_color_stack;
		cxt->text_color_stack = node;
	}
}

internal void ui_pop_text_color(UI_Context *cxt)
{
	cxt->text_color_stack = cxt->text_color_stack->next;
}

internal void ui_push_bg_color(UI_Context *cxt, v4f color)
{
	UI_Color_node *node = push_struct(cxt->arena, UI_Color_node);
	node->color = color;
	if(!cxt->bg_color_stack)
	{
		cxt->bg_color_stack = node;
	}
	else
	{
		node->next = cxt->bg_color_stack;
		cxt->bg_color_stack = node;
	}
}

internal void ui_pop_bg_color(UI_Context *cxt)
{
	cxt->bg_color_stack = cxt->bg_color_stack->next;
}

internal void ui_push_pref_width(UI_Context *cxt, f32 w)
{
	UI_Pref_width_node *node = push_struct(cxt->arena, UI_Pref_width_node);
	node->w = w;
	if(!cxt->pref_width_stack)
	{
		cxt->pref_width_stack = node;
	}
	else
	{
		node->next = cxt->pref_width_stack;
		cxt->pref_width_stack = node;
	}
}

internal void ui_pop_pref_width(UI_Context *cxt)
{
	cxt->pref_width_stack = cxt->pref_width_stack->next;
}

internal void ui_push_pref_height(UI_Context *cxt, f32 h)
{
	UI_Pref_height_node *node = push_struct(cxt->arena, UI_Pref_height_node);
	node->h = h;
	if(!cxt->pref_height_stack)
	{
		cxt->pref_height_stack = node;
	}
	else
	{
		node->next = cxt->pref_height_stack;
		cxt->pref_height_stack = node;
	}
}

internal void ui_pop_pref_height(UI_Context *cxt)
{
	cxt->pref_height_stack = cxt->pref_height_stack->next;
}

internal void ui_push_fixed_pos(UI_Context *cxt, v2f pos)
{
	UI_Fixed_pos_node *node = push_struct(cxt->arena, UI_Fixed_pos_node);
	node->pos = pos;
	if(!cxt->fixed_pos_stack)
	{
		cxt->fixed_pos_stack = node;
	}
	else
	{
		node->next = cxt->fixed_pos_stack;
		cxt->fixed_pos_stack = node;
	}
}

internal void ui_pop_fixed_pos(UI_Context *cxt)
{
	cxt->fixed_pos_stack = cxt->fixed_pos_stack->next;
}

internal void ui_push_child_layout_axis(UI_Context *cxt, Axis2 axis)
{
	UI_Axis2_node *node = push_struct(cxt->arena, UI_Axis2_node);
	
	node->axis = axis;
	if(!cxt->child_layout_axis_stack)
	{
		cxt->child_layout_axis_stack = node;
	}
	else
	{
		node->next = cxt->child_layout_axis_stack;
		cxt->child_layout_axis_stack = node;
	}
}

internal void ui_set_next_child_layout_axis(UI_Context *cxt, Axis2 axis)
{
	UI_Axis2_node *node = push_struct(cxt->arena, UI_Axis2_node);
	
	node->axis = axis;
	if(!cxt->child_layout_axis_stack)
	{
		cxt->child_layout_axis_stack = node;
	}
	else
	{
		node->next = cxt->child_layout_axis_stack;
		cxt->child_layout_axis_stack = node;
	}
	cxt->child_layout_axis_stack->auto_pop = 1;
}

internal void ui_pop_child_layout_axis(UI_Context *cxt)
{
	cxt->child_layout_axis_stack = cxt->child_layout_axis_stack->next;
}

internal UI_Widget *ui_make_widget(UI_Context *cxt)
{
  UI_Widget *widget = push_struct(cxt->arena, UI_Widget);
  widget->id = cxt->num++;
  
	if(cxt->parent_stack)
	{
		UI_Widget *parent = cxt->parent_stack->parent;
		if(parent->last)
		{
			widget->prev = parent->last;
			parent->last = parent->last->next = widget;
		}
		else
		{
			parent->last = parent->first = widget;
		}
	}
	
	widget->color = cxt->text_color_stack->color;
	widget->bg_color = cxt->bg_color_stack->color;
	widget->pref_size[Axis2_X].value = cxt->pref_width_stack->w;
	widget->pref_size[Axis2_Y].value = cxt->pref_height_stack->h;
	
	//widget->computed_rel_position[Axis2_X] = cxt->fixed_pos_stack->pos.x;
	//widget->computed_rel_position[Axis2_Y] = cxt->fixed_pos_stack->pos.y;
	
	widget->fixed_position = cxt->fixed_pos_stack->pos;
	
	if(cxt->child_layout_axis_stack)
	{
		if(cxt->child_layout_axis_stack->auto_pop)
		{
			widget->child_layout_axis = cxt->child_layout_axis_stack->axis;
			cxt->child_layout_axis_stack->auto_pop = 0;
			ui_pop_child_layout_axis(cxt);
		}
		
	}
	
	return widget;
}

internal void ui_begin_row(UI_Context *cxt)
{
	ui_set_next_child_layout_axis(cxt, Axis2_X);
	UI_Widget *widget = ui_make_widget(cxt);
	ui_push_parent(cxt, widget);
}

internal void ui_end_row(UI_Context *cxt)
{
	ui_pop_parent(cxt);
}

internal void ui_begin_col(UI_Context *cxt)
{
	ui_set_next_child_layout_axis(cxt, Axis2_Y);
	UI_Widget *widget = ui_make_widget(cxt);
	ui_push_parent(cxt, widget);
}

internal void ui_end_col(UI_Context *cxt)
{
	ui_pop_parent(cxt);
}

internal UI_Signal ui_label(UI_Context *cxt, Str8 text)
{
	UI_Widget *widget = ui_make_widget(cxt);
	widget->text = text;
	widget->flags = UI_Flags_has_text;
	
	UI_Signal out = {};
	
	return out;
}

#define ui_row(v) UI_DeferLoop(ui_begin_row(v), ui_end_row(v))
#define ui_col(v) UI_DeferLoop(ui_begin_col(v), ui_end_col(v))


#endif //UI_H
