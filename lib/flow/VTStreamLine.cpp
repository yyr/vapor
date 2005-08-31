
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
				computeFieldLine(BACKWARD,m_integrationOrder, STEADY, *backTrace, *stepList, thisSeed->m_pointInfo);
				SampleFieldline(points, posInPoints, backTrace, stepList, true);
				backTrace->clear();
				stepList->clear();
			}
			if(m_itsTraceDir & FORWARD_DIR)
			{
				vtListSeedTrace* forwardTrace;
				list<float>* stepList;
				forwardTrace = new vtListSeedTrace;
				stepList = new list<float>;
				computeFieldLine(FORWARD,m_integrationOrder, STEADY, *forwardTrace, *stepList, thisSeed->m_pointInfo);
				SampleFieldline(points, posInPoints, forwardTrace, stepList, true);
				forwardTrace->clear();
				stepList->clear();
			}
		}
	}
}


void vtCStreamLine::computeFieldLine(TIME_DIR time_dir,
									 INTEG_ORD integ_ord,
									 TIME_DEP time_dep, 
									 vtListSeedTrace& seedTrace,
									 list<float>& stepList,
									 PointInfo& seedInfo)
{
	int istat, res;
	PointInfo thisParticle;
	VECTOR3 thisInterpolant, prevInterpolant, second_prevInterpolant;
	float dt, dt_estimate, cell_volume, mag, curTime;
	VECTOR3 vel;
	float totalStepsize = 0.0;

	// the first particle
	thisParticle = seedInfo;
	prevInterpolant = thisInterpolant = thisParticle.interpolant;
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = m_fCurrentTime;

	res = m_pField->at_phys(seedInfo.fromCell, seedInfo.phyCoord, seedInfo, m_fCurrentTime, vel);
	if(res == -1)
		return;
	if((abs(vel[0]) < EPS) && (abs(vel[1]) < EPS) && (abs(vel[2]) < EPS))
		return;
		
	// get the initial step size
	cell_volume = m_pField->volume_of_cell(seedInfo.inCell);
	mag = vel.GetMag();
	if(fabs(mag) < 1.0e-6f)
		dt_estimate = 1.0e-5f;
	else
		dt_estimate = m_fInitStepSize * pow(cell_volume, (float)0.3333333f) / mag;
	dt = dt_estimate;

#ifdef DEBUG
	fprintf(fDebug, "************************************************\n");
#endif

/*	// start to advect
	while(totalStepsize < (float)((m_nMaxsize-1)*m_fSamplingRate))
	{
		float diff;
		int retrace = true;
		PointInfo tempParticle;

		while(retrace)
		{
			tempParticle = thisParticle;
			retrace = false;
			istat = runge_kutta4(time_dir, time_dep, tempParticle, &curTime, dt, &diff);
			if(istat != 1)			// out of boundary
				return;
			retrace = adapt_step(diff, m_fIntegrationAccurace, &dt);
		}
		thisParticle = tempParticle;
		seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
		fprintf(fDebugOut, "temp (%f, %f, %f)\n", thisParticle.phyCoord[0], thisParticle.phyCoord[1], thisParticle.phyCoord[2]);
		stepList.push_back(dt);
		totalStepsize += dt;			// accumulation of step size
	}// end of advection*/

	// start to advect
	while(totalStepsize < (float)((m_nMaxsize-1)*m_fSamplingRate))
	{
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

			if(istat != 1)			// out of boundary
				return;

			m_pField->at_phys(thisParticle.fromCell, thisParticle.phyCoord, thisParticle, m_fCurrentTime, vel);
			if((abs(vel[0]) < EPS) && (abs(vel[1]) < EPS) && (abs(vel[2]) < EPS))
				return;
			int xc, yc, zc;
			xc = (int)(thisParticle.phyCoord[0]*63);
			yc = (int)(thisParticle.phyCoord[1]*63);
			zc = (int)(thisParticle.phyCoord[2]*63);

			thisInterpolant = thisParticle.interpolant;
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			stepList.push_back(dt);
			totalStepsize += dt;			// accumulation of step size

			// just generate valid new point
			if((int)seedTrace.size() > 2)
			{
				VECTOR3 thisPhy, prevPhy, second_prevPhy;
				list<VECTOR3*>::iterator pIter = seedTrace.end();
				pIter--;
				thisPhy = **pIter;
				pIter--;
				prevPhy = **pIter;
				pIter--;
				second_prevPhy = **pIter;
				retrace = adapt_step(second_prevPhy, prevPhy, thisPhy, dt_estimate, &dt);
			}

			// roll back and retrace
			if(retrace == true)			
			{
				thisInterpolant = prevInterpolant = second_prevInterpolant;
				seedTrace.pop_back();
				seedTrace.pop_back();
				thisParticle.Set(*(seedTrace.back()), thisInterpolant, -1, -1);
				totalStepsize -= stepList.back();
				stepList.pop_back();
				totalStepsize -= stepList.back();
				stepList.pop_back();
			}
		}// end of retrace
	}// end of advection
}