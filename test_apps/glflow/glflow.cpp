#include "glflow.h"

#include <cmath>
#include <cstdio>
#include "glinc.h"
#include "vec.h"

static inline int ringSize(int q){return 3 << (2 + q);}
static inline int tubeSize(int q){return 2 * ringSize(q);}
static inline int coneSize(int q){return ringSize(q) + 3;}

//create a ring with the given direction, radius, number of subdivisions
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
        //every additional index is offset from the previous by i
        int io2 = i >> 1;
        int io2t3 = io2 * 3;
        int it3 = i * 3;
        for(int c = io2; c < nv; c += i)
        {
            int curr = c * 3;
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

static inline void mkcone(const float* back, float r, int q, float* o, float* on = NULL)
{
    o[0] = 0;
    o[1] = 1;
    o[2] = 2;
    int rsize = 3 << (2 + q);
    int total = rsize + 3;
    mkring(back, q, r, o + 3);
    if(on)
    {
        //make normals and shift vertices in the same loop
        for(int i = 3; i < total; i+=3)
        {
            cross(back, o + i, on + i);
            add(o + i, back, o + i);
            cross(on + i, o + i, on + i);
        }
    }
    //just shift the vertices into place
    else for(int i = 3; i < total; i+=3) add(o + i, back, o + i);
}

static inline void drawRing(const float* n, int d, float r)
{
    //the initial 4 vertices, doubled once for every subdivision,
    //with 3 floats per vertex
    int size = (4 << d) * 3;
    float o[size];
    mkring(n, d, r, o);
    glPointSize(1.f);
    glBegin(GL_POINTS);
    for(int i = 0; i < size; i += 3)
    {
        glVertex3fv(o + i);
    }
    glEnd();
}

static inline void drawCone(const float* v, const float* n, int q)
{
    int sz = (3 << (2 + q)) + 3;
    glBegin(GL_TRIANGLE_FAN);
        glNormal3fv(n + 3);
        glVertex3fv(v);
        if(n)
        {
            for(int i = 3; i < sz; i+=3)
            {
                glNormal3fv(n + i);
                glVertex3fv(v + i);
            }
        }
        else
        {
            for(int i = 3; i < sz; i+=3)
                glVertex3fv(v + i);
        }
    glEnd();
}

inline void coneTest(const float* b, int q, float r)
{
    float* v = new float[coneSize(0)];
    float* n = new float[coneSize(0)];
    
    mkcone(b, r, q, v, n);
    drawCone(v, n, q);
    
    delete[] v;
    delete[] n;
}

static inline void drawTube(const float* o, int d, int noffset = 0)
{
    //2 rings of 4 << d vertices, each with 3 floats
    int rsize = ringSize(d); //size of one ring's vertices in floats
    int tsize = rsize * 2; //size of the whole buffer for one tube
    
    const float* nm = noffset ? o + noffset : NULL;
    
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
            glVertex3fv(o + i + rsize + 3);
            glVertex3fv(o + i + 3);
            glVertex3fv(o + i);
            glVertex3fv(o + i + rsize);
        }
        glEnd();
    }
    else
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
            //printf("N%d: ", (tsize - 3) / 3);
            //printvec(nm + tsize - 3);
        //glColor4f(1.f, 0.f, 0.f, 1.f);
        glVertex3fv(o + tsize - 3);
            //printf("V%d: ", (tsize - 3) / 3);
            //printvec(o + tsize - 3);
        glNormal3fv(nm + rsize - 3);
            //printf("N%d: ", (rsize - 3) / 3);
            //printvec(nm + rsize - 3);
        //glColor4f(0.f, 1.f, 0.f, 1.f);
        glVertex3fv(o + rsize - 3);
            //printf("V%d: ", (rsize - 3) / 3);
            //printvec(o + rsize - 3);
        glNormal3fv(nm);
            //printf("N%d: ", 0);
            //printvec(nm);
        //glColor4f(0.f, 0.f, 1.f, 1.f);
        glVertex3fv(o);
            //printf("V%d: ", 0);
            //printvec(o);
        glNormal3fv(nm + rsize);
            //printf("N%d: ", rsize / 3);
            //printvec(nm + rsize);
        //glColor4f(1.f, 1.f, 0.f, 1.f);
        glVertex3fv(o + rsize);
            //printf("V%d: ", rsize / 3);
            //printvec(o + (rsize / 3));
        for(int i = 0; i < rsize - 3; i += 3)
        {
            glNormal3fv(nm + i + rsize);
                //printf("N%d: ", (i + rsize) / 3);
                //printvec(nm + i + rsize);
            //glColor4f(1.f, 0.f, 0.f, 1.f);
            glVertex3fv(o + i + rsize);
                //printf("V%d: ", (i + rsize) / 3);
                //printvec(o + i + rsize);
            glNormal3fv(nm + i);
                //printf("N%d: ", i / 3);
                //printvec(nm + i);
            //glColor4f(0.f, 1.f, 0.f, 1.f);
            glVertex3fv(o + i);
                //printf("V%d: ", i / 3);
                //printvec(o + i);
            glNormal3fv(nm + i + 3);
                //printf("N%d: ", (i + 3) / 3);
                //printvec(nm + i + 3);
            //glColor4f(0.f, 0.f, 1.f, 1.f);
            glVertex3fv(o + i + 3);
                //printf("V%d: ", (i + 3) / 3);
                //printvec(o + i + 3);
            glNormal3fv(nm + i + rsize + 3);
                //printf("N%d: ", (i + rsize + 3) / 3);
                //printvec(nm + i + rsize + 3);
            //glColor4f(1.f, 1.f, 0.f, 1.f);
            glVertex3fv(o + i + rsize + 3);
                //printf("V%d: ", (i + rsize + 3) / 3);
                //printvec(o + i + rsize + 3);
        }
        glEnd();
    }
}

