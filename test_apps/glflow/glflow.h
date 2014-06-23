#ifndef GLFLOW_H
#define GLFLOW_H

/** \brief Subclasses of this can be used to draw various kinds of flow representations.

To use a FlowRenderer...\n
1) Construct it\n
2) Provide it with appropriate Params (some subclasses have their own Params classes)\n
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
        virtual float GetDefaultRadius();
        ///general appearance options
        ///the type of shape to draw
        Style _style;
        ///the limits of the drawing domain
        ///used to estimate correct proportions
        ///takes the form (minX, maxX, minY, maxY, minZ, maxZ)
        float _extents[6];
        ///the default color to use if none is specified
        ///format: (r, g, b, a)
        float _baseColor[4];
        ///the multiplier for the radius of the shapes
        float _radius;
        ///the ratio of an arrow's head's radius
        ///to that of the arrow itself
        ///a larger value results in larger arrowheads
        ///also scales the length of the arrowhead
        float _arrowRatio;
        ///optimization options
        ///how many steps to skip when reading data
        int _stride;
        ///how many times to subdivide shapes
        ///for smoother appearance
        int _quality;
    };

    ///Used to get and set params before drawing
    ///Does some sanity checks on the incoming params, nothing more
    virtual bool SetParams(const Params *params);
    virtual const Params *GetParams() const;
    virtual bool SetData(const float **X, const int *sizes, int count) = 0;
    virtual bool SetData(const float **X, const float **rgba, const int *sizes, int count) = 0;
    
    ///The meaning of X depends on the FlowRenderer subclass
    virtual void Draw(const float **X, const int* sizes, int count) = 0;
    ///rgba is coloring options, but can also have different
    ///structure depending on FlowRenderer subclass
    ///rgba format: repeated (r, g, b, a) where each is between 0 and 1
    virtual void Draw(const float **X, const float **rgba, const int* sizes, int count) = 0;
    //virtual void Draw();
    
protected:
    Params _p;
    bool _changed;
    
    const float** _prevdata;
    
    float** _copydata;
    float** _copycolor;
    int* _copysizes;
    int _copycount;
    
    float** _output;
    float** _colors;
    int* _osizes;
    int _ocount;
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
        float GetDefaultLength();
        ///A multiplier for vector lengths
        ///larger values result in longer vectors
        float _length;
    };

    GLHedgeHogger();
    ~GLHedgeHogger();

    bool SetParams(const Params *params);
    const GLHedgeHogger::Params *GetParams() const;
    bool SetData(const float **vecs, const int *sizes, int count);
    bool SetData(const float **vecs, const float **rgba, const int *sizes, int count);
    
    ///vectors: repeated (x, y, z, dx, dy, dz),
    ///    where (x, y, z) is the position of the vector,
    ///    and (dx, dy, dz) is the un-normalized direction
    ///n: the number of vectors in 'vectors'
    void Draw(const float **vecs, const int *sizes, int count);
    ///rgba: one (r, g, b, a) for each vector in 'vectors'
    void Draw(const float **vecs, const float **rgba, const int *sizes, int count);
    //void Draw();
    
protected:
    Params _hhp;
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
        int _arrowStride;
    };

    GLPathRenderer();
    ~GLPathRenderer();
    
    bool SetParams(const Params *params);
    const GLPathRenderer::Params *GetParams() const;
    bool SetData(const float **pts, const int *sizes, int count);
    bool SetData(const float **pts, const float **rgba, const int *sizes, int count);
    
    ///points: repeated (x, y, z) where each is a point in the path
    ///rgba: repeated (r, g, b, a), with one color for each point
    void Draw(const float **pts, const int *sizes, int count);
    void Draw(const float **pts, const float **rgba, const int *sizes, int count);
    //void Draw();
    
protected:
    Params _prp;
};

#endif
