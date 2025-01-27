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
	UI_Flags_has_scroll = 1 << 3,
};

struct UI_Widget
{
	UI_Widget *first;
	UI_Widget *last;
	UI_Widget *next;
	UI_Widget *prev;
	UI_Widget *parent;
	u64 num_child;
	
	UI_Widget *hash_next;
	UI_Widget *hash_prev;
	
	u64 hash;
	u64 last_frame_touched_index;
	
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
	
	// persistant
	v2f pos;
	v2f size;
	
	b32 hot;
	b32 active;
};

#define ui_make_style_struct(Name, Type) \
struct UI_##Name##_node \
{ \
UI_##Name##_node *next; \
Type v;\
};

ui_make_style_struct(Parent, UI_Widget *)
ui_make_style_struct(Color, v4f)
ui_make_style_struct(Pref_width, f32)
ui_make_style_struct(Pref_height, f32)
ui_make_style_struct(Fixed_pos, v2f)
ui_make_style_struct(Axis2, Axis2)

ui_make_style_struct(SizeKind_x, UI_SizeKind)
ui_make_style_struct(SizeKind_y, UI_SizeKind)

#define ui_make_style_struct_stack(Name, name) \
struct \
{\
UI_##Name##_node *top;\
UI_##Name##_node *free;\
b32 auto_pop;\
}name##_stack;

struct UI_Hash_slot
{
	UI_Widget *first;
	UI_Widget *last;
};

struct UI_Context
{
	Arena *arena;
	Arena *str_arena;
	
	v2f mpos;
	b32 mheld;
	b32 mclick;
	
	u64 hash_table_size;
	UI_Hash_slot *hash_slots;
	
	UI_Widget *root;
	u64 frames;
	Atlas *atlas;
	
	UI_Widget *widget_free_list;
	
	ui_make_style_struct_stack(Parent, parent);
	ui_make_style_struct_stack(Color, text_color);
	ui_make_style_struct_stack(Color, bg_color);
	ui_make_style_struct_stack(Pref_width, pref_width);
	ui_make_style_struct_stack(Pref_height, pref_height);
	ui_make_style_struct_stack(Fixed_pos, fixed_pos);
	ui_make_style_struct_stack(Axis2, child_layout_axis);
	ui_make_style_struct_stack(SizeKind_x, size_kind_x);
	ui_make_style_struct_stack(SizeKind_y, size_kind_y);
	
	u32 num;
};

internal UI_Context *ui_alloc_cxt()
{
	Arena *arena = arena_create();
	
	UI_Context *cxt = push_struct(arena, UI_Context);
	
	cxt->arena = arena;
	cxt->hash_table_size = 1024;
	cxt->hash_slots = push_array(arena, UI_Hash_slot, cxt->hash_table_size);
	cxt->str_arena = arena_create();
	return cxt;
}

struct text_extent
{
	v2f tl;
	v2f br;
};

internal text_extent ui_text_spacing_stats(Glyph *atlas, Str8 text, f32 scale)
{
	text_extent out = {};
	
	for(u32 i = 0; i < text.len; i ++)
	{
		char c = text.c[i];
		
		Glyph ch = *(atlas + (u32)c);
		
		if(out.tl.y < ch.y1 && c!=' ')
		{
			out.tl.y = ch.y1;
		}
		if(out.br.y > ch.y0 && c!= ' ')
		{
			out.br.y = ch.y0;
		}
		
		out.br.x += ch.advance;
	}
	
	out.tl *= scale;
	out.br *= scale;
	
	return out;
}

internal b32 ui_signal(v2f pos, v2f size, v2f mpos)
{
	b32 hot = 0;
	v2f tl = {};
	tl.x = pos.x;
	tl.y = pos.y;
	
	v2f br = {};
	br.x = tl.x + size.x;
	br.y = tl.y - size.y;
	
	if(mpos.x > tl.x && mpos.x < br.x && mpos.y < tl.y && mpos.y > br.y)
	{
		hot = true;
	}
	return hot;
}

internal UI_Widget *ui_alloc_widget(UI_Context *cxt)
{
	UI_Widget *out = cxt->widget_free_list;
	
	if(out)
	{
		cxt->widget_free_list= cxt->widget_free_list->next;
		memset(out, 0, sizeof(*out));
	}
	else
	{
		out = push_struct(cxt->arena, UI_Widget);
	}
	
	return out;
}

internal void ui_free_widget(UI_Context *cxt, UI_Widget *node)
{
	node->next = cxt->widget_free_list;
	cxt->widget_free_list = node;
}

