#include "glinc.h"
#include <cstdlib>
#include <cstdio>
#include "glflow.h"
#include <cmath>
#include "vec.h"
#include <iostream>
#include <cstring>
#include <ctime>
#ifndef WIN32
#include <sys/time.h>
#endif
using namespace std;

#include <vapor/OptionParser.h>
#include <vapor/CFuncs.h>
using namespace VetsUtil;

//possible conventions for the vector system I've created...
//suffix-ify means that the function outputs the something-ified version of
//its input, possibly with respect to other inputs.
//no suffix means that it should be obvious what the function does
//is-prefix means that the function performs and returns a boolean check
//f-prefix means that it is a faster (less precise) version of another function
//z-suffix means that the function operates using zero-vector or matrix
//all vector and vector-array outputs will be returned by last pointer argument
//all single-primitive outputs will be given by return value

struct TestOptions
{
    char* datafile;
    char* colorfile;
    char* style;
    int quality;
    float radius;
    int stride;
    int arrowstride;
    float ratio;
    float length;
    bool help;
    int prof;
} opt;

OptionParser::OptDescRec_T set_options[] = {
	{"help", 0, "", "Print this message and exit"},
	{"data", 1, "", "A test data source"},
	{"colors", 1, "", "A test color source"},
	{"style", 1, "tube", "Render mode: tube(s) | arrow(s) | line(s)"},
	{"radius", 1, "1.0", "Radius multiplier of rendered shapes"},
	{"quality", 1, "0", "Number of subdivisions of rendered shapes"},
	{"stride", 1, "1", "Traverse how many vertices between rendering?"},
	{"arrowstride", 1, "1", "How much to spread out arrowheads?"},
	{"ratio", 1, "2", "Ratio of arrow to tube radius"},
	{"length", 1, "1.0", "Arrow cone length"},
	{"profile", 1, "-1", "Profile, how many iterations?"},
	{NULL}
};

OptionParser::Option_T get_options[] = {
    {"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"data", VetsUtil::CvtToString, &opt.datafile, sizeof(opt.datafile)},
	{"colors", VetsUtil::CvtToString, &opt.colorfile, sizeof(opt.colorfile)},
	{"style", VetsUtil::CvtToString, &opt.style, sizeof(opt.style)},
	{"radius", VetsUtil::CvtToFloat, &opt.radius, sizeof(opt.radius)},
	{"quality", VetsUtil::CvtToInt, &opt.quality, sizeof(opt.quality)},
	{"stride", VetsUtil::CvtToInt, &opt.stride, sizeof(opt.stride)},
	{"arrowstride", VetsUtil::CvtToInt, &opt.arrowstride, sizeof(opt.arrowstride)},
	{"ratio", VetsUtil::CvtToFloat, &opt.ratio, sizeof(opt.ratio)},
	{"length", VetsUtil::CvtToFloat, &opt.length, sizeof(opt.length)},
	{"profile", VetsUtil::CvtToInt, &opt.prof, sizeof(opt.prof)},
	{NULL}
};

GLFlowRenderer::Style getStyle(char* instyle)
{
    if(!strcmp(instyle, "point") || !strcmp(instyle, "points"))
        return GLFlowRenderer::Point;
    else if(!strcmp(instyle, "tube") || !strcmp(instyle, "tubes"))
        return GLFlowRenderer::Tube;
    else return GLFlowRenderer::Arrow;
}

static float** getPaths(char* filename, int* size, int** sizes, float*** colors)
{
    /*
    npaths
    {
    x1 y1 z1 r1 g1 b1 a1
    x2 y2 z2 r2 g2 b2 a2
    ...
    }
    {
    x1 y1 z1 r1 g1 b1 a1
    x2 y2 z2 r1 g1 b1 a1
    ...
    }
    */
    
    FILE* file = fopen(filename, "r");
    float** paths;
    
    if(!file)
    {
        printf("failed to open %s\n", filename);
        return 0;
    }
    
    int npaths = 0;
    int found = fscanf(file, "%d", &npaths);
    if(!found) return 0;
    
    float** cdata = 0;
    *colors = new float*[npaths];
    cdata = *colors;
    
    paths = new float*[npaths];
    *sizes = new int[npaths];
    for(int i = 0; i < npaths; i++) paths[i] = 0;
    
    for(int i = 0; i < npaths; i++)
    {
        while(fgetc(file) != '{');
        long int pos = ftell(file);
        float buf[7];
        int count = 0;
        while(fscanf(file, "%f %f %f %f %f %f %f", buf, buf + 1, buf + 2, buf + 3, buf + 4, buf + 5, buf + 6) == 7) count++;
        fseek(file, pos, SEEK_SET);
        paths[i] = new float[count * 3];
        (*sizes)[i] = count;
        
        cdata[i] = new float[count * 4];
        
        int pidx = 0;
        int cidx = 0;
        while(fscanf(file, "%f %f %f %f %f %f %f",
                     paths[i] + pidx,
                     paths[i] + pidx + 1,
                     paths[i] + pidx + 2,
                     cdata[i] + cidx,
                     cdata[i] + cidx + 1,
                     cdata[i] + cidx + 2,
                     cdata[i] + cidx + 3) == 7)
        {
            pidx += 3;
            cidx += 4;
        }
    }
    
    *size = npaths;
    return paths;
}

static void drawCube();
bool manual = false;
int scrw, scrh, midx, midy;
double px, py, rx, ry;
double fov = 90.0;
float v_distance = 20.f;

double near_plane = 0.1;
double far_plane = 1000.0;

void init();
void display(double* profout = 0);

//glfw window event callbacks
void windowSize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)scrw/(double)scrh, near_plane, far_plane);
    glMatrixMode(GL_MODELVIEW);
    scrw = width;
    scrh = height;
    midx = width / 2;
    midy = height / 2;
    glfwGetCursorPos(window, &px, &py);
}

