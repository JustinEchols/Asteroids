/*
 * TODO:
 *	- Collision Detection
 *		- Asteroid collisions
 *		- Projectile collisions
 *
 * 	- Asset loading
 * 	- VfX
 *		- Animations
 *			- Asteroid destruction
 *			- Lasers/beams
 *			- Warping
 *			- Shield
 *				- Appears on collision
 *				- Fades shortly thereafter
 *			- Ship phasing
 *				- Ship can enter a "flux state" and temporarily phase through
 *				objects avoiding collisions
 * 		- Particles
 *			- Ship thrusters
 *			- Energy beam
 *			- Asteroids destruction
 * 	- Game of life?
 *		- Destorying an asteoid spawns alien
 *		- Alien behavior adheres to the rules of the game of life
 *		- Include a weighting so that the alien movement is biased towards the player.
 * 	- SFX
 *		- Audio mixer
 * 	- Score
 * 	- Menu
 * 	- Optimization pass
 *		- Threading
 *		- Profiling
 *		- SIMD
 *		- Intrinsics
 * Physics
	 - Angular velocity
	 - Mass
	 - Asteroid acceleration?

 * 	- Audio mixing
 * 	- Bitmap transformations (rotations, scaling, ...)
 * 	- UV coordinate mapping
 * 	- Normal mapping
 *
 */

// TODO(Justin) Collision based on whether or not the
// player is shielded.

// TODO(Justin): Getting the normal of the ship's side
// that the asteroid makes contact with, I think, is
// incorrect.

// NOTE(Justin): Collisions have been colesced into the sat_collision
// function. Not sure if this was a good idea.

#include "game.h"
#include "game_geometry.h"
#include "game_geometry.cpp"
#include "game_render_group.h"
#include "game_render_group.cpp"
#include "game_random.h"
#include "game_entity.h"
#include "game_string.cpp"
#include "game_asset.cpp"

#define White V3F(1.0f, 1.0f, 1.0f)
#define Red V3F(1.0f, 0.0f, 0.0f)
#define Green V3F(0.0f, 1.0f, 0.0f)
#define Blue V3F(0.0f, 0.0f, 1.0f)
#define Black V3F(0.0f, 0.0f, 0.0f)

internal void
debug_sound_buffer_fill(sound_buffer *SoundBuffer)
{
	s16 wave_amplitude = 3000;
	int wave_frequency = 256;
	int samples_per_period = SoundBuffer->samples_per_second / wave_frequency;

	local_persist f32 t = 0.0f;
	f32 dt = 2.0f * PI32 / samples_per_period;

	s16 *sample = SoundBuffer->samples;
	for(int sample_index = 0; sample_index < SoundBuffer->sample_count; sample_index++)
	{
		s16 sample_value = (s16)(wave_amplitude * sinf(t));

		*sample++ = sample_value;
		*sample++ = sample_value;

		t += dt;
	}
}

internal void
sound_buffer_fill(game_state *GameState, sound_buffer *SoundBuffer)
{
	s16 *sample_out = SoundBuffer->samples;
	for(s32 sample_index = 0; sample_index < SoundBuffer->sample_count; sample_index++)
	{
		s16 sample_value = GameState->TestSound.samples[0][(GameState->test_sample_index + sample_index) %
			GameState->TestSound.sample_count];
		*sample_out++ = sample_value;
		*sample_out++ = sample_value;
	}
	GameState->test_sample_index += SoundBuffer->sample_count;
}

internal v2f
lerp_points(v2f P1, v2f P2, f32 t)
{
	v2f Result = {0};

	Result.x = (1 - t) * P1.x + t * P2.x;
	Result.y = (1 - t) * P1.y + t * P2.y;

	return(Result);
}

internal v3f
lerp_color(v3f ColorA, v3f ColorB, f32 t)
{
	v3f Result = {0};

	Result.x = (1 - t) * ColorA.x + t * ColorB.x; 
	Result.y = (1 - t) * ColorA.y + t * ColorB.y; 
	Result.z = (1 - t) * ColorA.z + t * ColorB.z; 

	return(Result);
}

inline entity *
entity_get(game_state *GameState, u32 index)
{
	entity *Result = 0;
	if((index > 0) && (index < ARRAY_COUNT(GameState->Entities)))
	{
		Result = GameState->Entities + index;
	}
	return(Result);
}

