/*
 * TODO:
 * - Explosion animation
 * - Game of life?
 * - sfx
 * - score
 * - menu
 * - optimization pass
 * - vfx
 * -...
*/

#include "game.h"
#include "game_tile.c"
#include "game_asset.c"

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
pixel_set(back_buffer *BackBuffer, s32 screen_x, s32 screen_y, u32 color)
{
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
pixel_set_v2(back_buffer *BackBuffer, s16 screen_x, s16 screen_y, s16 color)
{
	s32 X = (s32)screen_x;
	s32 Y = (s32)screen_y;

	if (screen_x >= BackBuffer->width) {
		X = screen_x - BackBuffer->width;
	}
	if (screen_x < 0) { 
		X = screen_x + BackBuffer->width;
	}
	if (screen_y >= BackBuffer->height) {
		Y = screen_y - BackBuffer->height;
	}
	if (screen_y < 0) {
		Y = screen_y + BackBuffer->height;
	}
	u8 *start = (u8 *)BackBuffer->memory + BackBuffer->stride * Y + BackBuffer->bytes_per_pixel * X;
	u32 *pixel = (u32 *)start;
	*pixel = color;
}


#if 0
internal void
line_dda_draw(back_buffer *BackBuffer, v2f P1, v2f P2, v3f Color)
{
	u32 step_count;

	v2f Pos = P1;

	v2f Delta = v2f_subtract(P2, P1);

	if (ABS(Delta.x) > ABS(Delta.y)) {
		step_count = (u32)ABS(Delta.x);
	} else {
		step_count = (u32)ABS(Delta.y);
	}

	v2f Step = v2f_scale(1.0f / (f32)step_count, Delta);

	for (u32 k = 0; k < step_count; k++) {
		Pos.x += Step.x;
		Pos.y += Step.y;

		pixel_set(BackBuffer, Pos, Color);
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

internal v2f
point_clip_to_world(v2f *WorldHalfDim, v2f Point)
{
	v2f Result = Point;
	if (Result.x > WorldHalfDim->x) {
		Result.x -= 2.0f * WorldHalfDim->x;
	}
	if (Result.x < -WorldHalfDim->x) {
		Result.x += 2.0f * WorldHalfDim->x;
	}
	if (Result.y > WorldHalfDim->y) {
		Result.y -= 2.0f * WorldHalfDim->y;
	}
	if (Result.y < -WorldHalfDim->y) {
		Result.y += 2.0f * WorldHalfDim->y;
	}
	return(Result);
}

internal v2f
get_screen_pos(back_buffer *BackBuffer, v2f Pos)
{
	v2f Result = {0};

	Result.x = Pos.x + ((f32)BackBuffer->width / 2.0f);
	Result.y = Pos.y + ((f32)BackBuffer->height / 2.0f);
	return(Result);
}


internal v2f
point_map_to_screen(game_state *GameState, back_buffer *BackBuffer, v2f Point)
{
	v2f Result;
	Result = v2f_scale(GameState->pixels_per_meter, Point);
	Result.x += ((f32)BackBuffer->width / 2.0f);
	Result.y += ((f32)BackBuffer->height / 2.0f);
	return(Result);
}

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

internal u32
pixel_count_between_points(v2f P1, v2f P2)
{
	u32 Result = 0;

	v2f Delta = v2f_subtract(P1, P2);

	if(ABS(Delta.x) > ABS(Delta.y)) {
		Result = f32_round_to_u32(ABS(Delta.x));
	} else {
		Result = f32_round_to_u32(ABS(Delta.y));
	}

	return(Result);
}

internal void
line_lerp_draw(back_buffer *BackBuffer, v2f P1, v2f P2, u32 color)
{
	u32 pixel_count = pixel_count_between_points(P1, P2);
	for (u32 i = 0; i < pixel_count; i++) {
		f32 t = (f32)i / (f32)pixel_count;
		v2f P = lerp_points(P1, P2, t);

		pixel_set(BackBuffer, f32_round_to_s32(P.x), f32_round_to_s32(P.y), color);
	}
}

internal void
bezier_curve_draw(back_buffer *BackBuffer, v2f P1, v2f P2, v2f P3, f32 t)
{
	v2f C1 = lerp_points(P1, P2, t);
	v2f C2 = lerp_points(P2, P3, t);
	v2f C3 = lerp_points(C1, C2, t);

	pixel_set(BackBuffer, f32_round_to_s32(C3.x), f32_round_to_s32(C3.y), 0xFFFFFF);
}



#if 0
internal void
player_draw(game_state *GameState, back_buffer *BackBuffer, player *Player)
{
	// NOTE(Justin): x in [-w/10. w/10] and y in [-h/10, h/10].
	// To draw the player to the screen we first need to map (x, y)
	// into [0, w] x [0, h]. This is what point_map_to_screen does. 

	v2f PlayerScreenLeft = tile_relative_pos_map_to_screen(GameState, BackBuffer, Player->TileMapPos, Player->Vertices[PLAYER_LEFT]);
	v2f PlayerScreenRight = tile_relative_pos_map_to_screen(GameState, BackBuffer, Player->TileMapPos, Player->Vertices[PLAYER_RIGHT]);
	v2f PlayerScreenTop = tile_relative_pos_map_to_screen(GameState, BackBuffer, Player->TileMapPos, Player->Vertices[PLAYER_TOP]);
	v2f PlayerScreenBezierTop = tile_relative_pos_map_to_screen(GameState, BackBuffer, Player->TileMapPos, Player->Vertices[PLAYER_BEZIER_TOP]);

	line_wu_draw(BackBuffer, PlayerScreenLeft, PlayerScreenTop, &Player->ColorWu);
	line_wu_draw(BackBuffer, PlayerScreenRight, PlayerScreenTop, &Player->ColorWu);

	// NOTE(Justin): The 100 is arbitrary and fudged to make sure the curve is filled with pixels.
	// TODO(Justin): Antialising and precise drawing.
	for (u32 i = 0; i < 100; i++) {
		f32 t = (f32)i / 100;
		bezier_curve_draw(BackBuffer, PlayerScreenLeft, PlayerScreenBezierTop, PlayerScreenRight, t);
	}

#if DEBUG_VERTICES
	v2f OffsetInPixels = {10.0f, 10.0f};
	v2f MinPos = tile_relative_pos_map_to_screen(GameState, BackBuffer, Player->TileMapPos, Player->TileMapPos.TileRelativePos);
	v2f MaxPos = v2f_add(MinPos, OffsetInPixels);

	v2f MinLeft = PlayerScreenLeft;
	v2f MaxLeft = v2f_add(MinLeft, OffsetInPixels);

	v2f MinRight = PlayerScreenRight;
	v2f MaxRight = v2f_add(MinRight, OffsetInPixels);

	v2f MinTop = PlayerScreenTop;
	v2f MaxTop = v2f_add(MinTop, OffsetInPixels);

	// Position - white
	rectangle_draw(BackBuffer, MinPos, MaxPos, 1.0f, 1.0f, 1.0f);
	// Left - red
	rectangle_draw(BackBuffer, MinLeft, MaxLeft, 1.0f, 0.0f, 0.0f);
	// Right - green
	rectangle_draw(BackBuffer, MinRight, MaxRight, 0.0f, 1.0f, 0.0f);
	// Top - blue
	rectangle_draw(BackBuffer, MinTop, MaxTop, 0.0f, 0.0f, 1.0f);
#endif

}
#endif

internal u32
string_length(char *str)
{
	u32 Result = 0;
	for (char *c = str; *c != '\0'; c++) {
		Result++;
	}
	return(Result);
}

internal b32
strings_are_same(char *str1, char *str2)
{
	// NOTE(Justin): Assume the strings are the same from the
	// outset, if we find that the lengths are in fact different, then they
	// are not the same and set Result to 0. If the lengths are the same
	// but the characters are not all the same, then the strings are
	// different and again set Result to 0.

	b32 Result = 1;

	u32 str1_length = string_length(str1);
	u32 str2_length = string_length(str2);
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

internal void
player_bounding_box_draw(game_state *GameState, back_buffer *BackBuffer, player *Player)
{
#if 0
	v2f P0 = Player->BoundingBox.Min;
	v2f OffsetP1 = v2f_scale(2.0f * Player->base_half_width, Player->Right);
	v2f P1 = v2f_add(P0, OffsetP1);
	v2f P2 = Player->BoundingBox.Max;
	v2f OffsetP3 = v2f_scale(-1.0f, OffsetP1);
	v2f P3 = v2f_add(P2, OffsetP3);

	P0 = get_screen_pos(BackBuffer, P0);
	P1 = get_screen_pos(BackBuffer, P1);
	P2 = get_screen_pos(BackBuffer, P2);
	P3 = get_screen_pos(BackBuffer, P3);

	line_wu_draw(BackBuffer, P0, P1, &Player->ColorWu);
	line_wu_draw(BackBuffer, P1, P2, &Player->ColorWu);
	line_wu_draw(BackBuffer, P2, P3, &Player->ColorWu);
	line_wu_draw(BackBuffer, P3, P0, &Player->ColorWu);
#endif

	v2f OffsetInPixels = v2f_create(10.0f, 10.0f);
	v2f Min = point_map_to_screen(GameState, BackBuffer, Player->BoundingBox.Min);
	v2f Max = v2f_add(Min, OffsetInPixels);
	rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 0.0f);

	Min = point_map_to_screen(GameState, BackBuffer, Player->BoundingBox.Max);
	Max = v2f_add(Min, OffsetInPixels);
	rectangle_draw(BackBuffer, Min, Max, 1.0f, 0.0f, 1.0f);


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

#if 0
internal void
debug_tile_map_position_draw(game_state *GameState, back_buffer *BackBuffer, tile_map_position TileMapPos)
{
	v2f Min;
	Min.x = (f32)TileMapPos.TilePos.x * GameState->meters_per_tile_side;
	Min.y = (f32)TileMapPos.TilePos.y * GameState->meters_per_tile_side;

	v2f Max;
	Max.x = Min.x + GameState->meters_per_tile_side;
	Max.y = Min.y + GameState->meters_per_tile_side;

	Min = v2f_scale(GameState->pixels_per_meter, Min);
	Max = v2f_scale(GameState->pixels_per_meter, Max);

	rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 0.0f);
}
#endif

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

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min ;
	for (int row = y_min; row < y_max; row++) {

		u32 *pixel = (u32 *)pixel_row;
		for (int col = x_min; col < x_max; col++)  {

			*pixel++ = colors_alpha_blend(*pixel, color, alpha);
		}
		pixel_row += BackBuffer->stride;
	}
}

#if 0
internal v2f
get_world_position(game_state *GameState, tile_map_position TileMapPos)
{
	v2f Result = {0};

	v2f TileMapXY;
	TileMapXY.x = TileMapPos.TilePos.x * GameState->meters_per_tile_side + GameState->meters_per_tile_side / 2.0f  + TileMapPos.TileRelativePos.x;
	TileMapXY.y = TileMapPos.TilePos.y * GameState->meters_per_tile_side + GameState->meters_per_tile_side / 2.0f  + TileMapPos.TileRelativePos.y;

	Result = TileMapXY;
	return(Result);
}
#endif


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
bitmap_draw(back_buffer *BackBuffer, loaded_bitmap *Bitmap, f32 x, f32 y)
{
	s32 x_min = f32_round_to_s32(x);
	s32 y_min = f32_round_to_s32(y);

	s32 x_max = x_min + Bitmap->width;
	s32 y_max = y_min + Bitmap->height;

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
update_and_render(game_memory *GameMemory, back_buffer *BackBuffer, sound_buffer *SoundBuffer, game_input *GameInput)
{
	ASSERT(sizeof(game_state) <= GameMemory->total_size);
	game_state *GameState = (game_state *)GameMemory->permanent_storage;
	if (!GameMemory->is_initialized) {

		//GameState->TestSound = wav_file_read_entire("bloop_00.wav");
		//GameState->Background = bitmap_file_read_entire("test_background.bmp");
		//GameState->Background = bitmap_file_read_entire("space_background.bmp");
		//GameState->Ship = bitmap_file_read_entire("ship/blueships1.bmp");

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


		// NOTE(Justin): The world coordinate system is defined by [-w/2, w/2] X [-h/2, h/2]. So (0,0)
		// is the center of the screen. Anytime we go to render a game object,
		// first we must map the objects position back into the coordinate
		// system of the screen. The routine get_screen_pos is used for this purpose.

		player *Player = &GameState->Player;

		Player->TileMapPos.Tile = v2i_create(1, 1);
		Player->TileMapPos.TileRel = v2f_create(0.0f, 0.0f);

		Player->height = 10.0f;
		Player->base_half_width = 4.0f;

		Player->Right = v2f_create(1.0f, 0.0f);
		Player->Direction = v2f_create(0.0f, 1.0f);

		Player->dPos = v2f_create(0.0f, 0.0f);
		Player->speed = 30.0f;
		Player->is_shooting = FALSE;

		GameState->projectile_next = 0;
		GameState->projectile_speed = 60.0f;
		GameState->projectile_half_width = 1.0f;

		f32 asteroid_scales[3] = {1.5f, 3.0f, 5.0f};
		srand(2023);
		for (u32 asteroid_index = 0; asteroid_index < ARRAY_COUNT(GameState->Asteroids); asteroid_index++) {

			// Position
			f32 x_pos = (2 * ((f32)rand() / (f32)RAND_MAX) - 1);
			x_pos *= GameState->WorldHalfDim.x;
			f32 y_pos = (2 * ((f32)rand() / (f32)RAND_MAX) - 1);
			y_pos *= GameState->WorldHalfDim.y;

			// Direction
			f32 x_dir = ((f32)rand() / (f32)RAND_MAX) - 0.5f;
			f32 y_dir = ((f32)rand() / (f32)RAND_MAX) - 0.5f;

			f32 speed_scale = ((f32)rand() / (f32)RAND_MAX);

			GameState->Asteroids[asteroid_index].Pos = v2f_create(x_pos, y_pos);
			GameState->Asteroids[asteroid_index].Direction = v2f_create(x_dir, y_dir);
			GameState->Asteroids[asteroid_index].speed = 9.0f;
			GameState->Asteroids[asteroid_index].dPos =
											v2f_scale(speed_scale * GameState->Asteroids[asteroid_index].speed,
															GameState->Asteroids[asteroid_index].Direction);
			// Local Vertices
			GameState->Asteroids[asteroid_index].LocalVertices[0] = v2f_create(1.7f, -0.6f);
			GameState->Asteroids[asteroid_index].LocalVertices[1] = v2f_create(0.8f , 0.7f);
			GameState->Asteroids[asteroid_index].LocalVertices[2] = v2f_create(-0.7f , 1.1f);
			GameState->Asteroids[asteroid_index].LocalVertices[3] = v2f_create(-0.8f, -1.2f);
			GameState->Asteroids[asteroid_index].LocalVertices[4] = v2f_create(0.4f, -0.6f);
			GameState->Asteroids[asteroid_index].LocalVertices[5] = v2f_create(1.2f, -1.0f);

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
		
			GameState->Asteroids[asteroid_index].LocalVertices[0] = v2f_scale(size_scale, GameState->Asteroids[asteroid_index].LocalVertices[0]);
			GameState->Asteroids[asteroid_index].LocalVertices[1] = v2f_scale(size_scale, GameState->Asteroids[asteroid_index].LocalVertices[1]);
			GameState->Asteroids[asteroid_index].LocalVertices[2] = v2f_scale(size_scale, GameState->Asteroids[asteroid_index].LocalVertices[2]);
			GameState->Asteroids[asteroid_index].LocalVertices[3] = v2f_scale(size_scale, GameState->Asteroids[asteroid_index].LocalVertices[3]);
			GameState->Asteroids[asteroid_index].LocalVertices[4] = v2f_scale(size_scale, GameState->Asteroids[asteroid_index].LocalVertices[4]);
			GameState->Asteroids[asteroid_index].LocalVertices[5] = v2f_scale(size_scale, GameState->Asteroids[asteroid_index].LocalVertices[5]);

			// Orientation
			f32 angle = 2.0f * PI32 * ((f32)rand() / (f32)RAND_MAX);
			m3x3 Rotation = m3x3_rotate_about_origin(angle);
			GameState->Asteroids[asteroid_index].LocalVertices[0] = m3x3_transform_v2f(Rotation, GameState->Asteroids[asteroid_index].LocalVertices[0]);
			GameState->Asteroids[asteroid_index].LocalVertices[1] = m3x3_transform_v2f(Rotation, GameState->Asteroids[asteroid_index].LocalVertices[1]);
			GameState->Asteroids[asteroid_index].LocalVertices[2] = m3x3_transform_v2f(Rotation, GameState->Asteroids[asteroid_index].LocalVertices[2]);
			GameState->Asteroids[asteroid_index].LocalVertices[3] = m3x3_transform_v2f(Rotation, GameState->Asteroids[asteroid_index].LocalVertices[3]);
			GameState->Asteroids[asteroid_index].LocalVertices[4] = m3x3_transform_v2f(Rotation, GameState->Asteroids[asteroid_index].LocalVertices[4]);
			GameState->Asteroids[asteroid_index].LocalVertices[5] = m3x3_transform_v2f(Rotation, GameState->Asteroids[asteroid_index].LocalVertices[5]);

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
			BoundingBoxMin = v2f_scale(1.10f, BoundingBoxMin);
			BoundingBoxMax = v2f_scale(0.9f, BoundingBoxMax);
			GameState->Asteroids[asteroid_index].BoundingBox.Min = v2f_add(GameState->Asteroids[asteroid_index].Pos, BoundingBoxMin);
			GameState->Asteroids[asteroid_index].BoundingBox.Max = v2f_add(GameState->Asteroids[asteroid_index].Pos, BoundingBoxMax);

		}

		GameMemory->is_initialized = TRUE;
	}


	v2f Min = {0};
	v2f Max = v2f_create((f32)BackBuffer->width, (f32)BackBuffer->height);
	rectangle_draw(BackBuffer, Min, Max, 0.0f, 0.0f, 0.0f);

	// NOTE(Justin): Since the backbuffer is bottom up, the first row of the
	// tilemap is the row that gets drawn first and gets drawn at the very
	// bottom of the screen. Which means that the first row maps to the bottom
	// of the screen and the last row maps to the top of the screen.
	// The reason why the backbuffer is bottom up is so that as y values
	// increase, objects move further up the screen.




	tile_map *TileMap = GameState->TileMap;


	v2f BottomLeft = v2f_create(5.0f, 5.0f);
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

	v2f *WorldHalfDim = &GameState->WorldHalfDim;
	if (WorldHalfDim->x != ((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter))) {
		WorldHalfDim->x = ((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter));
	}
	if (WorldHalfDim->y != ((f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter))) {
		WorldHalfDim->y = ((f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter));
	}

	f32 dt_for_frame = GameInput->dt_for_frame;

	player *Player = &GameState->Player;
#if 0
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
#endif

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
			Projectile->Pos = Player->Vertices[PLAYER_TOP];
			Projectile->distance_remaining = 100.0f;
			Projectile->Direction = Player->Direction;
			Projectile->dPos = v2f_scale(GameState->projectile_speed, Projectile->Direction);
			Projectile->active = TRUE;
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
				Projectile->Pos = Player->Vertices[PLAYER_TOP];
				Projectile->distance_remaining = 100.0f;
				Projectile->Direction = Player->Direction;
				Projectile->dPos = v2f_scale(GameState->projectile_speed, Projectile->Direction);
				Projectile->active = TRUE;
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
		if (Projectile->active) {
			v2f DeltaPos = v2f_scale(dt_for_frame, Projectile->dPos);
			Projectile->Pos = v2f_add(Projectile->Pos, DeltaPos);
			Projectile->Pos = point_clip_to_world(WorldHalfDim, Projectile->Pos);

			Projectile->distance_remaining -= v2f_length(DeltaPos);
			if (Projectile->distance_remaining > 0.0f) {
				v2f OffsetInPixels = v2f_create(5.0f, 5.0f);
				v2f Min = point_map_to_screen(GameState, BackBuffer, Projectile->Pos);
				v2f Max = v2f_add(Min, OffsetInPixels);
				rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 1.0f);
			} else {
				Projectile->active = FALSE;
			}

			// TODO(Justin): Do this only if the projectile has distance
			// remaining?
			Projectile->time_between_next_trail -= dt_for_frame;
			if (Projectile->time_between_next_trail <= 0.0f) {
				projectile_trail *Trail = Projectile->Trails + Projectile->trail_next++;
				if (Projectile->trail_next >= ARRAY_COUNT(Projectile->Trails)) {
					Projectile->trail_next = 0;
				}
				Trail->Pos = Projectile->Pos;
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
					v2f Min = point_map_to_screen(GameState, BackBuffer, Trail->Pos);
					v2f Max = v2f_add(Min, OffsetInPixels);
					rectangle_transparent_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 1.0f, Trail->time_left);
				}
			}
		}
	}

	//
	// NOTE(Justin): Move player.
	//

	v2f ddPos = {0};
