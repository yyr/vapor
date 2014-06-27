#include "glflow.h"

#include <cmath>
#include <cstdio>
#include "glinc.h"
#include "vec.h"



///////////////////////////////////////////////////////////////////////////////
// BEGIN ANONYMOUS NAMESPACE //////////////////////////////////////////////////
namespace
{
//sizes of various structures, in floats
//these denote the sizes of the vertex sections, not normal sections
inline int ringSize(int q){return 3 << (2 + q);}
inline int tubeSize(int q){return 2 * ringSize(q);}
inline int coneSize(int q){return ringSize(q) + 3;}

//create a ring with the given direction, radius, number of subdivisions
//hint allows us to point the first-index spoke of the ring
inline void mkring(const float* d, int n, float r, float* o, float* on = NULL, float* hint = NULL)
{
    if(n < 0) return;
    
    float dn[3];
    norm(d, dn);
    //get our first orthonormal
    if(!hint) ortho(j3, dn, o);
    else ortho(hint, dn, o);
    if(cmpz(o)) neg(i3, o);
    if(on) norm(o, on);
    resize(o, r, o);
    
    //make the initial ring of four
    int i1 = 3 << n;
    int i2 = 6 << n;
    int i3 = 9 << n;
    cross(dn, o, o + i1);
    if(on) mov(o + i1, on + i1);
    resize(o + i1, r, o + i1);
    neg(o, o + i2);
    if(on) mov(o + i2, on + i2);
    resize(o + i2, r, o + i2);
    cross(dn, o + i2, o + i3);
    if(on) mov(o + i3, on + i3);
    resize(o + i3, r, o + i3);
    
    //now, to subdivide the ring for awesomeness
    int nv = 4 << n; //number of vertices in the array
    //first = interval >> 1
    //interval = nv >> (1 + i)
    //this for loop subdivides the ring n times, each time doubling #vertices
    //printf("n = %d\nnv = %d\n", n, nv);
    for(int i = nv >> 2; i > 1; i = i >> 1)
    {
        //io2 is the index of the first element we need to set.
        //each time we iterate, we fill in all the midpoints of
        //the points generated in the previous iteration.
        int io2 = i >> 1;
        int io2t3 = io2 * 3;
        int it3 = i * 3;
        for(int curr = io2; curr < nv * 3; curr += it3)
        {
            int next = (curr + io2t3) % (nv * 3);
            int prev = (curr - io2t3) % (nv * 3);
            //get an average of the two adjacent directions
            add(o + next, o + prev, o + curr);
            //normalize
            if(on)
            {
                norm(o + curr, on + curr);
                mul(on + curr, r, o + curr);
            }
            else resize(o + curr, r, o + curr);
        }
    }
}

//draws a ring as a set of vertices
//use glPointSize to adjust point size in pixels
inline void drawRing(const float* n, int d, float r)
{
    //the initial 4 vertices, doubled once for every subdivision,
    //with 3 floats per vertex
    int size = (4 << d) * 3;
    float o[size];
    mkring(n, d, r, o);
    glBegin(GL_POINTS);
    for(int i = 0; i < size; i += 3)
    {
        glVertex3fv(o + i);
    }
    glEnd();
}

//o must be the output of a call to mktube
//noffset is how far the norms are from the verts
inline void drawTube(const float* o, int d, int noffset = 0, float* colors = 0)
{
    //2 rings of 4 << d vertices, each with 3 floats
    int rsize = ringSize(d); //size of one ring's vertices in floats
    int tsize = rsize * 2; //size of the whole buffer for one tube
    
    const float* nm = noffset ? o + noffset : NULL;
    
    if(nm == o)
    {
        glBegin(GL_QUADS);
        //draw the quad that loops around the array
        glVertex3fv(o + tsize - 3);
        glVertex3fv(o + rsize - 3);
        glVertex3fv(o);
        glVertex3fv(o + rsize);
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glVertex3fv(o + i + rsize);
            glVertex3fv(o + i);
            glVertex3fv(o + i + 3);
            glVertex3fv(o + i + rsize + 3);
        }
        glEnd();
    }
    else if(colors)
    {
        glBegin(GL_QUADS);
        //0         rsize+0
        //1         rsize+1
        //READ IN ORDER
        //2         1
        //3         4
        //BUT FOR LOOPAROUND
        //rsize-1   tsize-1
        //0         rsize
        //READ IN ORDER
        //2         1
        //3         4
        glNormal3fv(nm + tsize - 3);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, colors + 4);
        glVertex3fv(o + tsize - 3);
        glNormal3fv(nm + rsize - 3);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, colors);
        glVertex3fv(o + rsize - 3);
        glNormal3fv(nm);
        glVertex3fv(o);
        glNormal3fv(nm + rsize);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, colors + 4);
        glVertex3fv(o + rsize);
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glNormal3fv(nm + i + rsize);
            glVertex3fv(o + i + rsize);
            glNormal3fv(nm + i);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, colors);
            glVertex3fv(o + i);
            glNormal3fv(nm + i + 3);
            glVertex3fv(o + i + 3);
            glNormal3fv(nm + i + rsize + 3);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, colors + 4);
            glVertex3fv(o + i + rsize + 3);
        }
        glEnd();
    }
    else
    {
        glBegin(GL_QUADS);
        glNormal3fv(nm + tsize - 3);
        glVertex3fv(o + tsize - 3);
        glNormal3fv(nm + rsize - 3);
        glVertex3fv(o + rsize - 3);
        glNormal3fv(nm);
        glVertex3fv(o);
        glNormal3fv(nm + rsize);
        glVertex3fv(o + rsize);
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glNormal3fv(nm + i + rsize);
            glVertex3fv(o + i + rsize);
            glNormal3fv(nm + i);
            glVertex3fv(o + i);
            glNormal3fv(nm + i + 3);
            glVertex3fv(o + i + 3);
            glNormal3fv(nm + i + rsize + 3);
            glVertex3fv(o + i + rsize + 3);
        }
        glEnd();
    }
}

//o is vertices, n is norms
inline void drawTube(const float* o, const float* n, int d, float* colors = 0)
{
    //2 rings of 4 << d vertices, each with 3 floats
    int rsize = ringSize(d); //size of one ring's vertices in floats
    int tsize = rsize * 2; //size of the whole buffer for one tube
    
    const float* nm = n;
    
    if(!nm)
    {
        glBegin(GL_QUADS);
        //draw the quad that loops around the array
        glVertex3fv(o + tsize - 3);
        glVertex3fv(o + rsize - 3);
        glVertex3fv(o);
        glVertex3fv(o + rsize);
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glVertex3fv(o + i + rsize);
            glVertex3fv(o + i);
            glVertex3fv(o + i + 3);
            glVertex3fv(o + i + rsize + 3);
        }
        glEnd();
    }
    else if(colors)
    {
        glBegin(GL_QUADS);
        //0         rsize + 0
        //1         rsize + 1
        //READ IN ORDER
        //2         1
        //3         4
        //BUT FOR LOOPAROUND
        //rsize-1   tsize-1
        //0         rsize
        //READ IN ORDER
        //2         1
        //3         4
        glNormal3fv(nm + tsize - 3);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, colors + 4);
        glVertex3fv(o + tsize - 3);
        glNormal3fv(nm + rsize - 3);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, colors);
        glVertex3fv(o + rsize - 3);
        glNormal3fv(nm);
        glVertex3fv(o);
        glNormal3fv(nm + rsize);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, colors + 4);
        glVertex3fv(o + rsize);
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glNormal3fv(nm + i + rsize);
            glVertex3fv(o + i + rsize);
            glNormal3fv(nm + i);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, colors);
            glVertex3fv(o + i);
            glNormal3fv(nm + i + 3);
            glVertex3fv(o + i + 3);
            glNormal3fv(nm + i + rsize + 3);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, colors + 4);
            glVertex3fv(o + i + rsize + 3);
        }
        glEnd();
    }
    else
    {
        glBegin(GL_QUADS);
        glNormal3fv(nm + tsize - 3);
        glVertex3fv(o + tsize - 3);
        glNormal3fv(nm + rsize - 3);
        glVertex3fv(o + rsize - 3);
        glNormal3fv(nm);
        glVertex3fv(o);
        glNormal3fv(nm + rsize);
        glVertex3fv(o + rsize);
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glNormal3fv(nm + i + rsize);
            glVertex3fv(o + i + rsize);
            glNormal3fv(nm + i);
            glVertex3fv(o + i);
            glNormal3fv(nm + i + 3);
            glVertex3fv(o + i + 3);
            glNormal3fv(nm + i + rsize + 3);
            glVertex3fv(o + i + rsize + 3);
        }
        glEnd();
    }
}

