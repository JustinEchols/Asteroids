#if !defined(GAME_SIM_REGION_H)

struct sim_region
{
	u32 entity_count_max;
	u32 entity_count;
	entity *Entities;
}
#define GAME_SIM_REGION_H
#endif
