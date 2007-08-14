/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	FieldLines															 *
*************************************************************************/
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "VTFieldLine.h"
#include "vapor/VaporFlow.h"
#include "vapor/flowlinedata.h"
#include "vapor/errorcodes.h"

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// definition of class FieldLine
//////////////////////////////////////////////////////////////////////////


vtCFieldLine::vtCFieldLine(CVectorField* pField):
m_nNumSeeds(0),
m_integrationOrder(FOURTH),
m_timeDir(FORWARD),
m_fInitStepSize(1.0),
m_fLowerAngleAccuracy((float)cos(15.0*DEG_TO_RAD)),
m_fUpperAngleAccuracy((float)cos(3.0*DEG_TO_RAD)),
m_nMaxsize(MAX_LENGTH),
m_pField(pField),
m_fSamplingRate(1.0)
{
}

vtCFieldLine::~vtCFieldLine(void)
{
	//printf("called in vtCFieldLine\n");
	releaseSeedMemory();
}

//////////////////////////////////////////////////////////////////////////
// release the memory allocated to seeds
//////////////////////////////////////////////////////////////////////////
void vtCFieldLine::releaseSeedMemory(void)
{
	vtListParticleIter pIter = m_lSeeds.begin();
	for( ; pIter != m_lSeeds.end(); ++pIter )
	{
		vtParticleInfo* thisPart = *pIter;
		delete thisPart;
	}
	m_lSeeds.erase(m_lSeeds.begin(), m_lSeeds.end() );
	m_nNumSeeds = 0;
	//printf("Release memory!\n");
}

//////////////////////////////////////////////////////////////////////////
// Integrate along a field line using the 2nd order Euler-Cauchy 
// predictor-corrector method. This routine is used for both steady and
// unsteady vector fields. 
//////////////////////////////////////////////////////////////////////////
int vtCFieldLine::euler_cauchy(TIME_DIR, TIME_DEP,float*, float)
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// Integrate along a field line using the 2th order Runge-Kutta method.
// This routine is used for both steady and unsteady vector fields. 
//////////////////////////////////////////////////////////////////////////
int vtCFieldLine::runge_kutta2(TIME_DIR time_dir, TIME_DEP time_dep, 
							   PointInfo& ci, 
							   double* t, float dt, float prevMag)
{
	int istat;
	istat = OKAY;
	return istat;
}

int vtCFieldLine::runge_kutta4(TIME_DIR time_dir, 
							   TIME_DEP time_dep, 
							   PointInfo& ci, 
							   double* t,			// initial time
							   float dt,			//stepsize
							   float maxMagDt)		//Largest value of dt*mag(vec)
{
	int i, istat;
	VECTOR3 pt0;
	VECTOR3 vel;
	VECTOR3 k1, k2, k3;
	VECTOR3 pt;
	int fromCell;

	pt = ci.phyCoord;

	// 1st step of the Runge-Kutta scheme
	istat = m_pField->at_phys(ci.fromCell, pt, ci, *t, vel);
	
	if ( istat != 1 )
		return OUT_OF_BOUND;

	if (vel.GetMag()*dt > maxMagDt) return FIELD_TOO_BIG;

	for( i=0; i<3; i++ )
	{
		pt0[i] = pt[i];
		k1[i] = time_dir*dt*vel[i];
		pt[i] = pt0[i]+k1[i]*(float)0.5;
	}
	
	// 2nd step of the Runge-Kutta scheme
	fromCell = ci.inCell;
	if ( time_dep  == UNSTEADY)
		*t += 0.5*time_dir*dt;
	istat=m_pField->at_phys(fromCell, pt, ci, *t, vel);

	if (vel.GetMag()*dt > maxMagDt) return FIELD_TOO_BIG;

	if ( istat!= 1 )
	{
		ci.phyCoord = pt;
		return OUT_OF_BOUND;
	}
	for( i=0; i<3; i++ )
	{
		k2[i] = time_dir*dt*vel[i];
		pt[i] = pt0[i]+k2[i]*(float)0.5;
	}

	// 3rd step of the Runge-Kutta scheme
	fromCell = ci.inCell;
	istat=m_pField->at_phys(fromCell, pt, ci, *t, vel);

	if (vel.GetMag()*dt > maxMagDt) return FIELD_TOO_BIG;

	if ( istat != 1 )
	{
		ci.phyCoord = pt;
		return OUT_OF_BOUND;
	}
	for( i=0; i<3; i++ )
	{
		k3[i] = time_dir*dt*vel[i];
		pt[i] = pt0[i]+k3[i];
	}

	//    4th step of the Runge-Kutta scheme
	if ( time_dep  == UNSTEADY)
		*t += 0.5*time_dir*dt;
	fromCell = ci.inCell;
	istat=m_pField->at_phys(fromCell, pt, ci, *t, vel);
	if (vel.GetMag()*dt > maxMagDt) return FIELD_TOO_BIG;
	if ( istat != 1 )
	{
		ci.phyCoord = pt;
		return OUT_OF_BOUND;
	}

	for( i=0; i<3; i++ ){
		pt[i] = pt0[i]+(k1[i]+(float)2.0*(k2[i]+k3[i])+time_dir*dt*vel[i])/(float)6.0;
	}
	ci.phyCoord = pt;

	return istat;
}

