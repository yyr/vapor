//
//  animate.cpp
//  VaporAnimation
//
//  Created by Ashish Dhital on 6/14/12.
//  Copyright (c) 2012 NCAR. All rights reserved.
//

#include "glutil.h"	// Must be included first
#include <iostream>
#include "animate.h"
#include <math.h>
#include<vector>
using namespace VAPoR;
//constructor
animate::animate(){
    zoom=0;
    totalSegments=0;
    slopes=0;
    approx_camPos=0;
    distance =0;
    
}


//destructor 
animate::~animate (){
    
    if (zoom) delete [] zoom;
    if (totalSegments) delete [] totalSegments;
    if (slopes) delete [] slopes;
    if (approx_camPos) delete [] approx_camPos;
    if (distance) delete [] distance ;

 }


///////////////////////////////////////////////////////////////////////////////////////////////////
//keyframeInterpolate()
//receives a vector of keyframe to interpolate and then fills in the view_ptr after interpolation
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void animate:: keyframeInterpolate(std::vector<Keyframe*>& key_vec, std::vector<Viewpoint*>& view_vec)
{
    
    noVPs = int (key_vec.size());
    //presets
    zoom = new float [noVPs];
    totalSegments= new int [noVPs];
    testPoints = 500;
    slopes=new slope[noVPs];
    approx_camPos = new vector_3D[testPoints];
    distance = new float [testPoints];
    incrementFactor= 1.0 / float(testPoints);
	for (int i = 0; i< noVPs; i++){
		float * qt = new float[4];
		viewQuats.push_back(qt);
	}
   
   
   //init of frames between two starting frames 
    for (int i=0; i < noVPs; i++) totalSegments[i]=0;
	
   //pre-calculations for the interpolation
    priorInterPolationCalcs(key_vec); 
    
    //Interpolation done. Filling in the outgoing view point vector addresses
    for (int i=0; i < outViewPoints.size();i++){
        
        view_vec.push_back(outViewPoints[i]);
    
    }
    
    //fill out the no of frames in between starting keyframes
	for (int i=0; i < noVPs ; i++) {
		if (!key_vec[i]->stationaryFlag)
			key_vec[i]->numFrames=totalSegments[i];
	}
    
    outViewPoints.clear();
	for (int i = 0; i<noVPs; i++){
		delete [] viewQuats[i];
	}
	viewQuats.clear();
}


////////////////////////////////////////////////////////////////
//priorInterPolationCalcs()
//Does the pre-interpolation calculations of zoom and also calls 
//the interpolation functions with various index values.
///////////////////////////////////////////////////////////////

