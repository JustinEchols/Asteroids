#if !defined(GAME_GEOMETRY_H)

enum shape_type
{
	SHAPE_TRIANGLE,
	SHAPE_CIRCLE,
	SHAPE_POLY,

	SHAPE_COUNT
};

struct rectangle
{
	v2f Min;
	v2f Max;
};

struct player_polygon
{
	v2f Vertices[7];
};

struct interval
{
	f32 min;
	f32 max;
};

struct bounding_box
{
	v2f Min;
	v2f Max;
};

struct square
{
	v2f Center;
	f32 half_width;
};

struct circle
{
	v2f Center;
	f32 radius;
};

struct triangle
{
	v2f Vertices[3];
	v2f Centroid;
};

#define GAME_GEOMETRY_H
#endif
