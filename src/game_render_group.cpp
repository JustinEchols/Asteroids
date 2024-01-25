
inline v4f
unpack4x8(u32 packed)
{
	v4f Result = {(f32)((packed >> 16) & 0xFF),
				  (f32)((packed >> 8) & 0xFF),
				  (f32)((packed >> 0) & 0xFF),
				  (f32)((packed >> 24) & 0xFF)};

	return(Result);
}

inline v4f
srgb_255_to_linear01(v4f C)
{
	v4f Result;

	f32 inv_255 = 1.0f / 255.0f;

	Result.r = SQUARE(inv_255 * C.r);
	Result.g = SQUARE(inv_255 * C.g);
	Result.b = SQUARE(inv_255 * C.b);
	Result.a = inv_255 * C.a;

	return(Result);
}

inline v4f
linear01_to_srgb_255(v4f C)
{
	v4f Result;

	f32 one_255 = 255.0f;

	Result.r = one_255 * f32_sqrt(C.r);
	Result.g = one_255 * f32_sqrt(C.g);
	Result.b = one_255 * f32_sqrt(C.b);
	Result.a = one_255 * C.a;
	
	return(Result);
}

inline v4f
normal_rescale_and_bias(v4f Normal)
{
	v4f Result;

	f32 inv_255 = 1.0f/ 255.0f;

	Result.x = -1.0f + 2.0f * (inv_255 * Normal.x);
	Result.y = -1.0f + 2.0f * (inv_255 * Normal.y);
	Result.z = -1.0f + 2.0f * (inv_255 * Normal.z);

	Result.w = inv_255 * Normal.w;

	return(Result);
}

struct bilinear_sample 
{
	u32 a, b, c, d;
};


inline bilinear_sample
bilinear_sample_texture(loaded_bitmap *Texture, s32 x, s32 y)
{
	bilinear_sample Result;

	u8 *texel_ptr = (u8 *)Texture->memory + y * Texture->stride + x * sizeof(u32);
	Result.a = *(u32 *)texel_ptr;
	Result.b = *(u32 *)(texel_ptr + sizeof(u32));
	Result.c = *(u32 *)(texel_ptr + Texture->stride);
	Result.d = *(u32 *)(texel_ptr + Texture->stride + sizeof(u32));

	return(Result);
}

inline v4f
srgb_bilinear_blend(bilinear_sample Sample, f32 fx, f32 fy)
{
	v4f TexelA = unpack4x8(Sample.a);
	v4f TexelB = unpack4x8(Sample.b);
	v4f TexelC = unpack4x8(Sample.c);
	v4f TexelD = unpack4x8(Sample.d);

	TexelA = srgb_255_to_linear01(TexelA);
	TexelB = srgb_255_to_linear01(TexelB);
	TexelC = srgb_255_to_linear01(TexelC);
	TexelD = srgb_255_to_linear01(TexelD);

	// NOTE(Justin): Bilinear filtering
	v4f Result = lerp(lerp(TexelA, fx, TexelB), fy, lerp(TexelC, fx, TexelD));

	return(Result);
}

internal v3f
normal_map_sample(v2f ScreenSpaceUV, environment_map *Map, v3f Normal, f32 roughness)
{
	v3f Result = Normal;

	return(Result);
}

internal v3f
environment_map_sample(v2f ScreenSpaceUV, environment_map *Map, v3f Normal, f32 roughness)
{
	v3f Result;

	u32 lod_index = (u32)(roughness * (f32)(ARRAY_COUNT(Map->LOD) - 1) + 0.5f);
	ASSERT(lod_index < ARRAY_COUNT(Map->LOD));

	loaded_bitmap *LOD = &Map->LOD[lod_index];

	//f32 tx = ((u * (f32)(Texture->width - 2)));
	//f32 ty = ((v * (f32)(Texture->height - 2)));
	
	f32 tx = 0.0f;
	f32 ty = 0.0f;

	s32 x = (s32)tx;
	s32 y = (s32)ty;

	// t values for bilinear filtering
	f32 fx = tx - (f32)x;
	f32 fy = ty - (f32)y;

	ASSERT((x >= 0) && (x < LOD->width));
	ASSERT((y >= 0) && (y < LOD->height));

	bilinear_sample Sample = bilinear_sample_texture(LOD, x, y);
	Result = srgb_bilinear_blend(Sample, fx, fy).xyz;

	return(Result);
}



