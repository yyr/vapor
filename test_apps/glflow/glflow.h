#ifndef GLFLOW_H
#define GLFLOW_H

/** \brief Subclasses of this can be used to draw various kinds of flow representations.

To use a FlowRenderer...\n
1) Construct it\n
2) Provide it with appropriate Params (some subclasses have their own Params)\n
3) Call "Draw" from a valid OpenGL context\n
*/
class GLFlowRenderer {
public:
    GLFlowRenderer();
    virtual ~GLFlowRenderer();

    ///some shape types that flow renderers can draw
    enum Style{Point, Arrow, Tube};

    ///used to parameterize FlowRenderers
    struct Params
    {
        Params();
        
        virtual void operator=(Params params);
        ///general appearance options
        ///the type of shape to draw
        Style style;
        ///the limits of the drawing domain
        ///used to estimate correct proportions
        ///takes the form (minX, maxX, minY, maxY, minZ, maxZ)
        float extents[6];
        ///the default color to use if none is specified
        ///format: (r, g, b, a)
        float baseColor[4];
        ///the multiplier for the radius of the shapes
        float radius;
        ///the ratio of an arrow's head's radius
        ///to that of the arrow itself
        ///a larger value results in larger arrowheads
        ///also scales the length of the arrowhead
        float arrowRatio;
        ///optimization options
        ///how many steps to skip when reading data
        int stride;
        ///how many times to subdivide shapes
        ///for smoother appearance
        int quality;
    };

    ///Used to get and set params before drawing
    ///Does some sanity checks on the incoming params, nothing more
    virtual bool SetParams(const Params *params);
    virtual const Params *GetParams() const;
    
    ///The meaning of X depends on the FlowRenderer subclass
    virtual void Draw(const float *X, int n) = 0;
    ///rgba is coloring options, but can also have different
    ///structure depending on FlowRenderer subclass
    ///rgba format: repeated (r, g, b, a) where each is between 0 and 1
    virtual void Draw(const float *X, const float *rgba, int n) = 0;
    
protected:
    Params p;
    bool changed;
    float* prevdata;
};

///used to draw hedgehog plots
class GLHedgeHogger : GLFlowRenderer
{
public:
    ///adds the 'length' parameter
    struct Params : public GLFlowRenderer::Params
    {
        Params();
        
        void operator=(Params params);
        ///A multiplier for vector lengths
        ///larger values result in longer vectors
        float length;
    };

    GLHedgeHogger();
    ~GLHedgeHogger();

    bool SetParams(const Params *params);
    const GLHedgeHogger::Params *GetParams() const;
    
    ///vectors: repeated (x, y, z, dx, dy, dz),
    ///    where (x, y, z) is the position of the vector,
    ///    and (dx, dy, dz) is the un-normalized direction
    ///n: the number of vectors in 'vectors'
    void Draw(const float *vectors, int n);
    ///rgba: one (r, g, b, a) for each vector in 'vectors'
    void Draw(const float *vectors, const float *rgba, int n);
    
protected:
    Params hhp;
};

///used to draw flow paths
class GLPathRenderer : GLFlowRenderer
{
public:
    ///adds the 'arrowstride' parameter
    struct Params : public GLFlowRenderer::Params
    {
        Params();
        
        void operator=(Params params);
        ///indicates how many strides should be taken
        ///between arrowheads
        int arrowstride;
    };

    GLPathRenderer();
    ~GLPathRenderer();
    
    bool SetParams(const Params *params);
    const GLPathRenderer::Params *GetParams() const;
    
    ///points: repeated (x, y, z) where each is a point in the path
    ///rgba: repeated (r, g, b, a), with one color for each point
    void Draw(const float *points, int n);
    void Draw(const float *points, const float *rgba, int n);
    
protected:
    Params prp;
};

#endif
