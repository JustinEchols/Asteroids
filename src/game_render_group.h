#if !defined(GAME_RENDER_GROUP_H)

struct entity_visible_piece
{
	loaded_bitmap *Bitmap;
	v2f Pos;
	f32 r, g, b, alpha;
	v2f Dim;
};

struct entity_visible_piece_group
{
	u32 piece_count;
	entity_visible_piece Pieces[256];
};


#define GAME_RENDER_GROUP_H
#endif