void windowRefresh(GLFWwindow* window)
{
#ifndef Darwin //avoid spurious redrawing
    display();
#endif
}

const int NEUTRAL = 0;
const int ROTATING = 1;
const int PANNING = 2;
const int ZOOMING = 3;
int mode = 0;

//glfw input callbacks
void cursorPos(GLFWwindow* window, double xpos, double ypos)
{
    double dx, dy;
    switch(mode)
    {
        case NEUTRAL:
            break;
        case ROTATING:
#ifdef Darwin
            dx = xpos - px;
            dy = ypos - py;
            px = xpos;
            py = ypos;
#else
            dx = xpos - (double)midx;
            dy = ypos - (double)midy;
            glfwSetCursorPos(window, midx, midy);
#endif
            rx += dx / 4.0;
            ry += dy / 4.0;
            if(ry > 90.0) ry = 90.0;
            if(ry < -90.0) ry = -90.0;
            if(rx > 180.0) rx -= 360.0;
            if(rx < -180.0) rx += 360.0;
            break;
        case PANNING:
            break;
        case ZOOMING:
#ifdef Darwin
            dx = xpos - px;
            dy = ypos - py;
            px = xpos;
            py = ypos;
#else
            dx = xpos - midx;
            dy = ypos - midy;
            glfwSetCursorPos(window, midx, midy);
#endif
            v_distance += dy / 20.0;
            if(v_distance < 0.0) v_distance = 0.0;
            break;
    }
}

//no keybinds defined yet
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

//use mouse button 1 to rotate, 2 to pan, middle to zoom
void mouseButton(GLFWwindow* window, int button, int action, int mods)
{
    int altmode;
    switch(button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
            altmode = ROTATING;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            altmode = ZOOMING;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            altmode = PANNING;
            break;
        default:
            break;
    }
    
    if(action == GLFW_PRESS)
    {
        if(mode == NEUTRAL)
        {
            mode = altmode;
#ifdef Darwin
            glfwGetCursorPos(window, &px, &py);
#else
            glfwSetCursorPos(window, midx, midy);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
        }
    }
    else
    {
        if(mode == altmode)
        {
            mode = NEUTRAL;
#ifndef Darwin
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif
        }
    }
}

//use scroll to zoom (in case no three-button mouse)
void mouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    v_distance -= yoffset;
    if(v_distance < 0.0) v_distance = 0.0;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)scrw/(double)scrh, near_plane, far_plane);
    glMatrixMode(GL_MODELVIEW);
}

void cursorEnter(GLFWwindow* window, int entered)
{
#ifndef Darwin
    mode = NEUTRAL;
#endif
}

GLfloat mat_specular[] = {0.f, 0.f, 0.f, 0.f};
GLfloat mat_shininess[] = {30.f};
GLfloat mat_color[] = {1.f, 0.f, 0.f};
GLfloat light_position[] = {0.f, 0.f, 0.f, 1.f};
GLfloat white_light[] = {1.f, 1.f, 1.f, 1.f};
GLfloat spot_direction[] = {0.f, 0.f, -1.f, 0.f};
GLfloat lmodel_ambient[] = {0.1f, 0.1f, 0.1f, 1.f};

