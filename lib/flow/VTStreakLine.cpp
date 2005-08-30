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
#include "vapor/VaporFlow.h"

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
							unsigned int* pointers,
							bool bInjectSeeds,
							int iInjection,
							float* speeds)
{
	computeStreakLine(userData, positions, pointers, bInjectSeeds, iInjection);
}

void vtCStreakLine::computeStreakLine(const void* userData, 
									  float* points, 
									  unsigned int* pointers,
									  bool bInjectSeeds,
									  int iInjection)
{
	float currentT = *(float*)userData;
	float finalT = currentT + m_timeDir * m_itsTimeInc;
	int numPreParticles;
	vector<vtListParticleIter> deadList;
	int istat;
		
	// how many particles alive from last time
	numPreParticles = (int)m_itsParticles.size();

	// advect the previous old particles
	deadList.clear();
	advectOldParticles(m_itsParticles.begin(), m_itsParticles.end(), points, pointers, currentT, finalT, deadList);

	if(bInjectSeeds)
	{
		unsigned int posInPoints, count;
		count = -1;			// enumerate seed in this injection
		posInPoints = 0;	// enumerate output position in Points array

		// advect the new generated particles from this time step
		vtListParticleIter pIter = m_lSeeds.begin();
		for(; pIter != m_lSeeds.end(); pIter++)
		{
            vtParticleInfo *thisSeed = *pIter;
			vtParticleInfo nextP;
			count++;
			posInPoints = iInjection*(int)m_lSeeds.size()*m_nMaxsize*3 + count * m_nMaxsize * 3;

			if(thisSeed->itsValidFlag == 1)
			{
				vtListSeedTrace* forwardTrace;
				list<float>* stepList;
				forwardTrace = new vtListSeedTrace;
				stepList = new list<float>;
				istat = advectParticle(m_integrationOrder, *thisSeed, currentT, finalT, *forwardTrace, *stepList, true);
				SampleFieldline(points, posInPoints, forwardTrace, stepList);	
				if(points[posInPoints] == END_FLOW_FLAG)
					pointers[iInjection*(int)m_lSeeds.size()+count] = posInPoints;
				
				// for next timestep's advection
				if(istat == 1)
				{
					nextP.m_pointInfo.Set(thisSeed->m_pointInfo);
					nextP.m_fStartTime = currentT;
					nextP.ptId = iInjection*(int)m_lSeeds.size()+count;
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
										unsigned int* pointers,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList)
{
	// advect the old particles first
	vtListParticleIter pIter = start;
	unsigned int posInPoints;
	int istat;
	
	while(pIter != end)
	{
		vtParticleInfo* thisParticle = *pIter;
		vtListSeedTrace* forwardTrace;
		list<float>* stepList;
		forwardTrace = new vtListSeedTrace;
		stepList = new list<float>;
		posInPoints = pointers[thisParticle->ptId];
		istat = advectParticle(m_integrationOrder, *thisParticle, initialTime, finalTime, *forwardTrace, *stepList, true);
		SampleFieldline(points, posInPoints, forwardTrace, stepList);	
		if(points[posInPoints] == END_FLOW_FLAG)
			pointers[thisParticle->ptId] = posInPoints;

		// for next timestep's advection
		if(istat == 1)
			++pIter;
		else				
			deadList.push_back(pIter++);
	}
}