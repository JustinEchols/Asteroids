#if !defined(WIN32_GAME_H)

typedef struct
{
	int samples_per_second;
	int bytes_per_sample;
	DWORD secondary_buffer_size;
	int sample_count;
	int wave_frequency;
	int wave_amplitude;
	int samples_per_period;
	int sample_count_latency;

} win32_sound_buffer;

typedef struct
{
	f32 r, g, b;

} win32_color;

typedef struct
{
	int width;
	int height;

} win32_client_dimensions;

typedef struct
{
	BITMAPINFO Info;
	int width;
	int height;
	int bytes_per_pixel;
	int stride;
	void *memory;

} win32_back_buffer;


#define WIN32_GAME_H
#endif
