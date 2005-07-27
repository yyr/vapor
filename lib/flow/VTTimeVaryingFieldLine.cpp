/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	TimeVaryingFieldLines												 *
*************************************************************************/
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "VTFieldLine.h"

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// methods common to all time varying field lines
//////////////////////////////////////////////////////////////////////////
vtCTimeVaryingFieldLine::vtCTimeVaryingFieldLine(CVectorField* pField) : 
vtCFieldLine(pField),
m_itsTimeAdaptionFlag(1),
m_itsMaxParticleLife(200),
m_itsTimeInc(1.0)
{
	m_timeDir = FORWARD;
}

vtCTimeVaryingFieldLine::~vtCTimeVaryingFieldLine(void)
{
	releaseParticleMemory();
}

///////////////////////////////////////////////

void vtCTimeVaryingFieldLine::setParticleLife(int steps)
{
	m_itsMaxParticleLife = steps;
}

int vtCTimeVaryingFieldLine::getParticleLife(void)
{
	return m_itsMaxParticleLife;
}

void vtCTimeVaryingFieldLine::releaseParticleMemory(void)
{
	vtListParticleIter pIter = m_itsParticles.begin();
	for( ; pIter != m_itsParticles.end(); ++pIter )
	{
		vtParticleInfo* thisPart = *pIter;
		delete thisPart;
	}
	m_itsParticles.erase(m_itsParticles.begin(), m_itsParticles.end());
}

//////////////////////////////////////////////////////////////////////////
// advect the particle from initialTime to finalTime
//////////////////////////////////////////////////////////////////////////
int vtCTimeVaryingFieldLine::advectParticle(INTEG_ORD int_order, 
											vtParticleInfo& initialPoint,
											float initialTime,
											vtParticleInfo& finalPoint,
											float finalTime)
{
	int istat;
	float curTime, dt;
	PointInfo pt;

	pt = initialPoint.m_pointInfo;

	if(m_itsTimeAdaptionFlag == 1)
		dt = (finalTime - initialTime) * 0.5;
	else
		dt = finalTime - initialTime;

	curTime = initialTime;

	while(curTime < finalTime)
	{
		if(int_order == SECOND)
			istat = runge_kutta2(m_timeDir, UNSTEADY, pt, &curTime, dt);
		else
			istat = runge_kutta4(m_timeDir, UNSTEADY, pt, &curTime, dt);

		if(istat != 1)
			return istat;
	}

	finalPoint.Set(pt, initialTime, 1, initialPoint.itsNumStepsAlive+1, initialPoint.ptId);
	return istat;
}

//////////////////////////////////////////////////////////////////////////
// advect the particle until it goes out of boundary or vel = 0
// return back the track of advection
//////////////////////////////////////////////////////////////////////////
int vtCTimeVaryingFieldLine::advectParticle(INTEG_ORD int_order, 
											vtParticleInfo& initialPoint,
											float initialTime,
											vtListSeedTrace& seedTrace)
{  
	int count = 0, istat, res;
	PointInfo seedInfo;
	PointInfo thisParticle, prevParticle, second_prevParticle;
	float dt, dt_estimate, cell_volume, mag, curTime;
	VECTOR3 vel;

	// the first particle
	seedInfo = initialPoint.m_pointInfo;
	res = m_pField->at_phys(seedInfo.fromCell, seedInfo.phyCoord, seedInfo, initialTime, vel);
	thisParticle = seedInfo;
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = initialTime;
	count++;

	// get the initial stepsize
	switch(m_pField->GetCellType())
	{
	case CUBE:
		dt = dt_estimate = m_fInitStepSize;
		break;

	case TETRAHEDRON:
		cell_volume = m_pField->volume_of_cell(seedInfo.inCell);
		mag = vel.GetMag();
		if(fabs(mag) < 1.0e-6f)
			dt_estimate = 1.0e-5f;
		else
			dt_estimate = pow(cell_volume, (float)0.3333333f) / mag;
		dt = dt_estimate;
		break;

	default:
		break;
	}

	// start to advect
	while(count < m_nMaxsize)
	{
		second_prevParticle = prevParticle;
		prevParticle = thisParticle;

		if(int_order == SECOND)
			istat = runge_kutta2(m_timeDir, UNSTEADY, thisParticle, &curTime, dt);
		else
			istat = runge_kutta4(m_timeDir, UNSTEADY, thisParticle, &curTime, dt);

		if(istat != 1)			// out of boundary
			return -1;
		else
		{
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			count++;
		}

		if((curTime < 0) || (curTime >= (float)(m_pField->GetTimeSteps()-1)))
			return -1;

		if(count > 2)
			adapt_step(second_prevParticle.phyCoord, prevParticle.phyCoord, thisParticle.phyCoord, dt_estimate, &dt);
	}
}

