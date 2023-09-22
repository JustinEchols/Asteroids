// Header file that interfaces between win32 and game layers.
// Services that the game provides to the platform layer...
#if !defined(GAME_PLATFORM_H)

#include <stdint.h>

typedef struct
{
	int width;
	int height;
	int bytes_per_pixel;
	int stride;
	void *memory;

} game_back_buffer;

#define GAME_PLATFORM_H
#endif
