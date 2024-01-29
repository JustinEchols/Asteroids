#if !defined(GAME_SIM_REGION_H)

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

struct entity
{
	u32 index;

	entity_type type;
	shape_type shape;
	u32 flags;

	b32 is_shooting;
	b32 is_warping;
	b32 is_shielded;

	f32 height;
	f32 base_half_width;

	b32 use_normalized_accel;
	f32 speed;
	f32 max_speed;
	f32 angular_speed;
	f32 mass;
	f32 radius;

	f32 distance_limit;

	v2f Pos;
	v2f dPos;
	v2f Right;
	v2f Direction;

	u8 hit_point_max;
	hit_point HitPoints;

	player_polygon Poly;
};

#define GAME_SIM_REGION_H
#endif