internal void 
rectangle_draw_slowly(loaded_bitmap *Buffer, v2f Origin, v2f XAxis, v2f YAxis, v4f Color,
					  loaded_bitmap *Texture, loaded_bitmap *NormalMap,
					  environment_map *Top,
					  environment_map *Middle,
					  environment_map *Bottom)

{
	// NOTE(Justin): Pre-muiltiple color at the start
	Color.rgb *= Color.a;

	f32 inv_x_axis_len_sq = 1.0f / v2f_dot(XAxis, XAxis);
	f32 inv_y_axis_len_sq = 1.0f / v2f_dot(YAxis, YAxis);

	s32 width_max = Buffer->width - 1;
	s32 height_max = Buffer->height - 1;

	f32 inv_width_max = 1.0f / (f32)width_max;
	f32 inv_height_max = 1.0f / (f32)height_max;

	s32 x_min = width_max;
	s32 y_min = height_max;
	s32 x_max = 0;
	s32 y_max = 0;

	v2f P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};
	for(u32 p_index = 0; p_index < ARRAY_COUNT(P); p_index++)
	{
		v2f TestP = P[p_index];
		s32 x_floor = f32_floor_to_s32(TestP.x);
		s32 y_floor = f32_floor_to_s32(TestP.y);
		s32 x_ceil = f32_ceil_to_s32(TestP.x);
		s32 y_ceil = f32_ceil_to_s32(TestP.y);

		if(x_min > x_floor) {x_min = x_floor;}
		if(y_min > y_floor) {y_min = y_floor;}
		if(x_max < x_ceil) {x_max = x_ceil;}
		if(y_max < y_ceil) {y_max = y_ceil;}
	}

	if(x_min < 0) {x_min = 0;}
	if(y_min < 0) {y_min = 0;}
	if(x_max > (width_max)) {x_max = width_max;}
	if(y_max > (height_max)) {y_max = height_max;}

	u8 *pixel_row = (u8 *)Buffer->memory + Buffer->stride * y_min + BITMAP_BYTES_PER_PIXEL * x_min;
	for(s32 row = y_min; row <= y_max; row++)
	{
		u32 *pixel = (u32 *)pixel_row;
		for(s32 col = x_min; col <= x_max; col++)
		{
			v2f PixelP = V2F((f32)col, (f32)row);
			v2f D = PixelP - Origin;
			f32 e0 = v2f_dot(D, -1.0f * v2f_perp(XAxis));
			f32 e1 = v2f_dot(D - XAxis, -1.0f * v2f_perp(YAxis));
			f32 e2 = v2f_dot(D - XAxis - YAxis, v2f_perp(XAxis));
			f32 e3 = v2f_dot(D - YAxis, v2f_perp(YAxis));

			if((e0 < 0) && (e1 < 0) && (e2 < 0) && (e3 < 0))
			{
				// (u, v) in [0, 1] X [0, 1]
				f32 u = inv_x_axis_len_sq * v2f_dot(D, XAxis);
				f32 v = inv_y_axis_len_sq * v2f_dot(D, YAxis);

				// TODO(Justin): Clamp.
				ASSERT((u >= 0.0f) && (u <= 1.0f));
				ASSERT((v >= 0.0f) && (v <= 1.0f));

				// Scale the uv coordinates to "almost" the entire dim of the bitmap
				f32 tx = ((u * (f32)(Texture->width - 2)));
				f32 ty = ((v * (f32)(Texture->height - 2)));
				s32 x = (s32)tx;
				s32 y = (s32)ty;

				// t values for bilinear filtering
				f32 fx = tx - (f32)x;
				f32 fy = ty - (f32)y;

				ASSERT((x >= 0) && (x < Texture->width));
				ASSERT((y >= 0) && (y < Texture->height));


				bilinear_sample TexelSample = bilinear_sample_texture(Texture, x, y);
				v4f Texel = srgb_bilinear_blend(TexelSample, fx, fy); 

				if(NormalMap)
				{
					bilinear_sample NormalSample = bilinear_sample_texture(NormalMap, x, y);

					v4f NormalA = unpack4x8(NormalSample.a);
					v4f NormalB = unpack4x8(NormalSample.b);
					v4f NormalC = unpack4x8(NormalSample.c);
					v4f NormalD = unpack4x8(NormalSample.d);

					v4f Normal = lerp(lerp(NormalA, fx, NormalB), fy, lerp(NormalC, fx, NormalD));

					Normal = normal_rescale_and_bias(Normal);

					environment_map *FarMap = 0;
					//f32 t_env_map = Normal.z;
					f32 t_env_map = Normal.y;
					f32 t_far_map = 0.0f;
					if(t_env_map < -0.5f)
					{
						FarMap = Bottom;
						t_far_map = 2.0f * (t_env_map + 1.0f);
					}
					else if(t_env_map > 0.5f)
					{
						FarMap = Top;
						t_far_map = 2.0f * (t_env_map - 0.5f);
					}

					v2f ScreenSpaceUV = {inv_width_max * (f32)x, inv_height_max * (f32)y};
					v3f LightColor = {0,0,0};//environment_map_sample(ScreenSpaceUV, Middle, Normal.xyz, Normal.w);
					if(FarMap)
					{
						v3f FarMapColor = environment_map_sample(ScreenSpaceUV, FarMap, Normal.xyz, Normal.w);
						LightColor = lerp(LightColor, t_far_map, FarMapColor);
					}
					Texel.rgb = Texel.rgb + Texel.a * LightColor;
				}

				// NOTE(Justin): Texture tinting
				Texel = v4f_hadamard(Texel, Color);
				Texel.r = clamp01(Texel.r);
				Texel.g = clamp01(Texel.g);
				Texel.b = clamp01(Texel.b);

				v4f Dest = unpack4x8(*pixel);
				Dest = srgb_255_to_linear01(Dest);

				v4f Blended255 = (1.0f - Texel.a) * Dest + Texel;

				Blended255 = linear01_to_srgb_255(Blended255);


				*pixel = (((u32)(Blended255.a + 0.5f) << 24) |
						  ((u32)(Blended255.r + 0.5f) << 16) |
						  ((u32)(Blended255.g + 0.5f) << 8) |
						  ((u32)(Blended255.b + 0.5f) << 0));
			}
			pixel++;
		}
		pixel_row += Buffer->stride;
	}
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
	u8 *start = (u8 *)BackBuffer->memory + BackBuffer->stride * screen_y + BITMAP_BYTES_PER_PIXEL * screen_x;
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

