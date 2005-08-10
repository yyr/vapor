/*************************************************************************
 *						OSU Flow Vector Field							 *
 *																		 *
 *																		 *
 *	Created:	Han-Wei Shen, Liya Li									 *
 *				The Ohio State University								 *
 *	Date:		06/2005													 *
 *																		 *
 *	Vector Field Lines													 *
 *************************************************************************/

#ifndef _VECTOR_FIELD_LINE_H_
#define _VECTOR_FIELD_LINE_H_

#include "Field.h"

namespace VAPoR
{
//////////////////////////////////////////////////////////////////////////
// definition
//////////////////////////////////////////////////////////////////////////
#define MAX_LENGTH 1000
const double RAD_TO_DEG = 57.2957795130823208768;	// 180 / PI

enum INTEG_ORD{ SECOND = 2, FOURTH = 4};		// integration order
enum TIME_DIR{ BACKWARD = -1, FORWARD = 1};		// advection direction
enum TIME_DEP{ STEADY=0,UNSTEADY=1 };	
enum TRACE_DIR{OFF=0, BACKWARD_DIR=1, FORWARD_DIR=2, BACKWARD_AND_FORWARD=3};

//////////////////////////////////////////////////////////////////////////
// information about particles
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtParticleInfo : public VetsUtil::MyBase
{
public:
	PointInfo m_pointInfo;		// basic information about this particle
	float m_fStartTime;			// start time
	int itsValidFlag;			// whether this particle is valid or not
	int itsNumStepsAlive;		// number of steps alive
	int ptId;					// particle ID

public:
	vtParticleInfo(void)
	{
	}

	vtParticleInfo(vtParticleInfo* source)
	{
		m_pointInfo.Set(source->m_pointInfo);
		m_fStartTime = source->m_fStartTime;
		itsValidFlag = source->itsValidFlag;
		itsNumStepsAlive = source->itsNumStepsAlive;
		ptId = source->ptId;
	}

	vtParticleInfo(vtParticleInfo& source)
	{
		m_pointInfo.Set(source.m_pointInfo);
		m_fStartTime = source.m_fStartTime;
		itsValidFlag = source.itsValidFlag;
		ptId = source.ptId;
		itsNumStepsAlive = source.itsNumStepsAlive;
	}

	void Set(PointInfo& pInfo, float startT, int validFlag, int life, int id)
	{
		m_pointInfo.Set(pInfo);
		m_fStartTime = startT;
		itsValidFlag = validFlag;
		itsNumStepsAlive = life;
		ptId = id;
	}
};

typedef list<vtParticleInfo*> vtListParticle;
typedef list<vtParticleInfo*>::iterator vtListParticleIter;
typedef list<VECTOR3*> vtListSeedTrace;

//////////////////////////////////////////////////////////////////////////
// base class for all field lines, and the structure is like:
//						vtCFieldLine
//						/			\
//			vtCStreamLine			vtCTimeVaryingFieldLine
//									/			|			\
//							vtCPathline		vtCTimeLine		vtCStreakLine
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtCFieldLine : public VetsUtil::MyBase
{
protected:
	int m_nNumSeeds;				// number of seeds
	INTEG_ORD m_integrationOrder;	// integration order
	TIME_DIR m_timeDir;				// advection direction
	float m_fInitStepSize;			// initial advection step size of particle
	float m_fLowerAngleAccuracy;	// for adaptive stepsize 
	float m_fUpperAngleAccuracy;
	float m_fMaxStepSize;			// maximal advection stepsize
	int m_nMaxsize;					// maximal number of particles this line advects
	vtListParticle m_lSeeds;		// list of seeds
	CVectorField* m_pField;			// vector field

public:
	vtCFieldLine(CVectorField* pField);
	virtual ~vtCFieldLine(void);

	void setSeedPoints(float*, int, float);
	void setMaxPoints(int val) { m_nMaxsize = val; }
	void setIntegrationOrder(INTEG_ORD ord) { m_integrationOrder = ord; }
	int  getMaxPoints(void){ return m_nMaxsize; }
	INTEG_ORD getIntegrationOrder(void){ return m_integrationOrder; }
	void SetMaxStepSize(float stepsize) {m_fMaxStepSize = stepsize;}
	float GetMaxStepSize(void) {return m_fMaxStepSize;}
	void SetInitStepSize(float initStep) { m_fInitStepSize = initStep; }
	float GetInitStepSize(void) { return m_fInitStepSize; }
	void SetLowerUpperAngle(float lowerAngle, float upperAngle) {m_fLowerAngleAccuracy = lowerAngle; m_fUpperAngleAccuracy = upperAngle;}

protected:
	void releaseSeedMemory(void);
	int euler_cauchy(TIME_DIR, TIME_DEP,float*, float);
	int runge_kutta4(TIME_DIR, TIME_DEP, PointInfo&, float*, float);
	int runge_kutta2(TIME_DIR, TIME_DEP, PointInfo&, float*, float);
	int adapt_step(const VECTOR3& p2, const VECTOR3& p1, const VECTOR3& p0, float dt_estimate,float* dt);
};

//////////////////////////////////////////////////////////////////////////
// class declaratin of timevaryingfieldline
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtCTimeVaryingFieldLine : public vtCFieldLine
{
protected:
	int m_itsTimeAdaptionFlag;
	int m_itsMaxParticleLife;				// how long the particles be alive
	float m_itsTimeInc;					
	list<vtParticleInfo*> m_itsParticles;

public:
	vtCTimeVaryingFieldLine(CVectorField* pField);
	~vtCTimeVaryingFieldLine(void);

	void SetTimeDir(TIME_DIR dir) { m_timeDir = dir; }

	void SetTimeAdaptionMode(int onoff)
	{
		m_itsTimeAdaptionFlag = onoff;
	}

	virtual void setParticleLife(int steps);
	virtual int getParticleLife(void);

protected:
	// code shared by all time varying field lines here
	void releaseParticleMemory(void);
	int advectParticle( INTEG_ORD int_order, 
						vtParticleInfo& initialPoint,
						float initialTime,
						vtParticleInfo& finalPoint,
						float finalTime);
	int advectParticle( INTEG_ORD int_order, 
						vtParticleInfo& initialPoint,
						float initialTime,
						vtListSeedTrace& seedTrace);
};

//////////////////////////////////////////////////////////////////////////
// class declaration of pathline
//////////////////////////////////////////////////////////////////////////
typedef struct vtPathlineParticle
{
	VECTOR3 pos;
	int ptId;
}vtPathlineParticle;

class FLOW_API vtCPathLine : public vtCTimeVaryingFieldLine
{
public:
	vtCPathLine(CVectorField* pField);
	~vtCPathLine(void);

