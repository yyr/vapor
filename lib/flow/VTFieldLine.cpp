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

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// definition of class FieldLine
//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
	extern FILE* fDebug;
#endif

vtCFieldLine::vtCFieldLine(CVectorField* pField):
m_nNumSeeds(0),
m_integrationOrder(FOURTH),
m_timeDir(FORWARD),
m_fLowerAngleAccuracy((float)cos(15.0*DEG_TO_RAD)),
m_fUpperAngleAccuracy((float)cos(3.0*DEG_TO_RAD)),
m_nMaxsize(MAX_LENGTH),
m_fInitStepSize(1.0),
m_fSamplingRate(1.0),
m_pField(pField)
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
							   float* t, float dt)
{
	int istat;
	istat = OKAY;
	return istat;
}

int vtCFieldLine::runge_kutta4(TIME_DIR time_dir, 
							   TIME_DEP time_dep, 
							   PointInfo& ci, 
							   float* t,			// initial time
							   float dt)			// stepsize
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
	for( i=0; i<3; i++ )
	{
		pt0[i] = pt[i];
		k1[i] = time_dir*dt*vel[i];
		pt[i] = pt0[i]+k1[i]*(float)0.5;
	}

	// 2nd step of the Runge-Kutta scheme
	fromCell = ci.inCell;
	if ( time_dep  == UNSTEADY)
		*t += (float)0.5*time_dir*dt;
	istat=m_pField->at_phys(fromCell, pt, ci, *t, vel);
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
		*t += (float)0.5*time_dir*dt;
	fromCell = ci.inCell;
	istat=m_pField->at_phys(fromCell, pt, ci, *t, vel);
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
// sample streamline to get points with correct interval
//AN:  Returns the amount of (unused) time remaining, to be used
//in next step.  Only used in streaklines, not streamlines.
//////////////////////////////////////////////////////////////////////////
float vtCFieldLine::SampleFieldline(float* positions,
								   const unsigned int* startPositions,
								   unsigned int& posInPoints,
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   bool bRecordSeed,
								   int traceState,
								   float* speeds,
								   float remainingTime)
{
	list<VECTOR3*>::iterator pIter1;
	list<VECTOR3*>::iterator pIter2;
	list<float>::iterator pStepIter;
	list<float>::iterator pStepIterEnd;
	float stepsizeLeft;
	int count;
	unsigned int ptr;
	unsigned int ptrSpeed;
	float leftoverTime = 0.f;

	ptr = posInPoints;
	
	pIter1 = seedTrace->begin();
	if(bRecordSeed)
	{
		assert(ptr < fullArraySize);
		// the first one is seed
		positions[ptr++] = (**pIter1)[0];
		positions[ptr++] = (**pIter1)[1];
		positions[ptr++] = (**pIter1)[2];

		if(speeds != NULL)
		{
			PointInfo pointInfo;
			VECTOR3 nodeData;
			float t;

			ptrSpeed = (ptr-3)/3;
			pointInfo.phyCoord.Set(positions[ptr-3], positions[ptr-2], positions[ptr-1]);
			if(!m_pField->isTimeVarying())
				t = m_pField->GetStartTime();
			else
				t = m_pField->GetStartTime() + m_fSamplingRate*(ptrSpeed-((int)((float)ptrSpeed/(float)m_nMaxsize)*m_nMaxsize));
			m_pField->at_phys(-1, pointInfo.phyCoord, pointInfo, t, nodeData);
			speeds[ptrSpeed] = nodeData.GetMag();
		}
#ifdef DEBUG
		//fprintf(fDebug, "point (%f, %f, %f)\n", positions[ptr-3], positions[ptr-2], positions[ptr-1]);
#endif
	}
	
	count = 1;
	//AN:  Removed this assert, 10/05/05.
	//The problem is that sometimes a point will be very slightly out, and the 
	//bounds test in the runge-kutta is imprecise enough to not detect it.
	//if((int)seedTrace->size() == 1) assert(traceState == CRITICAL_POINT);
	if((int)seedTrace->size() == 1 && traceState != CRITICAL_POINT)
	{
		assert(ptr < fullArraySize);
		positions[ptr] = END_FLOW_FLAG;
		posInPoints = ptr;
		return 0.f;
	}

	// other advecting result
	pIter2 = seedTrace->begin();
	pIter2++;
	pStepIter = stepList->begin();
	stepsizeLeft = *pStepIter + remainingTime;
	if(traceState != OUT_OF_BOUND)
		pStepIterEnd = stepList->end();
	else
	{
		pStepIterEnd = stepList->end();
		pStepIterEnd--;
	}
	while((count < m_nMaxsize) && (pStepIter != pStepIterEnd))
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
			assert(ptr < fullArraySize);
			positions[ptr++] = Lerp((**pIter1)[0], (**pIter2)[0], ratio);
			positions[ptr++] = Lerp((**pIter1)[1], (**pIter2)[1], ratio);
			positions[ptr++] = Lerp((**pIter1)[2], (**pIter2)[2], ratio);

#ifdef DEBUG
			//fprintf(fDebug, "point (%f, %f, %f)\n", positions[ptr-3], positions[ptr-2], positions[ptr-1]);
#endif

			// get the velocity value of this point
			if(speeds != NULL)
			{
				PointInfo pointInfo;
				VECTOR3 nodeData;
				float t;

				ptrSpeed = (ptr-3)/3;
				pointInfo.phyCoord.Set(positions[ptr-3], positions[ptr-2], positions[ptr-1]);
				if(!m_pField->isTimeVarying())
					t = m_pField->GetStartTime();
				else
					t = m_pField->GetStartTime() + m_fSamplingRate*(ptrSpeed-((int)((float)ptrSpeed/(float)m_nMaxsize)*m_nMaxsize));
				m_pField->at_phys(-1, pointInfo.phyCoord, pointInfo, t, nodeData);
				speeds[ptrSpeed] = nodeData.GetMag();
			}

			count++;
		}
	}

	// if # of sampled points < maximal points asked
	int numSampled, whichSeed;
	whichSeed = (ptr-1)/(m_nMaxsize*3);
	numSampled = (ptr - startPositions[whichSeed])/3;
	if((numSampled < m_nMaxsize)&& (traceState == OUT_OF_BOUND))				// out of boundary
	{
	 	pIter1 = seedTrace->end();
		pIter1--;
		assert(ptr < fullArraySize);
		positions[ptr++] = (**pIter1)[0];
		positions[ptr++] = (**pIter1)[1];
		positions[ptr++] = (**pIter1)[2];
		//AN: 10/10/05
		//Set the END_FLOW_FLAG if this is not the last point in the flow:
		if (numSampled < m_nMaxsize -1){
			positions[ptr] = END_FLOW_FLAG;
			assert(ptr < fullArraySize);
		}
		
		if(speeds != NULL)
		{
			ptrSpeed = (ptr-3)/3;
			speeds[ptrSpeed] = speeds[ptrSpeed-1];
		}

	}
	// if # of sampled points < maximal points asked
	else if((numSampled < m_nMaxsize)&& (traceState == CRITICAL_POINT))				// critical pt
	{
		positions[ptr] = STATIONARY_STREAM_FLAG;

		if(speeds != NULL)
		{
			ptrSpeed = ptr/3;
			speeds[ptrSpeed] = 0.f;
		}

	}
	else if(numSampled < m_nMaxsize) {
		positions[ptr] = END_FLOW_FLAG;
		assert(ptr < fullArraySize);
	}

	posInPoints = ptr; 
	return leftoverTime;
}