//////////////////////////////////////////////////////////////////////////
// aptive step size
//////////////////////////////////////////////////////////////////////////
#define STREAM_ACCURACY 1.0e-14

// according to curvature
int vtCFieldLine::adapt_step(const VECTOR3& p2, 
							 const VECTOR3& p1, 
							 const VECTOR3& p0,
							 const float& minStepsize, 
							 const float& maxStepsize,
							 float* dt,
							 bool& bAdaptive)
{
	int retrace = false;
	VECTOR3 p2p1 = p1 - p2;
	VECTOR3 p1p0 = p0 - p1;
	
	float denom = p1p0.GetMag() * p2p1.GetMag();
	float cos_p2p1p0;
	if(denom > STREAM_ACCURACY)
		cos_p2p1p0 = dot(p1p0, p2p1)/denom;
	else
		cos_p2p1p0 = m_fLowerAngleAccuracy;

	if(cos_p2p1p0 < m_fLowerAngleAccuracy)
	{
		*dt = (*dt) * (float)0.5;
		retrace = true;
		if(*dt < minStepsize)
		{
			*dt = minStepsize;
			bAdaptive = false;
			retrace = false;
		}
	}
	else if(cos_p2p1p0 > m_fUpperAngleAccuracy)
	{
		*dt = (*dt) * (float)1.25;
		if(*dt >= maxStepsize)
			*dt = maxStepsize;
	}
	
	return retrace;
}

	
//////////////////////////////////////////////////////////////////////////
// initialize seeds
//////////////////////////////////////////////////////////////////////////
void vtCFieldLine::setSeedPoints(float* points, int numPoints, float t)
{
	int i;
	VECTOR3 nodeData;

	if(points == NULL)
		return;

	// if the rake size has changed, forget the previous seed pointsQ
	if( m_nNumSeeds != numPoints )
		releaseSeedMemory();

	if( m_nNumSeeds == 0 )
	{
		for(i = 0 ; i < numPoints ; i++ )
		{
			vtParticleInfo* newParticle = new vtParticleInfo;

			newParticle->m_pointInfo.phyCoord = VECTOR3(points[3*i+0], points[3*i+1], points[3*i+2]);
			newParticle->m_fStartTime = t;
			newParticle->ptId = i;
			newParticle->itsNumStepsAlive = 0;
			
			//Here we test whether the seed particle is in the region
			//If it's out, we set the flag to invalid.  At the first step
			//of the advection, the invalid seeds get marked as being out of region.
			//Note that invalid seeds are still inserted into the seed list.
			
			if (!m_pField->is_in_grid(newParticle->m_pointInfo.phyCoord)) {
				newParticle->itsValidFlag = 0;
				
			} else newParticle->itsValidFlag = 1;
			// query the field in order to get the starting cell interpolant for the seed point
			//VECTOR3 pos;
			//pos.Set(points[3*i+0], points[3*i+1], points[3*i+2]);
			//res = m_pField->at_phys(-1, pos, newParticle->m_pointInfo, t, nodeData);
			//newParticle->itsValidFlag =  (res == 1) ? 1 : 0 ;
			
			m_lSeeds.push_back( newParticle );
		}
	} 
	else
	{
		vtListParticleIter sIter = m_lSeeds.begin();

		for(i = 0 ; sIter != m_lSeeds.end() ; ++sIter, ++i )
		{
			vtParticleInfo* thisSeed = *sIter;

			// set the new location for this seed point
			thisSeed->m_pointInfo.phyCoord = VECTOR3(points[3*i+0], points[3*i+1], points[3*i+2]);
			thisSeed->m_fStartTime = t;
			thisSeed->ptId = i;
			thisSeed->itsNumStepsAlive = 0;
			//VECTOR3 pos;
			//pos.Set(points[3*i+0], points[3*i+1], points[3*i+2]);
			//res = m_pField->at_phys(-1, pos, thisSeed->m_pointInfo, t, nodeData);
			//thisSeed->itsValidFlag =  (res == 1) ? 1 : 0 ;
			thisSeed->itsValidFlag = 1;
		}    
	}

	m_nNumSeeds = numPoints;
}


