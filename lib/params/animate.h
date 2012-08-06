//
//  animate.h
//  VaporAnimation
//
//  Created by Ashish Dhital on 6/14/12.
//  Copyright (c) 2012 NCAR. All rights reserved.
//

#ifndef animate_h
#define animate_h

#include "glutil.h"
#include "animationparams.h"
#include "viewpoint.h"

#include <vector>
namespace VAPoR {
class PARAMS_API animate {
    
  private:
    
    //for slopes
    struct slope{
        float cam[3];
        float view[3];
        float up[3];
        float zoom;
        float rot[3];
        float quat[3];
                
    };
    
    //temporary storage
    struct vector_3D{
        float x;
        float y;
        float z;  
    };
 

	//struct array for slopes
	slope * slopes;

	//quaternions
	float  quat1[3]; float quat2[3]; 
	int noVPs ; //no of input view points
	int testPoints;//the no of test points in between

	float *zoom;

	// total segments between each frames

	int *totalSegments;

	 //for distance approximation and speed control 
	 vector_3D *approx_camPos;
	 float *distance;
	 float incrementFactor;


  public:

    //for out viewpoints
    std::vector <Viewpoint*>  outViewPoints;

    //constructor and destructor
    animate();
    ~animate ();
    void keyframeInterpolate(std::vector<Keyframe*>& key_vec, std::vector<Viewpoint*>& view_vec);
    void priorInterPolationCalcs(const std::vector<Keyframe*>& key_vec);
    void interpolate (float T[], int N,int startIndex,const std::vector<Keyframe*>& key_vec, bool approx);
    void hermite_function(float t[],int noFrames,float inputPoints[], float out_pts[],float slope1, float slope2);
    void calculate_quats (int startIndex,const std::vector<Keyframe*>& key_vec);
    bool speedController(int startIndex,const std::vector<Keyframe*>& key_vec);
    void slopeCalculator(const std::vector<Keyframe*>& key_vec);
    void evaluateCameraPos(int startIndex,const std::vector<Keyframe*>& key_vec);
    float t_distanceFunc(float d);

    //Hermite base functions
    inline float H0(float t)                                                                
        {
        return(2*t*t*t - 3*t*t + 1);
        }                                                                                      
    inline float H1(float t)
        {
        return(-2*t*t*t+ 3*t*t);
        }                                                                                      
    inline float H2(float t)
        {
            return(t*t*t - 2*t*t + t);
        }                           
    float H3(float t)
        {
            return(t*t*t  - t*t );
        }  
};
};
#endif
