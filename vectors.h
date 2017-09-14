#include <math.h>

typedef struct _M3DVECTOR{
	float x;
	float y;
	float z;

public:
    _M3DVECTOR() { }
    _M3DVECTOR(float f);
    _M3DVECTOR(float X, float Y, float Z);

    // =====================================
    // Assignment operators
    // =====================================

    _M3DVECTOR& operator += (const _M3DVECTOR& v);
    _M3DVECTOR& operator -= (const _M3DVECTOR& v);
    _M3DVECTOR& operator *= (const _M3DVECTOR& v);
    _M3DVECTOR& operator /= (const _M3DVECTOR& v);
    _M3DVECTOR& operator *= (float s);
    _M3DVECTOR& operator /= (float s);

    // =====================================
    // Unary operators
    // =====================================

    friend _M3DVECTOR operator + (const _M3DVECTOR& v);
    friend _M3DVECTOR operator - (const _M3DVECTOR& v);


    // =====================================
    // Binary operators
    // =====================================

    // Addition and subtraction
        friend _M3DVECTOR operator + (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
        friend _M3DVECTOR operator - (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
    // Scalar multiplication and division
        friend _M3DVECTOR operator * (const _M3DVECTOR& v, float s);
        friend _M3DVECTOR operator * (float s, const _M3DVECTOR& v);
        friend _M3DVECTOR operator / (const _M3DVECTOR& v, float s);
    // Memberwise multiplication and division
        friend _M3DVECTOR operator * (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
        friend _M3DVECTOR operator / (const _M3DVECTOR& v1, const _M3DVECTOR& v2);

    // Vector dominance
        friend int operator < (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
        friend int operator <= (const _M3DVECTOR& v1, const _M3DVECTOR& v2);

    // Bitwise equality
        friend int operator == (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
        friend int operator != (const _M3DVECTOR& v1, const _M3DVECTOR& v2);

    // Length-related functions
        friend float SquareMagnitude (const _M3DVECTOR& v);
        friend float Magnitude (const _M3DVECTOR& v);

    // Returns vector with same direction and unit length
        friend _M3DVECTOR Normalize (const _M3DVECTOR& v);

    // Return min/max component of the input vector
        friend float Min (const _M3DVECTOR& v);
        friend float Max (const _M3DVECTOR& v);

    // Dot and cross product
        friend float DotProduct (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
        friend _M3DVECTOR CrossProduct (const _M3DVECTOR& v1, const _M3DVECTOR& v2);
}M3DVECTOR;



inline _M3DVECTOR::_M3DVECTOR(float f){
        x=y=z=f;
}

inline _M3DVECTOR::_M3DVECTOR(float X, float Y, float Z){
	x = X;y = Y;z = Z;
}
// =====================================
// Assignment operators
// =====================================

inline _M3DVECTOR& _M3DVECTOR::operator += (const _M3DVECTOR& v)
{
   x += v.x;   y += v.y;   z += v.z;
   return *this;
}

inline _M3DVECTOR& _M3DVECTOR::operator -= (const _M3DVECTOR& v)
{
   x -= v.x;   y -= v.y;   z -= v.z;
   return *this;
}

inline _M3DVECTOR& _M3DVECTOR::operator *= (const _M3DVECTOR& v)
{
   x *= v.x;   y *= v.y;   z *= v.z;
   return *this;
}

inline _M3DVECTOR& _M3DVECTOR::operator /= (const _M3DVECTOR& v)
{
   x /= v.x;   y /= v.y;   z /= v.z;
   return *this;
}

inline _M3DVECTOR& _M3DVECTOR::operator *= (float s)
{
   x *= s;   y *= s;   z *= s;
   return *this;
}

inline _M3DVECTOR& _M3DVECTOR::operator /= (float s)
{
   x /= s;   y /= s;   z /= s;
   return *this;
}

inline _M3DVECTOR operator + (const _M3DVECTOR& v)
{
   return v;
}

inline _M3DVECTOR operator - (const _M3DVECTOR& v)
{
   return _M3DVECTOR(-v.x, -v.y, -v.z);
}

inline _M3DVECTOR operator + (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return _M3DVECTOR(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z);
}

inline _M3DVECTOR operator - (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return _M3DVECTOR(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z);
}

inline _M3DVECTOR operator * (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return _M3DVECTOR(v1.x*v2.x, v1.y*v2.y, v1.z*v2.z);
}

inline _M3DVECTOR operator / (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return _M3DVECTOR(v1.x/v2.x, v1.y/v2.y, v1.z/v2.z);
}

inline _M3DVECTOR operator * (const _M3DVECTOR& v, float s)
{
   return _M3DVECTOR(s*v.x, s*v.y, s*v.z);
}

inline _M3DVECTOR operator * (float s, const _M3DVECTOR& v)
{
   return _M3DVECTOR(s*v.x, s*v.y, s*v.z);
}

inline _M3DVECTOR operator / (const _M3DVECTOR& v, float s)
{
   return _M3DVECTOR(v.x/s, v.y/s, v.z/s);
}

inline int operator == (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return v1.x==v2.x && v1.y==v2.y && v1.z == v2.z;
}
inline int operator != (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return v1.x!=v2.x || v1.y!=v2.y || v1.z != v2.z;
}

inline float Magnitude (const _M3DVECTOR& v)
{
   return (float) sqrt(SquareMagnitude(v));
}

inline float SquareMagnitude (const _M3DVECTOR& v)
{
   return v.x*v.x + v.y*v.y + v.z*v.z;
}

inline _M3DVECTOR Normalize (const _M3DVECTOR& v)
{
   return v / Magnitude(v);
}

inline float Min (const _M3DVECTOR& v)
{
   float ret = v.x;
   if (v.y < ret) ret = v.y;
   if (v.z < ret) ret = v.z;
   return ret;
}

inline float Max (const _M3DVECTOR& v)
{
   float ret = v.x;
   if (ret < v.y) ret = v.y;
   if (ret < v.z) ret = v.z;
   return ret;
}

inline float DotProduct (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
   return v1.x*v2.x + v1.y * v2.y + v1.z*v2.z;
}

inline _M3DVECTOR CrossProduct (const _M3DVECTOR& v1, const _M3DVECTOR& v2)
{
        return _M3DVECTOR(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}






typedef struct _M3DMATRIX{
        union{
	struct{
                float _11,_12,_13,_14;
                float _21,_22,_23,_24;
                float _31,_32,_33,_34;
                float _41,_42,_43,_44;
	};
	float m[4][4];
        };

public:

	_M3DMATRIX() { }
	float& operator()(int iRow, int iColumn) { return m[iRow][iColumn]; }
        _M3DMATRIX( float _m00, float _m01, float _m02, float _m03,
                    float _m10, float _m11, float _m12, float _m13,
                    float _m20, float _m21, float _m22, float _m23,
                    float _m30, float _m31, float _m32, float _m33
                  )
        {
                m[0][0] = _m00; m[0][1] = _m01; m[0][2] = _m02; m[0][3] = _m03;
                m[1][0] = _m10; m[1][1] = _m11; m[1][2] = _m12; m[1][3] = _m13;
                m[2][0] = _m20; m[2][1] = _m21; m[2][2] = _m22; m[2][3] = _m23;
                m[3][0] = _m30; m[3][1] = _m31; m[3][2] = _m32; m[3][3] = _m33;
        }
        friend _M3DMATRIX operator * (const _M3DMATRIX&, const _M3DMATRIX&);
}M3DMATRIX;


inline _M3DMATRIX operator* (const _M3DMATRIX& a, const _M3DMATRIX& b)
{
    _M3DMATRIX ret;

    ret._11=a._11*b._11+a._12*b._21+a._13*b._31+a._14*b._41;
    ret._12=a._11*b._12+a._12*b._22+a._13*b._32+a._14*b._42;
    ret._13=a._11*b._13+a._12*b._23+a._13*b._33+a._14*b._43;
    ret._14=a._11*b._14+a._12*b._24+a._13*b._34+a._14*b._44;

    ret._21=a._21*b._11+a._22*b._21+a._23*b._31+a._24*b._41;
    ret._22=a._21*b._12+a._22*b._22+a._23*b._32+a._24*b._42;
    ret._23=a._21*b._13+a._22*b._23+a._23*b._33+a._24*b._43;
    ret._24=a._21*b._14+a._22*b._24+a._23*b._34+a._24*b._44;

    ret._31=a._31*b._11+a._32*b._21+a._33*b._31+a._34*b._41;
    ret._32=a._31*b._12+a._32*b._22+a._33*b._32+a._34*b._42;
    ret._33=a._31*b._13+a._32*b._23+a._33*b._33+a._34*b._43;
    ret._34=a._31*b._14+a._32*b._24+a._33*b._34+a._34*b._44;

    ret._41=a._41*b._11+a._42*b._21+a._43*b._31+a._44*b._41;
    ret._42=a._41*b._12+a._42*b._22+a._43*b._32+a._44*b._42;
    ret._43=a._41*b._13+a._42*b._23+a._43*b._33+a._44*b._43;
    ret._44=a._41*b._14+a._42*b._24+a._43*b._34+a._44*b._44;
      
    return ret;
}










typedef struct _M4DVECTOR{
	float x;
	float y;
	float z;
	float t;

public:
    _M4DVECTOR() { }
    _M4DVECTOR(float f);
    _M4DVECTOR(float X, float Y, float Z, float t);
    _M4DVECTOR(M3DVECTOR);

    // =====================================
    // Assignment operators
    // =====================================

    _M4DVECTOR& operator += (const _M4DVECTOR& v);
    _M4DVECTOR& operator -= (const _M4DVECTOR& v);
    _M4DVECTOR& operator *= (const _M4DVECTOR& v);
    _M4DVECTOR& operator /= (const _M4DVECTOR& v);
    _M4DVECTOR& operator *= (float s);
    _M4DVECTOR& operator /= (float s);

    // =====================================
    // Unary operators
    // =====================================

    friend _M4DVECTOR operator + (const _M4DVECTOR& v);
    friend _M4DVECTOR operator - (const _M4DVECTOR& v);


    // =====================================
    // Binary operators
    // =====================================

    // Addition and subtraction
        friend _M4DVECTOR operator + (const _M4DVECTOR& v1, const _M4DVECTOR& v2);
        friend _M4DVECTOR operator - (const _M4DVECTOR& v1, const _M4DVECTOR& v2);
    // Scalar multiplication and division
        friend _M4DVECTOR operator * (const _M4DVECTOR& v, float s);
        friend _M4DVECTOR operator * (float s, const _M4DVECTOR& v);
        friend _M4DVECTOR operator / (const _M4DVECTOR& v, float s);
    // Memberwise multiplication and division
        friend _M4DVECTOR operator * (const _M4DVECTOR& v1, const _M4DVECTOR& v2);
        friend _M4DVECTOR operator / (const _M4DVECTOR& v1, const _M4DVECTOR& v2);

    // Vector dominance
        friend int operator < (const _M4DVECTOR& v1, const _M4DVECTOR& v2);
        friend int operator <= (const _M4DVECTOR& v1, const _M4DVECTOR& v2);

    // Bitwise equality
        friend int operator == (const _M4DVECTOR& v1, const _M4DVECTOR& v2);
        friend int operator != (const _M4DVECTOR& v1, const _M4DVECTOR& v2);

    // Length-related functions
        friend float SquareMagnitude (const _M4DVECTOR& v);
        friend float Magnitude (const _M4DVECTOR& v);

    // Returns vector with same direction and unit length
        friend _M4DVECTOR Normalize (const _M4DVECTOR& v);

    // Return min/max component of the input vector
        friend float Min (const _M4DVECTOR& v);
        friend float Max (const _M4DVECTOR& v);

    // Dot product
        friend float DotProduct (const _M4DVECTOR& v1, const _M4DVECTOR& v2);
}M4DVECTOR;



inline _M4DVECTOR::_M4DVECTOR(float f){
        x=y=z=f;
}

inline _M4DVECTOR::_M4DVECTOR(float X, float Y, float Z, float T){
	x = X;y = Y;z = Z;t = T;
}
inline _M4DVECTOR::_M4DVECTOR(M3DVECTOR v){
        x=v.x; y=v.y; z=v.z; t=0.0f;
}
// =====================================
// Assignment operators
// =====================================

inline _M4DVECTOR& _M4DVECTOR::operator += (const _M4DVECTOR& v)
{
   x += v.x;   y += v.y;   z += v.z;	t +=v.t;
   return *this;
}

inline _M4DVECTOR& _M4DVECTOR::operator -= (const _M4DVECTOR& v)
{
   x -= v.x;   y -= v.y;   z -= v.z;	t -=v.z;
   return *this;
}

inline _M4DVECTOR& _M4DVECTOR::operator *= (const _M4DVECTOR& v)
{
   x *= v.x;   y *= v.y;   z *= v.z;	t *= v.t; 
   return *this;
}

inline _M4DVECTOR& _M4DVECTOR::operator /= (const _M4DVECTOR& v)
{
   x /= v.x;   y /= v.y;   z /= v.z;	t /= v.t;
   return *this;
}

inline _M4DVECTOR& _M4DVECTOR::operator *= (float s)
{
   x *= s;   y *= s;   z *= s;	t *= s;
   return *this;
}

inline _M4DVECTOR& _M4DVECTOR::operator /= (float s)
{
   x /= s;   y /= s;   z /= s;	t /= s;
   return *this;
}

inline _M4DVECTOR operator + (const _M4DVECTOR& v)
{
   return v;
}

inline _M4DVECTOR operator - (const _M4DVECTOR& v)
{
   return _M4DVECTOR(-v.x, -v.y, -v.z, -v.t);
}

inline _M4DVECTOR operator + (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return _M4DVECTOR(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z, v1.t+v2.t);
}

inline _M4DVECTOR operator - (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return _M4DVECTOR(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z, v1.t-v2.t);
}

inline _M4DVECTOR operator * (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return _M4DVECTOR(v1.x*v2.x, v1.y*v2.y, v1.z*v2.z, v1.t*v2.t);
}

inline _M4DVECTOR operator / (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return _M4DVECTOR(v1.x/v2.x, v1.y/v2.y, v1.z/v2.z, v1.t/v2.t);
}

inline _M4DVECTOR operator * (const _M4DVECTOR& v, float s)
{
   return _M4DVECTOR(s*v.x, s*v.y, s*v.z, s*v.t);
}

inline _M4DVECTOR operator * (float s, const _M4DVECTOR& v)
{
   return _M4DVECTOR(s*v.x, s*v.y, s*v.z, s*v.t);
}

inline _M4DVECTOR operator / (const _M4DVECTOR& v, float s)
{
   return _M4DVECTOR(v.x/s, v.y/s, v.z/s, v.t/s);
}

inline int operator == (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return v1.x==v2.x && v1.y==v2.y && v1.z == v2.z && v1.t == v2.t;
}
inline int operator != (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return v1.x!=v2.x || v1.y!=v2.y || v1.z != v2.z || v1.t != v2.t;
}

inline float Magnitude (const _M4DVECTOR& v)
{
   return (float) sqrt(SquareMagnitude(v));
}

inline float SquareMagnitude (const _M4DVECTOR& v)
{
   return v.x*v.x + v.y*v.y + v.z*v.z + v.t*v.t;
}

inline _M4DVECTOR Normalize (const _M4DVECTOR& v)
{
   return v / Magnitude(v);
}

inline float Min (const _M4DVECTOR& v)
{
   float ret = v.x;
   if (v.y < ret) ret = v.y;
   if (v.z < ret) ret = v.z;
   if (v.t < ret) ret = v.t;
   return ret;
}

inline float Max (const _M4DVECTOR& v)
{
   float ret = v.x;
   if (ret < v.y) ret = v.y;
   if (ret < v.z) ret = v.z;
   if (ret < v.t) ret = v.t;
   return ret;
}

inline float DotProduct (const _M4DVECTOR& v1, const _M4DVECTOR& v2)
{
   return v1.x*v2.x + v1.y * v2.y + v1.z*v2.z + v1.t*v2.t;
}