#if 0
	if (GameInput->Controller.Up.ended_down) {
		ddPos = v2f_create(PlayerNewDirection.x, PlayerNewDirection.y);
	}
	if ((ddPos.x != 0.0f) && (ddPos.y != 0.0f)) {
		ddPos = v2f_scale(HALF_ROOT_TWO, ddPos);
	}
#endif

	if (GameInput->Controller.Right.ended_down) {
		ddPos = v2f_create(1.0f, 0.0f);
	}
	if (GameInput->Controller.Left.ended_down) {
		ddPos = v2f_create(-1.0f, 0.0f);
	}
	if (GameInput->Controller.Up.ended_down) {
		ddPos = v2f_create(0.0f, 1.0f);
	}
	if (GameInput->Controller.Down.ended_down) {
		ddPos = v2f_create(0.0f, -1.0f);
	}
	if ((ddPos.x != 0.0f) && (ddPos.y != 0.0f)) {
		ddPos = v2f_scale(HALF_ROOT_TWO, ddPos);
	}
	ddPos = v2f_scale(Player->speed, ddPos);

#if 0
	v2f dPos;
	dPos.x = ddPos.x * dt_for_frame + Player->dPos.x;
	dPos.y = ddPos.y * dt_for_frame + Player->dPos.y;

	Player->dPos = dPos;

	v2f PosOffset;
	PosOffset.x = 0.5f * ddPos.x * SQURE(dt_for_frame) + Player->dPos.x * dt_for_frame;
	PosOffset.y = 0.5f * ddPos.y * SQURE(dt_for_frame) + Player->dPos.y * dt_for_frame;
