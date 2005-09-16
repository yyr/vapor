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
#ifdef DEBUG
	extern FILE* fDebug;
#endif

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
void vtCStreakLine::execute(const float t, 
							float* positions, 
							const unsigned int* startPositions, 
							unsigned int* pointers,
							bool bInjectSeeds,
							int iInjection,
							float* speeds)
{
	computeStreakLine(t, positions, startPositions, pointers, bInjectSeeds, iInjection, speeds);
}

void vtCStreakLine::computeStreakLine(const float t, 
									  float* points, 
									  const unsigned int* startPositions, 
									  unsigned int* pointers,
									  bool bInjectSeeds,
									  int iInjection,
									  float* speeds)
{
	float currentT = t;
	float finalT = currentT + m_timeDir;
	int numPreParticles;
	vector<vtListParticleIter> deadList;
	int istat;

	// how many particles alive from last time
	numPreParticles = (int)m_itsParticles.size();
#ifdef DEBUG
	fprintf(fDebug, "**********************Advect Old Particles**************************\n");
#endif

	// advect the previous old particles
	deadList.clear();
	//m_itsParticles is the current list of particles being advected.
	advectOldParticles( m_itsParticles.begin(), 
						m_itsParticles.end(), 
						points, 
						startPositions, 
						pointers, 
						currentT, 
						finalT, 
						deadList,
						speeds);

	if(bInjectSeeds)
	{

#ifdef DEBUG
		fprintf(fDebug, "**********************Advect New Particles**************************\n");
#endif

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
				istat = advectParticle( m_integrationOrder, 
										*thisSeed, 
										nextP, 
										currentT, 
										finalT, 
										*forwardTrace, 
										*stepList, 
										true);

#ifdef DEBUG
				fprintf(fDebug, "************************************************\n");
				fprintf(fDebug, "Start(%f, %f, %f), end(%f, %f, %f)\n", 
						thisSeed->m_pointInfo.phyCoord[0], 
						thisSeed->m_pointInfo.phyCoord[1], 
						thisSeed->m_pointInfo.phyCoord[2], 
						nextP.m_pointInfo.phyCoord[0], 
						nextP.m_pointInfo.phyCoord[1], 
						nextP.m_pointInfo.phyCoord[2]);
#endif

				SampleFieldline(points, startPositions, posInPoints, forwardTrace, stepList, true, istat, speeds);	
				if(points[posInPoints] == END_FLOW_FLAG)
					pointers[iInjection*(int)m_lSeeds.size()+count] = posInPoints;

#ifdef DEBUG
				fprintf(fDebug, "istat = %d, posInPoints = %u\n", istat, posInPoints);
#endif

#ifdef DEBUG
				fprintf(fDebug, "************************************************\n");
#endif

				// for next timestep's advection
				if(istat == OKAY)
				{
					nextP.m_fStartTime = currentT;
					nextP.ptId = iInjection*(int)m_lSeeds.size()+count;
					m_itsParticles.push_back(new vtParticleInfo(nextP));
				}
			}
			else                // out of data region.  Just mark seed as end, don't advect.
			{
				points[posInPoints++] = thisSeed->m_pointInfo.phyCoord.x();
				points[posInPoints++] = thisSeed->m_pointInfo.phyCoord.y();
				points[posInPoints++] = thisSeed->m_pointInfo.phyCoord.z();
				points[posInPoints] = END_FLOW_FLAG;
			}
		}
	}

	// process those particles that have exited the region.
	// These are removed from m_itsParticles, so they won't be advected again.
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
										const unsigned int* startPositions, 
										unsigned int* pointers,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList,
										float* speeds)
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

#ifdef DEBUG
		fprintf(fDebug, "************************************************\n");
		fprintf(fDebug, "Start(%f, %f, %f)", 
				thisParticle->m_pointInfo.phyCoord[0], 
				thisParticle->m_pointInfo.phyCoord[1], 
				thisParticle->m_pointInfo.phyCoord[2]);
#endif

		istat = advectParticle( m_integrationOrder,
								*thisParticle, 
								*thisParticle, 
								initialTime, 
								finalTime, 
								*forwardTrace, 
								*stepList, 
								true);

#ifdef DEBUG
		fprintf(fDebug, "End(%f, %f, %f)\n", 
				thisParticle->m_pointInfo.phyCoord[0], 
				thisParticle->m_pointInfo.phyCoord[1], 
				thisParticle->m_pointInfo.phyCoord[2]);
#endif

		SampleFieldline(points, startPositions, posInPoints, forwardTrace, stepList, false, istat, speeds);	
		if(points[posInPoints] == END_FLOW_FLAG)
			pointers[thisParticle->ptId] = posInPoints;

#ifdef DEBUG
		fprintf(fDebug, "istat = %d, posInPoints = %u\n", istat, posInPoints);
#endif

#ifdef DEBUG
		fprintf(fDebug, "************************************************\n");
#endif

		// for next timestep's advection
		if(istat == OKAY)
			++pIter;
		else				
			deadList.push_back(pIter++);
	}
}