internal entity *
entity_add(game_state *GameState, entity_type type)
{
	ASSERT(GameState->entity_count < ARRAY_COUNT(GameState->Entities));

	u32 entity_index = 0;
	if(type == ENTITY_PROJECTILE)
	{
		b32 projectile_overwrite = false;
		for(u32 i = 1; i < ARRAY_COUNT(GameState->Entities); i++)
		{
			entity *Entity = GameState->Entities + i;
			if((Entity->type == ENTITY_PROJECTILE) && (entity_flag_is_set(Entity, ENTITY_FLAG_NON_SPATIAL)))
			{
				entity_index = i;
				projectile_overwrite = true;
				break;
			}
		}
		if(!projectile_overwrite)
		{
			entity_index = GameState->entity_count++;
		}
	}
	else
	{
		entity_index = GameState->entity_count++;
	}

	entity *Entity = GameState->Entities + entity_index;
	*Entity = {};
	Entity->index = entity_index;
	Entity->type = type;

	return(Entity);
}

internal void
player_polygon_update(entity *EntityPlayer)
{
	v2f FrontCenter = EntityPlayer->Pos + EntityPlayer->height * EntityPlayer->Direction;
	v2f FrontLeft = FrontCenter + -1.5f * EntityPlayer->Right;
	v2f FrontRight = FrontCenter + 1.5f * EntityPlayer->Right;
	v2f BackCenter = EntityPlayer->Pos - 0.5f * EntityPlayer->height * EntityPlayer->Direction;

	v2f Right = EntityPlayer->Pos + 0.75f * EntityPlayer->base_half_width * EntityPlayer->Right;
	v2f Left = EntityPlayer->Pos -0.75f * EntityPlayer->base_half_width * EntityPlayer->Right;

	v2f RearLeftEngine = Left -1.0f * EntityPlayer->height * EntityPlayer->Direction;
	v2f RearRightEngine = Right - 1.0f * EntityPlayer->height * EntityPlayer->Direction;

	v2f SideLeftEngine = BackCenter -1.0f * EntityPlayer->base_half_width * EntityPlayer->Right;
	v2f SideRightEngine = BackCenter + 1.0f * EntityPlayer->base_half_width * EntityPlayer->Right;

	EntityPlayer->Poly.Vertices[0] = FrontLeft;
	EntityPlayer->Poly.Vertices[1] = SideLeftEngine;
	EntityPlayer->Poly.Vertices[2] = RearLeftEngine;
	EntityPlayer->Poly.Vertices[3] = BackCenter;
	EntityPlayer->Poly.Vertices[4] = RearRightEngine;
	EntityPlayer->Poly.Vertices[5] = SideRightEngine;
	EntityPlayer->Poly.Vertices[6] = FrontRight;
}

internal void
collision_rule_add(game_state *GameState, u32 index_a, u32 index_b, b32 should_collide)
{
	if(index_a > index_b)
	{
		u32 temp = index_a;
		index_a = index_b;
		index_b = temp;
	}
	pairwise_collision_rule *Found = 0;

	// TODO(Justin): Better hash function.
	u32 hash_bucket = index_a & (ARRAY_COUNT(GameState->CollisionRuleHash) - 1);
	for(pairwise_collision_rule *Rule = GameState->CollisionRuleHash[hash_bucket];
		Rule;
		Rule = Rule->NextInHash)
	{
		if((Rule->index_a == index_a) &&
		   (Rule->index_b == index_b))
		{
			Found = Rule;
			break;
		}
	}

	if(!Found)
	{
		Found = push_struct(&GameState->WorldArena, pairwise_collision_rule);
		Found->NextInHash = GameState->CollisionRuleHash[hash_bucket];
		GameState->CollisionRuleHash[hash_bucket] = Found;
	}

	if(Found)
	{
		Found->index_a = index_a;
		Found->index_b = index_b;
		Found->should_collide = should_collide;
	}
}

internal b32
entities_should_collide(game_state *GameState, entity *EntityA, entity *EntityB)
{
	b32 Result = false;

	if(EntityA->index > EntityB->index)
	{
		entity *Temp = EntityA;
		EntityA = EntityB;
		EntityB = Temp;
	}

	if(!entity_flag_is_set(EntityA, ENTITY_FLAG_NON_SPATIAL) &&
	   !entity_flag_is_set(EntityB, ENTITY_FLAG_NON_SPATIAL))
	  {
		  // TODO(Justn) Property based logic here.
		  Result = true;
	  }

	// TODO(Justin): Better hash function.
	u32 hash_bucket = EntityA->index & (ARRAY_COUNT(GameState->CollisionRuleHash) - 1);
	for(pairwise_collision_rule *Rule = GameState->CollisionRuleHash[hash_bucket];
		Rule;
		Rule = Rule->NextInHash)
	{
		if((Rule->index_a == EntityA->index) &&
		   (Rule->index_b == EntityB->index))
		{
			Result = Rule->should_collide;
			break;
		}
	}

	return(Result);
}