//builds and draws a tube from point A to point B, used only in unit tests
inline void drawTube(const float* a, const float* b, int d, float r)
{
    //our direction vector (everything else is based on this)
    float diff[3];
    float dn[3];
    sub(b, a, diff);
    norm(diff, dn);
    
    //2 rings of 4 << d vertices, each with 3 floats
    int rsize = 4 << d; //size of one ring in vertices
    int osizeo2 = 3 * rsize; //size of one ring's vertices in floats
    int osize = osizeo2 * 2; //size of the whole buffer
    float o[osize];
    mkring(dn, d, r, o);
    for(int i = 0; i < osizeo2; i += 3)
    {
        glColor4f(1.f, 0.f, 0.f, 1.f);
        o[i + osizeo2] = o[i] + b[0];
        glColor4f(0.f, 1.f, 0.f, 1.f);
        o[i] = o[i] + a[0];
        glColor4f(0.f, 0.f, 1.f, 1.f);
        o[i + osizeo2 + 1] = o[i + 1] + b[1];
        glColor4f(1.f, 1.f, 0.f, 1.f);
        o[i + 1] = o[i + 1] + a[1];
        glColor4f(0.f, 1.f, 1.f, 1.f);
        o[i + osizeo2 + 2] = o[i + 2] + b[2];
        glColor4f(1.f, 0.f, 1.f, 1.f);
        o[i + 2] = o[i + 2] + a[2];
    }
    
    drawTube(o, d);
}

//builds a cone whose point is at origin, skirt 45 degrees, -dir from tip
inline void mkcone(const float* dir, float r, int q, float* o, float* on = NULL)
{
    float d[3];
    mov(dir, d);
    //put our zero-vector (the tip) in the first vector of the vertex array
    mov(z3, o);
    int rsize = ringSize(q);
    int sz = rsize + 3;
    int nsz = rsize * 2;
    //fill out the rest of the vertex array with radial vectors
    mkring(d, q, r, o + 3);
    int ni = 0;
    //for each vector
    for(int i = 3; i < sz && ni < nsz; i+=3)
    {
        //place its normal in the correct location in the normal array
        if(on) norm(o + i, on + ni);
        //subtract the forward-vector of the cone to finalize
        sub(o + i, d, o + i);
        //get correct location in normal array for next normal
        ni += 6;
    }
    if(on)
    {
        //correct normals, get intermediates for tip normals
        //orthogonalize the first normal with respect to the first radial vertex
        ortho(on, o + 3, on);
        norm(on, on);
        int vi = 6;
        int i = 6;
        float mid[3];
        add(o + 3, o + sz - 3, mid); //something to orthogonalize against
        add(on, on + nsz - 6, on + nsz - 3); //something to orthogonalize
        ortho(on + nsz - 3, mid, on + nsz - 3); //orthogonalization
        norm(on + nsz - 3, on + nsz - 3);
        for(; i < nsz && vi < sz; i+=6)
        {
            add(o + vi, o + vi - 3, mid);
            add(on + i, on + i - 6, on + i - 3);
            ortho(on + i - 3, mid, on + i - 3);
            ortho(on + i, o + vi, on + i);
            norm(on + i - 3, on + i - 3);
            norm(on + i, on + i);
            
            vi += 3;
        }
    }
}

//draws a cone whose normals have been placed in n, vertices in v
inline void drawCone(const float* v, const float* n, int q, float* color = 0)
{
    if(n)
    {
        int rsz = ringSize(q);
        int sz = rsz + 3;
        int nsz = rsz * 2;
        int vi = 3;
        int i = 0;
        int ci = 0; //color-index for debug-coloring
        if(color) glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
        glBegin(GL_TRIANGLES);
        for(; i < nsz - 6 && vi < sz - 3; i+=6)
        {
            //two-step to the next triangle
            glNormal3fv(n + (i % nsz));
            glVertex3fv(v + (vi % sz));
            glNormal3fv(n + ((i + 3) % nsz));
            glVertex3fv(v);
            glNormal3fv(n + ((i + 6) % nsz));
            glVertex3fv(v + ((vi + 3) % sz));
            
            vi += 3;
            ci = (ci + 4) % 24;
        }
        //do the last face, needs special handling, as it loops around
        //glMaterialfv(GL_FRONT, GL_DIFFUSE, testcolors + ci);
        glNormal3fv(n + (i % nsz));
        glVertex3fv(v + (vi % sz));
        glNormal3fv(n + ((i + 3) % nsz));
        glVertex3fv(v);
        glNormal3fv(n + ((i + 6) % nsz));
        glVertex3fv(v + 3);
        glEnd();
    }
    else
    {
        //complete this once normals repaired
        if(color) glColor4fv(color);
        int sz = coneSize(q);
        glBegin(GL_TRIANGLE_FAN);
        for(int i = 0; i < sz; i+=3)
            glVertex3fv(v + i);
        glEnd();
    }
}

float minwidth(float extents[6])
{
    float mindiff = extents[1] - extents[0];
    float other = extents[3] - extents[2];
    if(mindiff > other) mindiff = other;
    other = extents[5] - extents[4];
    if(mindiff > other) mindiff = other;
    return mindiff;
}

}
// END ANONYMOUS NAMESPACE ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



//using false and NULL to represent things not currently filled in
//this is the basis of our (VERY) basic caching system
GLFlowRenderer::GLFlowRenderer()
{
    _p = GLFlowRenderer::Params();
    _changed = false;
    
    _copydata = NULL;
    _copycolor = NULL;
    _copysizes = NULL;
    _copycount = 0;
    
    _output = NULL;
    _osizes = NULL;
    _colors = NULL;
    _ocount = 0;
}

GLFlowRenderer::~GLFlowRenderer()
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    if(_osizes)
    {
        delete[] _osizes;
        _osizes = NULL;
    }
    if(_output)
    {
        for(int i = 0; i < _ocount; i++)
            delete[] _output[i];
        delete[] _output;
        _output = NULL;
    }
    if(_colors)
    {
        for(int i = 0; i < _ocount; i++)
            delete[] _colors[i];
        delete[] _colors;
        _colors = NULL;
    }
    if(_ocount) _ocount = 0;
}

//default values for basic params variables
GLFlowRenderer::Params::Params()
{
    _style = Tube;
    _extents[0] = -100.f;
    _extents[1] = 100.f;
    _extents[2] = -100.f;
    _extents[3] = 100.f;
    _extents[4] = -100.f;
    _extents[5] = 100.f;
    _baseColor[0] = 1.f;
    _baseColor[1] = 0.f;
    _baseColor[2] = 0.f;
    _baseColor[3] = 1.f;
    _radius = 1.f;
    _arrowRatio = 1.4f;
    _stride = 1;
    _quality = 0;
}

float GLFlowRenderer::Params::GetDefaultRadius()
{
    return minwidth(_extents) / 1000.f;
}