#define ui_make_alloc_node(Name, name) \
internal UI_##Name##_node *ui_alloc_##name##_node(UI_Context *cxt) \
{ \
UI_##Name##_node *node = cxt->name##_stack.free;\
if(node)\
{\
cxt->name##_stack.free = cxt->name##_stack.free->next;\
memset(node, 0, sizeof(*node));\
}\
else\
{\
node = push_struct(cxt->arena, UI_##Name##_node);\
}\
return node;\
}

#define ui_make_free_node(Name, name) \
internal void ui_free_##name##_node(UI_Context *cxt, UI_##Name##_node *node)\
{\
node->next = cxt->name##_stack.free;\
cxt->name##_stack.free = node;\
}

ui_make_alloc_node(Parent, parent)
ui_make_free_node(Parent, parent)

ui_make_alloc_node(Color, text_color)
ui_make_free_node(Color, text_color)

ui_make_alloc_node(Color, bg_color)
ui_make_free_node(Color, bg_color)

ui_make_alloc_node(Pref_width, pref_width)
ui_make_free_node(Pref_width, pref_width)

ui_make_alloc_node(Pref_height, pref_height)
ui_make_free_node(Pref_height, pref_height)

ui_make_alloc_node(Fixed_pos, fixed_pos)
ui_make_free_node(Fixed_pos, fixed_pos)

ui_make_alloc_node(Axis2, child_layout_axis)
ui_make_free_node(Axis2, child_layout_axis)

ui_make_alloc_node(SizeKind_x, size_kind_x)
ui_make_free_node(SizeKind_x, size_kind_x)

ui_make_alloc_node(SizeKind_y, size_kind_y)
ui_make_free_node(SizeKind_y, size_kind_y)


#define ui_make_push_style(Name, name, Type) \
internal void ui_push_##name(UI_Context *cxt, Type val) { \
UI_##Name##_node *node = ui_alloc_##name##_node(cxt);\
node->v = val; \
if (!cxt->name##_stack.top) { \
cxt->name##_stack.top = node; \
} else { \
node->next = cxt->name##_stack.top; \
cxt->name##_stack.top = node; \
} \
}

#define ui_make_set_next_style(Name, name, Type) \
internal void ui_set_next_##name(UI_Context *cxt, Type val) { \
UI_##Name##_node *node = ui_alloc_##name##_node(cxt); \
node->v = val; \
if (!cxt->name##_stack.top) { \
cxt->name##_stack.top = node; \
} else { \
node->next = cxt->name##_stack.top; \
cxt->name##_stack.top = node; \
} \
cxt->name##_stack.auto_pop = 1;\
}

#define ui_make_pop_style(Name, name) \
internal void ui_pop_##name(UI_Context *cxt) { \
UI_##Name##_node *pop = cxt->name##_stack.top;\
cxt->name##_stack.top = cxt->name##_stack.top->next;\
ui_free_##name##_node(cxt, pop);\
}

ui_make_push_style(Parent, parent, UI_Widget*)
ui_make_pop_style(Parent, parent)

ui_make_push_style(Color, text_color, v4f)
ui_make_pop_style(Color, text_color)

ui_make_push_style(Color, bg_color, v4f)
ui_make_pop_style(Color, bg_color)

ui_make_push_style(Pref_width, pref_width, f32)
ui_make_pop_style(Pref_width, pref_width)

ui_make_push_style(Pref_height, pref_height, f32)
ui_make_pop_style(Pref_height, pref_height)

ui_make_push_style(Fixed_pos, fixed_pos, v2f)
ui_make_pop_style(Fixed_pos, fixed_pos)

ui_make_push_style(Axis2, child_layout_axis, Axis2)
ui_make_pop_style(Axis2, child_layout_axis)

ui_make_set_next_style(Axis2, child_layout_axis, Axis2)

ui_make_push_style(SizeKind_x, size_kind_x, UI_SizeKind)
ui_make_pop_style(SizeKind_x, size_kind_x)

ui_make_push_style(SizeKind_y, size_kind_y, UI_SizeKind)
ui_make_pop_style(SizeKind_y, size_kind_y)

#define ui_push_size_kind(cxt, kind) ui_push_size_kind_x(cxt, kind); \
ui_push_size_kind_y(cxt, kind);

#define ui_pop_size_kind(cxt) ui_pop_size_kind_x(cxt); \
ui_pop_size_kind_y(cxt);

