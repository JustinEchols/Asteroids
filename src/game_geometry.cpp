
internal bounding_box
circle_bounding_box(circle Circle)
{
	bounding_box Result = {};

	Result.Min.x = Circle.Center.x - Circle.radius;
	Result.Min.y = Circle.Center.y - Circle.radius;

	Result.Max.x = Circle.Center.x + Circle.radius;
	Result.Max.y = Circle.Center.y + Circle.radius;

	return(Result);
}

internal v2f
circle_support_point(circle *Circle, v2f Dir)
{
	v2f Result = {};
	Result = Circle->Center + Circle->radius * Dir;
	return(Result);
}

internal v2f
circle_tangent(circle *Circle, v2f Dir)
{
	v2f Result = v2f_perp(Circle->radius * Dir);

	return(Result);
}

inline b32
interval_contains(interval Interval, f32 x)
{
	b32 Result = false;

	if((x >= Interval.min) && (x <= Interval.max))
	{
		Result = true;
	}
	return(Result);
}

inline f32
interval_length(interval Interval)
{
	f32 Result = Interval.max - Interval.min;

	return(Result);
}

internal circle
circle_init(v2f Center, f32 radius)
{
	circle Result = {};

	Result.Center = Center;
	Result.radius = radius;

	return(Result);
}

internal interval 
sat_projected_interval(v2f *Vertices, u32 vertex_count, v2f ProjectedAxis)
{
	interval Result = {};

	f32 min = f32_infinity();
	f32 max = f32_neg_infinity();
	for(u32 vertex_i = 0; vertex_i < vertex_count; vertex_i++)
	{
		v2f Vertex = Vertices[vertex_i];

		f32 c = v2f_dot(Vertex, ProjectedAxis);
		min = MIN(c, min);
		max = MAX(c, max);
	}

	Result.min = min;
	Result.max = max;

	return(Result);
}

internal interval
circle_project_onto_axis(circle *Circle, v2f ProjectedAxis)
{
	interval Result = {};

	f32 min = f32_infinity();
	f32 max = f32_neg_infinity();

	v2f CircleMin = circle_support_point(Circle, -1.0f * ProjectedAxis);
	v2f CircleMax = circle_support_point(Circle, ProjectedAxis);

	f32 projected_min = v2f_dot(CircleMin, ProjectedAxis);
	f32 projected_max = v2f_dot(CircleMax, ProjectedAxis);

	min = MIN(projected_min, min);
	max = MAX(projected_max, max);

	Result.min = min;
	Result.max = max;

	return(Result);
}

internal v2f
triangle_closest_point_to_circle(triangle *Triangle, circle *Circle)
{
	v2f Result = {};

	v2f ClosestPoint = Triangle->Vertices[0];
	v2f CenterToVertex = ClosestPoint - Circle->Center;
	f32 min_sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
	for(u32 vertex_i = 1; vertex_i < ARRAY_COUNT(Triangle->Vertices); vertex_i++)
	{
		v2f Vertex = Triangle->Vertices[vertex_i];
		CenterToVertex = Vertex - Circle->Center;
		f32 sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
		if(sq_distance < min_sq_distance)
		{
			ClosestPoint = Vertex;
			min_sq_distance = sq_distance;
		}
	}
	Result = ClosestPoint;

	return(Result);
}

internal v2f 
closest_point_to_circle(v2f *Vertices, u32 vertex_count, circle *Circle)
{
	v2f Result = {};

	v2f ClosestPoint = Vertices[0];
	v2f CenterToVertex = ClosestPoint - Circle->Center;
	f32 min_sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
	for(u32 vertex_i = 1; vertex_i < vertex_count; vertex_i++)
	{
		v2f Vertex = Vertices[vertex_i];
		CenterToVertex = Vertex - Circle->Center;
		f32 sq_distance = v2f_dot(CenterToVertex, CenterToVertex);
		if(sq_distance < min_sq_distance)
		{
			ClosestPoint = Vertex;
			min_sq_distance = sq_distance;
		}
	}
	Result = ClosestPoint;

	return(Result);
}