//assignment operator creates deep copy
void GLFlowRenderer::Params::operator=(GLFlowRenderer::Params params)
{
    for(int i = 0; i < 6; i++) _extents[i] = params._extents[i];
    for(int i = 0; i < 4; i++) _baseColor[i] = params._baseColor[i];
    _radius = params._radius;
    _arrowRatio = params._arrowRatio;
    _stride = params._stride;
    _quality = params._quality;
    _style = params._style;
}

//lots of bounds checking and validation, followed by assignment
bool GLFlowRenderer::SetParams(const GLFlowRenderer::Params *params)
{
    if(params->_extents[0] >= params->_extents[1]) return false;
    if(params->_extents[2] >= params->_extents[3]) return false;
    if(params->_extents[4] >= params->_extents[5]) return false;
    if(params->_baseColor[0] > 1.f || params->_baseColor[0] < 0.f) return false;
    if(params->_baseColor[1] > 1.f || params->_baseColor[1] < 0.f) return false;
    if(params->_baseColor[2] > 1.f || params->_baseColor[2] < 0.f) return false;
    if(params->_baseColor[3] > 1.f || params->_baseColor[3] < 0.f) return false;
    if(params->_radius < 0.f) return false;
    if(params->_arrowRatio < 0.f) return false;
    if(params->_stride < 1) return false;
    if(params->_quality < 0) return false;
    for(int i = 0; i < 6; i++) _p._extents[i] = params->_extents[i];
    for(int i = 0; i < 4; i++) _p._baseColor[i] = params->_baseColor[i];
    //ASSIGNMENT OPERATOR CANNOT ACT FROM CONST PARAMS
    _p._radius = params->_radius;
    _p._arrowRatio = params->_arrowRatio;
    _p._stride = params->_stride;
    _p._quality = params->_quality;
    _changed = true;
    return true;
}

const GLFlowRenderer::Params *GLFlowRenderer::GetParams() const
{
    return &_p;
}



GLHedgeHogger::GLHedgeHogger() : GLFlowRenderer()
{
    _hhp = GLHedgeHogger::Params();
}

//identical to parent class destructor
GLHedgeHogger::~GLHedgeHogger()
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    if(_osizes)
    {
        delete[] _osizes;
        _osizes = NULL;
    }
    if(_output)
    {
        for(int i = 0; i < _ocount; i++)
            delete[] _output[i];
        delete[] _output;
        _output = NULL;
    }
    if(_colors)
    {
        for(int i = 0; i < _ocount; i++)
            delete[] _colors[i];
        delete[] _colors;
        _colors = NULL;
    }
    if(_ocount) _ocount = 0;
}

GLHedgeHogger::Params::Params() : GLFlowRenderer::Params()
{
    _length = 1.f;
}

float GLHedgeHogger::Params::GetDefaultLength()
{
    return minwidth(_extents) / 100.f;
}

void GLHedgeHogger::Params::operator=(GLHedgeHogger::Params params)
{
    *((GLFlowRenderer::Params*)this) = (GLFlowRenderer::Params)params;
    this->_length = params._length;
}

//quick validation/assignment
bool GLHedgeHogger::SetParams(const GLHedgeHogger::Params *params)
{
    if(params->_length < 0.f) return false;
    bool success = ((GLFlowRenderer*)this)->SetParams(params);
    if(success) _hhp = *params;
    _changed = true;
    return success;
}

const GLHedgeHogger::Params *GLHedgeHogger::GetParams() const
{
    return &_hhp;
}



///////////////////////////////////////////////////////////////////////////////
// BEGIN ANONYMOUS NAMESPACE //////////////////////////////////////////////////
namespace
{
//builds hedgehogged tubes from given points, returns a chunk of memory
inline float* hhTubes(const float* v, int n, GLHedgeHogger::Params p)
{
    //build a series of rings from the given vectors
    float buf[3];
    int rnverts = 4 << p._quality; //vertices in a ring
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int rsize = tusize * n; //size of the rings-array verts section
    float radius = p._radius * p.GetDefaultRadius();
    float* r = new float[rsize * 2]; //rings-array
    float* nm = r + rsize; //normals section
    
    int itv = 0;
    int itr = 0;
    while(itr < rsize)
    {
        sub(v + itv + 3, v + itv, r + itr);
        mul(r + itr, p._length, buf);
        add(buf, v + itv, buf);
        mkring(r + itr, p._quality, radius, r + itr, nm + itr);
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            mov(nm + i, nm + i + rnsize);
            add(buf, r + i, r + i + rnsize);
            //add(v + itv + 3, r + i, r + i + rnsize);
            add(v + itv, r + i, r + i);
        }
        itv += 6 * p._stride;
        itr += tusize * p._stride;
    }
    
    return r; //let them eat cake
}

//builds hedgehogged tubes with colors from given points
//returns a chunk of memory containing that data, except color which is in 'co'
inline float* hhTubesC(const float* v, const float* ci, float** co, int n, GLHedgeHogger::Params p)
{
    //NOTE: n has already been divided by p.stride
    //I will fix this later...
    //build a series of rings from the given vectors
    float buf[3];
    int rnverts = 4 << p._quality; //vertices in a ring
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int rsize = tusize * n; //size of the rings-array
    float radius = p._radius * p.GetDefaultRadius();
    float* r = new float[rsize * 2]; //rings-array AND norms-array
    float* c = new float[4 * n];
    *co = c;
    float* nm = r + rsize; //normals
    
    int itv = 0;
    int itr = 0;
    int itci = 0;
    int itco = 0;
    while(itr < rsize)
    {
        sub(v + itv + 3, v + itv, r + itr);
        mul(r + itr, p._length, buf);
        add(buf, v + itv, buf);
        mkring(r + itr, p._quality, radius, r + itr, nm + itr);
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            mov(nm + i, nm + i + rnsize);
            add(buf, r + i, r + i + rnsize);
            //add(v + itv + 3, r + i, r + i + rnsize);
            add(v + itv, r + i, r + i);
        }
        
        c[itco    ] = ci[itci    ];
        c[itco + 1] = ci[itci + 1];
        c[itco + 2] = ci[itci + 2];
        c[itco + 3] = ci[itci + 3];
        
        itv += 6 * p._stride;
        itr += tusize * p._stride;
        itci += 4 * p._stride;
        itco += 4;
    }
    
    return r; //let them eat cake
}

//builds hedgehogged arrows and returns the chunk of memory
inline float* hhArrows(const float* v, int n, GLHedgeHogger::Params p)
{
    int rnverts = 4 << p._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int cosize = coverts * 3; //floats in a cone
    int narrows = n / p._stride; //number of tubes
    float radius = p._radius * p.GetDefaultRadius();
    int rsizet = narrows * tusize; //size of tube section
    int rsizec = narrows * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    int nsizec = narrows * rnsize * 2; //size of cone norm section
    int total = rsizet + nsizet + rsizec + nsizec;
    float* result = new float[total]; //result
    float* tubes = result; //location of tube vertices
    float* nmtube = tubes + rsizet; //location of tube normals
    float* cones = result + 2 * rsizet; //location of cone vertices
    float* nmcone = cones + rsizec; //location of cone normals
    float coneRadius = radius * p._arrowRatio;
    //tubes | tube norms | cones | cone norms
    int itr = 0; //used to read input positions
    int itw = 0; //used to write tube data
    int itn = 0; //used to copy deltas to cone section
    //code re-use from hhTubes! :D
    float buf[3]; //used to keep endpoint of tube
    //endpoint of tube must be before endpoint of cone
    while(itw < rsizet)
    {
        sub(v + itr + 3, v + itr, tubes + itw);
        float m = mag(tubes + itw); //total length of arrow
        resize(tubes + itw, m - radius, buf); //reducing tube section length
        add(buf, v + itr, buf); //placing tube endpoint
        resize(tubes + itw, coneRadius, cones + itn); //flip and resize
        mkring(tubes + itw, p._quality, radius, tubes + itw, nmtube + itw);
        for(int i = itw; i < itw + rnsize; i += 3)
        {
            mov(nmtube + i, nmtube + i + rnsize);
            add(buf, tubes + i, tubes + i + rnsize);
            add(v + itr, tubes + i, tubes + i);
        }
        itr += 6 * p._stride;
        itw += tusize;
        itn += cosize;
    }
    itr = 3; //used to read input positions
    itw = 0; //used to write cones
    itn = 0; //used to iterate through normals section
    while(itw < rsizec)
    {
        mkcone(cones + itw, radius * p._arrowRatio,
               p._quality, cones + itw, nmcone + itn);
        for(int i = 0; i < cosize; i += 3)
        {
            add(cones + itw + i, v + itr, cones + itw + i);
        }
        itr += 6 * p._stride;
        itw += cosize;
        itn += tusize; //tusize is also size of norms for cone
    }
    return result;
}

