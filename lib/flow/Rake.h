//	File:		Rake.h
//
//	Author:		Liya Li
//
//	Date:		July 2005
//
//	Description:	This class is used to generate seed in the flow field. Currently, all rakes
//					are axis-aligned.
//

#ifndef	_RAKE_h_
#define	_RAKE_h_

#include <time.h>
#include "vapor/DataMgr.h"
#include "vapor/MyBase.h"
#include "vaporinternal/common.h"
#include "Interpolator.h"
#include "assert.h"

namespace VAPoR
{
	
	class FieldData;
	class VaporFlow;
	enum RakeDim
	{
		POINT,
		LINE,
		PLANE,
		SOLID
	};
	//ordering C functions used for quicksorting
	int  compLT(const void* val1, const void* val2); 
	int  compGT(const void* val1, const void* val2); 

	class FLOW_API SeedGenerator : public VetsUtil::MyBase
	{
	public:
		SeedGenerator(const float min[3], const float max[3], const size_t numSeeds[3]);
		~SeedGenerator();

		void SetRakeDim(void);
		size_t GetRakeDim(void);
		bool GetSeeds(VaporFlow* vFlow, float* pSeeds, const bool bRandom, const unsigned int randomSeed, int stride = 3);
		void SetSeedDistrib(float distbias, int ts, size_t numrefin,
			const char* xvar, const char* yvar, const char* zvar){
				assert( distbias >= -15.f && distbias <= 15.f);
			distribBias = distbias;
			timeStep = ts;
			numRefinements = numrefin;
			varx = xvar;
			vary = yvar;
			varz = zvar;
		}
		
	private:
		float rakeMin[3], rakeMax[3];		// minimal and maximal positions
		size_t numSeeds[3];					// number of seeds
		int rakeDimension;					// 0, 1, 2, 3
		float distribBias;					//For nonuniform distribution
		const char* varx, *vary, *varz;		//Field used for distrib seeds
		size_t numRefinements;				//refinement used for dist. seeds.
		int timeStep;						//Timestep to be sampled for dist. seeds
	};

	class FLOW_API Rake : public VetsUtil::MyBase
	{
	public:
		virtual ~Rake(){}
		virtual void GenSeedRandom( const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3) = 0;
		virtual void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, int stride = 3) = 0;
		virtual bool GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData*, 
			const size_t numSeeds[3], const float minrake[3], const float maxrake[3], float* pSeed, unsigned int randSeed, int stride = 3) = 0;
	
	
	//Internal classes to qsort list of points
	//PointSorter is basically an array of 4-tuples.  The
	//first of the four is the sort key, the other three are point coordinates.
	class PointSorter {
		public:
			PointSorter(int numPoints);
			~PointSorter();
			void sortPoints(int first, int last, bool increasing);
			void setPoint(int index, float key, float* point){
				pointHolder[4*index] = key;
				pointHolder[4*index+1] = *point;
				pointHolder[4*index+2] = *(point+1);
				pointHolder[4*index+3] = *(point+2);
			}
			float getPoint(int index, int crd){return pointHolder[4*index+1+crd];}
			float getKey(int index) {return pointHolder[4*index];}
			bool isValid(){return (pointHolder != 0);}
		private:
			float* pointHolder;
		};
	};
	class FLOW_API PointRake : public Rake
	{
	public:
		PointRake();
		virtual ~PointRake(){}
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, int stride = 3);
		bool GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData*, const size_t numSeeds[3], 
			const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
	};

	class FLOW_API LineRake : public Rake
	{
	public:
		LineRake();
		virtual ~LineRake(){}
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, int stride = 3);
		bool GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData*, const size_t numSeeds[3],
			const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
	};

	class FLOW_API PlaneRake : public Rake
	{
	public:
		PlaneRake();
		virtual ~PlaneRake(){}
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, int stride = 3);
		bool GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData*, const size_t numSeeds[3],
			const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
	};

	class FLOW_API SolidRake : public Rake
	{
	public:
		SolidRake();
		virtual ~SolidRake(){}
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, int stride = 3);
		bool GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData*, const size_t numSeeds[3],
			const float min[3], const float max[3], float* pSeed, unsigned int randomSeed, int stride = 3);
	};
};

#endif