void animate::priorInterPolationCalcs(const std::vector<Keyframe*>& key_vec){
    
    //Calculating the zoom distance at each keyframe
    for (int i=0; i < noVPs; i++){
        zoom[i] = sqrt (pow (key_vec[i]->viewpoint->getCameraPosLocal(0) 
			- key_vec[i]->viewpoint->getRotationCenterLocal(0), 2) 
			+ pow (key_vec[i]->viewpoint->getCameraPosLocal(1) 
			- key_vec[i]->viewpoint->getRotationCenterLocal(1), 2) 
			+pow (key_vec[i]->viewpoint->getCameraPosLocal(2) 
			- key_vec[i]->viewpoint->getRotationCenterLocal(2), 2));
    }
	for (int i = 0; i<noVPs; i++)
		view2Quat(key_vec[i]->viewpoint->getViewDir(), key_vec[i]->viewpoint->getUpVec(), viewQuats[i]);
	//Make adjacent viewQuats have positive dot product, for shortest path in slerp calc
	for (int i = 0; i<noVPs-1; i++){
		float* vw1 = viewQuats[i];
		float* vw2 = viewQuats[i+1];
		float dotprod = vdot(vw1,vw2)+vw1[3]*vw2[3];
		if (dotprod < 0){
			for (int j = 0; j<4; j++) vw2[j] = -vw2[j];
		}
	}
 
     //various slopes calculations
     slopeCalculator(key_vec);
   
    //approximate positions
    float *T = new float[testPoints];
    int val=0;
    for(float k=0;k < (1.0-incrementFactor);k+=incrementFactor){
        assert(val < testPoints);
        T[val] = k;
        val++;
    }
    
    
     for( int i=0; i < noVPs-1; i++){
		//If next keyframe has the stationaryFlag set, then this keyframe has the same viewpoint as the next one.
		//Just copy the viewpoint for each of the inbetween frames
		if (key_vec[i+1]->stationaryFlag){
			assert(i>0);
			for (int j = 0; j<key_vec[i+1]->numFrames; j++){
				Viewpoint* outVP = new Viewpoint(*(key_vec[i]->viewpoint));
				outViewPoints.push_back(outVP);
			}
			continue;
		}
 
        //evaluate approximate camera positions using approx=true
  	    interpolate(T,  testPoints ,i, key_vec,true);

		//If synch is true, calculate the speed required to obtain the desired number of frames
		bool speedOK = true;
		if(key_vec[i+1]->synch){
			//calculate the distance
			float totDist = 0.;
			for (int k=0; k<testPoints-1;k++){
				totDist += sqrt (pow (approx_camPos[k+1].x - approx_camPos[i].x, 2) 
					+ pow (approx_camPos[k+1].y - approx_camPos[k].y, 2) 
					+ pow (approx_camPos[k+1].z - approx_camPos[k].z, 2));
			}
			int skipRate = (key_vec[i+1]->timestepsPerFrame);
			int frameCount = (key_vec[i+1]->timeStep - key_vec[i]->timeStep)/skipRate;
			assert(frameCount != 0);
			float needSpeed = totDist/(float)(abs(frameCount));
			//end speed plus start speed should average to needSpeed
			float endSpeed = 2.*needSpeed - key_vec[i]->speed;
			//If endSpeed is <0, we cannot smoothly vary speed.  Will just need to interpolate, with end speed = 0.
			if (endSpeed >= 0) key_vec[i+1]->speed = endSpeed;
			else {
				key_vec[i+1]->speed = 0.f;
				speedOK = false;
			}
		}
		
        //calculate the intervals
		if (speedOK){
			if (!speedController(i,key_vec)){
				//Nothing to interpolate.  Just push the starting keyframe into the output
				Viewpoint* outVP = new Viewpoint(); 
				//Start KeyFrame 
				outVP->setCameraPosLocal(0, key_vec[i]->viewpoint->getCameraPosLocal(0)); outVP->setCameraPosLocal(1, key_vec[i]->viewpoint->getCameraPosLocal(1)); outVP->setCameraPosLocal(2, key_vec[i]->viewpoint->getCameraPosLocal(2));
    
				outVP->setViewDir(0,key_vec[i]->viewpoint->getViewDir(0));  outVP->setViewDir(1,key_vec[i]->viewpoint->getViewDir(1));  outVP->setViewDir(2,key_vec[i]->viewpoint->getViewDir(2));
				outVP->setUpVec(0,key_vec[i]->viewpoint->getUpVec(0));    outVP->setUpVec(1,key_vec[i]->viewpoint->getUpVec(1)); outVP->setUpVec(2,key_vec[i]->viewpoint->getUpVec(2)); 
				outVP->setRotationCenterLocal(0,key_vec[i]->viewpoint->getRotationCenterLocal(0)); outVP->setRotationCenterLocal(1,key_vec[i]->viewpoint->getRotationCenterLocal(1)); outVP->setRotationCenterLocal(2,key_vec[i]->viewpoint->getRotationCenterLocal(2));
    
				outViewPoints.push_back(outVP);
			}
		} else {
			//Do a quick and dirty interp to match frame numbers
		}
     
     }
    
    Viewpoint* outVP = new Viewpoint(); 
    //Final KeyFrame 
    outVP->setCameraPosLocal(0, key_vec[noVPs-1]->viewpoint->getCameraPosLocal(0)); outVP->setCameraPosLocal(1, key_vec[noVPs-1]->viewpoint->getCameraPosLocal(1)); outVP->setCameraPosLocal(2, key_vec[noVPs-1]->viewpoint->getCameraPosLocal(2));
    
    outVP->setViewDir(0,key_vec[noVPs-1]->viewpoint->getViewDir(0));  outVP->setViewDir(1,key_vec[noVPs-1]->viewpoint->getViewDir(1));  outVP->setViewDir(2,key_vec[noVPs-1]->viewpoint->getViewDir(2));
    outVP->setUpVec(0,key_vec[noVPs-1]->viewpoint->getUpVec(0));    outVP->setUpVec(1,key_vec[noVPs-1]->viewpoint->getUpVec(1)); outVP->setUpVec(2,key_vec[noVPs-1]->viewpoint->getUpVec(2)); 
    outVP->setRotationCenterLocal(0,key_vec[noVPs-1]->viewpoint->getRotationCenterLocal(0)); outVP->setRotationCenterLocal(1,key_vec[noVPs-1]->viewpoint->getRotationCenterLocal(1)); outVP->setRotationCenterLocal(2,key_vec[noVPs-1]->viewpoint->getRotationCenterLocal(2));
    
    outViewPoints.push_back(outVP);
	delete [] T;
}