//builds and draws a tube from point A to point B
//uses available information from params
static inline void drawTube(const float* a, const float* b, int d, float r)
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



GLFlowRenderer::GLFlowRenderer()
{
    p = GLFlowRenderer::Params();
}

GLFlowRenderer::~GLFlowRenderer()
{

}

GLFlowRenderer::Params::Params()
{
    style = GLFlowRenderer::Arrow;
    extents[0] = -100.f;
    extents[1] = 100.f;
    extents[2] = -100.f;
    extents[3] = 100.f;
    extents[4] = -100.f;
    extents[5] = 100.f;
    baseColor[0] = 1.f;
    baseColor[1] = 0.f;
    baseColor[2] = 0.f;
    baseColor[3] = 1.f;
    radius = 1.f;
    arrowRatio = 1.4f;
    stride = 1;
    quality = 0;
}

void GLFlowRenderer::Params::operator=(GLFlowRenderer::Params params)
{
    for(int i = 0; i < 6; i++) extents[i] = params.extents[i];
    for(int i = 0; i < 4; i++) baseColor[i] = params.baseColor[i];
    radius = params.radius;
    arrowRatio = params.arrowRatio;
    stride = params.stride;
    quality = params.quality;
}

bool GLFlowRenderer::SetParams(const GLFlowRenderer::Params *params)
{
    if(params->extents[0] >= params->extents[1]) return false;
    if(params->extents[2] >= params->extents[3]) return false;
    if(params->extents[4] >= params->extents[5]) return false;
    if(params->baseColor[0] > 1.f || params->baseColor[0] < 0.f) return false;
    if(params->baseColor[1] > 1.f || params->baseColor[1] < 0.f) return false;
    if(params->baseColor[2] > 1.f || params->baseColor[2] < 0.f) return false;
    if(params->baseColor[3] > 1.f || params->baseColor[3] < 0.f) return false;
    if(params->radius < 0.f) return false;
    if(params->arrowRatio < 0.f) return false;
    if(params->stride < 1) return false;
    if(params->quality < 0) return false;
    for(int i = 0; i < 6; i++) p.extents[i] = params->extents[i];
    for(int i = 0; i < 4; i++) p.baseColor[i] = params->baseColor[i];
    p.radius = params->radius;
    p.arrowRatio = params->arrowRatio;
    p.stride = params->stride;
    p.quality = params->quality;
    return true;
}

const GLFlowRenderer::Params *GLFlowRenderer::GetParams() const
{
    return &p;
}



GLHedgeHogger::GLHedgeHogger() : GLFlowRenderer()
{
    hhp = GLHedgeHogger::Params();
}

GLHedgeHogger::~GLHedgeHogger()
{

}

GLHedgeHogger::Params::Params() : GLFlowRenderer::Params()
{
    length = 1.f;
}

void GLHedgeHogger::Params::operator=(GLHedgeHogger::Params params)
{
    *((GLFlowRenderer::Params*)this) = (GLFlowRenderer::Params)params;
    this->length = params.length;
}

bool GLHedgeHogger::SetParams(const GLHedgeHogger::Params *params)
{
    if(params->length < 0.f) return false;
    bool success = ((GLFlowRenderer*)this)->SetParams(params);
    if(success) hhp = *params;
    return success;
}

const GLHedgeHogger::Params *GLHedgeHogger::GetParams() const
{
    return &hhp;
}

static inline float* hhTubes(const float* v, int n, GLHedgeHogger::Params p)
{
    //build a series of rings from the given vectors
    int rnverts = 4 << p.quality; //vertices in a ring
    int tuverts = rnverts * 2; //vertices in a tube
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int rsize = tusize * n; //size of the rings-array
    float* r = new float[rsize * 2]; //rings-array
    float* nm = r + rsize; //normals
    
    int itv = 0;
    int itr = 0;
    while(itr < rsize)
    {
        sub(v + itv + 3, v + itv, r + itr);
        mkring(r + itr, p.quality, p.radius, r + itr, nm + itr);
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            mov(nm + i, nm + i + rnsize);
            add(v + itv + 3, r + i, r + i + rnsize);
            add(v + itv, r + i, r + i);
        }
        itv += 6 * p.stride;
        itr += tusize * p.stride;
    }
    
    return r; //let them eat cake
}