// djb2
unsigned long
hash(Str8 str)
{
	unsigned long hash = 5381;
	int c;
	
	for(u32 i = 0; i < str.len; i++)
	{
		c = str.c[i];
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	
	return hash;
}

internal UI_Widget *ui_widget_from_hash(UI_Context *cxt, u64 hash)
{
	UI_Widget *widget = 0;
	
	u64 slot = hash % cxt->hash_table_size;
	
	UI_Widget *cur = cxt->hash_slots[slot].first;
	while(cur)
	{
		if(cur->hash == hash)
		{
			widget = cur;
			break;
		}
		
		cur = cur->hash_next;
	}
	
	return widget;
}

internal UI_Widget *ui_make_widget(UI_Context *cxt, Str8 text)
{
	u64 text_hash = hash(text);
	
	UI_Widget *widget = ui_widget_from_hash(cxt, text_hash);
	
	if(!widget)
	{
		widget = ui_alloc_widget(cxt);
		widget->hash = text_hash;
		u64 slot = text_hash % cxt->hash_table_size;
		
		if(cxt->hash_slots[slot].last)
		{
			cxt->hash_slots[slot].last = cxt->hash_slots[slot].last->hash_next = widget;
		}
		else
		{
			cxt->hash_slots[slot].last = cxt->hash_slots[slot].first = widget;
		}
		//		cxt->hash_slots[slot].first = widget;
		widget->last_frame_touched_index = cxt->frames;
		widget->id = cxt->num++;
	}
	else
	{
		if(widget->hot && cxt->mclick)
		{
			widget->active = !widget->active;
		}
		
		widget->prev = 0;
		widget->last = 0;
		widget->next = 0;
		widget->last_frame_touched_index = cxt->frames;
		widget->computed_size[0] = 0;
		widget->computed_size[1] = 0;
		
		widget->computed_rel_position[0] = 0;
		widget->computed_rel_position[1] = 0;
	}
	
	widget->text.c = push_array(cxt->str_arena, u8, text.len);
	widget->text.len = text.len;
	str8_cpy(&widget->text, &text);
	
	if(cxt->parent_stack.top)
	{
		UI_Widget *parent = cxt->parent_stack.top->v;
		parent->num_child++;
		widget->parent = parent;
		
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
	
	widget->color = cxt->text_color_stack.top->v;
	widget->bg_color = cxt->bg_color_stack.top->v;
	
	widget->pref_size[Axis2_X].kind = cxt->size_kind_x_stack.top->v;
	
	switch(widget->pref_size[Axis2_X].kind)
	{
		default:
		case UI_SizeKind_Null: {}break;
		case UI_SizeKind_ChildrenSum:
		{
			widget->pref_size[Axis2_X].value = 0;
		}break;
		case UI_SizeKind_PercentOfParent:
		case UI_SizeKind_Pixels:
		{
			widget->pref_size[Axis2_X].value = cxt->pref_width_stack.top->v;
		}break;
		case UI_SizeKind_TextContent:
		{
			text_extent extent = ui_text_spacing_stats(cxt->atlas->glyphs, text, 0.00007);
			
			widget->pref_size[Axis2_X].value = extent.br.x;
			
		}break;
		
	}
	
	widget->pref_size[Axis2_Y].kind = cxt->size_kind_y_stack.top->v;
	
	switch(widget->pref_size[Axis2_Y].kind)
	{
		default:
		case UI_SizeKind_Null: {}break;
		
		case UI_SizeKind_ChildrenSum:
		{
			widget->pref_size[Axis2_Y].value = 0;
		}break;
		case UI_SizeKind_PercentOfParent:
		case UI_SizeKind_Pixels:
		{
			widget->pref_size[Axis2_Y].value = cxt->pref_height_stack.top->v;
		}break;
		case UI_SizeKind_TextContent:
		{
			text_extent extent = ui_text_spacing_stats(cxt->atlas->glyphs, text, 0.00007);
			
			widget->pref_size[Axis2_Y].value = extent.tl.y - extent.br.y;
		}break;
		
	}
	
	widget->fixed_position = cxt->fixed_pos_stack.top->v;
	
	if(cxt->child_layout_axis_stack.auto_pop)
	{
		widget->child_layout_axis = cxt->child_layout_axis_stack.top->v;
		cxt->child_layout_axis_stack.auto_pop = 0;
		ui_pop_child_layout_axis(cxt);
	}
	return widget;
}

internal UI_Signal ui_begin_rowf(UI_Context *cxt, char *fmt, ...)
{
	Arena_temp temp = scratch_begin(0,0);
	va_list args;
	va_start(args, fmt);
	Str8 text = push_str8fv(temp.arena, fmt, args);
	va_end(args);
	
	ui_set_next_child_layout_axis(cxt, Axis2_X);
	UI_Widget *widget = ui_make_widget(cxt, text);
	ui_push_parent(cxt, widget);
	arena_temp_end(&temp);
	
	b32 hot = ui_signal(widget->pos, widget->size, cxt->mpos);
	widget->hot = hot;
	
	UI_Signal out = {};
	out.hot = hot;
	out.active = widget->active;
	
	return out;
}

internal void ui_end_row(UI_Context *cxt)
{
	ui_pop_parent(cxt);
}

internal UI_Signal ui_begin_colf(UI_Context *cxt, char *fmt, ...)
{
	Arena_temp temp = scratch_begin(0,0);
	va_list args;
	va_start(args, fmt);
	Str8 text = push_str8fv(temp.arena, fmt, args);
	va_end(args);
	
	ui_set_next_child_layout_axis(cxt, Axis2_Y);
	UI_Widget *widget = ui_make_widget(cxt, text);
	ui_push_parent(cxt, widget);
	arena_temp_end(&temp);
	
	b32 hot = ui_signal(widget->pos, widget->size, cxt->mpos);
	widget->hot = hot;
	
	UI_Signal out = {};
	out.hot = hot;
	out.active = widget->active;
	
	return out;
}

internal void ui_end_col(UI_Context *cxt)
{
	ui_pop_parent(cxt);
}

internal UI_Signal ui_label(UI_Context *cxt, Str8 text)
{
	UI_Widget *widget = ui_make_widget(cxt, text);
	widget->flags = UI_Flags_has_text;
	
	b32 hot = ui_signal(widget->pos, widget->size, cxt->mpos);
	widget->hot = hot;
	
	UI_Signal out = {};
	out.hot = hot;
	out.active = widget->active;
	
	return out;
}

internal UI_Signal ui_labelf(UI_Context *cxt, char *fmt, ...)
{
	Arena_temp temp = scratch_begin(0,0);
	va_list args;
	va_start(args, fmt);
	Str8 text = push_str8fv(temp.arena, fmt, args);
	va_end(args);
	
	UI_Signal out = ui_label(cxt, text); 
	arena_temp_end(&temp);
	return out;
}

internal UI_Signal ui_spacer(UI_Context *cxt, Str8 text)
{
	UI_Widget *widget = ui_make_widget(cxt, text);
	
	b32 hot = ui_signal(widget->pos, widget->size, cxt->mpos);
	widget->hot = hot;
	
	UI_Signal out = {};
	out.hot = hot;
	out.active = widget->active;
	
	return out;
}

internal UI_Signal ui_spacerf(UI_Context *cxt, char *fmt, ...)
{
	Arena_temp temp = scratch_begin(0,0);
	va_list args;
	va_start(args, fmt);
	Str8 text = push_str8fv(temp.arena, fmt, args);
	va_end(args);
	
	UI_Signal out = ui_spacer(cxt, text); 
	arena_temp_end(&temp);
	return out;
}

#define ui_rowf(v,...) UI_DeferLoop(ui_begin_rowf(v,__VA_ARGS__), ui_end_row(v))
#define ui_colf(v,...) UI_DeferLoop(ui_begin_colf(v,__VA_ARGS__), ui_end_col(v))

internal void ui_layout_fixed_size(UI_Widget *root, Axis2 axis)
{
	for(UI_Widget *child = root->first; child; child = child->next)
	{
		ui_layout_fixed_size(child, axis);
	}
	
	switch(root->pref_size[axis].kind)
	{
		default:
		case UI_SizeKind_Null:
		break;
		case UI_SizeKind_TextContent:
		case UI_SizeKind_Pixels:
		{
			root->computed_size[axis] += root->pref_size[axis].value;
		}break;
	}
}

internal void ui_layout_upward_dependent(UI_Widget *root, Axis2 axis)
{
	if(root->pref_size[axis].kind == UI_SizeKind_PercentOfParent)
	{
		if(root->parent->pref_size[axis].kind != UI_SizeKind_ChildrenSum)
		{
			//printf("%s\n", root->text.c);
		}
	}
	
	for(UI_Widget *child = root->first; child; child = child->next)
	{
		ui_layout_upward_dependent(child, axis);
	}
}

internal void ui_layout_downward_dependent(UI_Widget *root, Axis2 axis)
{
	for(UI_Widget *child = root->first; child; child = child->next)
	{
		ui_layout_downward_dependent(child, axis);
	}
	UI_Widget *parent = root->parent;
	if(parent)
	{
		if(parent->pref_size[axis].kind == UI_SizeKind_ChildrenSum)
		{
			if(parent->child_layout_axis == axis)
			{
				parent->computed_size[axis] += root->computed_size[axis];
			}
			else
			{
				if(parent->computed_size[axis] < root->computed_size[axis])
				{
					parent->computed_size[axis] = root->computed_size[axis];
				}
			}
		}
	}
}

// pre order
internal void ui_layout_pos(UI_Widget *root)
{
	if(root->parent)
	{
		root->computed_rel_position[0] = root->parent->computed_rel_position[0];
		root->computed_rel_position[1] = root->parent->computed_rel_position[1];
	}
	
	// TODO(mizu): Note how this just adds / subtracts instead of cumulatively adding 
	// /subtracting. I get this weird distance bug where the distance is accounted for twice when I remove it. I will record the commit so its easier to understand / demonstrate. 
	
	if(root->prev)
	{
		if(root->parent->child_layout_axis == Axis2_X)
		{
			root->computed_rel_position[Axis2_X] = root->prev->computed_rel_position[Axis2_X] + root->prev->computed_size[Axis2_X];
		}
		else if(root->parent->child_layout_axis == Axis2_Y)
		{
			root->computed_rel_position[Axis2_Y] = root->prev->computed_rel_position[Axis2_Y] - root->prev->computed_size[Axis2_Y];
		}
	}
	
	for(UI_Widget *child = root->first; child; child = child->next)
	{
		ui_layout_pos(child);
	}
	
}

// post order dfs
internal void ui_print_nodes_post_order(UI_Widget *root, i32 depth)
{
	for(UI_Widget *child = root->first; child; child = child->next)
	{
		ui_print_nodes_post_order(child, depth + 1);
	}
	
	for(i32 i = 0; i < depth; ++i) 
	{
		printf("-");
	}
	
	printf("%s [%.2f , %.2f] [%.2f , %.2f] \n", root->text.c, root->computed_size[0], root->computed_size[1], root->computed_rel_position[0], root->computed_rel_position[1]);
}

internal void ui_print_nodes_pre_order(UI_Widget *root, i32 depth)
{
	
	for(i32 i = 0; i < depth; ++i) 
	{
		printf("-");
	}
	
	printf("%s [%.2f , %.2f] [%.2f , %.2f] \n", root->text.c, root->computed_size[0], root->computed_size[1], root->computed_rel_position[0], root->computed_rel_position[1]);
	
	for(UI_Widget *child = root->first; child; child = child->next)
	{
		ui_print_nodes_pre_order(child, depth + 1);
	}
}

internal void ui_layout(UI_Widget *root)
{
	for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis + 1))
	{
		ui_layout_fixed_size(root, axis);
		ui_layout_upward_dependent(root, axis);
		ui_layout_downward_dependent(root, axis);
	}
	ui_layout_pos(root);
	//ui_print_nodes_pre_order(root, 0);
	//printf("\n");
}

