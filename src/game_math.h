#if !defined(GAME_MATH_H)

//
// NOTE(Justin): Math types
//

struct v2i
{
	s32 x, y;
};

struct v2u
{
	u32 x, y;
};

union v2f
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
};

union v3f
{
	struct
	{
		f32 x, y, z;
	};
	struct
	{
		f32 u, v, w;
	};
	struct
	{
		f32 r, g, b;
	};
	struct
	{
		v2f xy;
		f32 ignored0_;
	};
	struct
	{
		f32 ignored1_;
		v2f yz;
	};
	struct
	{
		v2f uv;
		f32 ignored2_;
	};
	f32 e[3];
};

union v4f
{
	struct
	{
		f32 x, y, z, w;
	};
	struct
	{
		f32 r, g, b, a;
	};
	f32 e[4];
};

struct m2x2
{
	f32 e[2][2];
};

struct m3x3
{
	f32 e[3][3];
};


inline v2i
V2I(s32 x, s32 y)
{
	v2i Result;

	Result.x = x;
	Result.y = y;

	return(Result);

}

inline v2f
V2F(f32 x, f32 y)
{
	v2f Result;

	Result.x = x;
	Result.y = y;

	return(Result);
}

inline v3f
V3F(f32 x, f32 y, f32 z)
{
	v3f Result;

	Result.x = x;
	Result.y = y;
	Result.z = z;

	return(Result);
}

inline v4f
V4F(f32 x, f32 y, f32 z, f32 w)
{
	v4f Result;

	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;

	return(Result);
}

//
// NOTE(Justin): Scalar operations
//

inline f32
lerp(f32 a, f32 t, f32 b)
{
	f32 Result = (1.0f - t) * a + t * b;
	
	return(Result);
}



//
// NOTE(Justin): v2 operations
//

inline v2f
v2f_perp(v2f V)
{
	v2f Result = {-V.y, V.x};
	return(Result);
}

inline v2i
operator *(f32 c, v2i V)
{
	v2i Result;

	Result.x = (s32)(c * V.x);
	Result.y = (s32)(c * V.y);

	return(Result);
}

inline v2i
operator *(v2i V, f32 c)
{
	v2i Result;

	Result = c * V;

	return(Result);
}

inline v2i
operator +(v2i V1, v2i V2)
{
	v2i Result;

	Result.x = V1.x + V2.x;
	Result.y = V1.y + V2.y;

	return(Result);
}

inline v2f
operator *(f32 c, v2f V)
{
	v2f Result;
	Result.x = c * V.x;
	Result.y = c * V.y;

	return(Result);
}

inline v2f
operator *(v2f V, f32 c)
{
	v2f Result = c * V;

	return(Result);
}

inline v2f &
operator *=(v2f &V, f32 c)
{
	V = c * V;

	return(V);
}

inline v2f
operator +(v2f U, v2f V)
{
	v2f Result;

	Result.x = U.x + V.x;
	Result.y = U.y + V.y;

	return(Result);
}

inline v2f &
operator +=(v2f &U, v2f V)
{
	U = U + V;

	return(U);
}

inline v2f
operator -(v2f U, v2f V)
{
	v2f Result;
	Result.x = U.x - V.x;
	Result.y = U.y - V.y;
	
	return(Result);
}

inline f32
v2f_dot(v2f V1, v2f V2)
{
	f32 Result = V1.x * V2.x + V1.y * V2.y;

	return(Result);
}

inline v2f
v2f_normalize(v2f V)
{
	v2f Result;

	f32 c = (1.0f / f32_sqrt(v2f_dot(V, V)));

	Result = c * V;

	return(Result);
}

inline f32
v2f_length(v2f V)
{
	f32 Result = f32_sqrt(v2f_dot(V, V));

	return(Result);
}

inline f32
v2f_length_squared(v2f V)
{
	f32 Result = v2f_dot(V, V);

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
operator *(f32 c, v3f V)
{
	v3f Result = {};

	Result.x = c * V.x;
	Result.y = c * V.y;
	Result.z = c * V.z;

	return(Result);
}

//
// NOTE(Justin): v4f operations
//

//
// NOTE(Justin): Matrix operations
//

inline m3x3
m3x3_identity_create()
{
	m3x3 Result = {};

	Result.e[0][0] = 1.0f;
	Result.e[1][1] = 1.0f;
	Result.e[2][2] = 1.0f;

	return(Result);
}

inline v3f
m3x3_transform_v3f(m3x3 M, v3f V)
{
	v3f Result = {};


	Result.x = M.e[0][0] * V.x + M.e[0][1] * V.y + M.e[0][2] * V.z;
	Result.y = M.e[1][0] * V.x + M.e[1][1] * V.y + M.e[1][2] * V.z;
	Result.z = M.e[2][0] * V.x + M.e[2][1] * V.y + M.e[2][2] * V.z;

	return(Result);
}

inline v3f
operator *(m3x3 T, v3f V)
{
	v3f Result = {};

	Result = m3x3_transform_v3f(T, V);

	return(Result);
}

inline v2f
m3x3_transform_v2f(m3x3 M, v2f V)
{
	v2f Result = {};

	v3f U = V3F(V.x, V.y, 1.0f);

	U = m3x3_transform_v3f(M, U);

	Result.x = U.x;
	Result.y = U.y;
	
	return(Result);
}

inline v2f
operator *(m3x3 M, v2f V)
{
	v2f Result = {};

	Result = m3x3_transform_v2f(M, V);

	return(Result);
}

inline m3x3
m3x3_scale_create(f32 c)
{
	m3x3 Result =
	{
		{{c, 0.0f, 0.0f},
		{0.0f, c, 0.0f},
		{0.0f, 0.0f, c}},
	};
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
	m3x3 Result = {};

	for(u32 row = 0; row <= 2; row++)
	{
		for(u32 col = 0; col <= 2; col++)
		{
			for(u32 k = 0; k <= 2; k++)
			{
				Result.e[row][col] += A.e[row][k] * B.e[k][col];
			}
		}
	}
	return(Result);
}

inline m3x3
operator *(m3x3 A, m3x3 B)
{
	m3x3 R = m3x3_matrix_mult(A, B);

	return(R);
}

#define GAME_MATH_H
#endif