	void execute(const void* userData, list<vtListSeedTrace*>& listSeedTraces);
	void execute(const void* userData, list<vtPathlineParticle*>& listSeedTraces);
		
protected:
	// code specific to pathline
	void computePathLine(const void*, list<vtListSeedTrace*>&);
	void computePathLine(const void*, list<vtPathlineParticle*>&);
};

//////////////////////////////////////////////////////////////////////////
// class declaration of streakline
// inject particles from a point
//////////////////////////////////////////////////////////////////////////
typedef struct vtStreakParticle
{
	PointInfo itsPoint;			// basic information
	float itsTime;				// start time
}vtStreakParticle;

typedef list<vtStreakParticle*> vtListStreakParticle;
typedef list<vtStreakParticle*>::iterator vtListStreakParticleIter;

class FLOW_API vtCStreakLine : public vtCTimeVaryingFieldLine
{
public:
	vtCStreakLine(CVectorField* pField);
	~vtCStreakLine(void);
	void execute(const void* userData, float* points, bool bInjectSeeds, int iInjection);

protected:
	// code specific to streakline
	void computeStreakLine(const void* userData, float* points, bool bInjectSeeds, int iInjection);
	void advectOldParticles(vtListParticleIter start, 
							vtListParticleIter end, 
							float* points,
							float initialTime, 
							float finalTime,
							vector<vtListParticleIter>& deadList);
};

//////////////////////////////////////////////////////////////////////////
// class declaration of timeline
// release a set of particles, injected periodically
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtCTimeLine : public vtCTimeVaryingFieldLine
{
public:
	vtCTimeLine(CVectorField* pField);
	~vtCTimeLine(void);

	void execute(const void* userData, vtListStreakParticle& listSeedTraces);
	void setTimeDelay(int delay);
	int getTimeDelay(void);

protected:
	// code specific to timeline
	void computeTimeLine(const void*, vtListStreakParticle&);
	void advectOldParticles(vtListParticleIter start, 
							vtListParticleIter end, 
							vtListStreakParticle& listSeedTraces,
							float initialTime, float finalTime,
							vector<vtListParticleIter>& deadList);
	
	int m_itsTimeDelay;
	int numTillRelease;
};

//////////////////////////////////////////////////////////////////////////
// class declaration of streamline
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtCStreamLine : public vtCFieldLine
{
public:
	vtCStreamLine(CVectorField* pField);
	~vtCStreamLine(void);

	void execute(const void* userData, float* positions);
	void setForwardTracing(int enabled);
	void setBackwardTracing(int enabled);
	int  getForwardTracing(void);
	int  getBackwardTracing(void);
	void SampleStreamline(float*, const unsigned int, vtListSeedTrace*, list<float>*);
	void SetSamplingRate(float rate) {m_fSamplingRate = rate;}
	
protected:
	void computeStreamLine(const void* userData, float* positions);
	void computeFieldLine(TIME_DIR, INTEG_ORD, TIME_DEP, vtListSeedTrace&, list<float>&, PointInfo&);

	TRACE_DIR m_itsTraceDir;
	float m_fCurrentTime;
	float m_fSamplingRate;
};

};
#endif