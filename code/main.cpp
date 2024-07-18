#include "s_platform.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

global u64 total_cmt;
global u64 total_res;

internal void *os_win32_reserve(u64 size)
{
	void *out = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
	if (out != NULL)
	{
		total_res += size;
	}
	return out;
}

internal b32 os_win32_commit(void *ptr, u64 size)
{
	if (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) == NULL)
	{
		printf("VirtualAlloc commit failed: %lu\r\n", GetLastError());
		return 0;
	}
	total_cmt += size;
	return 1;
}

internal void os_win32_decommit(void *ptr, u64 size)
{
	VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal void os_win32_release(void *ptr, u64 size)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}

internal u64 os_win32_get_page_size()
{
	SYSTEM_INFO sysinfo = {};
	GetSystemInfo(&sysinfo);
	return sysinfo.dwPageSize;
}

Str8 os_win32_get_app_dir(Arena *arena)
{
	char buffer[256];
	DWORD len = GetModuleFileName(0, buffer, 256);
	
	char *c = &buffer[len];
  while(*(--c) != '\\')
  {
    *c = 0;
    --len;
  }
  
	u8 *str = push_array(arena, u8, len);
	mem_cpy(str, buffer, len);
	
	Str8 out = str8(str, len);
	
	return out;
}

int main(int argc, char **argv)
{
	
	S_Platform pf = {};
	pf.argc = argc;
	pf.argv = argv;
	
	pf.api.os_reserve = os_win32_reserve;
	pf.api.os_commit = os_win32_commit;
	pf.api.os_decommit = os_win32_decommit;
	pf.api.os_release = os_win32_release;
	pf.api.os_get_page_size = os_win32_get_page_size;
	pf.api.os_get_app_dir = os_win32_get_app_dir;
	
	s_global_platform_api_init(&pf.api);
	
	Arena *arena = arena_create();
	
	Str8 app_dir = os_get_app_dir(arena);
	
	pf.app_dir = app_dir;
	
	Str8 dll_rel_path = str8_lit("yk.dll");
	Str8 dll_path = str8_join(arena, app_dir, dll_rel_path);
	HMODULE game_dll = LoadLibrary((char *)dll_path.c);
	
	if(!game_dll)
	{
		printf("dll not found\n\r");
	}
	
	update_and_render_fn update_and_render = (update_and_render_fn)GetProcAddress(game_dll, "update_and_render");
	
	if(!update_and_render)
	{
		printf("fn not found\n\r");
	}
	
	while(1)
	{
		update_and_render(&pf);
	}
	
	
	return 0;
}