/////////////////////////////////////////////////////////////////////////////////
//hermite_function()
//Does the interpolation between the given points in inputPoints[] using hermite
//interpolation and fills out the out_pts[] array.
////////////////////////////////////////////////////////////////////////////////
void animate::hermite_function (float t[],int noFrames,float inputPoints[],float out_pts[], float slope1, float slope2){

    for (int k=0;k < noFrames;k++){    
        out_pts[k] =inputPoints[0] *H0(t[k]) + inputPoints[1] * H1(t[k]) +  slope1 * H2(t[k]) + slope2*H3(t[k]);  
    }
        
}

////////////////////////////////////////////////////////////////////////////
//slopeCalculator()
//Calculates the slopes for each component of camearPos, viewDir,  upVec,
//rotCenter, zoom and quaternions
/////////////////////////////////////////////////////////////////////////////
void animate::slopeCalculator(const std::vector<Keyframe*>& key_vec)
{
	
	//Handle endpoints and where speed is 0:
	for (int i = 0; i<= noVPs-1; i++){
		if (i>0 && i<noVPs-1 && key_vec[i]->speed > 0.f) continue;
		for (int j = 0; j<3; j++){
			slopes[i].cam[j] = 0.;
			slopes[i].rot[j] = 0.;
			slopes[i].view[j] = 0.;
			slopes[i].up[j] = 0.;
		}
		slopes[i].zoom = 0.;
    
		for (int j = 0; j<4; j++){
			slopes[i].quat[j] = viewQuats[i][j];
		}
	}
   
    
    //rest of the slopes
    float val1, val2;
     
    for (int i=1; i < noVPs-1;i++)
    {
		if (key_vec[i]->speed == 0.f) continue;
        //camera
        val1 = key_vec[i+1]->viewpoint->getCameraPosLocal(0); val2 = key_vec[i-1]->viewpoint->getCameraPosLocal(0);
        slopes[i].cam[0] = (val1-val2)/2;
        val1 = key_vec[i+1]->viewpoint->getCameraPosLocal(1); val2 = key_vec[i-1]->viewpoint->getCameraPosLocal(1);
        slopes[i].cam[1] = (val1-val2)/2;
        val1 = key_vec[i+1]->viewpoint->getCameraPosLocal(2); val2 = key_vec[i-1]->viewpoint->getCameraPosLocal(2);
        slopes[i].cam[2] = (val1-val2)/2;
        
        //zoom
        val1  = zoom[i+1] ; val2 = zoom[i-1];
        slopes[i].zoom = (val1-val2)/2;
        
        //rotation
        val1 = key_vec[i+1]->viewpoint->getRotationCenterLocal(0); val2 = key_vec[i-1]->viewpoint->getRotationCenterLocal(0);
        slopes[i].rot[0] = (val1-val2)/2;
        val1 = key_vec[i+1]->viewpoint->getRotationCenterLocal(1); val2 = key_vec[i-1]->viewpoint->getRotationCenterLocal(1);
        slopes[i].rot[1] = (val1-val2)/2;
        val1 = key_vec[i+1]->viewpoint->getRotationCenterLocal(2); val2 = key_vec[i-1]->viewpoint->getRotationCenterLocal(2);;
        slopes[i].rot[2] = (val1-val2)/2;
        
        
        //quat slopes (are actually control points)
        
       
		float* q0, *q1, *q2;
		float q3[4],q4[4],q5[4],q6[4],q1inv[4];
		q0 = viewQuats[i-1];
		q1 = viewQuats[i];
		q2 = viewQuats[i+1];
        qconj(q1,q1inv);
		qmult(q1inv,q0,q3);
		qmult(q1inv,q2,q4);
		qlog(q3,q5);
		qlog(q4,q6);
		qadd(q5,q6,q3);
		//Multiply by -1/4 and exponentiate.  ignore (zero) real part..
		vscale(q3, -0.25);
		float mag = vlength(q3);
		//exponentiate in place:
		q3[3]= cos(mag);
		if (mag > 0.) vscale(q3,sin(mag)/mag);
		qmult(q1,q3,slopes[i].quat); 
    }
}