internal void
player_collision_handle(entity *Entity)
{
	if(!Entity->is_shielded)
	{
		Entity->HitPoints.count = Entity->hit_point_max;
		Entity->is_shielded = true;
		Entity->Pos = V2F(0.0f, 0.0f);
		Entity->dPos = V2F(0.0f, 0.0f);
		Entity->Direction = V2F(1.0f, 0.0f);

		player_polygon_update(Entity);
	}
	else
	{
		Entity->HitPoints.count--;
		if(Entity->HitPoints.count == 0)
		{
			Entity->is_shielded = false;
		}
	}
}

internal void
entity_move(game_state *GameState, entity *Entity, v2f ddPos, f32 dt)
{ 
	world *World = GameState->World;

	f32 ddp_length_squared = v2f_length_squared(ddPos);
	if(ddp_length_squared > 1.0f)
	{
		ddPos *= (1.0f / f32_sqrt(ddp_length_squared));
	}
	ddPos *= Entity->speed;

	v2f EntityDelta = (0.5f * ddPos * SQUARE(dt) + dt * Entity->dPos);

	v2f EntityOldPos = Entity->Pos;
	v2f EntityNewPos = Entity->Pos;

	EntityNewPos = EntityOldPos + EntityDelta;
	v2f EntityNewVel = dt * ddPos + Entity->dPos;

	f32 max_speed = 50.0f;
	f32 dp_length_squared = v2f_length_squared(EntityNewVel);
	if(dp_length_squared > 75.0f)
	{
		if(ABS(EntityNewVel.x) > 50.0f)
		{
			EntityNewVel.x *= ABS(1.0f / EntityNewVel.x);
			EntityNewVel.x *= max_speed;
		}
		if(ABS(EntityNewVel.y) > 50.0f)
		{
			EntityNewVel.y *= ABS(1.0f / EntityNewVel.y);
			EntityNewVel.y *= max_speed;
		}
	}
	Entity->dPos = EntityNewVel;

	for(u32 iteration = 0; iteration < 4; iteration++)
	{
		f32 t_min = 1.0f;
		v2f Normal = {};
		v2f DesiredPosition = Entity->Pos + EntityDelta;
		entity *HitEntity = 0;
		if(entity_flag_is_set(Entity, ENTITY_FLAG_COLLIDES) &&
		 (!entity_flag_is_set(Entity, ENTITY_FLAG_NON_SPATIAL)))
		{
			for(u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++)
			{
				entity *TestEntity = entity_get(GameState, entity_index);
				if(Entity != TestEntity)
				{
					if(entity_flag_is_set(TestEntity, ENTITY_FLAG_COLLIDES) &&
					  !entity_flag_is_set(TestEntity, ENTITY_FLAG_NON_SPATIAL))
					{
						if((Entity->shape == SHAPE_CIRCLE) &&
						   (TestEntity->shape == SHAPE_CIRCLE))
						{
							v2f DeltaBetweenCenters = TestEntity->Pos - Entity->Pos;
							v2f TestEntityDelta = dt * TestEntity->dPos;

							if(circles_collide(Entity->Pos, EntityDelta, Entity->radius,
										   TestEntity->Pos, TestEntityDelta, TestEntity->radius, &t_min))
							{
								Normal = v2f_normalize(-1.0f * DeltaBetweenCenters);
								HitEntity = TestEntity;
							}
						}
						else if((Entity->shape == SHAPE_CIRCLE) &&
								(TestEntity->shape == SHAPE_POLY))
						{
							if(TestEntity->is_shielded)
							{
								v2f DeltaBetweenCenters = TestEntity->Pos - Entity->Pos;
								v2f TestEntityDelta = dt * TestEntity->dPos;

								if(circles_collide(Entity->Pos, EntityDelta, Entity->radius,
											TestEntity->Pos, TestEntityDelta, TestEntity->radius, &t_min))
								{
									Normal = v2f_normalize(-1.0f * DeltaBetweenCenters);
									HitEntity = TestEntity;
								}
							}
							else
							{
								if(sat_collision(TestEntity, Entity))
								{ 
									// TODO(Justin): The normal is incorrect.
									v2f Delta = TestEntity->Pos - Entity->Pos;
									for(u32 vertex_i = 0; vertex_i < ARRAY_COUNT(TestEntity->Poly.Vertices); vertex_i++)
									{
										v2f P0 = TestEntity->Poly.Vertices[vertex_i];
										v2f P1 = TestEntity->Poly.Vertices[(vertex_i + 1)  % ARRAY_COUNT(TestEntity->Poly.Vertices)];

										v2f Edge = P1 - P0;
										v2f Perp = -1.0f * v2f_perp(Edge);

										if(v2f_dot(Perp, Delta) < 0.0f)
										{
											Normal = v2f_normalize(Perp);
											break;
										}
									}
									HitEntity = TestEntity;
								}
							}
						}
					}
				}
			}

			Entity->Pos += t_min * EntityDelta;
			if(HitEntity)
			{
				if((Entity->type == ENTITY_ASTEROID) &&
				   (HitEntity->type == ENTITY_ASTEROID))
				{
					Entity->dPos = Entity->speed * Normal;
					Entity->Direction = v2f_normalize(Entity->dPos);
					EntityDelta = DesiredPosition - Entity->Pos;

					HitEntity->dPos = Entity->speed * -1.0f * Normal;
					HitEntity->Direction = v2f_normalize(Entity->dPos);
				}

				if((Entity->type == ENTITY_PLAYER) &&
				   (HitEntity->type == ENTITY_ASTEROID))
				{
					player_collision_handle(Entity);
				}

				if((Entity->type == ENTITY_ASTEROID) &&
					(HitEntity->type == ENTITY_PLAYER))
				{
					player_collision_handle(HitEntity);

					if(HitEntity->is_shielded)
					{
						Entity->dPos = Entity->speed * Normal;
						Entity->Direction = v2f_normalize(Entity->dPos);
					}
					else
					{
						Entity->dPos = Entity->dPos - 2.0f * v2f_dot(Entity->dPos, Normal) * Normal;
						Entity->Direction = v2f_normalize(Entity->dPos);
						Entity->Pos = EntityNewPos;
					}
				}

				if((Entity->type == ENTITY_ASTEROID) &&
				   (HitEntity->type == ENTITY_PROJECTILE))
				{
					entity_flag_set(Entity, ENTITY_FLAG_NON_SPATIAL);
					entity_flag_set(HitEntity, ENTITY_FLAG_NON_SPATIAL);
				}

			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if(Entity->Pos.x > World->Dim.x)
	{
		Entity->Pos.x = Entity->Pos.x - 2.0f * World->Dim.x;
	}
	if(Entity->Pos.x < -World->Dim.x)
	{
		Entity->Pos.x += 2.0f * World->Dim.x;
	}
	if(Entity->Pos.y > World->Dim.y)
	{
		Entity->Pos.y = Entity->Pos.y - 2.0f * World->Dim.y;
	}
	if(Entity->Pos.y < -World->Dim.y)
	{
		Entity->Pos.y += 2.0f * World->Dim.y;
	}

	if(Entity->type == ENTITY_PLAYER)
	{
		player_polygon_update(Entity);
	}
}

internal entity *
player_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_PLAYER);

	Entity->shape = SHAPE_POLY;
	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	Entity->is_shooting = false;
	Entity->is_warping = false;
	Entity->is_shielded = true;

	Entity->height = 18.0f;
	Entity->base_half_width = 15.0f;
	Entity->speed = 60.0f;
	Entity->max_speed = 80.0f;
	Entity->radius = Entity->height;

	Entity->Pos = V2F(0.0f, 0.0f);
	Entity->Right = V2F(1.0f, 0.0f);
	Entity->Direction = V2F(0.0f, 1.0f);

	Entity->hit_point_max = 3;
	Entity->HitPoints.count = Entity->hit_point_max;

	// NOTE(Justin): This is not a vertex but used for calculating vertices.
	// Vertices are also stored in CCW order which is requried.
	v2f FrontCenter = Entity->Pos + Entity->height * Entity->Direction;

	v2f FrontLeft = FrontCenter + -1.5f * Entity->Right;
	v2f FrontRight = FrontCenter + 1.5f * Entity->Right;
	v2f BackCenter = Entity->Pos - 0.5f * Entity->height * Entity->Direction;

	v2f Right = Entity->Pos + 0.75f * Entity->base_half_width * Entity->Right;
	v2f Left = Entity->Pos -0.75f * Entity->base_half_width * Entity->Right;

	v2f RearLeftEngine = Left -1.0f * Entity->height * Entity->Direction;
	v2f RearRightEngine = Right - 1.0f * Entity->height * Entity->Direction;
	v2f SideLeftEngine = BackCenter -1.0f * Entity->base_half_width * Entity->Right;
	v2f SideRightEngine = BackCenter + 1.0f * Entity->base_half_width * Entity->Right;

	Entity->Poly.Vertices[0] = FrontLeft;
	Entity->Poly.Vertices[1] = SideLeftEngine;
	Entity->Poly.Vertices[2] = RearLeftEngine;
	Entity->Poly.Vertices[3] = BackCenter;
	Entity->Poly.Vertices[4] = RearRightEngine;
	Entity->Poly.Vertices[5] = SideRightEngine;
	Entity->Poly.Vertices[6] = FrontRight;

	return(Entity);
}


internal entity *
asteroid_add(game_state *GameState, asteroid_size SIZE, v2f Pos, v2f Dir)
{
	entity *Entity = entity_add(GameState, ENTITY_ASTEROID);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	f32 speed_scale = f32_rand_percent();

	Entity->Direction = Dir;
	Entity->speed = 19.0f;

	f32 asteroid_speed = speed_scale * Entity->speed;

	// TODO(Justin): Make sure that asteroids cannot spwan on top of the player
	world *World = GameState->World;
	Entity->Pos = Pos;//V2F(f32_rand_between(-World->Dim.x, World->Dim.x), f32_rand_between(-World->Dim.y, World->Dim.y));

	Entity->dPos = asteroid_speed * Entity->Direction;

	// TODO(Jusitn): Rand scaling, orientation, size, and mass.

	Entity->shape = SHAPE_CIRCLE;

	if(SIZE == ASTEROID_SMALL)
	{
		Entity->radius = 8.0f;
		Entity->hit_point_max = 1;
		Entity->HitPoints.count = 1;
	}

	Entity->mass = Entity->radius * 10.0f;

	collision_rule_add(GameState, GameState->player_entity_index, Entity->index, true);
	return(Entity);
}

internal entity *
asteroid_add(game_state *GameState, asteroid_size SIZE)
{
	entity *Entity = entity_add(GameState, ENTITY_ASTEROID);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	f32 speed_scale = f32_rand_percent();

	Entity->Direction = v2f_normalize(V2F(f32_rand_between(-1.0f, 1.0f), f32_rand_between(-1.0f, 1.0f)));
	Entity->speed = 19.0f;

	f32 asteroid_speed = speed_scale * Entity->speed;

	// TODO(Justin): Make sure that asteroids cannot spwan on top of the player
	world *World = GameState->World;
	Entity->Pos = V2F(f32_rand_between(-World->Dim.x, World->Dim.x), f32_rand_between(-World->Dim.y, World->Dim.y));

	Entity->dPos = asteroid_speed * Entity->Direction;

	// TODO(Jusitn): Rand scaling, orientation, size, and mass.

	Entity->shape = SHAPE_CIRCLE;

	if(SIZE == ASTEROID_SMALL)
	{
		Entity->radius = 8.0f;
	}

	Entity->mass = Entity->radius * 10.0f;

	return(Entity);
}

internal entity *
familiar_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_FAMILIAR);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	Entity->speed = 1.0f;

	return(Entity);
}

