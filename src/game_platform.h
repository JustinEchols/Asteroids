// Header file that interfaces between win32 and game layers.
// Contains services that the game provides to the platform layer.
#if !defined(GAME_PLATFORM_H)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int8_t		s8;
typedef int16_t 	s16;
typedef int32_t 	s32;
typedef int64_t 	s64;
typedef s32			b32;

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef float		f32;

#if GAME_SLOW
#define ASSERT(expression) if (!(expression)) {*(int *)0 = 0;}
#else
#define ASSERT(expression)
#endif

#define INVALID_CODE_PATH ASSERT(!"Invaid code path")

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define KILOBYTES(kilobyte_count) 1024 * kilobyte_count
#define MEGABYTES(megabyte_count) 1024 * KILOBYTES(megabyte_count)
#define GIGABYTES(gigabyte_count) 1024 * MEGABYTES(gigabyte_count)
#define TERABYTES(terabyte_count) 1024 * GIGABYTES(terabyte_count)

#define HALF_ROOT_TWO 0.707106780f
#define PI32 3.1415926535897f

#define internal		static
#define local_persist	static
#define global_variable static

#define TRUE 1
#define FALSE 0

#define ABS(x) (((x) > 0) ? (x) : -(x))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))

typedef struct
{
	void *contents;
	u32 size;
} debug_file_read;

internal void platform_file_free_memory(void *file_memory);
internal debug_file_read platform_file_read_entire(char *filename);

typedef struct
{
	int sample_count;
	int samples_per_second;
	s16 *samples;
} sound_buffer;

typedef struct
{
	void *memory;
	int width;
	int height;
	int bytes_per_pixel;
	int stride;

} back_buffer;


typedef struct
{
	u32 half_transition_count;
	b32 ended_down;
} game_button_state;

typedef struct
{
	union
	{
		game_button_state Buttons[11];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state Space;
			game_button_state Enter;
			game_button_state Shift;
			game_button_state ArrowUp;
			game_button_state ArrowDown;
			game_button_state ArrowLeft;
			game_button_state ArrowRight;
		};
	};
} game_controller_input;

// TODO(Justin): Enum for mouse buttons
typedef struct
{
	game_button_state MouseButtons[5];
	s32 mouse_x, mouse_y;

	game_controller_input Controller;
	f32 dt_for_frame;
} game_input;

typedef struct
{
	u64 total_size;
	u64 permanent_storage_size;
	u64 transient_storage_size;
	b32 is_initialized;
	void *permanent_storage;
	void *transient_storage;

} game_memory;

#ifdef __cplusplus
}
#endif
#define GAME_PLATFORM_H
#endif