internal void 
rectangle_draw(loaded_bitmap *Buffer, v2f Min, v2f Max, f32 r, f32 g, f32 b)
{
	s32 x_min = f32_round_to_s32(Min.x);
	s32 y_min = f32_round_to_s32(Min.y);
	s32 x_max = f32_round_to_s32(Max.x);
	s32 y_max = f32_round_to_s32(Max.y);

	if(x_min < 0)
	{
		x_min += Buffer->width;
	}
	if(x_max > Buffer->width)
	{
		x_max -= Buffer->width;
	}
	if(y_min < 0)
	{
		y_min += Buffer->height;
	}
	if(y_max > Buffer->height)
	{
		y_max -= Buffer->height;
	}

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)Buffer->memory + Buffer->stride * y_min + BITMAP_BYTES_PER_PIXEL * x_min ;
	for(int row = y_min; row < y_max; row++)
	{
		u32 *pixel = (u32 *)pixel_row;
		for(int col = x_min; col < x_max; col++)
		{
			*pixel++ = color;
		}
		pixel_row += Buffer->stride;
	}
}

internal void 
rectangle_draw(loaded_bitmap *Buffer, v2f Min, v2f Max, v4f Color)
{
	rectangle_draw(Buffer, Min, Max, Color.r, Color.g, Color.b);
}