internal v2f
poly_and_circle_collides(v2f *Vertices, u32 vertex_count, circle Circle)
{
	//b32 GapExists = false;
	v2f Normal = {};

	f32 min_length = f32_infinity();
	f32 overlap = 0.0f;

	for(u32 vertex_i = 0; vertex_i < vertex_count; vertex_i++)
	{
		v2f P0 = Vertices[vertex_i];
		v2f P1 = Vertices[(vertex_i + 1) % vertex_count];
		v2f D = P1 - P0;

		v2f ProjectedAxis = v2f_normalize(-1.0f * v2f_perp(D));

		interval PolyInterval = sat_projected_interval(Vertices, vertex_count, ProjectedAxis);
		interval CircleInterval = circle_project_onto_axis(&Circle, ProjectedAxis);

		if(CircleInterval.min > CircleInterval.max)
		{
			f32 temp = CircleInterval.min;
			CircleInterval.min = CircleInterval.max;
			CircleInterval.max = temp;
		}

		if(!((CircleInterval.max >= PolyInterval.min) &&
			(PolyInterval.max >= CircleInterval.min)))
		{

			return(Normal);
		}

		overlap = ABS(interval_length(PolyInterval) - interval_length(CircleInterval));
		if(overlap < min_length)
		{
			min_length = overlap;
			Normal = ProjectedAxis;
		}

	}

	v2f ClosestPoint = closest_point_to_circle(Vertices, vertex_count, &Circle);

	v2f ProjectedAxis = v2f_normalize(ClosestPoint - Circle.Center);

	interval PolyInterval = sat_projected_interval(Vertices, vertex_count, ProjectedAxis);
	interval CircleInterval = circle_project_onto_axis(&Circle, ProjectedAxis);

	if(!((CircleInterval.max >= PolyInterval.min) &&
		(PolyInterval.max >= CircleInterval.min)))
	{
		return(Normal);
	}

	overlap = ABS(interval_length(PolyInterval) - interval_length(CircleInterval));
	if(overlap < min_length)
	{
		min_length = overlap;
		Normal = ProjectedAxis;
	}

	return(Normal);
}

internal b32
circles_collide(v2f CircleACenter, v2f CircleADelta, f32 radius_a,
				v2f CircleBCenter, v2f CircleBDelta, f32 radius_b, f32 *t_min)
{
	b32 Result = false;

	v2f DeltaBetweenCenters = CircleBCenter - CircleACenter;
	f32 radii_sum = radius_b + radius_a;
	v2f RelVel = CircleBDelta - CircleADelta;

	f32 c = v2f_dot(DeltaBetweenCenters, DeltaBetweenCenters) - SQUARE(radii_sum);
	if(c < 0.0f)
	{
		// NOTE(Justin): The distance squared of the vector
		// between the centers is less than the square of
		// the sum of the radii. This means that the circles
		// were initially overlapping.
	}
	else
	{
		f32 a = v2f_dot(RelVel, RelVel);
		f32 epsilon = 0.001f;
		if(a < epsilon)
		{
			// NOTE(Justin): Circles are not moving relative to
			// each other. So they will not intersect.
		}
		else
		{
			f32 b = v2f_dot(DeltaBetweenCenters, RelVel);
			if(b >= 0.0f)
			{
				// NOTE(Justin): Circles are not moving towards each
				// other
			}
			else
			{
				f32 d = SQUARE(b) - a * c;
				if(d < 0.0f)
				{
					// NOTE(Justin): Circles do not intersect
				}
				else
				{
					f32 t = (-b - f32_sqrt(d)) / a;
					if(*t_min > t)
					{
						*t_min = MAX(0.0f, t - epsilon);
						Result = true;
					}
					else
					{
						// NTOE(Justin): It is possible
						// that we find a t value far
						// into the future which we do
						// not care about
					}
				}
			}
		}
	}
	return(Result);
}

internal triangle
triangle_init(v2f *Vertices, u32 vertex_count)
{
	// NOTE(Justin): Vertices are sorted into CCW order.

	ASSERT(vertex_count == 3);

	triangle Result = {};

	v2f *Right = Vertices;
	v2f *Middle = Vertices + 1;
	v2f *Left = Vertices + 2;

	if(Right->x < Middle->x)
	{
		v2f *Temp = Right;
		Right = Middle;
		Middle = Temp;
	}
	if(Right->x < Left->x)
	{
		v2f *Temp = Right;
		Right = Left;
		Left = Temp;
	}
	if(Middle->x < Left->x)
	{
		v2f *Temp = Middle;
		Middle = Left;
		Left = Temp;
	}

	Result.Centroid.x = ((Right->x + Middle->x + Left->x) / 3);
	Result.Centroid.y = ((Right->y + Middle->y + Left->y) / 3);

	Result.Vertices[0] = *Right;
	Result.Vertices[1] = *Middle;
	Result.Vertices[2] = *Left;

	return(Result);
}
