
inline u32
tile_map_get_tile_value_unchecked(tile_map *TileMap, tile_map_position TileMapPos)
{
	u32 TileValue = TileMap->tiles[TileMapPos.Tile.y * TileMap->tile_count_x + TileMapPos.Tile.x];
	return(TileValue);

}

internal b32
tile_map_tile_is_empty(tile_map *TileMap, tile_map_position TestPos)
{
	b32 Result = 0;
	if ((TestPos.Tile.x >= 0) && (TestPos.Tile.x < TileMap->tile_count_x) &&
		(TestPos.Tile.y >= 0) && (TestPos.Tile.y < TileMap->tile_count_y)) {

		u32 tile_map_value = TileMap->tiles[TestPos.Tile.y * TileMap->tile_count_x + TestPos.Tile.x];
		Result = (tile_map_value == 0);
	}
	return(Result);
}

inline void
tile_map_coord_remap(tile_map *TileMap, s32 tile_count, s32 *tile, f32 *tile_rel_coord)
{
	s32 offset = f32_round_to_s32(*tile_rel_coord / (f32)TileMap->tile_side_in_meters);
	*tile += offset;

	// NOTE(Justin): Bad bug here before. tile_side_in_meters was an u32 and we
	// multiplied it by s32 and got garbage back
	*tile_rel_coord -= offset * TileMap->tile_side_in_meters;

	ASSERT(*tile_rel_coord >= 0);
	ASSERT(*tile_rel_coord < TileMap->tile_side_in_meters);

	if (*tile < 0) {
		*tile += tile_count;
	}
	if (*tile >= tile_count) {
		*tile -= tile_count;
	}
}

inline tile_map_position
tile_map_position_remap(tile_map *TileMap, tile_map_position TileMapPos)
{
	tile_map_position Result = TileMapPos;

	tile_map_coord_remap(TileMap, TileMap->tile_count_x, &Result.Tile.x, &Result.TileOffset.x);
	tile_map_coord_remap(TileMap, TileMap->tile_count_y, &Result.Tile.y, &Result.TileOffset.y);

	return(Result);
}

inline void
tile_map_tile_set_value(tile_map *TileMap, v2i Tile, u32 tile_value)
{
	ASSERT(TileMap->tiles);
	TileMap->tiles[Tile.y * TileMap->tile_count_x + Tile.x] = tile_value;
}
