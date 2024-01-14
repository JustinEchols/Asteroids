#if !defined(GAME_RENDER_GROUP_H)

struct render_entry 
{
	v2f Origin;
	v2f XAxis;
	v2f YAxis;
	v4f Color;
	v2f Dim;
	loaded_bitmap *Texture;
};

struct render_group 
{
	f32 pixels_per_meter;
	m3x3 MapToScreenSpace;

	render_entry Elements[256];
	u32 element_count;
};

#define GAME_RENDER_GROUP_H
#endif


