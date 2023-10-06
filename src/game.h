#if !defined(GAME_H)

#include "game_platform.h"


// WAV PCM is 16 bits per sample, either one or two channels, and samples are
// interleaved (rlrlrlrl).
typedef struct 
{
	u8 *data;
	u64 length;
} string_u8;

typedef size_t memory_index;

typedef struct 
{
	memory_index size;
	u8 *base;
	memory_index used;
} memory_arena;

internal void
memory_arena_initialize(memory_arena *MemoryArena, memory_index arena_size, u8 *base)
{
	MemoryArena->size = arena_size;
	MemoryArena->base = base;
	MemoryArena->used = 0;
}

#define push_size(MemoryArena, type) (type *)push_size_(MemoryArena, sizeof(type))
#define push_array(MemoryArena, count, type) (type *)push_size_(MemoryArena, (count) * sizeof(type))
void *
push_size_(memory_arena *MemoryArena, memory_index size)
{
	ASSERT((MemoryArena->used + size) <= MemoryArena->size);
	void *Result = MemoryArena->base + MemoryArena->used;
	MemoryArena->used += size;
	return(Result);
}

#include "game_intrinsics.h"
#include "game_math.h"
#include "game_asset.h"

typedef struct
{
	char *name;
	v3f Value;
} color;

typedef struct 
{
	v2f Min;
	v2f Max;
} bounding_box;

typedef struct
{
	v2f Center;
	f32 radius;

} circle;

typedef enum
{
	ASTEROID_SMALL,
	ASTEROID_MEDIUM,
	ASTEROID_LARGE,
 
	ASTEROID_SIZE_COUNT

} asteroid_size;

typedef struct
{
	v2f Pos;
	v2f Direction;
	f32 speed;
	v2f dPos;
	v2f LocalVertices[6];
	bounding_box BoundingBox;

	f32 mass;
	asteroid_size size;
	b32 hit;
} asteroid;

enum 
{
	PLAYER_LEFT,

	PLAYER_RIGHT,
	PLAYER_TOP,
	PLAYER_BEZIER_TOP,

	PLAYER_VERTEX_COUNT
};

typedef struct
{
	v2i Tile;
	v2f TileOffset;
} tile_map_position;

typedef struct
{
	f32 height;
	f32 base_half_width;
	v2f Vertices[4];
	v2f Right;
	v2f Direction;
	v2f dPos;
	f32 speed;
	b32 is_shooting;
	b32 is_warping;

	bounding_box BoundingBox;
	circle Shield;

	tile_map_position TileMapPos;
} player;

typedef struct
{
	tile_map_position TileMapPos;
	f32 time_left;
} projectile_trail;

typedef struct
{
	tile_map_position TileMapPos;
	v2f Direction;
	v2f dPos;

	f32 distance_remaining;
	b32 is_active;


	projectile_trail Trails[4];
	u32 trail_next;
	f32 time_between_next_trail;
} projectile;


typedef struct
{
	s32 tile_count_x;
	s32 tile_count_y;
	s32 tile_side_in_pixels;
	f32 tile_side_in_meters;
	f32 meters_to_pixels;


	u32* tiles;
} tile_map;

typedef struct
{
	f32 tile_side_in_meters;
} world;

typedef struct
{
	loaded_bitmap Background;
	loaded_bitmap Ship;
	loaded_bitmap WarpFrames[8];
	loaded_bitmap Test;

	u32 warp_frame_index;
	f32 pixels_per_meter;
	v2f WorldHalfDim;

	memory_arena TileMapArena;
	tile_map *TileMap;


	player Player;

	projectile Projectiles[64];
	u32 projectile_next;

	f32 projectile_speed;
	f32 projectile_half_width;
	f32 time_between_new_projectiles;

	u32 asteroid_count;
	asteroid Asteroids[16];

	loaded_sound TestSound;
	u32 test_sample_index;

	b32 is_initialized;
} game_state;

#define GAME_H
#endif
