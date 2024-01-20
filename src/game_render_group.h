#if !defined(GAME_RENDER_GROUP_H)

struct environment_map
{
	loaded_bitmap *LOD[4];
};

enum render_entry_type
{
	RENDER_ENTRY_BITMAP,
	RENDER_ENTRY_RECT,
	RENDER_ENTRY_LINE_SEG,
};

struct render_group_entry 
{
	render_entry_type TYPE;

	v2f Origin;
	v2f XAxis;
	v2f YAxis;
	v4f Color;
	v2f Dim;
	loaded_bitmap *Texture;
	loaded_bitmap *NormalMap;
};

struct render_group 
{
	f32 pixels_per_meter;
	m3x3 MapToScreenSpace;

	u32 push_buffer_size;
	u32 push_buffer_size_max;
	u8 *push_buffer_base;
};

#define GAME_RENDER_GROUP_H
#endif