//////////////////////////////////////////////////////////////////////////
//resample stream/streak line to get points with correct interval
//AN:  Returns the amount of (unused) time remaining, to be used
//in next step.  return value only used in pathlines, not streamlines.
//This new version uses FlowLineData.  Resampled points are inserted in
//either the forward or reverse direction
//////////////////////////////////////////////////////////////////////////
float vtCFieldLine::SampleFieldline(FlowLineData* container,
								   int lineNum, // = seedNum
								   int direction, // -1 or +1
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   bool bRecordSeed,
								   int traceState,
								   float remainingTime)
{
	list<VECTOR3*>::iterator pIter1;
	list<VECTOR3*>::iterator pIter2;
	list<float>::iterator pStepIter;
	list<float>::iterator pStepIterEnd;
	float stepsizeLeft;
	// "count" counts the points along the flowline.
	// insertionPosn is direction*count
	int count;
	float x,y,z;
	float currentSpeed = 0.f;
	int insertionPosn = 0;
	
	float leftoverTime = 0.f;

	pIter1 = seedTrace->begin();
	if(bRecordSeed) // always record seed in steady flow..
	{
		
		// the first one is seed
		x = (**pIter1)[0];
		y = (**pIter1)[1];
		z = (**pIter1)[2];
		//For steady flow, the first point is always position 0.
		container->setFlowPoint(lineNum, 0, x,y,z); 
		
		if (container->doSpeeds())
		{
			PointInfo pointInfo;
			VECTOR3 nodeData;
			float t = 0.f;

			pointInfo.phyCoord.Set(x,y,z);
			if(!m_pField->isTimeVarying())
				t = m_pField->GetStartTime();
			else assert(0);
			
			m_pField->at_phys(-1, pointInfo.phyCoord, pointInfo, t, nodeData);
			currentSpeed = nodeData.GetMag()/
				(m_pField->getTimeScaleFactor()* m_pField->GetCurUserTimeStep());
			container->setSpeed(lineNum, 0, currentSpeed);
		}

	}
	insertionPosn = direction;
	count = 1;
	//AN:  Removed this assert, 10/05/05.
	//The problem is that sometimes a point will be very slightly out, and the 
	//bounds test in the runge-kutta is imprecise enough to not detect it.
	//if((int)seedTrace->size() == 1) assert(traceState == CRITICAL_POINT);

	//Deal with line that exits instantly (i.e. seed outside of region)
	if((int)seedTrace->size() == 1 && traceState != CRITICAL_POINT)
	{
		if (direction > 0){
			container->setFlowEnd(lineNum, 0);
			container->addExit(lineNum, 2);
		}
		else {
			container->setFlowStart(lineNum, 0);
			container->addExit(lineNum, 1);
		}
		return 0.f;
	}

	// process points after the first:
	pIter2 = seedTrace->begin();
	pIter2++;
	pStepIter = stepList->begin();
	stepsizeLeft = *pStepIter + remainingTime;
	//Establish the last point.  Stop one before end if OUT_OF_BOUND:
	if(traceState != OUT_OF_BOUND) 
		pStepIterEnd = stepList->end();
	else
	{
		pStepIterEnd = stepList->end();
		pStepIterEnd--;
	}
	while((count < container->getMaxLength(direction)) && (pStepIter != pStepIterEnd))
	{
		float ratio;
		//AN:  Reduced the threshold to 100*EPS because we were clipping
		//off the last sample point in path lines
		if(stepsizeLeft < (m_fSamplingRate-(1.e-4)))
		{
			pIter1++;
			pIter2++;
			pStepIter++;
			if (pStepIter == pStepIterEnd){
				leftoverTime = stepsizeLeft;
			} else 
				stepsizeLeft += *pStepIter;
		}
		else
		{
			stepsizeLeft -= m_fSamplingRate;
			ratio = (*pStepIter - stepsizeLeft)/(*pStepIter);
			
			x = Lerp((**pIter1)[0], (**pIter2)[0], ratio);
			y = Lerp((**pIter1)[1], (**pIter2)[1], ratio);
			z = Lerp((**pIter1)[2], (**pIter2)[2], ratio);
			container->setFlowPoint(lineNum, insertionPosn,x,y,z);


			// get the velocity value of this point
			if(container->doSpeeds())
			{
				PointInfo pointInfo;
				VECTOR3 nodeData;
				float t = 0.f;

				
				pointInfo.phyCoord.Set(x, y, z);
				if(!m_pField->isTimeVarying())
					t = (float) m_pField->GetStartTime();
				else assert (0);
				//	t = m_pField->GetStartTime() + m_fSamplingRate*(ptrSpeed-((int)((float)ptrSpeed/(float)m_nMaxsize)*m_nMaxsize));
				m_pField->at_phys(-1, pointInfo.phyCoord, pointInfo, t, nodeData);
				currentSpeed =  nodeData.GetMag()/
					(m_pField->getTimeScaleFactor()* m_pField->GetCurUserTimeStep());
				container->setSpeed(lineNum, insertionPosn, currentSpeed);
			}

			count++;
			insertionPosn += direction;
		}
	}

	//After exiting the loop, count == getMaxLength(direction), or we got to the end
	//of the data to be resampled.
	// if # of sampled points < maximal points available:
	
	//if((numSampled < m_nMaxsize)&& (traceState == OUT_OF_BOUND))				// out of boundary
	if (count < container->getMaxLength(direction) && (traceState == OUT_OF_BOUND))
	{
	 	pIter1 = seedTrace->end();
		pIter1--;
		
		x = (**pIter1)[0];
		y = (**pIter1)[1];
		z = (**pIter1)[2];
		container->setFlowPoint(lineNum, insertionPosn,x,y,z);
		if (direction > 0)
			container->addExit(lineNum, 2);
		else 
			container->addExit(lineNum, 1);

		
		//AN: 10/10/05
		//Set the END_FLOW_FLAG if this is not the last point in the flow:
		if (count < container->getMaxLength(direction)-1) {
			container->setFlowPoint(lineNum, insertionPosn + direction, END_FLOW_FLAG,0.f,0.f);
		}
		
		//Just replicate last speed:
		if (container->doSpeeds()){
			container->setSpeed(lineNum, insertionPosn, currentSpeed);
		}

	}
	// if # of sampled points < maximal points asked and critical point:
	else if((count < container->getMaxLength(direction))&& (traceState == CRITICAL_POINT))				// critical pt
	{
		
		container->setFlowPoint(lineNum, insertionPosn, STATIONARY_STREAM_FLAG,0.f,0.f);
		if (container->doSpeeds()){
			container->setSpeed(lineNum, insertionPosn, 0.f);
		}

	}
	//Got to the end of the data, still haven't filled the container to max length
	else if(count < container->getMaxLength(direction)) {//can we ever get here?  Maybe just for unsteady?
		assert(0);
		container->setFlowPoint(lineNum, insertionPosn, END_FLOW_FLAG, 0.f,0.f);
	} else { //otherwise we filled up the container.  Mark the previously inserted point as end
		insertionPosn -= direction;
	}
	if(direction > 0) container->setFlowEnd(lineNum, insertionPosn);
	else container->setFlowStart(lineNum, insertionPosn);
	
	return leftoverTime;
}

