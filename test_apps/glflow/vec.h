#ifndef VEC_H
#define VEC_H

//canonical vectors, and the zero vector
static float i3[3] = {1.f, 0.f, 0.f};
static float j3[3] = {0.f, 1.f, 0.f};
static float k3[3] = {0.f, 0.f, 1.f};
static float z3[3] = {0.f, 0.f, 0.f};
//uncomment this if you want fancy stuff
//static float* c3[4] = {i3, j3, k3, z3};

//move a into b
inline void mov(const float* a, float* b)
{
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
}

//multiply a by f
inline void mul(const float* a, float f, float* r)
{
    r[0] = a[0] * f;
    r[1] = a[1] * f;
    r[2] = a[2] * f;
}

//negate a
inline void neg(const float* a, float* r)
{
    r[0] = -a[0];
    r[1] = -a[1];
    r[2] = -a[2];
}

//divide a by d
inline void div(const float* a, float d, float* r)
{
    if(d == 0.f)
    {
        fprintf(stderr, "DIV: DIVIDING BY ZERO\n");
        r[0] = 0.f;
        r[1] = 0.f;
        r[2] = 0.f;
        return;
    }
    r[0] = a[0] / d;
    r[1] = a[1] / d;
    r[2] = a[2] / d;
}

//should be faster than normal div, but less stable
inline void fdiv(const float* a, float d, float* r)
{
    if(d == 0.f)
    {
        fprintf(stderr, "FDIV: DIVIDING BY ZERO\n");
        r[0] = 0.f;
        r[1] = 0.f;
        r[2] = 0.f;
        return;
    }
    mul(a, 1.f / d, r);
}

//add a to b
inline void add(const float* a, const float* b, float* r)
{
    r[0] = a[0] + b[0];
    r[1] = a[1] + b[1];
    r[2] = a[2] + b[2];
}

//subtract b from a
inline void sub(const float* a, const float* b, float* r)
{
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    r[2] = a[2] - b[2];
}

//compare a to b (true/false)
inline bool cmp(const float* a, const float* b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

//true if target vector is zeroes
inline bool cmpz(const float* a)
{
    return a[0] == 0.f && a[1] == 0.f && a[2] == 0.f;
}

//get a*b
inline float dot(const float* a, const float* b)
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

//get the magnitude-squared of a
inline float mag2(const float* a)
{
    return (a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]);
}

//get the magnitude of a
inline float mag(const float* a)
{
    return sqrtf(mag2(a));
}

//true if vectors are parallel
inline bool ispara(const float* a, const float* b)
{
    return mag2(a) == dot(a, b);
}

//true if vectors are perpendicular
inline bool isperp(const float* a, const float* b)
{
    return dot(a, b) == 0.f;
}

//get normal of a
inline void norm(const float* a, float* r)
{
    float m = mag(a);
    if(m == 0.f)
    {
        fprintf(stderr, "NORM: DIVIDING BY ZERO\n");
        r[0] = 0.f;
        r[1] = 0.f;
        r[2] = 0.f;
        return;
    }
    div(a, m, r);
}

//get faster normal, less stable, of a
inline void fnorm(const float* a, float* r)
{
    fdiv(a, mag(a), r);
}

//set the size of the given vector (shouldn't affect direction)
inline void resize(const float* a, float newmag, float* r)
{
    float m = mag(a);
    if(m == 0.f)
    {
        fprintf(stderr, "RESIZE: DIVIDING BY ZERO\n");
        r[0] = 0.f;
        r[1] = 0.f;
        r[2] = 0.f;
        return;
    }
    mul(a, newmag / m, r);
}

//set r to the cross product a x b
inline void cross(const float* a, const float* b, float* r)
{
    r[0] = a[1] * b[2] - b[1] * a[2];
    r[1] = a[2] * b[0] - b[2] * a[0];
    r[2] = a[0] * b[1] - b[0] * a[1];
}

inline void printvec(const float* a)
{
    printf("(%10f,%10f,%10f)\n", a[0], a[1], a[2]);
}

//project a onto b, return in r
inline void proj(const float* a, const float* b, float* r)
{
    float m = mag2(b);
    if(m == 0.f)
    {
        fprintf(stderr, "PROJ: DIVIDING BY ZERO\n");
        r[0] = 0.f;
        r[1] = 0.f;
        r[2] = 0.f;
        return;
    }
    mul(b, dot(a, b) / m, r);
}

//get cos of angle between two vectors
inline float cos(const float* a, const float* b)
{
    float m = mag(a) * mag(b);
    if(m == 0.f)
    {
        fprintf(stderr, "COS: DIVIDING BY ZERO\n");
        return 0.f;
    }
    return dot(a, b) / m;
}

//orthogonalize a with respect to b, return in r
inline void ortho(const float* a, const float* b, float* r)
{
    float t[3];
    proj(a, b, t);
    sub(a, t, r); //r, a, r
}

//as seen from 0, are 1 2 and 3 listed in CW or CCW order?
//returns positive values for ccw data, negative for cw data
//returns 0.f if the values are coplanar
inline float cwccw(const float* data)
{
    float oa[3];
    float ab[3];
    float bc[3];
    float up[3];
    
    //get difference vectors
    sub(data + 3, data, oa);
    sub(data + 6, data + 3, ab);
    sub(data + 9, data + 6, bc);
    
    //get the "up" vector (oa is forward, ab points right)
    cross(oa, ab, up);
    //return "up"-ness of bc
    return dot(up, bc);
}

inline bool icCCW(const float* data)
{
    return cwccw(data) > 0.f;
}

inline bool isCW(const float* data)
{
    return cwccw(data) < 0.f;
}

inline bool coplanar(const float* data)
{
    return cwccw(data) == 0.f;
}

#endif
