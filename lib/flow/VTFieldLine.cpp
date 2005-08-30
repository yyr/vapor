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
	printf("called in vtCFieldLine\n");
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
	printf("Release memory!\n");
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

	return istat;
}

//////////////////////////////////////////////////////////////////////////
// Integrate along a field line using the 4th order Runge-Kutta method.
// This routine is used for both steady and unsteady vector fields. 
//////////////////////////////////////////////////////////////////////////
int vtCFieldLine::runge_kutta4(TIME_DIR time_dir, TIME_DEP time_dep, 
							   PointInfo& ci, 
							   float* t,			// initial time
							   float dt,			// stepsize
							   float *diff)			
{
	int i, istat;
	VECTOR3 pt0;
	VECTOR3 vel;
	VECTOR3 k1, k2, k3, k4;
	VECTOR3 pt;
	int fromCell;

	pt = ci.phyCoord;
	// 1st step of the Runge-Kutta scheme
	istat = m_pField->at_phys(ci.fromCell, pt, ci, *t, vel);

	if ( istat != 1 )
		return( istat );

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
		return( istat );

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
		return( istat );
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
		return( istat );
	}

	for( i=0; i<3; i++ )
	{
		pt[i] = pt0[i]+(k1[i]+(float)2.0*(k2[i]+k3[i])+time_dir*dt*vel[i])/(float)6.0;
		k4[i] = time_dir*dt*vel[i];
	}
	ci.phyCoord = pt;

	istat = m_pField->at_phys(ci.fromCell, pt, ci, *t, vel);
	*diff = (k4[0]-dt*time_dir*vel[0])*(k4[0]-dt*time_dir*vel[0]) + 
		(k4[1]-dt*time_dir*vel[1])*(k4[1]-dt*time_dir*vel[1]) +
		(k4[2]-dt*time_dir*vel[2])*(k4[2]-dt*time_dir*vel[2]);
	*diff = sqrt(*diff);

	return( istat );
}

int vtCFieldLine::runge_kutta4(TIME_DIR time_dir, TIME_DEP time_dep, 
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
		return( istat );
	
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
		return( istat );
	
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
		return( istat );
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
		return( istat );
	}

	for( i=0; i<3; i++ )
	{
		pt[i] = pt0[i]+(k1[i]+(float)2.0*(k2[i]+k3[i])+time_dir*dt*vel[i])/(float)6.0;
	}
	ci.phyCoord = pt;

	return( istat );
}

//////////////////////////////////////////////////////////////////////////
// aptive step size
//////////////////////////////////////////////////////////////////////////
#define STREAM_ACCURACY 1.0e-14

// according to curvature
int vtCFieldLine::adapt_step(const VECTOR3& p2,
							 const VECTOR3& p1,
							 const VECTOR3& p0,
							 float dt_estimate,
							 float* dt)
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
	}
	else if(cos_p2p1p0 > m_fUpperAngleAccuracy)
	{
		*dt = (*dt) * (float)1.25;
		//if(*dt >= m_fMaxStepSize)
		//	*dt = m_fMaxStepSize;
	}
	
	return retrace;
}

// according to integration difference, FastLIC paper
int vtCFieldLine::adapt_step(const float diff, const float accuracy, float* dt)
{
	float delta = diff / 6.0;
	int retrace = false;

	// regular Cartesian Grid, so cells have same spacing
	float gridSpacing = m_pField->GetGridSpacing(0)*accuracy;

	if(delta > (gridSpacing))
	{
		retrace = true;
		*dt = pow(*dt, 2) * sqrt(0.75 * (gridSpacing/delta));
	}

	return retrace;
}
//////////////////////////////////////////////////////////////////////////
// initialize seeds
//////////////////////////////////////////////////////////////////////////
void vtCFieldLine::setSeedPoints(float* points, int numPoints, float t)
{
	int i, res;
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
			
			// query the field in order to get the starting cell interpolant for the seed point
			//VECTOR3 pos;
			//pos.Set(points[3*i+0], points[3*i+1], points[3*i+2]);
			//res = m_pField->at_phys(-1, pos, newParticle->m_pointInfo, t, nodeData);
			//newParticle->itsValidFlag =  (res == 1) ? 1 : 0 ;
			newParticle->itsValidFlag = 1;
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
//////////////////////////////////////////////////////////////////////////
void vtCFieldLine::SampleFieldline(float* positions,
								   unsigned int& posInPoints,
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   float* speeds)
{
	list<VECTOR3*>::iterator pIter1;
	list<VECTOR3*>::iterator pIter2;
	list<float>::iterator pStepIter;
	float stepsizeLeft;
	int count;
	unsigned int ptr;
	unsigned int ptrSpeed;

	ptr = posInPoints;
	ptrSpeed = posInPoints/3;

	pIter1 = seedTrace->begin();
	// the first one is seed
	positions[ptr++] = (**pIter1)[0];
	positions[ptr++] = (**pIter1)[1];
	positions[ptr++] = (**pIter1)[2];
	count = 1;
	if((int)seedTrace->size() == 1)
	{
		positions[ptr] = END_FLOW_FLAG;
		posInPoints = ptr;
		return;
	}

	// other advecting result
	pIter2 = seedTrace->begin();
	pIter2++;
	pStepIter = stepList->begin();
	stepsizeLeft = *pStepIter;
	while((count < m_nMaxsize) && (pStepIter != stepList->end()))
	{
		float ratio;
		if(stepsizeLeft < m_fSamplingRate)
		{
			pIter1++;
			pIter2++;
			pStepIter++;
			stepsizeLeft += *pStepIter;
		}
		else
		{
			stepsizeLeft -= m_fSamplingRate;
			ratio = (*pStepIter - stepsizeLeft)/(*pStepIter);
			positions[ptr++] = Lerp((**pIter1)[0], (**pIter2)[0], ratio);
			positions[ptr++] = Lerp((**pIter1)[1], (**pIter2)[1], ratio);
			positions[ptr++] = Lerp((**pIter1)[2], (**pIter2)[2], ratio);
			count++;
		}
	}

	// if # of sampled points < maximal points asked
	if(count < m_nMaxsize)
		positions[ptr] = END_FLOW_FLAG;

	posInPoints = ptr; 
}