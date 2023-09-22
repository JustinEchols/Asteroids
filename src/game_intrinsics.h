#if !defined(GAME_INTRINSICS_H)

internal s16
f32_round_to_s16(f32 x)
{
	s16 Result = (s16)floorf(x);
	return(Result);
}

internal s32
f32_round_to_s32(f32 x)
{
	s32 result = (s32)floorf(x);
	return(result);
}

internal u32
f32_round_to_u32(f32 x)
{
	u32 result = (u32)(x + 0.5f);
	return(result);
}

internal s32
f32_truncate_to_s32(f32 x)
{
	s32 Result = (s32)x;
	return(Result);
}

internal f32
f32_fractional_part(f32 x)
{
	f32 Result = 0.0f;
	if (x > 0) {
		Result = x - f32_round_to_s32(x);
	} else {
		Result = x - f32_round_to_s32(x + 1.0f);
	}
	return(Result);
}


#define GAME_INTRINSICS_H
#endif