float hogdata[54] = 
{
    -1.f, -.5f, -1.f,
    -1.f, .5f,  -1.f,
     0.f, -.5f, -1.f,
     0.f, .5f,  -1.f,
     1.f, -.5f, -1.f,
     1.f, .5f,  -1.f,

    -1.f, -.5f, 0.f,
    -1.f, .5f,  0.f,
     0.f, -.5f, 0.f,
     0.f, .5f,  0.f,
     1.f, -.5f, 0.f,
     1.f, .5f,  0.f,

    -1.f, -.5f, 1.f,
    -1.f, .5f,  1.f,
     0.f, -.5f, 1.f,
     0.f, .5f,  1.f,
     1.f, -.5f, 1.f,
     1.f, .5f,  1.f,
};

float hogcolors[36] =
{
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 0.f, 1.f, 1.f,
    
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 0.f, 1.f, 1.f,
    
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 0.f, 1.f, 1.f
};

float hogdata2[6] =
{
    0.f, -1.f, 0.f,
    0.f, 1.f, 0.f
};

float pathdata[33] =
{
    -1.f, -5.f, -1.f,
     1.f, -4.f, -1.f,
     1.f, -3.f,  1.f,
    -1.f, -2.f,  1.f,
    -1.f, -1.f, -1.f,
     1.f,  0.f, -1.f,
     1.f,  1.f,  1.f,
    -1.f,  2.f,  1.f,
    -1.f,  3.f, -1.f,
     1.f,  4.f, -1.f,
     1.f,  5.f,  1.f
};

float pathdata2[9] =
{
    0.f, -1.f, 0.f,
    0.f, 0.f, 0.f,
    0.f, 1.f, 0.f
};
float pathdata3[9] =
{
    0.f, -1.f, 0.f,
    0.f, 0.f, 0.f,
    1.f, 0.f, 0.f
};
float pathdata4[6] =
{
    0.f, -1.f, 0.f,
    0.f, 1.f, 0.f
};
float pathdata5[18] =
{
    -3.f, -1.f, 0.f,
    -2.f, 1.f, 0.f,
    -1.f, -1.f, -1.f,
    0.f, 1.f, 0.f,
    1.f, 2.f, 1.f,
    2.f, 3.f, 3.f
};

#define SPIRAL_SZ 400
#define SPIRAL_Q 4
#define RING_SZ (3<<(SPIRAL_Q+2))
#define SPIRAL_R 5.f
#define SPIRAL_L 20.f
#define SPIRAL_D (2*SPIRAL_L/SPIRAL_SZ)
#define SPIRAL_TSZ (3*SPIRAL_SZ)
#define SPIRAL_CSZ (4*SPIRAL_SZ)
float pathdata6[SPIRAL_TSZ]; //autogenerated spiral
float pathcolor6[SPIRAL_CSZ];

float** pathdata7 = NULL; //loaded path
float** pathcolor7 = NULL; //loaded colors
int pd7sz = 0; //number of paths in pd7
int* pd7szs = NULL; //number of vertices in each path in pd7

GLfloat conedir[] = {0.f, -1.f, 0.f};

GLHedgeHogger hog;
GLPathRenderer path;

float rot = 0.f;

bool paused = false;

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
    for(int i = nv >> 2; i >= 1; i = i >> 1)
    {
        //io2 is the index of the first element we need to set.
        //every additional index is offset from the previous by i
        int io2 = i >> 1;
        int io2t3 = io2 * 3;
        int it3 = i * 3;
        for(int curr = io2 * 3; curr < nv * 3; curr += it3)
        {
            int next = (curr + io2t3) % (nv * 3);
            int prev = (curr - io2t3) % (nv * 3);
            //get an average of the two adjacent directions
            add(o + next, o + prev, o + curr);
            //normalize and scale
            if(on)
            {
                norm(o + curr, on + curr);
                mul(on + curr, r, o + curr);
            }
            else resize(o + curr, r, o + curr);
        }
    }
}