#endif
	v2f PosOffset;
	PosOffset = v2f_scale(dt_for_frame, ddPos);

	tile_map_position PlayerNewPos = Player->TileMapPos;
	PlayerNewPos.TileRel = v2f_add(PlayerNewPos.TileRel, PosOffset);
	PlayerNewPos = tile_map_position_remap(TileMap, PlayerNewPos);

	tile_map_position PlayerLeft = PlayerNewPos;
	PlayerLeft.TileRel.x -= 0.5f * TileMap->tile_side_in_meters;
	PlayerLeft = tile_map_position_remap(TileMap, PlayerLeft);

	tile_map_position PlayerRight = PlayerNewPos;
	PlayerRight.TileRel.x += 0.5f * TileMap->tile_side_in_meters;
	PlayerRight = tile_map_position_remap(TileMap, PlayerRight);

	if (tile_map_tile_is_empty(TileMap, PlayerNewPos) &&
 	    tile_map_tile_is_empty(TileMap, PlayerLeft) &&
	    tile_map_tile_is_empty(TileMap, PlayerRight)) {

		Player->TileMapPos = PlayerNewPos;
	   }

	v2f PlayerMin;
	PlayerMin.x = BottomLeft.x + Player->TileMapPos.Tile.x * TileMap->tile_side_in_pixels +
		TileMap->meters_to_pixels * Player->TileMapPos.TileRel.x - TileMap->tile_side_in_pixels / 2.0f;
	PlayerMin.y = BottomLeft.y + Player->TileMapPos.Tile.y * TileMap->tile_side_in_pixels +
		TileMap->meters_to_pixels * Player->TileMapPos.TileRel.y;

	v2f PlayerMax;
	PlayerMax.x = PlayerMin.x + TileMap->tile_side_in_pixels;
	PlayerMax.y = PlayerMin.y + TileMap->tile_side_in_pixels;

	rectangle_draw(BackBuffer, PlayerMin, PlayerMax, 1.0f, 1.0f, 0.0f);

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