void GLHedgeHogger::Draw(const float *v, int n) const
{
    float* rings = hhTubes(v, n, hhp);
    int tsize = (8 << p.quality) * 3;
    int total = (n << (3 + p.quality)) * 3;
    //glColor4fv(hhp.baseColor);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, hhp.baseColor);
    for(int i = 0; i < total; i += tsize * hhp.stride)
        drawTube(rings + i, p.quality, total);
    delete[] rings; //later I'll come back and fix this so it caches
}

void GLHedgeHogger::Draw(const float *vectors, const float *rgba, int n) const
{

}



GLPathRenderer::GLPathRenderer() : GLFlowRenderer()
{
    prp = GLPathRenderer::Params();
}

GLPathRenderer::~GLPathRenderer()
{

}

GLPathRenderer::Params::Params() : GLFlowRenderer::Params()
{
    arrowstride = 3;
}

void GLPathRenderer::Params::operator=(GLPathRenderer::Params params)
{
    *((GLFlowRenderer::Params*)this) = (GLFlowRenderer::Params)params;
    this->arrowstride = params.arrowstride;
}

bool GLPathRenderer::SetParams(const GLPathRenderer::Params *params)
{
    if(params->arrowstride < 1) return false;
    bool success = ((GLFlowRenderer*)this)->SetParams(params);
    if(success) prp = *params;
    return success;
}

const GLPathRenderer::Params *GLPathRenderer::GetParams() const
{
    return &prp;
}

//TODO: REWRITE
static inline float* prTubes(const float* v, int n, GLPathRenderer::Params p)
{
    if(n < 2) return NULL;
    //build a series of rings from the given vectors
    int rnverts = 4 << p.quality; //vertices in a ring
    int rnsize = 3 * rnverts; //floats in a ring
    int rsize = rnsize * n; //size of the rings-array
    float* r = new float[rsize * 2]; //rings-array
    float* nm = r + rsize; //normals
    int vsize = n * 3;
    int vlast = vsize - 3;
    int step = rnsize * p.stride;
    int rlast = rsize - step;
    //the first ring, which we will not iterate over
    //because we need adjacent vectors to calculate direction
    //for the other rings, whereas this just points in the direction
    //of the endpoint vector
    sub(v + 3, v, r + step);
    mkring(r + step, p.quality, p.radius, r, nm);
    for(int i = 0; i < step; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v, r + i, r + i);
    }

    int itv = 3;
    int rprev = 0;
    //start at idx = 1, continue until second-to-last
    for(int itr = step; itr < rlast; itr += step)
    {
        //get the current direction and pass it forward
        sub(v + itv + 3, v + itv, r + itr + step);
        //correct if rather large direction
        //calculated using current and previous direction
        //previous direction was passed forward by previous iteration
        add(r + itr, r + itr + step, r + itr);
        norm(r + itr, r + itr);
        //make a ring, in-place
        mkring(r + itr, p.quality, p.radius, r + itr, nm + itr, nm + rprev);
        //shift the ring into position, set normals
        for(int i = itr; i < itr + rnsize; i += 3)
        {
            norm(r + i, r + rsize + i);
            add(v + itv, r + i, r + i);
        }
        itv += 3;
        rprev = itr;
    }
    
    //the last ring, calculated afterward to avoid being overwritten by loop
    sub(v + vlast, v + vlast - 3, r + rlast);
    mkring(r + rlast, p.quality, p.radius, r + rlast, nm + rlast, nm + rprev);
    for(int i = rlast; i < rsize; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v + vlast, r + i, r + i);
    }

    return r;
}

static bool printed = false;
void GLPathRenderer::Draw(const float *v, int n) const
{
    float* rings = prTubes(v, n, prp);
    int rsize = (4 << prp.quality) * 3;
    //int total = ((n << (2 + prp.quality)) * 3) - rsize;
    int total = rsize * n;
    //glColor4fv(prp.baseColor);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, prp.baseColor);
    for(int i = 0; i < total - rsize; i += rsize * prp.stride)
        drawTube(rings + i, prp.quality, total);
        
    /*
    if(!printed)
    {
        printed = true;
        int mid = total;
        int end = mid << 1;
        for(int i = 0; i < mid; i+=3)
        {
            printf("(%5f, %5f, %5f)\n", rings[i], rings[i + 1], rings[i + 2]);
        }
        printf("\n");
        for(int i = mid; i < end; i+=3)
        {
            printf("(%5f, %5f, %5f)\n", rings[i], rings[i + 1], rings[i + 2]);
        }
    }
    */
    
    delete[] rings;
}

void GLPathRenderer::Draw(const float *vectors, const float *rgba, int n) const
{

}