internal entity *
projectile_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_PROJECTILE);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	entity *EntityPlayer = GameState->Entities + GameState->player_entity_index;

	Entity->distance_remaining = 200.0f;
	Entity->Direction = EntityPlayer->Direction;
	Entity->Pos = EntityPlayer->Pos + EntityPlayer->height * Entity->Direction;

	Entity->speed = 100.0f;
	Entity->dPos = Entity->speed * Entity->Direction;

	Entity->shape = SHAPE_CIRCLE;
	Entity->radius = 3.0f;
	collision_rule_add(GameState, GameState->player_entity_index, Entity->index, false);

	return(Entity);
}

internal entity *
circle_add(game_state *GameState, v2f Pos, v2f Dir)
{
	entity *Entity = entity_add(GameState, ENTITY_CIRCLE);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	world *World = GameState->World;

	Entity->Pos = Pos;
	Entity->Direction = Dir;

	//Entity->Pos = v2f_rand((f32)World->width, (f32)World->height);
	//Entity->Direction = v2f_normalize(V2F(f32_rand(-1.0f, 1.0f), f32_rand(-1.0f, 1.0f)));
	Entity->speed = 10.0f;
	Entity->dPos = Entity->speed * Entity->Direction;

	Entity->radius = 25.0f;

	return(Entity);
}