//builds hedgehogged arrows with color
//returns a chunk of memory, but colors are returned in 'co'
inline float* hhArrowsC(const float* v, const float* ci, float** co, int n, GLHedgeHogger::Params p)
{
    int rnverts = 4 << p._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int cosize = coverts * 3; //floats in a cone
    float radius = p._radius * p.GetDefaultRadius();
    int narrows = n / p._stride; //number of tubes
    int rsizet = narrows * tusize; //size of tube section
    int rsizec = narrows * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    int nsizec = narrows * rnsize * 2; //size of cone norm section
    int total = rsizet + nsizet + rsizec + nsizec;
    float* result = new float[total]; //result
    float* tubes = result; //location of tube vertices
    float* nmtube = tubes + rsizet; //location of tube normals
    float* cones = result + 2 * rsizet; //location of cone vertices
    float* nmcone = cones + rsizec; //location of cone normals
    float coneRadius = radius * p._arrowRatio;
    float* c = new float[4 * n];
    *co = c;
    //tubes | tube norms | cones | cone norms
    int itr = 0; //used to read input positions
    int itw = 0; //used to write tube data
    int itn = 0; //used to copy deltas to cone section
    int itci = 0;
    int itco = 0;
    //code re-use from hhTubes! :D
    float buf[3]; //used to keep endpoint of tube
    //endpoint of tube must be before endpoint of cone
    while(itw < rsizet)
    {
        sub(v + itr + 3, v + itr, tubes + itw);
        float m = mag(tubes + itw); //total length of arrow
        resize(tubes + itw, m - radius, buf); //reducing tube section length
        add(buf, v + itr, buf); //placing tube endpoint
        resize(tubes + itw, coneRadius, cones + itn); //flip and resize
        mkring(tubes + itw, p._quality, radius, tubes + itw, nmtube + itw);
        for(int i = itw; i < itw + rnsize; i += 3)
        {
            mov(nmtube + i, nmtube + i + rnsize);
            add(buf, tubes + i, tubes + i + rnsize);
            add(v + itr, tubes + i, tubes + i);
        }
        
        c[itco    ] = ci[itci    ];
        c[itco + 1] = ci[itci + 1];
        c[itco + 2] = ci[itci + 2];
        c[itco + 3] = ci[itci + 3];
        
        itr += 6 * p._stride;
        itw += tusize;
        itn += cosize;
        itci += 4 * p._stride;
        itco += 4;
    }
    itr = 3; //used to read input positions
    itw = 0; //used to write cones
    itn = 0; //used to iterate through normals section
    while(itw < rsizec)
    {
        mkcone(cones + itw, radius * p._arrowRatio,
               p._quality, cones + itw, nmcone + itn);
        for(int i = 0; i < cosize; i += 3)
        {
            add(cones + itw + i, v + itr, cones + itw + i);
        }
        itr += 6 * p._stride;
        itw += cosize;
        itn += tusize; //tusize is also size of norms for cone
    }
    return result;
}

//draws hedgehogged tubes as generated by hhTubes
inline void hhDrawTubes(const float* v, int n, GLHedgeHogger::Params hhp)
{
    int tsize = (8 << hhp._quality) * 3;
    int total = (n << (3 + hhp._quality)) * 3;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, hhp._baseColor);
    for(int i = 0; i < total; i += tsize * hhp._stride)
        drawTube(v + i, hhp._quality, total);
}

//draws hedgehogged tubes as generated by hhTubes, with colors
inline void hhDrawTubesC(const float* v, float* c, int n, GLHedgeHogger::Params hhp)
{
    int tsize = (8 << hhp._quality) * 3;
    int total = (n << (3 + hhp._quality)) * 3;
    int cidx = 0;
    int itr = 0;
    for(itr = 0; itr < total; itr += tsize * hhp._stride)
    {
        glMaterialfv(GL_FRONT, GL_DIFFUSE, c + cidx);
        drawTube(v + itr, hhp._quality, total);
        cidx += 4;
    }
}

//draws hedgehogged arrows as generated by hhArrows
inline void hhDrawArrows(const float* v, int n, GLHedgeHogger::Params hhp)
{
    //drawCone(const float* v, const float* n, int q)
    int rnverts = 4 << hhp._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int cosize = coverts * 3; //floats in a cone
    int narrows = n / hhp._stride; //number of tubes
    int rsizet = narrows * tusize; //size of tube section
    int rsizec = narrows * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    const float* cones = v + rsizet + nsizet;
    
    //code re-use from drawing for tubes! :D
    int tsize = (8 << hhp._quality) * 3;
    int total = (n << (3 + hhp._quality)) * 3;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, hhp._baseColor);
    for(int i = 0; i < total; i += tsize * hhp._stride)
        drawTube(v + i, hhp._quality, total);
    
    int itn = 0;
    for(int i = 0; i < rsizec; i += cosize)
    {
        drawCone(cones + i, cones + rsizec + itn, hhp._quality);
        itn += rnsize * 2;
    }
}

//draws hedgehogged arrows as generated by hhArrows, but with color
inline void hhDrawArrowsC(const float* v, float* c, int n, GLHedgeHogger::Params hhp)
{
    //drawCone(const float* v, const float* n, int q)
    int rnverts = 4 << hhp._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int cosize = coverts * 3; //floats in a cone
    int narrows = n / hhp._stride; //number of tubes
    int rsizet = narrows * tusize; //size of tube section
    int rsizec = narrows * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    const float* cones = v + rsizet + nsizet;
    
    //code re-use from drawing for tubes! :D
    int itc = 0;
    int tsize = (8 << hhp._quality) * 3;
    int total = (n << (3 + hhp._quality)) * 3;
    for(int i = 0; i < total; i += tsize * hhp._stride)
    {
        glMaterialfv(GL_FRONT, GL_DIFFUSE, c + itc);
        drawTube(v + i, hhp._quality, total);
        itc += 4;
    }
    
    int itn = 0;
    itc = 0;
    for(int i = 0; i < rsizec; i += cosize)
    {
        glMaterialfv(GL_FRONT, GL_DIFFUSE, c + itc);
        drawCone(cones + i, cones + rsizec + itn, hhp._quality);
        itn += rnsize * 2;
        itc += 4;
    }
}

//used to select from available drawing styles
inline void hhDraw(const float* v, int n, GLHedgeHogger::Params hhp)
{
    switch(hhp._style)
    {
        case GLFlowRenderer::Tube:
            hhDrawTubes(v, n, hhp);
            break;
        case GLFlowRenderer::Arrow:
            hhDrawArrows(v, n, hhp);
            break;
        default:
            //print an error message?
            break;
    }
}

