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
	ENTITY_PROJECTILE,
	ENTITY_FAMILIAR
};

struct hit_point
{
	u8 flags;
	u8 count;
};

struct entity
{
	u32 index;

	b32 exists;
	b32 collides;

	f32 height;
	f32 base_half_width;
	v2f Right;
	v2f Direction;
	v2f dPos;
	f32 speed;
	b32 is_shooting;
	b32 is_warping;
	b32 is_shielded;

	u8 hit_point_max;
	hit_point HitPoints;

	tile_map_position TileMapPos;

	entity_type type;
};

struct entity_visible_piece
{
	loaded_bitmap *Bitmap;
	v2f Offset;
	f32 r, g, b, alpha;
};

struct entity_visible_piece_group
{
	u32 piece_count;
	entity_visible_piece Pieces[8];
};

struct game_state
{
	// TODO(Justin): Should we break up some bitmaps for VFX purposes?
	loaded_bitmap Background;
	loaded_bitmap Ship;
	loaded_bitmap WarpFrames[8];
	loaded_bitmap AsteroidSprite;
	loaded_bitmap LaserBlue;

	u32 warp_frame_index;

	memory_arena TileMapArena;
	memory_arena AsteroidsArena;

	tile_map *TileMap;

	projectile Projectiles[64];
	u32 projectile_next;

	f32 projectile_speed;
	f32 projectile_half_width;
	f32 time_between_new_projectiles;

	u32 asteroid_count;
	u32 entity_count;
	entity Entities[256];

	u32 player_entity_index;

	loaded_sound TestSound;
	u32 test_sample_index;

	b32 is_initialized;
};

#define GAME_H
#endif
