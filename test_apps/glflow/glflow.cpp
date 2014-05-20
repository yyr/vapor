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

static inline void mkcone(const float* dir, float r, int q, float* o, float* on = NULL)
{
    //put our zero-vector (the tip) in the first vector of the vertex array
    mov(z3, o);
    int rsize = ringSize(q);
    int sz = rsize + 3;
    int nsz = rsize * 2;
    //fill out the rest of the vertex array with radial vectors
    mkring(dir, q, r, o + 3);
    int ni = 0;
    //for each vector
    for(int i = 3; i < sz && ni < nsz; i+=3)
    {
        //place its normal in the correct location in the normal array
        if(on) norm(o + i, on + ni);
        //subtract the forward-vector of the cone to finalize
        sub(o + i, dir, o + i);
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
            //printf("ni = %d/%d\nvi = %d/%d\n", i, nsz, vi, sz);
            add(o + vi, o + vi - 3, mid);
            add(on + i, on + i - 6, on + i - 3);
            ortho(on + i - 3, mid, on + i - 3);
            ortho(on + i, o + vi, on + i);
            norm(on + i - 3, on + i - 3);
            norm(on + i, on + i);
            
            vi += 3;
        }
        //this is a really, really approximate solution for the missing normal
        //add(on, on + i - 6, on + i - 3);
        //norm(on + i - 3, on + i - 3);
    }
}

