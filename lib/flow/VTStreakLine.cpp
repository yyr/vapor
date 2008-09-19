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
#include "vapor/errorcodes.h"

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
void vtCStreakLine::execute(const float t, 
							PathLineData* container, 
							bool bInjectSeeds,
							bool doingFLA)
							
							
{
	computeStreakLine(t, container, bInjectSeeds, doingFLA);
}
////////////////////////////////////////////////
// New version uses path line data and lineNums
//////////////////////////////////////////////
void vtCStreakLine::computeStreakLine(const float t, 
									  PathLineData* container,
									  bool bInjectSeeds,
									  bool doingFLA)
{
	float currentT = t;
	float finalT = currentT + m_timeDir;
	int numPointsAdvected;
	vector<vtListParticleIter> deadList;
	int istat;
	int dir = container->getFlowDirection();
	


	// advect the previous old particles
	deadList.clear();
	//m_itsParticles is the current list of particles being advected.
	numPointsAdvected = advectOldParticles( m_itsParticles.begin(), 
						m_itsParticles.end(), 
						container,
						currentT, 
						finalT, 
						deadList,
						doingFLA);

	if(bInjectSeeds)
	{



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


	
				nextP.unusedTime = SampleFieldline(container, currentT, finalT, thisSeed->ptId, dir, 
					forwardTrace, stepList, true, istat, 0.f, doingFLA);	

				// for next timestep's advection
				if(istat == OKAY)
				{
					numPointsAdvected++;
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
				if (container->getFlowDirection() > 0)
					container->setFlowEndAtTime(thisSeed->ptId, currentT);
				else 
					container->setFlowStartAtTime(thisSeed->ptId, currentT);
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
	if(numPointsAdvected == 0) {
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"All unsteady flow lines have exited region.");
	}
}

////////////////////////////////////////////////
// Like above, but puts the result of advection into FlowLineData array
// 
//////////////////////////////////////////////
void vtCStreakLine::advectFLAPoints(int tstep, int flowDir, FlowLineData** flData, bool bInjectSeeds)
{
	float currentT = (float)tstep;
	float finalT = currentT + m_timeDir;
	int numPointsAdvected;
	vector<vtListParticleIter> deadList;
	int istat;
	int numLines = flData[tstep]->getNumLines();
	//The first time, we will advect the seeds, subsequently just
	//advect old particles

	// advect the previous old particles
	deadList.clear();
	//m_itsParticles is the current list of particles being advected.
	//advectOldParticles puts its results in the container:
	numPointsAdvected = advectOldParticles( m_itsParticles.begin(), 
						m_itsParticles.end(), 
						flData,
						currentT, 
						finalT, 
						deadList);
	if(bInjectSeeds)
	{
		assert(numPointsAdvected == 0);
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
				//Get the linenum, pointnum:
				int lineNum = (thisSeed->ptId)%numLines;
				int pointNum = ((thisSeed->ptId) - lineNum)/numLines;
				nextP.unusedTime = SampleFLALine(flData, currentT, finalT,lineNum,pointNum, flowDir, 
					forwardTrace, stepList, istat, 0.f);	

				// if the point survives, prepare for next timestep's advection, by
				// inserting it into the next time step:
				if(istat == OKAY)
				{
					numPointsAdvected++;
					nextP.m_fStartTime = currentT;
					nextP.ptId = thisSeed->ptId;
					m_itsParticles.push_back(new vtParticleInfo(nextP));
				}
			}
			else                // seed is out of data region.  
				//This shouldn't really happen, since the seeds were sampled from
				//the valid points
			{
				assert(0);
			}
		}
	}

	// Next, process those particles that have exited the region.
	// These are removed from m_itsParticles, so they won't be advected again.
	vector<vtListParticleIter>::iterator deadIter = deadList.begin();
	for(; deadIter != deadList.end(); deadIter++)
	{
		vtListParticleIter v = *deadIter;
		vtParticleInfo* pi = *v;
		delete pi;
		m_itsParticles.erase(v);
	}
	if(numPointsAdvected == 0) {
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"No field lines remain in region\nat time step %d",
			(int)finalT);
	}
}

//////////////////////////////////////////////////////////////////////////
// advect those particles from previous steps
// New version, uses PathLineData, lineNums indicating which
// lines in the container are active.
//////////////////////////////////////////////////////////////////////////
int vtCStreakLine::advectOldParticles( vtListParticleIter start, 
										vtListParticleIter end, 
										PathLineData* container,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList,
										bool doingFLA)
										
