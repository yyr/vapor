#include "glinc.h"
#include <cstdlib>
#include <cstdio>
#include "glflow.h"
#include <cmath>
#include "vec.h"
#include <iostream>
#include <cstring>
using namespace std;

#include <vapor/OptionParser.h>
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
//

struct {
    char* datafile;
    char* colorfile;
    char* mode;
    int quality;
    float radius;
    int stride;
    float ratio;
    float length;
    bool help;
} opt;

OptionParser::OptDescRec_T set_options[] = {
	{"help", 0, "", "Print this message and exit"},
	{"data", 1, "", "A test data source"},
	{"colors", 1, "", "A test color source"},
	{"mode", 1, "tubes", "Render mode: tubes | arrows | lines"},
	{"radius", 1, "1.0", "Radius multiplier of rendered shapes"},
	{"quality", 1, "0", "Number of subdivisions of rendered shapes"},
	{"stride", 1, "1", "Traverse how many vertices between rendering?"},
	{"ratio", 1, "1.0", "Ratio of arrow to tube radius"},
	{"length", 1, "1.0", "Arrow cone length"},
	{NULL}
};

OptionParser::Option_T get_options[] = {
    {"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"data", VetsUtil::CvtToString, &opt.datafile, sizeof(opt.datafile)},
	{"colors", VetsUtil::CvtToString, &opt.colorfile, sizeof(opt.colorfile)},
	{"mode", VetsUtil::CvtToString, &opt.mode, sizeof(opt.mode)},
	{"radius", VetsUtil::CvtToFloat, &opt.radius, sizeof(opt.radius)},
	{"quality", VetsUtil::CvtToInt, &opt.quality, sizeof(opt.quality)},
	{"stride", VetsUtil::CvtToInt, &opt.stride, sizeof(opt.stride)},
	{"ratio", VetsUtil::CvtToFloat, &opt.ratio, sizeof(opt.ratio)},
	{"length", VetsUtil::CvtToFloat, &opt.length, sizeof(opt.length)},
	{NULL}
};

static int nfloats(char* filename)
{
    FILE* file = fopen(filename, "r");
    if(!file) return 0;
    int size;
    fscanf(file, "%u\n", &size);
    fclose(file);
    return size;
}
static int getfloats(char* filename, float* buff, int max)
{
    FILE* file = fopen(filename, "r");
    int size;
    fscanf(file, "%u\n", &size);
    int total = 0;
    for(int i = 0; i < size; i++)
    {
        int diff = fscanf(file, "%f", buff + i);
        if(diff == 0) break;
        total += diff;
    }
    fclose(file);
    
    return total;
}
static void drawCube();
int scrw, scrh, midx, midy;
double dx, dy, rx, ry;
double fov = 90.0;
float v_distance = 5.f;

//glfw window event callbacks
void windowSize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)scrw/(double)scrh, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    scrw = width;
    scrh = height;
    midx = width / 2;
    midy = height / 2;
}

const int NEUTRAL = 0;
const int ROTATING = 1;
const int PANNING = 2;
const int ZOOMING = 3;
int mode = 0;

//glfw input callbacks
void cursorPos(GLFWwindow* window, double xpos, double ypos)
{
    switch(mode)
    {
        case NEUTRAL:
            break;
        case ROTATING:
            dx = xpos - (double)midx;
            dy = ypos - (double)midy;
            rx += dx / 4.0;
            ry += dy / 4.0;
            if(ry > 90) ry = 90;
            if(ry < -90) ry = -90;
            if(rx > 180) rx -= 360;
            if(rx < -180) rx += 360;
            glfwSetCursorPos(window, midx, midy);
            break;
        case PANNING:
            break;
        case ZOOMING:
            dx = xpos - midx;
            dy = ypos - midy;
            v_distance += dy / 10.0;
            glfwSetCursorPos(window, midx, midy);
            break;
    }
}

//no keybinds defined yet
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    
}

//use mouse button 1 to rotate, 2 to pan, middle to zoom
void mouseButton(GLFWwindow* window, int button, int action, int mods)
{
    switch(button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
            if(action == GLFW_PRESS){if(mode == NEUTRAL) mode = ROTATING;
                                        glfwSetCursorPos(window, midx, midy);}
            else{if(mode == ROTATING) mode = NEUTRAL;}
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if(action == GLFW_PRESS){if(mode == NEUTRAL) mode = PANNING;
                                        glfwSetCursorPos(window, midx, midy);}
            else{if(mode == PANNING) mode = NEUTRAL;}
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            if(action == GLFW_PRESS){if(mode == NEUTRAL) mode = ZOOMING;
                                        glfwSetCursorPos(window, midx, midy);}
            else{if(mode == ZOOMING) mode = NEUTRAL;}
            break;
        default:
            break;
    }
}

//use scroll to zoom (in case no three-button mouse)
void mouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    v_distance -= yoffset;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)scrw/(double)scrh, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
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
float pathdata6[SPIRAL_TSZ]; //autogenerated spiral

float* pathdata7 = NULL; //loaded file
int pd7sz = 0; //size in floats of file

GLfloat conedir[] = {0.f, -1.f, 0.f};

GLHedgeHogger hog;
GLPathRenderer path;