void init(void)
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glShadeModel(GL_SMOOTH);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_color);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    /*
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.5f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.5f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.2f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.f);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_direction);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 2.f);
    */
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)scrw/(double)scrh, near_plane, far_plane);
    glMatrixMode(GL_MODELVIEW);
    
    GLFlowRenderer::Style style = getStyle(opt.style);

    hog = GLHedgeHogger();
    const GLHedgeHogger::Params* hparams = hog.GetParams();
    GLHedgeHogger::Params hcopy = *hparams;
    hcopy.radius = opt.radius;
    hcopy.quality = opt.quality;
    hcopy.baseColor[0] = 0.5f;
    hcopy.baseColor[1] = 0.5f;
    hcopy.baseColor[2] = 0.5f;
    hcopy.baseColor[3] = 1.f;
    hcopy.stride = opt.stride;
    hcopy.style = style;
    hcopy.arrowRatio = opt.ratio;

    hog.SetParams(&hcopy);
    
    path = GLPathRenderer();
    const GLPathRenderer::Params* pparams = path.GetParams();
    GLPathRenderer::Params pcopy = *pparams;
    pcopy.radius = opt.radius;
    pcopy.quality = opt.quality;
    pcopy.baseColor[0] = 0.9f;
    pcopy.baseColor[1] = 0.9f;
    pcopy.baseColor[2] = 0.0f;
    pcopy.baseColor[3] = 1.f;
    pcopy.stride = opt.stride;
    pcopy.style = style;
    pcopy.arrowRatio = opt.ratio;
    pcopy.arrowStride = opt.arrowstride;
    
    path.SetParams(&pcopy);
    
    float ring[RING_SZ];
    mkring(j3, SPIRAL_Q, SPIRAL_R, ring);
    for(int i = 0 ; i < SPIRAL_TSZ; i+=3)
    {
        pathdata6[i    ] = ring[i % RING_SZ];
        pathdata6[i + 1] = -SPIRAL_L + ((i / 3) * SPIRAL_D);
        pathdata6[i + 2] = ring[(i + 2) % RING_SZ];
    }
    srand(time(NULL));
    for(int i = 0; i < SPIRAL_CSZ; i++)
    {
        pathcolor6[i] = (float)(rand() % 1000000) / 1000000.f;
    }
}

static inline int ringSize(int q){return 3 << (2 + q);}
static inline int tubeSize(int q){return 2 * ringSize(q);}
static inline int coneSize(int q){return ringSize(q) + 3;}

float testcolors[24] = 
{
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 0.f, 1.f, 1.f,
    1.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 1.f, 1.f,
    1.f, 0.f, 1.f, 1.f
};

const int s = 9;
const int s2 = SPIRAL_SZ;
const float* thedata;
const float* thecolors;
const float* themoar;
const float** evenmoar;
const float** moarcolors;
const float* themcolors;

void display(double* profout)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glTranslatef(0.f, 0.f, -v_distance);
    //if(!paused) rot += (float)frameTime;
    //glRotatef((float) rot * 0.1f, 1.f, 1.f, 1.f);
    glRotatef((float) ry, 1.f, 0.f, 0.f);
    glRotatef((float) rx, 0.f, 1.f, 0.f);

    //glColor4f(1.f, 0.f, 0.f, 0.f);

    double t1;
    t1 = GetTime();
    //const int s = 9;
    const int s2 = SPIRAL_SZ;
    //hog.Draw(hogdata2, 1); //single tube
    //hog.Draw(&thedata, &thecolors, &s, 1); //regular array
    //path.Draw(pathdata2, 3); //90 degree, dual-segment
    //path.Draw(pathdata3, 3); //straight, dual-segment
    //path.Draw(pathdata4, 2); //single-segment
    //path.Draw(pathdata5, 6); //kink testing
    if(pathdata7) path.Draw(evenmoar, moarcolors, pd7szs, pd7sz);
    else path.Draw(&themoar, &themcolors, &s2, 1); //autospiral
    //use -radius 3.1355 at high quality for awesome autospiralness
    //glTranslatef(0.f, 0.f, -3.f);
    //coneTest(conedir, opt.quality, opt.radius);
    //drawCube();
    glFlush();
    double t2 = GetTime();
    if(profout) *profout = t2 - t1;
}

