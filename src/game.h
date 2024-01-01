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

struct rectangle
{
	v2f Min;
	v2f Max;
};

struct player_polygon
{
	v2f Vertices[7];
};

struct projected_interval
{
	f32 min;
	f32 max;
};

struct bounding_box
{
	v2f Min;
	v2f Max;
};

struct square
{
	v2f Center;
	f32 half_width;
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

struct projectile_trail
{
	v2f Pos;
	f32 time_left;
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
	ENTITY_SQUARE,
};

// NOTE(Justin): We can use u32 for the enums so that we can do bitwise
// operations on the flags. Otherwise we cannot do bitwise operations.

enum entity_flags
{
	ENTITY_FLAG_COLLIDES = (1 << 1),
	ENTITY_FLAG_NON_SPATIAL = (1 << 2),
};

struct hit_point
{
	u8 flags;
	u8 count;
};

enum shape_type
{
	SHAPE_TRIANGLE,
	SHAPE_CIRCLE,
	SHAPE_POLY,

	SHAPE_COUNT
};

struct move_spec
{
	b32 use_normalized_accel;
	f32 speed;
	f32 angular_speed;
	f32 mass;
};


struct entity
{
	u32 index;

	entity_type type;
	shape_type shape;
	u32 flags;

	b32 exists;
	//b32 collides;
	b32 is_shooting;
	b32 is_warping;
	b32 is_shielded;

	f32 height;
	f32 base_half_width;

	v2f Pos;
	v2f dPos;
	v2f Right;
	v2f Direction;


	f32 speed;
	f32 mass;
	f32 radius;
	f32 distance_remaining;

	u8 hit_point_max;
	hit_point HitPoints;

	player_polygon Poly;
};

struct world
{
	v2f Dim;
};

struct game_state
{
	// TODO(Justin): Should we break up some bitmaps for VFX purposes?

	f32 pixels_per_meter;

	memory_arena WorldArena;
	world *World;

	loaded_sound TestSound;
	u32 test_sample_index;

	loaded_bitmap Background;

	// TODO(Justin): Maybe use gimp to create partition the ship bitmap 
	// into different bitmaps for animation, visual effects purposes. Left
	// engine, right engine, etc..
	loaded_bitmap Ship;

	loaded_bitmap WarpFrames[8];
	u32 warp_frame_index;

	loaded_bitmap AsteroidBitmap;
	loaded_bitmap LaserBlueBitmap;

	f32 time_between_new_projectiles;
	u32 entity_count;
	entity Entities[256];

	u32 player_entity_index;

	b32 is_initialized;

	u32 square_count;
	square Squares[10];
};

#define GAME_H
#endif