//used to select from available styles when using color
inline void hhDrawC(const float* v, float* c, int n, GLHedgeHogger::Params hhp)
{
    switch(hhp._style)
    {
        case GLFlowRenderer::Tube:
            hhDrawTubesC(v, c, n, hhp);
            break;
        case GLFlowRenderer::Arrow:
            hhDrawArrowsC(v, c, n, hhp);
            break;
        default:
            //print an error message?
            break;
    }
}

}
// END ANONYMOUS NAMESPACE ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool GLHedgeHogger::SetData(const float **vecs, const int *sizes, int count)
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    
    _copycount = count;
    _copydata = new float*[_copycount];
    _copysizes = new int[_copycount];
    for(int i = 0; i < _copycount; i++)
    {
        _copysizes[i] = sizes[i];
        int datasz = _copysizes[i] * 6;
        _copydata[i] = new float[datasz];
        for(int j = 0; j < datasz; j+=6)
        {
            _copydata[i][j    ] = vecs[i][j    ];
            _copydata[i][j + 1] = vecs[i][j + 1];
            _copydata[i][j + 2] = vecs[i][j + 2];
            _copydata[i][j + 3] = vecs[i][j + 3];
            _copydata[i][j + 4] = vecs[i][j + 4];
            _copydata[i][j + 5] = vecs[i][j + 5];
        }
    }
    _changed = true;

    return false;
}

bool GLHedgeHogger::SetData(const float **vecs, const float **rgba, const int *sizes, int count)
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    
    _copycount = count;
    _copydata = new float*[_copycount];
    _copycolor = new float*[_copycount];
    _copysizes = new int[_copycount];
    for(int i = 0; i < _copycount; i++)
    {
        _copysizes[i] = sizes[i];
        int datasz = _copysizes[i] * 6;
        int colorsz = _copysizes[i] * 4;
        _copydata[i] = new float[datasz];
        _copycolor[i] = new float[colorsz];
        int jc = 0;
        for(int j = 0; j < _copysizes[i]; j+=6)
        {
            _copydata[i][j    ] = vecs[i][j    ];
            _copydata[i][j + 1] = vecs[i][j + 1];
            _copydata[i][j + 2] = vecs[i][j + 2];
            _copydata[i][j + 3] = vecs[i][j + 3];
            _copydata[i][j + 4] = vecs[i][j + 4];
            _copydata[i][j + 5] = vecs[i][j + 5];
            _copycolor[i][j    ] = rgba[i][j    ];
            _copycolor[i][j + 1] = rgba[i][j + 1];
            _copycolor[i][j + 2] = rgba[i][j + 2];
            _copycolor[i][j + 3] = rgba[i][j + 3];
            jc += 4;
        }
    }
    _changed = true;

    return false;
}

//draws provided data. generates geometry if necessary
void GLHedgeHogger::Draw()
{
    if(_copycolor)
    {
        if(_changed)
        {
            float* (*funcptr)(const float*, const float*, float**, int, GLHedgeHogger::Params) = _hhp._style == Tube ? hhTubesC : _hhp._style == Arrow ? hhArrowsC : NULL;
            if(_osizes)
            {
                delete[] _osizes;
                _osizes = NULL;
            }
            if(_output)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _output[i];
                delete[] _output;
                _output = NULL;
            }
            if(_colors)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _colors[i];
                delete[] _colors;
                _colors = NULL;
            }
            if(_ocount) _ocount = 0;
            _ocount = _copycount;
            _output = new float*[_ocount];
            _osizes = new int[_ocount];
            _colors = new float*[_ocount];
            for(int i = 0; i < _ocount; i++)
            {
                _osizes[i] = _copysizes[i];
                _output[i] = funcptr(_copydata[i], _copycolor[i], &_colors[i], _osizes[i], _hhp);
            }
            _changed = false;
        }
        for(int i = 0; i < _ocount; i++)
            hhDrawC(_output[i], _colors[i], _osizes[i], _hhp);
    }
    else
    {
        if(_changed)
        {
            float* (*funcptr)(const float*, int, GLHedgeHogger::Params) = _hhp._style == Tube ? hhTubes : _hhp._style == Arrow ? hhArrows : NULL;
            if(_osizes)
            {
                delete[] _osizes;
                _osizes = NULL;
            }
            if(_output)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _output[i];
                delete[] _output;
                _output = NULL;
            }
            if(_colors)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _colors[i];
                delete[] _colors;
                _colors = NULL;
            }
            if(_ocount) _ocount = 0;
            _ocount = _copycount;
            _output = new float*[_ocount];
            _osizes = new int[_ocount];
            for(int i = 0; i < _ocount; i++)
            {
                _osizes[i] = _copysizes[i];
                _output[i] = funcptr(_copydata[i], _osizes[i], _hhp);
            }
            _changed = false;
        }
        for(int i = 0; i < _ocount; i++)
            hhDraw(_output[i], _osizes[i], _hhp);
    }
}

GLPathRenderer::GLPathRenderer() : GLFlowRenderer()
{
    _prp = GLPathRenderer::Params();
}

GLPathRenderer::~GLPathRenderer()
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    if(_osizes)
    {
        delete[] _osizes;
        _osizes = NULL;
    }
    if(_output)
    {
        for(int i = 0; i < _ocount; i++)
            delete[] _output[i];
        delete[] _output;
        _output = NULL;
    }
    if(_colors)
    {
        for(int i = 0; i < _ocount; i++)
            delete[] _colors[i];
        delete[] _colors;
        _colors = NULL;
    }
    if(_ocount) _ocount = 0;
}

//default values
GLPathRenderer::Params::Params() : GLFlowRenderer::Params()
{
    _arrowStride = 3;
}

//deep copy
void GLPathRenderer::Params::operator=(GLPathRenderer::Params params)
{
    *((GLFlowRenderer::Params*)this) = (GLFlowRenderer::Params)params;
    _arrowStride = params._arrowStride;
}

//validation, deep copy assignment
bool GLPathRenderer::SetParams(const GLPathRenderer::Params *params)
{
    if(params->_arrowStride < 1) return false;
    bool success = ((GLFlowRenderer*)this)->SetParams(params);
    if(success) _prp = *params;
    _changed = true;
    return success;
}

const GLPathRenderer::Params *GLPathRenderer::GetParams() const
{
    return &_prp;
}



///////////////////////////////////////////////////////////////////////////////
// BEGIN ANONYMOUS NAMESPACE //////////////////////////////////////////////////
namespace
{
//builds a single path of tubes, returns a chunk of memory
inline float* prTubes(const float* v, int n, GLPathRenderer::Params p)
{
    n = n / p._stride;
    if(n < 2) return NULL;
    //build a series of rings from the given vectors
    int rnverts = 4 << p._quality; //vertices in a ring
    int rnsize = 3 * rnverts; //floats in a ring
    int rsize = rnsize * n; //size of the rings-array
    float radius = p._radius * p.GetDefaultRadius();
    float* r = new float[rsize * 2]; //rings-array
    float* nm = r + rsize; //normals
    int step = rnsize;
    int rlast = rsize - step;
    int stride = 3 * p._stride;
    //the first ring, which we will not iterate over
    //because we need adjacent vectors to calculate direction
    //for the other rings, whereas this just points in the direction
    //of the endpoint vector
    sub(v + stride, v, r + step);
    mkring(r + step, p._quality, radius, r, nm);
    for(int i = 0; i < step; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v, r + i, r + i);
    }