#if 0
	for (u32 asteroid_index = 0; asteroid_index < ARRAY_COUNT(GameState->Asteroids); asteroid_index++) {
		asteroid *Asteroid = &GameState->Asteroids[asteroid_index];
		PosOffset = v2f_scale(dt_for_frame, Asteroid->dPos);
		v2f AsteroidNewPos = v2f_add(Asteroid->Pos, PosOffset);
		AsteroidNewPos = point_clip_to_world(WorldHalfDim, AsteroidNewPos);

		// TODO(Justin): Collision Detection after calculating offset and bounding box.

		bounding_box AsteroidNewBoundingBox = Asteroid->BoundingBox;
		AsteroidNewBoundingBox.Min = v2f_add(PosOffset, AsteroidNewBoundingBox.Min);
		AsteroidNewBoundingBox.Max = v2f_add(PosOffset, AsteroidNewBoundingBox.Max);
		AsteroidNewBoundingBox.Min = point_clip_to_world(WorldHalfDim, AsteroidNewBoundingBox.Min);
		AsteroidNewBoundingBox.Max = point_clip_to_world(WorldHalfDim, AsteroidNewBoundingBox.Max);

#if DEBUG_BOUNDING_BOX
		f32 BoundingBoxDx = AsteroidNewBoundingBox.Max.x - AsteroidNewBoundingBox.Min.x;
		f32 BoundingBoxDy = AsteroidNewBoundingBox.Max.y - AsteroidNewBoundingBox.Min.y;

		v2f BoundingBoxP0 = AsteroidNewBoundingBox.Min;
		v2f BoundingBoxP1 = v2f_create(AsteroidNewBoundingBox.Min.x + BoundingBoxDx, AsteroidNewBoundingBox.Min.y);
		v2f BoundingBoxP2 = v2f_create(BoundingBoxP1.x, AsteroidNewBoundingBox.Min.y + BoundingBoxDy);
		v2f BoundingBoxP3 = v2f_create(BoundingBoxP0.x, BoundingBoxP2.y);

		// NOTE(Justin): We must clip the vertices to the world first, then
		// transform the units from meters to pixels. Clipping vertices into
		// the world is done in world space. And we measure distances in
		// meters, not pixels, in the world.
		BoundingBoxP0 = point_clip_to_world(WorldHalfDim, BoundingBoxP0); 
		BoundingBoxP1 = point_clip_to_world(WorldHalfDim, BoundingBoxP1); 
		BoundingBoxP2 = point_clip_to_world(WorldHalfDim, BoundingBoxP2); 
		BoundingBoxP3 = point_clip_to_world(WorldHalfDim, BoundingBoxP3); 

		BoundingBoxP0 = v2f_scale(GameState->pixels_per_meter, BoundingBoxP0); 
		BoundingBoxP1 = v2f_scale(GameState->pixels_per_meter, BoundingBoxP1); 
		BoundingBoxP2 = v2f_scale(GameState->pixels_per_meter, BoundingBoxP2); 
		BoundingBoxP3 = v2f_scale(GameState->pixels_per_meter, BoundingBoxP3); 

		BoundingBoxP0 = get_screen_pos(BackBuffer, BoundingBoxP0);
		BoundingBoxP1 = get_screen_pos(BackBuffer, BoundingBoxP1);
		BoundingBoxP2 = get_screen_pos(BackBuffer, BoundingBoxP2);
		BoundingBoxP3 = get_screen_pos(BackBuffer, BoundingBoxP3);

		line_wu_draw(BackBuffer, BoundingBoxP0, BoundingBoxP1, &GameState->ColorWu[WU_WHITE]);
		line_wu_draw(BackBuffer, BoundingBoxP1, BoundingBoxP2, &GameState->ColorWu[WU_WHITE]);
		line_wu_draw(BackBuffer, BoundingBoxP2, BoundingBoxP3, &GameState->ColorWu[WU_WHITE]);
		line_wu_draw(BackBuffer, BoundingBoxP3, BoundingBoxP0, &GameState->ColorWu[WU_WHITE]);
#endif

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
		Asteroid->Pos = AsteroidNewPos;
		Asteroid->BoundingBox = AsteroidNewBoundingBox;

#if DEBUG_VERTICES
		// Asteroid position
		v2f Min = v2f_scale(GameState->pixels_per_meter, Asteroid->Pos);
		Min = get_screen_pos(BackBuffer, Min);

		v2f Offset = v2f_create(5.0f, 5.0f);
		v2f Max = v2f_add(Min, Offset);
		rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 1.0f);