int main(int argc, char** argv)
{
    scrw = 800;
    scrh = 600;
    midx = 400;
    midy = 300;
    
    OptionParser op;
    if(op.AppendOptions(set_options) < 0)
    {
        fprintf(stderr, "ERR: %s\n", op.GetErrMsg());
        exit(EXIT_FAILURE);
    }
    if(op.ParseOptions(&argc, argv, get_options) < 0)
    {
        fprintf(stderr, "ERR: %s\n", op.GetErrMsg());
        exit(EXIT_FAILURE);
    }
	if (opt.help)
	{
		cerr << "Usage: test_glflow [options]" << endl;
		op.PrintOptionHelp(stderr);
		exit(EXIT_FAILURE);
	}
    if(strcmp(opt.datafile, ""))
        pathdata7 = getPaths("testdata", &pd7sz, &pd7szs, &pathcolor7);

    if(!glfwInit()) exit(EXIT_FAILURE);
    GLFWwindow* window = glfwCreateWindow(scrw, scrh, "test_glflow", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if(opt.prof < 0)
    {
        glfwSetWindowSizeCallback(window, &windowSize);
        glfwSetFramebufferSizeCallback(window, &windowSize);
        glfwSetWindowRefreshCallback(window, &windowRefresh);
        glfwSetKeyCallback(window, &keyboard);
        glfwSetMouseButtonCallback(window, &mouseButton);
        glfwSetCursorPosCallback(window, &cursorPos);
        glfwSetScrollCallback(window, &mouseScroll);
        glfwSetCursorEnterCallback(window, &cursorEnter);
    }
    
    glfwGetWindowSize(window, &scrw, &scrh);
    
    thedata = (const float*)hogdata;
    thecolors = (const float*)hogcolors;
    themoar = (const float*)pathdata6;
    themcolors = (const float*)pathcolor6;
    evenmoar = (const float**)pathdata7;
    moarcolors = (const float**)pathcolor7;
    
    init();
    if(opt.prof < 0)
    {
        while(!glfwWindowShouldClose(window))
        {
            display();
            glfwSwapBuffers(window);
            glfwWaitEvents(); //blocks until something happens
        }
    }
    else
    {
        printf("PROFILING...\n[ CONTROLS ARE DISABLED ]\n");
        double mean = 0.0;
        glfwSetTime(0.0);
        for(int i = 1; i <= opt.prof; i++)
        {
            double currtime = (double)i / 60.0;
            rx = 360.0 * cos(0.22 * currtime);
            ry = 70.0 * sin(1.5 * currtime);
            v_distance = 300.0 + (100.0 * sin(0.8 * currtime));
        
            double elapsed;
            display(&elapsed);
            double toAdd = (1.0 / (double)i) * (elapsed - mean);
            //printf("Diff is %f nanoseconds\n", toAdd);
            if(toAdd > -1000000.0 && toAdd < 1000000.0) mean += toAdd;
            
            glfwPollEvents(); //non-blocking
            glfwSwapBuffers(window);
        }
        double total = glfwGetTime();
        printf("Mean draw time is %f seconds\n", mean);
        printf("Total execution time is %f seconds\n", total);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

static void drawBox(GLfloat size, GLenum type)
{
    static GLfloat n[6][3] =
    {
        {-1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, -1.0, 0.0},
        {0.0, 0.0, 1.0},
        {0.0, 0.0, -1.0}
    };
    static GLint faces[6][4] =
    {
        {0, 1, 2, 3},
        {3, 2, 6, 7},
        {7, 6, 5, 4},
        {4, 5, 1, 0},
        {5, 6, 2, 1},
        {7, 4, 0, 3}
    };
    GLfloat v[8][3];
    GLint i;

    v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

    for (i = 5; i >= 0; i--)
    {
        glBegin(type);
            printf("NORM: %f, %f, %f\n", n[i][0], n[i][1], n[i][2]);
            glNormal3fv(&n[i][0]);
            printf("VERT: %f, %f, %f\n", v[faces[i][0]][0], v[faces[i][0]][1], v[faces[i][0]][2]);
            glVertex3fv(&v[faces[i][0]][0]);
            printf("VERT: %f, %f, %f\n", v[faces[i][1]][0], v[faces[i][1]][1], v[faces[i][1]][2]);
            glVertex3fv(&v[faces[i][1]][0]);
            printf("VERT: %f, %f, %f\n", v[faces[i][2]][0], v[faces[i][2]][1], v[faces[i][2]][2]);
            glVertex3fv(&v[faces[i][2]][0]);
            printf("VERT: %f, %f, %f\n", v[faces[i][3]][0], v[faces[i][3]][1], v[faces[i][3]][2]);
            glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
    }
}

void my_glutSolidCube(GLdouble size)
{
  drawBox(size, GL_QUADS);
}

// "I've half a mind to join a club and beat you over the head with it"
//
// silly, geeky names for characters
//   Tars Cuffs
//   Tars Xeffs
//   Remmie Slastar
//   Apta Geita
//   Resynic Rivap
//   Sesh Usaathost
//   Hashdt Binush
//   Listilde Edstop
//   Borj Dumpen
//   Lis Blokkus
//   Lis Pcidi
//   G'Debus
//   Sudot
//   Gecci
//   

