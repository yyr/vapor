//
//  animate.cpp
//  VaporAnimation
//
//  Created by Ashish Dhital on 6/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "animate.h"
#include <math.h>
#include <vector>
using namespace VAPoR;
//constructor
animate::animate(){
    
    //initialize quats
    for (int i=0; i<3; i++)
    {
        quat1[i]=0;quat2[i]=0;
		
        warpVec[i]=1.0;
    }
    
}


//destructor 
animate::~animate (){
    
    delete [] zoom;
    delete [] speed;
    delete [] totalSegments;
    delete [] slopes;
    delete [] approx_camPos;
    delete [] distance ;

    
  }


///////////////////////////////////////////////////////////////////////////////////////////////////
//keyframeInterpolate()
//receives a vector of keyframe to interpolate and then fills in the view_ptr after interpolation
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void animate:: keyframeInterpolate(std::vector<Keyframe*>& key_vec, std::vector<Viewpoint*>& view_vec,float stretchFactor)
{
    
    noVPs = int (key_vec.size());
    //presets
    zoom = new float [noVPs];
    speed= new float [noVPs];
    totalSegments= new int [noVPs];
    testPoints = 500;
    slopes=new slope[noVPs];
    approx_camPos = new vector_3D[testPoints];
    distance = new float [testPoints];
    incrementFactor= 1.0 / float(testPoints);
   
   
    //Pre-steps
    for (int i=0; i < noVPs; i++)
	{
	 //init of frames between two starting frames
	 totalSegments[i]=0;
	//copy speed to a local store with stretch factor
	speed[i] = key_vec[i]->speed * stretchFactor;
    
  	}

   //start doing the interpolation
    priorInterPolationCalcs(key_vec, view_vec); 
    
    //Interpolation done. Filling in the outgoing view point vector addresses
    for (int i=0; i < outViewPoints.size();i++){
        
        view_vec.push_back(&outViewPoints[i]);
    
    }
    
    //fill out the no of frames in between starting keyframes
    for (int i=0; i < noVPs ; i++) key_vec[i]->frameNum=totalSegments[i];
    
   
    outViewPoints.clear();
   
 
}


////////////////////////////////////////////////////////////////
//priorInterPolationCalcs()
//Does the pre-interpolation calculations of zoom and also calls 
//the interpolation functions with various index values.
///////////////////////////////////////////////////////////////