///////////////////////////////////////////////////////////////////////////
//speedController()
//Controls the speed between keyframes by deciding
//how many keyframes should go in between the original
//frames.
//Returns false if there are no inbetween frames.
//////////////////////////////////////////////////////////////////////////
bool animate::speedController(int startIndex,const  std::vector<Keyframe*>& key_vec)
{
    float sum=0;
    //calculate the distance
    for (int i=0; i<testPoints;i++)
    {
        
        for (int k=1;k<=i;k++){
            sum+= sqrt (pow (approx_camPos[k].x - approx_camPos[k-1].x, 2) 
				+ pow (approx_camPos[k].y - approx_camPos[k-1].y, 2) 
				+ pow (approx_camPos[k].z - approx_camPos[k-1].z, 2));
        }
        
        distance[i] = sum ;
        sum=0;
    }
	assert((key_vec[startIndex]->speed +key_vec[startIndex+1]->speed) > 0.f);
    //No of frames
    int N = (int) ((2*distance[testPoints-1]/ (key_vec[startIndex]->speed +key_vec[startIndex+1]->speed))   +  0.5);
	if (N == 0){
		totalSegments[startIndex+1] = 1;
		return false;
	}
   
    float n = (2*distance[testPoints-1] )/(key_vec[startIndex]->speed + key_vec[startIndex+1]->speed);
    
    //correction factor
    float F = float (n/N);
    float s1 = F*key_vec[startIndex]->speed; 
    float s2 = F*key_vec[startIndex+1]->speed;
  
    //delta
    float delta=0;
   
    delta= ((s2-s1) /float(N));
    
    //distance between adjacent frams
    float* dist = new float[N]; 

    float sumD=0;
    
    for (int i=0; i < N  ;i++)
    {
        dist[i] = s1 + i * delta;
        sumD+=dist[i];
        
    }

    //calculation of value of parameter t associated with each of dist[i]
    float* T = new float[N+1];
    float tempSum=0;
	//total distance
    float* dist_prime = new float[N];

    for (int i=0; i<N;i++){
        for (int k=0; k<=i;k++){
            tempSum += dist[k];
        }
        dist_prime[i] = tempSum;
        tempSum=0;
    }
    
    //Has to pass through the start key frame.
    T[0]=0;
    for (int i=1; i<N+1; i++){
        T[i] = t_distanceFunc(dist_prime[i-1]);
    }
    //value of num of segments
    totalSegments[startIndex+1]=N;
    
    //call the interpolate function with the appropriate 't' and N parameters
    interpolate (T,N,startIndex, key_vec, false);
 
    delete dist;
	delete dist_prime;
	delete T;
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//interpolate ()
//Interpolates all the points necessary for animation by recieving the 't'
// and N parameters from the speedController() function.
////////////////////////////////////////////////////////////////////////////
void animate::interpolate (float T[], int N,int startIndex,const std::vector<Keyframe*>& key_vec, bool approx)
{
    //for output points
    float* out_Points = new float[N];
    float inputPoints [2] ; //two input points to interpolate in between 
    
    //for all out put points
    vector_3D* camOut = new vector_3D[N]; 
	vector_3D* viewD = new vector_3D[N]; 
	vector_3D* upD= new vector_3D[N]; 
	vector_3D* qOut= new vector_3D[N]; 
	vector_3D* rotCentOut= new vector_3D[N];
    float* zoom_out= new float[N];

   
    // A. Interpolation of each x, y, and z of the Center of Rotation
    
    //Rot_X
    inputPoints[0] = key_vec[startIndex]->viewpoint->getRotationCenterLocal(0);
    inputPoints[1] = key_vec[startIndex+1]->viewpoint->getRotationCenterLocal(0);
    
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[0], slopes[startIndex+1].rot[0]);

    for (int i=0;i < N ; i++){
        rotCentOut[i].x = out_Points[i];
    }

    //Rot_Y
    inputPoints[0] = key_vec[startIndex]->viewpoint->getRotationCenterLocal(1);
    inputPoints[1] = key_vec[startIndex+1]->viewpoint->getRotationCenterLocal(1);
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[1], slopes[startIndex+1].rot[1]);
    
    for (int i=0;i < N ; i++){
        rotCentOut[i].y = out_Points[i];
    }
  
    //Rot_Z
    inputPoints[0] = key_vec[startIndex]->viewpoint->getRotationCenterLocal(2);
    inputPoints[1] = key_vec[startIndex+1]->viewpoint->getRotationCenterLocal(2);
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[2], slopes[startIndex+1].rot[2]);
  
    for (int i=0;i < N ; i++){
        rotCentOut[i].z = out_Points[i];
       
    }
    
    
    //B. Interpolation of ZOOM
    inputPoints[0]= zoom[startIndex];
    inputPoints[1]=zoom[startIndex+1];
    hermite_function (T,N, inputPoints, out_Points,  slopes[startIndex].zoom, slopes[startIndex+1].zoom);
    
    for (int i=0; i <N; i++){
        zoom_out[i]=  out_Points[i];
    }
   

    //C.quaternion interpolation between points A and B.  uses squad instead of Hermite poly.
	float quatInterp[4], viewdir[3], upvec[3];
	for (int i = 0; i<N; i++){
		squad(viewQuats[startIndex],viewQuats[startIndex+1],slopes[startIndex].quat,slopes[startIndex+1].quat,T[i],quatInterp);
		
		quat2View(quatInterp, viewdir,upvec);
		viewD[i].x = viewdir[0];
		viewD[i].y = viewdir[1];
		viewD[i].z = viewdir[2];
		upD[i].x = upvec[0];
		upD[i].y = upvec[1];
		upD[i].z = upvec[2];
	}
	
    //camera positions 
    for (int i=0; i < N ; i++){
        
        camOut[i].x = rotCentOut[i].x - zoom_out[i] * viewD[i].x;
        camOut[i].y = rotCentOut[i].y - zoom_out[i] * viewD[i].y;
        camOut[i].z = rotCentOut[i].z - zoom_out[i] * viewD[i].z; 
    }
    
    if(!approx)//if its not the approximate calculation then push values into out view point
    {
        //Push everything into the output view point vector
    
        for (int i=0; i < N ; i++){
			Viewpoint* outVP = new Viewpoint;
	        
			outVP->setCameraPosLocal(0, camOut[i].x); outVP->setCameraPosLocal(1, camOut[i].y); outVP->setCameraPosLocal(2, camOut[i].z);
			outVP->setViewDir(0,viewD[i].x);  outVP->setViewDir(1,viewD[i].y);  outVP->setViewDir(2,viewD[i].z);
			outVP->setUpVec(0,upD[i].x);    outVP->setUpVec(1,upD[i].y); outVP->setUpVec(2,upD[i].z); 
			outVP->setRotationCenterLocal(0,rotCentOut[i].x); outVP->setRotationCenterLocal(1,rotCentOut[i].y); outVP->setRotationCenterLocal(2,rotCentOut[i].z);
	        
			outViewPoints.push_back(outVP);
        
        }
    }
    else //if its approx calculation, just fill in the approximate camera positions
    {   
        for(int i=0;  i<N;i++)
        {
            
            approx_camPos[i].x=camOut[i].x;
            approx_camPos[i].y=camOut[i].y;
            approx_camPos[i].z=camOut[i].z;
          
        }
    }
    
	delete out_Points;
	delete camOut; 
    delete viewD ; 
    delete upD; 
    delete qOut; 
    delete rotCentOut;
	delete zoom_out;
    
}

