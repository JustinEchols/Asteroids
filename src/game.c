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
#include "game_math.h"
#include "game_intrinsics.h"

typedef s32 s32_fixed;
#define SHIFT_AMOUNT 16 // number of bits used to represent fractional part, 2^16 = 65536
#define SHIFT_MASK ((1 << SHIFT_AMOUNT) - 1) // Sets LSB to all 1's and MSB to all 0's
#define FIXED_POINT_ONE (1 << SHIFT_AMOUNT)
#define INT_TO_FIXED(x) ((x) << SHIFT_AMOUNT)
#define MAKE_FLOAT_FIXED(x) ((s32_fixed)((x) * FIXED_POINT_ONE))
#define FIXED_TO_INT(x) ((x) >> SHIFT_AMOUNT)
#define FLOAT_TO_FIXED(x) (s32_fixed)(x * (f32)(FIXED_POINT_ONE))
#define FIXED_TO_FLOAT(x) (((f32)(x) / FIXED_POINT_ONE))


#define ONES_COMP(x) ~x
#define TWOS_COMP(x) ~x + 1


#define FIXED_MULT(x, y) ((s32_fixed))((((s64)x) * ((s64)y)) >> SHIFT_AMOUNT)

//#define FIXED_MULT(x, y) (((x) * (y)) >> SHIFT_AMOUNT)
#define FIXED_DIV(x, y) (((x) << SHIFT_AMOUNT) / (y))


#define HALF_ROOT_TWO 0.707106780f
#define PI32 3.1415926535897f


#define NUM_WU_COLORS 2 /* # of colors we'll do antialiased drawing with */
typedef struct
{        
   s16 BaseColor;       /* # of start of palette intensity block in DAC */
   s16 NumLevels;       /* # of intensity levels */
   s16 IntensityBits;   /* IntensityBits == log2 NumLevels */
   s16 MaxRed;          /* red component of color at full intensity */
   s16 MaxGreen;        /* green component of color at full intensity */
   s16 MaxBlue;         /* blue component of color at full intensity */
} WUWU;
//enum {WU_BLUEBLUE=0, WU_WHITEWHITE=1};             /* drawing colors */
WUWU Colors[NUM_WU_COLORS] =  /* blue and white */
    {{192, 32, 5, 0, 0, 0x3F}, {224, 32, 5, 0x3F, 0x3F, 0x3F}};


// If the number of different intensity levels chosen is 32 and we want to
// interpolate white then the base_color is FF - 32 = CD FOR EACH COLOR CHANNEL.
// We must increment each channel, in the case for white, otherwise incrementing
// only one channel changes the inherent color itself. This does not hold for
// other colors. 
//
// How to pipuldate struct
// pick number of levels to be power of 2.
// intensity bits is log_2 of num levels. 
// The base color will be able to assume values from a chosen start color
// plus 0 to num_levels of intensity for example if white is chosen as base
// color and we have 32 levels of intensity then the range of colors is 0xCD to
// 0xFF because the max intensity is 0xFF and the min intensity is 0xFF - 32 = 0xCD. Note
// that each channel will range from 0xCD to 0xFF. That is for a white pixel, the pixel
// is in [0xCDCDCD, 0xFFFFFF].

//color_wu ColorWU[WU_COLOR_COUNT] = {{0x00000000, 256, 8, 0, 0, 0xFF}, {0x00FFFFFF, 32, 5, 0xFF, 0xFF, 0xFF}};



internal void
sound_buffer_fill(sound_buffer *SoundBuffer)
{
	s16 wave_amplitude = 3000;
	int wave_frequency = 256;
	int samples_per_period = SoundBuffer->samples_per_second / wave_frequency;

	local_persist f32 t = 0.0f;
	f32 dt = 2.0f * PI / samples_per_period;

	s16 *sample = SoundBuffer->samples;
	for (int sample_index = 0; sample_index < SoundBuffer->sample_count; sample_index++) {

		s16 sample_value = (s16)(wave_amplitude * sinf(t));

		*sample++ = sample_value;
		*sample++ = sample_value;

		t += dt;
	}
}

