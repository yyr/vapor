/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	StreakLines															 *
*************************************************************************/
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "VTFieldLine.h"

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// class implementation of Numerical StreakLine
//////////////////////////////////////////////////////////////////////////

vtCStreakLine::vtCStreakLine(CVectorField* pField) : 
vtCTimeVaryingFieldLine(pField)
{
}

vtCStreakLine::~vtCStreakLine(void)
{
}

//////////////////////////////////////////////////////////////////////////
// this is an animated process
// input: 
// userData: current time
// listSeedTraces:	the output position of particles for the current time
//					step
//////////////////////////////////////////////////////////////////////////
void vtCStreakLine::execute(const void* userData, 
							float* positions, 
							bool bInjectSeeds,
							int iInjection)
{
	computeStreakLine(userData, positions, bInjectSeeds, iInjection);
}

void vtCStreakLine::computeStreakLine(const void* userData, 
									  float* points, 
									  bool bInjectSeeds,
									  int iInjection)
{
	float currentT = *(float*)userData;
	float finalT = currentT + m_timeDir * m_itsTimeInc;
	int numPreParticles;
	vector<vtListParticleIter> deadList;
	
	// how many particles alive from last time
	numPreParticles = (int)m_itsParticles.size();

	// advect the previous old particles
	deadList.clear();
	advectOldParticles(m_itsParticles.begin(), m_itsParticles.end(), points, currentT, finalT, deadList);

	if(bInjectSeeds)
	{
		// advect the new generated particles from this time step
		vtListParticleIter pIter = m_lSeeds.begin();
		int index = iInjection*(int)m_lSeeds.size();
		for(; pIter != m_lSeeds.end(); pIter++, index++)
		{
			vtParticleInfo *thisSeed = *pIter;
			vtParticleInfo nextP;

			if(thisSeed->itsValidFlag == 1)
			{
				int res = advectParticle(m_integrationOrder, *thisSeed, currentT, nextP, finalT);
				if(res == 1)
				{
					// for output
					int offset;
					offset = index*m_itsMaxParticleLife*3;
					points[offset++] = nextP.m_pointInfo.phyCoord[0];
					points[offset++] = nextP.m_pointInfo.phyCoord[1];
					points[offset++] = nextP.m_pointInfo.phyCoord[2];
					points[offset++] = 1.0e+30f;
					
					// for next timestep's advection
					nextP.ptId = index;
					m_itsParticles.push_back(new vtParticleInfo(nextP));
				}
			}
		}
	}

	// process those dead particles
	vector<vtListParticleIter>::iterator deadIter = deadList.begin();
	for(; deadIter != deadList.end(); deadIter++)
	{
		vtListParticleIter v = *deadIter;
		vtParticleInfo* pi = *v;
		delete pi;
		m_itsParticles.erase(v);
	}
}

//////////////////////////////////////////////////////////////////////////
// advect those particles from previous steps
//////////////////////////////////////////////////////////////////////////
void vtCStreakLine::advectOldParticles( vtListParticleIter start, 
										vtListParticleIter end, 
										float* points,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList)
{
	// advect the old particles first
	vtListParticleIter pIter = start;
	while(pIter != end)
	{
		vtParticleInfo* thisParticle = *pIter;

		// kill this particle, too old
		if(thisParticle->itsNumStepsAlive > m_itsMaxParticleLife)
		{
			deadList.push_back(pIter++);
			continue;
		}

		// advect more
		int res = advectParticle(m_integrationOrder, *thisParticle, initialTime, *thisParticle, finalTime);

		if(res == 1)
		{
			int offset;
			offset = thisParticle->ptId*m_itsMaxParticleLife*3 + thisParticle->itsNumStepsAlive*3;
			points[offset++] = thisParticle->m_pointInfo.phyCoord[0];
			points[offset++] = thisParticle->m_pointInfo.phyCoord[1];
			points[offset++] = thisParticle->m_pointInfo.phyCoord[2];
			points[offset++] = 1.0e+30f;
			++thisParticle->itsNumStepsAlive;
			++pIter;
		}
		else
			deadList.push_back(pIter++);
	}
}