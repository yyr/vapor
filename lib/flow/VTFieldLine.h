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
#define MAX_LENGTH 20 //this is the default size of maxPoints.
const double RAD_TO_DEG = 57.2957795130823208768;	// 180 / PI
const double DEG_TO_RAD = 0.0174532925199432957692;	// PI / 180
const double EPS = 1.0e-6;

enum INTEG_ORD{ SECOND = 2, FOURTH = 4};		// integration order

enum TIME_DEP{ STEADY=0,UNSTEADY=1 };	
enum TRACE_DIR{OFF=0, BACKWARD_DIR=1, FORWARD_DIR=2, BACKWARD_AND_FORWARD=3};
enum ADVECT_STATUS{OUT_OF_BOUND = -1, CRITICAL_POINT = 0, FIELD_TOO_BIG = -2, MISSING_VALUE = 2, OKAY = 1};

class FlowLineData;
class PathLineData;

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
	int ptId;					// particle ID.  AN: This is lineNum for VTStreakLine 
	float unusedTime;			//AN:  remember time remaining from previous sampling

public:
	vtParticleInfo(void)
	{
		m_pointInfo.phyCoord.Zero();

		m_fStartTime = 0.0;
		itsValidFlag = 0;
		itsNumStepsAlive = 0;
		ptId = 0;
		unusedTime = 0.0;
	}

	vtParticleInfo(vtParticleInfo* source)
	{
		m_pointInfo.Set(source->m_pointInfo);
		m_fStartTime = source->m_fStartTime;
		itsValidFlag = source->itsValidFlag;
		itsNumStepsAlive = source->itsNumStepsAlive;
		ptId = source->ptId;
		unusedTime = source->unusedTime;
		
	}

	vtParticleInfo(vtParticleInfo& source)
	{
		m_pointInfo.Set(source.m_pointInfo);
		m_fStartTime = source.m_fStartTime;
		itsValidFlag = source.itsValidFlag;
		ptId = source.ptId;
		itsNumStepsAlive = source.itsNumStepsAlive;
		unusedTime = source.unusedTime;
		
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
//						|			|
//			vtCStreamLine			vtCTimeVaryingFieldLine
//									 		|
//							 			vtCStreakLine
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtCFieldLine : public VetsUtil::MyBase
{
protected:
	int m_nNumSeeds;				// number of seeds
	
	TIME_DIR m_timeDir;				// advection direction
	double m_fInitStepSize;			// initial advection step size of particle
	float m_fLowerAngleAccuracy;	// for adaptive stepsize 
	float m_fUpperAngleAccuracy;
	double m_fMaxStepSize;			// maximal advection stepsize
	int m_nMaxsize;					// maximal number of particles this line advects
	vtListParticle m_lSeeds;		// list of seeds
	CVectorField* m_pField;			// vector field
	float m_fSamplingRate;
	double m_fStationaryCutoff;		//defines when flowline is stationary
	

public:
	vtCFieldLine(CVectorField* pField);
	virtual ~vtCFieldLine(void);

	void setSeedPoints(float*, int, float);
	void setMaxPoints(int val) { m_nMaxsize = val; }
	
	int  getMaxPoints(void){ return m_nMaxsize; }
	
	void SetMaxStepSize(double stepsize) {m_fMaxStepSize = stepsize;}
	double GetMaxStepSize(void) {return m_fMaxStepSize;}
	void SetInitStepSize(double initStep) { m_fInitStepSize = initStep; }
	double GetInitStepSize(void) { return m_fInitStepSize; }
	void SetSamplingRate(float rate) {m_fSamplingRate = rate;}
	void SetStationaryCutoff(float cutoff) {m_fStationaryCutoff = cutoff;}

	//For Debugging:
	int fullArraySize;
	
protected:
	void releaseSeedMemory(void);
	
	int runge_kutta4(TIME_DIR, TIME_DEP, PointInfo&, double*, double, double);
	
	int adapt_step( const VECTOR3& p2, 
					const VECTOR3& p1, 
					const VECTOR3& p0, 
					const double& minStepsize, 
					const double& maxStepsize, 
					double* dt, 
					bool& bAdaptive);
	
	float SampleFieldline(FlowLineData* container,
								   int lineNum, // = seedNum
								   int direction, // -1 or +1
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   bool bRecordSeed,
								   int traceState,
								   float remainingTime = 0.f);
	//version for unsteady flow, will eventually replace above:
	float SampleFieldline(PathLineData* container,
									float firstT, float lastT, //timestep interval
								   int lineNum, // != seedNum
								   int direction, // -1 or +1 .. can be determined from firstT, lastT
								   vtListSeedTrace* seedTrace,
								   list<float>* stepList,
								   bool bRecordSeed,
								   int traceState,
								   float remainingTime = 0.f,
								   bool doingFLA = false);
	//Version of above for multiFLA, puts sampled points into containers from firstT to lastT, at
	//position specified by pointNum, lineNum
	float SampleFLALine(FlowLineData** containers,
			float firstT, float lastT, //timestep interval
			int lineNum, int pointNum,
			int direction, // -1 or +1 .. could be determined from firstT, lastT
			vtListSeedTrace* seedTrace,
			list<float>* stepList,
			int traceState,
			float remainingTime = 0.f);
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
	
	int advectParticle( vtParticleInfo& initialPoint,
						vtParticleInfo& finalPoint,
						float initialTime,
						float finalTime,
						vtListSeedTrace& seedTrace,
						list<float>& stepList,
                        bool bAdaptive);
};

//////////////////////////////////////////////////////////////////////////
// class declaration of pathline
//////////////////////////////////////////////////////////////////////////
typedef struct vtPathlineParticle
{
	VECTOR3 pos;
	int ptId;
}vtPathlineParticle;



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
	//void execute(const float t, float* points, const unsigned int* startPositions, unsigned int* pointers, bool bInjectSeeds, int iInjection, float* speeds=0);
	void execute(const float t, PathLineData* container, bool bInjectSeeds, bool doingFLA);
	int addSeeds(int tstep, PathLineData* container);
	int addFLASeeds(int tstep, FlowLineData* container, int maxNumSamples);
	//advect points of field lines, put them in an array of fieldLineData.
	void advectFLAPoints(int tstep, int flowDir, FlowLineData** flDataArray, bool bInjectSeeds);
protected:
	

	// code specific to streakline
	void computeStreakLine(const float t, PathLineData* container, bool bInjectSeeds,
		bool doingFLA);
	
	int advectOldParticles(vtListParticleIter start, 
							vtListParticleIter end, 
							PathLineData* container,
							float initialTime, 
							float finalTime,
							vector<vtListParticleIter>& deadList,
							bool doingFLA);
	//Alternate version for advecting multiple points into field line array
	int advectOldParticles( vtListParticleIter start, 
										vtListParticleIter end, 
										FlowLineData** flArray,
										float initialTime,
										float finalTime,
										vector<vtListParticleIter>& deadList);
							
};


//////////////////////////////////////////////////////////////////////////
// class declaration of streamline
//////////////////////////////////////////////////////////////////////////
class FLOW_API vtCStreamLine : public vtCFieldLine
{
public:
	vtCStreamLine(CVectorField* pField);
	~vtCStreamLine(void);

	void setForwardTracing(int enabled);
	void setBackwardTracing(int enabled);
	int  getForwardTracing(void);
	int  getBackwardTracing(void);
	//New version, uses flowlinedata
	void computeStreamLine(float curTime, FlowLineData* container);
	
protected:
	int computeFieldLine(TIME_DIR, TIME_DEP, vtListSeedTrace&, list<float>&, PointInfo&);

	

	TRACE_DIR m_itsTraceDir;
	float m_fCurrentTime;
};

};
#endif
