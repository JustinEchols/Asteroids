
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
rectangle_draw(back_buffer *BackBuffer, v2f Min, v2f Max, v4f Color)
{
	rectangle_draw(BackBuffer, Min, Max, Color.r, Color.g, Color.b);
}

internal void 
rectangle_draw_slowly(back_buffer *BackBuffer, loaded_bitmap *Bitmap, v2f Dim, v2f Origin, v2f XAxis, v2f YAxis,
		v4f Color)
{
	f32 inv_x_axis_len_sq = 1.0f / v2f_dot(XAxis, XAxis);
	f32 inv_y_axis_len_sq = 1.0f / v2f_dot(YAxis, YAxis);

	v2f P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};

	s32 x_min = BackBuffer->width;
	s32 y_min = BackBuffer->height;
	s32 x_max = 0;
	s32 y_max = 0;

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

	u8 *pixel_row = (u8 *)BackBuffer->memory + BackBuffer->stride * y_min + BackBuffer->bytes_per_pixel * x_min;
	for(int row = y_min; row < y_max; row++)
	{
		u32 *pixel = (u32 *)pixel_row;
		for(int col = x_min; col < x_max; col++)
		{
			v2f PixelP = V2F((f32)col, (f32)row);
			v2f D = PixelP - Origin;
			f32 e0 = v2f_dot(D, -1.0f * YAxis);
			f32 e1 = v2f_dot(D - XAxis, XAxis);
			f32 e2 = v2f_dot(D - XAxis - YAxis, YAxis);
			f32 e3 = v2f_dot(D - YAxis, -1.0f * XAxis);

			if((e0 < 0) && (e1 < 0) && (e2 < 0) && (e3 < 0))
			{
				f32 u = inv_x_axis_len_sq * v2f_dot(D, XAxis);
				f32 v = inv_y_axis_len_sq * v2f_dot(D, YAxis);

				// TODO(Justin): Clamp.
				ASSERT((u >= 0.0f) && (u <= 1.0f));
				ASSERT((v >= 0.0f) && (v <= 1.0f));

				f32 tx = 1.0f + ((u * (f32)(Bitmap->width - 3)));
				f32 ty = 1.0f + ((v * (f32)(Bitmap->height - 3)));

				s32 x = (s32)tx;
				s32 y = (s32)ty;

				f32 fx = tx - (f32)x;
				f32 fy = ty - (f32)y;

				ASSERT((x >= 0) && (x <= Bitmap->width));
				ASSERT((y >= 0) && (y <= Bitmap->height));

				u8 *texel_ptr = (u8 *)Bitmap->memory + y * Bitmap->stride + x * sizeof(u32);
				u32 texela = *(u32 *)texel_ptr;
				u32 texelb = *(u32 *)(texel_ptr + sizeof(u32));
				u32 texelc = *(u32 *)(texel_ptr + Bitmap->stride);
				u32 texeld = *(u32 *)(texel_ptr + Bitmap->stride + sizeof(u32));

				v4f TexelA = {(f32)((texela >> 16) & 0xFF),
							 (f32)((texela >> 8) & 0xFF),
							 (f32)((texela >> 0) & 0xFF),
							 (f32)((texela >> 24) & 0xFF)};

				v4f TexelB = {(f32)((texelb >> 16) & 0xFF),
							 (f32)((texelb >> 8) & 0xFF),
							 (f32)((texelb >> 0) & 0xFF),
							 (f32)((texelb >> 24) & 0xFF)};

				v4f TexelC = {(f32)((texelc >> 16) & 0xFF),
							 (f32)((texelc >> 8) & 0xFF),
							 (f32)((texelc >> 0) & 0xFF),
							 (f32)((texelc >> 24) & 0xFF)};

				v4f TexelD = {(f32)((texeld >> 16) & 0xFF),
							 (f32)((texeld >> 8) & 0xFF),
							 (f32)((texeld >> 0) & 0xFF),
							 (f32)((texeld >> 24) & 0xFF)};

				v4f Texel = lerp(lerp(TexelA, fx, TexelB), fy, lerp(TexelC, fx, TexelD));

				f32 src_a = Texel.a;
				f32 src_r = Texel.r;
				f32 src_g = Texel.g;
				f32 src_b = Texel.b;

				f32 src_ra = (src_a / 255.0f) * Color.a;

				f32 dest_a = (f32)((*pixel >> 24) & 0xFF);
				f32 dest_r = (f32)((*pixel >> 16) & 0xFF);
				f32 dest_g = (f32)((*pixel >> 8) & 0xFF);
				f32 dest_b = (f32)((*pixel >> 0) & 0xFF);
				f32 dest_ra = dest_a / 255.0f;

				f32 a = 255.0f * (src_ra + dest_ra - src_ra * dest_ra);
				f32 r = (1.0f - src_ra) * dest_r + src_r;
				f32 g = (1.0f - src_ra) * dest_g + src_g; 
				f32 b = (1.0f - src_ra) * dest_b + src_b;

				*pixel = (((u32)(a + 0.5f) << 24) |
						   ((u32)(r + 0.5f) << 16) |
						   ((u32)(g + 0.5f) << 8) |
						   ((u32)(b + 0.5f) << 0));

			}
			pixel++;
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
				f32 sa = (f32)((*src >> 24) & 0xFF);
				f32 sr = (f32)((*src >> 16) & 0xFF);
				f32 sg = (f32)((*src >> 8) & 0xFF);
				f32 sb = (f32)((*src >> 0) & 0xFF);
				f32 rsa = (sa / 255.0f) * c_alpha;

				f32 da = (f32)((*dest >> 24) & 0xFF);
				f32 dr = (f32)((*dest >> 16) & 0xFF);
				f32 dg = (f32)((*dest >> 8) & 0xFF);
				f32 db = (f32)((*dest >> 0) & 0xFF);
				f32 rda = (da / 255.0f);

				f32 inv_rsa = (1.0f - rsa);

				f32 a = 255.0f * (rsa + rda - rsa * rda);
				f32 r = inv_rsa * dr + sr;
				f32 g = inv_rsa * dg + sg; 
				f32 b = inv_rsa * db + sb;

				*dest = (((u32)(a + 0.5f) << 24) |
						 ((u32)(r + 0.5f) << 16) |
						 ((u32)(g + 0.5f) << 8) |
						 ((u32)(b + 0.5f) << 0));

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

inline void
push_piece(render_group *RenderGroup, loaded_bitmap *Texture,
		v2f Origin, v2f XAxis, v2f YAxis, v2f Align, v2f Dim, v4f Color)
{

	ASSERT(RenderGroup->element_count < ARRAY_COUNT(RenderGroup->Elements));
	render_entry *Element = RenderGroup->Elements + RenderGroup->element_count++;
	m3x3 M = RenderGroup->MapToScreenSpace;

	Element->Texture = Texture;
	Element->Origin = M * Origin - Align;
	Element->XAxis = XAxis;
	Element->YAxis = YAxis;
	Element->Color = Color;
	Element->Dim = Dim;
}

inline void
push_bitmap(render_group *RenderGroup, loaded_bitmap *Bitmap,
		v2f Origin, v2f XAxis, v2f YAxis, v2f Align, f32 alpha = 1.0f)
{
	push_piece(RenderGroup, Bitmap, Origin, XAxis, YAxis,
			Align, V2F(0, 0), V4F(1.0f, 1.0f, 1.0f, alpha));
}

inline void
push_rectangle(render_group *RenderGroup, v2f Origin, v2f XAxis, v2f YAxis, v2f Dim, v4f Color)
{
	push_piece(RenderGroup, 0, Origin, XAxis, YAxis, V2F(0, 0), Dim, Color);
}

