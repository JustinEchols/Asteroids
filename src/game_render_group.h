#if !defined(GAME_RENDER_GROUP_H)

struct environment_map
{
	loaded_bitmap *LOD[4];
};

enum render_group_entry_type
{
	RENDER_GROUP_ENTRY_TYPE_render_entry_clear,
	RENDER_GROUP_ENTRY_TYPE_render_entry_bitmap,
	RENDER_GROUP_ENTRY_TYPE_render_entry_rectangle,
};

struct render_group_entry_header
{
	render_group_entry_type Type;
};

struct render_entry_clear
{
	render_group_entry_header Header;
	v4f Color;
};

struct render_entry_bitmap
{
	render_group_entry_header Header;

	v2f Origin;
	v2f XAxis;
	v2f YAxis;
	v4f Color;
	v2f Dim;
	loaded_bitmap *Texture;
	loaded_bitmap *NormalMap;

};

struct render_entry_rectangle
{
	render_group_entry_header Header;

	v2f Origin;
	v2f XAxis;
	v2f YAxis;
	v4f Color;
	v2f Dim;
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


