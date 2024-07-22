/* date = July 22nd 2024 4:41 pm */

#ifndef BASE_TCXT_H
#define BASE_TCXT_H


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
	debug_cycle_counter counters_last[DEBUG_CYCLE_COUNTER_COUNT];
	
};

global TCXT tcxt;

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
		debug_cycle_counter *counter_last = tcxt.counters_last + i;
		
		counter_last->hit_count = counter->hit_count;
		counter_last->cycle_count = counter->cycle_count;
		
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


#endif //BASE_TCXT_H