internal void
bitmap_draw(loaded_bitmap *Buffer, loaded_bitmap *Bitmap, f32 x, f32 y, f32 c_alpha = 1.0f)
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
		region_1_x_min = x_min + Buffer->width;
		region_1_x_max = Buffer->width;
		region_2_x_min = 0;
		region_2_x_max = x_max;
		bitmap_across_x_boundary = true;
	}

	if(y_min < 0)
	{
		region_1_y_min = y_min + Buffer->height;;
		region_1_y_max = Buffer->height;
		region_2_y_min = 0;
		region_2_y_max = y_max;
		bitmap_across_y_boundary = true;
	}

	if(x_max > Buffer->width)
	{
		region_1_x_min = x_min;
		region_1_x_max = Buffer->width;
		region_2_x_min = 0;
		region_2_x_max = x_max - Buffer->width; 
		bitmap_across_x_boundary = true;
	}

	if(y_max > Buffer->height)
	{
		region_1_y_min = y_min;
		region_1_y_max = Buffer->height;
		region_2_y_min = 0;
		region_2_y_max = y_max - Buffer->height;
		bitmap_across_y_boundary = true;
	}

	if(!(bitmap_across_x_boundary || bitmap_across_y_boundary))
	{
		u8 *dest_row = (u8 *)Buffer->memory + y_min * Buffer->stride + x_min * BITMAP_BYTES_PER_PIXEL;
		u8 *src_row = (u8 *)Bitmap->memory;
		for(s32 y = y_min; y < y_max; y++)
		{
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for(s32 x = x_min; x < x_max; x++)
			{
				v4f Texel = unpack4x8(*src);

				Texel = srgb_255_to_linear01(Texel);

				Texel *= c_alpha;

				v4f Dest = unpack4x8(*dest);

				Dest = srgb_255_to_linear01(Dest);

				v4f Result = (1.0f - Texel.a) * Dest + Texel;

				Result = linear01_to_srgb_255(Result);

				*dest = (((u32)(Result.a + 0.5f) << 24) |
						 ((u32)(Result.r + 0.5f) << 16) |
						 ((u32)(Result.g + 0.5f) << 8) |
						 ((u32)(Result.b + 0.5f) << 0));

				dest++;
				src++;
			}
			dest_row += Buffer->stride;
			src_row += Bitmap->stride;
		}
	}
	else
	{
		if(!bitmap_across_y_boundary)
		{
			region_1_y_min = y_min;
			region_1_y_max = y_max;
			region_2_y_min = y_min;
			region_2_y_max = y_max;
		}

		if(!bitmap_across_x_boundary)
		{
			region_1_x_min = x_min;
			region_1_x_max = x_max;
			region_2_x_min = x_min;
			region_2_x_max = x_max;
		}

		u8 *dest_row = (u8 *)Buffer->memory + region_1_y_min * Buffer->stride + region_1_x_min * BITMAP_BYTES_PER_PIXEL;
		u8 *src_row = (u8 *)Bitmap->memory;
		for(s32 y = region_1_y_min; y < region_1_y_max; y++)
		{
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for(s32 x = region_1_x_min; x < region_1_x_max; x++)
			{
				v4f Texel = unpack4x8(*src);

				Texel = srgb_255_to_linear01(Texel);

				Texel *= c_alpha;

				v4f Dest = unpack4x8(*dest);

				Dest = srgb_255_to_linear01(Dest);

				v4f Result = (1.0f - Texel.a) * Dest + Texel;

				Result = linear01_to_srgb_255(Result);

				*dest = (((u32)(Result.a + 0.5f) << 24) |
						 ((u32)(Result.r + 0.5f) << 16) |
						 ((u32)(Result.g + 0.5f) << 8) |
						 ((u32)(Result.b + 0.5f) << 0));

				dest++;
				src++;
			}
			dest_row += Buffer->stride;
			src_row += Bitmap->stride;
		}

		s32 bitmap_offset_x = 0; 
		if(x_min < 0)
		{
			bitmap_offset_x = -1 * x_min;
		}
		if(x_max > Buffer->width)
		{
			bitmap_offset_x = Buffer->width - x_min;
		}

		s32 bitmap_offset_y = 0;
		if(y_min < 0)
		{
			bitmap_offset_y = -1 * y_min;
		}

		if(y_max > Buffer->height)
		{
			bitmap_offset_y = Buffer->height - y_min;
		}

		dest_row = (u8 *)Buffer->memory + region_2_y_min * Buffer->stride + region_2_x_min * BITMAP_BYTES_PER_PIXEL;
		src_row = (u8 *)Bitmap->memory + bitmap_offset_y * Bitmap->stride + bitmap_offset_x * BITMAP_BYTES_PER_PIXEL;
		for(s32 y = region_2_y_min; y < region_2_y_max; y++)
		{
			u32 *src = (u32 *)src_row;
			u32 *dest = (u32 *)dest_row;
			for(s32 x = region_2_x_min; x < region_2_x_max; x++)
			{

				v4f Texel = unpack4x8(*src);

				Texel = srgb_255_to_linear01(Texel);

				Texel *= c_alpha;

				v4f Dest = unpack4x8(*dest);

				Dest = srgb_255_to_linear01(Dest);

				v4f Result = (1.0f - Texel.a) * Dest + Texel;

				Result = linear01_to_srgb_255(Result);

				*dest = (((u32)(Result.a + 0.5f) << 24) |
						 ((u32)(Result.r + 0.5f) << 16) |
						 ((u32)(Result.g + 0.5f) << 8) |
						 ((u32)(Result.b + 0.5f) << 0));

				dest++;
				src++;
			}
			dest_row += Buffer->stride;
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

internal void
circle_draw(back_buffer *BackBuffer, circle *Circle, f32 r, f32 b, f32 g)
{
	bounding_box CircleBoudingBox = circle_bounding_box(*Circle);

	s32 x_min = f32_round_to_s32(CircleBoudingBox.Min.x);
	s32 y_min = f32_round_to_s32(CircleBoudingBox.Min.y);
	s32 x_max = f32_round_to_s32(CircleBoudingBox.Max.x);
	s32 y_max = f32_round_to_s32(CircleBoudingBox.Max.y);

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));


	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BITMAP_BYTES_PER_PIXEL * x_min;

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

internal void
line_vertical_draw(back_buffer *BackBuffer, f32 x, f32 r, f32 g, f32 b)
{
	s32 x_col = f32_round_to_s32(x);

	u32 red = f32_round_to_u32(255.0f * r);
	u32 green = f32_round_to_u32(255.0f * g);
	u32 blue = f32_round_to_u32(255.0f * b);
	u32 color = ((red << 16) | (green << 8) | (blue << 0));

	u8 *pixel_row = (u8 *)BackBuffer->memory + x_col * BITMAP_BYTES_PER_PIXEL;
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

#define push_render_element(RenderGroup, type) (type *)push_render_element_(RenderGroup, sizeof(type), RENDER_GROUP_ENTRY_TYPE_##type)
inline void *
push_render_element_(render_group *RenderGroup, u32 size, render_group_entry_type Type)
{
	void *Result = 0;

	size += sizeof(render_group_entry_header);

	if((RenderGroup->push_buffer_size + size) < RenderGroup->push_buffer_size_max)
	{
		render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->push_buffer_base + RenderGroup->push_buffer_size);
		Header->Type = Type;
		//Result = (Header + 1);
		Result = (u8 *)Header + sizeof(*Header);
		RenderGroup->push_buffer_size += size;
	}
	else
	{
		INVALID_CODE_PATH;
	}

	return(Result);
}

inline void
push_piece(render_group *RenderGroup, loaded_bitmap *Texture, loaded_bitmap *NormalMap,
		v2f Origin, v2f XAxis, v2f YAxis, v2f Align, v2f Dim, v4f Color)
{
	render_entry_bitmap *Entry = push_render_element(RenderGroup, render_entry_bitmap);
	if(Entry)
	{
		m3x3 M = RenderGroup->MapToScreenSpace;

		Entry->Texture = Texture;
		Entry->NormalMap = NormalMap;
		Entry->Origin = M * Origin - Align;
		Entry->XAxis = XAxis;
		Entry->YAxis = YAxis;
		Entry->Color = Color;
		Entry->Dim = Dim;
	}
}

inline void
push_bitmap(render_group *RenderGroup, loaded_bitmap *Bitmap, loaded_bitmap *NormalMap,
		v2f Origin, v2f XAxis, v2f YAxis, v2f Align, f32 alpha = 1.0f)
{
	push_piece(RenderGroup, Bitmap, NormalMap, Origin, XAxis, YAxis,
			Align, V2F(0, 0), V4F(1.0f, 1.0f, 1.0f, alpha));
}

inline void
push_rectangle(render_group *RenderGroup, v2f Origin, v2f XAxis, v2f YAxis, v2f Dim, v4f Color)
{
	render_entry_rectangle *Entry = push_render_element(RenderGroup, render_entry_rectangle);
	if(Entry)
	{
		m3x3 M = RenderGroup->MapToScreenSpace;
		f32 pixels_per_meter = RenderGroup->pixels_per_meter;

		v2f HalfDim = 0.5f * RenderGroup->pixels_per_meter * Dim;

		Entry->Origin = M * Origin - HalfDim;
		Entry->XAxis = pixels_per_meter * XAxis;
		Entry->YAxis = pixels_per_meter * YAxis;
		Entry->Color = Color;
		Entry->Dim = pixels_per_meter * Dim;
	}
}

inline void
clear(render_group *RenderGroup, v4f Color)
{
	render_entry_clear *Entry = push_render_element(RenderGroup, render_entry_clear);
	if(Entry)
	{
		Entry->Color = Color;
	}
}

/*

NOTE(Justin): In what units should the axes be? Pixels or meters?

*/

inline render_entry_coordinate_system * 
coordinate_system(render_group *RenderGroup, v2f Origin, v2f XAxis, v2f YAxis, v4f Color,
				 loaded_bitmap *Texture,
				 loaded_bitmap *NormalMap,
				 environment_map *Top,
				 environment_map *Middle,
				 environment_map *Bottom)
{
	render_entry_coordinate_system *Entry = push_render_element(RenderGroup, render_entry_coordinate_system);
	if(Entry)
	{
		m3x3 M = RenderGroup->MapToScreenSpace;
		f32 pixels_per_meter = RenderGroup->pixels_per_meter;


		// NOTE(Justin): This requires the coordinate system to be defined
		// completely in screen space. Which will need to be changed
		Entry->Origin = Origin;
		Entry->XAxis = XAxis;
		Entry->YAxis = YAxis;
#if 0
		Entry->Origin = M * Origin - Texture->Align;

		Entry->XAxis = pixels_per_meter * XAxis;
		Entry->YAxis = pixels_per_meter * YAxis;
#endif
		Entry->Color = Color;

		Entry->Texture = Texture;
		Entry->NormalMap = NormalMap;

		Entry->Top = Top;
		Entry->Middle = Middle;
		Entry->Bottom = Bottom;
	}

	return(Entry);
}

internal void
render_group_to_output(render_group *RenderGroup, loaded_bitmap *OutputTarget)
{
	for(u32 base_address = 0; base_address < RenderGroup->push_buffer_size; )
	{
		render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->push_buffer_base + base_address);
		base_address += sizeof(*Header);

		void *render_data = (u8 *)Header + sizeof(*Header);
		switch(Header->Type)
		{
			case RENDER_GROUP_ENTRY_TYPE_render_entry_clear:
			{
				render_entry_clear *Entry = (render_entry_clear *)render_data;
				rectangle_draw(OutputTarget, V2F(0.0f, 0.0f), V2F((f32)OutputTarget->width, (f32)OutputTarget->height), Entry->Color);
				base_address += sizeof(*Entry);
			} break;
			case RENDER_GROUP_ENTRY_TYPE_render_entry_bitmap:
			{
				render_entry_bitmap *Entry = (render_entry_bitmap *)render_data;
				ASSERT(Entry->Texture);  
				bitmap_draw(OutputTarget, Entry->Texture, Entry->Origin.x, Entry->Origin.y, Entry->Color.a);
				base_address += sizeof(*Entry);
			} break;
			case RENDER_GROUP_ENTRY_TYPE_render_entry_rectangle:
			{
				render_entry_rectangle *Entry = (render_entry_rectangle *)render_data;

				rectangle_draw(OutputTarget, Entry->Origin, Entry->Origin + Entry->Dim,
						Entry->Color);
				base_address += sizeof(*Entry);

			} break;
			case RENDER_GROUP_ENTRY_TYPE_render_entry_coordinate_system:
			{
				render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)render_data;

				rectangle_draw_slowly(OutputTarget, Entry->Origin, Entry->XAxis, Entry->YAxis, Entry->Color,
						Entry->Texture, Entry->NormalMap, Entry->Top, Entry->Middle, Entry->Bottom);

				v4f Color = {1, 1, 0, 1};
				v2f Dim = {2, 2};

				v2f P = Entry->Origin;
				rectangle_draw(OutputTarget, P - Dim, P + Dim, Color);

				P = Entry->Origin + Entry->XAxis;
				rectangle_draw(OutputTarget, P - Dim, P + Dim, Color);

				P = Entry->Origin + Entry->YAxis;
				rectangle_draw(OutputTarget, P - Dim, P + Dim, Color);

				P = Entry->Origin + Entry->XAxis + Entry->YAxis;
				rectangle_draw(OutputTarget, P - Dim, P + Dim, Color);
				

				base_address += sizeof(*Entry);

			} break;

			INVALID_DEFAULT_CASE;
		}
	}
}

internal render_group *
render_group_allocate(memory_arena *Arena, u32 push_buffer_size_max, f32 pixels_per_meter, m3x3 MapToScreenSpace)
{
	render_group *Result = push_struct(Arena, render_group);

	Result->pixels_per_meter = pixels_per_meter;
	Result->MapToScreenSpace = MapToScreenSpace;

	Result->push_buffer_size = 0;
	Result->push_buffer_size_max = push_buffer_size_max;
	Result->push_buffer_base = (u8 *)push_size(Arena, push_buffer_size_max);

	return(Result);
}
