
internal v2f
tile_map_get_screen_coordinates(tile_map *TileMap, tile_map_position *TileMapPos, v2f BottomLeft)
{
	v2f Result = {};

	Result.x = BottomLeft.x + TileMap->tile_side_in_pixels * TileMapPos->Tile.x + 
		(TileMap->tile_side_in_pixels / 2) + TileMap->meters_to_pixels * TileMapPos->TileOffset.x;
	Result.y = BottomLeft.y + TileMap->tile_side_in_pixels * TileMapPos->Tile.y +
		(TileMap->tile_side_in_pixels / 2) + TileMap->meters_to_pixels * TileMapPos->TileOffset.y;

	return(Result);
}

inline u32
tile_map_get_tile_value_unchecked(tile_map *TileMap, v2i Tile)
{
	u32 TileValue = TileMap->tiles[Tile.y * TileMap->tile_count_x + Tile.x];
	return(TileValue);

}

internal b32
tile_map_tile_is_empty(tile_map *TileMap, v2i TestTile)
{
	b32 Result = 0;
	if ((TestTile.x >= 0) && (TestTile.x < TileMap->tile_count_x) &&
		(TestTile.y >= 0) && (TestTile.y < TileMap->tile_count_y)) {

		u32 tile_value = TileMap->tiles[TestTile.y * TileMap->tile_count_x + TestTile.x];
		Result = (tile_value == 0);
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

	ASSERT(*tile_rel_coord >= -0.5f * TileMap->tile_side_in_meters);
	ASSERT(*tile_rel_coord <= 0.5f * TileMap->tile_side_in_meters);

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

inline b32
tile_map_is_tile_value_empty(u32 tile_value)
{
	b32 Result = (tile_value == 0);

	return(Result);
}


inline tile_map_position
tile_map_get_centered_position(s32 tile_x, s32 tile_y)
{
	tile_map_position Result = {};

	Result.Tile.x = tile_x;
	Result.Tile.y = tile_y;

	return(Result);
}



internal tile_map_pos_delta
tile_map_get_pos_delta(tile_map *TileMap, tile_map_position *Pos1, tile_map_position *Pos2)
{
	tile_map_pos_delta Result = {};

	v2f TileDelta;
	TileDelta.x = (f32)Pos1->Tile.x - (f32)Pos2->Tile.x;
	TileDelta.y = (f32)Pos1->Tile.y - (f32)Pos2->Tile.y;

	v2f TileOffsetDelta = Pos1->TileOffset - Pos2->TileOffset;

	Result.dOffset = TileMap->tile_side_in_meters * TileDelta + TileOffsetDelta;

	return(Result);
}

inline b32
tile_map_on_same_tile(tile_map_position *Pos1, tile_map_position *Pos2)
{
	b32 Result = ((Pos1->Tile.x == Pos2->Tile.x) &&
				 (Pos1->Tile.y == Pos2->Tile.y));

	return(Result);


}