static inline void drawCone(const float* v, const float* n, int q)
{
    GLint oldmodel;
    if(n)
    {
        int rsz = ringSize(q);
        int sz = rsz + 3;
        int nsz = rsz * 2;
        int vi = 3;
        int i = 0;
        int ci = 0; //color-index for debug-coloring
        //printf("NEW CONE, sz = %d, nsz = %d\n", sz, nsz);
        glBegin(GL_TRIANGLES);
        for(; i < nsz - 6 && vi < sz - 3; i+=6)
        {
            //two-step to the next triangle
            //glMaterialfv(GL_FRONT, GL_DIFFUSE, testcolors + ci);
            glNormal3fv(n + (i % nsz));
            glVertex3fv(v + (vi % sz));
            glNormal3fv(n + ((i + 3) % nsz));
            glVertex3fv(v);
            glNormal3fv(n + ((i + 6) % nsz));
            glVertex3fv(v + ((vi + 3) % sz));
            /*
            printf("N %d %d %d\n", i % nsz, (i + 3) % nsz, (i + 6) % nsz);
            printvec(n + (i % nsz));
            printvec(n + ((i + 3) % nsz));
            printvec(n + ((i + 6) % nsz));
            printf("F %d %d %d\n", vi % sz, 0, (vi + 3) % sz);
            printvec(v + (vi % sz));
            printvec(v);
            printvec(v + (vi + 3) % sz);
            */
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
        /*
        printf("N %d %d %d\n", i % nsz, (i + 3) % nsz, (i + 6) % nsz);
        printvec(n + (i % nsz));
        printvec(n + ((i + 3) % nsz));
        printvec(n + ((i + 6) % nsz));
        printf("F %d %d %d\n", vi % sz, 0, 3);
        printvec(v + (vi % sz));
        printvec(v);
        printvec(v + 3);
        */
    }
    else
    {
        //complete this once normals repaired
        int sz = coneSize(q);
        glBegin(GL_TRIANGLE_FAN);
        for(int i = 0; i < sz; i+=3)
            glVertex3fv(v + i);
        glEnd();
    }
}



GLFlowRenderer::GLFlowRenderer()
{
    p = GLFlowRenderer::Params();
    changed = false;
    prevdata = 0;
}

GLFlowRenderer::~GLFlowRenderer()
{

}

GLFlowRenderer::Params::Params()
{
    style = Arrow;
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
    changed = true;
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
    changed = true;
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

static inline float* hhArrows(const float* v, int n, GLHedgeHogger::Params p)
{
    int rnverts = 4 << p.quality; //vertices in a ring
    int tuverts = rnverts * 2; //vertices in a tube
    int coverts = rnverts + 1; //vertices in a cone
    int rnsize = 3 * rnverts; //floats in a ring
    int tusize = rnsize * 2; //floats in a tube
    int cosize = coverts * 3; //floats in a cone
    int narrows = n / p.stride; //number of tubes
    int rsizet = narrows * tusize; //size of tube section
    int rsizec = narrows * cosize; //size of cone section
    int nsizet = rsizet; //size of tube norm section
    int nsizec = narrows * rnsize * 2; //size of cone norm section
    float* result = new float[rsizet + nsizet + rsizec + nsizec]; //result
    float* tubes = result; //location of tube vertices
    float* nmtube = tubes + rsizet; //location of tube normals
    float* cones = result + 2 * rsizet; //location of cone vertices
    float* nmcone = cones + rsizec; //location of cone normals
    float coneRadius = p.radius * p.arrowRatio;
    //tubes | tube norms | cones | cone norms
    int itr = 0; //used to read input positions
    int itw = 0; //used to write tube data
    int itn = 0; //used to copy deltas to cone section
    //code re-use from hhTubes! :D
    while(itw < rsizet)
    {
        sub(v + itr + 3, v + itr, tubes + itw);
        resize(tubes + itw, -2.f * coneRadius, cones + itn); //flip and resize
        mkring(tubes + itw, p.quality, p.radius, tubes + itw, nmtube + itw);
        for(int i = itw; i < itw + rnsize; i += 3)
        {
            mov(nmtube + i, nmtube + i + rnsize);
            add(v + itr + 3, tubes + i, tubes + i + rnsize);
            add(v + itr, tubes + i, tubes + i);
        }
        itr += 6 * p.stride;
        itw += tusize;
        itn += cosize;
    }
    itr = 3; //used to read input positions
    itw = 0; //used to write cones
    itn = 0; //used to iterate through normals section
    while(itw < rsizec)
    {
        mkcone(cones + itw, p.radius * p.arrowRatio,
               p.quality, cones + itw, nmcone + itn);
        for(int i = 0; i < cosize; i++)
        {
            add(cones + itw + i, v + itr + i, cones + itw + i);
        }
        itr += 6 * p.stride;
        itw += cosize;
    }
    return result;
}

void GLHedgeHogger::Draw(const float *v, int n)
{
    switch(hhp.style)
    {
        case Tube:
        {
            if(changed || v != prevdata)
            {
                if(prevdata != 0) delete[] prevdata;
                prevdata = hhTubes(v, n, hhp);
            }
            int tsize = (8 << p.quality) * 3;
            int total = (n << (3 + p.quality)) * 3;
            glMaterialfv(GL_FRONT, GL_DIFFUSE, hhp.baseColor);
            for(int i = 0; i < total; i += tsize * hhp.stride)
                drawTube(prevdata + i, p.quality, total);
        }
        break;
        case Arrow:
        {
            printf("reached arrow drawing section!\n");
            if(changed || v != prevdata)
            {
                if(prevdata != 0) delete[] prevdata;
                prevdata = hhArrows(v, n, hhp);
            }
            //drawCone(const float* v, const float* n, int q)
            int rnverts = 4 << p.quality; //vertices in a ring
            int tuverts = rnverts * 2; //vertices in a tube
            int coverts = rnverts + 1; //vertices in a cone
            int rnsize = 3 * rnverts; //floats in a ring
            int tusize = rnsize * 2; //floats in a tube
            int cosize = coverts * 3; //floats in a cone
            int narrows = n / p.stride; //number of tubes
            int rsizet = narrows * tusize; //size of tube section
            int rsizec = narrows * cosize; //size of cone section
            int nsizet = rsizet; //size of tube norm section
            int nsizec = narrows * rnsize * 2; //size of cone norm section
            float* cones = prevdata + rsizet + nsizet;
            
            //code re-use from drawing for tubes! :D
            int tsize = (8 << p.quality) * 3;
            int total = (n << (3 + p.quality)) * 3;
            glMaterialfv(GL_FRONT, GL_DIFFUSE, hhp.baseColor);
            for(int i = 0; i < total; i += tsize * hhp.stride)
                drawTube(prevdata + i, p.quality, total);
            
            int itn = 0;
            for(int i = 0; i < rsizec; i += cosize)
            {
                drawCone(cones + i, cones + rsizec + itn, p.quality);
                itn += rnsize * 2;
            }
        }
        break;
        case Point:
        {
        
        }
    }
}

void GLHedgeHogger::Draw(const float *vectors, const float *rgba, int n)
{

}



GLPathRenderer::GLPathRenderer() : GLFlowRenderer()
{
    prp = GLPathRenderer::Params();
    p.style = Tube;
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
    changed = true;
    return success;
}

const GLPathRenderer::Params *GLPathRenderer::GetParams() const
{
    return &prp;
}

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
    int step = rnsize;
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
        sub(v + itv + (3 * p.stride), v + itv, r + itr + step);
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
        itv += 3 * p.stride;
        rprev = itr;
    }
    
    //the last ring, calculated afterward to avoid overstepping bounds in loop
    sub(v + itv, v + itv - 3, r + rlast);
    mkring(r + rlast, p.quality, p.radius, r + rlast, nm + rlast, nm + rprev);
    for(int i = rlast; i < rsize; i+=3)
    {
        norm(r + i, r + rsize + i);
        add(v + itv, r + i, r + i);
    }

    return r;
}

static bool printed = false;
void GLPathRenderer::Draw(const float *v, int n)
{
    switch(p.style)
    {
        case Tube:
        {
            n = n / prp.stride;
            if(changed || prevdata != v)
            {
                if(prevdata != 0) delete[] prevdata;
                prevdata = prTubes(v, n, prp);
            }
            int rsize = (4 << prp.quality) * 3;
            //int total = ((n << (2 + prp.quality)) * 3) - rsize;
            int total = rsize * n;
            //glColor4fv(prp.baseColor);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, prp.baseColor);
            for(int i = 0; i < total - rsize; i += rsize)
            drawTube(prevdata + i, prp.quality, total);
        }
        break;
        case Arrow:
        {
        
        }
        break;
        case Point:
        {
        
        }
        break;
    }
}

void GLPathRenderer::Draw(const float *vectors, const float *rgba, int n)
{

}