internal u32
color_make(f32 r, f32 g, f32 b)
{
	u32 result = 0;
	if (0.0f <= (r * g * b) <= 1.0f) {
		u32 red = (u32)(r * 255.0f);
		u32 green = (u32)(g * 255.0f);
		u32 blue = (u32)(b * 255.0f);

		result = ((red << 16) | (green << 8) | (blue << 0));
	}
	return(result);
}



internal void 
rectangle_draw(back_buffer *BackBuffer, v2f Min, v2f Max, f32 r, f32 g, f32 b)
{
	s32 x_min = f32_round_to_s32(Min.x);
	s32 y_min = f32_round_to_s32(Min.y);
	s32 x_max = f32_round_to_s32(Max.x);
	s32 y_max = f32_round_to_s32(Max.y);

	// Check bounds
	if (x_min < 0) {
		//x_min = BackBuffer->width;
		x_min = 0;
	}
	if (x_max > BackBuffer->width) {
		//x_max = 0;
		x_max = BackBuffer->width;
	}
	if (y_min < 0) {
		//y_min = BackBuffer->height;
		y_min = 0;
	}
	if (y_max > BackBuffer->height) {
		//y_max = 0;
		y_max = BackBuffer->height;
	}

	u32 color = color_make(r, g, b);

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


// Without the bounding box the lowest frame rate was ~75 f/s. With the bounding
// box the lowest frame rate was ~113 f/s. Huge difference!! The "area" of the
// screen that we check without the bounding box is the ENTIRE screen. With the
// bounding box the area is much smaller. We are calling the CRT sqrt() function
// which is EXPENSIVE. Without the bounding box we call this function FOR EACH
// pixel. VERY EXPENSIVE...

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

#if 0
	if (x_min < 0) {
		x_min = BackBuffer->width;
	}
	if (x_max > BackBuffer->width) {
		x_max = 0;
	}
	if (y_min < 0) {
		y_min = BackBuffer->height;
	}
	if (y_max > BackBuffer->height) {
		y_max = 0;
	}
#endif

	u32 color = color_make(r, g, b);
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

internal void
line_wu_draw(back_buffer *BackBuffer, v2f P1, v2f P2, color_wu *ColorWu)
{
	s32 x0 = f32_round_to_s32(P1.x);
	s32 y0 = f32_round_to_s32(P1.y);

	s32 x1 = f32_round_to_s32(P2.x);
	s32 y1 = f32_round_to_s32(P2.y);

	u32 base_color = ColorWu->base_color;

	if (y0 > y1) {
		s32 temp = x0;
		x0 = x1;
		x1 = temp;

		temp = y0;
		y0 = y1;
		y1 = temp;
	}
	pixel_set(BackBuffer, x0, y0, base_color);

	s32 dy = y1 - y0;
	s32 dx = x1 - x0;

	s32 x_dir;
	if (dx >= 0) {
		x_dir = 1;
	} else {
		x_dir = -1;
		dx = -dx;
	}

	if (dy == 0) {
		do {
			x0 += x_dir;
			pixel_set(BackBuffer, x0, y0, base_color);
		} while (--dx != 0);
		return;
	}

	if (dx == 0) {
		do {
			y0++;
			pixel_set(BackBuffer, x0, y0, base_color);
		} while (--dy != 0);
		return;
	}

	if (dy == dx) {
		do {
			x0 += x_dir;
			y0++;
			pixel_set(BackBuffer, x0, y0, base_color);
		} while (--dy != 0);
		return;
	}


	u32 error_accum = 0;
	u32 intensity_shift = 32 - ColorWu->intensity_bits;
	u32 weighting_comp_mask = ColorWu->num_levels - 1;

	u32 red_mask = ColorWu->red_mask;
	u32 green_mask = ColorWu->green_mask;
	u32 blue_mask = ColorWu->blue_mask;

	u32 error_adj, error_accum_temp, weighting;
	if (dy > dx) {
		error_adj = ((u64)dx << 32) / (u64)dy;
		while (--dy) {
			error_accum_temp = error_accum;
			error_accum += error_adj;
			if (error_accum <= error_accum_temp) {
				x0 += x_dir;
			}
			y0++;

			weighting = error_accum >> intensity_shift;

			u32 red = ((weighting & red_mask) << 16);
			u32 green = ((weighting & green_mask) << 8);
			u32 blue = weighting & blue_mask;
			u32 color = (red | green | blue);
			pixel_set(BackBuffer, x0 + x_dir, y0, color);

			red = (((weighting ^ weighting_comp_mask) & red_mask) << 16);
			green = (((weighting ^ weighting_comp_mask) & green_mask) << 8);
			blue = ((weighting ^ weighting_comp_mask) & blue_mask);
			u32 comp_color = (red | green | blue);
			pixel_set(BackBuffer, x0, y0, comp_color);
		}
		pixel_set(BackBuffer, x1, y1, base_color);
		return;
	}

	error_adj = ((u64)dy << 32) / (u64)dx;
	while (--dx) {
		error_accum_temp = error_accum;
		error_accum += error_adj;
		if (error_accum <= error_accum_temp) {
			y0++;
		}
		x0 += x_dir;

		weighting = error_accum >> intensity_shift;

		u32 red = ((weighting & red_mask) << 16);
		u32 green = ((weighting & green_mask) << 8);
		u32 blue = weighting & blue_mask;
		u32 color = (red | green | blue);
		pixel_set(BackBuffer, x0, y0 + 1, color);

		red = (((weighting ^ weighting_comp_mask) & red_mask) << 16);
		green = (((weighting ^ weighting_comp_mask) & green_mask) << 8);
		blue = ((weighting ^ weighting_comp_mask) & blue_mask);
		u32 comp_color = (red | green | blue);
		pixel_set(BackBuffer, x0, y0, comp_color);
	}
	pixel_set(BackBuffer, x1, y1, base_color);
}

internal v2f
tile_relative_pos_map_to_screen(game_state *GameState, back_buffer *BackBuffer,
								tile_map_position TileMapPos, v2f PlayerVertex)
{
	v2f Result = {0};

#if 1
	v2f TileCenter;
	TileCenter.x = (f32)TileMapPos.TilePos.x * GameState->meters_per_tile + GameState->meters_per_tile / 2.0f;
	TileCenter.y = (f32)TileMapPos.TilePos.y * GameState->meters_per_tile + GameState->meters_per_tile / 2.0f;


	v2f WorldXY;
	WorldXY.x = TileCenter.x + TileMapPos.TileRelativePos.x + PlayerVertex.x; 
	WorldXY.y = TileCenter.y + TileMapPos.TileRelativePos.y + PlayerVertex.y;

	v2f ScreenXY;
	ScreenXY.x = GameState->pixels_per_meter * WorldXY.x;
	ScreenXY.y = GameState->pixels_per_meter * WorldXY.y;
	Result = ScreenXY;
#endif
#if 0
	Result.x = (f32)TileMapPos.TilePos.x * GameState->meters_per_tile + 
		TileMapPos.TileRelativePos.x + PlayerVertex.x + (GameState->meters_per_tile / 2.0f);
	Result.y = (f32)TileMapPos.TilePos.y * GameState->meters_per_tile + 
		TileMapPos.TileRelativePos.y + PlayerVertex.y + (GameState->meters_per_tile / 2.0f);

	Result = v2f_scale(GameState->pixels_per_meter, Result);
#endif


	return(Result);
}




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




	//v2f PlayerScreenLeft = point_map_to_screen(GameState, BackBuffer, Player->Vertices[PLAYER_LEFT]);
	//v2f PlayerScreenRight = point_map_to_screen(GameState, BackBuffer, Player->Vertices[PLAYER_RIGHT]);
	//v2f PlayerScreenTop = point_map_to_screen(GameState, BackBuffer, Player->Vertices[PLAYER_TOP]);
	//v2f PlayerScreenBezierTop = point_map_to_screen(GameState, BackBuffer, Player->Vertices[PLAYER_BEZIER_TOP]);

	line_wu_draw(BackBuffer, PlayerScreenLeft, PlayerScreenTop, &Player->ColorWu);
	line_wu_draw(BackBuffer, PlayerScreenRight, PlayerScreenTop, &Player->ColorWu);

	// NOTE(Justin): The 100 is arbitrary and fudged to make sure the curve is filled with pixels.
	// TODO(Justin): Antialising and precise drawing.
	for (u32 i = 0; i < 100; i++) {
		f32 t = (f32)i / 100;
		bezier_curve_draw(BackBuffer, PlayerScreenLeft, PlayerScreenBezierTop, PlayerScreenRight, t);
	}

	// Shield
	//circle_draw(GameState, BackBuffer, &Player->Shield, 1.0f, 1.0f, 1.0f);

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

internal v3f
color_get(game_state *GameState, char *name)
{
	v3f Result = {0};
	for (u32 color_index = 0; color_index < ARRAY_COUNT(GameState->Colors); color_index++) {
		color TestColor = GameState->Colors[color_index];
		if (strings_are_same(name, TestColor.name)) {
			Result = TestColor.ColorValue;
			break;
		}
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

internal void
debug_tile_map_position_draw(game_state *GameState, back_buffer *BackBuffer, tile_map_position TileMapPos)
{
	v2f Min;
	Min.x = (f32)TileMapPos.TilePos.x * GameState->meters_per_tile;
	Min.y = (f32)TileMapPos.TilePos.y * GameState->meters_per_tile;

	v2f Max;
	Max.x = Min.x + GameState->meters_per_tile;
	Max.y = Min.y + GameState->meters_per_tile;

	Min = v2f_scale(GameState->pixels_per_meter, Min);
	Max = v2f_scale(GameState->pixels_per_meter, Max);

	rectangle_draw(BackBuffer, Min, Max, 1.0f, 1.0f, 0.0f);
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

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min ;
	for (int row = y_min; row < y_max; row++) {

		u32 *pixel = (u32 *)pixel_row;
		for (int col = x_min; col < x_max; col++)  {

			*pixel++ = colors_alpha_blend(*pixel, color, alpha);
		}
		pixel_row += BackBuffer->stride;
	}
}

internal void
DrawWuLine (back_buffer *BackBuffer, short X0, short Y0, short X1, short Y1, short BaseColor, short NumLevels, unsigned short IntensityBits)
{
   unsigned short IntensityShift, ErrorAdj, ErrorAcc;
   unsigned short ErrorAccTemp, Weighting, WeightingComplementMask;
   short DeltaX, DeltaY, Temp, XDir;

   /* Make sure the line runs top to bottom */
   if (Y0 > Y1) {
      Temp = Y0; Y0 = Y1; Y1 = Temp;
      Temp = X0; X0 = X1; X1 = Temp;
   }
   /* Draw the initial pixel, which is always exactly intersected by
      the line and so needs no weighting */
   pixel_set_v2(BackBuffer ,X0, Y0, BaseColor);

   if ((DeltaX = X1 - X0) >= 0) {
      XDir = 1;
   } else {
      XDir = -1;
      DeltaX = -DeltaX; /* make DeltaX positive */
   }
   /* Special-case horizontal, vertical, and diagonal lines, which
      require no weighting because they go right through the center of
      every pixel */
   if ((DeltaY = Y1 - Y0) == 0) {
      /* Horizontal line */
      while (DeltaX-- != 0) {
         X0 += XDir;
		 pixel_set_v2(BackBuffer ,X0, Y0, BaseColor);
      }
      return;
   }
   if (DeltaX == 0) {
      /* Vertical line */
      do {
         Y0++;
		 pixel_set_v2(BackBuffer ,X0, Y0, BaseColor);
      } while (--DeltaY != 0);
      return;
   }
   if (DeltaX == DeltaY) {
      /* Diagonal line */
      do {
         X0 += XDir;
         Y0++;
		 pixel_set_v2(BackBuffer ,X0, Y0, BaseColor);
      } while (--DeltaY != 0);
      return;
   }
   /* Line is not horizontal, diagonal, or vertical */
   ErrorAcc = 0;  /* initialize the line error accumulator to 0 */
   /* # of bits by which to shift ErrorAcc to get intensity level */
   IntensityShift = 16 - IntensityBits;
   /* Mask used to flip all bits in an intensity weighting, producing the
      result (1 - intensity weighting) */
   WeightingComplementMask = NumLevels - 1;
   /* Is this an X-major or Y-major line? */
   if (DeltaY > DeltaX) {
      /* Y-major line; calculate 16-bit fixed-point fractional part of a
         pixel that X advances each time Y advances 1 pixel, truncating the
         result so that we won't overrun the endpoint along the X axis */
      ErrorAdj = ((unsigned long) DeltaX << 16) / (unsigned long) DeltaY;
      /* Draw all pixels other than the first and last */
      while (--DeltaY) {
         ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
         ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
         if (ErrorAcc <= ErrorAccTemp) {
            /* The error accumulator turned over, so advance the X coord */
            X0 += XDir;
         }
         Y0++; /* Y-major, so always advance Y */
         /* The IntensityBits most significant bits of ErrorAcc give us the
            intensity weighting for this pixel, and the complement of the
            weighting for the paired pixel */
         Weighting = ErrorAcc >> IntensityShift;
		 pixel_set_v2(BackBuffer ,X0, Y0, BaseColor + Weighting);
		 pixel_set_v2(BackBuffer ,X0 + XDir, Y0, BaseColor + (Weighting ^ WeightingComplementMask));
      }
      /* Draw the final pixel, which is always exactly intersected by the line
         and so needs no weighting */
	  pixel_set_v2(BackBuffer ,X1, Y1, BaseColor);
      return;
   }
   /* It's an X-major line; calculate 16-bit fixed-point fractional part of a
      pixel that Y advances each time X advances 1 pixel, truncating the
      result to avoid overrunning the endpoint along the X axis */
   ErrorAdj = ((unsigned long) DeltaY << 16) / (unsigned long) DeltaX;
   /* Draw all pixels other than the first and last */
   while (--DeltaX) {
      ErrorAccTemp = ErrorAcc;   /* remember currrent accumulated error */
      ErrorAcc += ErrorAdj;      /* calculate error for next pixel */
      if (ErrorAcc <= ErrorAccTemp) {
         /* The error accumulator turned over, so advance the Y coord */
         Y0++;
      }
      X0 += XDir; /* X-major, so always advance X */
      /* The IntensityBits most significant bits of ErrorAcc give us the
         intensity weighting for this pixel, and the complement of the
         weighting for the paired pixel */
      Weighting = ErrorAcc >> IntensityShift;
	  pixel_set_v2(BackBuffer ,X0, Y0, BaseColor + Weighting);
	  pixel_set_v2(BackBuffer ,X0, Y0 + 1, BaseColor + (Weighting ^ WeightingComplementMask));
   }
   /* Draw the final pixel, which is always exactly intersected by the line
      and so needs no weighting */
   pixel_set_v2(BackBuffer ,X1, Y1, BaseColor);
}


internal void
update_and_render(game_memory *GameMemory, back_buffer *BackBuffer, sound_buffer *SoundBuffer, game_input *GameInput)
{
	game_state *GameState = (game_state *)GameMemory->memory_block;
	if (!GameMemory->is_initialized) {

		GameState->pixels_per_meter = 5.0f;
		GameState->WorldHalfDim = v2f_create((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter),
											 (f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter));

		GameState->tile_count_x = 17;
		GameState->tile_count_y = 9;
		GameState->meters_per_tile = 11.0f;

		//GameState->tile_count_x = f32_round_to_u32(2.0f * GameState->WorldHalfDim.x / GameState->meters_per_tile);
		//GameState->tile_count_y = f32_round_to_u32(2.0f * GameState->WorldHalfDim.y / GameState->meters_per_tile);

		v3f White = {1.0f, 1.0f, 1.0f};
		v3f Red = {1.0f, 0.0f, 0.0f};
		v3f Green = {0.0f, 1.0f, 0.0f};
		v3f Blue = {0.0f, 0.0f, 1.0f};
		v3f Black = {0.0f, 0.0f, 0.0f};

		GameState->Colors[0].name = "white";
		GameState->Colors[0].ColorValue = White;
		GameState->Colors[1].name = "red";
		GameState->Colors[1].ColorValue = Red;
		GameState->Colors[2].name = "green";
		GameState->Colors[2].ColorValue = Green;
		GameState->Colors[3].name = "blue";
		GameState->Colors[3].ColorValue = Blue;
		GameState->Colors[4].name = "black";
		GameState->Colors[4].ColorValue = Black;

		// NOTE(Justin): The world coordinate system is defined by [-w/2, w/2] X [-h/2, h/2]. So (0,0)
		// is the center of the screen. Anytime we go to render a game object,
		// first we must map the objects position back into the coordinate
		// system of the screen. The routine get_screen_pos is used for this purpose.

		player *Player = &GameState->Player;

		Player->TileMapPos.TilePos = v2i_create(0, 0);
		Player->TileMapPos.TileRelativePos = v2f_create(0.0f, 0.0f);

		Player->Pos = v2f_create(0.0f, 0.0f);
		Player->height = 10.0f;
		Player->base_half_width = 4.0f;

		Player->Right = v2f_create(1.0f, 0.0f);
		Player->Direction = v2f_create(0.0f, 1.0f);

		v2f DownOffset = v2f_scale(-1.0f * (Player->height / 2.0f), Player->Direction);

		v2f RightOffset = v2f_scale(Player->base_half_width, Player->Right);
		RightOffset = v2f_add(DownOffset, RightOffset);
		v2f Right = v2f_add(Player->TileMapPos.TileRelativePos, RightOffset);

		v2f LeftOffset = v2f_scale(-1.0f * Player->base_half_width, Player->Right);
		LeftOffset = v2f_add(DownOffset, LeftOffset);
		v2f Left = v2f_add(Player->TileMapPos.TileRelativePos, LeftOffset);

		v2f TopOffset = v2f_scale(Player->height / 2.0f, Player->Direction);
		v2f Top = v2f_add(Player->TileMapPos.TileRelativePos, TopOffset);

		v2f BezierTopOffset = v2f_scale(-0.5f, TopOffset);
		v2f BezierTop = v2f_add(Player->TileMapPos.TileRelativePos, BezierTopOffset);

		Player->Vertices[0] = Left;
		Player->Vertices[1] = Right;
		Player->Vertices[2] = Top;
		Player->Vertices[3] = BezierTop;

		Player->dPos = v2f_create(0.0f, 0.0f);
		Player->speed = 30.0f;
		Player->is_shooting = FALSE;

		color_wu WuWhite = {0xFFFFFF, 256, 8, 0xFF, 0xFF, 0xFF};

		Player->ColorWu = WuWhite;

		Player->BoundingBox.Min = Left;
		Player->BoundingBox.Max = v2f_scale(-1.0f, Player->BoundingBox.Min);

		Player->Shield.Center = Player->TileMapPos.TileRelativePos;
		Player->Shield.radius = Player->height / 2.0f;

	
		GameState->projectile_next = 0;
		GameState->projectile_speed = 60.0f;
		GameState->projectile_half_width = 1.0f;

		color_wu WuBlack = {0x00000000, 256, 8, 0, 0, 0xFF};
		color_wu WuRed = {0xFF0000, 256, 8, 0xFF, 0x00, 0x00};
		color_wu WuGreen = {0x00FF00, 256, 8, 0x00, 0xFF, 0x00};
		color_wu WuBlue = {0x0000FF, 256, 8, 0x00, 0x00, 0xFF};
		color_wu WuYellow = {0xFFFF00, 256, 8, 0x00, 0x00, 0xFF};
		color_wu WuPurple = {0xFF00FF, 256, 8, 0x00, 0x00, 0xFF};

		GameState->ColorWu[WU_BLACK] = WuBlack;
		GameState->ColorWu[WU_WHITE] = WuWhite;
		GameState->ColorWu[WU_RED] = WuRed;
		GameState->ColorWu[WU_GREEN] = WuGreen;
		GameState->ColorWu[WU_BLUE] = WuBlue;
		GameState->ColorWu[WU_YELLOW] = WuYellow;
		GameState->ColorWu[WU_PURPLE] = WuPurple;

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

		memory_arena_initialize(&GameState->WorldArena, GameMemory->total_size - sizeof(game_state),
										(u8 *)GameMemory->memory_block + sizeof(game_state));

		GameMemory->is_initialized = TRUE;
	}

	v2f BackBufferMin = {0};
	v2f BackBufferMax = {(f32)BackBuffer->width, (f32)BackBuffer->height};
	rectangle_draw(BackBuffer, BackBufferMin, BackBufferMax, 0.0f, 0.0f, 0.0f); 

	v2f *WorldHalfDim = &GameState->WorldHalfDim;
	if (WorldHalfDim->x != ((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter))) {
		WorldHalfDim->x = ((f32)BackBuffer->width / (2.0f * GameState->pixels_per_meter));
	}
	if (WorldHalfDim->y != ((f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter))) {
		WorldHalfDim->y = ((f32)BackBuffer->height / (2.0f * GameState->pixels_per_meter));
	}


#if DEBUG_TILE_MAP
	for (s32 tile_y = 0; tile_y < GameState->tile_count_y; tile_y++) {
		f32 horizontal_line_y = tile_y * GameState->meters_per_tile * GameState->pixels_per_meter;
		line_horizontal_draw(BackBuffer, horizontal_line_y, 1.0f, 1.0f, 1.0f);
	}
	for (s32 tile_x = 0; tile_x < GameState->tile_count_x; tile_x++) {
		f32 vertical_line_x = tile_x * GameState->meters_per_tile * GameState->pixels_per_meter;
		line_vertical_draw(BackBuffer, vertical_line_x, 1.0f, 1.0f, 1.0f);
	}
#endif


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
	if (GameInput->Controller.Up.ended_down) {
		ddPos = v2f_create(PlayerNewDirection.x, PlayerNewDirection.y);
	}
	if ((ddPos.x != 0.0f) && (ddPos.y != 0.0f)) {
		ddPos = v2f_scale(HALF_ROOT_TWO, ddPos);
	}
	ddPos = v2f_scale(Player->speed, ddPos);

	v2f dPos;
	dPos.x = ddPos.x * dt_for_frame + Player->dPos.x;
	dPos.y = ddPos.y * dt_for_frame + Player->dPos.y;

	v2f PosOffset;
	PosOffset.x = 0.5f * ddPos.x * SQURE(dt_for_frame) + Player->dPos.x * dt_for_frame;
	PosOffset.y = 0.5f * ddPos.y * SQURE(dt_for_frame) + Player->dPos.y * dt_for_frame;

	v2f PlayerNewTileRelativePos = v2f_add(Player->TileMapPos.TileRelativePos, PosOffset);
	v2i PlayerNewTilePos = Player->TileMapPos.TilePos;

	if (PlayerNewTileRelativePos.x > (GameState->meters_per_tile / 2.0f)) {
		//PlayerNewTileRelativePos.x -= GameState->meters_per_tile;
		PlayerNewTileRelativePos.x -= GameState->meters_per_tile / 2.0f;
		PlayerNewTilePos.x++;
		if (PlayerNewTilePos.x >= GameState->tile_count_x) {
			PlayerNewTilePos.x = 0;
		}
	}
	if (PlayerNewTileRelativePos.x < (-GameState->meters_per_tile / 2.0f)) {
		//PlayerNewTileRelativePos.x += GameState->meters_per_tile;
		PlayerNewTileRelativePos.x += GameState->meters_per_tile / 2.0f;
		PlayerNewTilePos.x--;
		if (PlayerNewTilePos.x < 0) {
			PlayerNewTilePos.x = GameState->tile_count_x - 1;
		}
	}
	if (PlayerNewTileRelativePos.y > (GameState->meters_per_tile / 2.0f)) {
		//PlayerNewTileRelativePos.y -= GameState->meters_per_tile;
		PlayerNewTileRelativePos.y -= GameState->meters_per_tile / 2.0f;
		PlayerNewTilePos.y++;
		if (PlayerNewTilePos.y >= GameState->tile_count_y) {
			PlayerNewTilePos.y = 0;
		}
	}
	if (PlayerNewTileRelativePos.y < (-GameState->meters_per_tile / 2.0f)) {
		//PlayerNewTileRelativePos.y += GameState->meters_per_tile;
		PlayerNewTileRelativePos.y += GameState->meters_per_tile / 2.0f;
		PlayerNewTilePos.y--;
		if (PlayerNewTilePos.y < 0) {
			PlayerNewTilePos.y = GameState->tile_count_y - 1;
		}

	}
#if DEBUG_TILE_MAP
	tile_map_position DebugTileMapPos;
	DebugTileMapPos.TilePos = PlayerNewTilePos;
	DebugTileMapPos.TileRelativePos = PlayerNewTileRelativePos;
	debug_tile_map_position_draw(GameState, BackBuffer, DebugTileMapPos);
#endif



	//PlayerNewPos = point_clip_to_world(WorldHalfDim, PlayerNewPos);
	//PlayerNewPos = v2i_scale(GameState->meters_per_tile, Player->TileMapPos);
	//PlayerNewPos.x = (f32)PlayerNewTilePos.x * GameState->meters_per_tile + PlayerNewTileRelativePos.x;
	//PlayerNewPos.y = (f32)PlayerNewTilePos.y * GameState->meters_per_tile + PlayerNewTileRelativePos.y;

	v2f PlayerNewVertices[PLAYER_VERTEX_COUNT];

	v2f DownOffset = v2f_scale(-1.0f * (Player->height / 2.0f), PlayerNewDirection);
	v2f RightOffset = v2f_scale(Player->base_half_width, PlayerNewRight);
	RightOffset = v2f_add(DownOffset, RightOffset);
	v2f PlayerNewVertexRight = v2f_add(PlayerNewTileRelativePos, RightOffset);

	v2f LeftOffset = v2f_scale(-1.0f * Player->base_half_width, PlayerNewRight);
	LeftOffset = v2f_add(DownOffset, LeftOffset);
	v2f PlayerNewVertexLeft = v2f_add(PlayerNewTileRelativePos, LeftOffset);

	v2f TopOffset = v2f_scale(Player->height / 2.0f, PlayerNewDirection);
	v2f PlayerNewVertexTop = v2f_add(PlayerNewTileRelativePos, TopOffset);

	v2f BezierTopOffset = v2f_scale(-0.5f, TopOffset);
	v2f PlayerNewVertexBezierTop = v2f_add(PlayerNewTileRelativePos, BezierTopOffset);

	PlayerNewVertices[PLAYER_LEFT] = PlayerNewVertexLeft;
	PlayerNewVertices[PLAYER_RIGHT] = PlayerNewVertexRight;
	PlayerNewVertices[PLAYER_TOP] = PlayerNewVertexTop;
	PlayerNewVertices[PLAYER_BEZIER_TOP] = PlayerNewVertexBezierTop;

	// NOTE(Justin): Assume that the bounding box min and max are the left and
	// right vertices of the player, respectively. Then check if it is true and
	// update the bounding box min/max if not true.

	v2f BoundingBoxMin = PlayerNewVertices[PLAYER_LEFT];
	v2f BoundingBoxMax = PlayerNewVertices[PLAYER_RIGHT];
	for (u32 vertex_index = 0; vertex_index < ARRAY_COUNT(PlayerNewVertices); vertex_index++) {
		v2f Vertex = PlayerNewVertices[vertex_index];
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
	bounding_box PlayerNewBoundingBox = {0};
	PlayerNewBoundingBox.Min = BoundingBoxMin;
	PlayerNewBoundingBox.Max = BoundingBoxMax;
	

#if DEBUG_BOUNDING_BOX
	player_bounding_box_draw(GameState, BackBuffer, Player);
#endif
	
	// TODO(Justin): Experiment with two bounding boxes for the player. A large
	// and small bounding box. Or continue with one bounding box and if the
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

	//Player->Pos = PlayerNewPos;
	Player->Right = PlayerNewRight;
	Player->Direction = PlayerNewDirection;
	Player->dPos = dPos;
	Player->BoundingBox = PlayerNewBoundingBox;
	Player->Vertices[PLAYER_LEFT] = PlayerNewVertices[PLAYER_LEFT];
	Player->Vertices[PLAYER_RIGHT] = PlayerNewVertices[PLAYER_RIGHT];
	Player->Vertices[PLAYER_TOP] = PlayerNewVertices[PLAYER_TOP];
	Player->Vertices[PLAYER_BEZIER_TOP] = PlayerNewVertices[PLAYER_BEZIER_TOP];
	Player->Shield.Center = Player->Pos;

	Player->TileMapPos.TilePos = PlayerNewTilePos;
	Player->TileMapPos.TileRelativePos = PlayerNewTileRelativePos;

	player_draw(GameState, BackBuffer, &GameState->Player);

#if 0
	s16 X0 = 0;
	s16 Y0 = (s16)BackBuffer->height - 1;
	s16 X1 = (s16)BackBuffer->width / 2;
	s16 Y1 = (s16)BackBuffer->height / 2;
	DrawWuLine (BackBuffer, X0, Y0, X1, Y1, Colors[0].BaseColor, Colors[0].NumLevels, Colors[0].IntensityBits);
#endif

	//sound_buffer_fill(SoundBuffer);

}
