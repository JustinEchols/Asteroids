/* NOTE(Justin):

	1) All inputs to the renderer are in "meters". Unless stated otherwise.

	2) All color values sent to the renderer as v4f's are in NON - premultiplied alpha.
	   The user is only expected to specify an alpha value.

	   E.g. (1,1,1,.5) => (.5,.5,.5,.5)
	3) 

*/
#if !defined(GAME_RENDER_GROUP_H)

typedef struct
{
	void *memory;
	s32 width;
	s32 height;
	u32 stride;

	v2f Align;

} loaded_bitmap;

struct environment_map
{
	loaded_bitmap LOD[4];
	f32 zp;
};

struct render_basis
{
	v3f P;
};

struct render_entity_basis
{
	render_basis *Basis;
	v3f Offset;
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
	render_entity_basis EntityBasis;
	v4f Color;
	loaded_bitmap *Bitmap;
};

struct render_entry_rectangle
{
	render_entity_basis EntityBasis;
	v4f Color;
	v2f Dim;
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



struct render_group 
{
	f32 pixels_per_meter;
	m3x3 MapToScreenSpace;
	render_basis *DefaultBasis;

	u32 push_buffer_size;
	u32 push_buffer_size_max;
	u8 *push_buffer_base;
};

// NOTE(Justin): Renderer API

#if 0
inline void * push_render_element_(render_group *RenderGroup, u32 size, render_group_entry_type Type)

inline void push_piece(render_group *RenderGroup, loaded_bitmap *Texture, loaded_bitmap *NormalMap,
		v2f Origin, v2f XAxis, v2f YAxis, v2f Align, v2f Dim, v4f Color)

inline void push_bitmap(render_group *RenderGroup, loaded_bitmap *Bitmap, loaded_bitmap *NormalMap,
		v2f Origin, v2f XAxis, v2f YAxis, v2f Align, f32 alpha = 1.0f)

inline void push_rectangle(render_group *RenderGroup, v2f Origin, v2f XAxis, v2f YAxis, v2f Dim, v4f Color)

inline void clear(render_group *RenderGroup, v4f Color)
#endif

#define GAME_RENDER_GROUP_H
#endif


