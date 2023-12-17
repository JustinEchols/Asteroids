#if !defined(GAME_H)

#include "game_platform.h"


// WAV PCM is 16 bits per sample, either one or two channels, and samples are
// interleaved (rlrlrlrl).


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


#include "game_string.h"
#include "game_intrinsics.h"
#include "game_math.h"
#include "game_tile.h"

#include "game_asset.h"

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

struct triangle
{
	v2f Vertices[3];
	v2f Centroid;
};

enum asteroid_size
{
	ASTEROID_SMALL,
	ASTEROID_MEDIUM,
	ASTEROID_LARGE,
 
	ASTEROID_SIZE_COUNT
};

struct asteroid
{
	tile_map_position TileMapPos;
	v2f Direction;
	f32 speed;
	v2f dPos;

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

enum entity_type
{
	ENTITY_NULL,
	ENTITY_PLAYER,
	ENTITY_ASTEROID,
	ENTITY_FAMILIAR,
	ENTITY_PROJECTILE,
	ENTITY_TRIANGLE,
	ENTITY_CIRCLE,

	ENTITY_NOT_USED
};

struct hit_point
{
	u8 flags;
	u8 count;
};

enum shape_type
{
	SHAPE_TRIANGLE,
	SHAPE_CIRCLE
};


struct entity
{
	u32 index;

	b32 exists;
	b32 collides;
	b32 is_shooting;
	b32 is_warping;
	b32 is_shielded;

	f32 height;
	f32 base_half_width;
	v2f Right;
	v2f Direction;
	v2f dPos;
	f32 speed;
	f32 distance_remaining;


	u8 hit_point_max;
	hit_point HitPoints;

	v2f Pos;

	entity_type type;
	shape_type shape;

	circle CircleHitBox;
	triangle TriangleHitBox;

	f32 radius;
};

struct world
{
	f32 meters_to_pixels;
	f32 width;
	f32 height;
};

struct game_state
{
	// TODO(Justin): Should we break up some bitmaps for VFX purposes?

	memory_arena WorldArena;
	world *World;

	loaded_bitmap Background;
	loaded_bitmap Ship;
	loaded_bitmap WarpFrames[8];
	loaded_bitmap AsteroidSprite;
	loaded_bitmap LaserBlue;

	u32 warp_frame_index;

	memory_arena TileMapArena;
	memory_arena AsteroidsArena;

	tile_map *TileMap;

	f32 time_between_new_projectiles;

	u32 asteroid_count;

	u32 entity_count;
	entity Entities[256];

	u32 player_entity_index;

	loaded_sound TestSound;
	u32 test_sample_index;

	triangle Triangle;
	circle CircleA;
	circle CircleB;

	b32 is_initialized;
};

#define GAME_H
#endif
