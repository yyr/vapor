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
#ifdef DEBUG
	extern FILE* fStreakDebug;
#endif

vtCTimeVaryingFieldLine::vtCTimeVaryingFieldLine(CVectorField* pField) : 
vtCFieldLine(pField),
m_itsTimeAdaptionFlag(1),
m_itsMaxParticleLife(200),
m_itsTimeInc(1.0)
{
	m_timeDir = FORWARD;
	m_itsParticles.clear();
}

vtCTimeVaryingFieldLine::~vtCTimeVaryingFieldLine(void)
{
	releaseParticleMemory();
}

///////////////////////////////////////////////

void vtCTimeVaryingFieldLine::setParticleLife(int steps)
{
	m_nMaxsize = steps;
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
											float finalTime,
											bool bAdaptive)
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
											vtParticleInfo& finalPoint,
											float initialTime,
											float finalTime,
											vtListSeedTrace& seedTrace,
											list<float>& stepList,
											bool bAdaptive)
{  
	int istat, res;
	PointInfo seedInfo;
	PointInfo thisParticle;
	VECTOR3 thisInterpolant, prevInterpolant, second_prevInterpolant;
	float dt, dt_estimate, cell_volume, mag, curTime;
	VECTOR3 vel;

	// the first particle
	seedInfo = initialPoint.m_pointInfo;
	thisParticle = seedInfo;
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = initialTime;

	// get the initial stepsize
	res = m_pField->at_phys(seedInfo.fromCell, seedInfo.phyCoord, seedInfo, initialTime, vel);
	if(res == -1)
		return -1;
	if((abs(vel[0]) < EPS) && (abs(vel[1]) < EPS) && (abs(vel[2]) < EPS))
		return -1;

	cell_volume = m_pField->volume_of_cell(seedInfo.inCell);
	mag = vel.GetMag();
	dt = pow(cell_volume, (float)0.3333333f) / mag;
	
	// start to advect
	while(curTime < finalTime)
	{
		// how much advection time left
		if((finalTime-curTime)<dt)
			dt = finalTime-curTime;

		second_prevInterpolant = prevInterpolant;
		prevInterpolant = thisInterpolant;
		int retrace = true;

		while(retrace)
		{
			retrace = false;

			if(int_order == SECOND)
				istat = runge_kutta2(m_timeDir, UNSTEADY, thisParticle, &curTime, dt);
			else
				istat = runge_kutta4(m_timeDir, UNSTEADY, thisParticle, &curTime, dt);

			if(istat != 1)			// out of boundary
				return -1;

			m_pField->at_phys(thisParticle.fromCell, thisParticle.phyCoord, thisParticle, curTime, vel);
			if((abs(vel[0]) < EPS) && (abs(vel[1]) < EPS) && (abs(vel[2]) < EPS))
				return -1;
						
			thisInterpolant = thisParticle.interpolant;
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			stepList.push_back(dt);
			
			if(bAdaptive)
			{
				// just generate valid new point
				if((int)seedTrace.size() > 2)
				{
					float minStepsize, maxStepsize;
					VECTOR3 thisPhy, prevPhy, second_prevPhy;
					list<VECTOR3*>::iterator pIter = seedTrace.end();
					pIter--;
					thisPhy = **pIter;
					pIter--;
					prevPhy = **pIter;
					pIter--;
					second_prevPhy = **pIter;

					cell_volume = m_pField->volume_of_cell(thisParticle.inCell);
					mag = vel.GetMag();
					minStepsize = m_fInitStepSize * pow(cell_volume, (float)0.3333333f) / mag;
					maxStepsize = m_fMaxStepSize * pow(cell_volume, (float)0.3333333f) / mag;
					retrace = adapt_step(second_prevPhy, prevPhy, thisPhy, minStepsize, maxStepsize, &dt);
				}

				// roll back and retrace
				if(retrace == true)			
				{
					thisInterpolant = prevInterpolant = second_prevInterpolant;
					seedTrace.pop_back();
					seedTrace.pop_back();
					thisParticle.Set(*(seedTrace.back()), thisInterpolant, -1, -1);
					list<float>::iterator pIter = stepList.end();
					pIter--;
					curTime -= *pIter;
					pIter--;
					curTime -= *pIter;
					stepList.pop_back();
					stepList.pop_back();
				}
			}
		}// end of retrace
	}// end of advection

	finalPoint.m_pointInfo.Set(thisParticle);
	return 1;
}

