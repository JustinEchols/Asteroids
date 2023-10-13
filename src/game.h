#if !defined(GAME_H)

#include "game_platform.h"


// WAV PCM is 16 bits per sample, either one or two channels, and samples are
// interleaved (rlrlrlrl).
struct string_u8
{
	u8 *data;
	u64 length;
};

typedef size_t memory_index;

struct memory_arena
{
	memory_index size;
	u8 *base;
	memory_index used;
};

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
#include "game_tile.h"

#include "game_asset.h"

struct color
{
	char *name;
	v3f Value;
};

struct bounding_box
{
	v2f Min;
	v2f Max;
};

struct circle
{
	v2f Center;
	f32 radius;

};



enum 
{
	PLAYER_LEFT,

	PLAYER_RIGHT,
	PLAYER_TOP,
	PLAYER_BEZIER_TOP,

	PLAYER_VERTEX_COUNT
};



struct player
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
	b32 is_shielded;

	bounding_box BoundingBox;
	circle Shield;

	tile_map_position TileMapPos;
};

typedef enum
{
	ASTEROID_SMALL,
	ASTEROID_MEDIUM,
	ASTEROID_LARGE,
 
	ASTEROID_SIZE_COUNT

} asteroid_size;

struct asteroid
{
	v2f Pos;
	tile_map_position TileMapPos;
	v2f Direction;
	f32 speed;
	v2f dPos;
	v2f LocalVertices[6];
	bounding_box BoundingBox;

	f32 mass;
	asteroid_size size;
	b32 hit;
	b32 is_active;
};

struct projectile_trail
{
	tile_map_position TileMapPos;
	f32 time_left;
};

struct projectile
{
	tile_map_position TileMapPos;
	v2f Direction;
	v2f dPos;

	f32 distance_remaining;
	b32 is_active;


	projectile_trail Trails[4];
	u32 trail_next;
	f32 time_between_next_trail;
};

struct entity
{
	b32 exists;
	tile_map_position TileMapPos;
	v2f dPos;
};


struct game_state
{
	loaded_bitmap Background;
	loaded_bitmap Ship;
	loaded_bitmap WarpFrames[8];
	loaded_bitmap AsteroidSprite;
	loaded_bitmap LaserBlue;

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
};

#define GAME_H
#endif
