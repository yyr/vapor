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

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// definition of class FieldLine
//////////////////////////////////////////////////////////////////////////
vtCFieldLine::vtCFieldLine(CVectorField* pField):
m_nNumSeeds(0),
m_integrationOrder(FOURTH),
m_timeDir(FORWARD),
m_fLowerAngleAccuracy((float)0.99),
m_fUpperAngleAccuracy((float)0.999),
m_nMaxsize(MAX_LENGTH),
m_fInitStepSize(1.0),
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

	/*int retrace = false;
	float angle;
	VECTOR3 p2p1 = p2 - p1;
	VECTOR3 p1p0 = p1 - p0;

	angle = (float)acos(dot(p1p0, p2p1)/(p1p0.GetMag() * p2p1.GetMag()))*(float)RAD_TO_DEG;
	if(angle > m_fUpperAngleAccuracy)
	{
		*dt = (*dt) * (float)0.5;
		retrace = true;
	}
	else
		if(angle < m_fLowerAngleAccuracy)
		{
			*dt = (*dt) * (float)1.25;
			if(*dt >= m_fMaxStepSize)
				*dt = m_fMaxStepSize;
		}

	return retrace;*/
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
			VECTOR3 pos;
			pos.Set(points[3*i+0], points[3*i+1], points[3*i+2]);
			res = m_pField->at_phys(-1, pos, newParticle->m_pointInfo, t, nodeData);
			newParticle->itsValidFlag =  (res == 1) ? 1 : 0 ;
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
			VECTOR3 pos;
			pos.Set(points[3*i+0], points[3*i+1], points[3*i+2]);
			res = m_pField->at_phys(-1, pos, thisSeed->m_pointInfo, t, nodeData);
			thisSeed->itsValidFlag =  (res == 1) ? 1 : 0 ;
		}    
	}

	m_nNumSeeds = numPoints;
}