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
	RENDER_GROUP_ENTRY_TYPE_render_entry_coordinate_system,
};

struct render_group_entry_header
{
	render_group_entry_type Type;
};

struct render_entry_clear
{
	v4f Color;
};

struct render_entry_bitmap
{
	v2f Origin;
	v2f XAxis;
	v2f YAxis;
	v4f Color;
	v2f Dim;

	loaded_bitmap *Texture;
	loaded_bitmap *NormalMap;
};

struct render_entry_coordinate_system
{
	v2f Origin;
	v2f XAxis;
	v2f YAxis;
	v4f Color;

	loaded_bitmap *Texture;
	loaded_bitmap *NormalMap;

	environment_map *Top;
	environment_map *Middle;
	environment_map *Bottom;
};

struct render_entry_rectangle
{
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


