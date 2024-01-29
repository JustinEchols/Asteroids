/*
  TODO:
Collision Detection
	- Asteroid collisions
 	- Projectile collisions
	- Better collision table
	- Free collision rules
 
Physics
    - Angular velocity
    - Mass
    - Asteroid acceleration?
   
 Asset loading

VfX
  	- Bitmap transformations (rotations, scaling, ...)
  	- UV coordinate mapping
  	- Normal mapping

	- Animations
		- Lasers/beams
		- Warping
		- Shield
			- Appears on collision
			- Fades shortly thereafter
		- Ship phasing
			- Ship can enter a "flux state" and temporarily phase through
			objects avoiding collisions
	- Particles
		- Ship thrusters
		- Energy beam
		- Asteroids destruction (split asteroids, asteroid pariticles)


Basics of DSound

	2-channels (stereo)
	stereo L/R channel two channels 8 bits per channel
	Block is a Sample which has two channels [L R]
	the member is in units of bytes so byte_count([L R]) = 2 channels * 16 bits per channel / 8 = 4 
	(# samples * sizeof(sample)) / seconds = bytes / second

	DSound setup
		- Load dsound.dll							
		- Get function ptr to DirectSoundCreate
		- DirectSoundCreate
		- SetCooperativeLevel
		- SetFormat of the PrimaryBuffer
		- Create GlobaalSecondaryBuffer

	Loading a WAV file into a DSound buffer
		- Read the header
		- If WAV file small create a secondary buffer large enought to hold sample
		- data. If WAV file large stream data to the buffer.
		- Fill buffer with data using Lock
		- If not streaming lock entire buffer otherwise
		- After copying unlock

- Audio/SFX
	- Audio mixer
  	- Score
  	- Menu
	- Play any audio
 	- Play any audio file
 	- open wav
 	- decode
 	- uncompress
 	- pan audio?
 	- change pitch 

Optimization pass
	- Threading
 	- Profiling
 	- SIMD
 	- Intrinsics

Game Design
	- Enemies?
	- Destorying an asteoid spawns alien
	- Alien behavior adheres to the rules of the game of life
	- Include a weighting so that the alien movement is biased towards the player.

*/

#if !defined(GAME_H)

#include "game_platform.h"

struct memory_arena
{
	memory_index size;
	u8 *base;
	memory_index used;

	s32 temp_count;
};

struct temporary_memory
{
	memory_arena *MemoryArena;
	memory_index used;
};

internal void
memory_arena_initialize(memory_arena *MemoryArena, memory_index arena_size, u8 *base)
{
	MemoryArena->size = arena_size;
	MemoryArena->base = base;
	MemoryArena->used = 0;
	MemoryArena->temp_count = 0;
}

#define push_struct(MemoryArena, type) (type *)push_size_(MemoryArena, sizeof(type))
#define push_array(MemoryArena, count, type) (type *)push_size_(MemoryArena, (count) * sizeof(type))
#define push_size(MemoryArena, size) push_size_(MemoryArena, size)
void *
push_size_(memory_arena *MemoryArena, memory_index size)
{
	ASSERT((MemoryArena->used + size) <= MemoryArena->size);
	void *Result = MemoryArena->base + MemoryArena->used;
	MemoryArena->used += size;
	return(Result);
}

inline temporary_memory
temporary_memory_begin(memory_arena *MemoryArena)
{
	temporary_memory Result;

	Result.MemoryArena = MemoryArena;
	Result.used = MemoryArena->used;

	MemoryArena->temp_count++;

	return(Result);
}

inline void
temporary_memory_end(temporary_memory TempMemory)
{
	memory_arena *MemoryArena = TempMemory.MemoryArena;
	ASSERT(MemoryArena->used >= TempMemory.used);
	MemoryArena->used = TempMemory.used;
	ASSERT(MemoryArena->temp_count > 0);
	MemoryArena->temp_count--;
}

inline void
memory_arena_check(memory_arena *Arena)
{
	ASSERT(Arena->temp_count == 0);
}

#define zero_struct(instance) zero_size(&(instance), sizeof(instance))
inline void
zero_size(void *ptr, memory_index size)
{
	u8 *byte = (u8*)ptr;
	while(size--)
	{
		*byte++ = 0;
	}
}

#include "game_intrinsics.h"
#include "game_math.h"
#include "game_geometry.h"
#include "game_render_group.h"
#include "game_sim_region.h"
#include "game_entity.h"
#include "game_asset.h"
#include "game_string.h"

#define White V3F(1.0f, 1.0f, 1.0f)
#define Red V3F(1.0f, 0.0f, 0.0f)
#define Green V3F(0.0f, 1.0f, 0.0f)
#define Blue V3F(0.0f, 0.0f, 1.0f)
#define Black V3F(0.0f, 0.0f, 0.0f)
#define Gray(c) V4F(c, c, c, 1.0f)

enum asteroid_size
{
	ASTEROID_SMALL,
	ASTEROID_MEDIUM,
	ASTEROID_LARGE,
 
	ASTEROID_SIZE_COUNT
};

// NOTE(Justin): We can use u32 for the enums so that we can do bitwise
// operations on the flags. Otherwise we cannot do bitwise operations.


struct world
{
	v2f Dim;
};

struct pairwise_collision_rule
{
	b32 should_collide;
	u32 index_a;
	u32 index_b;

	pairwise_collision_rule *NextInHash;
};

struct game_state
{
	// TODO(Justin): Should we break up some bitmaps for VFX purposes?
	
	// TODO(Justin): Maybe use gimp to create partition of the ship bitmap 
	// into different bitmaps for animation, visual effects purposes. Left
	// engine, right engine, etc..

	f32 pixels_per_meter;

	memory_arena WorldArena;
	world *World;

	loaded_sound TestSound;
	u32 test_sample_index;

	v2f ScreenCenter;

	loaded_bitmap Background;

	loaded_bitmap Ship;
	loaded_bitmap ShipNormalMap;



	loaded_bitmap WarpFrames[8];
	u32 warp_frame_index;

	loaded_bitmap AsteroidBitmap;
	loaded_bitmap LaserBlueBitmap;

	f32 time_between_new_projectiles;
	u32 entity_count;
	entity Entities[256];

	u32 player_entity_index;

	m3x3 MapToScreenSpace;

	// NOTE(Justin): Muse be power of two.
	pairwise_collision_rule *CollisionRuleHash[256];

	f32 time;

	b32 is_initialized;

	loaded_bitmap TestBuffer;

	loaded_bitmap TestTree;
	loaded_bitmap TreeNormalMap;
	loaded_bitmap TestBackground;

	loaded_bitmap TestDiffuse;
	loaded_bitmap TestNormal;
	
};

struct transient_state
{
	b32 is_initialized;

	memory_arena TransientArena;

	u32 env_map_width;
	u32 env_map_height;
	environment_map EnvMaps[3];
};

#define GAME_H
#endif
