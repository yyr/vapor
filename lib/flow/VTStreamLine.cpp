
/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	StreamLines															 *
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
extern FILE* fDebug;

vtCStreamLine::vtCStreamLine(CVectorField* pField):
vtCFieldLine(pField),
m_itsTraceDir(BACKWARD_AND_FORWARD),
m_fCurrentTime(0.0)
{
}

vtCStreamLine::~vtCStreamLine(void)
{
	printf("called in vtCStreamLine\n");
}

//////////////////////////////////////////////////////////////////////////
// Set the advection direction
//////////////////////////////////////////////////////////////////////////
void vtCStreamLine::setForwardTracing(int enabled)
{
	switch( m_itsTraceDir )
	{
	case BACKWARD_DIR:
		if(enabled)
			m_itsTraceDir = BACKWARD_AND_FORWARD;
		break;
	case FORWARD_DIR:
		if(!enabled)
			m_itsTraceDir = OFF;
		break;
	case BACKWARD_AND_FORWARD:
		if(!enabled)
			m_itsTraceDir = BACKWARD_DIR;
		break;
	case OFF:
		if (enabled)
			m_itsTraceDir = FORWARD_DIR;
		break;
	}
}

void vtCStreamLine::setBackwardTracing(int enabled)
{
	switch( m_itsTraceDir )
	{
	  case BACKWARD_DIR:
		  if(!enabled)
			  m_itsTraceDir = OFF;
		  break;
	  case FORWARD_DIR:
		  if(enabled)
			  m_itsTraceDir = BACKWARD_AND_FORWARD;
		  break;
	  case BACKWARD_AND_FORWARD:
		  if(!enabled)
			  m_itsTraceDir = FORWARD_DIR;
		  break;
	  case OFF:
		  if (enabled)
			  m_itsTraceDir = BACKWARD_DIR;
		  break;
	}
}

int vtCStreamLine::getForwardTracing(void)
{
	if( m_itsTraceDir==FORWARD_DIR || m_itsTraceDir==BACKWARD_AND_FORWARD )
		return 1;
	else
		return 0;
}