internal void ui_begin(UI_Context *cxt)
{
	UI_Widget *root = ui_make_widget(cxt, str8_lit("rootere"));
	ui_push_parent(cxt, root);
	cxt->root = root;
	cxt->str_arena->used = ARENA_HEADER_SIZE;
}

internal void ui_end(UI_Context *cxt)
{
	ui_pop_parent(cxt);
	
	for(i32 i = 0; i < cxt->hash_table_size; i++)
	{
		UI_Widget *first_hash = (cxt->hash_slots + i)->first;
		if(!first_hash)
		{
			continue;
		}
		if(first_hash)
		{
			UI_Widget *cur = first_hash;
			UI_Widget *prev = 0;
			while(cur)
			{
				if(cur->last_frame_touched_index != cxt->frames)
				{
					//printf("pruned %.*s\n", str8_varg(cur->text));
					
					if(prev)
					{
						prev->hash_next = cur->hash_next;
						if (!cur->hash_next)
						{
							(cxt->hash_slots + i)->last = prev;
						}
					}
					else
					{
						(cxt->hash_slots + i)->first = cur->hash_next;
						if (!cur->hash_next)
						{
							(cxt->hash_slots + i)->last = 0;
						}
					}
					
					UI_Widget *to_free = cur;
					cur = cur->hash_next;
					ui_free_widget(cxt, to_free);
				}
				else
				{
					prev = cur;
					cur = cur->hash_next;
				}
			}
		}
	}
	//printf("\n");
}

/*
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
	
*/

#endif //UI_H
