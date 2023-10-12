/*
 * TODO:
 *	- Threading
 * 	- Asset loading
 * 	- Animations
 *		- Explosions
 *		- Lasers
 *		- Warp
 *		- Shield
 *			- Appears on collision
 *			- Fades shortly thereafter
 * 	- Game of life?
 *		- Destorying an asteoid spawns alien
 *		- Alien behavior adheres to the rules of the game of life
 *		- Include a weighting so that the alien movement is biased towards the player.
 * 	- SFX
 * 	- Score
 * 	- Menu
 * 	- Optimization pass
 * 	- Profiling
 * 	- SIMD
 * 	- Intrinsics
 * 	- VfX
 * 	- Audio mixing
 * 	- Minkoski based collision detection
 * 	- Bitmap transformations (rotations, scaling, ...)
 * 	- Normal mapping
 * 	- Asteroid collision 
 * 	- Angular momentum
*/

#include "game.h"
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
	if (x_max >= BackBuffer->width) {
		x_max -= BackBuffer->width;
	}
	if (y_min < 0) {
		y_min += BackBuffer->height;
	}
	if (y_max >= BackBuffer->height) {
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

#if 1
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
#endif

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


#if 0
internal void
circle_draw(game_state *GameState, back_buffer *BackBuffer, circle *Circle,
		f32 r, f32 b, f32 g)
{
	bounding_box CircleBoudingBox = circle_bounding_box_find(*Circle);

	CircleBoudingBox.Min = point_clip_to_world(&GameState->WorldHalfDim, CircleBoudingBox.Min);
	CircleBoudingBox.Max = point_clip_to_world(&GameState->WorldHalfDim, CircleBoudingBox.Max);

	CircleBoudingBox.Min = point_map_to_screen(GameState, BackBuffer, CircleBoudingBox.Min);
	CircleBoudingBox.Max = point_map_to_screen(GameState, BackBuffer, CircleBoudingBox.Max);

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

	v2f Center = point_map_to_screen(GameState, BackBuffer, Circle->Center);
	f32 radius = GameState->pixels_per_meter * Circle->radius;
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
#endif

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

internal u32
str_length(char *str)
{
	u32 Result = 0;
	for (char *c = str; *c != '\0'; c++) {
		Result++;
	}
	return(Result);
}

internal b32
str_are_same(char *str1, char *str2)
{
	// NOTE(Justin): Assume the strings are the same from the
	// outset, if we find that the lengths are in fact different, then they
	// are not the same and set Result to 0. If the lengths are the same
	// but the characters are not all the same, then the strings are
	// different and again set Result to 0.

	b32 Result = 1;

	u32 str1_length = str_length(str1);
	u32 str2_length = str_length(str2);
	if (str1_length == str2_length) {
		char *c1 = str1;
		char *c2 = str2;
		for (u32 char_index = 0; char_index < str1_length; char_index++) {
			if (*c1++ != *c2++) {
				Result = 0;
				break;
			}
		}
	} else {
		Result = 0;
	}
	return(Result);
}

internal string_u8
str_u8(char *str)
{
	string_u8 Result;
	Result.data = (u8 *)str;
	for (char *c = str; *c != '\0'; c++) {
		Result.length++;
	}
	return(Result);
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
bitmap_draw(back_buffer *BackBuffer, loaded_bitmap *Bitmap, f32 x, f32 y)
{
	s32 x_min = f32_round_to_s32(x);
	s32 y_min = f32_round_to_s32(y);
	s32 x_max = f32_round_to_s32(x + (f32)Bitmap->width);
	s32 y_max = f32_round_to_s32(y + (f32)Bitmap->height);

	if (x_min < 0) {
		x_min += BackBuffer->width;
	}
	if (y_min < 0) {
		y_min += BackBuffer->height;
	}
	if (x_max > BackBuffer->width) {
		x_max -= BackBuffer->width;
	}
	if (y_max > BackBuffer->height) {
		y_max -= BackBuffer->height;
	}

	u8 *dest_row = (u8 *)BackBuffer->memory + y_min * BackBuffer->stride + x_min * BackBuffer->bytes_per_pixel;
	u8 *src_row = (u8 *)Bitmap->memory;
	for (s32 y = y_min; y < y_max; y++) {
		u32 *src = (u32 *)src_row;
		u32 *dest = (u32 *)dest_row;
		for (s32 x = x_min; x < x_max; x++) {

			f32 alpha = (f32)((*src >> 24) & 0xFF) / 255.0f;
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

internal void
debug_vector_draw_at_point(back_buffer * BackBuffer, v2f Point, v2f Direction)
{
	f32 c = 60.0f;
	line_dda_draw(BackBuffer, Point, Point + c * Direction, 1.0f, 1.0f, 1.0f);
}

internal v2f
tile_map_get_screen_coordinates(tile_map *TileMap, tile_map_position *TileMapPos, v2f BottomLeft)
{
	v2f Result = {};

	Result.x = BottomLeft.x + TileMap->tile_side_in_pixels * TileMapPos->Tile.x + 
		(TileMap->tile_side_in_pixels / 2) + TileMap->meters_to_pixels * TileMapPos->TileOffset.x;
	Result.y = BottomLeft.y + TileMap->tile_side_in_pixels * TileMapPos->Tile.y +
		(TileMap->tile_side_in_pixels / 2) + TileMap->meters_to_pixels * TileMapPos->TileOffset.y;

	return(Result);
}

internal b32
test_wall(f32 max_corner_x, f32 rel_x, f32 rel_y, f32 *t_min,
		  f32 player_delta_x, f32 player_delta_y, f32 min_corner_y, f32 max_corner_y)
{
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

internal void
player_move(tile_map *TileMap, player *Player, v2f ddPos, f32 dt_for_frame)
{
	f32 ddp_length_squared = v2f_length_squared(ddPos);
	if (ddp_length_squared > 1.0f) {
		ddPos *= HALF_ROOT_TWO;
	}
	ddPos *= Player->speed;

	v2f dPos;
	dPos = dt_for_frame * ddPos + Player->dPos;
	Player->dPos = dPos;

	v2f PlayerDelta = 0.5f * ddPos * SQUARE(dt_for_frame) + dt_for_frame * Player->dPos;

	tile_map_position PlayerOldPos = Player->TileMapPos;

	tile_map_position PlayerNewPos = Player->TileMapPos;
	PlayerNewPos.TileOffset += PlayerDelta;
	PlayerNewPos = tile_map_position_remap(TileMap, PlayerNewPos);

	s32 tile_min_x = MIN(PlayerOldPos.Tile.x, PlayerNewPos.Tile.x);
	s32 tile_min_y = MIN(PlayerOldPos.Tile.y, PlayerNewPos.Tile.y);
	s32 tile_max_x_one_past = MAX(PlayerOldPos.Tile.x, PlayerNewPos.Tile.x) + 1;
	s32 tile_max_y_one_past = MAX(PlayerOldPos.Tile.y, PlayerNewPos.Tile.y) + 1;

	// TODO(Justin): Collision detection against asteroids not tiles.
	// TODO(Justin): Player shield allows for collisions with 
	f32 t_min = 1.0f;
	v2f WallNormal = {};
	b32 collided = false;
	for (s32 tile_y = tile_min_y; tile_y != tile_max_y_one_past; tile_y++) {
		for (s32 tile_x = tile_min_x; tile_x != tile_max_x_one_past; tile_x++) {
			tile_map_position TestTilePos = tile_map_get_centered_position(tile_x, tile_y);
			u32 tile_value = tile_map_get_tile_value_unchecked(TileMap, TestTilePos.Tile);
			if (!tile_map_is_tile_value_empty(tile_value)) {
				v2f MinCorner = -0.5f * v2f_create(TileMap->tile_side_in_meters, TileMap->tile_side_in_meters);
				v2f MaxCorner = 0.5f * v2f_create(TileMap->tile_side_in_meters, TileMap->tile_side_in_meters);

				tile_map_pos_delta TileRelDelta = tile_map_get_pos_delta(TileMap, &PlayerOldPos, &TestTilePos);
				v2f Rel = TileRelDelta.dXY;

				// Right wall
				if (test_wall(MaxCorner.x, Rel.x, Rel.y, &t_min, PlayerDelta.x, PlayerDelta.y,
							MinCorner.y, MaxCorner.y)) {
					WallNormal = {1.0f, 0.0f};
					collided = true;
				}

				// Left wall
				if (test_wall(MinCorner.x, Rel.x, Rel.y, &t_min, PlayerDelta.x, PlayerDelta.y,
							MinCorner.y, MaxCorner.y)) {

					WallNormal= {-1.0f, 0.0f};
					collided = true;
				}

				// Bottom wall
				if (test_wall(MinCorner.y, Rel.y, Rel.x, &t_min, PlayerDelta.y, PlayerDelta.x,
							MinCorner.x, MaxCorner.x)) {

					WallNormal = {0.0f, 1.0f};
					collided = true;
				}

				// Top wall
				if (test_wall(MaxCorner.y, Rel.y, Rel.x, &t_min, PlayerDelta.y, PlayerDelta.x,
							MinCorner.x, MaxCorner.x)) {

					WallNormal = {0.0f, -1.0f};
					collided = true;
				}
			}
		}
	}

	PlayerNewPos = PlayerOldPos;
	PlayerNewPos.TileOffset += t_min * PlayerDelta;
	PlayerNewPos = tile_map_position_remap(TileMap, PlayerNewPos);

	Player->TileMapPos = PlayerNewPos;
	//Player->dPos += dt_for_frame * ddPos;

	if (collided) {
		if (!Player->is_shielded) {
			Player->dPos = {};
			Player->TileMapPos = {};
			Player->TileMapPos.Tile = {TileMap->tile_count_x / 2, TileMap->tile_count_y / 2};
		} else {
			Player->dPos = Player->dPos - 2.0f * v2f_dot(Player->dPos, WallNormal) * WallNormal;
		}
	}
}

internal void
update_and_render(game_memory *GameMemory, back_buffer *BackBuffer, sound_buffer *SoundBuffer, game_input *GameInput)
{
	ASSERT(sizeof(game_state) <= GameMemory->total_size);
	game_state *GameState = (game_state *)GameMemory->permanent_storage;
	if (!GameMemory->is_initialized) {

		//GameState->TestSound = wav_file_read_entire("bloop_00.wav");
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

		GameState->WorldHalfDim = v2f_create((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter),
											 (f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter));

		player *Player = &GameState->Player;

		Player->TileMapPos.Tile = v2i_create(TileMap->tile_count_x / 2, TileMap->tile_count_y / 2);
		Player->TileMapPos.TileOffset = v2f_create(0.0f, 0.0f);

		Player->height = 10.0f;
		Player->base_half_width = 4.0f;

		Player->Right = v2f_create(1.0f, 0.0f);
		Player->Direction = v2f_create(0.0f, 1.0f);

		Player->dPos = v2f_create(0.0f, 0.0f);
		Player->speed = 30.0f;
		Player->is_shooting = false;
		Player->is_warping = false;
		Player->is_shielded = true;

		GameState->projectile_next = 0;
		GameState->projectile_speed = 60.0f;
		GameState->projectile_half_width = 1.0f;

		f32 asteroid_scales[3] = {1.5f, 3.0f, 5.0f};
		srand(2023);
		for (u32 asteroid_index = 0; asteroid_index < ARRAY_COUNT(GameState->Asteroids); asteroid_index++) {

			// Tile offset
			f32 x_offset = (12.0f * ((f32)rand() / (f32)RAND_MAX) - 6);
			f32 y_offset = (12.0f * ((f32)rand() / (f32)RAND_MAX) - 6);

			// Tile
			s32 tile_x =  (s32)((TileMap->tile_count_x - 1) * ((f32)rand() / (f32)(RAND_MAX)));
			s32 tile_y =  (s32)((TileMap->tile_count_y - 1) * ((f32)rand() / (f32)(RAND_MAX)));

			GameState->Asteroids[asteroid_index].TileMapPos.TileOffset = {x_offset, y_offset};
			GameState->Asteroids[asteroid_index].TileMapPos.Tile = {tile_x, tile_y};


			// Direction
			f32 x_dir = ((f32)rand() / (f32)RAND_MAX) - 0.5f;
			f32 y_dir = ((f32)rand() / (f32)RAND_MAX) - 0.5f;

		

			f32 speed_scale = ((f32)rand() / (f32)RAND_MAX);

			GameState->Asteroids[asteroid_index].Direction = v2f_create(x_dir, y_dir);
			GameState->Asteroids[asteroid_index].speed = 9.0f;

			f32 asteroid_speed = speed_scale * GameState->Asteroids[asteroid_index].speed;
			GameState->Asteroids[asteroid_index].dPos = asteroid_speed * GameState->Asteroids[asteroid_index].Direction;

			// Size scale
			u32 scale_index = rand() % 3;
			f32 size_scale = asteroid_scales[scale_index];
			if (size_scale == asteroid_scales[ASTEROID_SMALL]) {
				GameState->Asteroids[asteroid_index].size = ASTEROID_SMALL;
			} else if (size_scale == asteroid_scales[ASTEROID_MEDIUM]) {
				GameState->Asteroids[asteroid_index].size = ASTEROID_MEDIUM;
			} else {
				GameState->Asteroids[asteroid_index].size = ASTEROID_LARGE;
			}
			GameState->Asteroids[asteroid_index].mass = size_scale;
		
			// Orientation
			f32 angle = 2.0f * PI32 * ((f32)rand() / (f32)RAND_MAX);
			m3x3 Rotation = m3x3_rotate_about_origin(angle);

			// Bounding Box
			v2f BoundingBoxMin = GameState->Asteroids[asteroid_index].LocalVertices[0];
			v2f BoundingBoxMax = GameState->Asteroids[asteroid_index].LocalVertices[1];
			for (u32 vertex_index = 0; vertex_index < ARRAY_COUNT(GameState->Asteroids[asteroid_index].LocalVertices); vertex_index++) {
				v2f Vertex = GameState->Asteroids[asteroid_index].LocalVertices[vertex_index];
				if (Vertex.x < BoundingBoxMin.x) {
					BoundingBoxMin.x = Vertex.x;
				}
				if (Vertex.y < BoundingBoxMin.y) {
					BoundingBoxMin.y = Vertex.y;
				}
				if (Vertex.x > BoundingBoxMax.x) {
					BoundingBoxMax.x = Vertex.x;
				}
				if (Vertex.y > BoundingBoxMax.y) {
					BoundingBoxMax.y = Vertex.y;
				}
			}
			BoundingBoxMin = 1.10f * BoundingBoxMin;
			BoundingBoxMax = 0.9f * BoundingBoxMax;
			GameState->Asteroids[asteroid_index].BoundingBox.Min += GameState->Asteroids[asteroid_index].Pos; 
			GameState->Asteroids[asteroid_index].BoundingBox.Max += GameState->Asteroids[asteroid_index].Pos; 

			GameState->Asteroids[asteroid_index].is_active = true;

		}

		for (u32 asteroid_index = 0; asteroid_index < ARRAY_COUNT(GameState->Asteroids); asteroid_index++) {
			asteroid *Asteroid = GameState->Asteroids + asteroid_index;
			tile_map_tile_set_value(TileMap, Asteroid->TileMapPos.Tile, 1);
		}

		GameMemory->is_initialized = TRUE;
	}



#if 1
	v2f Min = {};
	v2f Max = v2f_create((f32)BackBuffer->width, (f32)BackBuffer->height);
	rectangle_draw(BackBuffer, Min, Max, 0.0f, 0.0f, 0.0f);
#endif

	// NOTE(Justin): Since the backbuffer is bottom up, the first row of the
	// tilemap is the row that gets drawn first and gets drawn at the very
	// bottom of the screen. Which means that the first row maps to the bottom
	// of the screen and the last row maps to the top of the screen.
	// The reason why the backbuffer is bottom up is so that as y values
	// increase, objects move further up the screen.

	tile_map *TileMap = GameState->TileMap;
	v2f BottomLeft = v2f_create(5.0f, 5.0f);
#if DEBUG_TILE_MAP
	for (s32 row = 0; row < TileMap->tile_count_y; row++) {
		for (s32 col = 0; col < TileMap->tile_count_x ; col++) {
			u32 tile_value = TileMap->tiles[row * TileMap->tile_count_x + col];
			f32 gray = 0.5f;
			if (tile_value == 1) {
				gray = 1.0f;
			} 
			if ((row == GameState->Player.TileMapPos.Tile.y) && 
				(col == GameState->Player.TileMapPos.Tile.x)) {
				gray = 0.0f;
			}
			v2f Min, Max;

			Min.x = BottomLeft.x + (f32)col * TileMap->tile_side_in_pixels;
			Min.y = BottomLeft.y + (f32)row * TileMap->tile_side_in_pixels;
			Max.x = Min.x + TileMap->tile_side_in_pixels;
			Max.y = Min.y + TileMap->tile_side_in_pixels;
			rectangle_draw(BackBuffer, Min, Max, gray, gray, gray);
		}
	}

	Min.x = BottomLeft.x + GameState->Player.TileMapPos.Tile.x * TileMap->tile_side_in_pixels;
	Min.y = BottomLeft.y + (f32)GameState->Player.TileMapPos.Tile.y * TileMap->tile_side_in_pixels;
	Max.x = Min.x + TileMap->tile_side_in_pixels;
	Max.y = Min.y + TileMap->tile_side_in_pixels;
	rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 0.0f);

#else
	bitmap_draw(BackBuffer, &GameState->Background, 0.0f, 0.0f);
#endif



	v2f *WorldHalfDim = &GameState->WorldHalfDim;
	if (WorldHalfDim->x != ((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter))) {
		WorldHalfDim->x = ((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter));
	}
	if (WorldHalfDim->y != ((f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter))) {
		WorldHalfDim->y = ((f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter));
	}

	f32 dt_for_frame = GameInput->dt_for_frame;
	player *Player = &GameState->Player;

	v2f PlayerNewRight = Player->Right;
	v2f PlayerNewDirection = Player->Direction;
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
	Player->Right = PlayerNewRight;
	Player->Direction = PlayerNewDirection;


	//
	// NOTE(Justin): Player projectiles
	//


	// NOTE(Justin): The projectiles are implemented with a circular buffer.
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
		if (!Player->is_shooting) {
			projectile *Projectile = GameState->Projectiles + GameState->projectile_next++;
			if (GameState->projectile_next >= ARRAY_COUNT(GameState->Projectiles)) {
				GameState->projectile_next = 0;
			}
			Projectile->TileMapPos = Player->TileMapPos;
			Projectile->TileMapPos.TileOffset += 10.0f * Player->Direction;
			Projectile->distance_remaining = 100.0f;
			Projectile->Direction = Player->Direction;
			Projectile->dPos = GameState->projectile_speed * Projectile->Direction;
			Projectile->is_active = TRUE;
			Projectile->time_between_next_trail = 0.1f;
			Player->is_shooting = TRUE;
		} else if (Player->is_shooting) {
			// NOTE(Justin): If the player is already shooting, wait some time
			// before shooting a new projectile. Otherwise MACHINE GUN.
			GameState->time_between_new_projectiles += dt_for_frame;
			if (GameState->time_between_new_projectiles > 0.6f) {
				projectile *Projectile = GameState->Projectiles + GameState->projectile_next++;
				if (GameState->projectile_next >= ARRAY_COUNT(GameState->Projectiles)) {
					GameState->projectile_next = 0;
				}
				Projectile->TileMapPos = Player->TileMapPos;
				Projectile->distance_remaining = 100.0f;
				Projectile->Direction = Player->Direction;
				Projectile->dPos = GameState->projectile_speed * Projectile->Direction;
				Projectile->is_active = TRUE;
				Projectile->time_between_next_trail = 0.1f;
				Player->is_shooting = TRUE;
				GameState->time_between_new_projectiles = 0.0f;
			}
		}
	} else {
		Player->is_shooting = FALSE;
	}

	for (u32 projectile_index = 0; projectile_index < ARRAY_COUNT(GameState->Projectiles); projectile_index++) {
		projectile *Projectile = GameState->Projectiles + projectile_index;
		if (Projectile->is_active) {
			v2f DeltaPos = dt_for_frame * Projectile->dPos;
			Projectile->TileMapPos.TileOffset += DeltaPos;
			Projectile->TileMapPos = tile_map_position_remap(TileMap, Projectile->TileMapPos);

			Projectile->distance_remaining -= v2f_length(DeltaPos);
			if (Projectile->distance_remaining > 0.0f) {
				v2f OffsetInPixels = v2f_create(5.0f, 5.0f);
				v2f Min;
				Min.x = BottomLeft.x + Projectile->TileMapPos.Tile.x * TileMap->tile_side_in_pixels + TileMap->meters_to_pixels * Projectile->TileMapPos.TileOffset.x;
				Min.y = BottomLeft.y + Projectile->TileMapPos.Tile.y * TileMap->tile_side_in_pixels + TileMap->meters_to_pixels * Projectile->TileMapPos.TileOffset.y;

				v2f Max = Min + OffsetInPixels;
				rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 1.0f);
			} else {
				Projectile->is_active = FALSE;
			}

			// TODO(Justin): Do this only if the projectile has distance
			// remaining?
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
					v2f OffsetInPixels = v2f_create(5.0f, 5.0f);
					v2f Min;
					Min.x = BottomLeft.x + Trail->TileMapPos.Tile.x * TileMap->tile_side_in_pixels + TileMap->meters_to_pixels * Trail->TileMapPos.TileOffset.x;
					Min.y = BottomLeft.y + Trail->TileMapPos.Tile.y * TileMap->tile_side_in_pixels + TileMap->meters_to_pixels * Trail->TileMapPos.TileOffset.y;

					v2f Max = Min + OffsetInPixels;
					rectangle_transparent_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 1.0f, Trail->time_left);
				}
			}
		}
	}

	//
	// NOTE(Justin): Move player.
	//


	v2f ddPos = {};
	if (GameInput->Controller.Up.ended_down) {
		ddPos = v2f_create(PlayerNewDirection.x, PlayerNewDirection.y);
	}
	player_move(TileMap, Player, ddPos, dt_for_frame);


	v2f PlayerScreenPos = tile_map_get_screen_coordinates(TileMap, &Player->TileMapPos, BottomLeft);
#if 1
	debug_vector_draw_at_point(BackBuffer, PlayerScreenPos, Player->Direction);
#endif

	// NOTE(Justin): To center the player bitmap within a tile we need to first
	// get the players screen position (above) and then offset this position by
	// half the width and height of the player sprite. The result is that the
	// player sprite is centered with the tile.

	v2f Alignment = {(f32)GameState->Ship.width / 2.0f, (f32)GameState->Ship.height / 2.0f};
	PlayerScreenPos += -1.0f * Alignment;

	bitmap_draw(BackBuffer, &GameState->Ship, PlayerScreenPos.x, PlayerScreenPos.y);

	// NOTE(Justin): Assume that the bounding box min and max are the left and
	// right vertices of the player, respectively. Then check if it is true and
	// update the bounding box min/max if not true.

	
	// TODO(Justin): Experiment with two bounding boxes for the player. A large
	// and small bounding box. Or continue with ne bounding box and if the
	// players, or an asteroids, bounding box intersects with another bounding
	// bo then do more robust collision detection.

	// NOTE(Justin): Asteroids have constant velocity and therefore no
	// acceleration. To move the asteroid for each frame, scale the velocity
	// vector of the asteroid by dt_for_frame and add the vector to the asteroids
	// position. Note that this vector is dx = dt_for_frame * dPos. 

#if 1
	for (u32 asteroid_index = 0; asteroid_index < ARRAY_COUNT(GameState->Asteroids); asteroid_index++) {
		asteroid *Asteroid = GameState->Asteroids + asteroid_index;
		v2f AsteroidDelta = dt_for_frame * Asteroid->dPos;
		tile_map_position AsteroidNewPos = Asteroid->TileMapPos;
		AsteroidNewPos.TileOffset += AsteroidDelta;
		AsteroidNewPos = tile_map_position_remap(TileMap, AsteroidNewPos);

		if (!tile_map_on_same_tile(&Asteroid->TileMapPos, &AsteroidNewPos)) {
			tile_map_tile_set_value(TileMap, Asteroid->TileMapPos.Tile, 0);
			tile_map_tile_set_value(TileMap, AsteroidNewPos.Tile, 1);
		}

		// TODO(Justin): Collision Detection after calculating offset and bounding box.

#if 0
		bounding_box AsteroidNewBoundingBox = Asteroid->BoundingBox;
		AsteroidNewBoundingBox.Min = v2f_add(PosOffset, AsteroidNewBoundingBox.Min);
		AsteroidNewBoundingBox.Max = v2f_add(PosOffset, AsteroidNewBoundingBox.Max);
		AsteroidNewBoundingBox.Min = point_clip_to_world(WorldHalfDim, AsteroidNewBoundingBox.Min);
		AsteroidNewBoundingBox.Max = point_clip_to_world(WorldHalfDim, AsteroidNewBoundingBox.Max);


		if ((PlayerNewBoundingBox.Min.x >= AsteroidNewBoundingBox.Min.x) &&
				(PlayerNewBoundingBox.Min.x <= AsteroidNewBoundingBox.Max.x)) {

			if ((PlayerNewBoundingBox.Min.y >= AsteroidNewBoundingBox.Min.y) &&
					(PlayerNewBoundingBox.Min.y <= AsteroidNewBoundingBox.Max.y)) {
				PlayerNewPos = v2f_create(0.0f, 0.0f);
				PlayerNewRight = v2f_create(1.0f, 0.0f);
				PlayerNewDirection = v2f_create(0.0f, 1.0f);
				dPos = v2f_create(0.0f, 0.0f);
			}
			else if ((PlayerNewBoundingBox.Max.y <= AsteroidNewBoundingBox.Max.y) && 
					(PlayerNewBoundingBox.Max.y >= AsteroidNewBoundingBox.Min.y)) {
				PlayerNewPos = v2f_create(0.0f, 0.0f);
				PlayerNewRight = v2f_create(1.0f, 0.0f);
				PlayerNewDirection = v2f_create(0.0f, 1.0f);
				dPos = v2f_create(0.0f, 0.0f);
			}
		} else if ((PlayerNewBoundingBox.Max.x >= AsteroidNewBoundingBox.Min.x) &&
				(PlayerNewBoundingBox.Max.x <= AsteroidNewBoundingBox.Max.x)) {

			if ((PlayerNewBoundingBox.Min.y >= AsteroidNewBoundingBox.Min.y) &&
					(PlayerNewBoundingBox.Min.y <= AsteroidNewBoundingBox.Max.y)) {
				PlayerNewPos = v2f_create(0.0f, 0.0f);
				PlayerNewRight = v2f_create(1.0f, 0.0f);
				PlayerNewDirection = v2f_create(0.0f, 1.0f);
				dPos = v2f_create(0.0f, 0.0f);
			}
			else if ((PlayerNewBoundingBox.Max.y <= AsteroidNewBoundingBox.Max.y) && 
					(PlayerNewBoundingBox.Max.y >= AsteroidNewBoundingBox.Min.y)) {
				PlayerNewPos = v2f_create(0.0f, 0.0f);
				PlayerNewRight = v2f_create(1.0f, 0.0f);
				PlayerNewDirection = v2f_create(0.0f, 1.0f);
				dPos = v2f_create(0.0f, 0.0f);
			}
		}
		Asteroid->BoundingBox = AsteroidNewBoundingBox;
#endif
		Asteroid->TileMapPos = AsteroidNewPos;

		v2f AsteroidScreenPos = tile_map_get_screen_coordinates(TileMap, &Asteroid->TileMapPos, BottomLeft);

		v2f AsteroidAlignment = {(f32)GameState->AsteroidSprite.width / 2.0f, (f32)GameState->AsteroidSprite.height / 2.0f};
		AsteroidScreenPos += -1.0f * AsteroidAlignment;

		bitmap_draw(BackBuffer, &GameState->AsteroidSprite, AsteroidScreenPos.x, AsteroidScreenPos.y);


	}
#endif




	//player_draw(GameState, BackBuffer, &GameState->Player);

	//sound_buffer_fill(GameState, SoundBuffer);

}