float rot = 0.f;
double lastTime = 0.0;

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
    //printf("n = %d\nnv = %d\n", n, nv);
    for(int i = nv >> 2; i >= 1; i = i >> 1)
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
    gluPerspective(fov, (double)scrw/(double)scrh, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    hog = GLHedgeHogger();
    const GLHedgeHogger::Params* hparams = hog.GetParams();
    GLHedgeHogger::Params hcopy = *hparams;
    hcopy.radius = opt.radius;
    hcopy.quality = opt.quality;
    hcopy.baseColor[0] = .5f;
    hcopy.baseColor[1] = .5f;
    hcopy.baseColor[2] = .5f;
    hcopy.baseColor[3] = 1.f;
    hcopy.stride = opt.stride;

    hog.SetParams(&hcopy);
    
    path = GLPathRenderer();
    const GLPathRenderer::Params* pparams = path.GetParams();
    GLPathRenderer::Params pcopy = *pparams;
    pcopy.radius = opt.radius;
    pcopy.quality = opt.quality;
    pcopy.baseColor[0] = 1.f;
    pcopy.baseColor[1] = 0.f;
    pcopy.baseColor[2] = 0.f;
    pcopy.baseColor[3] = 1.f;
    pcopy.stride = opt.stride;
    
    path.SetParams(&pcopy);
    
    float ring[RING_SZ];
    mkring(j3, SPIRAL_Q, SPIRAL_R, ring);
    for(int i = 0 ; i < SPIRAL_TSZ; i+=3)
    {
        pathdata6[i    ] = ring[i % RING_SZ];
        pathdata6[i + 1] = -SPIRAL_L + ((i / 3) * SPIRAL_D);
        pathdata6[i + 2] = ring[(i + 2) % RING_SZ];
    }
}

static inline int ringSize(int q){return 3 << (2 + q);}
static inline int tubeSize(int q){return 2 * ringSize(q);}
static inline int coneSize(int q){return ringSize(q) + 3;}

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

float testcolors[24] = 
{
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 0.f, 1.f, 1.f,
    1.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 1.f, 1.f,
    1.f, 0.f, 1.f, 1.f
};

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

inline void coneTest(const float* b, int q, float r)
{
    int rnsz = ringSize(q);
    float* v = new float[rnsz + 3];
    float* n = new float[rnsz * 2];
    
    mkcone(b, r, q, v, n);
    drawCone(v, n, q);
    
    delete[] n;
    delete[] v;
}

void display(void)
{
    //in case we need dt or elapsed time
    double newTime = glfwGetTime();
    double frameTime = newTime - lastTime;
    lastTime = newTime;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glTranslatef(0.f, 0.f, -v_distance);
    //if(!paused) rot += (float)frameTime;
    //glRotatef((float) rot * 0.1f, 1.f, 1.f, 1.f);
    glRotatef((float) ry, 1.f, 0.f, 0.f);
    glRotatef((float) rx, 0.f, 1.f, 0.f);

    //glColor4f(1.f, 0.f, 0.f, 0.f);

    //hog.Draw(hogdata2, 1); //single tube
    //hog.Draw(hogdata, 9); //regular array
    //path.Draw(pathdata, 11); //spiral
    //path.Draw(pathdata2, 3); //90 degree, dual-segment
    //path.Draw(pathdata3, 3); //straight, dual-segment
    //path.Draw(pathdata4, 2); //single-segment
    //path.Draw(pathdata5, 6); //kink testing
    if(pathdata7) path.Draw(pathdata7, pd7sz / 3);
    else path.Draw(pathdata6, SPIRAL_SZ); //autospiral
    //glTranslatef(0.f, 0.f, -3.f);
    //coneTest(conedir, opt.quality, opt.radius);
    //drawCube();

    //glutSwapBuffers();
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
    {
        pd7sz = nfloats(opt.datafile);
        pathdata7 = new float[pd7sz];
        getfloats(opt.datafile, pathdata7, pd7sz);
    }

    if(!glfwInit()) exit(EXIT_FAILURE);
    GLFWwindow* window = glfwCreateWindow(scrw, scrh, "test_glflow", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, &windowSize);
    glfwSetKeyCallback(window, &keyboard);
    glfwSetMouseButtonCallback(window, &mouseButton);
    glfwSetCursorPosCallback(window, &cursorPos);
    glfwSetScrollCallback(window, &mouseScroll);
    
    glfwSetTime(0.0);
    
    init();
    while(!glfwWindowShouldClose(window))
    {
        display();
        
        glfwWaitEvents(); //blocks until something happens
        glfwSwapBuffers(window);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

/*
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(scrw, scrh);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    //glutIdleFunc(display);
    glutReshapeFunc(reshape);
    glutMotionFunc(mousemove);
    glutMouseFunc(mouse);
    glutMainLoop();
*/

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

static void drawCube()
{
    //glutSolidCube(1.f);
    //my_glutSolidCube(1.0);
    // Render a cube
    glBegin( GL_QUADS );
        // Front face
        glNormal3f(0.f, 0.f, -1.f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
    //glEnd();
    //glBegin( GL_QUADS );
        // Front face
        glNormal3f(0.f, 0.f, 1.f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
    //glEnd();
    //glBegin( GL_QUADS );
        glNormal3f(0.f, -1.f, 0.f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
    //glEnd();
    //glBegin( GL_QUADS );
        glNormal3f(0.f, 1.f, 0.f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
    //glEnd();
    //glBegin( GL_QUADS );
        glNormal3f(-1.f, 0.f, 0.f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
    //glEnd();
    //glBegin( GL_QUADS );
        glNormal3f(1.f, 0.f, 0.f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glVertex3f(0.5f, -0.5f, -0.5f);
    glEnd();
}

// "I've half a mind to join a club and beat you over the head with it"