int vtCStreamLine::getBackwardTracing(void)
{
	if( m_itsTraceDir==BACKWARD_DIR || m_itsTraceDir==BACKWARD_AND_FORWARD )
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////////////
// Compute streamlines.
// output
//	listSeedTraces: For each seed, return a list keeping the trace it
//					advects
//////////////////////////////////////////////////////////////////////////
void vtCStreamLine::execute(const void* userData, 
							float* points,
							float* speeds)
{
	m_fCurrentTime = *(float *)userData;
	computeStreamLine(userData, points, speeds);
}

void vtCStreamLine::computeStreamLine(const void* userData,
									  float* points,
									  float* speeds)
{
	vtListParticleIter sIter;
	unsigned int posInPoints;			// used to record the current points usage
	int count;
	int istat;
	unsigned int* startPositions;
	
	startPositions = new unsigned int[(int)m_lSeeds.size()];
	for(int iFor = 0; iFor < (int)m_lSeeds.size(); iFor++)
		startPositions[iFor] = iFor * m_nMaxsize * 3;
	
	posInPoints = 0;
	count = 0;
	for(sIter = m_lSeeds.begin(); sIter != m_lSeeds.end(); ++sIter)
	{
		vtParticleInfo* thisSeed = *sIter;
		posInPoints = count * m_nMaxsize * 3;
		count++;
	
		if(thisSeed->itsValidFlag == 1)			// valid seed
		{
			if(m_itsTraceDir & BACKWARD_DIR)
			{
				vtListSeedTrace* backTrace;
				list<float>* stepList;
				backTrace = new vtListSeedTrace;
				stepList = new list<float>;
				istat = computeFieldLine(BACKWARD,m_integrationOrder, STEADY, *backTrace, *stepList, thisSeed->m_pointInfo);
				SampleFieldline(points, startPositions, posInPoints, backTrace, stepList, true, istat, speeds);
				backTrace->clear();
				stepList->clear();
			}
			if(m_itsTraceDir & FORWARD_DIR)
			{
				vtListSeedTrace* forwardTrace;
				list<float>* stepList;
				forwardTrace = new vtListSeedTrace;
				stepList = new list<float>;
				istat = computeFieldLine(FORWARD,m_integrationOrder, STEADY, *forwardTrace, *stepList, thisSeed->m_pointInfo);
				SampleFieldline(points, startPositions, posInPoints, forwardTrace, stepList, true, istat, speeds);
				forwardTrace->clear();
				stepList->clear();
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

	delete[] startPositions;
}


int vtCStreamLine::computeFieldLine(TIME_DIR time_dir,
									 INTEG_ORD integ_ord,
									 TIME_DEP time_dep, 
									 vtListSeedTrace& seedTrace,
									 list<float>& stepList,
									 PointInfo& seedInfo)
{
	int istat;
	PointInfo thisParticle;
	VECTOR3 thisInterpolant, prevInterpolant, second_prevInterpolant;
	float dt, cell_volume, mag, curTime;
	VECTOR3 vel;
	float totalStepsize = 0.0;
	bool onAdaptive = true;
	int nSetAdaptiveCount = 0;

	// the first particle
	thisParticle = seedInfo;
	prevInterpolant = thisInterpolant = thisParticle.interpolant;
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = m_fCurrentTime;

	istat = m_pField->at_phys(seedInfo.fromCell, seedInfo.phyCoord, seedInfo, m_fCurrentTime, vel);
	if(istat == OUT_OF_BOUND)
		return OUT_OF_BOUND;			// the advection is out of boundary
	if((abs(vel[0]) < m_fStationaryCutoff) && (abs(vel[1]) < m_fStationaryCutoff) && (abs(vel[2]) < m_fStationaryCutoff))
		return CRITICAL_POINT;			// this is critical point
		
	// get the initial step size
	cell_volume = m_pField->volume_of_cell(seedInfo.inCell);
	mag = vel.GetMag();
	dt = m_fInitStepSize * pow(cell_volume, (float)0.3333333f) / mag;

#ifdef DEBUG
	fprintf(fDebug, "************************************************\n");
#endif
	int rollbackCount = 0;
	// start to advect
	while(totalStepsize < (float)((m_nMaxsize-1)*m_fSamplingRate))
	{
		bool doingRetrace = false;
		second_prevInterpolant = prevInterpolant;
		prevInterpolant = thisInterpolant;
		int retrace = true;

#ifdef DEBUG
		fprintf(fDebug, "Point(%f, %f, %f), stepSize = %f\n", 
				thisParticle.phyCoord[0], thisParticle.phyCoord[1], thisParticle.phyCoord[2],dt);
#endif

		while(retrace)
		{
			retrace = false;
			
			if(integ_ord == SECOND)
				istat = runge_kutta2(time_dir, time_dep, thisParticle, &curTime, dt);
			else
				istat = runge_kutta4(time_dir, time_dep, thisParticle, &curTime, dt);

			thisInterpolant = thisParticle.interpolant;
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			stepList.push_back(dt);

			if(istat == OUT_OF_BOUND)			// out of boundary
				return OUT_OF_BOUND;

			m_pField->at_phys(thisParticle.fromCell, thisParticle.phyCoord, thisParticle, m_fCurrentTime, vel);
			if((abs(vel[0]) < m_fStationaryCutoff) && (abs(vel[1]) < m_fStationaryCutoff) && (abs(vel[2]) < m_fStationaryCutoff))
				return CRITICAL_POINT;

			totalStepsize += dt;			// accumulation of step size
			nSetAdaptiveCount++;
			
			if((nSetAdaptiveCount == 2) && (onAdaptive == false))
				onAdaptive = true;

			// just generate valid new point
			if(((int)seedTrace.size() > 2)&&(onAdaptive))
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
				retrace = adapt_step(second_prevPhy, prevPhy, thisPhy, minStepsize, maxStepsize, &dt, onAdaptive);
				if(onAdaptive == false)
					nSetAdaptiveCount = 0;
#ifdef DEBUG
				fprintf(fDebug, "retrace = %d\n", retrace);
#endif
				assert (rollbackCount < 1000);//If we got here, it's surely an infinite loop!
					
				// roll back and retrace
				//A hack to stop double retracing (which results in infinite loop!)
				if(retrace == true && !doingRetrace)			
				{
					thisInterpolant = prevInterpolant = second_prevInterpolant;
					seedTrace.pop_back();
					seedTrace.pop_back();
					thisParticle.Set(*(seedTrace.back()), thisInterpolant, -1, -1);
					totalStepsize -= stepList.back();
					stepList.pop_back();
					totalStepsize -= stepList.back();
					stepList.pop_back();
					rollbackCount++;
					doingRetrace = true;
				} else doingRetrace = false;
			}
		}// end of retrace
	}// end of advection
	return (OKAY);
}