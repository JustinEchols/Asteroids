
// NOTE(Justin):
// Header file that interfaces between platform and game layers.
// Contains services that the game provides to the platform layer.

#if !defined(GAME_PLATFORM_H)

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE(Justin): Compilers
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

//
// NOTE(Justin): Types
//

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

typedef int8_t		s8;
typedef int16_t 	s16;
typedef int32_t 	s32;
typedef int64_t 	s64;
typedef s32			b32;

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef size_t		memory_index;

typedef float		f32;
typedef double		f64;

#define internal		static
#define local_persist	static
#define global_variable static

#define PI32 3.1415926535897f
#define ONE_OVER_ROOT_TWO 0.707106781188f

#if GAME_SLOW
#define ASSERT(expression) if (!(expression)) {*(int *)0 = 0;}
#else
#define ASSERT(expression)
#endif

#define INVALID_CODE_PATH ASSERT(!"Invaid code path")
#define INVALID_DEFAULT_CASE default: {INVALID_CODE_PATH;} break

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define KILOBYTES(kilobyte_count) (1024LL * kilobyte_count)
#define MEGABYTES(megabyte_count) (1024LL * KILOBYTES(megabyte_count))
#define GIGABYTES(gigabyte_count) (1024LL * MEGABYTES(gigabyte_count))
#define TERABYTES(terabyte_count) (1024LL * GIGABYTES(terabyte_count))

#define ABS(x) (((x) > 0) ? (x) : -(x))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))

typedef struct
{
	int place_holder;
} thread_context;

#if GAME_INTERNAL

typedef struct
{
	u32 size;
	void *contents;
} debug_file_read_result;

#define DEBUG_PLATFORM_FILE_FREE_MEMORY(name) void name(thread_context *Thread, void *memory)
typedef DEBUG_PLATFORM_FILE_FREE_MEMORY(DebugPlatformFileFreeMemory);

#define DEBUG_PLATFORM_FILE_READ_ENTIRE(name) debug_file_read_result name(thread_context *Thread, char *filename)
typedef DEBUG_PLATFORM_FILE_READ_ENTIRE(DebugPlatformFileReadEntire);

#define DEBUG_PLATFORM_FILE_WRITE_ENTIRE(name) b32 name(thread_context *Thread, char *filename, u32 memory_size, void *memory)
typedef DEBUG_PLATFORM_FILE_WRITE_ENTIRE(DebugPlatformFileWriteEntire);

#endif

#define BITMAP_BYTES_PER_PIXEL 4
typedef struct
{
	void *memory;
	int width;
	int height;
	int stride;

} back_buffer;

typedef struct
{
	int sample_count;
	int samples_per_second;
	s16 *samples;
} sound_buffer;


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

typedef struct
{
	game_button_state MouseButtons[5];
	s32 mouse_x, mouse_y;

	game_controller_input Controller;
	f32 dt_for_frame;
} game_input;

typedef struct
{
	b32 is_initialized;

	void *permanent_storage;
	u64 permanent_storage_size;

	void *transient_storage;
	u64 transient_storage_size;

	DebugPlatformFileFreeMemory *debug_platform_file_free_memory;
	DebugPlatformFileReadEntire *debug_platform_file_read_entire;
	DebugPlatformFileWriteEntire *debug_platform_file_write_entire;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *GameMemory, back_buffer *BackBuffer, game_input *GameInput)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_SOUND_SAMPLES_GET(name) void name(thread_context *Thread, game_memory *GameMemory, sound_buffer *SoundBuffer)
typedef GAME_SOUND_SAMPLES_GET(game_get_sound_samples);

#ifdef __cplusplus
}
#endif

#define GAME_PLATFORM_H
#endif
