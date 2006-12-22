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
	
	vector<vtListParticleIter> deadList;
	int istat;

	
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

		unsigned int posInPoints;
		int count;
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
	
				nextP.unusedTime = SampleFieldline(points, startPositions, posInPoints, forwardTrace, stepList, true, istat, speeds, 0.f);	
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
		float timeLeft = thisParticle->unusedTime;
		thisParticle->unusedTime = SampleFieldline(points, startPositions, posInPoints, forwardTrace, stepList, false, istat, speeds, timeLeft);	
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

//////////////////////////////////////////////////////////////////////////
// this is an animated process
// input: 
// userData: current time
// listSeedTraces:	the output position of particles for the current time
//					step
//////////////////////////////////////////////////////////////////////////
void vtCStreakLine::execute(const float t, 
							PathLineData* container, 
							bool bInjectSeeds)
							
							
{
	computeStreakLine(t, container, bInjectSeeds);
}
////////////////////////////////////////////////
// New version uses path line data and lineNums
//////////////////////////////////////////////
void vtCStreakLine::computeStreakLine(const float t, 
									  PathLineData* container,
									  bool bInjectSeeds)
{
	float currentT = t;
	float finalT = currentT + m_timeDir;
	
	vector<vtListParticleIter> deadList;
	int istat;
	int dir = container->getFlowDirection();
	
#ifdef DEBUG
	fprintf(fDebug, "**********************Advect Old Particles**************************\n");
#endif

	// advect the previous old particles
	deadList.clear();
	//m_itsParticles is the current list of particles being advected.
	advectOldParticles( m_itsParticles.begin(), 
						m_itsParticles.end(), 
						container,
						currentT, 
						finalT, 
						deadList);

	if(bInjectSeeds)
	{

#ifdef DEBUG
		fprintf(fDebug, "**********************Advect New Particles**************************\n");
#endif

		int count;
		count = -1;			// enumerate seed in this injection

		// advect the new generated particles from this time step
		vtListParticleIter pIter = m_lSeeds.begin();
		for(; pIter != m_lSeeds.end(); pIter++)
		{
            vtParticleInfo *thisSeed = *pIter;
			vtParticleInfo nextP;
			count++;

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
	
				nextP.unusedTime = SampleFieldline(container, currentT, finalT, thisSeed->ptId, dir, forwardTrace, stepList, true, istat, 0.f);	

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
					nextP.ptId = thisSeed->ptId;
					m_itsParticles.push_back(new vtParticleInfo(nextP));
				}
			}
			else                // seed is out of data region.  Just mark seed as end, don't advect.
			{
				container->setPointAtTime(thisSeed->ptId, currentT, 
					thisSeed->m_pointInfo.phyCoord.x(),
					thisSeed->m_pointInfo.phyCoord.y(),
					thisSeed->m_pointInfo.phyCoord.z());
				//Indicate that the path ends here (at least for this direction)
				container->setFlowEndAtTime(thisSeed->ptId, currentT);
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
// New version, uses PathLineData, lineNums indicating which
// lines in the container are active.
//////////////////////////////////////////////////////////////////////////
void vtCStreakLine::advectOldParticles( vtListParticleIter start, 
										vtListParticleIter end, 
										PathLineData* container,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList)
										
{
	// advect the old particles first
	vtListParticleIter pIter = start;
	int istat;
	int dir = container->getFlowDirection();
	assert((initialTime > finalTime && dir < 0 )|| (initialTime < finalTime && dir > 0));
	
	
	while(pIter != end)
	{
		vtParticleInfo* thisParticle = *pIter;
		vtListSeedTrace* forwardTrace;
		list<float>* stepList;
		forwardTrace = new vtListSeedTrace;
		stepList = new list<float>;

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
		float timeLeft = thisParticle->unusedTime;
		thisParticle->unusedTime = SampleFieldline(container, initialTime, finalTime, thisParticle->ptId, dir, forwardTrace, stepList, false, istat, timeLeft);	
		

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
//////////////////////////////////////////////////////////////////////////
// AN:  initialize seeds from container.  replaces VtFieldLine::SetSeedPoints().
// The 'seeds' are really all points in the container that aren't already in the
// pStreakLine, not necessarily identified as seeds in the container.
/////////////////////////////////////////////////////////////////////////
int vtCStreakLine::addSeeds(int tstep, PathLineData* container)
{
	//release previous seeds
	releaseSeedMemory();
	//Look for any points in the container that start at the specified time step, but
	//not already specified in the "old" particle list.  These are exactly the points that
	//are defined at the specific time step, whose lineNum is not used by another particle.
	int totLines = container->getNumLines();
	
	//Make a flag array to identify the currently used lines:
	bool* usedLines = new bool[totLines];

	for (int i = 0; i< totLines; i++) usedLines[i] = false;
	for (vtListParticleIter pIter = m_itsParticles.begin();pIter != m_itsParticles.end(); pIter++) {
		vtParticleInfo *thisPoint = *pIter;
		usedLines[thisPoint->ptId] = true;
	}
	
	int numSeeds = 0;
	
	for (int line = 0; line < container->getNumLines(); line++){
		//Ignore lines that are already used..
		if (usedLines[line]) continue;
		//Test that the current time step is between the current start and end of the line..
		if (container->getFirstTimestep(line) <= tstep && container->getLastTimestep(line) >= tstep ) {
			//Found a seed.  
			vtParticleInfo* newParticle = new vtParticleInfo;
			float* point = container->getPointAtTime(line, (float)tstep);
			newParticle->m_pointInfo.phyCoord = VECTOR3(point[0], point[1], point[2]);
			newParticle->m_fStartTime = (float)tstep;
			newParticle->ptId = line;
			newParticle->itsNumStepsAlive = 0;
			
			//Here we test whether the seed particle is in the region
			//If it's out, we set the flag to invalid.  At the first step
			//of the advection, the invalid seeds get marked as being out of region.
			//Note that invalid seeds are still inserted into the seed list.
			
			if (!m_pField->is_in_grid(newParticle->m_pointInfo.phyCoord)) {
				newParticle->itsValidFlag = 0;
				
			} else newParticle->itsValidFlag = 1;
			
			m_lSeeds.push_back(newParticle);
			numSeeds++;
		}
	}
	m_nNumSeeds = numSeeds;
	delete usedLines;
	return numSeeds;
}
