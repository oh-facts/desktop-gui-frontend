/* date = July 18th 2024 5:41 am */

#ifndef S_PLATFORM_H
#define S_PLATFORM_H

#define S_VERSION_MAJOR (0)
#define S_VERSION_MINOR (1)
#define S_VERSION_PATCH (0)

#include "stdio.h"
#include "string.h"
#include "base_core.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#include "base_string.h"
#include "base_string.cpp"

typedef void *(*os_reserve_fn)(u64 size);
typedef b32 (*os_commit_fn)(void *ptr, u64 size);
typedef void (*os_decommit_fn)(void *ptr, u64 size);
typedef void (*os_release_fn)(void *ptr, u64 size);
typedef u64 (*os_get_page_size_fn)();
typedef Str8 (*os_get_app_dir_fn)(Arena *arena);

global os_reserve_fn os_reserve;
global os_commit_fn os_commit;
global os_decommit_fn os_decommit;
global os_release_fn os_release;
global os_get_page_size_fn os_get_page_size;
global os_get_app_dir_fn os_get_app_dir;

struct S_Platform_api
{
	os_reserve_fn os_reserve;
	os_commit_fn os_commit;
	os_decommit_fn os_decommit;
	os_release_fn os_release;
	os_get_page_size_fn os_get_page_size;
	os_get_app_dir_fn os_get_app_dir;
};

internal void s_global_platform_api_init(S_Platform_api *api)
{
	os_reserve = api->os_reserve;
	os_commit = api->os_commit;
	os_decommit = api->os_decommit;
	os_release = api->os_release;
	os_get_page_size = api->os_get_page_size;
	os_get_app_dir =  api->os_get_app_dir;
}

#include "base_core.cpp"


#include <stdlib.h>

enum DEBUG_CYCLE_COUNTER
{
	DEBUG_CYCLE_COUNTER_UPDATE_AND_RENDER,
	DEBUG_CYCLE_COUNTER_RECORD,
	DEBUG_CYCLE_COUNTER_SUBMIT,
	DEBUG_CYCLE_COUNTER_COUNT
};

global char *debug_cycle_to_str[DEBUG_CYCLE_COUNTER_COUNT] = 
{
	"update and render",
	"record",
	"submit",
};

struct debug_cycle_counter
{
	u64 cycle_count;
	u32 hit_count;
};

struct TCXT
{
	Arena *arenas[2];
	debug_cycle_counter counters[DEBUG_CYCLE_COUNTER_COUNT];
};

global TCXT tcxt;

struct S_Platform
{
	int argc;
	char **argv;
	Str8 app_dir;
	S_Platform_api api;
	b32 initialized;
	b32 reloaded;
	u64 res;
	u64 cmt;
	void *memory;
};

// ty yeti
#if 0
internal Str8 os_linux_get_app_dir(Arena *arena)
{
	char buffer[256];
  ssize_t len = readlink("/proc/self/exe", buffer, 256);
	
	char *c = &buffer[len];
  while(*(--c) != '/')
  {
    *c = 0;
    --len;
  }
	
  u8 *str = push_array(arena, u8, len);
	mem_cpy(str, buffer, len);
	
	Str8 out = str8(str, len);
	
	return out;
}
#endif

#if defined (OS_WIN32)
#define BEGIN_TIMED_BLOCK(ID) u64 start_cycle_count_##ID = __rdtsc(); ++tcxt.counters[DEBUG_CYCLE_COUNTER_##ID].hit_count
#define END_TIMED_BLOCK(ID)  tcxt.counters[DEBUG_CYCLE_COUNTER_##ID].cycle_count += __rdtsc() - start_cycle_count_##ID
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#endif

internal void tcxt_init()
{
	for(u32 i = 0; i < ARRAY_LEN(tcxt.arenas); i ++)
	{
		tcxt.arenas[i] = arena_create(Megabytes(10), Megabytes(64));
	}
}

internal void process_debug_counters()
{
	for(i32 i = 0; i < ARRAY_LEN(tcxt.counters); i ++)
	{
		debug_cycle_counter *counter = tcxt.counters + i;
		
		//printf("%d: %lu\n", i, counter->cycle_count);
		counter->hit_count = 0;
		counter->cycle_count = 0;
	}
}

internal Arena *tcxt_get_scratch(Arena **conflicts, u64 count)
{
	Arena *out = 0;
	for(u32 i = 0; i < ARRAY_LEN(tcxt.arenas); i ++)
	{
		b32 has_conflict = 0;
		for(u32 j = 0; j < count; j ++)
		{
			if(tcxt.arenas[i] == conflicts[j])
			{
				has_conflict = 1;
				break;
			}
		}
		if(!has_conflict)
		{
			out = tcxt.arenas[i];
		}
	}
	
	return out;
}

#define scratch_begin(conflicts, count) arena_temp_begin(tcxt_get_scratch(conflicts, count))
#define scratch_end(scratch) arena_temp_end(scratch);

internal char *file_name_from_path(Arena *arena, Str8 path)
{
	char *cur = (char*)&path.c[path.len - 1];
	u32 count = 0;
	
	//NOTE(mizu): pig
	while(*cur != '/' && *cur != '\0')
	{
		cur--;
		count++;
	}
	
	char *file_name_cstr = push_array(arena, char, count + 1);
	mem_cpy(file_name_cstr, cur + 1, count);
	file_name_cstr[count] = '\0';
	
	return file_name_cstr;
}

typedef void (*update_and_render_fn)(S_Platform *);

enum FILE_TYPE
{
  FILE_TYPE_TEXT,
  FILE_TYPE_BINARY,
  FILE_TYPE_COUNT
};

// ty pine
#if defined(OS_WIN32)
#define _file_open(file, filepath, mode) fopen_s(file, filepath, mode)
#elif defined (OS_LINUX) || defined (OS_APPLE)
#define _file_open(file, filepath, mode) *file = fopen(filepath, mode)
#endif

internal u8 *read_file(Arena *arena, const char *filepath, FILE_TYPE type)
{
  FILE *file;
  
  local_persist char *file_type_table[FILE_TYPE_COUNT] = 
  {
    "r",
    "rb"
  };
#if 0
	if (access(filepath, F_OK) != 0)
	{
    file = fopen(filepath, "wb+");
    
    fclose(file);
  }
#endif
  _file_open(&file, filepath, file_type_table[type]);
  
  fseek(file, 0, SEEK_END);
  
  i32 len = ftell(file);
  //print("%d", len);
  
  fseek(file, 0, SEEK_SET);
  
  u8 *buffer = push_array(arena, u8, len);
  fread(buffer, sizeof(u8), len, file);
  
  fclose(file);
  
  return buffer;
}

internal void write_file(const char *filepath, FILE_TYPE type, void *data, size_t size)
{
	FILE *file;
	
	local_persist char *file_type_table[FILE_TYPE_COUNT] = 
  {
    "w",
    "wb"
  };
	
	_file_open(&file, filepath, file_type_table[type]);
	
	fwrite(data, size, 1, file);
	
	fclose(file);
	
}

#endif //S_PLATFORM_H
