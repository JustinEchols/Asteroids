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
#include "game_random.h"
#include "game_render_group.h"
#include "game_render_group.cpp"
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

internal void 
rectangle_draw(back_buffer *BackBuffer, v2f Min, v2f Max, f32 r, f32 g, f32 b)
{
	s32 x_min = f32_round_to_s32(Min.x);
	s32 y_min = f32_round_to_s32(Min.y);
	s32 x_max = f32_round_to_s32(Max.x);
	s32 y_max = f32_round_to_s32(Max.y);

	if(x_min < 0)
	{
		x_min += BackBuffer->width;
	}
	if(x_max > BackBuffer->width)
	{
		x_max -= BackBuffer->width;
	}
	if(y_min < 0)
	{
		y_min += BackBuffer->height;
	}
	if(y_max > BackBuffer->height)
	{
		y_max -= BackBuffer->height;
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min ;
	for(int row = y_min; row < y_max; row++)
	{
		u32 *pixel = (u32 *)pixel_row;
		for(int col = x_min; col < x_max; col++)
		{
			*pixel++ = color;
		}
		pixel_row += BackBuffer->stride;
	}
}

internal void 
rectangle_draw(back_buffer *BackBuffer, v2f Min, v2f Max, v3f Color)
{
	rectangle_draw(BackBuffer, Min, Max, Color.r, Color.g, Color.b);
}

internal void
pixel_set(back_buffer *BackBuffer, v2f ScreenXY, u32 color)
{
	s32 screen_x = f32_round_to_s32(ScreenXY.x);
	s32 screen_y = f32_round_to_s32(ScreenXY.y);

	if(screen_x >= BackBuffer->width)
	{
		screen_x = screen_x - BackBuffer->width;
	}
	if(screen_x < 0) 
	{
		screen_x = screen_x + BackBuffer->width;
	}
	if(screen_y >= BackBuffer->height)
	{
		screen_y = screen_y - BackBuffer->height;
	}
	if(screen_y < 0)
	{
		screen_y = screen_y + BackBuffer->height;
	}
	u8 *start = (u8 *)BackBuffer->memory + BackBuffer->stride * screen_y + BackBuffer->bytes_per_pixel * screen_x;
	u32 *pixel = (u32 *)start;
	*pixel = color;
}

internal void
line_dda_draw(back_buffer *BackBuffer, v2f P1, v2f P2, f32 r, f32 g, f32 b)
{
	u32 step_count;

	v2f Pos = P1;

	v2f Delta = P2 - P1;

	if(ABS(Delta.x) > ABS(Delta.y))
	{
		step_count = (u32)ABS(Delta.x);
	}
	else
	{
		step_count = (u32)ABS(Delta.y);
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	v2f Step = (1.0f / (f32)step_count) * Delta;
	for(u32 k = 0; k < step_count; k++)
	{
		Pos += Step;
		pixel_set(BackBuffer, Pos, color);
	}
}

internal void
line_dda_draw(back_buffer *BackBuffer, v2f P1, v2f P2, v3f Color)
{
	line_dda_draw(BackBuffer, P1, P2, Color.r, Color.g, Color.b);
}

internal bounding_box
circle_bounding_box_find(circle Circle)
{
	bounding_box Result = {};

	Result.Min.x = Circle.Center.x - Circle.radius;
	Result.Min.y = Circle.Center.y - Circle.radius;

	Result.Max.x = Circle.Center.x + Circle.radius;
	Result.Max.y = Circle.Center.y + Circle.radius;

	return(Result);
}

internal v2f
circle_support_point(circle *Circle, v2f Dir)
{
	v2f Result = {};
	Result = Circle->Center + Circle->radius * Dir;
	return(Result);
}

internal v2f
circle_tangent(circle *Circle, v2f Dir)
{
	v2f Result = v2f_perp(Circle->radius * Dir);

	return(Result);
}

struct interval
{
	f32 min;
	f32 max;
};

inline b32
interval_contains(interval Interval, f32 x)
{
	b32 Result = false;

	if((x >= Interval.min) && (x <= Interval.max))
	{
		Result = true;
	}
	return(Result);
}

inline f32
interval_length(interval Interval)
{
	f32 Result = Interval.max - Interval.min;

	return(Result);
}

internal void
circle_draw(back_buffer *BackBuffer, circle *Circle, f32 r, f32 b, f32 g)
{
	bounding_box CircleBoudingBox = circle_bounding_box_find(*Circle);

	s32 x_min = f32_round_to_s32(CircleBoudingBox.Min.x);
	s32 y_min = f32_round_to_s32(CircleBoudingBox.Min.y);
	s32 x_max = f32_round_to_s32(CircleBoudingBox.Max.x);
	s32 y_max = f32_round_to_s32(CircleBoudingBox.Max.y);

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));


	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min;

	v2f Center = Circle->Center;
	f32 radius =  Circle->radius;
	interval Interval = {};
	for(s32 row = y_min; row < y_max; row++)
	{
		u32 *pixel = (u32 *)pixel_row;
		for(s32 col = x_min; col < x_max; col++)
		{
			f32 dx = Center.x - col;
			f32 dy = Center.y - row;

			f32 d = (f32)sqrt((dx * dx) + (dy * dy));

			Interval.min = d - 1.0f;
			Interval.max = d + 1.0f;
			if(interval_contains(Interval, radius))
			{
				*pixel++ = color;
			}
			else
			{
				pixel++;
			}
		}
		pixel_row += BackBuffer->stride;
	}
}

internal void
circle_draw(back_buffer *BackBuffer, circle *Circle, v3f Color)
{
	circle_draw(BackBuffer, Circle, Color.r, Color.g, Color.b);
}

