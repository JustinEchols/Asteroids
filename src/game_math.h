#if !defined(GAME_MATH_H)

typedef struct
{
	f32 e[3][3];
} m3x3;

typedef struct 
{
	s32 x, y;
} v2i;

typedef struct
{
	u32 x, y;
} v2u;

typedef struct
{
	f32 e[2][2];
} m2x2;

typedef union
{
	struct
	{
		f32 x, y;
	};
	struct
	{
		f32 u, v;
	};
	f32 e[2];
} v2f;

typedef union
{
	struct
	{
		f32 x, y, z;
	};
	struct
	{
		f32 u, v, w;
	};
	f32 e[3];
} v3f;

inline v2i
v2i_create(s32 x, s32 y)
{
	v2i Result = {0};
	Result.x = x;
	Result.y = y;
	return(Result);

}

inline v2i
v2i_scale(f32 c, v2i V)
{
	v2i Result = {0};
	Result.x = (s32)(c * V.x);
	Result.y = (s32)(c * V.y);
	return(Result);
}

inline v2f
v2f_create(f32 x, f32 y)
{
	v2f Result = {0};

	Result.x = x;
	Result.y = y;

	return(Result);
}

inline v2f
v2f_scale(f32 c, v2f V)
{
	v2f Result = {0};

	Result.x = c * V.x;
	Result.y = c * V.y;

	return(Result);
}

inline v2f
v2f_add(v2f V1, v2f V2)
{
	v2f Result = {0};

	Result.x = V1.x + V2.x;
	Result.y = V1.y + V2.y;

	return(Result);
}

inline v2f
v2f_subtract(v2f V1, v2f V2)
{
	v2f Result = {0};

	Result.x = V1.x - V2.x;
	Result.y = V1.y - V2.y;

	return(Result);
}

inline f32
v2f_dot(v2f V1, v2f V2)
{
	f32 Result = 0.0f;
	Result = V1.x * V2.x + V1.y * V2.y;
	return(Result);
}

inline v2f
v2f_normalize(v2f V)
{
	v2f Result = {0};

	f32 c = sqrtf(v2f_dot(V, V));

	Result = v2f_scale(1 / c, V);

	return(Result);
}

inline f32
v2f_length(v2f V)
{
	f32 Result = 0.0f;

	Result = sqrtf(v2f_dot(V, V));

	return(Result);
}

inline v2i
v2i_add(v2i V1, v2i V2)
{
	v2i Result = {0};

	Result.x = V1.x + V2.x;
	Result.y = V1.y + V2.y;

	return(Result);
}

inline v2i
v2i_subtract(v2i V1, v2i V2)
{
	v2i Result = {0};

	Result.x = V1.x - V2.x;
	Result.y = V1.y - V2.y;

	return(Result);
}

inline v2f
m2x2_transform_v2f(m2x2 M, v2f V)
{
	v2f Result = {0};

	Result.x = M.e[0][0] * V.x + M.e[0][1] * V.y;
	Result.y = M.e[1][0] * V.x + M.e[1][1] * V.y;

	return(Result);
}

inline m2x2
m2x2_scale_create(f32 c)
{
	m2x2 R =
	{
		{{c, 0.0f},
		{0.0f, c}}
	};
	return(R);
}

inline m2x2
m2x2_rotation_create(f32 angle)
{
	m2x2 R =
	{
		{{cosf(angle), sinf(angle)},
		{-sinf(angle), cosf(angle)}}
	};
	return(R);
}







inline v3f
v3f_create(f32 x, f32 y, f32 z)
{
	v3f Result = {0};

	Result.x = x;
	Result.y = y;
	Result.z = z;

	return(Result);
}

inline v3f
v3f_scale(f32 c, v3f V)
{
	v3f Result = {0};

	Result.x = c * V.x;
	Result.y = c * V.y;
	Result.z = c * V.z;

	return(Result);
}

inline m3x3
m3x3_identity_create()
{
	m3x3 Result = {0};

	Result.e[0][0] = 1.0f;
	Result.e[1][1] = 1.0f;
	Result.e[2][2] = 1.0f;

	return(Result);
}


inline v3f
m3x3_transform_v3f(m3x3 M, v3f V)
{
	v3f Result = {0};


	Result.x = M.e[0][0] * V.x + M.e[0][1] * V.y + M.e[0][2] * V.z;
	Result.y = M.e[1][0] * V.x + M.e[1][1] * V.y + M.e[1][2] * V.z;
	Result.z = M.e[2][0] * V.x + M.e[2][1] * V.y + M.e[2][2] * V.z;

	return(Result);
}

inline v2f
m3x3_transform_v2f(m3x3 M, v2f V)
{
	v2f Result = {0};

	v3f U = v3f_create(V.x, V.y, 1.0f);

	U = m3x3_transform_v3f(M, U);

	Result.x = U.x;
	Result.y = U.y;
	
	return(Result);
}

inline m3x3
m3x3_translation_create(v2f V)
{
	m3x3 Result =
	{
		{{1.0f, 0.0f, V.x},
		{0.0f, 1.0f, V.y},
		{0.0f, 0.0f, 1.0f}},
	};
	return(Result);
}

inline v2f
m3x3_translate_v2f(m3x3 T, v2f V)
{
	// TODO(Justin): Assert T is a translation matrix;
	v2f Result = {0};

	Result.x = V.x + T.e[0][2];
	Result.y = V.y + T.e[1][2];
	//Result = m3x3_transform_v2f(T, V);
	return(Result);
}

inline m3x3
m3x3_rectangle_transform_create(int width, int height)
{
	f32 w = (f32)width;
	f32 h = (f32)height;
	m3x3 Result = 
	{
		{{w / 2.0f, 0.0f, w / 2.0f},
		{0.0f, h / 2.0f, h / 2.0f},
		{0.0f, 0.0f, 1.0f}}
	};
	return(Result);
}

inline m3x3
m3x3_rotate_about_origin(f32 angle)
{
	m3x3 Result =
	{
		{{cosf(angle), -sinf(angle), 0.0f},
		{sinf(angle), cosf(angle), 0.0f},
		{0.0f, 0.0f, 1.0f}}
	};
	return(Result);
}

inline m3x3
m3x3_matrix_mult(m3x3 A, m3x3 B)
{
	m3x3 Result = {0};

	for (u32 row = 0; row <= 2; row++) {
		for (u32 col = 0; col <= 2; col++) {
			for (u32 k = 0; k <= 2; k++) {
				Result.e[row][col] += A.e[row][k] * B.e[k][col];
			}
		}
	}
	return(Result);
}

#if 0
inline v2f
m3x3_transform_to_screen_space(m3x3 MapToScreenSpace, v2f V)
{
	v2f Result = {0};

	v3f U = v3f_create(V.x, V.y, 1.0f);

	U = m3x3_transform_v3f(MapToScreenSpace, U);

	Result.x = U.x;
	Result.y = U.y;

	return(Result);



}
#endif
#define GAME_MATH_H
#endif

