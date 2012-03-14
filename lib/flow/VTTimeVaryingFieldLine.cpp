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
	int istat = -1;
	double curTime, dt;
	PointInfo pt;
	assert(0);   // This code is not used in VAPOR
	pt = initialPoint.m_pointInfo;

	if(m_itsTimeAdaptionFlag == 1)
		dt = (finalTime - initialTime) * 0.5;
	else
		dt = finalTime - initialTime;

	curTime = initialTime;

	while((float)curTime < finalTime)
	{
		if(int_order == SECOND)
			istat = runge_kutta2(m_timeDir, UNSTEADY, pt, &curTime, dt,0.);
		else
			istat = runge_kutta4(m_timeDir, UNSTEADY, pt, &curTime, dt,0.);

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
											bool bAdaptive)//not always true!!
{  
	int istat;
	PointInfo seedInfo;
	PointInfo thisParticle;
	VECTOR3 thisInterpolant, prevInterpolant, second_prevInterpolant;
	double dt, cell_side, mag; 
	double curTime;
	VECTOR3 vel;
	int nSetAdaptiveCount = 0;

	// the first particle
	seedInfo = initialPoint.m_pointInfo;
	thisParticle = seedInfo;
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = initialTime;
	istat = m_pField->getFieldValue(seedInfo.phyCoord, initialTime, vel);
	
	if(istat == OUT_OF_BOUND)
		return OUT_OF_BOUND;
	
	// get the initial stepsize
	//cell_volume = m_pField->volume_of_cell(seedInfo.inCell);
	mag = vel.GetDMag();
	dt = m_pField->GetMaxMinGridSpacing()/mag;
	//dt = pow(cell_volume, 0.3333333) / mag;
	//Allow dt*mag to be 10 times the initial setting:
	float maxDtMag = dt*mag*10.f;

	int currentProgress = 0;
	int maxProgress = 0;
	
	// start to advect
	
	while(curTime*m_timeDir < finalTime*m_timeDir)
	{
		// how much advection time left
		double timeLeft;
		if (m_timeDir == FORWARD) {
			timeLeft = (finalTime - curTime);
		} else {
			timeLeft = curTime - finalTime;
		}
		if (dt > timeLeft) dt = timeLeft;
		if (timeLeft < 1.e-6) break;
		
		
		
		
		second_prevInterpolant = prevInterpolant;
		prevInterpolant = thisInterpolant;
		int retrace = true;
		
		while(retrace)
		{
			retrace = false;
			//Loop until dt*mag is small enough, dividing dt by 10 if necessary
			for (int tries = 0; tries < 40; tries++){
				if(int_order == SECOND)
					istat = runge_kutta2(m_timeDir, UNSTEADY, thisParticle, &curTime, dt,maxDtMag);
				else
					istat = runge_kutta4A(m_timeDir, UNSTEADY, thisParticle, &curTime, dt,maxDtMag);
				if (istat != FIELD_TOO_BIG) break;
				//Must retry.  Reset curTime, use a smaller dt.
				curTime -= dt;
				dt = dt *0.1;
			}
			assert(istat != FIELD_TOO_BIG);
			thisInterpolant = thisParticle.interpolant;
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			stepList.push_back(dt);
			
			currentProgress++;
			if (currentProgress > maxProgress) {
				maxProgress = currentProgress;
			}
			
			
			if(istat == OUT_OF_BOUND)			// out of boundary
				return OUT_OF_BOUND;

			//m_pField->at_phys(thisParticle.fromCell, thisParticle.phyCoord, thisParticle, curTime, vel);
			// Don't test for critical points on streaklines!
			m_pField->getFieldValue(thisParticle.phyCoord, curTime, vel);

			nSetAdaptiveCount++;
			//nSetAdaptiveCount counts the number of advections since the last time
			//the adaptive step size was at the minimum, or from the very start.
			//The following test says that we turn adaptive integration back on
			//two steps after the minimum stepsize is attained.
			//We could avoid this by not turning off adaptive integration at all,
			//Since, when we are at minimum stepsize, retrace will be false
			if((nSetAdaptiveCount == 2) && (bAdaptive == false))
				bAdaptive = true;

			// just generate valid new point
			if(((int)seedTrace.size() > 2) && (bAdaptive))
			{
				double minStepsize, maxStepsize;
				VECTOR3 thisPhy, prevPhy, second_prevPhy;
				list<VECTOR3*>::iterator pIter = seedTrace.end();
				pIter--;
				thisPhy = **pIter;
				pIter--;
				prevPhy = **pIter;
				pIter--;
				second_prevPhy = **pIter;
			    cell_side = m_pField->GetMaxMinGridSpacing();
				//cell_volume = m_pField->volume_of_cell(thisParticle.inCell);
				mag = vel.GetDMag();
				minStepsize = m_fInitStepSize * cell_side / mag;
				maxStepsize = m_fMaxStepSize * cell_side / mag;
				retrace = adapt_step(second_prevPhy, prevPhy, thisPhy, minStepsize, maxStepsize, &dt, bAdaptive);
				
				if(bAdaptive == false)
					nSetAdaptiveCount = 0;

				// If the stepsize was reduced, 
				// roll back and retrace, but only if we have come back two steps from
				// the last pop_back
				if(retrace && (currentProgress >= maxProgress))	//The stepsize was reduced.		
				{
					thisInterpolant = prevInterpolant = second_prevInterpolant;
					seedTrace.pop_back();
					seedTrace.pop_back();
					thisParticle.Set(*(seedTrace.back()), thisInterpolant, -1, -1);
					list<float>::iterator pIter = stepList.end();
					pIter--;
					curTime -= (*pIter)*m_timeDir;
					
					pIter--;
					curTime -= (*pIter)*m_timeDir;
					
					stepList.pop_back();
					stepList.pop_back();
					currentProgress -= 2;
				}
			}
		}// end of retrace
	}// end of advection
	
	finalPoint.m_pointInfo.Set(thisParticle);
	return 1;
}