    int itv = stride;
    int rprev = 0;
    //start at idx = 1, continue until second-to-last
    for(int itr = step; itr < rlast; itr += step)
    {
        //get the current direction and pass it forward
        sub(v + itv + stride, v + itv, r + itr + step);
        //correct if rather large direction
        //calculated using current and previous direction
        //previous direction was passed forward by previous iteration
        add(r + itr, r + itr + step, r + itr);
        norm(r + itr, r + itr);
        //make a ring, in-place
        mkring(r + itr, p._quality, radius, r + itr, nm + itr, nm + rprev);
        //shift the ring into position, set normals
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            norm(r + i, r + rsize + i);
            add(v + itv, r + i, r + i);
        }
        itv += stride;
        rprev = itr;
    }
    
    //the last ring, calculated afterward to avoid overstepping bounds in loop
    sub(v + itv, v + itv - 3, r + rlast);
    mkring(r + rlast, p._quality, radius, r + rlast, nm + rlast, nm + rprev);
    for(int i = rlast; i < rsize; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v + itv, r + i, r + i);
    }

    return r;
}

//builds a path of tubes with color, returns a chunk of memory
inline float* prTubesC(const float* v, const float* ci, float** co, int n, GLPathRenderer::Params p)
{
    n = n / p._stride;
    if(n < 2) return NULL;
    //build a series of rings from the given vectors
    int rnverts = 4 << p._quality; //vertices in a ring
    int rnsize = 3 * rnverts; //floats in a ring
    int rsize = rnsize * n; //size of the rings-array
    float radius = p._radius * p.GetDefaultRadius();
    float* r = new float[rsize * 2]; //rings-array
    float* c = new float[4 * n];
    *co = c;
    float* nm = r + rsize; //normals
    int step = rnsize;
    int rlast = rsize - step;
    int stride = 3 * p._stride;
    //the first ring, which we will not iterate over
    //because we need adjacent vectors to calculate direction
    //for the other rings, whereas this just points in the direction
    //of the endpoint vector
    sub(v + stride, v, r + step);
    mkring(r + step, p._quality, radius, r, nm);
    for(int i = 0; i < step; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v, r + i, r + i);
    }
    c[0] = ci[0];
    c[1] = ci[1];
    c[2] = ci[2];
    c[3] = ci[3];

    int itv = stride;
    int rprev = 0;
    int itci = 4 * p._stride;
    int itco = 4;
    //start at idx = 1, continue until second-to-last
    for(int itr = step; itr < rlast; itr += step)
    {
        //get the current direction and pass it forward
        sub(v + itv + stride, v + itv, r + itr + step);
        //correct if rather large direction
        //calculated using current and previous direction
        //previous direction was passed forward by previous iteration
        add(r + itr, r + itr + step, r + itr);
        norm(r + itr, r + itr);
        //make a ring, in-place
        mkring(r + itr, p._quality, radius, r + itr, nm + itr, nm + rprev);
        //shift the ring into position, set normals
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            norm(r + i, r + rsize + i);
            add(v + itv, r + i, r + i);
        }
        
        c[itco    ] = ci[itci    ];
        c[itco + 1] = ci[itci + 1];
        c[itco + 2] = ci[itci + 2];
        c[itco + 3] = ci[itci + 3];
        
        itv += stride;
        rprev = itr;
        itci += 4 * p._stride;
        itco += 4;
    }
    
    //the last ring, calculated afterward to avoid overstepping bounds in loop
    sub(v + itv, v + itv - 3, r + rlast);
    mkring(r + rlast, p._quality, radius, r + rlast, nm + rlast, nm + rprev);
    for(int i = rlast; i < rsize; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v + itv, r + i, r + i);
    }

    c[itco    ] = ci[itci    ];
    c[itco + 1] = ci[itci + 1];
    c[itco + 2] = ci[itci + 2];
    c[itco + 3] = ci[itci + 3];

    return r;
}

//builds a path of arrows, returns a chunk of memory
float* prArrows(const float* v, int n, GLPathRenderer::Params p)
{
    if(n < 2) return NULL;
    int rnverts = 4 << p._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube, floats in a cone's norms
    int cosize = coverts * 3; //floats in a cone
    //number of rings in tubes
    //int nrings = (n / p.stride) + (n / (p.stride * p.arrowStride));
    int nrings = (((p._arrowStride + 1) * n) / (p._stride * p._arrowStride)) - 1;
    int ncones = nrings / p._arrowStride; //number of cones
    float radius = p._radius * p.GetDefaultRadius();
    float coneRadius = radius * p._arrowRatio;
    int arrowOffset = p._arrowStride - 1;
    //there is an extra ring for every arrow, to prevent tubes from
    //clipping through their cones
    int rsizet = nrings * rnsize; //size of tube section
    int rsizec = ncones * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    int nsizec = ncones * tusize; //size of cone norm section
    int total = rsizet + nsizet + rsizec + nsizec;
    float* result = new float[total];
    float* tubes = result;
    float* nmtube = tubes + rsizet;
    float* cones = nmtube + nsizet;
    float* nmcone = cones + rsizec;
    
    //build a series of rings from the given vectors
    //int rsize = rnsize * nrings; //size of the rings-array
    //float* nm = result + rsize; //normals
    int step = rnsize;
    int rlast = rsizet - step;
    int stride = 3 * p._stride;
    //the first ring, which we will not iterate over
    //because we need adjacent vectors to calculate direction
    //for the other rings, whereas this just points in the direction
    //of the endpoint vector
    sub(v + stride, v, tubes + step);
    mkring(tubes + step, p._quality, radius, tubes, nmtube);
    for(int i = 0; i < step; i+=3)
    {
        norm(tubes + i, nmtube + i);
        add(v, tubes + i, tubes + i);
    }

    int itr = step; //ring iterator
    int itv = 0; //input iterator
    int itc = 0; //cone iterator
    int itn = 0; //cone-normal iterator
    int rprev = 0;
    if(p._arrowStride == 1)
    {
        //get position for additional ring
        float m = mag(tubes + itr);
        mov(tubes + itr, tubes + itr + step);
        div(tubes + itr, m, tubes + itr);
        mul(tubes + itr, coneRadius, cones);
        mul(tubes + itr, m - radius, cones + 6);
        add(v + itv, cones + 6, cones + 3);
        mkring(tubes + itr, p._quality, radius, tubes + itr, nmtube + itr);
        for(int i = 0; i < rnsize; i += 3)
        {
            add(cones + 3, tubes + itr + i, tubes + itr + i);
        }
        //mul(cones + itc, coneRadius, cones + itc);
        mkcone(cones, coneRadius, p._quality, cones, nmcone);
        for(int i = 0 ; i < cosize; i += 3)
        {
            add(cones + i, v + itv + stride, cones + i);
        }
        rprev = itr;
        itr += step;
        itc += cosize;
        itn += tusize;
    }
    itv += stride;
    
    int count = 1;
    //start at idx = 1, continue until second-to-last
    while(itr < rlast)
    {
        //get the current direction and pass it forward
        sub(v + itv + stride, v + itv, result + itr + step);
        //correct if rather large direction
        //calculated using current and previous direction
        //previous direction was passed forward by previous iteration
        add(result + itr, result + itr + step, result + itr);
        norm(result + itr, result + itr);
        //make a ring, in-place
        mkring(result + itr, p._quality, radius, result + itr, nmtube + itr, nmtube + rprev);
        //shift the ring into position, set normals
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            norm(result + i, result + rsizet + i);
            add(v + itv, result + i, result + i);
        }
        rprev = itr;
        itr += step;
        
        if(count % p._arrowStride == arrowOffset || itr == rlast)
        {
            //get position for additional ring
            float m = mag(tubes + itr);
            mov(tubes + itr, tubes + itr + step);
            div(tubes + itr, m, tubes + itr);
            mul(tubes + itr, coneRadius, cones + itc);
            mul(tubes + itr, m - radius, cones + itc + 6);
            add(v + itv, cones + itc + 6, cones + itc + 3);
            mkring(tubes + itr, p._quality, radius, tubes + itr, nmtube + itr);
            for(int i = 0; i < rnsize; i += 3)
            {
                add(cones + itc + 3, tubes + itr + i, tubes + itr + i);
            }
            //mul(cones + itc, coneRadius, cones + itc);
            mkcone(cones + itc, coneRadius, p._quality, cones + itc, nmcone + itn);
            for(int i = 0 ; i < cosize; i += 3)
            {
                add(cones + itc + i, v + itv + stride, cones + itc + i);
            }
            rprev = itr;
            itr += step;
            itc += cosize;
            itn += tusize;
        }
        itv += stride;
        count++;
    }

    return result;
}