internal entity *
square_add(game_state *GameState, v2f Pos, v2f Dir)
{
	entity *Entity = entity_add(GameState, ENTITY_SQUARE);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);

	world *World = GameState->World;

	Entity->Pos = Pos;
	Entity->Direction = Dir;

	Entity->speed = 10.0f;
	Entity->dPos = Entity->speed * Entity->Direction;

	Entity->radius = 25.0f;

	return(Entity);
}



internal void
familiar_update(game_state *GameState, entity *Entity, f32 dt)
{
	// TODO(Justin): Spatial partition search?

	// NOTE(Justin): 10 meter maximum search
	entity Player = {};
	f32 distance_sq_max = SQUARE(100.0f);
	f32 closest_dist = 0.0f;

	v2f PlayerPos = {};
	v2f EntityPos = Entity->Pos;
	for(u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++)
	{
		entity *TestEntity = entity_get(GameState, entity_index);
		if(TestEntity->type == ENTITY_PLAYER)
		{
			PlayerPos = TestEntity->Pos;
			f32 test_distance_sq = v2f_length_squared(EntityPos - PlayerPos);
			if(test_distance_sq < distance_sq_max)
			{
				Player = *TestEntity;
				closest_dist = test_distance_sq;
			}
		}
	}
	v2f ddPos = {};
	if(closest_dist > 0.0f)
	{
		f32 acceleration = 1.0f;
		f32 one_over_length = acceleration / f32_sqrt(distance_sq_max);
		ddPos = one_over_length * (PlayerPos - EntityPos);
		Entity->Direction = ddPos;
	}
	entity_move(GameState, Entity, ddPos, dt);
}