///////////////////////////////////////////////////////////////////////
//t_distanceFunc()
//It calculates the parameter t as a function of distance d and returns 
//the value
///////////////////////////////////////////////////////////////////////
float animate::t_distanceFunc(float d){
    
    int max=testPoints-1; int min =0; int mid;
    int K; //default
    double E=0;
    if (d == 0){ K=0;}
    else if (d>=distance[testPoints-1]) {K=testPoints-1;}
    
    else{
    
    //Binary Search to figure out K
    while (max > min)
    {   
        //mid points
        mid = (min + max) / 2;
        
        // determine which subarray to search
        if (distance[mid] <  d)
        {   
			min = mid;
            if (d <distance[mid+1])
            {K = mid; break;}
            
        }
        else
        {
            if (distance[mid] > d )
            {max = mid;
               
                if (d >distance[mid-1]){
					K=mid-1; break;
				}     
            }
            
            else 
            {
                if (distance[mid]==d)
                    // key found at index mid
                    K=mid;
            }
		}
        
    }
        //E
		assert(distance[K+1] != distance[K]);
		assert(distance[K] <= d);
		assert(distance[K+1] >= d);
        E = double(d-distance[K])/ double(distance[K+1]-distance[K]);
    }
  
    return (incrementFactor * (double(K)+E));
    
}
