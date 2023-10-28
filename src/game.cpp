/*
 * TODO:
 *	- Collision Detection
 *		- Minkoski based collision detection
 *		- Asteroid collisions
 *		- Projectile collisions
 *
 *	- Hash table tile map storage?
 *		- Use has value to get tiles that are actually set
 *		- No need to loop through entire tile map

 * 	- Asset loading
 * 	- VfX
 *		- Animations
 *			- Asteroid destruction
 *			- Lasers/beams
 *			- Warping
 *			- Shield
 *				- Appears on collision
 *				- Fades shortly thereafter
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

 * 	- Audio mixing

 * 	- Bitmap transformations (rotations, scaling, ...)
 * 	- UV coordinate mapping
 * 	- Normal mapping
 * 	- Asteroid collision 
 * 	- Angular momentum
*/

#include "game.h"
#include "game_string.cpp"
#include "game_tile.cpp"
#include "game_asset.cpp"

internal void
debug_sound_buffer_fill(sound_buffer *SoundBuffer)
{
	s16 wave_amplitude = 3000;
	int wave_frequency = 256;
	int samples_per_period = SoundBuffer->samples_per_second / wave_frequency;

	local_persist f32 t = 0.0f;
	f32 dt = 2.0f * PI32 / samples_per_period;

	s16 *sample = SoundBuffer->samples;
	for (int sample_index = 0; sample_index < SoundBuffer->sample_count; sample_index++) {

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
	for (s32 sample_index = 0; sample_index < SoundBuffer->sample_count; sample_index++) {
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

	if (x_min < 0) {
		x_min += BackBuffer->width;
	}
	if (x_max > BackBuffer->width) {
		x_max -= BackBuffer->width;
	}
	if (y_min < 0) {
		y_min += BackBuffer->height;
	}
	if (y_max > BackBuffer->height) {
		y_max -= BackBuffer->height;
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min ;
	for (int row = y_min; row < y_max; row++) {
		u32 *pixel = (u32 *)pixel_row;
		for (int col = x_min; col < x_max; col++)  {
			*pixel++ = color;
		}
		pixel_row += BackBuffer->stride;
	}
}

internal void
pixel_set(back_buffer *BackBuffer, v2f ScreenXY, u32 color)
{
	s32 screen_x = f32_round_to_s32(ScreenXY.x);
	s32 screen_y = f32_round_to_s32(ScreenXY.y);

	if (screen_x >= BackBuffer->width) {
		screen_x = screen_x - BackBuffer->width;
	}
	if (screen_x < 0) { 
		screen_x = screen_x + BackBuffer->width;
	}
	if (screen_y >= BackBuffer->height) {
		screen_y = screen_y - BackBuffer->height;
	}
	if (screen_y < 0) {
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

	if (ABS(Delta.x) > ABS(Delta.y)) {
		step_count = (u32)ABS(Delta.x);
	} else {
		step_count = (u32)ABS(Delta.y);
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	v2f Step = (1.0f / (f32)step_count) * Delta;
	for (u32 k = 0; k < step_count; k++) {
		Pos += Step;
		pixel_set(BackBuffer, Pos, color);
	}
}

internal bounding_box
circle_bounding_box_find(circle Circle)
{
	bounding_box Result = {0, 0, 0, 0};

	Result.Min.x = Circle.Center.x - Circle.radius;
	Result.Min.y = Circle.Center.y - Circle.radius;
	Result.Max.x = Result.Min.x + 2 * Circle.radius;
	Result.Max.y = Result.Min.y + 2 * Circle.radius;

	return(Result);
}

internal v2f
circle_support_point(circle *Circle, v2f Dir)
{
	v2f Result = {};
	Result = Circle->Center + Circle->radius * Dir;
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


	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min
											 + BackBuffer->bytes_per_pixel * x_min;

	//v2f Center = point_map_to_screen(GameState, BackBuffer, Circle->Center);
	//f32 radius = GameState->pixels_per_meter * Circle->radius;
	v2f Center = Circle->Center;
	f32 radius =  Circle->radius;
	for (s32 row = y_min; row < y_max; row++) {
		u32 *pixel = (u32 *)pixel_row;
		for (s32 col = x_min; col < x_max; col++) {
			f32 dx = Center.x - col;
			f32 dy = Center.y - row;

			f32 d = (f32)sqrt((dx * dx) + (dy * dy));

			if (d <= radius) {
				*pixel++ = color;
			} else {
				pixel++;
			}
		}
		pixel_row += BackBuffer->stride;
	}
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
	for (s32 y = 0; y < BackBuffer->height; y++) {
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
	for (s32 x = 0; x < BackBuffer->width; x++) {
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

	// Check bounds
	if (x_min < 0) {
		x_min = 0;
	}
	if (x_max > BackBuffer->width) {
		x_max = BackBuffer->width;
	}
	if (y_min < 0) {
		y_min = 0;
	}
	if (y_max > BackBuffer->height) {
		y_max = BackBuffer->height;
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min;
	for (int row = y_min; row < y_max; row++) {

		u32 *pixel = (u32 *)pixel_row;
		for (int col = x_min; col < x_max; col++)  {

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
	if (x_min < 0) {
		region_1_x_min = x_min + BackBuffer->width;
		region_1_x_max = BackBuffer->width;
		region_2_x_min = 0;
		region_2_x_max = x_max;
		bitmap_across_x_boundary = true;
	}
	if (y_min < 0) {
		region_1_y_min = y_min + BackBuffer->height;;
		region_1_y_max = BackBuffer->height;
		region_2_y_min = 0;
		region_2_y_max = y_max;
		bitmap_across_y_boundary = true;
	}
	if (x_max > BackBuffer->width) {
		region_1_x_min = x_min;
		region_1_x_max = BackBuffer->width;
		region_2_x_min = 0;
		region_2_x_max = x_max - BackBuffer->width; 
		bitmap_across_x_boundary = true;
	}
	if (y_max > BackBuffer->height) {
		region_1_y_min = y_min;
		region_1_y_max = BackBuffer->height;
		region_2_y_min = 0;
		region_2_y_max = y_max - BackBuffer->height;
		bitmap_across_y_boundary = true;
	}

	if (!(bitmap_across_x_boundary || bitmap_across_y_boundary)) {
	u8 *dest_row = (u8 *)BackBuffer->memory + y_min * BackBuffer->stride + x_min * BackBuffer->bytes_per_pixel;
	u8 *src_row = (u8 *)Bitmap->memory;
	for (s32 y = y_min; y < y_max; y++) {
		u32 *src = (u32 *)src_row;
		u32 *dest = (u32 *)dest_row;
		for (s32 x = x_min; x < x_max; x++) {

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
	} else {

		if (!bitmap_across_y_boundary) {
			region_1_y_min = y_min;
			region_1_y_max = y_max;

			region_2_y_min = region_1_y_min;
			region_2_y_max = region_1_y_max;
		} else if (!bitmap_across_x_boundary) {
			region_1_x_min = x_min;
			region_1_x_max = x_max;

			region_2_x_min = region_1_x_min;
			region_2_x_max = region_1_x_max;
		}

		u8 *dest_row = (u8 *)BackBuffer->memory + region_1_y_min * BackBuffer->stride + region_1_x_min * BackBuffer->bytes_per_pixel;
		u8 *src_row = (u8 *)Bitmap->memory;
		for (s32 y = region_1_y_min; y < region_1_y_max; y++) {
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for (s32 x = region_1_x_min; x < region_1_x_max; x++) {

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
		if (x_min < 0) {
			bitmap_offset_x = -1 * x_min;
		}
		if (x_max > BackBuffer->width) {
			bitmap_offset_x = BackBuffer->width - x_min;
		}

		s32 bitmap_offset_y = 0;
		if (y_min < 0) {
			bitmap_offset_y = -1 * y_min;
		}
		if (y_max > BackBuffer->height) {
			bitmap_offset_y = BackBuffer->height - y_min;
		}

		dest_row = (u8 *)BackBuffer->memory + region_2_y_min * BackBuffer->stride + region_2_x_min * BackBuffer->bytes_per_pixel;
		src_row = (u8 *)Bitmap->memory + bitmap_offset_y * Bitmap->stride + bitmap_offset_x * Bitmap->bytes_per_pixel;
		for (s32 y = region_2_y_min; y < region_2_y_max; y++) {
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for (s32 x = region_2_x_min; x < region_2_x_max; x++) {

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
debug_vector_draw_at_point(back_buffer * BackBuffer, v2f Point, v2f Direction)
{
	f32 c = 60.0f;
	line_dda_draw(BackBuffer, Point, Point + c * Direction, 1.0f, 1.0f, 1.0f);
}

internal b32
test_tile_side(f32 max_corner_x, f32 rel_x, f32 rel_y, f32 *t_min,
		  f32 player_delta_x, f32 player_delta_y, f32 min_corner_y, f32 max_corner_y)
{
	// NOTE(Justin): Using epsilons is not ideal..
	b32 hit = false;
	f32 epsilon = 0.001f;
	f32 wall_x = max_corner_x;
	if (player_delta_x != 0) {
		f32 t_result = (wall_x - rel_x) / player_delta_x;
		f32 y = rel_y + t_result * player_delta_y;
		if ((t_result >= 0.0f) && (*t_min > t_result)) {
			if ((y >= min_corner_y) && (y <= max_corner_y)) {
				*t_min = MAX(0.0f, t_result - epsilon);
				hit = true;
			}
		}
	}
	return(hit);
}

internal b32
triangle_circle_collision(triangle *Triangle, circle *Circle)
{
	b32 GapExists = false;
	for (u32 vertex_i = 0; vertex_i < ARRAY_COUNT(Triangle->Vertices); vertex_i++) {

		// Define edges s.t. a normal to the edge, the projection axis,
		// can be used as the interval for the intersection test.

		v2f P0 = Triangle->Vertices[vertex_i];
		v2f P1 = Triangle->Vertices[(vertex_i + 1) % ARRAY_COUNT(Triangle->Vertices)];
		v2f D = P1 - P0;
		v2f ProjectedAxis = V2F(D.y, -D.x);
		ProjectedAxis = v2f_normalize(ProjectedAxis);

		f32 triangle_min_projected, triangle_max_projected;
		triangle_min_projected = f32_infinity();
		triangle_max_projected = f32_neg_infinity();
		for (u32 vertex_j = 0; vertex_j < ARRAY_COUNT(Triangle->Vertices); vertex_j++) {
			v2f Vertex = Triangle->Vertices[vertex_j];

			f32 c = v2f_dot(Vertex, ProjectedAxis);
			triangle_min_projected = MIN(c, triangle_min_projected);
			triangle_max_projected = MAX(c, triangle_max_projected);
		}

		f32 circle_min_projected, circle_max_projected;

		circle_min_projected = f32_infinity();
		circle_max_projected = f32_neg_infinity();

		v2f CircleMin = circle_support_point(BackBuffer, Circle, -1.0f * ProjectedAxis);
		f32 c = v2f_dot(CircleMin, ProjectedAxis);

		circle_min_projected = MIN(c, circle_min_projected);

		v2f CircleMax = circle_support_point(BackBuffer, Circle, ProjectedAxis);
		c = v2f_dot(CircleMax, ProjectedAxis);
		circle_max_projected = MAX(c, circle_max_projected);

		if (circle_min_projected > circle_max_projected) {
			f32 temp = circle_min_projected;
			circle_min_projected = circle_max_projected;
			circle_max_projected = temp;
		}

		if (!((circle_max_projected >= triangle_min_projected) &&
					(triangle_max_projected >= circle_min_projected))) {

			return(GapExists);
		}
	}

	v2f ClosestPoint = Triangle->Vertices[0];
	v2f CenterToVertex = ClosestPoint - Circle->Center;
	f32 min_sq_distance = v2f_dot(CenterToVertex, CenterToVertex);

	for (u32 vertex_i = 1; vertex_i < ARRAY_COUNT(Triangle->Vertices); vertex_i++) {
		v2f Vertex = Triangle->Vertices[vertex_i];
		CenterToVertex = Vertex - Circle->Center;
		f32 sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
		if (sq_distance < min_sq_distance) {
			ClosestPoint = Vertex;
			min_sq_distance = sq_distance;
		}
	}
	v2f ProjectedAxis = v2f_normalize(ClosestPoint - Circle->Center);

	f32 triangle_min_projected = f32_infinity();
	f32 triangle_max_projected = f32_neg_infinity();
	for (u32 vertex_j = 0; vertex_j < ARRAY_COUNT(Triangle->Vertices); vertex_j++) {
		v2f Vertex = Triangle->Vertices[vertex_j];

		f32 c = v2f_dot(Vertex, ProjectedAxis);
		triangle_min_projected = MIN(c, triangle_min_projected);
		triangle_max_projected = MAX(c, triangle_max_projected);
	}

	f32 circle_min_projected = f32_infinity();
	f32 circle_max_projected = f32_neg_infinity();

	v2f CircleMin = circle_support_point(BackBuffer, Circle, -1.0f * ProjectedAxis);
	f32 c = v2f_dot(CircleMin, ProjectedAxis);
	circle_min_projected = MIN(c, circle_min_projected);

	v2f CircleMax = circle_support_point(BackBuffer, Circle, ProjectedAxis);
	c = v2f_dot(CircleMax, ProjectedAxis);
	circle_max_projected = MAX(c, circle_max_projected);

	if (!((circle_max_projected >= triangle_min_projected) &&
				(triangle_max_projected >= circle_min_projected))) {

		return(GapExists);
	}
	return(!GapExists);
}


internal void
entity_move(game_state *GameState, entity *Entity, v2f ddPos, f32 dt_for_frame)
{
	tile_map *TileMap = GameState->TileMap;
	f32 ddp_length_squared = v2f_length_squared(ddPos);
	if (ddp_length_squared > 1.0f) {
		ddPos *= HALF_ROOT_TWO;
		//ddPos *= 1 / f32_sqrt(ddp_length_squared);
	}

	ddPos *= Entity->speed;
	

	v2f dPos;
	dPos = dt_for_frame * ddPos + Entity->dPos;
	Entity->dPos = dPos;

	v2f EntityDelta = 0.5f * ddPos * SQUARE(dt_for_frame) + dt_for_frame * Entity->dPos;

	tile_map_position EntityOldPos = Entity->TileMapPos;
	tile_map_position EntityNewPos = Entity->TileMapPos;
	EntityNewPos.TileOffset += EntityDelta;
	EntityNewPos = tile_map_position_remap(TileMap, EntityNewPos);

	s32 tile_min_x = MIN(EntityOldPos.Tile.x, EntityNewPos.Tile.x);
	s32 tile_min_y = MIN(EntityOldPos.Tile.y, EntityNewPos.Tile.y);
	s32 tile_max_x_one_past = MAX(EntityOldPos.Tile.x, EntityNewPos.Tile.x) + 1;
	s32 tile_max_y_one_past = MAX(EntityOldPos.Tile.y, EntityNewPos.Tile.y) + 1;

	// TODO(Justin): Collision detection against asteroids not tiles. Tiles
	// should be used to check whether or not something exists in it. If
	f32 t_min = 1.0f;
	v2f TileNormal = {};
	b32 collided = false;
	for (s32 tile_y = tile_min_y; tile_y != tile_max_y_one_past; tile_y++) {
		for (s32 tile_x = tile_min_x; tile_x != tile_max_x_one_past; tile_x++) {
			tile_map_position TestTilePos = tile_map_get_centered_position(tile_x, tile_y);
			u32 tile_value = tile_map_get_tile_value_unchecked(TileMap, TestTilePos.Tile);
			if (!tile_map_is_same_tile(&Entity->TileMapPos, &TestTilePos)) {
				if (!tile_map_is_tile_value_empty(tile_value)) {
					v2f MinCorner = -0.5f * V2F(TileMap->tile_side_in_meters, TileMap->tile_side_in_meters);
					v2f MaxCorner = 0.5f * V2F(TileMap->tile_side_in_meters, TileMap->tile_side_in_meters);

					tile_map_pos_delta TileRelDelta = tile_map_get_pos_delta(TileMap, &EntityOldPos, &TestTilePos);
					v2f Rel = TileRelDelta.dOffset;

					// Right wall
					if (test_tile_side(MaxCorner.x, Rel.x, Rel.y, &t_min, EntityDelta.x, EntityDelta.y,
								MinCorner.y, MaxCorner.y)) {
						TileNormal = {1.0f, 0.0f};
						collided = true;
					}

					// Left wall
					if (test_tile_side(MinCorner.x, Rel.x, Rel.y, &t_min, EntityDelta.x, EntityDelta.y,
								MinCorner.y, MaxCorner.y)) {

						TileNormal= {-1.0f, 0.0f};
						collided = true;
					}

					// Bottom wall
					if (test_tile_side(MinCorner.y, Rel.y, Rel.x, &t_min, EntityDelta.y, EntityDelta.x,
								MinCorner.x, MaxCorner.x)) {

						TileNormal = {0.0f, 1.0f};
						collided = true;
					}

					// Top wall
					if (test_tile_side(MaxCorner.y, Rel.y, Rel.x, &t_min, EntityDelta.y, EntityDelta.x,
								MinCorner.x, MaxCorner.x)) {

						TileNormal = {0.0f, -1.0f};
						collided = true;
					}
				}
			}
		}
	}

	EntityNewPos = EntityOldPos;
	EntityNewPos.TileOffset += t_min * EntityDelta;
	EntityNewPos = tile_map_position_remap(TileMap, EntityNewPos);

	if ((Entity->type == ENTITY_PLAYER) ||
		(Entity->type == ENTITY_ASTEROID) || 
		(Entity->type == ENTITY_FAMILIAR)) {
		if (!tile_map_is_same_tile(&EntityOldPos, &EntityNewPos)) {
			tile_map_tile_set_value(TileMap, EntityOldPos.Tile, TILE_EMPTY);
			tile_map_tile_set_value(TileMap, EntityNewPos.Tile, TILE_OCCUPIED);
		}
	}

	Entity->TileMapPos = EntityNewPos;

	if (Entity->collides) {
		if (collided) {
			if (Entity->type == ENTITY_PLAYER) {
				if (!Entity->is_shielded) {

					tile_map_tile_set_value(TileMap, Entity->TileMapPos.Tile, TILE_EMPTY);

					Entity->dPos = {};
					Entity->TileMapPos = {};
					Entity->TileMapPos.Tile = {TileMap->tile_count_x / 2, TileMap->tile_count_y / 2};
					Entity->HitPoints.count = Entity->hit_point_max;
					Entity->is_shielded = true;

					tile_map_tile_set_value(TileMap, Entity->TileMapPos.Tile, TILE_OCCUPIED);
				} else {
					Entity->HitPoints.count--;
					if(Entity->HitPoints.count == 0)
					{
						Entity->is_shielded = false;
					}
					Entity->dPos = Entity->dPos - 2.0f * v2f_dot(Entity->dPos, TileNormal) * TileNormal;
				}
			} else if (Entity->type == ENTITY_ASTEROID) {
				// TODO(Justin): Asteroids also have hit points ranging from 1
				// to 3. If the asteroid hitpoint goes to 0 then the asteroid is
				// destroyed.
				Entity->dPos = Entity->dPos - 2.0f * v2f_dot(Entity->dPos, TileNormal) * TileNormal;
			}
		}
	}
}

inline entity *
entity_get(game_state *GameState, u32 index)
{
	entity *Result = 0;
	if ((index > 0) && (index < ARRAY_COUNT(GameState->Entities))) {
		Result = GameState->Entities + index;
	}
	return(Result);
}

internal entity *
entity_add(game_state *GameState, entity_type type)
{
	ASSERT(GameState->entity_count < ARRAY_COUNT(GameState->Entities));

	u32 entity_index = GameState->entity_count++;

	entity *Entity = GameState->Entities + entity_index;
	*Entity = {};
	Entity->index = entity_index;
	Entity->type = type;

	return(Entity);
}

internal entity *
player_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_PLAYER);

	tile_map *TileMap = GameState->TileMap;

	Entity->exists = true;
	Entity->collides = true;

	Entity->TileMapPos.Tile.x = TileMap->tile_count_x / 2;
	Entity->TileMapPos.Tile.y = TileMap->tile_count_y / 2;
	Entity->TileMapPos.TileOffset.x = 0.0f;
	Entity->TileMapPos.TileOffset.y = 0.0f;
	Entity->height = 10.0f;
	Entity->base_half_width = 4.0f;
	Entity->Right = {1.0f, 0.0f};
	Entity->Direction = {0.0f, 1.0f};
	Entity->speed = 30.0f;
	Entity->is_shooting = false;
	Entity->is_warping = false;
	Entity->is_shielded = true;

	Entity->hit_point_max = 3;
	Entity->HitPoints.count = Entity->hit_point_max;

	Entity->shape = SHAPE_TRIANGLE;
	Entity->vertex_count = 3;



	tile_map_tile_set_value(TileMap, Entity->TileMapPos.Tile, TILE_OCCUPIED);

	return(Entity);
}

internal entity *
asteroid_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_ASTEROID);

	tile_map *TileMap = GameState->TileMap;

	Entity->exists = true;
	Entity->collides = true;

	f32 x_offset = (12.0f * ((f32)rand() / (f32)RAND_MAX) - 6);
	f32 y_offset = (12.0f * ((f32)rand() / (f32)RAND_MAX) - 6);

	s32 tile_x =  (s32)((TileMap->tile_count_x - 1) * ((f32)rand() / (f32)(RAND_MAX)));
	s32 tile_y =  (s32)((TileMap->tile_count_y - 1) * ((f32)rand() / (f32)(RAND_MAX)));

	Entity->TileMapPos.TileOffset = {x_offset, y_offset};
	Entity->TileMapPos.Tile = {tile_x, tile_y};

	f32 x_dir = ((f32)rand() / (f32)RAND_MAX) - 0.5f;
	f32 y_dir = ((f32)rand() / (f32)RAND_MAX) - 0.5f;

	f32 speed_scale = ((f32)rand() / (f32)RAND_MAX);

	Entity->Direction = {x_dir, y_dir};
	Entity->speed = 9.0f;

	f32 asteroid_speed = speed_scale * Entity->speed;

	Entity->dPos = asteroid_speed * Entity->Direction;

	tile_map_tile_set_value(TileMap, Entity->TileMapPos.Tile, TILE_OCCUPIED);

	// TODO(Jusitn): Rand scaling, orientation, size, and mass.
	
	Entity->shape = SHAPE_CIRCLE;
	Entity->vertex_count = 0;
	return(Entity);
}

internal entity *
familiar_add(game_state *GameState)
{
	entity *Entity = entity_add(GameState, ENTITY_FAMILIAR);

	Entity->exists = true;
	Entity->collides = true;

	Entity->speed = 1.0f;

	tile_map_tile_set_value(GameState->TileMap, Entity->TileMapPos.Tile, TILE_OCCUPIED);

	return(Entity);
}

inline void
push_piece(entity_visible_piece_group *PieceGroup, loaded_bitmap *Bitmap, v2f Offset, v2f Align, f32 alpha = 1.0f)
{
	ASSERT(PieceGroup->piece_count < ARRAY_COUNT(PieceGroup->Pieces));
	entity_visible_piece *Piece = PieceGroup->Pieces + PieceGroup->piece_count++;
	Piece->Bitmap = Bitmap;
	Piece->Offset = Offset - Align;
	Piece->alpha = alpha;
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

internal triangle
triangle_init(v2f *Vertices, u32 vertex_count)
{
	ASSERT(vertex_count == 3);

	triangle Result = {};

	v2f *Right = Vertices;
	v2f *Middle = Vertices + 1;
	v2f *Left = Vertices + 2;

	if (Right->x < Middle->x) {
		v2f *Temp = Right;
		Right = Middle;
		Middle = Temp;
	}
	if (Right->x < Left->x) {
		v2f *Temp = Right;
		Right = Left;
		Left = Temp;
	}
	if (Middle->x < Left->x) {
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

inline void
familiar_update(game_state *GameState, entity *Entity, f32 dt_for_frame)
{
	tile_map *TileMap = GameState->TileMap;
	// TODO(Justin): Spatial partition search
	
	// NOTE(Justin): 10 meter maximum search
	entity Player = {};
	f32 distance_sq_max = SQUARE(100.0f);
	f32 closest_dist = 0.0f;

	v2f PlayerAbsPos = {};
	v2f EntityAbsPos = tile_map_get_absolute_pos(TileMap, Entity->TileMapPos);
	for (u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++) {
		entity *TestEntity = entity_get(GameState, entity_index);
		if (TestEntity->type == ENTITY_PLAYER) {
			PlayerAbsPos = tile_map_get_absolute_pos(TileMap, TestEntity->TileMapPos);
			//EntityAbsPos = tile_map_get_absolute_pos(TileMap, Entity->TileMapPos);
			f32 test_distance_sq = v2f_length_squared(EntityAbsPos - PlayerAbsPos);
			if (test_distance_sq < distance_sq_max) {
				Player = *TestEntity;
				closest_dist = test_distance_sq;
			}
		}
	}
	v2f ddPos = {};
	if (closest_dist > 0.0f) {
		f32 acceleration = 1.0f;
		f32 one_over_length = acceleration / f32_sqrt(distance_sq_max);
		ddPos = one_over_length * (PlayerAbsPos - EntityAbsPos);
		Entity->Direction = ddPos;
	}
	entity_move(GameState, Entity, ddPos, dt_for_frame);
}


internal void
update_and_render(game_memory *GameMemory, back_buffer *BackBuffer, sound_buffer *SoundBuffer, game_input *GameInput)
{
	ASSERT(sizeof(game_state) <= GameMemory->total_size);
	game_state *GameState = (game_state *)GameMemory->permanent_storage;
	if (!GameMemory->is_initialized) {

		// NOTE(Justin): Reserve slot 0 for null entity.

		entity *EntityNull = entity_add(GameState, ENTITY_NULL);

		//GameState->TestSound = wav_file_read_entire("sfx/bloop_00.wav");
		GameState->Background = bitmap_file_read_entire("space_background.bmp");
		GameState->Ship = bitmap_file_read_entire("ship/blueships1.bmp");

		GameState->WarpFrames[0] = bitmap_file_read_entire("vfx/warp_01.bmp");
		GameState->WarpFrames[1] = bitmap_file_read_entire("vfx/warp_02.bmp");
		GameState->WarpFrames[2] = bitmap_file_read_entire("vfx/warp_03.bmp");
		GameState->WarpFrames[3] = bitmap_file_read_entire("vfx/warp_04.bmp");
		GameState->WarpFrames[4] = bitmap_file_read_entire("vfx/warp_05.bmp");
		GameState->WarpFrames[5] = bitmap_file_read_entire("vfx/warp_06.bmp");
		GameState->WarpFrames[6] = bitmap_file_read_entire("vfx/warp_07.bmp");
		GameState->WarpFrames[7] = bitmap_file_read_entire("vfx/warp_08.bmp");

		GameState->AsteroidSprite = bitmap_file_read_entire("asteroids/01.bmp");
		GameState->LaserBlue = bitmap_file_read_entire("lasers/laser_small_blue.bmp");

		memory_arena_initialize(&GameState->TileMapArena, GameMemory->total_size - sizeof(game_state),
				(u8 *)GameMemory->permanent_storage + sizeof(game_state));


		GameState->TileMap = push_array(&GameState->TileMapArena, 1, tile_map);
		tile_map *TileMap = GameState->TileMap;

		TileMap->tile_count_x = 17;
		TileMap->tile_count_y = 9;
		TileMap->tile_side_in_pixels = 56;
		TileMap->tile_side_in_meters = 12.0f;
		TileMap->meters_to_pixels = TileMap->tile_side_in_pixels / TileMap->tile_side_in_meters;
		TileMap->tiles = push_array(&GameState->TileMapArena, TileMap->tile_count_x * TileMap->tile_count_y, u32);


		entity *EntityPlayer = player_add(GameState);
		GameState->player_entity_index = EntityPlayer->index;

		GameState->projectile_next = 0;
		GameState->projectile_speed = 30.0f;
		GameState->projectile_half_width = 1.0f;

		f32 asteroid_scales[3] = {1.5f, 3.0f, 5.0f};

		srand(2023);
		asteroid_add(GameState);
		familiar_add(GameState);

		GameInput->Mouse.x = BackBuffer->width / 2;
		GameInput->Mouse.y = BackBuffer->height / 2;

		v2f vertices[3] = {};
		vertices[0] = V2F((f32)BackBuffer->width / 2.0f, (f32)BackBuffer->height / 2.0f);
		vertices[1] = vertices[0] + V2F(-20.0f, 20.0f);
		vertices[2] = vertices[1] + V2F(-20.0f, -20.0f);
		GameState->Triangle = triangle_init(vertices, 3);

		GameMemory->is_initialized = true;

	}

	tile_map *TileMap = GameState->TileMap;
	v2f BottomLeft = {5.0f, 5.0f};

	entity *EntityPlayer = entity_get(GameState, GameState->player_entity_index);
	v2f temp = tile_map_get_absolute_pos(TileMap, EntityPlayer->TileMapPos);

#if DEBUG_TILE_MAP
	v2f MinScreenXY = {};
	v2f MaxScreenXY = V2F((f32)BackBuffer->width, (f32)BackBuffer->height);
	rectangle_draw(BackBuffer, MinScreenXY, MaxScreenXY, 0.0f, 0.0f, 0.0f);

	for (s32 row = 0; row < TileMap->tile_count_y; row++) {
		for (s32 col = 0; col < TileMap->tile_count_x ; col++) {
			u32 tile_value = TileMap->tiles[row * TileMap->tile_count_x + col];
			f32 gray = 0.5f;
			if (tile_value == 1) {
				gray = 1.0f;
			} 
			v2f Min, Max;

			Min.x = BottomLeft.x + (f32)col * TileMap->tile_side_in_pixels;
			Min.y = BottomLeft.y + (f32)row * TileMap->tile_side_in_pixels;
			Max.x = Min.x + TileMap->tile_side_in_pixels;
			Max.y = Min.y + TileMap->tile_side_in_pixels;
			rectangle_draw(BackBuffer, Min, Max, gray, gray, gray);
		}
	}

	MinScreenXY.x = BottomLeft.x + (f32)EntityPlayer->TileMapPos.Tile.x * TileMap->tile_side_in_pixels;
	MinScreenXY.y = BottomLeft.y + (f32)EntityPlayer->TileMapPos.Tile.y * TileMap->tile_side_in_pixels;
	MaxScreenXY.x = MinScreenXY.x + TileMap->tile_side_in_pixels;
	MaxScreenXY.y = MinScreenXY.y + TileMap->tile_side_in_pixels;
	rectangle_draw(BackBuffer, MinScreenXY, MaxScreenXY, 1.0f, 1.0f, 0.0f);

#else
	bitmap_draw(BackBuffer, &GameState->Background, 0.0f, 0.0f);
#endif
	f32 dt_for_frame = GameInput->dt_for_frame;


	v2f PlayerNewRight = EntityPlayer->Right;
	v2f PlayerNewDirection = EntityPlayer->Direction;
	if (GameInput->Controller.Left.ended_down) {

		// NOTE(Justin): The player has a coordinate frame of two basis vectors
		// which are Direction and Right. These basis vectors are at the world
		// origin (0,0). They are at the origin because when the player rotates
		// the orientation of the player needs to be updated and if the
		// vectors are at the origin there does not exist a position bias in
		// them and we can just rotate them. Otherwise we would need to
		// translate the frame to the origin, rotate it, and then translate it
		// back. Instead when we go to draw the player
		// we get a copy of both vectors and offset them to the position of
		// the player and then draw the player using these two vectors and the
		// players height and base half width.
		//
		// OTOH if we define local coordinates of an object, the objects
		// poistion is the origin (0,0) and we can still do the rotation. What
		// is better?

		m3x3 Rotation = m3x3_rotate_about_origin(5.0f * dt_for_frame);
		PlayerNewRight = m3x3_transform_v2f(Rotation, PlayerNewRight);
		PlayerNewDirection = m3x3_transform_v2f(Rotation, PlayerNewDirection);
	}
	if (GameInput->Controller.Right.ended_down) {
		m3x3 Rotation = m3x3_rotate_about_origin(-5.0f * dt_for_frame);
		PlayerNewRight = m3x3_transform_v2f(Rotation, PlayerNewRight);
		PlayerNewDirection = m3x3_transform_v2f(Rotation, PlayerNewDirection);
	}
	EntityPlayer->Right = PlayerNewRight;
	EntityPlayer->Direction = PlayerNewDirection;


	//
	// NOTE(Justin): Player projectiles
	//

	// TODO(Justin) Entity projectiles?

	// NOTE(Justin): The projectiles are implemented with a "circular buffer".
	// When the player starts shooting we intialize a new projectile and
	// increment an index into this buffer, the index is incremented to be able
	// to pick the next projectile. When the index id at least the size of
	// the projectile buffer, we set the index back to 0. One potential problem
	// with this is we can overwrite a projectile that is still active. To avoid
	// this situation the buffer needs to be big enough sudh that by the time
	// the last projectile element in the buffer is initilized, the first
	// projectile element in the buffer needs to be not active. Two paramters to
	// make this happen are the size of the projectile buffer and the time
	// between consecutive projecitles. 

	
	if (GameInput->Controller.Space.ended_down) {
		if (!EntityPlayer->is_shooting) {
			projectile *Projectile = GameState->Projectiles + GameState->projectile_next++;
			if (GameState->projectile_next >= ARRAY_COUNT(GameState->Projectiles)) {
				GameState->projectile_next = 0;
			}
			Projectile->TileMapPos = EntityPlayer->TileMapPos;
			Projectile->distance_remaining = 100.0f;
			Projectile->Direction = EntityPlayer->Direction;
			Projectile->dPos = GameState->projectile_speed * Projectile->Direction;
			Projectile->is_active = true;
			Projectile->time_between_next_trail = 0.1f;
			EntityPlayer->is_shooting = true;
		} else if (EntityPlayer->is_shooting) {

			// NOTE(Justin): If the player is already shooting, wait some time
			// before shooting a new projectile. Otherwise MACHINE GUN.

			GameState->time_between_new_projectiles += dt_for_frame;
			if (GameState->time_between_new_projectiles > 0.6f) {
				projectile *Projectile = GameState->Projectiles + GameState->projectile_next++;
				if (GameState->projectile_next >= ARRAY_COUNT(GameState->Projectiles)) {
					GameState->projectile_next = 0;
				}
				Projectile->TileMapPos = EntityPlayer->TileMapPos;
				Projectile->distance_remaining = 100.0f;
				Projectile->Direction = EntityPlayer->Direction;
				Projectile->dPos = GameState->projectile_speed * Projectile->Direction;
				Projectile->is_active = true;
				Projectile->time_between_next_trail = 0.1f;
				EntityPlayer->is_shooting = true;
				GameState->time_between_new_projectiles = 0.0f;
			}
		}
	} else {
		EntityPlayer->is_shooting = false;
	}

	for (u32 projectile_index = 0; projectile_index < ARRAY_COUNT(GameState->Projectiles); projectile_index++) {
		projectile *Projectile = GameState->Projectiles + projectile_index;
		if (Projectile->is_active) {
			v2f DeltaPos = dt_for_frame * Projectile->dPos;
			Projectile->TileMapPos.TileOffset += DeltaPos;
			Projectile->TileMapPos = tile_map_position_remap(TileMap, Projectile->TileMapPos);

			Projectile->distance_remaining -= v2f_length(DeltaPos);
			if (Projectile->distance_remaining > 0.0f) {
				v2f OffsetInPixels = V2F(5.0f, 5.0f);

				v2f Min = tile_map_get_screen_coordinates(TileMap, &Projectile->TileMapPos, BottomLeft);
				Min += (f32)(GameState->Ship.width / 2.0f) * Projectile->Direction;
				v2f Max = Min + OffsetInPixels;

				rectangle_draw(BackBuffer, Min, Max, 0.0f, 0.0f, 1.0f);
			} else {
				Projectile->is_active = false;
			}

			Projectile->time_between_next_trail -= dt_for_frame;
			if (Projectile->time_between_next_trail <= 0.0f) {
				projectile_trail *Trail = Projectile->Trails + Projectile->trail_next++;
				if (Projectile->trail_next >= ARRAY_COUNT(Projectile->Trails)) {
					Projectile->trail_next = 0;
				}
				Trail->TileMapPos = Projectile->TileMapPos;

				// NOTE(Justin): A projectile trail is just a sequence of
				// projectiles that start fully opaque and increase in
				// transparency until the last projectile is fully transparent.
				// Visually this makes the projectile look similar to a beam of energy
				// from your run of the mill sci-fi movie.
				//
				// We are using the time_left parameter as the
				// alpha blend value which is nice. When the time left is 1 at
				// the outset the trail will be fully opaque. As we step into
				// the next frames the time left value is decreased by
				// dt_for_frame. So the trail becomes more and more transparent
				// until the time left is <= 0 which represents fully transparent.

				Trail->time_left = 1.0f;
				Projectile->time_between_next_trail = 0.1f;
			}
			for (u32 trail_index = 0; trail_index < ARRAY_COUNT(Projectile->Trails); trail_index++) {
				projectile_trail *Trail = Projectile->Trails + trail_index;
				Trail->time_left -= dt_for_frame;
				if (Trail->time_left > 0.0f) {
					v2f OffsetInPixels = V2F(5.0f, 5.0f);

					v2f Min = tile_map_get_screen_coordinates(TileMap, &Trail->TileMapPos, BottomLeft);
					Min += (f32)(GameState->Ship.width / 2.0f) * Projectile->Direction;
					v2f Max = Min + OffsetInPixels;

					rectangle_transparent_draw(BackBuffer, Min, Max, 0.0f, 0.0f, 1.0f, Trail->time_left);
				}
			}
		}
	}

	//
	// NOTE(Justin): Move entities.
	//

	v2f PlayerAccel = {};
	if (GameInput->Controller.Up.ended_down) {
		PlayerAccel = {PlayerNewDirection.x, PlayerNewDirection.y};
	}
	entity_move(GameState, EntityPlayer, PlayerAccel, dt_for_frame);

	
	v2f AsteroidAccel = {};
	for (u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++) {
		entity *Entity = GameState->Entities + entity_index;
		if (Entity->type == ENTITY_ASTEROID) {
			entity_move(GameState, Entity, AsteroidAccel, dt_for_frame);
		}
	}

	// 
	// NOTE(Justin): Render
	//

	entity_visible_piece_group PieceGroup;
	for (u32 entity_index = 1; entity_index < GameState->entity_count; entity_index++) {

		PieceGroup.piece_count = 0;
		entity *Entity = GameState->Entities + entity_index;
		switch(Entity->type) {
			case ENTITY_PLAYER:
			{
				v2f PlayerScreenPos = tile_map_get_screen_coordinates(TileMap, &Entity->TileMapPos,
						BottomLeft);

				debug_vector_draw_at_point(BackBuffer, PlayerScreenPos, Entity->Direction);

				v2f Alignment = {(f32)GameState->Ship.width / 2.0f, (f32)GameState->Ship.height / 2.0f};
				push_piece(&PieceGroup, &GameState->Ship, PlayerScreenPos, Alignment);

#if DEBUG_DRAW_PLAYER_POS
				v2f OffsetInPixels = {10.0f, 10.0f};
				Min = PlayerScreenPos;
				Min += -0.5f * OffsetInPixels;
				Max = Min + OffsetInPixels;
				rectangle_draw(BackBuffer, Min, Max, 0.0f, 0.0f, 0.0f);
#endif
			} break;
			case ENTITY_ASTEROID:
			{
				v2f AsteroidScreenPos = tile_map_get_screen_coordinates(TileMap, &Entity->TileMapPos,
						BottomLeft);
				v2f Alignment = {(f32)GameState->AsteroidSprite.width / 2.0f,
					(f32)GameState->AsteroidSprite.height / 2.0f};

				push_piece(&PieceGroup, &GameState->AsteroidSprite, AsteroidScreenPos, Alignment);

			} break;
			case ENTITY_FAMILIAR:
			{
				familiar_update(GameState, Entity, dt_for_frame);

				v2f AsteroidScreenPos = tile_map_get_screen_coordinates(TileMap, &Entity->TileMapPos,
																									BottomLeft);
				debug_vector_draw_at_point(BackBuffer, AsteroidScreenPos, Entity->Direction);
				v2f Alignment = {(f32)GameState->AsteroidSprite.width / 2.0f,
								 (f32)GameState->AsteroidSprite.height / 2.0f};

				push_piece(&PieceGroup, &GameState->AsteroidSprite, AsteroidScreenPos, Alignment);

			} break;
			case ENTITY_PROJECTILE:
			{

			} break;
			default:
			{
				INVALID_CODE_PATH;
			} break;
		}

		for (u32 piece_index = 0; piece_index < PieceGroup.piece_count; piece_index++) {
			entity_visible_piece *Piece = &PieceGroup.Pieces[piece_index];

			f32 x = Piece->Offset.x;
			f32 y = Piece->Offset.y;

			if (Piece->Bitmap) {
				bitmap_draw(BackBuffer, Piece->Bitmap, x, y, Piece->alpha);
			} else {

			}
		}
	}

	v2f Delta = {};
	if (GameInput->Controller.ArrowLeft.ended_down) {
		Delta += V2F(-1.0f, 0.0f);
	}
	if (GameInput->Controller.ArrowRight.ended_down) {
		Delta += V2F(1.0f, 0.0f);
	}
	if (GameInput->Controller.ArrowUp.ended_down) {
		Delta += V2F(0.0f, 1.0f);
	}
	if (GameInput->Controller.ArrowDown.ended_down) {
		Delta += V2F(0.0f, -1.0f);
	}

	// NOTE(Justin): The arguement to the function is a v2f but the matrix
	// returned has a final column of (0,0,1)^T 
	m3x3 M = m3x3_translation_create(Delta);
	triangle *T = &GameState->Triangle;


	circle Circle = circle_init(V2F((f32)BackBuffer->width / 2.0f + 50.0f, (f32)BackBuffer->height / 2.0f + 50.0f), 25.0f);


	triangle Test = *T;
	for (u32 i = 0; i < 3; i++) {
		Test.Vertices[i] = M * Test.Vertices[i];
	}
	if (triangle_circle_collision(BackBuffer, &Test, &Circle)) {
		triangle_draw(BackBuffer, T, 1.0f, 0.0f, 0.0f);
	} else
	{
		*T = Test;
		triangle_draw(BackBuffer, T, 1.0f, 1.0f, 1.0f);
	}

	circle_draw(BackBuffer, &Circle, 1.0f, 1.0f, 1.0f);
	line_dda_draw(BackBuffer, Circle.Center, Circle.Center + V2F(Circle.radius, 0.0f), 1.0f, 0.0f, 0.0f);


	//sound_buffer_fill(GameState, SoundBuffer);

}