#endif
		for (u32 vertex_index = 0; vertex_index < ARRAY_COUNT(Asteroid->LocalVertices) - 1; vertex_index++) {
			v2f P1 = v2f_add(Asteroid->Pos, Asteroid->LocalVertices[vertex_index]);
			v2f P2 = v2f_add(Asteroid->Pos, Asteroid->LocalVertices[vertex_index + 1]);

			P1 = v2f_scale(GameState->pixels_per_meter, P1);
			P2 = v2f_scale(GameState->pixels_per_meter, P2);

			P1 = get_screen_pos(BackBuffer, P1);
			P2 = get_screen_pos(BackBuffer, P2);

			line_wu_draw(BackBuffer, P1, P2, &GameState->ColorWu[WU_WHITE]);
		}
		v2f P1 = v2f_add(Asteroid->Pos, Asteroid->LocalVertices[5]);
		v2f P2 = v2f_add(Asteroid->Pos, Asteroid->LocalVertices[0]);
		P1 = v2f_scale(GameState->pixels_per_meter, P1);
		P2 = v2f_scale(GameState->pixels_per_meter, P2);
		P1 = get_screen_pos(BackBuffer, P1);
		P2 = get_screen_pos(BackBuffer, P2);
		line_wu_draw(BackBuffer, P1, P2, &GameState->ColorWu[WU_WHITE]);
	}
#endif




	//player_draw(GameState, BackBuffer, &GameState->Player);

	//sound_buffer_fill(GameState, SoundBuffer);

}