internal circle
circle_init(v2f Center, f32 radius)
{
	circle Result = {};

	Result.Center = Center;
	Result.radius = radius;

	return(Result);
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

internal void
bezier_curve_draw(back_buffer *BackBuffer, v2f P1, v2f P2, v2f P3, f32 t)
{
	v2f C1 = lerp_points(P1, P2, t);
	v2f C2 = lerp_points(P2, P3, t);
	v2f C3 = lerp_points(C1, C2, t);

	u32 color = 0xFFFFFF;
	pixel_set(BackBuffer, C3, color);
}

internal void
line_vertical_draw(back_buffer *BackBuffer, f32 x, f32 r, f32 g, f32 b)
{
	s32 x_col = f32_round_to_s32(x);

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + x_col * BackBuffer->bytes_per_pixel;
	for(s32 y = 0; y < BackBuffer->height; y++)
	{
		u32 *pixel = (u32 *)pixel_row;
		*pixel = color;
		pixel_row += BackBuffer->stride;
	}
}

internal void
line_horizontal_draw(back_buffer *BackBuffer, f32 y, f32 r, f32 g, f32 b) 
{
	s32 y_row = f32_round_to_s32(y);

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + y_row * BackBuffer->stride;
	u32 *pixel = (u32 *)pixel_row;
	for(s32 x = 0; x < BackBuffer->width; x++)
	{
		*pixel++ = color;
	}
}

inline u32
colors_alpha_blend(u32 color_a, u32 color_b, f32 alpha)
{
	u32 Result;

	u32 color_a_red = ((color_a & 0xFF0000) >> 16);
	u32 color_a_green = ((color_a & 0xFF00) >> 8);
	u32 color_a_blue = (color_a & 0xFF);

	u32 color_b_red = ((color_b & 0xFF0000) >> 16);
	u32 color_b_green = ((color_b & 0xFF00) >> 8);
	u32 color_b_blue = (color_b & 0xFF);

	u32 result_red = f32_round_to_u32((1.0f - alpha) * color_a_red + alpha * color_b_red);
	u32 result_green = f32_round_to_u32((1.0f - alpha) * color_a_green + alpha * color_b_green);
	u32 result_blue = f32_round_to_u32((1.0f - alpha) * color_a_blue + alpha * color_b_blue);

	Result = ((result_red << 16) | (result_green << 8) | (result_blue << 0));
	return(Result);
}

internal void
rectangle_transparent_draw(back_buffer *BackBuffer, v2f Min, v2f Max, f32 r, f32 g, f32 b, f32 alpha)
{
	s32 x_min = f32_round_to_s32(Min.x);
	s32 y_min = f32_round_to_s32(Min.y);
	s32 x_max = f32_round_to_s32(Max.x);
	s32 y_max = f32_round_to_s32(Max.y);

	if(x_min < 0)
	{
		x_min = 0;
	}
	if(x_max > BackBuffer->width)
	{
		x_max = BackBuffer->width;
	}
	if(y_min < 0)
	{
		y_min = 0;
	}
	if(y_max > BackBuffer->height)
	{
		y_max = BackBuffer->height;
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min;
	for(int row = y_min; row < y_max; row++)
	{
		u32 *pixel = (u32 *)pixel_row;
		for(int col = x_min; col < x_max; col++)
		{
			*pixel++ = colors_alpha_blend(*pixel, color, alpha);
		}
		pixel_row += BackBuffer->stride;
	}
}

internal void
bitmap_draw(back_buffer *BackBuffer, loaded_bitmap *Bitmap, f32 x, f32 y, f32 c_alpha = 1.0f)
{
	s32 x_min = f32_round_to_s32(x);
	s32 y_min = f32_round_to_s32(y);
	s32 x_max = f32_round_to_s32(x + (f32)Bitmap->width);
	s32 y_max = f32_round_to_s32(y + (f32)Bitmap->height);

	s32 region_1_x_min = 0;
	s32 region_1_x_max = 0;
	s32 region_2_x_min = 0;
	s32 region_2_x_max = 0;

	s32 region_1_y_min = 0;
	s32 region_1_y_max = 0;
	s32 region_2_y_min = 0;
	s32 region_2_y_max = 0;

	b32 bitmap_across_x_boundary = false;
	b32 bitmap_across_y_boundary = false;
	if(x_min < 0)
	{
		region_1_x_min = x_min + BackBuffer->width;
		region_1_x_max = BackBuffer->width;
		region_2_x_min = 0;
		region_2_x_max = x_max;
		bitmap_across_x_boundary = true;
	}

	if(y_min < 0)
	{
		region_1_y_min = y_min + BackBuffer->height;;
		region_1_y_max = BackBuffer->height;
		region_2_y_min = 0;
		region_2_y_max = y_max;
		bitmap_across_y_boundary = true;
	}

	if(x_max > BackBuffer->width)
	{
		region_1_x_min = x_min;
		region_1_x_max = BackBuffer->width;
		region_2_x_min = 0;
		region_2_x_max = x_max - BackBuffer->width; 
		bitmap_across_x_boundary = true;
	}

	if(y_max > BackBuffer->height)
	{
		region_1_y_min = y_min;
		region_1_y_max = BackBuffer->height;
		region_2_y_min = 0;
		region_2_y_max = y_max - BackBuffer->height;
		bitmap_across_y_boundary = true;
	}

	if(!(bitmap_across_x_boundary || bitmap_across_y_boundary))
	{
		u8 *dest_row = (u8 *)BackBuffer->memory + y_min * BackBuffer->stride + x_min * BackBuffer->bytes_per_pixel;
		u8 *src_row = (u8 *)Bitmap->memory;
		for(s32 y = y_min; y < y_max; y++)
		{
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for(s32 x = x_min; x < x_max; x++)
			{

				f32 alpha = (f32)((*src >> 24) & 0xFF) / 255.0f;
				alpha *= c_alpha;

				f32 src_r = (f32)((*src >> 16) & 0xFF);
				f32 src_g = (f32)((*src >> 8) & 0xFF);
				f32 src_b = (f32)((*src >> 0) & 0xFF);

				f32 dest_r = (f32)((*dest >> 16) & 0xFF);
				f32 dest_g = (f32)((*dest >> 8) & 0xFF);
				f32 dest_b = (f32)((*dest >> 0) & 0xFF);

				f32 r = (1.0f - alpha) * dest_r + alpha * src_r;
				f32 g = (1.0f - alpha) * dest_g + alpha * src_g; 
				f32 b = (1.0f - alpha) * dest_b + alpha * src_b;

				*dest = (((u32)(r + 0.5f) << 16) | ((u32)(g + 0.5f) << 8) | ((u32)(b + 0.5f) << 0));

				dest++;
				src++;
			}
			dest_row += BackBuffer->stride;
			src_row += Bitmap->stride;
		}
	}
	else
	{
		if(!bitmap_across_y_boundary)
		{
			region_1_y_min = y_min;
			region_1_y_max = y_max;

			region_2_y_min = region_1_y_min;
			region_2_y_max = region_1_y_max;
		}
		else if(!bitmap_across_x_boundary)
		{
			region_1_x_min = x_min;
			region_1_x_max = x_max;

			region_2_x_min = region_1_x_min;
			region_2_x_max = region_1_x_max;
		}

		u8 *dest_row = (u8 *)BackBuffer->memory + region_1_y_min * BackBuffer->stride + region_1_x_min * BackBuffer->bytes_per_pixel;
		u8 *src_row = (u8 *)Bitmap->memory;
		for(s32 y = region_1_y_min; y < region_1_y_max; y++)
		{
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for(s32 x = region_1_x_min; x < region_1_x_max; x++)
			{

				f32 alpha = (f32)((*src >> 24) & 0xFF) / 255.0f;
				alpha *= c_alpha;
				f32 src_r = (f32)((*src >> 16) & 0xFF);
				f32 src_g = (f32)((*src >> 8) & 0xFF);
				f32 src_b = (f32)((*src >> 0) & 0xFF);

				f32 dest_r = (f32)((*dest >> 16) & 0xFF);
				f32 dest_g = (f32)((*dest >> 8) & 0xFF);
				f32 dest_b = (f32)((*dest >> 0) & 0xFF);

				f32 r = (1.0f - alpha) * dest_r + alpha * src_r;
				f32 g = (1.0f - alpha) * dest_g + alpha * src_g; 
				f32 b = (1.0f - alpha) * dest_b + alpha * src_b;

				*dest = (((u32)(r + 0.5f) << 16) | ((u32)(g + 0.5f) << 8) | ((u32)(b + 0.5f) << 0));

				dest++;
				src++;
			}
			dest_row += BackBuffer->stride;
			src_row += Bitmap->stride;
		}

		s32 bitmap_offset_x = 0; 
		if(x_min < 0)
		{
			bitmap_offset_x = -1 * x_min;
		}
		if(x_max > BackBuffer->width)
		{
			bitmap_offset_x = BackBuffer->width - x_min;
		}

		s32 bitmap_offset_y = 0;
		if(y_min < 0)
		{
			bitmap_offset_y = -1 * y_min;
		}
		if(y_max > BackBuffer->height)
		{
			bitmap_offset_y = BackBuffer->height - y_min;
		}

		dest_row = (u8 *)BackBuffer->memory + region_2_y_min * BackBuffer->stride + region_2_x_min * BackBuffer->bytes_per_pixel;
		src_row = (u8 *)Bitmap->memory + bitmap_offset_y * Bitmap->stride + bitmap_offset_x * Bitmap->bytes_per_pixel;
		for(s32 y = region_2_y_min; y < region_2_y_max; y++)
		{
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for(s32 x = region_2_x_min; x < region_2_x_max; x++)
			{

				f32 alpha = (f32)((*src >> 24) & 0xFF) / 255.0f;
				alpha *= c_alpha;

				f32 src_r = (f32)((*src >> 16) & 0xFF);
				f32 src_g = (f32)((*src >> 8) & 0xFF);
				f32 src_b = (f32)((*src >> 0) & 0xFF);

				f32 dest_r = (f32)((*dest >> 16) & 0xFF);
				f32 dest_g = (f32)((*dest >> 8) & 0xFF);
				f32 dest_b = (f32)((*dest >> 0) & 0xFF);

				f32 r = (1.0f - alpha) * dest_r + alpha * src_r;
				f32 g = (1.0f - alpha) * dest_g + alpha * src_g; 
				f32 b = (1.0f - alpha) * dest_b + alpha * src_b;

				*dest = (((u32)(r + 0.5f) << 16) | ((u32)(g + 0.5f) << 8) | ((u32)(b + 0.5f) << 0));

				dest++;
				src++;
			}
			dest_row += BackBuffer->stride;
			src_row += Bitmap->stride;
		}
	}
}

internal void
debug_vector_draw_at_point(back_buffer * BackBuffer, v2f Point, v2f Direction, v3f Color)
{
	f32 c = 60.0f;
	line_dda_draw(BackBuffer, Point, Point + c * Direction, Color.r, Color.g, Color.b);
}


// NOTE(Jusitn): Confirmed that this works with not only triangles but with
// the convex polygon of the player spacesphip bounding polygon.

internal interval 
sat_projected_interval(v2f *Vertices, u32 vertex_count, v2f ProjectedAxis)
{
	interval Result = {};

	f32 min = f32_infinity();
	f32 max = f32_neg_infinity();
	for(u32 vertex_i = 0; vertex_i < vertex_count; vertex_i++)
	{
		v2f Vertex = Vertices[vertex_i];

		f32 c = v2f_dot(Vertex, ProjectedAxis);
		min = MIN(c, min);
		max = MAX(c, max);
	}

	Result.min = min;
	Result.max = max;

	return(Result);
}

internal interval
circle_project_onto_axis(circle *Circle, v2f ProjectedAxis)
{
	interval Result = {};

	f32 min = f32_infinity();
	f32 max = f32_neg_infinity();

	v2f CircleMin = circle_support_point(Circle, -1.0f * ProjectedAxis);
	v2f CircleMax = circle_support_point(Circle, ProjectedAxis);

	f32 projected_min = v2f_dot(CircleMin, ProjectedAxis);
	f32 projected_max = v2f_dot(CircleMax, ProjectedAxis);

	min = MIN(projected_min, min);
	max = MAX(projected_max, max);

	Result.min = min;
	Result.max = max;

	return(Result);
}

internal v2f
triangle_closest_point_to_circle(triangle *Triangle, circle *Circle)
{
	v2f Result = {};

	v2f ClosestPoint = Triangle->Vertices[0];
	v2f CenterToVertex = ClosestPoint - Circle->Center;
	f32 min_sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
	for(u32 vertex_i = 1; vertex_i < ARRAY_COUNT(Triangle->Vertices); vertex_i++)
	{
		v2f Vertex = Triangle->Vertices[vertex_i];
		CenterToVertex = Vertex - Circle->Center;
		f32 sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
		if(sq_distance < min_sq_distance)
		{
			ClosestPoint = Vertex;
			min_sq_distance = sq_distance;
		}
	}
	Result = ClosestPoint;

	return(Result);
}

// TODO(Justin): Could Pass two entities in if the sat_collision does in fact handle all
// convex polygons

internal v2f 
closest_point_to_circle(v2f *Vertices, u32 vertex_count, circle *Circle)
{
	v2f Result = {};

	v2f ClosestPoint = Vertices[0];
	v2f CenterToVertex = ClosestPoint - Circle->Center;
	f32 min_sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
	for(u32 vertex_i = 1; vertex_i < vertex_count; vertex_i++)
	{
		v2f Vertex = Vertices[vertex_i];
		CenterToVertex = Vertex - Circle->Center;
		f32 sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
		if(sq_distance < min_sq_distance)
		{
			ClosestPoint = Vertex;
			min_sq_distance = sq_distance;
		}
	}
	Result = ClosestPoint;

	return(Result);
}


internal b32
circles_collision(circle *CircleA, circle *CircleB)
{
	b32 Colliding = false;

	v2f CenterBToCenterA = CircleA->Center - CircleB->Center;

	f32 distance_squared = v2f_dot(CenterBToCenterA, CenterBToCenterA);
	f32 radii_squared_sum = SQUARE(CircleA->radius + CircleB->radius);

	if(distance_squared < radii_squared_sum)
	{
		Colliding = true;
	}

	return(Colliding);
}

// TODO(Justin): Need to figure the shapes associated with the entities. ATM they
// this only works in a special case and was implemented in this way as a step
// towards the final game.

// TODO(Jusitn): I do not like the idea of testing circles in the sat_collision
// algorithm. The sat collision detection is for convex polygons. Now we can
// test against a polygon and another shape but different work is involved and
// is the current implementation

internal b32
sat_collision(entity *EntityA, entity *EntityB)
{
	b32 GapExists = true;

	entity *EntityWithPoly;
	entity *EntityWithOutPoly;

	if(EntityA->shape == SHAPE_POLY)
	{
		EntityWithPoly = EntityA;
		EntityWithOutPoly = EntityB;
	}
	else
	{
		EntityWithPoly = EntityB;
		EntityWithOutPoly = EntityA;
	}

	circle Circle = circle_init(EntityWithOutPoly->Pos, EntityWithOutPoly->radius);
	for(u32 vertex_i = 0; vertex_i < ARRAY_COUNT(EntityWithPoly->Poly.Vertices); vertex_i++)
	{
		v2f P0 = EntityWithPoly->Poly.Vertices[vertex_i];
		v2f P1 = EntityWithPoly->Poly.Vertices[(vertex_i + 1) % ARRAY_COUNT(EntityWithPoly->Poly.Vertices)];
		v2f D = P1 - P0;

		v2f ProjectedAxis = v2f_normalize(-1.0f * v2f_perp(D));

		interval PolyInterval = sat_projected_interval(EntityWithPoly->Poly.Vertices,
				ARRAY_COUNT(EntityWithPoly->Poly.Vertices), ProjectedAxis);

		interval CircleInterval = circle_project_onto_axis(&Circle, ProjectedAxis);

		if(CircleInterval.min > CircleInterval.max)
		{
			f32 temp = CircleInterval.min;
			CircleInterval.min = CircleInterval.max;
			CircleInterval.max = temp;
		}

		if(!((CircleInterval.max >= PolyInterval.min) &&
					(PolyInterval.max >= CircleInterval.min)))
		{

			GapExists = false;
		}
	}

	v2f ClosestPoint = closest_point_to_circle(EntityWithPoly->Poly.Vertices,
			ARRAY_COUNT(EntityWithPoly->Poly.Vertices), &Circle);

	v2f ProjectedAxis = v2f_normalize(ClosestPoint - Circle.Center);

	interval PolyInterval = sat_projected_interval(EntityWithPoly->Poly.Vertices,
			ARRAY_COUNT(EntityWithPoly->Poly.Vertices), ProjectedAxis);

	interval CircleInterval = circle_project_onto_axis(&Circle, ProjectedAxis);

	if(!((CircleInterval.max >= PolyInterval.min) &&
				(PolyInterval.max >= CircleInterval.min)))
	{
		GapExists = false;
	}

	return(GapExists);
}

#if 0
internal b32
triangle_circle_collision(triangle *Triangle, circle *Circle)
{
	// NOTE(Justin): The collision detection algorithim is an implementation of the Separated Axis
	// Algorithm.

	b32 GapExists = false;

	for(u32 vertex_i = 0; vertex_i < ARRAY_COUNT(Triangle->Vertices); vertex_i++)
	{

		// Define edges s.t. a normal to the edge, the projection axis,
		// can be used as the interval for the intersection test.

		v2f P0 = Triangle->Vertices[vertex_i];
		v2f P1 = Triangle->Vertices[(vertex_i + 1) % ARRAY_COUNT(Triangle->Vertices)];
		v2f D = P1 - P0;

		// NOTE(Justin): The triangle's vertices are sorted into CCW order.
		// The perpendicular vector of an edge (x, y) -> (-y, x) is the perpendicular
		// pointing INTO the triangle. Why? The perp operation is a CCW
		// operation. Meaning that the result is a vector from rotating the
		// input 90 degrees CCW. 
		// Therefore use (y, -x) as the perpendicualr to the edge, which points
		// outside.

		v2f ProjectedAxis = v2f_normalize(-1.0f * v2f_perp(D));

		projected_interval TriangleInterval = triangle_project_onto_axis(Triangle, ProjectedAxis);
		projected_interval CircleInterval = circle_project_onto_axis(Circle, ProjectedAxis);

		// TODO(Justin): Why is there uncertainty of the order of the points of
		// the interval?

		if(CircleInterval.min > CircleInterval.max)
		{
			f32 temp = CircleInterval.min;
			CircleInterval.min = CircleInterval.max;
			CircleInterval.max = temp;
		}

		if(!((CircleInterval.max >= TriangleInterval.min) &&
					(TriangleInterval.max >= CircleInterval.min)))
		{

			return(GapExists);
		}
	}

	v2f ClosestPoint = triangle_closest_point_to_circle(Triangle, Circle);
	v2f ProjectedAxis = v2f_normalize(ClosestPoint - Circle->Center);

	projected_interval TriangleInterval = triangle_project_onto_axis(Triangle, ProjectedAxis);
	projected_interval CircleInterval = circle_project_onto_axis(Circle, ProjectedAxis);

	if(!((CircleInterval.max >= TriangleInterval.min) &&
				(TriangleInterval.max >= CircleInterval.min)))
	{
		return(GapExists);
	}
	return(!GapExists);
}
#endif


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

inline b32
entities_are_same(entity *EntityA, entity *EntityB)
{
	b32 Result = false;

	if(EntityA->index == EntityB->index)
	{
		Result = true;
	}

	return(Result);
}

internal triangle
player_triangle(game_state *GameState, entity *EntityPlayer)
{
	triangle Result = {};

	Result.Centroid = EntityPlayer->Pos;

	v2f Right = -1.0f * v2f_perp(EntityPlayer->Direction);

	Result.Vertices[0] = Result.Centroid - 0.5f * GameState->Ship.height * EntityPlayer->Direction + (GameState->Ship.width / 2.0f) * Right + -1.0f * EntityPlayer->Direction;
	Result.Vertices[1] = Result.Centroid - 0.5f * GameState->Ship.height * EntityPlayer->Direction + (-1.0f * GameState->Ship.width / 2.0f) * Right + -1.0f * EntityPlayer->Direction;
	Result.Vertices[2] = Result.Centroid + 0.75f * GameState->Ship.height * EntityPlayer->Direction;

	return(Result);
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
collision_handle(entity *EntityA, entity *EntityB)
{
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
	if(EntityNewPos.x > World->Dim.x)
	{
		EntityNewPos.x = EntityNewPos.x - 2.0f * World->Dim.x;
	}
	if(EntityNewPos.x < -World->Dim.x)
	{
		EntityNewPos.x += 2.0f * World->Dim.x;
	}
	if(EntityNewPos.y > World->Dim.y)
	{
		EntityNewPos.y = EntityNewPos.y - 2.0f * World->Dim.y;
	}
	if(EntityNewPos.y < -World->Dim.y)
	{
		EntityNewPos.y += 2.0f * World->Dim.y;
	}

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

	v2f Normal = {};
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
				  (!entity_flag_is_set(TestEntity, ENTITY_FLAG_NON_SPATIAL)))
				{
					if((Entity->shape == SHAPE_CIRCLE) &&
					   (TestEntity->shape == SHAPE_CIRCLE))
					{
						circle CircleA = circle_init(Entity->Pos, Entity->radius);
						circle CircleB = circle_init(TestEntity->Pos, TestEntity->radius);

						if(circles_collision(&CircleA, &CircleB))
						{
							Normal = v2f_normalize(Entity->Pos - TestEntity->Pos);
							HitEntity = TestEntity;
						}
					}
					else
					{
						if(sat_collision(TestEntity, Entity))
						{ 
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

	if(HitEntity)
	{
		if(Entity->type == ENTITY_PLAYER)
		{
			if(!Entity->is_shielded)
			{
				EntityNewPos = V2F(0.0f, 0.0f);
				EntityNewVel = {};
				Entity->is_shielded = true;
			}
			else
			{
				Entity->HitPoints.count--;
				if(Entity->HitPoints.count == 0)
				{
					Entity->is_shielded = false;
				}

			}
			// TODO(Justin): The ship always moves the same after a collision.
			// Need the ship to do something after a collision?

			Entity->Pos += EntityDelta;
			Entity->dPos = EntityNewVel;
		}
		else if((Entity->type == ENTITY_ASTEROID) &&
				(HitEntity->type == ENTITY_ASTEROID))
		{
				Entity->dPos = Entity->speed * Normal;
				Entity->Pos += dt * Entity->dPos;
		}
		else if((Entity->type == ENTITY_ASTEROID) &&
				(HitEntity->type == ENTITY_PLAYER))
		{

			//Entity->dPos = Entity->speed * Normal;
			//Entity->dPos = Entity->dPos - 2.0f * v2f_dot(Entity->dPos, Normal) * Normal;
			Entity->dPos = Entity->dPos - 2.0f * v2f_dot(Entity->dPos, Normal) * Normal;
			Entity->Pos = EntityNewPos;
		}
	}
	else
	{
		Entity->Pos = EntityNewPos;
		Entity->dPos = EntityNewVel;
	}

	if(Entity->type == ENTITY_PLAYER)
	{
		player_polygon_update(Entity);
	}
}


internal triangle
triangle_init(v2f *Vertices, u32 vertex_count)
{
	// NOTE(Justin): Vertices are sorted into CCW order.

	ASSERT(vertex_count == 3);

	triangle Result = {};

	v2f *Right = Vertices;
	v2f *Middle = Vertices + 1;
	v2f *Left = Vertices + 2;

	if(Right->x < Middle->x)
	{
		v2f *Temp = Right;
		Right = Middle;
		Middle = Temp;
	}
	if(Right->x < Left->x)
	{
		v2f *Temp = Right;
		Right = Left;
		Left = Temp;
	}
	if(Middle->x < Left->x)
	{
		v2f *Temp = Middle;
		Middle = Left;
		Left = Temp;
	}

	Result.Centroid.x = ((Right->x + Middle->x + Left->x) / 3);
	Result.Centroid.y = ((Right->y + Middle->y + Left->y) / 3);

	Result.Vertices[0] = *Right;
	Result.Vertices[1] = *Middle;
	Result.Vertices[2] = *Left;

	return(Result);
}

internal entity *
player_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_PLAYER);

	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);
	Entity->is_shooting = false;
	Entity->is_warping = false;
	Entity->is_shielded = true;

	Entity->height = 18.0f;
	Entity->base_half_width = 15.0f;

	Entity->Pos = V2F(0.0f, 0.0f);
	Entity->Right = V2F(1.0f, 0.0f);
	Entity->Direction = V2F(0.0f, 1.0f);
	Entity->speed = 60.0f;

	Entity->hit_point_max = 3;
	Entity->HitPoints.count = Entity->hit_point_max;

	Entity->shape = SHAPE_POLY;

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
asteroid_add(game_state *GameState, asteroid_size SIZE)
{
	entity *Entity = entity_add(GameState, ENTITY_ASTEROID);

	Entity->exists = true;
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

	Entity->exists = true;
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

	Entity->speed = 85.0f;
	Entity->dPos = Entity->speed * Entity->Direction;

	return(Entity);
}

#if 0
internal entity *
triangle_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_TRIANGLE);

	Entity->exists = true;
	entity_flag_set(Entity, ENTITY_FLAG_COLLIDES);
	//Entity->collides = true;

	world *World = GameState->World;

	Entity->Pos = V2F(f32_rand_between(-World->Dim.x, World->Dim.x),
					  f32_rand_between(World->Dim.y, World->Dim.y));

	Entity->Direction = v2f_normalize(V2F(f32_rand_between(-1.0f, 1.0f),
										  f32_rand_between(-1.0f, 1.0f)));
	Entity->speed = 10.0f;
	Entity->dPos = Entity->speed * Entity->Direction;

	Entity->TriangleHitBox.Vertices[0] = V2F((f32)World->Dim.x / 2.0f + 150.0f, (f32)World->Dim.y / 2.0f - 150.0f);
	Entity->TriangleHitBox.Vertices[1] = Entity->TriangleHitBox.Vertices[0] + V2F(-20.0f, 20.0f);
	Entity->TriangleHitBox.Vertices[2] = Entity->TriangleHitBox.Vertices[0] + V2F(-40.0f, 0.0f);

	triangle_init(&Entity->TriangleHitBox.Vertices[0], 3);

	return(Entity);
}
#endif


internal entity *
circle_add(game_state *GameState, v2f Pos, v2f Dir)
{
	entity *Entity = entity_add(GameState, ENTITY_CIRCLE);

	Entity->exists = true;
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

	Entity->exists = true;
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
square_draw(back_buffer *BackBuffer, v2f Pos, f32 half_width, v3f Color)
{
	v2f HalfDim = V2F(half_width, half_width);

	v2f UpperRight = Pos + HalfDim;
	v2f UpperLeft = UpperRight - 2.0f * V2F(half_width, 0.0f);
	v2f LowerLeft = Pos - HalfDim; 
	v2f LowerRight = LowerLeft + 2.0f * V2F(half_width, 0.0f);

	line_dda_draw(BackBuffer, UpperRight, UpperLeft, Color.r, Color.g, Color.b);
	line_dda_draw(BackBuffer, UpperLeft, LowerLeft, Color.r, Color.g, Color.b);
	line_dda_draw(BackBuffer, LowerLeft, LowerRight, Color.r, Color.g, Color.b);
	line_dda_draw(BackBuffer, LowerRight, UpperRight, Color.r, Color.g, Color.b);
}

inline void
push_piece(entity_visible_piece_group *PieceGroup, loaded_bitmap *Bitmap,
		v2f Pos, v2f Align, v2f Dim, v4f Color)
{
	ASSERT(PieceGroup->piece_count < ARRAY_COUNT(PieceGroup->Pieces));
	entity_visible_piece *Piece = PieceGroup->Pieces + PieceGroup->piece_count++;
	Piece->Bitmap = Bitmap;
	Piece->Pos = Pos - Align;
	Piece->r = Color.r;
	Piece->g = Color.g;
	Piece->b = Color.b;
	Piece->alpha = Color.a;
	Piece->Dim = Dim;
}

inline void
push_bitmap(entity_visible_piece_group *PieceGroup, loaded_bitmap *Bitmap,
		v2f Pos, v2f Align, f32 alpha = 1.0f)
{
	push_piece(PieceGroup, Bitmap, Pos, Align, V2F(0, 0), V4F(1.0f, 1.0f, 1.0f, alpha));
}

inline void
push_rectangle(entity_visible_piece_group *PieceGroup, v2f Pos, v2f Dim, v4f Color)
{
	push_piece(PieceGroup, 0, Pos, V2F(0, 0), Dim, Color);
}

internal void
triangle_draw(back_buffer *BackBuffer, triangle *Triangle, f32 r, f32 g, f32 b)
{
	v2f Right = Triangle->Vertices[0];
	v2f Middle = Triangle->Vertices[1];
	v2f Left = Triangle->Vertices[2];

	line_dda_draw(BackBuffer, Right, Middle, r, g, b);
	line_dda_draw(BackBuffer, Middle, Left, r, g, b); 
	line_dda_draw(BackBuffer, Left, Right, r, g, b);
}

internal void
triangle_draw(back_buffer *BackBuffer, triangle *Triangle, v3f Color)
{
	triangle_draw(BackBuffer, Triangle, Color.r, Color.g, Color.b);
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

	//v2f DeltaPos = dt * Entity->dPos;
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

internal v2f
v2f_world_to_screen(game_state *GameState, v2f BottomLeft, v2f Pos)
{

	v2f Result = BottomLeft + 0.5f * GameState->pixels_per_meter * (Pos + GameState->World->Dim);

	return(Result);
}

internal v2f
v2f_screen_to_world(game_state *GameState, v2f ScreenXY)
{
	v2f Result = {};

	world *World = GameState->World;

	Result.x = (2.0f * ScreenXY.x - World->Dim.x * GameState->pixels_per_meter) / GameState->pixels_per_meter;
	Result.y = (2.0f * ScreenXY.y - World->Dim.y * GameState->pixels_per_meter) / GameState->pixels_per_meter;

	return(Result);
}


internal void
player_polygon_draw(back_buffer *BackBuffer, game_state *GameState, v2f BottomLeft, entity *EntityPlayer)
{
	v2f FrontLeft = EntityPlayer->Poly.Vertices[0];
	v2f SideLeftEngine = EntityPlayer->Poly.Vertices[1];
	v2f RearLeftEngine = EntityPlayer->Poly.Vertices[2];
	v2f BackCenter = EntityPlayer->Poly.Vertices[3];
	v2f RearRightEngine = EntityPlayer->Poly.Vertices[4];
	v2f SideRightEngine = EntityPlayer->Poly.Vertices[5];
	v2f FrontRight = EntityPlayer->Poly.Vertices[6];

	v2f ScreenPos = v2f_world_to_screen(GameState, BottomLeft, EntityPlayer->Pos);

	v2f ScreenBackCenter = v2f_world_to_screen(GameState, BottomLeft, BackCenter);
	v2f ScreenFrontLeft = v2f_world_to_screen(GameState, BottomLeft, FrontLeft);
	v2f ScreenFrontRight = v2f_world_to_screen(GameState, BottomLeft, FrontRight);
	v2f ScreenRearLeftEngine = v2f_world_to_screen(GameState, BottomLeft, RearLeftEngine);
	v2f ScreenRearRightEngine = v2f_world_to_screen(GameState, BottomLeft, RearRightEngine);
	v2f ScreenSideLeftEngine = v2f_world_to_screen(GameState, BottomLeft, SideLeftEngine);
	v2f ScreenSideRightEngine = v2f_world_to_screen(GameState, BottomLeft, SideRightEngine);

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

		// NOTE(Justin): Since there is not a ground in the game all the
		// bitmaps, most likely, will be drawn at the center of the object. In
		// order to center the bitmap at the point we just need to subtract off
		// half the width and half the height of the bitmap position and it will
		// be centered at the objects position.

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
		GameState->World = push_size(&GameState->WorldArena, world);

		GameState->pixels_per_meter = 5.0f;

		world *World = GameState->World;
		World->Dim.x = (f32)BackBuffer->width / GameState->pixels_per_meter;
		World->Dim.y = (f32)BackBuffer->height / GameState->pixels_per_meter;

		entity *EntityPlayer = player_add(GameState);
		GameState->player_entity_index = EntityPlayer->index;


		srand(2023);

		// TODO(Justin): This size of the asteroid should be random.
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);
		asteroid_add(GameState, ASTEROID_SMALL);


		m3x3 *M = &GameState->MapToScreenSpace;
		m3x3 *T = &GameState->InverseScreenOffset;

		m3x3 InverseWorldTranslate = m3x3_translation_create(V2F(World->Dim.x, World->Dim.y));
		m3x3 Scale = m3x3_scale_create(GameState->pixels_per_meter / 2.0f);
		m3x3 InverseScreenOffset = m3x3_translation_create(V2F(5.0f, 5.0f));

		*M = Scale * InverseWorldTranslate;
		*T = InverseScreenOffset;

		GameMemory->is_initialized = true;
	}

	v2f BottomLeft = {5.0f, 5.0f};

	world *World = GameState->World;

	entity *EntityPlayer = entity_get(GameState, GameState->player_entity_index);

	bitmap_draw(BackBuffer, &GameState->Background, 0.0f, 0.0f);

	f32 dt = GameInput->dt_for_frame;

	v2f PlayerNewRight = EntityPlayer->Right;
	v2f PlayerNewDirection = EntityPlayer->Direction;

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
	entity_move(GameState, EntityPlayer, PlayerAccel, dt);

	v2f AsteroidAccel = {};
	for(u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++)
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

	m3x3 MapToScreenSpace = GameState->MapToScreenSpace;
	m3x3 InverseScreenOffset = GameState->InverseScreenOffset;
	entity_visible_piece_group PieceGroup;
	for(u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++)
	{
		PieceGroup.piece_count = 0;
		entity *Entity = GameState->Entities + entity_index;
		switch(Entity->type)
		{
			case ENTITY_PLAYER:
			{
				v2f PlayerPos = Entity->Pos;

				v2f ScreenPos = MapToScreenSpace * PlayerPos;
				ScreenPos = InverseScreenOffset * ScreenPos;

				push_bitmap(&PieceGroup, &GameState->Ship, ScreenPos, GameState->Ship.Align);
				player_polygon_draw(BackBuffer, GameState, BottomLeft, Entity);

				if(Entity->is_shielded)
				{
					circle Circle = circle_init(Entity->Pos, Entity->height);

					Circle.Center = v2f_world_to_screen(GameState, BottomLeft, Circle.Center);
					Circle.radius *= GameState->pixels_per_meter;
					circle_draw(BackBuffer, &Circle, White);

				}
			} break;
			case ENTITY_ASTEROID:
			{
				v2f AsteroidPos = Entity->Pos;
				v2f ScreenPos = MapToScreenSpace * AsteroidPos;
				ScreenPos = InverseScreenOffset * ScreenPos;

				push_bitmap(&PieceGroup, &GameState->AsteroidBitmap, ScreenPos, GameState->AsteroidBitmap.Align);

#if 0
				v2f temp = v2f_normalize(Entity->dPos);
				debug_vector_draw_at_point(BackBuffer, ScreenPos, temp, White);
#endif
			} break;
			case ENTITY_FAMILIAR:
			{
				//familiar_update(GameState, Entity, dt);

				//v2f AsteroidScreenPos = tile_map_get_screen_coordinates(TileMap, &Entity->TileMapPos,
				//		BottomLeft, tile_side_in_pixels, meters_to_pixels);
				//debug_vector_draw_at_point(BackBuffer, AsteroidScreenPos, Entity->Direction);
				//v2f Alignment = {(f32)GameState->AsteroidBitmap.width / 2.0f,
				//				 (f32)GameState->AsteroidBitmap.height / 2.0f};

				//push_piece(&PieceGroup, &GameState->AsteroidBitmap, AsteroidScreenPos, Alignment);

			} break;
			case ENTITY_PROJECTILE:
			{
				projectile_update(GameState, Entity, dt);
				if(!entity_flag_is_set(Entity, ENTITY_FLAG_NON_SPATIAL))
				{
					v2f LaserPos = Entity->Pos;

					v2f ScreenPos = MapToScreenSpace * LaserPos;
					ScreenPos = InverseScreenOffset * ScreenPos;

					push_bitmap(&PieceGroup, &GameState->LaserBlueBitmap, ScreenPos, GameState->LaserBlueBitmap.Align);
				}
			} break;
			case ENTITY_TRIANGLE:
			{
#if 0
				v2f Pos = Entity->Pos;
				entity_move(GameState, Entity, Entity->dPos, dt);
				v2f NewPos = Entity->Pos;
				v2f Delta = NewPos - Pos;

				for(u32 i = 0; i < 3; i++)
				{
					Entity->TriangleHitBox.Vertices[i] += Delta;
				}
				triangle_draw(BackBuffer, &Entity->TriangleHitBox, White);
#endif

			} break;
			case ENTITY_CIRCLE:
			{
				entity_move(GameState, Entity, Entity->dPos, dt);
				circle EntityCircle = circle_init(Entity->Pos, Entity->radius);
				circle_draw(BackBuffer, &EntityCircle, White);
			} break;
			case ENTITY_SQUARE:
			{
				square S;
				S.Center = Entity->Pos;
				S.half_width = Entity->radius;

				square_draw(BackBuffer, S.Center, S.half_width, White);
			} break;
			default:
			{
				INVALID_CODE_PATH;
			} break;
		}

		for(u32 piece_index = 0; piece_index < PieceGroup.piece_count; piece_index++)
		{
			entity_visible_piece *Piece = PieceGroup.Pieces + piece_index;

			f32 x = Piece->Pos.x;
			f32 y = Piece->Pos.y;

			if(Piece->Bitmap)
			{
				bitmap_draw(BackBuffer, Piece->Bitmap, x, y, Piece->alpha);
			}
			else
			{
			}
		}
	}
}
