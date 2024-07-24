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
  Arena *build_arena;
	
	v2f mpos;
  b32 mheld;
  b32 mclick;
  
	u64 hash_table_size;
	UI_Hash_slot *hash_slots;
	
	UI_Widget *root;
  
	ui_make_style_struct_stack(Parent, parent);
	ui_make_style_struct_stack(Color, text_color);
	ui_make_style_struct_stack(Color, bg_color);
	ui_make_style_struct_stack(Pref_width, pref_width);
	ui_make_style_struct_stack(Pref_height, pref_height);
	ui_make_style_struct_stack(Fixed_pos, fixed_pos);
	ui_make_style_struct_stack(Axis2, child_layout_axis);
	
	u32 num;
};

internal UI_Context *ui_alloc_cxt()
{
	Arena *arena = arena_create();
	
	UI_Context *cxt = push_struct(arena, UI_Context);
	
	cxt->arena = arena;
	cxt->hash_table_size = 1024;
	cxt->hash_slots = push_array(arena, UI_Hash_slot, cxt->hash_table_size);
	
	return cxt;
}

internal b32 ui_signal(v2f pos, v2f size, v2f mpos)
{
	b32 hot = 0;
  v2f tl = {};
  tl.x = pos.x;
  tl.y = pos.y;
  
  v2f br = {};
  br.x = pos.x + size.x;
  br.y = pos.y + size.y;
  
  if(mpos.x > tl.x && mpos.x < br.x && mpos.y > tl.y && mpos.y < br.y)
  {
		hot = true;
  }
  return hot;
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

internal void ui_push_parent(UI_Context *cxt, UI_Widget *widget)
{
	UI_Parent_node *node = ui_alloc_parent_node(cxt);
	node->v = widget;
	if(!cxt->parent_stack.top)
	{
		cxt->parent_stack.top = node;
		cxt->root = widget;
	}
	else
	{
		node->next = cxt->parent_stack.top;
		cxt->parent_stack.top = node;
	}
}

internal void ui_pop_parent(UI_Context *cxt)
{
	UI_Parent_node *pop = cxt->parent_stack.top;
	cxt->parent_stack.top = cxt->parent_stack.top->next;
	
	ui_free_parent_node(cxt, pop);
}

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
		widget = push_struct(cxt->arena, UI_Widget);
		widget->hash = text_hash;
		widget->text.c = push_array(cxt->arena, u8, text.len);
		widget->text.len = text.len;
		str8_cpy(&widget->text, &text);
		u64 slot = text_hash % cxt->hash_table_size;
		cxt->hash_slots[slot].first = widget;
		widget->id = cxt->num++;
		
		// temp. needs to move out of here. chain links are meant to be remade every frame
		// before it overflowed because parent's old chains persisted and kept ykwim
		
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
	}
	
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
	widget->pref_size[Axis2_X].value = cxt->pref_width_stack.top->v;
	widget->pref_size[Axis2_Y].value = cxt->pref_height_stack.top->v;
	
	//widget->computed_rel_position[Axis2_X] = cxt->fixed_pos_stack->pos.x;
	//widget->computed_rel_position[Axis2_Y] = cxt->fixed_pos_stack->pos.y;
	
	widget->fixed_position = cxt->fixed_pos_stack.top->v;
	
	if(cxt->child_layout_axis_stack.auto_pop)
	{
		widget->child_layout_axis = cxt->child_layout_axis_stack.top->v;
		cxt->child_layout_axis_stack.auto_pop = 0;
		ui_pop_child_layout_axis(cxt);
	}
	return widget;
}

internal void ui_begin_rowf(UI_Context *cxt, char *fmt, ...)
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
}

internal void ui_end_row(UI_Context *cxt)
{
	ui_pop_parent(cxt);
}

internal void ui_begin_colf(UI_Context *cxt, char *fmt, ...)
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

#define ui_rowf(v,...) UI_DeferLoop(ui_begin_rowf(v,__VA_ARGS__), ui_end_row(v))
#define ui_colf(v,...) UI_DeferLoop(ui_begin_colf(v,__VA_ARGS__), ui_end_col(v))


#endif //UI_H