void animate::priorInterPolationCalcs(std::vector<Keyframe*>& key_vec, std::vector<Viewpoint*>& view_vec){
    //temp Viewpoint to be pushed into the view_vec
    Viewpoint outVP;  
    
    //Calculating the zoom distance at each viewpoint
    for (int i=0; i < noVPs; i++){
        zoom[i] = sqrt (pow (key_vec[i]->viewpoint->getCameraPosLocal(0) - key_vec[i]->viewpoint->getRotationCenterLocal(0), 2) + pow (key_vec[i]->viewpoint->getCameraPosLocal(1) - key_vec[i]->viewpoint->getRotationCenterLocal(1), 2) +pow (key_vec[i]->viewpoint->getCameraPosLocal(2) - key_vec[i]->viewpoint->getRotationCenterLocal(2), 2));
     //   std::cout<<"Zoom  "<<zoom[i] <<std::endl;
        
    }
    

     //various slopes calculations
     slopeCalculator(key_vec);
   
     for( int i=0; i < noVPs-1; i++){
    
            //checking for two consecutive speeds = 0
         if (speed[i] ==0 && speed[i+1] ==0){
             
             //push in the currrent keyframe into the vector
             for (int j=0 ; j <key_vec[i]->frameNum ; j++)
             {
                 
                    outVP.setCameraPosLocal(0, key_vec[i]->viewpoint->getCameraPosLocal(0)); outVP.setCameraPosLocal(1, key_vec[i]->viewpoint->getCameraPosLocal(1)); outVP.setCameraPosLocal(2, key_vec[i]->viewpoint->getCameraPosLocal(2));                            
                 
                    outVP.setViewDir(0,key_vec[i]->viewpoint->getViewDir(0));  outVP.setViewDir(1,key_vec[i]->viewpoint->getViewDir(1));  outVP.setViewDir(2,key_vec[i]->viewpoint->getViewDir(2));
                    outVP.setUpVec(0,key_vec[i]->viewpoint->getUpVec(0));    outVP.setUpVec(1,key_vec[i]->viewpoint->getUpVec(1)); outVP.setUpVec(2,key_vec[i]->viewpoint->getUpVec(2)); 
                    outVP.setRotationCenterLocal(0,key_vec[i]->viewpoint->getRotationCenterLocal(0)); outVP.setRotationCenterLocal(1,key_vec[i]->viewpoint->getRotationCenterLocal(1)); outVP.setRotationCenterLocal(2,key_vec[i]->viewpoint->getRotationCenterLocal(2));
            
             
             
                 outViewPoints.push_back(outVP);
             
             
             }
             
             
             totalSegments[i] = key_vec[i]->frameNum;
         }
         
         else{
            //evaluate the approximate camera positions
            evaluateCameraPos(i, key_vec);
    
            //calculate the quaternions
            calculate_quats(i, key_vec);
        
      
      
            //calculate the intervals
            speedController(i,key_vec);
         
         }
        
     }
    
    
        
    //Final KeyFrame 
    outVP.setCameraPosLocal(0, key_vec[noVPs-1]->viewpoint->getCameraPosLocal(0)); outVP.setCameraPosLocal(1, key_vec[noVPs-1]->viewpoint->getCameraPosLocal(1)); outVP.setCameraPosLocal(2, key_vec[noVPs-1]->viewpoint->getCameraPosLocal(2));
    
    outVP.setViewDir(0,key_vec[noVPs-1]->viewpoint->getViewDir(0));  outVP.setViewDir(1,key_vec[noVPs-1]->viewpoint->getViewDir(1));  outVP.setViewDir(2,key_vec[noVPs-1]->viewpoint->getViewDir(2));
    outVP.setUpVec(0,key_vec[noVPs-1]->viewpoint->getUpVec(0));    outVP.setUpVec(1,key_vec[noVPs-1]->viewpoint->getUpVec(1)); outVP.setUpVec(2,key_vec[noVPs-1]->viewpoint->getUpVec(2)); 
    outVP.setRotationCenterLocal(0,key_vec[noVPs-1]->viewpoint->getRotationCenterLocal(0)); outVP.setRotationCenterLocal(1,key_vec[noVPs-1]->viewpoint->getRotationCenterLocal(1)); outVP.setRotationCenterLocal(2,key_vec[noVPs-1]->viewpoint->getRotationCenterLocal(2));
    
    
    outViewPoints.push_back(outVP);
   
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
void animate::slopeCalculator(std::vector<Keyframe*>& key_vec)
{
    //first slope =0
    slopes[0].cam[0]=0;slopes[0].cam[1]=0;slopes[0].cam[2]=0;
    slopes[0].view[0]=0; slopes[0].view[1]=0; slopes[0].view[2]=0;
    slopes[0].up[0]=0;slopes[0].up[1]=0;slopes[0].up[2]=0;
    slopes[0].zoom=0;
    slopes[0].quat[0]=0; slopes[0].quat[1]=0; slopes[0].quat[2]=0;
    
    //last slope =0 
    slopes[noVPs-1].cam[0]=0;slopes[noVPs-1].cam[1]=0;slopes[noVPs-1].cam[2]=0;
    slopes[noVPs-1].view[0]=0; slopes[noVPs-1].view[1]=0; slopes[noVPs-1].view[2]=0;
    slopes[noVPs-1].up[0]=0;slopes[noVPs-1].up[1]=0;slopes[noVPs-1].up[2]=0;
    slopes[noVPs-1].zoom=0;
    slopes[noVPs-1].quat[0]=0; slopes[noVPs-1].quat[1]=0; slopes[noVPs-1].quat[2]=0;
   
    
    //rest of the slopes
    float val1, val2;
    float vDir1[3]; float vDir2[3]; float vDir0[3]; 
    float upVec1[3];float upVec2[3];float upVec0[3];
    float q1[3]; float q2[3]; float q3[3]; float q4[3];
    
    for (int i=1; i < noVPs-1;i++)
    {
	
	
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
        
        
        //quat slopes
        
        //the current point
        vDir1[0] = key_vec[i]->viewpoint->getViewDir(0);  vDir1[1] = key_vec[i]->viewpoint->getViewDir(1);          vDir1[2] = key_vec[i]->viewpoint->getViewDir(2);   
        upVec1[0]= key_vec[i]->viewpoint->getUpVec(0);    upVec1[1]= key_vec[i]->viewpoint->getUpVec(1);            upVec1[2]=  key_vec[i]->viewpoint->getUpVec(2); 
        
        //the next point
        vDir2[0] = key_vec[i+1]->viewpoint->getViewDir(0);  vDir2[1] = key_vec[i+1]->viewpoint->getViewDir(1);        vDir2[2] = key_vec[i+1]->viewpoint->getViewDir(2);   
        upVec2[0]= key_vec[i+1]->viewpoint->getUpVec(0);    upVec2[1]= key_vec[i+1]->viewpoint->getUpVec(1);          upVec2[2]= key_vec[i+1]->viewpoint->getUpVec(2); 
        
        //the previous point
        vDir0[0] = key_vec[i-1]->viewpoint->getViewDir(0);  vDir0[1] = key_vec[i-1]->viewpoint->getViewDir(1);   vDir0[2] = key_vec[i-1]->viewpoint->getViewDir(2);
        upVec0[0]= key_vec[i-1]->viewpoint->getUpVec(0);    upVec0[1]= key_vec[i-1]->viewpoint->getUpVec(1);     upVec0[2]= key_vec[i-1]->viewpoint->getUpVec(2); 
            
      

        //quats before and current
        views2ImagQuats(vDir0,upVec0, vDir1, upVec1, q1, q2);
        //quats current and after
        views2ImagQuats(vDir1, upVec1, vDir2, upVec2, q3, q4);
        
        slopes[i].quat[0] = ((q2[0]-q1[0]) + (q4[0]-q3[0]) ) *0.5;
        slopes[i].quat[1] = ((q2[1]-q1[1]) + (q4[1]-q3[1]) ) *0.5;
        slopes[i].quat[2] = ((q2[2]-q1[2]) + (q4[2]-q3[2]) ) *0.5;
      
        
    }
    
    
}


/////////////////////////////////////////////////////////////////////////////////
//calculate_quats()
//Calulates the quats at two points.
////////////////////////////////////////////////////////////////////////////////
void animate::calculate_quats (int startIndex,std::vector<Keyframe*>& key_vec){
    
    
    
    float vDir1[3]; float vDir2[3]; 
    float upVec1[3];float upVec2[3];
    
    float q1[3]; float q2[3]; 
 
    //the current point
    vDir1[0] = key_vec[startIndex]->viewpoint->getViewDir(0);  vDir1[1] = key_vec[startIndex]->viewpoint->getViewDir(1);          vDir1[2] = key_vec[startIndex]->viewpoint->getViewDir(2);   
    upVec1[0]= key_vec[startIndex]->viewpoint->getUpVec(0);    upVec1[1]= key_vec[startIndex]->viewpoint->getUpVec(1);            upVec1[2] = key_vec[startIndex]->viewpoint->getUpVec(2); 
    
    //the next point
    vDir2[0] = key_vec[startIndex+1]->viewpoint->getViewDir(0);  vDir2[1] = key_vec[startIndex+1]->viewpoint->getViewDir(1);   vDir2[2] = key_vec[startIndex+1]->viewpoint->getViewDir(2);   
    upVec2[0]= key_vec[startIndex+1]->viewpoint->getUpVec(0);    upVec2[1]= key_vec[startIndex+1]->viewpoint->getUpVec(1);     upVec2[2]= key_vec[startIndex+1]->viewpoint->getUpVec(2); 
    
    
    //initalize quats
    for (int i=0; i <3; i++){
	q1[i]=0; q2[i]=0; 	
	}

    //calculate the quats for the two points
     views2ImagQuats(vDir1, upVec1, vDir2, upVec2, q1, q2);
    
    //quat1 and quat2 for interpolation
    for (int i=0; i < 3; i++){
        quat1[i]=q1[i];
        quat2[i]=q2[i];
        
    }

}

////////////////////////////////////////////////////////////////////////
//setDistanceWarp()
//sets the values for the warp vec
///////////////////////////////////////////////////////////////////////
void animate::setDistanceWarp(float dWarp[3])
{
	warpVec[0] = dWarp[0];warpVec[1] = dWarp[1];warpVec[2] = dWarp[2];

}

///////////////////////////////////////////////////////////////////////////
//speedController()
//Controls the speed between keyframes by deciding
//how many keyframes should go in between the original
//frames.
//////////////////////////////////////////////////////////////////////////
void animate::speedController(int startIndex, std::vector<Keyframe*>& key_vec)
{
    float sum=0;
    //calculate the distance
    for (int i=0; i<testPoints;i++)
    {
        
        for (int k=1;k<=i;k++){
           
            sum+= sqrt (pow (approx_camPos[k].x - approx_camPos[k-1].x, 2) * warpVec[0]+ pow (approx_camPos[k].y - approx_camPos[k-1].y, 2) * warpVec[1]+pow (approx_camPos[k].z - approx_camPos[k-1].z, 2)* warpVec[2]);
        
        }
        
        distance[i] = sum;
      //  std::cout<<i<<"]Distance = "<<distance[i]<<std::endl;

        sum=0;
    
        
    }


    //No of frames
    int N = (int) ((2*distance[testPoints-1]/ (speed[startIndex] +speed[startIndex+1]))   +  0.5);
   // std::cout<<"No of frames... "<<N<<std::endl;
   
    float n = (2*distance[testPoints-1] )/(speed[startIndex] + speed[startIndex+1]);
   // std::cout<<"n "<<n<<std::endl;
    
    //correction factor
    float F = float (n/N);
    float s1 = F*speed[startIndex]; 
    float s2 = F*speed[startIndex+1];
    
    N = (int) (2*distance[testPoints-1]/ (s1 +s2));
  
  
    //delta
    float delta=0;
   // if (N>1)
    delta= ((s2-s1) /float(N));

   // std::cout<<"Delta "<<delta<<std::endl;
   // std::cout<<"S1 "<<s1 <<" "<<" S2 "<<s2<<std::endl;
    
    //distance between adjacent frams
    float* dist = new float[N]; 

    float sumD=0;
    
    for (int i=0; i < N  ;i++)
    {
        dist[i] = s1 + i * delta;
        sumD+=dist[i];
       // std::cout<<"dist "<<dist[i]<<std::endl;
        
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
        //std::cout<<"DIST PRIME "<<dist_prime[i]<<std::endl;
        tempSum=0;
    }
    
    //Has to pass through the start key frame.
    T[0]=0;
	//std::cout<<" "<<"%%Frame pos " <<T[0]<<" Distance== "<<dist_prime[0]<<std::endl;
    for (int i=1; i<N+1; i++){
        
        T[i] = t_distanceFunc(dist_prime[i-1]);
       // std::cout<<" "<<"%%Frame pos " <<T[i]<<" Distance== "<<dist_prime[i-1]<<std::endl;
    }
  
	
    //value of num of segments
    totalSegments[startIndex]=N+1;
    
   //call the interpolate function with the appropriate 't' and N parameters
    interpolate (T,N+1,startIndex, key_vec);
 
    	delete dist;
	delete dist_prime;
	delete T;
    
}

/////////////////////////////////////////////////////////////////////////////
//interpolate ()
//Interpolates all the points necessary for animation by recieving the 't'
// and N parameters from the speedController() function.
////////////////////////////////////////////////////////////////////////////
void animate::interpolate (float T[], int N,int startIndex,std::vector<Keyframe*>& key_vec)
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
        //std::cout<<"Rottaation X "<<rotCentOut[i].x<<" ";
        
    }
    
    
    //Rot_Y
    inputPoints[0] = key_vec[startIndex]->viewpoint->getRotationCenterLocal(1);
    inputPoints[1] = key_vec[startIndex+1]->viewpoint->getRotationCenterLocal(1);
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[1], slopes[startIndex+1].rot[1]);
    
    
    for (int i=0;i < N ; i++){
        rotCentOut[i].y = out_Points[i];
        //std::cout<<"Rottaation Y "<<rotCentOut[i].y<<" ";
        
    }
    

    //Rot_Z
    inputPoints[0] = key_vec[startIndex]->viewpoint->getRotationCenterLocal(2);
    inputPoints[1] = key_vec[startIndex+1]->viewpoint->getRotationCenterLocal(2);
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[2], slopes[startIndex+1].rot[2]);
    
    
    for (int i=0;i < N ; i++){
        rotCentOut[i].z = out_Points[i];
        //std::cout<<"Rottaation Z "<<rotCentOut[i].z<<" ";
        
    }
    
    
    //B. Interpolation of ZOOM
    inputPoints[0]= zoom[startIndex];
    inputPoints[1]=zoom[startIndex+1];
    hermite_function (T,N, inputPoints, out_Points,  slopes[startIndex].zoom, slopes[startIndex+1].zoom);
    
    for (int i=0; i <N; i++){
        zoom_out[i]=  out_Points[i];
        //std::cout<<" Zoom Out "<< zoom_out[i];
    }
   

    //C.quaternion interpolation between points A and B
    //q[0]
    inputPoints[0]= quat1[0];
    inputPoints[1] =quat2[0];
    hermite_function (T, N, inputPoints, out_Points,  slopes[startIndex].quat[0], slopes[startIndex+1].quat[0]);
    
    
    for (int i=0;i < N ; i++){
        qOut[i].x = out_Points[i];
    }
    
    //q[1]
    inputPoints[0]= quat1[1];
    inputPoints[1] =quat2[1];
    hermite_function (T, N, inputPoints, out_Points,  slopes[startIndex].quat[1], slopes[startIndex+1].quat[1]);
    
    for (int i=0;i < N ; i++){
        qOut[i].y = out_Points[i];
        
    }
    
    
    //q[2]
    inputPoints[0]= quat1[2];
    inputPoints[1] =quat2[2];
    hermite_function (T, N, inputPoints, out_Points, slopes[startIndex].quat[2], slopes[startIndex+1].quat[2]);
    
    for (int i=0;i < N ; i++){
        qOut[i].z = out_Points[i];
        
    }
    
    //view and up vectors
    for (int i=0; i < N ; i++){
        float vDir[3]; float uVec[3]; float quat[3];
        quat[0]= qOut[i].x; quat[1]=qOut[i].y; quat[2]=qOut[i].z;
        imagQuat2View(quat,vDir,uVec); 
      
        viewD[i].x= vDir[0]; viewD[i].y=vDir[1]; viewD[i].z=vDir[2];
        upD[i].x=uVec[0]; upD[i].y=uVec[1]; upD[i].z=uVec[2];
        
        
    }
    
    //camera positions 
    for (int i=0; i < N ; i++){
        
        camOut[i].x = rotCentOut[i].x - zoom_out[i] * viewD[i].x;
        camOut[i].y = rotCentOut[i].y - zoom_out[i] * viewD[i].y;
        camOut[i].z = rotCentOut[i].z - zoom_out[i] * viewD[i].z;
    
        
    }
    
    //Push everything into the output view point vector
    
    Viewpoint outVP;
    
    for (int i=0; i < N ; i++){
        
        
        outVP.setCameraPosLocal(0, camOut[i].x); outVP.setCameraPosLocal(1, camOut[i].y); outVP.setCameraPosLocal(2, camOut[i].z);
        outVP.setViewDir(0,viewD[i].x);  outVP.setViewDir(1,viewD[i].y);  outVP.setViewDir(2,viewD[i].z);
        outVP.setUpVec(0,upD[i].x);    outVP.setUpVec(1,upD[i].y); outVP.setUpVec(2,upD[i].z); 
        outVP.setRotationCenterLocal(0,rotCentOut[i].x); outVP.setRotationCenterLocal(1,rotCentOut[i].y); outVP.setRotationCenterLocal(2,rotCentOut[i].z);
        
        outViewPoints.push_back(outVP);
        
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
    float E=0;
    if (d == 0){ K=0;}
    else if (d>=distance[testPoints-1]) {K=testPoints-1;}
    
    else{
    
    //Binary Search to figure out K
    while (max > min)
    {   
        //mid points
        mid = (min + max) / 2;
        
        // std::cout<<"Max : "<<max<<" Min : "<<min<<" Mid: "<<mid<<std::endl;
        // determine which subarray to search
        if (distance[mid] <  d)
            
        {   min = mid + 1;
           // std::cout<<" val1 "<<distance[mid]<<" d " <<d<<std::endl;
            
            if (d <distance[mid+1])
            {K = mid; break;}
            
        }
        else
            
        {
            if (distance[mid] > d )
            {max = mid - 1;
               
                if (d >distance[mid-1]){K=mid; break;}
                
                
            }
            
            else 
            {
                if (distance[mid]==d)
                    // key found at index mid
                    K=mid;
            
            }}
        
    }
        //E
        E = (d-distance[K])/ (distance[K+1]-distance[K]);
    }
    
    
   // std::cout<<"Key Found at*  "<<K;
   // std::cout<<"E = "<<E<<std::endl;
   // std::cout<<"t_d"<<(.01 * (K+E));
    return (incrementFactor * (float(K)+E));
    
}

    
///////////////////////////////////////////////////////////////
//evaluateCameraPos()
//Evaluates approximate camera positions between each frames
//using hermite interpolation.
//////////////////////////////////////////////////////////////
    
    void animate::evaluateCameraPos (int startIndex,std::vector<Keyframe*>& key_vec){
        int val;
        float k;
        val=0; //index for output points
        
        
        
        //using interpolation to find out 100 approximate camera positions between 0<=k<1.0
        for (k=0;k < (1.0-incrementFactor);k+=incrementFactor){
            
            approx_camPos[val].x =key_vec[startIndex]->viewpoint->getCameraPosLocal(0) *H0(k) + key_vec[startIndex+1]->viewpoint->getCameraPosLocal(0) * H1(k) +  slopes[startIndex].cam[0] * H2(k) + slopes[startIndex+1].cam[0]*H3(k);
            approx_camPos[val].y =key_vec[startIndex]->viewpoint->getCameraPosLocal(1) *H0(k) + key_vec[startIndex+1]->viewpoint->getCameraPosLocal(1) * H1(k) +  slopes[startIndex].cam[1] * H2(k) + slopes[startIndex+1].cam[1]*H3(k);
            approx_camPos[val].z =key_vec[startIndex]->viewpoint->getCameraPosLocal(2) *H0(k) + key_vec[startIndex+1]->viewpoint->getCameraPosLocal(1) * H1(k) +  slopes[startIndex].cam[2] * H2(k) + slopes[startIndex+1].cam[2]*H3(k);
            
            // std::cout<<val<<"]Camera Pos => "<<approx_camPos[val].x<<" "<<approx_camPos[val].y<<" "<<approx_camPos[val].z<<"  K= " <<k<<std::endl;
            val++;

        }
        
          
       
    }

    
    
    
    