//////////////////////////////////////////////////////////////////////////
//resample path line to get points with correct interval
//Returns the amount of (unused) time remaining, to be used
//in next step.  return value only used in pathlines, not streamlines.
//This new version uses PathLineData.  Resampled points are inserted in
//either the forward or reverse direction
//////////////////////////////////////////////////////////////////////////
float vtCFieldLine::SampleFieldline(PathLineData* container,
								   float firstT, float lastT,
								   int lineNum, // = seedNum
								   int direction, // -1 or +1
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   bool bRecordSeed,
								   int traceState,
								   float remainingTime,
								   bool doFLA)
{
	list<VECTOR3*>::iterator pIter1;
	list<VECTOR3*>::iterator pIter2;
	list<float>::iterator pStepIter;
	list<float>::iterator pStepIterEnd;
	float stepsizeLeft;
	float x,y,z;
	float currentSpeed = 0.f;
	float leftoverTime = 0.f;

	pIter1 = seedTrace->begin();
	if(bRecordSeed) //Used (if necessary) to put the 1st point into the pathLine
	{
		
		// the first one is seed
		x = (**pIter1)[0];
		y = (**pIter1)[1];
		z = (**pIter1)[2];
		
		container->setPointAtTime(lineNum, firstT, x,y,z); 
		
		if (container->doSpeeds())
		{
			PointInfo pointInfo;
			VECTOR3 nodeData;

			pointInfo.phyCoord.Set(x,y,z);
			
			m_pField->at_phys(-1, pointInfo.phyCoord, pointInfo, firstT, nodeData);
			currentSpeed = nodeData.GetMag()/
				(m_pField->getTimeScaleFactor()*m_pField->GetCurUserTimeStep());
			container->setSpeedAtTime(lineNum, firstT, currentSpeed);
		}

	}
	
	//AN:  Removed this assert, 10/05/05.
	//The problem is that sometimes a point will be very slightly out, and the 
	//bounds test in the runge-kutta is imprecise enough to not detect it.
	//if((int)seedTrace->size() == 1) assert(traceState == CRITICAL_POINT);

	//Deal with line that exits instantly (i.e. seed outside of region??)
	if((int)seedTrace->size() == 1 && traceState != CRITICAL_POINT)
	{
		if (direction > 0)
			container->setFlowEndAtTime(lineNum, firstT);
		else 
			container->setFlowStartAtTime(lineNum, firstT);

		MyBase::SetErrMsg(VAPOR_WARNING_FLOW, "Seed for flow line %d outside of region",lineNum);
		return 0.f;
	}

	// process points after the first:
	pIter2 = seedTrace->begin();
	pIter2++;
	pStepIter = stepList->begin();
	stepsizeLeft = *pStepIter + remainingTime;
	//Establish the last point.  Stop one before end if OUT_OF_BOUND:
	if(traceState != OUT_OF_BOUND) 
		pStepIterEnd = stepList->end();
	else
	{
		pStepIterEnd = stepList->end();
		pStepIterEnd--;
	}
	float nextTime = firstT;
	while(pStepIter != pStepIterEnd)
	{
		
		float ratio;
		//AN:  Reduced the threshold to 100*EPS because we were clipping
		//off the last sample point in path lines
		if(stepsizeLeft < (m_fSamplingRate-(1.e-4)))
		{
			pIter1++;
			pIter2++;
			pStepIter++;
			if (pStepIter == pStepIterEnd){
				leftoverTime = stepsizeLeft;
			} else 
				stepsizeLeft += *pStepIter;

			//x = (**pIter1)[0];
			//y = (**pIter1)[1];
			//z = (**pIter1)[2];
			
		}
		else
		{
			stepsizeLeft -= m_fSamplingRate;
			nextTime += m_fSamplingRate*direction;
			ratio = (*pStepIter - stepsizeLeft)/(*pStepIter);
			
			x = Lerp((**pIter1)[0], (**pIter2)[0], ratio);
			y = Lerp((**pIter1)[1], (**pIter2)[1], ratio);
			z = Lerp((**pIter1)[2], (**pIter2)[2], ratio);
			container->setPointAtTime(lineNum, nextTime,x,y,z);



			// get the velocity value of this point, use next time step.
			if(container->doSpeeds())
			{
				PointInfo pointInfo;
				VECTOR3 nodeData;
				pointInfo.phyCoord.Set(x, y, z);
				m_pField->at_phys(-1, pointInfo.phyCoord, pointInfo, nextTime, nodeData);
				currentSpeed =  nodeData.GetMag()/
					(m_pField->getTimeScaleFactor()*m_pField->GetCurUserTimeStep()); 
				container->setSpeedAtTime(lineNum, nextTime, currentSpeed);
			}
		}
	}

	//After exiting the loop we got to the end
	//of the data to be resampled. 
	//Output the out of bounds end point if the line went out of bounds,
	//Providing we are not doing FLA.
	//( with FLA, do not do anything, leave last point alone)

	if (!doFLA && traceState == OUT_OF_BOUND){
		pIter1++;
		nextTime += m_fSamplingRate*direction;
		float x1 = (**pIter1)[0];
		float y1 = (**pIter1)[1];
		float z1 = (**pIter1)[2];
		//Do we need to remove this point from the seedTrace??????
		container->setPointAtTime(lineNum, nextTime,x1,y1,z1);
		if(container->doSpeeds()){ //Repeat the last speed
			container->setSpeedAtTime(lineNum, nextTime, currentSpeed);
		}
	}
	//Don't carry over insignificant negative leftover times, since,
	//They can screw up the next sampling
	assert(leftoverTime > -1.e-4);
	if (leftoverTime < 0.f) leftoverTime= 0.f;
	return leftoverTime;
}

