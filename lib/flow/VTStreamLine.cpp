
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

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
// definition of class FieldLine
//////////////////////////////////////////////////////////////////////////
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
							float* points)
{
	m_fCurrentTime = *(float *)userData;
	computeStreamLine(userData, points);
}

void vtCStreamLine::computeStreamLine(const void* userData,
									  float* points)
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
				SampleStreamline(points, posInPoints, backTrace, stepList);
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
				SampleStreamline(points, posInPoints, forwardTrace, stepList);
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
	PointInfo thisParticle, prevParticle, second_prevParticle;
	float dt, dt_estimate, cell_volume, mag, curTime;
	VECTOR3 vel;
	float totalStepsize = 0.0;

	// the first particle
	res = m_pField->at_phys(seedInfo.fromCell, seedInfo.phyCoord, seedInfo, m_fCurrentTime, vel);
	if(res == -1)
		return;
	thisParticle = seedInfo;
	seedTrace.push_back(new VECTOR3(seedInfo.phyCoord));
	curTime = m_fCurrentTime;
	
	// get the initial stepsize
	switch(m_pField->GetCellType())
	{
	case CUBE:
		dt = dt_estimate = m_fInitStepSize;
		break;

	case TETRAHEDRON:
		cell_volume = m_pField->volume_of_cell(seedInfo.inCell);
		mag = vel.GetMag();
		if(fabs(mag) < 1.0e-6f)
			dt_estimate = 1.0e-5f;
		else
			dt_estimate = pow(cell_volume, (float)0.3333333f) / mag;
		dt = dt_estimate;
		break;

	default:
		break; 
	}

	// start to advect
	while(totalStepsize < (float)(m_nMaxsize-1))
	{
		second_prevParticle = prevParticle;
		prevParticle = thisParticle;

		if(integ_ord == SECOND)
			istat = runge_kutta2(time_dir, time_dep, thisParticle, &curTime, dt);
		else
			istat = runge_kutta4(time_dir, time_dep, thisParticle, &curTime, dt);

		if(istat != 1)			// out of boundary
			return;
		else
		{
			seedTrace.push_back(new VECTOR3(thisParticle.phyCoord));
			stepList.push_back(dt);
			totalStepsize += dt;			// accumulation of step size
		}

		if((int)seedTrace.size() > 2)
			adapt_step(second_prevParticle.phyCoord, prevParticle.phyCoord, thisParticle.phyCoord, dt_estimate, &dt);
	}
}

//////////////////////////////////////////////////////////////////////////
// sample streamline to get points with correct interval
//////////////////////////////////////////////////////////////////////////
void vtCStreamLine::SampleStreamline(float* positions,
									 const unsigned int posInPoints,
									 vtListSeedTrace* seedTrace,
									 list<float>* stepList)
{
	list<VECTOR3*>::iterator pIter1;
	list<VECTOR3*>::iterator pIter2;
	list<float>::iterator pStepIter;
	float stepsizeLeft;
	int count;
	unsigned int ptr;

	ptr = posInPoints;
	pIter1 = seedTrace->begin();
	pIter2 = pIter1++;
	pStepIter = stepList->begin();
	stepsizeLeft = *pStepIter;
	
	// the first one is seed
	positions[ptr++] = (**pIter1)[0];
	positions[ptr++] = (**pIter1)[1];
	positions[ptr++] = (**pIter1)[2];
	count = 1;

	// other advecting result
	while((count < m_nMaxsize) && (pStepIter != stepList->end()))
	{
		float ratio;
		if(stepsizeLeft < 1.0)
		{
			pIter1++;
			pIter2++;
			pStepIter++;
			stepsizeLeft += *pStepIter;
		}
		else
		{
			stepsizeLeft -= 1;
			ratio = (*pStepIter - stepsizeLeft)/(*pStepIter);
			positions[ptr++] = Lerp((**pIter1)[0], (**pIter2)[0], ratio);
			positions[ptr++] = Lerp((**pIter1)[1], (**pIter2)[1], ratio);
			positions[ptr++] = Lerp((**pIter1)[2], (**pIter2)[2], ratio);
			count++;
		}
	}

	// if # of sampled points < maximal points asked
	if(count < m_nMaxsize)
		positions[ptr] = 1.0e+30f;
}