#if !defined(GAME_TILE_H)

enum
{
	TILE_EMPTY,
	TILE_OCCUPIED
};

struct tile_map_pos_delta
{
	v2f dOffset;
};

struct tile_map_position
{
	v2i Tile;
	v2f TileOffset;
};

struct tile_map
{
	s32 tile_count_x;
	s32 tile_count_y;
	f32 tile_side_in_meters;

	u32* tiles;
};

#define GAME_TILE_H
#endif