//builds a path of arrows with colors, returns a chunk of memory
inline float* prArrowsC(const float* v, const float* ci, float** co, int n, GLPathRenderer::Params p)
{
    if(n < 2) return NULL;
    int rnverts = 4 << p._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube, floats in a cone's norms
    int cosize = coverts * 3; //floats in a cone
    //number of rings in tubes
    //int nrings = (n / p.stride) + (n / (p.stride * p.arrowStride));
    int nrings = (((p._arrowStride + 1) * n) / (p._stride * p._arrowStride)) - 1;
    int ncones = nrings / p._arrowStride; //number of cones
    float radius = p._radius * p.GetDefaultRadius();
    float coneRadius = radius * p._arrowRatio;
    int arrowOffset = p._arrowStride - 1;
    //there is an extra ring for every arrow, to prevent tubes from
    //clipping through their cones
    int rsizet = nrings * rnsize; //size of tube section
    int rsizec = ncones * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    int nsizec = ncones * tusize; //size of cone norm section
    int total = rsizet + nsizet + rsizec + nsizec;
    float* result = new float[total];
    float* tubes = result;
    float* nmtube = tubes + rsizet;
    float* cones = nmtube + nsizet;
    float* nmcone = cones + rsizec;
    float* c = new float[n * 4];
    *co = c;
    
    //build a series of rings from the given vectors
    //int rsize = rnsize * nrings; //size of the rings-array
    //float* nm = result + rsize; //normals
    int step = rnsize;
    int rlast = rsizet - step;
    int stride = 3 * p._stride;
    int clast = (4 * n) - 4;
    //the first ring, which we will not iterate over
    //because we need adjacent vectors to calculate direction
    //for the other rings, whereas this just points in the direction
    //of the endpoint vector
    sub(v + stride, v, tubes + step);
    mkring(tubes + step, p._quality, radius, tubes, nmtube);
    for(int i = 0; i < step; i+=3)
    {
        norm(tubes + i, nmtube + i);
        add(v, tubes + i, tubes + i);
    }

    int itr = step; //ring iterator
    int itv = 0; //input iterator
    int itc = 0; //cone iterator
    int itn = 0; //cone-normal iterator
    int rprev = 0;
    if(p._arrowStride == 1)
    {
        //get position for additional ring
        float m = mag(tubes + itr);
        mov(tubes + itr, tubes + itr + step);
        div(tubes + itr, m, tubes + itr);
        mul(tubes + itr, coneRadius, cones);
        mul(tubes + itr, m - radius, cones + 6);
        add(v + itv, cones + 6, cones + 3);
        mkring(tubes + itr, p._quality, radius, tubes + itr, nmtube + itr);
        for(int i = 0; i < rnsize; i += 3)
        {
            add(cones + 3, tubes + itr + i, tubes + itr + i);
        }
        //mul(cones + itc, coneRadius, cones + itc);
        mkcone(cones, coneRadius, p._quality, cones, nmcone);
        for(int i = 0 ; i < cosize; i += 3)
        {
            add(cones + i, v + itv + stride, cones + i);
        }
        rprev = itr;
        itr += step;
        itc += cosize;
        itn += tusize;
    }
    itv += stride;
    
    c[0] = ci[0];
    c[1] = ci[1];
    c[2] = ci[2];
    c[3] = ci[3];
    
    int count = 1;
    int itci = 4 * p._stride;
    int itco = 4;
    //start at idx = 1, continue until second-to-last
    while(itr < rlast)
    {
        //get the current direction and pass it forward
        sub(v + itv + stride, v + itv, result + itr + step);
        //correct if rather large direction
        //calculated using current and previous direction
        //previous direction was passed forward by previous iteration
        add(result + itr, result + itr + step, result + itr);
        norm(result + itr, result + itr);
        //make a ring, in-place
        mkring(result + itr, p._quality, radius, result + itr, nmtube + itr, nmtube + rprev);
        //shift the ring into position, set normals
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            norm(result + i, result + rsizet + i);
            add(v + itv, result + i, result + i);
        }
        rprev = itr;
        itr += step;
        
        if(count % p._arrowStride == arrowOffset || itr == rlast)
        {
            //get position for additional ring
            float m = mag(tubes + itr);
            mov(tubes + itr, tubes + itr + step);
            div(tubes + itr, m, tubes + itr);
            mul(tubes + itr, coneRadius, cones + itc);
            mul(tubes + itr, m - radius, cones + itc + 6);
            add(v + itv, cones + itc + 6, cones + itc + 3);
            mkring(tubes + itr, p._quality, radius, tubes + itr, nmtube + itr);
            for(int i = 0; i < rnsize; i += 3)
            {
                add(cones + itc + 3, tubes + itr + i, tubes + itr + i);
            }
            //mul(cones + itc, coneRadius, cones + itc);
            mkcone(cones + itc, coneRadius, p._quality, cones + itc, nmcone + itn);
            for(int i = 0 ; i < cosize; i += 3)
            {
                add(cones + itc + i, v + itv + stride, cones + itc + i);
            }
            rprev = itr;
            itr += step;
            itc += cosize;
            itn += tusize;
        }
        
        c[itco    ] = ci[itci    ];
        c[itco + 1] = ci[itci + 1];
        c[itco + 2] = ci[itci + 2];
        c[itco + 3] = ci[itci + 3];
        
        itv += stride;
        count++;
        itci += 4 * p._stride;
        itco += 4;
    }
    
    c[itco    ] = ci[clast    ];
    c[itco + 1] = ci[clast + 1];
    c[itco + 2] = ci[clast + 2];
    c[itco + 3] = ci[clast + 3];

    return result;
}

//draws a path of tubes as built by prTubes
inline void prDrawTubes(const float* v, int n, GLPathRenderer::Params prp)
{
    int rsize = (4 << prp._quality) * 3;
    //int total = ((n << (2 + prp.quality)) * 3) - rsize;
    int total = rsize * n;
    //glColor4fv(prp.baseColor);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, prp._baseColor);
    for(int i = 0; i < total - rsize; i += rsize)
        drawTube(v + i, prp._quality, total);
}

//draws a path of tubes as built by prTubes, with color
inline void prDrawTubesC(const float* v, float* c, int n, GLPathRenderer::Params prp)
{
    int rsize = (4 << prp._quality) * 3;
    //int total = ((n << (2 + prp.quality)) * 3) - rsize;
    int total = rsize * n;
    //glColor4fv(prp.baseColor);
    int itc = 0;
    for(int i = 0; i < total - rsize; i += rsize)
    {
        drawTube(v + i, prp._quality, total, c + itc);
        itc += 4;
    }
}

//draws a path of arrows as built by prArrows
inline void prDrawArrows(const float* v, int n, GLPathRenderer::Params prp)
{
    int rnverts = 4 << prp._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube, floats in a cone's
    int cosize = coverts * 3; //floats in a cone
    int nrings = (((prp._arrowStride + 1) * n) / (prp._stride * prp._arrowStride)) - 1;
    int ncones = nrings / prp._arrowStride; //number of cones
    //there is an extra ring for every arrow, to prevent tubes from
    //clipping through their cones
    int rsizet = nrings * rnsize; //size of tube section
    int rsizec = ncones * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    const float* cones = v + rsizet + nsizet;
    const float* nmcone = cones + rsizec;
    int arrowOffset = prp._arrowStride - 1;
    int count = 0;
    int itc = 0;
    int itn = 0;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, prp._baseColor);
    for(int i = 0; i < nsizet - rnsize; i += rnsize)
    {
        drawTube(v + i, prp._quality, rsizet);
        if(count % prp._arrowStride == arrowOffset)
        {
            //increment counter, draw arrowhead!
            drawCone(cones + itc, nmcone + itn, prp._quality);
            i += rnsize;
            itc += cosize;
            itn += tusize;
        }
        count++;
    }
    drawCone(cones + itc, nmcone + itn, prp._quality);
}

