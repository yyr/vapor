
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
#include <vapor/VaporFlow.h>
#include <vapor/flowlinedata.h>

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// definition of class FieldLine
//////////////////////////////////////////////////////////////////////////
//extern FILE* fDebug;

vtCStreamLine::vtCStreamLine(CVectorField* pField):
vtCFieldLine(pField),
m_itsTraceDir(BACKWARD_AND_FORWARD),
m_fCurrentTime(0.0)
{
}

vtCStreamLine::~vtCStreamLine(void)
{
	//printf("called in vtCStreamLine\n");
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
//New version, uses FlowLineData instead of points array to write results of advection.
void vtCStreamLine::computeStreamLine(float curTime, FlowLineData* container){
	m_fCurrentTime = curTime;

	vtListParticleIter sIter;
	int istat;
	int seedNum = 0;
	for(sIter = m_lSeeds.begin(); sIter != m_lSeeds.end(); ++sIter)
	{
		vtParticleInfo* thisSeed = *sIter;
		if(thisSeed->itsValidFlag == 1)			// valid seed
		{
			if(m_itsTraceDir & BACKWARD_DIR)
			{
				vtListSeedTrace* backTrace;
				list<float>* stepList;
				backTrace = new vtListSeedTrace;
				stepList = new list<float>;
				istat = computeFieldLine(BACKWARD, STEADY, *backTrace, *stepList, thisSeed->m_pointInfo);
				SampleFieldline(container, seedNum, BACKWARD, backTrace, stepList, true, istat );
				delete backTrace;
				delete stepList;
			} else {
				//must be pure forward, set start point
				container->setFlowStart(seedNum, 0);
			}
			if(m_itsTraceDir & FORWARD_DIR)
			{
				vtListSeedTrace* forwardTrace;
				list<float>* stepList;
				forwardTrace = new vtListSeedTrace;
				stepList = new list<float>;
				istat = computeFieldLine(FORWARD,STEADY, *forwardTrace, *stepList, thisSeed->m_pointInfo);
				SampleFieldline(container, seedNum, FORWARD, forwardTrace, stepList, true, istat);
				delete forwardTrace;
				delete stepList;
			} else {
				//Must be pure backward, establish end of flowline: 
				container->setFlowEnd(seedNum, 0);
			}
		}
		else                // out of data region.  Just mark seed as end, don't advect.
        {
			float x = thisSeed->m_pointInfo.phyCoord.x();
			float y = thisSeed->m_pointInfo.phyCoord.y();
			float z = thisSeed->m_pointInfo.phyCoord.z();
			container->setFlowPoint(seedNum, 0, x,y,z);
			if(container->getMaxLength(BACKWARD) > 0)
				container->setFlowPoint(seedNum, -1, END_FLOW_FLAG,0.f,0.f);
			if(container->getMaxLength(FORWARD) > 0) 
				container->setFlowPoint(seedNum, 1, END_FLOW_FLAG,0.f,0.f);
			
			if (container->doSpeeds()){
				PointInfo pointInfo;
				VECTOR3 nodeData;
				float t = m_pField->GetStartTime();

				pointInfo.phyCoord.Set(x,y,z);	
				m_pField->getFieldValue(pointInfo.phyCoord, t, nodeData);
				container->setSpeed(seedNum, 0, nodeData.GetMag()/m_pField->getTimeScaleFactor());
			}
			container->setFlowStart(seedNum, 0);
			container->setFlowEnd(seedNum, 0);
        }

		seedNum++;
	}
}



int vtCStreamLine::computeFieldLine(TIME_DIR time_dir,
									 TIME_DEP time_dep, 
									 vtListSeedTrace& seedTrace,
									 list<float>& stepList,
									 PointInfo& seedInfo)
{
	int istat;
	PointInfo thisParticle;
	double dt, cell_side, mag; 
	double curTime;
	VECTOR3 vel;
	float totalStepsize = 0.0;
	bool onAdaptive = true;
	int nSetAdaptiveCount = 0;

	// the first particle
	thisParticle = seedInfo;
	
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = (double)m_fCurrentTime;
	istat = m_pField->getFieldValue(seedInfo.phyCoord, m_fCurrentTime, vel);
	if(istat == OUT_OF_BOUND)
		return OUT_OF_BOUND;			// the advection is out of boundary
	if((abs(vel[0]) < m_fStationaryCutoff) && (abs(vel[1]) < m_fStationaryCutoff) && (abs(vel[2]) < m_fStationaryCutoff))
		return CRITICAL_POINT;			// this is critical point
		
	// get the initial step size
	
	cell_side = m_pField->GetMaxMinGridSpacing();
	mag = vel.GetDMag();
	
	dt = m_fInitStepSize * cell_side / mag;
	
	//Determine the value of mag*dt to project 10 times init size.  
	double maxMagDt = 10.*dt*mag;

	int rollbackCount = 0;
	// start to advect
	
	while(totalStepsize < (float)((m_nMaxsize-1)*m_fSamplingRate))
	{
		bool doingRetrace = false;
		
		int retrace = true;
		
		
		while(retrace)
		{
			retrace = false;
			
			for(int magTry = 0; magTry < 40; magTry++) {
				
				istat = runge_kutta4(time_dir, time_dep, thisParticle, &curTime, dt, maxMagDt);
			
				if (istat != FIELD_TOO_BIG) break;
				//Shrink dt by factor of 10.
				dt = 0.1*dt;

			}
			assert (istat != FIELD_TOO_BIG);
			
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			stepList.push_back(dt);

			if(istat == OUT_OF_BOUND)			// out of boundary
				return OUT_OF_BOUND;

			m_pField->getFieldValue(thisParticle.phyCoord, m_fCurrentTime, vel);
			
			if((abs(vel[0]) < m_fStationaryCutoff) && (abs(vel[1]) < m_fStationaryCutoff) && (abs(vel[2]) < m_fStationaryCutoff))
				return CRITICAL_POINT;

			totalStepsize += dt;			// accumulation of step size
			nSetAdaptiveCount++;
			
			if((nSetAdaptiveCount == 2) && (onAdaptive == false))
				onAdaptive = true;

			// just generate valid new point
			if(((int)seedTrace.size() > 2)&&(onAdaptive))
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

				
				mag = vel.GetDMag();
				minStepsize = m_fInitStepSize * cell_side / mag;
				maxStepsize = m_fMaxStepSize * cell_side / mag;
				retrace = adapt_step(second_prevPhy, prevPhy, thisPhy, minStepsize, maxStepsize, &dt, onAdaptive);
				if(onAdaptive == false)
					nSetAdaptiveCount = 0;

				assert (rollbackCount < 10000);//If we got here, it's surely an infinite loop!
					
				// roll back and retrace
				//the doingRetrace flag is to stop double retracing (which results in infinite loop!)
				//Note a slightly different approach is in VTTimeVaryingFieldLine.cpp
				if(retrace && !doingRetrace)			
				{
					seedTrace.pop_back();
					seedTrace.pop_back();
					thisParticle.Set(*(seedTrace.back()));
					totalStepsize -= stepList.back();
					stepList.pop_back();
					totalStepsize -= stepList.back();
					stepList.pop_back();
					rollbackCount++;
					doingRetrace = true;
				} else doingRetrace = false;
			}
		}// end of retrace
		//What was the distance advected?
	}// end of advection
	return (OKAY);
}