internal void
projectile_update(game_state *GameState, entity *Entity, f32 dt)
{
	ASSERT(Entity->type == ENTITY_PROJECTILE);

	v2f OldPos = Entity->Pos;
	v2f ddPos = {};

	entity_move(GameState, Entity, ddPos, dt);

	f32 distance_traveled = v2f_length(Entity->Pos - OldPos);
	Entity->distance_remaining -= distance_traveled;
	if(Entity->distance_remaining < 0.0f)
	{
		entity_flag_set(Entity, ENTITY_FLAG_NON_SPATIAL);
	}
}

internal void
player_polygon_draw(back_buffer *BackBuffer, game_state *GameState, v2f BottomLeft, entity *EntityPlayer)
{
	m3x3 M = GameState->MapToScreenSpace;

	v2f FrontLeft = EntityPlayer->Poly.Vertices[0];
	v2f SideLeftEngine = EntityPlayer->Poly.Vertices[1];
	v2f RearLeftEngine = EntityPlayer->Poly.Vertices[2];
	v2f BackCenter = EntityPlayer->Poly.Vertices[3];
	v2f RearRightEngine = EntityPlayer->Poly.Vertices[4];
	v2f SideRightEngine = EntityPlayer->Poly.Vertices[5];
	v2f FrontRight = EntityPlayer->Poly.Vertices[6];

	v2f ScreenPos = M * EntityPlayer->Pos;
	
	v2f ScreenBackCenter =  M * BackCenter;
	v2f ScreenFrontLeft = M * FrontLeft;
	v2f ScreenFrontRight = M * FrontRight;;
	v2f ScreenRearLeftEngine = M * RearLeftEngine;
	v2f ScreenRearRightEngine = M * RearRightEngine;
	v2f ScreenSideLeftEngine = M * SideLeftEngine; 
	v2f ScreenSideRightEngine = M * SideRightEngine;

	line_dda_draw(BackBuffer, ScreenPos, ScreenBackCenter, White);
	line_dda_draw(BackBuffer, ScreenPos, ScreenFrontLeft, White);
	line_dda_draw(BackBuffer, ScreenPos, ScreenFrontRight, White);
	line_dda_draw(BackBuffer, ScreenBackCenter, ScreenRearLeftEngine, White);
	line_dda_draw(BackBuffer, ScreenBackCenter, ScreenRearRightEngine, White);
	line_dda_draw(BackBuffer, ScreenRearLeftEngine, ScreenSideLeftEngine, White);
	line_dda_draw(BackBuffer, ScreenRearRightEngine, ScreenSideRightEngine, White);
	line_dda_draw(BackBuffer, ScreenFrontLeft, ScreenSideLeftEngine, White);
	line_dda_draw(BackBuffer, ScreenFrontRight, ScreenSideRightEngine, White);
}

