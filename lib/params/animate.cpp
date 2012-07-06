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
#include<vector>
using namespace VAPoR;
//constructor
animate::animate(){
    
       //initalize the quats
    for (int i=0; i<3; i++)
    {
        quat1[i]=0; quat2[i]=0; q1_slope[i]=0;q2_slope[i]=0;
        
    }
    
}


//destructor 
animate::~animate (){
    
    delete [] zoom;
    delete [] speed;
    delete [] inputPoints;
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

void animate:: keyframeInterpolate(std::vector<Keyframe*>& key_vec, std::vector<Viewpoint*>& view_vec)
{
    
    noVPs = int (key_vec.size());
    //presets
    zoom = new float [noVPs];
    speed= new float [noVPs];
    inputPoints = new float [noVPs];
    totalSegments= new int [noVPs];
    testPoints = 100;
    slopes=new slope[noVPs];
    approx_camPos = new vector_3D[testPoints];
    distance = new float [testPoints];
    incrementFactor=.01;
    //struct
    KeyFrame kf;

    
    float  * tempPtr;
    Viewpoint* tempVP;

    //segment in between initialization
    for (int i=0; i < noVPs; i++) totalSegments[i]=0;
    
    
    for (int i=0; i < key_vec.size(); i++){
    
   //frame no
    kf.frameNum = key_vec[i]->frameNum;
        
   //cam 
    tempPtr = key_vec[i]->viewpoint->getCameraPosLocal();
    kf.vPt.cameraPos[0]=tempPtr[0];   kf.vPt.cameraPos[1]=tempPtr[1];   kf.vPt.cameraPos[2]=tempPtr[2]; 
        
    //view
    tempPtr = key_vec[i]->viewpoint->getViewDir();
    kf.vPt.viewDir[0]= tempPtr[0];     kf.vPt.viewDir[1]= tempPtr[1];    kf.vPt.viewDir[2]= tempPtr[2];

    //Up 
    tempPtr = key_vec[i]->viewpoint->getUpVec();
    kf.vPt.upVec[0]=tempPtr[0]; kf.vPt.upVec[1]=tempPtr[1]; kf.vPt.upVec[2]=tempPtr[2];

    //rot center
    tempPtr = key_vec[i]->viewpoint->getRotationCenterLocal();
    kf.vPt.rotCenter[0]=tempPtr[0]; kf.vPt.rotCenter[1]=tempPtr[1];kf.vPt.rotCenter[2]=tempPtr[2];
    
    //speed 
    kf.speed = key_vec[i]->speed;
        
        
    //push it into the vector
    startKeyFrames.push_back(kf);    
    }    
        
    
  //start doing the interpolation
    priorInterPolationCalcs(); 
    
    //Interpolation done! Filling in the outgoing view point vector
    for (int i=0; i < outViewPoints.size();i++){
        
        tempVP = new Viewpoint();
        
        //camera
        tempPtr[0] = outViewPoints[i].cameraPos[0];   tempPtr[1] = outViewPoints[i].cameraPos[1];   tempPtr[2] = outViewPoints[i].cameraPos[2];
        tempVP->setCameraPosLocal(tempPtr);
        
    
        //view
        tempPtr[0]=outViewPoints[i].viewDir[0];  tempPtr[1]=outViewPoints[i].viewDir[1];  tempPtr[2]=outViewPoints[i].viewDir[2];
        tempVP->setViewDir(tempPtr);
       
        //up
        tempPtr[0]=outViewPoints[i].upVec[0];   tempPtr[1]=outViewPoints[i].upVec[1];   tempPtr[2]=outViewPoints[i].upVec[2];
        tempVP->setUpVec(tempPtr);
        
        //rot center
        tempPtr[0]=outViewPoints[i].rotCenter[0];  tempPtr[1]=outViewPoints[i].rotCenter[1];  tempPtr[2]=outViewPoints[i].rotCenter[2];
        tempVP->setRotationCenterLocal(tempPtr);
        
        
        
        view_vec.push_back(tempVP);
    
    }
    

    //fill out the no of frames in between starting keyframes
    for (int i=0; i < noVPs ; i++) key_vec[i]->frameNum=totalSegments[i];
    
    
    //clearing up memory
    delete tempVP;
    startKeyFrames.clear();
    outViewPoints.clear();
   
 
}



/////////////////////////////////////////////////
//writeToFile()
//Writes all the interpolated points to the file.
/////////////////////////////////////////////////

void animate::writeToFile (){
    
    outputfile.open("/Users/ashishd/Desktop/output.txt", std::ios::binary);

    //std::cout<<"SIZE OF OUTPUT VIEW POINT VECTOR = "<<outViewPoints.size()<<std::endl;
    for (int i=0; i < outViewPoints.size();i++){
    
    outputfile<<0<<" "<<outViewPoints[i].cameraPos[0]<<" "<<outViewPoints[i].cameraPos[1]<<" " <<outViewPoints[i].cameraPos[2]<<" "
    <<outViewPoints[i].viewDir[0]<<" "<<outViewPoints[i].viewDir[1]<<" "<<outViewPoints[i].viewDir[2]<<" "
    << outViewPoints[i].upVec[0]<<" " << outViewPoints[i].upVec[1]<<" " << outViewPoints[i].upVec[2]<<" "
        <<outViewPoints[i].rotCenter[0]<<" " <<outViewPoints[i].rotCenter[1]<<" " <<outViewPoints[i].rotCenter[2]<<std::endl;
    
   
        
    
    }
    
    
    
    outputfile.close();    
    

}

////////////////////////////////////////////////////////////////
//priorInterPolationCalcs()
//Does the pre-interpolation calculations of zoom and also calls 
//the interpolation functions with various index values.
///////////////////////////////////////////////////////////////

void animate::priorInterPolationCalcs(){
    
    //Calculating the log of the zoom distance at each viewpoint
    for (int i=0; i < noVPs; i++){
        zoom[i] =log ( sqrt (pow (startKeyFrames[i].vPt.cameraPos[0] - startKeyFrames[i].vPt.rotCenter[0], 2) + pow (startKeyFrames[i].vPt.cameraPos[1] - startKeyFrames[i].vPt.rotCenter[1], 2) +pow (startKeyFrames[i].vPt.cameraPos[2] - startKeyFrames[i].vPt.rotCenter[2], 2)));
        //std::cout<<"Zoom "<<zoom[i] <<std::endl;
        
    }

    
  
     //various slopes calculations
     slopeCalculator();
   
     for( int i=0; i < noVPs-1; i++){
    
    
        //evaluate the approximate camera positions
        evaluateCameraPos(i);
    
        //calculate the quaternions
        calculate_quats(i);
        
      
      
        //calculate the intervals
        speedController(i);
         
   
        
     }
    
    //Final KeyFrame 
    
    viewpoint outVP;        
    outVP.cameraPos[0]= startKeyFrames[noVPs-1].vPt.cameraPos[0]; outVP.cameraPos[1]= startKeyFrames[noVPs-1].vPt.cameraPos[1]; outVP.cameraPos[2]= startKeyFrames[noVPs-1].vPt.cameraPos[2];
    outVP.viewDir[0] =  startKeyFrames[noVPs-1].vPt.viewDir[0];   outVP.viewDir[1] =  startKeyFrames[noVPs-1].vPt.viewDir[1];     outVP.viewDir[2] =  startKeyFrames[noVPs-1].vPt.viewDir[2];
    outVP.upVec[0]=startKeyFrames[noVPs-1].vPt.upVec[0]; outVP.upVec[1]=startKeyFrames[noVPs-1].vPt.upVec[1]; outVP.upVec[2]=startKeyFrames[noVPs-1].vPt.upVec[2];
    outVP.rotCenter[0]=startKeyFrames[noVPs-1].vPt.rotCenter[0];outVP.rotCenter[1]=startKeyFrames[noVPs-1].vPt.rotCenter[1];outVP.rotCenter[2]=startKeyFrames[noVPs-1].vPt.rotCenter[2];
    outViewPoints.push_back(outVP);
   
   // frame_num.push_back(startKeyFrames[noVPs-1].frameNum);
    //write to the output file
    writeToFile();
    

}


/////////////////////////////////////////////////////////////////////////////////
//hermite_function()
//Does the interpolation between the given points in inputPoints[] using hermite
//interpolation and fills out the out_pts[] array.
////////////////////////////////////////////////////////////////////////////////
void animate::hermite_function (float t[],int noFrames,float inputPoints[],float out_pts[], float slope1, float slope2){

   
    //std::cout<<"SLope 1 = "<<slope1<<" Slope 2 "<<slope2<<std::endl;
   // std::cout<<"Input 1 = "<<inputPoints[0] <<" "<<"Input 2 = "<<inputPoints[1];
    for (int k=0;k < noFrames;k++){
            

        //std::cout<<"@@Parameter: "<<t[k]<<" -- "<<std::endl;
        out_pts[k] =inputPoints[0] *H0(t[k]) + inputPoints[1] * H1(t[k]) +  slope1 * H2(t[k]) + slope2*H3(t[k]);            
            
        //std::cout<<"OUT PTS "<<out_pts[k]<<" "<<"K : "<<k<<std::endl;
        
         
        }
        
}

////////////////////////////////////////////////////////////////////////////
//slopeCalculator()
//Calculates the slopes for each component of camearPos, viewDir,  upVec,
//rotCenter, zoom and quaternions
/////////////////////////////////////////////////////////////////////////////
void animate::slopeCalculator()
{
    //first slope =0
    slopes[0].cam[0]=0;slopes[0].cam[1]=0;slopes[0].cam[2]=0;
    slopes[0].view[0]=0; slopes[0].view[1]=0; slopes[0].view[2]=0;
    slopes[0].up[0]=0;slopes[0].up[1]=0;slopes[0].up[2]=0;
    slopes[0].zoom=0;
   // slopes[0].quat[0]=0; slopes[0].quat[1]=0; slopes[0].quat[2]=0;
    
    //last slope =0 
    slopes[noVPs-1].cam[0]=0;slopes[noVPs-1].cam[1]=0;slopes[noVPs-1].cam[2]=0;
    slopes[noVPs-1].view[0]=0; slopes[noVPs-1].view[1]=0; slopes[noVPs-1].view[2]=0;
    slopes[noVPs-1].up[0]=0;slopes[noVPs-1].up[1]=0;slopes[noVPs-1].up[2]=0;
    slopes[noVPs-1].zoom=0;
   // slopes[noVPs-1].quat[0]=0; slopes[noVPs-1].quat[1]=0; slopes[noVPs-1].quat[2]=0;
    
  //  std::cout<<"##CAMERA SLOPES : "<< slopes[0].cam[0]<<" "<<slopes[0].cam[1]<<" "<<slopes[0].cam[2]<<std::endl;
    
    //rest of the slopes
    float val1, val2;
    for (int i=1; i < noVPs-1;i++)
    {
        //camera
        val1 = startKeyFrames[i+1].vPt.cameraPos[0]; val2 = startKeyFrames[i-1].vPt.cameraPos[0];
        slopes[i].cam[0] = (val1-val2)/2;
        val1 = startKeyFrames[i+1].vPt.cameraPos[1]; val2 = startKeyFrames[i-1].vPt.cameraPos[1];
        slopes[i].cam[1] = (val1-val2)/2;
        val1 = startKeyFrames[i+1].vPt.cameraPos[2]; val2 = startKeyFrames[i-1].vPt.cameraPos[2];
        slopes[i].cam[2] = (val1-val2)/2;

      //  std::cout<<"##CAMERA SLOPES : "<< slopes[i].cam[0]<<" "<<slopes[i].cam[1]<<" "<<slopes[i].cam[2]<<std::endl;
        
    
        //zoom
        val1  = zoom[i+1] ; val2 = zoom[i-1];
        slopes[i].zoom = (val1-val2)/2;
        
        //rotation
        val1 = startKeyFrames[i+1].vPt.rotCenter[0]; val2 = startKeyFrames[i-1].vPt.rotCenter[0];
        slopes[i].rot[0] = (val1-val2)/2;
        val1 = startKeyFrames[i+1].vPt.rotCenter[1]; val2 = startKeyFrames[i-1].vPt.rotCenter[1];
        slopes[i].rot[1] = (val1-val2)/2;
        val1 = startKeyFrames[i+1].vPt.rotCenter[2]; val2 = startKeyFrames[i-1].vPt.rotCenter[2];
        slopes[i].rot[2] = (val1-val2)/2;
        
    
        
    }
    
    
}


/////////////////////////////////////////////////////////////////////////////////
//calculate_quats
//Does the quaternion interpolatinos between each two points using hermite
//interpolation.
////////////////////////////////////////////////////////////////////////////////
void animate::calculate_quats (int startIndex){
    
    
    
    float vDir0[3]; float vDir1[3]; float vDir2[3]; float vDir3[3]; 
    float upVec0[3];float upVec1[3];float upVec2[3];float upVec3[3];
    float q0[3]; float q1[3]; float q2[3]; float q3[3]; 
 
    vDir1[0] = startKeyFrames[startIndex].vPt.viewDir[0];  vDir1[1] = startKeyFrames[startIndex].vPt.viewDir[1];   vDir1[2] = startKeyFrames[startIndex].vPt.viewDir[2];   
    upVec1[0]= startKeyFrames[startIndex].vPt.upVec[0]; upVec1[1]= startKeyFrames[startIndex].vPt.upVec[1]; upVec1[2]= startKeyFrames[startIndex].vPt.upVec[2]; 
    
    
    vDir2[0] = startKeyFrames[startIndex+1].vPt.viewDir[0];  vDir2[1] = startKeyFrames[startIndex+1].vPt.viewDir[1];   vDir2[2] = startKeyFrames[startIndex+1].vPt.viewDir[2];   
    upVec2[0]= startKeyFrames[startIndex+1].vPt.upVec[0]; upVec2[1]= startKeyFrames[startIndex+1].vPt.upVec[1]; upVec2[2]= startKeyFrames[startIndex+1].vPt.upVec[2]; 
    
    
    //startQuat
 
    VAPoR::view2Quat(vDir1, upVec1, startQuat);
   // calc.view2Quat(vDir1, upVec1, startQuat);
    //imaginary quaternions
    
    q0[0]=0;q0[1]=0;q0[2]=0;
     q1[0]=0;q1[1]=0;q1[2]=0;
     q2[0]=0;q2[1]=0;q2[2]=0;
   
    

    
    VAPoR::view2ImagQuat(startQuat, vDir1, upVec1, q1);
    //calc.view2ImagQuat(startQuat, vDir1, upVec1, q1);
   
    VAPoR::view2ImagQuat(startQuat, vDir2, upVec2, q2);
    
   // calc.view2ImagQuat(startQuat, vDir2, upVec2, q2);
   // std::cout<<std::endl<<" Indexing Starts at "<<startIndex<<std::endl;
    
    
    if (startIndex > 0 ) // interpolation does not stop at (vDir1, upVec1)
        
    {
        vDir0[0] = startKeyFrames[startIndex-1].vPt.viewDir[0];  vDir0[1] = startKeyFrames[startIndex-1].vPt.viewDir[1];   vDir0[2] = startKeyFrames[startIndex-1].vPt.viewDir[2];   
        upVec0[0]= startKeyFrames[startIndex-1].vPt.upVec[0]; upVec0[1]= startKeyFrames[startIndex-1].vPt.upVec[1]; upVec0[2]= startKeyFrames[startIndex-1].vPt.upVec[2]; 
        
       VAPoR::view2ImagQuat(startQuat, vDir0,upVec0,q0);
        
        //calc.view2ImagQuat(startQuat, vDir0,upVec0,q0);
    }
    
    
    else if (startIndex==0) { //no point before VDir1, upVec1
        q0[0]=0;  q0[1]=0; q0[2] = 0;
            }
    
    //interpolation does not stop at (vdir2, upvec2)
    if ( (startIndex+1) < (noVPs-1) ){
        
        vDir3[0] = startKeyFrames[startIndex+2].vPt.viewDir[0]; vDir3[1] = startKeyFrames[startIndex+2].vPt.viewDir[1]; vDir3[2] = startKeyFrames[startIndex+2].vPt.viewDir[2]; 
        upVec3[0]=startKeyFrames[startIndex+2].vPt.upVec[0]; upVec3[1]=startKeyFrames[startIndex+2].vPt.upVec[1]; upVec3[2]=startKeyFrames[startIndex+2].vPt.upVec[2];
        
       VAPoR::view2ImagQuat(startQuat, vDir3, upVec3, q3);
      //  calc.view2ImagQuat(startQuat, vDir3, upVec3, q3);
              
    }
    
    else if ((startIndex+1) == (noVPs-1))
    {
        q3[0]= 0; q3[1]=0; q3[2]=0;
    
    }
 
    
   // std::cout<<"q0= " <<q0[0]<<" "<<q0[1]<<" "<<q0[2]<<std::endl;
   // std::cout<<"q1= " <<q1[0]<<" "<<q1[1]<<" "<<q1[2]<<std::endl;
   // std::cout<<"q2= " <<q2[0]<<" "<<q2[1]<<" "<<q2[2]<<std::endl;
   
    
    
    //slope 1
    if (startIndex==0)
    {  q1_slope[0]=0; q1_slope[1]=0; q1_slope[2]=0;}
    else
    {q1_slope[0] = (q2[0] - q0[0])/2;  q1_slope[1]= (q2[1] - q0[1])/2;  q1_slope[2] = (q2[2] - q0[2])/2; }
    
    //slope 2
    if (startIndex+1==noVPs-1)
    { q2_slope[0]=0; q2_slope[1]=0; q2_slope[2]=0;}
    else
    {q2_slope[0] = (q3[0]-q1[0])/2; q2_slope[1] = (q3[1]-q1[1])/2; q2_slope[2] = (q3[2]-q1[2])/2; }
    
    
    //std::cout<<"Slope 1 = "<< q1_slope[0]<<" "<<q1_slope[1]<<" "<<q1_slope[2]<<std::endl;
    //std::cout<<"Slope 2 = "<< q2_slope[0]<<" "<<q2_slope[1]<<" "<<q2_slope[2]<<std::endl;
    
    //quat1 and quat2
    for (int i=0; i < 3; i++){
        quat1[i]=q1[i];
        quat2[i]=q2[i];
        
    }
    

}


///////////////////////////////////////////////////////////////////////////
//speedController()
//Controls the speed between keyframes by deciding
//how many keyframes should go in between the original
//frames.
//////////////////////////////////////////////////////////////////////////
void animate::speedController(int startIndex)
{
    float sum=0;

    //calculate the distance
    for (int i=0; i<100;i++)
    {
        
        for (int k=1;k<=i;k++){
           
            sum+= sqrt (pow (approx_camPos[k].x - approx_camPos[k-1].x, 2) + pow (approx_camPos[k].y - approx_camPos[k-1].y, 2) +pow (approx_camPos[k].z - approx_camPos[k-1].z, 2));
        
        }
        
        distance[i] = sum;
       // std::cout<<i<<"]Distance = "<<distance[i]<<std::endl;

        sum=0;
    
        
    }
    
    //No of frames
    int N = (int) ((2*distance[testPoints-1]/ (startKeyFrames[startIndex].speed +startKeyFrames[startIndex+1].speed))   +  0.5);
   // std::cout<<"No of frames... "<<N<<std::endl;
   
    float n = (2*distance[testPoints-1] )/(startKeyFrames[startIndex].speed + startKeyFrames[startIndex+1].speed);
   // std::cout<<"n "<<n<<std::endl;
    
    //correction factor
    float F = n/N;
    float s1 = F* startKeyFrames[startIndex].speed; float s2= F*startKeyFrames[startIndex+1].speed;
    
    N = (int) (2*distance[testPoints-1]/ (s1 +s2));
  
  
    //delta
    float delta=0;
   // if (N>1)
    delta= ((s2-s1) /(N-1));
    
   // std::cout<<"Delta "<<delta<<std::endl;
   // std::cout<<"S1 "<<s1 <<" "<<" S2 "<<s2<<std::endl;
    //distance between adjacent frams
	float* dist = new float[N]; 
   // dist[0] = s1; 
    //dist[N-1]=s2;

    float sumD=0;
    
    for (int i=0; i < N  ;i++)
    {
        dist[i] = s1 + i * delta;
        sumD+=dist[i];
      //  std::cout<<"dist "<<dist[i]<<std::endl;
        
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
       // std::cout<<"DIST PRIME "<<dist_prime[i]<<std::endl;
        tempSum=0;
    }
    
    //Has to pass through the start key frame.
    T[0]=0;

    for (int i=1; i<N+1; i++){
        
        T[i] = t_distanceFunc(dist_prime[i-1]);
        //std::cout<<" "<<"%%Frame pos " <<T[i]<<" Distance== "<<dist[i]<<std::endl;
    }
  

    //value of num of segments
    totalSegments[startIndex]=N+1;
    
    
   //call the interpolate function with the appropriate 't' and N parameters
    interpolate (T,N+1,startIndex);
 
    delete dist;
	delete dist_prime;
	delete T;
    
}

/////////////////////////////////////////////////////////////////////////////
//interpolate (T,N)
//Interpolates all the points necessary for animation by recieving the 't'
// and N parameters from the speedController() function.
////////////////////////////////////////////////////////////////////////////
void animate::interpolate (float T[], int N,int startIndex)
{
    //for output points
    float* out_Points = new float[N];
    viewpoint* tempVP = new viewpoint[N] ; 
    float inputPoints [2] ; //two input points to interpolate in between 
    
   
    //for all out put points
    vector_3D* camOut = new vector_3D[N]; 
	vector_3D* viewD = new vector_3D[N]; 
	vector_3D* upD= new vector_3D[N]; 
	vector_3D* qOut= new vector_3D[N]; 
	vector_3D* rotCentOut= new vector_3D[N];
    float* zoom_out= new float[N];

    
    //initialize the temp struct
    for (int i=0; i < N ; i++){
        tempVP[i].cameraPos[0]=0; tempVP[i].cameraPos[1]=0; tempVP[i].cameraPos[2]=0;
        tempVP[i].viewDir[0]=0;tempVP[i].viewDir[1]=0;tempVP[i].viewDir[2]=0;
        tempVP[i].upVec[0]=0; tempVP[i].upVec[1]=0;tempVP[i].upVec[2]=0;
        tempVP[i].rotCenter[0]=0; tempVP[i].rotCenter[1]=0; tempVP[i].rotCenter[2]=0;
    }
    
    // A. Interpolation of each x, y, and z of the Center of Rotation
    
    //X
    inputPoints[0] = startKeyFrames[startIndex].vPt.rotCenter[0];
    inputPoints[1] = startKeyFrames[startIndex+1].vPt.rotCenter[0];
    
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[0], slopes[startIndex+1].rot[0]);
    for (int i=0;i < N ; i++){
    
        rotCentOut[i].x = out_Points[i];
        //std::cout<<"Rottaation X "<<rotCentOut[i].x<<" ";
        
    }
    
    
    //Y
    inputPoints[0] = startKeyFrames[startIndex].vPt.rotCenter[1];
    inputPoints[1] = startKeyFrames[startIndex+1].vPt.rotCenter[1];
    hermite_function (T,N,inputPoints,out_Points,slopes[startIndex].rot[1], slopes[startIndex+1].rot[1]);
    
    
    for (int i=0;i < N ; i++){
        rotCentOut[i].y = out_Points[i];
        //std::cout<<"Rottaation Y "<<rotCentOut[i].y<<" ";
        
    }
    

    //Z
    inputPoints[0] = startKeyFrames[startIndex].vPt.rotCenter[2];
    inputPoints[1] = startKeyFrames[startIndex+1].vPt.rotCenter[2];
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
        zoom_out[i]=  exp (out_Points[i]);
        //std::cout<<" Zoom Out "<< zoom_out[i];
    }
   

    //C.quaternion interpolation between points A and B
    //q[0]
    inputPoints[0]= quat1[0];
    inputPoints[1] =quat2[0];
    hermite_function (T, N, inputPoints, out_Points,  q1_slope[0], q2_slope[0]);
    
    
    for (int i=0;i < N ; i++){
        qOut[i].x = out_Points[i];
    }
    
    //q[1]
    inputPoints[0]= quat1[1];
    inputPoints[1] =quat2[1];
    hermite_function (T, N, inputPoints, out_Points,  q1_slope[1], q2_slope[1]);
    
    for (int i=0;i < N ; i++){
        qOut[i].y = out_Points[i];
        
    }
    
    
    //q[2]
    inputPoints[0]= quat1[2];
    inputPoints[1] =quat2[2];
    hermite_function (T, N, inputPoints, out_Points, q1_slope[2], q2_slope[2]);
    
    for (int i=0;i < N ; i++){
        qOut[i].z = out_Points[i];
        
    }
    
    //view and up vectors
    for (int i=0; i < N ; i++){
        float vDir[3]; float uVec[3]; float quat[3];
        quat[0]= qOut[i].x; quat[1]=qOut[i].y; quat[2]=qOut[i].z;
        VAPoR::imagQuat2View(startQuat,quat,vDir,uVec); 
        //calc.imagQuat2View(startQuat,quat,vDir,uVec); 
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
    
    viewpoint outVP;
    
    for (int i=0; i < N ; i++){
        
        outVP.cameraPos[0]= camOut[i].x; outVP.cameraPos[1]=camOut[i].y;  outVP.cameraPos[2]=camOut[i].z;
        outVP.viewDir[0] = viewD[i].x ; outVP.viewDir[1] = viewD[i].y ;outVP.viewDir[2] = viewD[i].z ;
        outVP.upVec[0]= upD[i].x; outVP.upVec[1]= upD[i].y; outVP.upVec[2]= upD[i].z; 
        outVP.rotCenter[0]= rotCentOut[i].x; outVP.rotCenter[1]= rotCentOut[i].y; outVP.rotCenter[2]= rotCentOut[i].z;
    
        
        outViewPoints.push_back(outVP);
        
        //push back the generated frame numbers too
       // frame_num.push_back(startKeyFrames[startIndex].frameNum);
        
    }
    
    delete out_Points;
    delete tempVP; 
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
    return (.01 * (K+E));
    
}

    
///////////////////////////////////////////////////////////////
//evaluateCameraPos()
//Evaluates approximate camera positions between each frames
//using hermite interpolation.
//////////////////////////////////////////////////////////////
    
    void animate::evaluateCameraPos (int startIndex){
        
        int val;
        float k;
        val=0; //index for output points
        
       
        
            //using interpolation to find out 100 approximate camera positions between 0<=k<1.0
            for (k=0;k < (1.0-incrementFactor);k+=incrementFactor){
                              
                
                approx_camPos[val].x =startKeyFrames[startIndex].vPt.cameraPos[0] *H0(k) + startKeyFrames[startIndex+1].vPt.cameraPos[0] * H1(k) +  slopes[startIndex].cam[0] * H2(k) + slopes[startIndex+1].cam[0]*H3(k);
                approx_camPos[val].y =startKeyFrames[startIndex].vPt.cameraPos[1] *H0(k) + startKeyFrames[startIndex+1].vPt.cameraPos[1] * H1(k) +  slopes[startIndex].cam[1] * H2(k) + slopes[startIndex+1].cam[1]*H3(k);
                approx_camPos[val].z =startKeyFrames[startIndex].vPt.cameraPos[2] *H0(k) + startKeyFrames[startIndex+1].vPt.cameraPos[2] * H1(k) +  slopes[startIndex].cam[2] * H2(k) + slopes[startIndex+1].cam[2]*H3(k);
                
                
                
               // std::cout<<val<<"]Camera Pos => "<<approx_camPos[val].x<<" "<<approx_camPos[val].y<<" "<<approx_camPos[val].z<<"  K= " <<k<<std::endl;
                val++;
            }
             
          
       
    }

    
    
    
    