//////////////////////////////////////////////////////////////////////////
//Alternate version of above for resampling the points advected from
//resampled field lines
//in next step.  return values are put into flData at specified time.
//////////////////////////////////////////////////////////////////////////
float vtCFieldLine::SampleFLALine(FlowLineData** flData,
								   float firstT, float lastT,
								   int lineNum, int pointNum, //from ptId
								   int direction, // -1 or +1
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   int traceState,
								   float remainingTime)
								  
{
	list<VECTOR3*>::iterator pIter1;
	list<VECTOR3*>::iterator pIter2;
	list<float>::iterator pStepIter;
	list<float>::iterator pStepIterEnd;
	float stepsizeLeft;
	float x,y,z;
	float leftoverTime = 0.f;

	pIter1 = seedTrace->begin();

	
	//Deal with line that exits instantly (i.e. seed outside of region??)
	if((int)seedTrace->size() == 1 && traceState != CRITICAL_POINT)
	{
		//This probably won't ever happen?
		assert(0);
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW, "Seed for flow line %d outside of region",lineNum);
		return 0.f;
	}

	// process points after the first:
	pIter2 = seedTrace->begin();
	pIter2++;
	pStepIter = stepList->begin();
	stepsizeLeft = *pStepIter + remainingTime;
	//Establish the last point.  Stop one before end if OUT_OF_BOUND:
	if(traceState != OUT_OF_BOUND) 
		pStepIterEnd = stepList->end();
	else
	{
		pStepIterEnd = stepList->end();
		pStepIterEnd--;
	}
	float nextTime = firstT;
	while(pStepIter != pStepIterEnd)
	{
		
		float ratio;
		//Check if we are within epsilon of end:
		if(stepsizeLeft < (m_fSamplingRate-(1.e-4)))
		{
			pIter1++;
			pIter2++;
			pStepIter++;
			if (pStepIter == pStepIterEnd){
				leftoverTime = stepsizeLeft;
			} else 
				stepsizeLeft += *pStepIter;
		}
		else
		{
			stepsizeLeft -= m_fSamplingRate;
			nextTime += m_fSamplingRate*direction;
			ratio = (*pStepIter - stepsizeLeft)/(*pStepIter);
			
			x = Lerp((**pIter1)[0], (**pIter2)[0], ratio);
			y = Lerp((**pIter1)[1], (**pIter2)[1], ratio);
			z = Lerp((**pIter1)[2], (**pIter2)[2], ratio);
			
			flData[(int)(nextTime+0.5f)]->setFlowPoint(lineNum, pointNum, x,y,z);
		}
	}

	//After exiting the loop we got to the end
	//of the data to be resampled. 
	//Output the END_FLOW_FLAG if the line went out of bounds,

	if (traceState == OUT_OF_BOUND){
		pIter1++;
		nextTime += m_fSamplingRate*direction;
		
		flData[(int)(nextTime+0.5)]->setFlowPoint(lineNum, pointNum, END_FLOW_FLAG,
			END_FLOW_FLAG, END_FLOW_FLAG);
	}
	//Don't carry over insignificant negative leftover times, since,
	//They can screw up the next sampling
	assert(leftoverTime > -1.e-4);
	if (leftoverTime < 0.f) leftoverTime= 0.f;
	return leftoverTime;
}