internal void
update_and_render(game_memory *GameMemory, back_buffer *BackBuffer, sound_buffer *SoundBuffer, game_input *GameInput)
{
	ASSERT(sizeof(game_state) <= GameMemory->total_size);
	game_state *GameState = (game_state *)GameMemory->permanent_storage;
	if(!GameMemory->is_initialized)
	{
		// NOTE(Justin): Reserve slot 0 for the null entity.

		entity *EntityNull = entity_add(GameState, ENTITY_NULL);

		//GameState->TestSound = wav_file_read_entire("sfx/bloop_00.wav");
		GameState->Background = bitmap_file_read_entire("space_background.bmp");

		GameState->Ship = bitmap_file_read_entire("ship/blueships1_up.bmp");

		loaded_bitmap *ShipBitmap = &GameState->Ship;
		ShipBitmap->Align = V2F((f32)ShipBitmap->width / 2.0f, (f32)ShipBitmap->height / 2.0f);

		GameState->AsteroidBitmap = bitmap_file_read_entire("asteroids/01.bmp");
		loaded_bitmap *AsteroidBitmap = &GameState->AsteroidBitmap;
		AsteroidBitmap->Align = V2F((f32)AsteroidBitmap->width / 2.0f, (f32)AsteroidBitmap->height / 2.0f);

		GameState->LaserBlueBitmap = bitmap_file_read_entire("lasers/laser_small_blue.bmp");
		loaded_bitmap *LaserBlueBitmap = &GameState->LaserBlueBitmap;
		LaserBlueBitmap->Align = V2F((f32)LaserBlueBitmap->width / 2.0f, (f32)LaserBlueBitmap->height / 2.0f);

		memory_arena_initialize(&GameState->WorldArena, GameMemory->total_size - sizeof(game_state),
				(u8 *)GameMemory->permanent_storage + sizeof(game_state));
		GameState->World = push_struct(&GameState->WorldArena, world);

		GameState->pixels_per_meter = 5.0f;

		world *World = GameState->World;
		World->Dim.x = (f32)BackBuffer->width / GameState->pixels_per_meter;
		World->Dim.y = (f32)BackBuffer->height / GameState->pixels_per_meter;

		entity *EntityPlayer = player_add(GameState);
		GameState->player_entity_index = EntityPlayer->index;

		srand(2023);

		// TODO(Justin): This size of the asteroid should be random.

		asteroid_add(GameState, ASTEROID_SMALL, V2F(0.0f, 0.5f * World->Dim.y), V2F(-1.0f, 0.0f));
		asteroid_add(GameState, ASTEROID_SMALL, V2F(-100.0f, 0.5f * World->Dim.y), V2F(1.0f, 0.0f));

#if 1
		for(u32 i = 0; i < 20; i++)
		{
			asteroid_add(GameState, ASTEROID_SMALL);
		}
#endif
		m3x3 *M = &GameState->MapToScreenSpace;

		m3x3 InverseWorldTranslate = m3x3_translation_create(V2F(World->Dim.x, World->Dim.y));
		m3x3 Scale = m3x3_scale_create(GameState->pixels_per_meter / 2.0f);
		m3x3 InverseScreenOffset = m3x3_translation_create(V2F(5.0f, 5.0f));

		*M = InverseScreenOffset * Scale * InverseWorldTranslate;

		GameMemory->is_initialized = true;
	}

	ASSERT(sizeof(transient_state) <= GameMemory->transient_storage_size);
	transient_state *TransientState = (transient_state *)GameMemory->transient_storage;
	if(!TransientState->is_initialized)
	{
		memory_arena_initialize(&TransientState->TransientArena,
				GameMemory->transient_storage_size - sizeof(transient_state),
				(u8 *)GameMemory->transient_storage + sizeof(transient_state));
		TransientState->is_initialized = true;
	}

	v2f BottomLeft = {5.0f, 5.0f};

	world *World = GameState->World;

	entity *EntityPlayer = entity_get(GameState, GameState->player_entity_index);

	bitmap_draw(BackBuffer, &GameState->Background, 0.0f, 0.0f);

	v2f PlayerNewRight = EntityPlayer->Right;
	v2f PlayerNewDirection = EntityPlayer->Direction;

	f32 dt = GameInput->dt_for_frame;
	game_controller_input *KeyboardController = &GameInput->Controller;
	if(KeyboardController->Left.ended_down)
	{
		m3x3 Rotation = m3x3_rotate_about_origin(5.0f * dt);
		PlayerNewRight = Rotation * PlayerNewRight;
		PlayerNewDirection = Rotation * PlayerNewDirection;
	}
	if(KeyboardController->Right.ended_down)
	{
		m3x3 Rotation = m3x3_rotate_about_origin(-5.0f * dt);
		PlayerNewRight = Rotation * PlayerNewRight;
		PlayerNewDirection = Rotation * PlayerNewDirection;
	}
	EntityPlayer->Right = PlayerNewRight;
	EntityPlayer->Direction = PlayerNewDirection;

	//
	// NOTE(Justin): Player projectiles
	//

	if(KeyboardController->Space.ended_down)
	{
		if(!EntityPlayer->is_shooting)
		{
			projectile_add(GameState);
			EntityPlayer->is_shooting = true;
		}
		else
		{
			GameState->time_between_new_projectiles += dt;
			if(GameState->time_between_new_projectiles > 0.6f)
			{
				projectile_add(GameState);
				EntityPlayer->is_shooting = true;
				GameState->time_between_new_projectiles = 0.0f;
			}
		}
	}
	else
	{
		EntityPlayer->is_shooting = false;
	}

	v2f PlayerAccel = {};
	if(KeyboardController->Up.ended_down)
	{
		PlayerAccel = V2F(PlayerNewDirection.x, PlayerNewDirection.y);
	}
	entity_move(GameState, GameState->Entities + GameState->player_entity_index, PlayerAccel, dt);

	v2f AsteroidAccel = {};
	for(u32 entity_index = 1; entity_index < ARRAY_COUNT(GameState->Entities); entity_index++)
	{
		entity *Entity = GameState->Entities + entity_index;
		if(Entity->type == ENTITY_ASTEROID)
		{
			entity_move(GameState, Entity, AsteroidAccel, dt);
		}
	}

	// 
	// NOTE(Justin): Render
	//

	m3x3 M = GameState->MapToScreenSpace;
	f32 pixels_per_meter = GameState->pixels_per_meter;

	render_group RenderGroup = {};
	RenderGroup.pixels_per_meter = pixels_per_meter;
	RenderGroup.MapToScreenSpace = M;
	for(u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++)
	{
		entity *Entity = GameState->Entities + entity_index;
		v2f Accel = {};
		switch(Entity->type)
		{
			case ENTITY_PLAYER:
			{
				v2f PlayerPos = Entity->Pos;
				push_bitmap(&RenderGroup, &GameState->Ship, PlayerPos, Entity->Right, Entity->Direction,
						GameState->Ship.Align);

				player_polygon_draw(BackBuffer, GameState, BottomLeft, Entity);

				if(Entity->is_shielded)
				{
					circle Circle = circle_init(Entity->Pos, Entity->height);
					Circle.Center = M * Circle.Center;
					Circle.radius *= 0.5f * GameState->pixels_per_meter;

					circle_draw(BackBuffer, &Circle, White);

				}
			} break;
			case ENTITY_ASTEROID:
			{
				v2f AsteroidPos = Entity->Pos;
				if(!entity_flag_is_set(Entity, ENTITY_FLAG_NON_SPATIAL))
				{
					push_bitmap(&RenderGroup, &GameState->AsteroidBitmap,
							AsteroidPos, -1.0f * v2f_perp(Entity->Direction), Entity->Direction,
							GameState->AsteroidBitmap.Align);
				}

			} break;
			case ENTITY_FAMILIAR:
			{
				//familiar_update(GameState, Entity, dt);

				//v2f AsteroidScreenPos = tile_map_get_screen_coordinates(TileMap, &Entity->TileMapPos,
				//		BottomLeft, tile_side_in_pixels, meters_to_pixels);
				//debug_vector_draw_at_point(BackBuffer, AsteroidScreenPos, Entity->Direction);
				//v2f Alignment = {(f32)GameState->AsteroidBitmap.width / 2.0f,
				//				 (f32)GameState->AsteroidBitmap.height / 2.0f};

				//push_piece(&RenderGroup, &GameState->AsteroidBitmap, AsteroidScreenPos, Alignment);

			} break;
			case ENTITY_PROJECTILE:
			{
				projectile_update(GameState, Entity, dt);
				if(!entity_flag_is_set(Entity, ENTITY_FLAG_NON_SPATIAL))
				{
					v2f LaserPos = Entity->Pos;
					push_bitmap(&RenderGroup, &GameState->LaserBlueBitmap, LaserPos,
							-1.0f * v2f_perp(Entity->Direction), Entity->Direction,
							GameState->LaserBlueBitmap.Align);
				}
			} break;
			default:
			{
				INVALID_CODE_PATH;
			} break;
		}
	}

	for(u32 index = 0; index < RenderGroup.element_count; index++)
	{
		render_entry *Element = RenderGroup.Elements + index;

		f32 x = Element->Origin.x;
		f32 y = Element->Origin.y;
		if(Element->Texture)
		{
			bitmap_draw(BackBuffer, Element->Texture, x, y, Element->Color.a);
		}
		else
		{
			rectangle_draw(BackBuffer, Element->Origin - Element->Dim, Element->Origin + Element->Dim,
																								Element->Color);
		}
	}

	f32 *time = &GameState->time;
	*time += dt;

	v2f D = 30.0f * V2F(cosf(*time), 0.0f);
	v2f O = D + V2F(220.0f, 220.0f);
	v2f X = (f32)GameState->Ship.width * V2F(1.0f, 0.0f);
	//v2f X = (f32)GameState->Ship.width * V2F(cosf(*time), sinf(*time));
	v2f Y = (f32)GameState->Ship.height * V2F(0.0f, 1.0f);

	rectangle_draw_slowly(BackBuffer, &GameState->Ship, V2F(0.0f, 0.0f), O - 0.5f * X - 0.5f * Y, X, Y, V4F(1.0f, 1.0f, 1.0f, 1.0f));
}
