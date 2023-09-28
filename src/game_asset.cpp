
#pragma pack(push, 1)
typedef struct
{
	u16 file_type;
	u32 file_size;
	u16 reserved_1;
	u16 reserved_2;
	u32 bitmap_offset;
	u32 size;
	s32 width;
	s32 height;
	u16 planes;
	u16 bits_per_pixel; 
	u32 compression;
	u32 bitmap_size;
	s32 horz_resolution;
	s32 vert_resolution;  
	u32 colors_used;
	u32 colors_important;
} bitmap_header;

typedef struct
{
	u32 chunk_id;
	u32 chunk_size;
	u32 wave_id;
} wave_header;

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

enum
{
	 WAVE_CHUNK_ID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
	 WAVE_CHUNK_ID_FORMAT = RIFF_CODE('f', 'm', 't', ' '),
	 WAVE_CHUNK_ID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
	 WAVE_CHUNK_ID_DATA = RIFF_CODE('d', 'a', 't', 'a')
};

typedef struct
{
	u32 id;
	u32 size;
} wave_chunk;

typedef struct
{
	u16 format;
	u16 channel_count;
	u32 samples_per_second;
	u32 avg_bytes_per_second;
	u16 block_align;
	u16 bits_per_sample;
	u16 extension_size;
	u16 valid_bits_per_sample;
	u32 channel_mask;
	u8 sub_format[16];

} wave_format_chunk;
#pragma pack(pop)

internal loaded_bitmap
bitmap_file_read_entire(char *filename)
{
	loaded_bitmap Result = {0};

	debug_file_read  BitmapFile = platform_file_read_entire(filename);
	if (BitmapFile.contents) {
		bitmap_header *BitmapHeader = (bitmap_header *)BitmapFile.contents;
		if (BitmapHeader->size != 0) {
			Result.memory = (void *)((u8 *)BitmapFile.contents + BitmapHeader->bitmap_offset);
			Result.width = BitmapHeader->width;
			Result.height = BitmapHeader->height;
			Result.bytes_per_pixel = BitmapHeader->bits_per_pixel / 8;
			Result.stride = BitmapHeader->width * Result.bytes_per_pixel;
		}
	}
	return(Result);
}

typedef struct
{
	u8 *at;
	u8 *stop;

} riff_iterator;

inline riff_iterator
parse_chunk_at(void *at, void *stop)
{
	riff_iterator Iter;
	Iter.at = (u8 *)at;
	Iter.stop = (u8 *)stop;
	return(Iter);
}

inline riff_iterator
next_chunk(riff_iterator Iter)
{
	// Note(Justin): Chunk size does not include the size of the header.
	wave_chunk *WaveChunk = (wave_chunk *)Iter.at;
	u32 size = (WaveChunk->size + 1) & ~1;
	Iter.at += sizeof(wave_chunk) + size;
	return(Iter);
}

inline b32
iterator_is_valid(riff_iterator Iter)
{
	b32 Result = (Iter.at < Iter.stop);
	return(Result);
}

inline void *
get_chunk_data(riff_iterator Iter)
{
	void *Result = (Iter.at + sizeof(wave_chunk));
	return(Result);
}

inline u32
get_type(riff_iterator Iter)
{
	wave_chunk  *WaveChunk = (wave_chunk *)Iter.at;
	u32 Result = WaveChunk->id;
	return(Result);
}

inline u32
get_chunk_data_size(riff_iterator Iter)
{
	wave_chunk *WaveChunk = (wave_chunk *)Iter.at;
	u32 Result = WaveChunk->size;
	return(Result);
}

internal loaded_sound
wav_file_read_entire(char *filename)
{
	loaded_sound Result = {0};
	debug_file_read WavFile = platform_file_read_entire(filename);
	if (WavFile.size != 0) {
		wave_header *WaveHeader = (wave_header *)WavFile.contents;

		ASSERT(WaveHeader->chunk_id == WAVE_CHUNK_ID_RIFF);
		ASSERT(WaveHeader->wave_id == WAVE_CHUNK_ID_WAVE);

		u32 channel_count = 0;
		u32 samples_data_size = 0;
		s16 *samples = 0;
		for (riff_iterator Iter = parse_chunk_at(WaveHeader + 1, (u8 *)(WaveHeader + 1) + WaveHeader->chunk_size - 4);
				iterator_is_valid(Iter);
				Iter = next_chunk(Iter)) {
			switch (get_type(Iter)) {
				case WAVE_CHUNK_ID_FORMAT:
				{
					wave_format_chunk *WaveFormat = (wave_format_chunk *)get_chunk_data(Iter);
					ASSERT(WaveFormat->format == 1);
					ASSERT(WaveFormat->samples_per_second == 48000);
					ASSERT(WaveFormat->bits_per_sample == 16);
					ASSERT(WaveFormat->block_align == (sizeof(s16) * WaveFormat->channel_count));
					channel_count = WaveFormat->channel_count;
				} break;
				case WAVE_CHUNK_ID_DATA:
				{
					samples = (s16 *)get_chunk_data(Iter);
					samples_data_size = get_chunk_data_size(Iter);
				} break;
			}
		}
		ASSERT(channel_count && samples);

		Result.channel_count = channel_count;
		Result.sample_count = samples_data_size / (sizeof(s16) * channel_count);
		if (channel_count == 1) {
			Result.samples[0] = samples;
			Result.samples[1] = 0;
		} else if(channel_count == 2) {
			Result.samples[0] = samples;
			Result.samples[1] = samples + Result.sample_count;

			for (u32 sample_index = 0; sample_index < Result.sample_count; sample_index++) {
				s16 source = samples[2 * sample_index];
				samples[2 * sample_index] = samples[sample_index];
				samples[sample_index] = source;
			}

		} else {
			ASSERT(!"Invalid channel count in WAV file");
		}
		//TODO(Justin): Left and right channels.
		Result.channel_count = 1;
	}
	return(Result);
}

