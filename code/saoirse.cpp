#include "s_platform.h"
#include "saoirse.h"

extern "C"
{
	YK_API void update_and_render(S_Platform *);
}

void update_and_render(S_Platform *)
{
	printf("hello");
}