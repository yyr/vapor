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

namespace VAPoR
{
	enum RakeDim
	{
		POINT,
		LINE,
		PLANE,
		SOLID
	};

	class FLOW_API SeedGenerator : public VetsUtil::MyBase
	{
	public:
		SeedGenerator(const float min[3], const float max[3], const size_t numSeeds[3]);
		~SeedGenerator();

		void SetRakeDim(void);
		size_t GetRakeDim(void);
		void GetSeeds(float* pSeeds, const bool bRandom, const unsigned int randomSeed);
		
	private:
		float rakeMin[3], rakeMax[3];		// minimal and maximal positions
		size_t numSeeds[3];					// number of seeds
		int rakeDimension;					// 0, 1, 2, 3
	};

	class FLOW_API Rake : public VetsUtil::MyBase
	{
	public:
		virtual void GenSeedRandom( const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed) = 0;
		virtual void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed) = 0;
	};

	class FLOW_API PointRake : public Rake
	{
	public:
		PointRake();
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed);
		~PointRake();
	};

	class FLOW_API LineRake : public Rake
	{
	public:
		LineRake();
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed);
		~LineRake();
	};

	class FLOW_API PlaneRake : public Rake
	{
	public:
		PlaneRake();
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed);
		~PlaneRake();
	};

	class FLOW_API SolidRake : public Rake
	{
	public:
		SolidRake();
		void GenSeedRandom(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed);
		void GenSeedRegular(const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed);
		~SolidRake();
	};
};

#endif
