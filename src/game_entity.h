#if !defined(GAME_ENTITY_H)

#define INVALID_POS V2F(1000.0f, 1000.0f)

inline b32
entity_flag_is_set(entity *Entity, u32 flag)
{
	b32 Result = (Entity->flags & flag);

	return(Result);
}

inline void
entity_flag_set(entity *Entity, u32 flag)
{
	Entity->flags |= flag;
}

inline void
entity_flag_clear(entity *Entity, u32 flag)
{
	Entity->flags &= ~flag;
}

inline void
entity_make_nonspatial(entity *Entity)
{
	entity_flag_set(Entity, ENTITY_FLAG_NON_SPATIAL);
	Entity->Pos = INVALID_POS;
}

inline void
entity_make_spatial(entity *Entity, v2f P, v2f dP)
{
	entity_flag_clear(Entity, ENTITY_FLAG_NON_SPATIAL);
	Entity->Pos = P;
	Entity->dPos = dP;
}

#define GAME_ENTITY_H
#endif
