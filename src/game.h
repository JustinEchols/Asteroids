#if !defined(GAME_H)


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



typedef struct
{
	f32 e[3][3];
} m3x3;

typedef struct 
{
	s32 x, y;
} v2i;

typedef struct
{
	u32 x, y;
} v2u;

typedef struct
{
	f32 e[2][2];
} m2x2;

typedef union
{
	struct
	{
		f32 x, y;
	};
	struct
	{
		f32 u, v;
	};
	f32 e[2];
} v2f;

typedef union
{
	struct
	{
		f32 x, y, z;
	};
	struct
	{
		f32 u, v, w;
	};
	f32 e[3];
} v3f;

typedef struct
{
	char *name;
	v3f ColorValue;
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

typedef struct
{
	s16 *samples;
	int sample_count;
	int samples_per_second;

} sound_buffer;

typedef struct
{
	void *memory;
	int width;
	int height;
	int bytes_per_pixel;
	int stride;

} back_buffer;

enum 
{
	KEY_W,
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_SPACE,

	KEY_COUNT
};



typedef struct
{
	u32 half_transition_count;
	b32 ended_down;
} game_button_state;

typedef struct
{
	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state Space;
			game_button_state ArrowUp;
			game_button_state ArrowDown;
		};
	};
} game_controller_input;



typedef struct
{
	game_controller_input Controller;
	f32 dt_for_frame;
} game_input;

enum
{
	WU_BLACK,
	WU_WHITE,
	WU_RED,
	WU_GREEN,
	WU_BLUE,
	WU_YELLOW,
	WU_PURPLE,

	WU_COLOR_COUNT
};

typedef struct
{
	u32 base_color;
	s32 num_levels;
	u32 intensity_bits;
	u32 red_mask;
	u32 green_mask;
	u32 blue_mask;
} color_wu;

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
	v2i TilePos;
	v2f TileRelativePos;
} tile_map_position;


typedef struct
{
	v2f Pos;
	f32 height;
	f32 base_half_width;
	v2f Vertices[4];
	v2f Right;
	v2f Direction;
	v2f dPos;
	f32 speed;
	b32 is_shooting;
	color_wu ColorWu;
	bounding_box BoundingBox;
	circle Shield;


	tile_map_position TileMapPos;
} player;

typedef struct
{
	u32 length;
	u8 *data;
} string;

enum
{
	GAME_PLAYING,
	GAME_EDITOR
};

typedef struct
{
	v2f Pos;
	f32 time_left;
} projectile_trail;

typedef struct
{
	v2f Pos;
	v2f Direction;
	v2f dPos;

	f32 distance_remaining;
	b32 active;


	projectile_trail Trails[4];
	u32 trail_next;
	f32 time_between_next_trail;
} projectile;



typedef struct
{
	f32 pixels_per_meter;
	v2f WorldHalfDim;

	f32 meters_per_tile;
	s32 tile_count_x;
	s32 tile_count_y;

	memory_arena WorldArena;

	color Colors[4];
	color_wu ColorWu[WU_COLOR_COUNT];
	b32 is_initialized;
	player Player;

	projectile Projectiles[64];
	u32 projectile_next;

	f32 projectile_speed;
	f32 projectile_half_width;
	f32 time_between_new_projectiles;

	u32 asteroid_count;
	asteroid Asteroids[16];




} game_state;

typedef struct
{
	u64 total_size;
	b32 is_initialized;
	void *memory_block;

} game_memory;

#define GAME_H
#endif