//draws a path of arrows as built by prArrows, with color
inline void prDrawArrowsC(const float* v, float* c, int n, GLPathRenderer::Params prp)
{
    int rnverts = 4 << prp._quality; //vertices in a ring
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube, floats in a cone's
    int cosize = coverts * 3; //floats in a cone
    int nrings = (((prp._arrowStride + 1) * n) / (prp._stride * prp._arrowStride)) - 1;
    int ncones = nrings / prp._arrowStride; //number of cones
    //there is an extra ring for every arrow, to prevent tubes from
    //clipping through their cones
    int rsizet = nrings * rnsize; //size of tube section
    int rsizec = ncones * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    const float* cones = v + rsizet + nsizet;
    const float* nmcone = cones + rsizec;
    int arrowOffset = prp._arrowStride - 1;
    int count = 0;
    int itc = 0; //cone iterator
    int itn = 0;
    int itco = 0; //color iterator
    for(int i = 0; i < nsizet - rnsize; i += rnsize)
    {
        drawTube(v + i, prp._quality, rsizet, c + itco);
        itco += 4;
        if(count % prp._arrowStride == arrowOffset)
        {
            //increment counter, draw arrowhead!
            glMaterialfv(GL_FRONT, GL_DIFFUSE, c + itco);
            drawCone(cones + itc, nmcone + itn, prp._quality);
            i += rnsize;
            itc += cosize;
            itn += tusize;
        }
        count++;
    }
    drawCone(cones + itc, nmcone + itn, prp._quality);
}

//selects appropriate draw function based on draw style
inline void prDraw(const float* v, int n, GLPathRenderer::Params prp)
{
    switch(prp._style)
    {
        case GLFlowRenderer::Tube:
            prDrawTubes(v, n / prp._stride, prp);
            break;
        case GLFlowRenderer::Arrow:
            prDrawArrows(v, n, prp);
            break;
        default:
            //print an error message?
            break;
    }
}

//selects appropriate draw function, but from colored versions
inline void prDrawC(const float* v, float* c, int n, GLPathRenderer::Params p)
{
    switch(p._style)
    {
        case GLFlowRenderer::Tube:
            prDrawTubesC(v, c, n / p._stride, p);
            break;
        case GLFlowRenderer::Arrow:
            prDrawArrowsC(v, c, n, p);
            break;
        default:
            //print an error message?
            break;
    }
}

}
// END ANONYMOUS NAMESPACE ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


bool GLPathRenderer::SetData(const float **pts, const int *sizes, int count)
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    
    _copycount = count;
    _copydata = new float*[_copycount];
    _copysizes = new int[_copycount];
    for(int i = 0; i < _copycount; i++)
    {
        _copysizes[i] = sizes[i];
        int datasz = _copysizes[i] * 3;
        _copydata[i] = new float[datasz];
        for(int j = 0; j < datasz; j += 3)
        {
            _copydata[i][j    ] = pts[i][j    ];
            _copydata[i][j + 1] = pts[i][j + 1];
            _copydata[i][j + 2] = pts[i][j + 2];
        }
    }
    _changed = true;
    
    return false;
}

bool GLPathRenderer::SetData(const float **pts, const float **rgba, const int *sizes, int count)
{
    if(_copydata)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copydata[i];
        }
        delete[] _copydata;
        _copydata = NULL;
    }
    if(_copycolor)
    {
        for(int i = 0; i < _copycount; i++)
        {
            delete[] _copycolor[i];
        }
        delete[] _copycolor;
        _copycolor = NULL;
    }
    if(_copysizes)
    {
        delete[] _copysizes;
        _copysizes = NULL;
    }
    if(_copycount) _copycount = 0;
    
    _copycount = count;
    _copydata = new float*[_copycount];
    _copycolor = new float*[_copycount];
    _copysizes = new int[_copycount];
    for(int i = 0; i < _copycount; i++)
    {
        _copysizes[i] = sizes[i];
        int datasz = _copysizes[i] * 3;
        int colorsz = _copysizes[i] * 4;
        _copydata[i] = new float[datasz];
        _copycolor[i] = new float[colorsz];
        int jc = 0;
        for(int j = 0; j < datasz; j += 3)
        {
            _copydata[i][j    ] = pts[i][j    ];
            _copydata[i][j + 1] = pts[i][j + 1];
            _copydata[i][j + 2] = pts[i][j + 2];
            _copycolor[i][j    ] = rgba[i][j    ];
            _copycolor[i][j + 1] = rgba[i][j + 1];
            _copycolor[i][j + 2] = rgba[i][j + 2];
            _copycolor[i][j + 3] = rgba[i][j + 3];
            jc += 4;
        }
    }
    _changed = true;

    return false;
}

//builds geometry if necessary, draws geometry
void GLPathRenderer::Draw()
{
    if(_copycolor)
    {
        if(_changed)
        {
            float* (*funcptr)(const float*, const float*, float**, int, GLPathRenderer::Params) = _prp._style == Tube ? prTubesC : _prp._style == Arrow ? prArrowsC : NULL;
            if(_osizes)
            {
                delete[] _osizes;
                _osizes = NULL;
            }
            if(_output)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _output[i];
                delete[] _output;
                _output = NULL;
            }
            if(_colors)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _colors[i];
                delete[] _colors;
                _colors = NULL;
            }
            if(_ocount) _ocount = 0;
            
            _ocount = _copycount;
            _output = new float*[_ocount];
            _colors = new float*[_ocount];
            _osizes = new int[_ocount];
            for(int i = 0; i < _ocount; i++)
            {
                _osizes[i] = _copysizes[i];
                int csize = _osizes[i] * 4;
                _output[i] = funcptr(_copydata[i], _copycolor[i], _colors + i, _osizes[i], _prp);
            }
            _changed = false;
        }
        if(_copycolor)
            for(int i = 0; i < _ocount; i++)
                prDrawC(_output[i], _colors[i], _osizes[i], _prp);
    }
    else
    {
        if(_changed)
        {
            float* (*funcptr)(const float*, int, GLPathRenderer::Params) = _prp._style == Tube ? prTubes : _prp._style == Arrow ? prArrows : NULL;
            if(_osizes)
            {
                delete[] _osizes;
                _osizes = NULL;
            }
            if(_output)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _output[i];
                delete[] _output;
                _output = NULL;
            }
            if(_colors)
            {
                for(int i = 0; i < _ocount; i++)
                    delete[] _colors[i];
                delete[] _colors;
                _colors = NULL;
            }
            if(_ocount) _ocount = 0;
            
            _ocount = _copycount;
            _output = new float*[_ocount];
            _colors = new float*[_ocount];
            _osizes = new int[_ocount];
            for(int i = 0; i < _ocount; i++)
            {
                _osizes[i] = _copysizes[i];
                //HEISENBUG IS COMING FROM THIS LINE!!!!!!!!!!!!!!!!!
                //it would appear that both non-coloring funcs cause it
                _output[i] = funcptr(_copydata[i], _osizes[i], _prp);
            }
            _changed = false;
        }
        for(int i = 0; i < _ocount; i++)
            prDraw(_output[i], _osizes[i], _prp);
    }
}

