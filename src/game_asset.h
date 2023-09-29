#if !defined(GAME_ASSET_H)

typedef struct
{
	u32 sample_count;
	u32 channel_count;
	s16 *samples[2];
} loaded_sound;

typedef struct
{
	void *memory;
	s32 width;
	s32 height;
	u32 bytes_per_pixel;
	u32 stride;

} loaded_bitmap;




#define GAME_ASSET_H
#endif