{
	int numAdvected = 0;
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

		istat = advectParticle( m_integrationOrder,
								*thisParticle, 
								*thisParticle, 
								initialTime, 
								finalTime, 
								*forwardTrace, 
								*stepList, 
								true);


		float timeLeft = thisParticle->unusedTime;
		thisParticle->unusedTime = SampleFieldline(container, initialTime, finalTime, 
			thisParticle->ptId, dir, forwardTrace, stepList, 
			false, istat, timeLeft, doingFLA);	
		

		// for next timestep's advection
		if(istat == OKAY){
			++pIter;
			numAdvected++;
		}
		else				
			deadList.push_back(pIter++);
	}
	return numAdvected;
}
//////////////////////////////////////////////////////////////////////////
// advect those particles from previous steps
// Newer version, uses array of FlowLineData, for field line advection
// lines in the container are active.
//////////////////////////////////////////////////////////////////////////
int vtCStreakLine::advectOldParticles( vtListParticleIter start, 
										vtListParticleIter end, 
										FlowLineData** flArray,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList)
										
{
	int numAdvected = 0;
	// advect the old particles first
	vtListParticleIter pIter = start;
	int istat;
	int dir = (initialTime > finalTime) ? -1 : 1 ;
	assert(initialTime != finalTime);
	
	int numLines = flArray[(int)initialTime]->getNumLines();
	while(pIter != end)
	{
		vtParticleInfo* thisParticle = *pIter;
		vtListSeedTrace* forwardTrace;
		list<float>* stepList;
		forwardTrace = new vtListSeedTrace;
		stepList = new list<float>;

		istat = advectParticle( m_integrationOrder,
								*thisParticle, 
								*thisParticle, 
								initialTime, 
								finalTime, 
								*forwardTrace, 
								*stepList, 
								true);


		float timeLeft = thisParticle->unusedTime;
		int lineNum = (thisParticle->ptId)%numLines;
		int pointNum = ((thisParticle->ptId) - lineNum)/numLines;
		thisParticle->unusedTime = SampleFLALine(flArray, initialTime, finalTime, 
			lineNum, pointNum, dir, forwardTrace, stepList, 
			istat, timeLeft);	
		

		// for next timestep's advection
		if(istat == OKAY){
			++pIter;
			numAdvected++;
		}
		else				
			deadList.push_back(pIter++);
	}
	return numAdvected;
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
//////////////////////////////////////////////////////////////////////////
// AN:  initialize seeds for field line advection.  
// The seeds are subsampled from the container, using the
// indexList, which has 
// pStreakLine, not necessarily identified as seeds in the container.
/////////////////////////////////////////////////////////////////////////
int vtCStreakLine::addFLASeeds(int tstep, FlowLineData* container, int maxNumSamples)
{
	//release previous seeds
	releaseSeedMemory();
	//Create arrays of indices pointing to the resampled time steps
	//Note: the number of lines should be the same for all the flowLineData's

	int numLines = container->getNumLines();
	//sampleIndices[lineNum][k] is the kth sample along specified line,
	//that will be used as a seed
	int** sampleIndices = new int*[numLines];
	int* sampleCount = new int[numLines];
	for (int i = 0; i< numLines; i++){
		sampleIndices[i] = new int[maxNumSamples];
		sampleCount[i] = container->resampleFieldLines(sampleIndices[i],maxNumSamples,i);
		assert(sampleCount[i]<=maxNumSamples);
	}
	
	int numSeeds = 0;
	
	for (int line = 0; line < container->getNumLines(); line++){
		for (int pointNum = 0; pointNum < sampleCount[line]; pointNum++){
			// Use the pointId to encode both the pointNum and the line:
			// line = ptId%numLines, pointNum = (ptId - line)/numLines
			// This will be used determine where the advected points get inserted at the
			// next time step
			int pointID = numLines*pointNum + line;
			//Get the seed
			vtParticleInfo* newParticle = new vtParticleInfo;
			//Get the resampled point
			float* point = container->getFlowPoint(line, sampleIndices[line][pointNum]);
			newParticle->m_pointInfo.phyCoord = VECTOR3(point[0], point[1], point[2]);
			newParticle->m_fStartTime = (float)tstep;
			newParticle->ptId = pointID;
			newParticle->itsNumStepsAlive = 0;
			
			//Here we test whether the seed particle is in the region
			//If it's out, we set the flag to invalid.  
			//This should not happen!
			
			if (!m_pField->is_in_grid(newParticle->m_pointInfo.phyCoord)) {
				newParticle->itsValidFlag = 0;
				//Occasionally, more than one point can be invalid at the end of a streamline,
				//because the resampling produces multiple samples out of bounds.  When that
				//occurs we just won't advect the invalid points to the next timestep.
			} else {
				newParticle->itsValidFlag = 1;
				m_lSeeds.push_back(newParticle);
				numSeeds++;
			}
		}
	}
	m_nNumSeeds = numSeeds;
	for (int i = 0; i<numLines; i++) delete sampleIndices[i];
	delete sampleIndices;
	delete sampleCount;
	return numSeeds;
}
