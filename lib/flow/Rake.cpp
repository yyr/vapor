#include "Rake.h"
#include "assert.h"
#include <vapor/VaporFlow.h>
#include <vapor/errorcodes.h>
#include "Field.h"
#include <search.h>
#include <stdlib.h>
//Limit to the number of megabytes used for sorting biased seeds
#define MAX_SORT_MBYTES 1.f
using namespace VetsUtil;
using namespace VAPoR;

/************************************************************************/
/*			 			SeedGenerator Class                              */
/************************************************************************/

SeedGenerator::SeedGenerator(const double localmin[3], 
							 const double localmax[3], 
							 const size_t numSeeds[3])
{
	for(int iFor = 0; iFor < 3; iFor++)
	{
		rakeLocalMin[iFor] = localmin[iFor];
		rakeLocalMax[iFor] = localmax[iFor];
		this->numSeeds[iFor] = numSeeds[iFor];
	}
	
	SetRakeDim();
	distribBias = 0.f;
	timeStep = 0;
	numRefinements = 0;
	varx = 0;
	vary = 0;
	varz = 0;
}

SeedGenerator::~SeedGenerator()
{
}

//////////////////////////////////////////////////////////////////////////
// figure out the dimension or type of rake
//////////////////////////////////////////////////////////////////////////
void SeedGenerator::SetRakeDim(void)
{
	int diff;

	diff = 0;
	for(int iFor = 0; iFor < 3; iFor++)
	{
		if(rakeLocalMax[iFor] == rakeLocalMin[iFor])
			diff++;
	}

	diff = 3 - diff;
	if(diff == 0)				// point
		rakeDimension = POINT;
	else if(diff == 1)
		rakeDimension = LINE;	// line
	else if(diff == 2)
		rakeDimension = PLANE;	// plane
	else if(diff == 3)
		rakeDimension = SOLID;	// solid
}

size_t SeedGenerator::GetRakeDim(void)
{
	return (size_t)rakeDimension;
}

//////////////////////////////////////////////////////////////////////////
// generate seeds
//////////////////////////////////////////////////////////////////////////
bool SeedGenerator::GetSeeds(int timestep, VaporFlow* vFlow,
							 float *pSeeds, 
							 const bool bRandom, 
							 const unsigned int randomSeed,
							 int stride)
{
	Rake* pRake = 0;

	if (bRandom) pRake = new SolidRake();
	else {
		if(rakeDimension == POINT)
			pRake = new PointRake();
		else if(rakeDimension == LINE)
			pRake = new LineRake();
		else if(rakeDimension == PLANE)
			pRake = new PlaneRake();
		else if(rakeDimension == SOLID)
			pRake = new SolidRake();
	}
	const vector<double>&usrExts = vFlow->getDataMgr()->GetExtents((size_t)timestep);
	if(bRandom){
		if (distribBias == 0.f){ 
			pRake->GenSeedRandom(usrExts,numSeeds, rakeLocalMin, rakeLocalMax, pSeeds, randomSeed, stride);
		} else {
			//Setup for biased distribution:
			//First calc min/max of field in rake.
			float fieldMin, fieldMax;
			vector<string>varnames;
			varnames.push_back(varx);
			varnames.push_back(vary);
			varnames.push_back(varz);
			bool rc = vFlow->getFieldMagBounds(&fieldMin, &fieldMax, varnames, 
				true, numRefinements, timeStep);
			if (!rc) {
				return false;
			}
			//Then set up the FieldData
			FieldData* fData = vFlow->setupFieldData(varnames, true, numRefinements, timeStep, false);
			if (!fData) {delete pRake; return false;}
			
			rc = pRake->GenSeedBiased(usrExts,distribBias,fieldMin,fieldMax, fData,numSeeds,  rakeLocalMin,rakeLocalMax, pSeeds, randomSeed, stride);
			if (!rc) {
				MyBase::SetErrMsg(VAPOR_ERROR_SEEDS,
					"Unable to generate requested number of\ndistributed seed points.\nTry a bias closer to 0.");
				delete fData; delete pRake; return false;
			}
			fData->releaseData(vFlow->getDataMgr());
			delete fData;
		}
	}
	else
		pRake->GenSeedRegular(usrExts,numSeeds, rakeLocalMin, rakeLocalMax, pSeeds, stride);
	delete pRake;
	return true;
}

/************************************************************************/
/*					 		PointRake Class                             */
/************************************************************************/
PointRake::PointRake()
{
}



//////////////////////////////////////////////////////////////////////////
// numSeeds should be 1
//////////////////////////////////////////////////////////////////////////
void PointRake::GenSeedRandom(const vector<double>&usrExts, 
							  const size_t numSeeds[3], 
							  const double localmin[3], 
							  const double localmax[3], 
							  float* pSeed,
							  unsigned int randomSeed,
							  int stride)
{
	GenSeedRegular(usrExts,numSeeds, localmin, localmax, pSeed, stride);
}
bool PointRake::GenSeedBiased(const vector<double>&usrExts, float , float , float , FieldData* , 
		const size_t numSeeds[3], const double localmin[3], const double localmax[3], float* pSeed , unsigned int , int stride)
{
	GenSeedRegular(usrExts, numSeeds, localmin, localmax, pSeed, stride);
	return true;
}
void PointRake::GenSeedRegular(const vector<double>&usrExts, 
							   const size_t numSeeds[3], 
							  const double localmin[3], 
							  const double localmax[3], 
							  float* pSeed,
							  int stride)
{
	int totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	for (int count = 0; count < totalNum; count++) {
		for(int iFor = 0; iFor < 3; iFor++)
			pSeed[count*stride + iFor] = usrExts[iFor]+(localmin[iFor]+localmax[iFor])*0.5;
	}
	return;
}
/************************************************************************/
/*					 		LineRake Class                              */
/************************************************************************/
LineRake::LineRake()
{
}



void LineRake::GenSeedRandom(const vector<double>&usrExts, 
							 const size_t numSeeds[3], 
							 const double localmin[3], 
							 const double localmax[3], 
							 float* pSeed,
							 unsigned int randomSeed, 
							 int stride)
{
	int totalNum;

	// initialize random number generator
	srand(randomSeed);

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	for(int iFor = 0; iFor < totalNum; iFor++)
	{
		// randomly generate a number between [0, 1]
		float alpha = (float)rand()/(float)RAND_MAX;

		// the random seed is generated by localmin*alpha + localmax*(1-alpha)
		pSeed[stride*iFor + 0] = usrExts[0]+Lerp(localmin[0], localmax[0], alpha);
		pSeed[stride*iFor + 1] = usrExts[1]+Lerp(localmin[1], localmax[1], alpha);
		pSeed[stride*iFor + 2] = usrExts[2]+Lerp(localmin[2], localmax[2], alpha);
	}
}

bool LineRake::GenSeedBiased(const vector<double>&usrExts, float bias, float fieldMin, float fieldMax, FieldData* fData, 
		const size_t numSeeds[3], const double localmin[3], const double localmax[3], float* pSeed, unsigned int randomSeed, int stride)
{
	int totalNum;
	//Note:  The following code is more or less replicated in solid rake and plane rake.
	// initialize random number generator with a negative value
	long rseed = randomSeed;
	if(rseed > 0) rseed = -rseed;

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	
	// Generate biased seeds by selecting largest or smallest seeds from
	// a number of trials determined by the seed bias
	// Number of trials is 2**abs(bias) per seed point output
	int numTrials = (int)(totalNum*pow(2.f,abs(bias)));
	//We shall allocate a PointSorter that holds at most totalNum+MAX_SORT_MBYTES/16 seeds
	//since each seed occupies 16 bytes
	int seedSorterSize = totalNum + numTrials;
	if (seedSorterSize > (totalNum + (int)(MAX_SORT_MBYTES*1.e06f/16.f)))
		seedSorterSize = totalNum + (int)(MAX_SORT_MBYTES*1.e06f/16.f);
	//We need to have at least twice as much space as seeds:
	if (seedSorterSize < totalNum*2) return false;
	PointSorter* seedSorter = new PointSorter(seedSorterSize);
	if (!seedSorter->isValid()) return false;

	int insertionPosn = 0;
	int startInsertPosn = 0;
	int numPointsToInsert = totalNum + numTrials;
	int totalPointsInserted = 0;
	while(1){ //outer loop fills seedSorter and sorts seeds
		insertionPosn = startInsertPosn;
		while(1) { //Inner loop fills seedSorter
			//Fill up the array from insertion point to seedSorterSize
			float alpha, point[3];
			alpha = (float)ran1(&rseed);
		
			point[0] = usrExts[0]+Lerp(localmin[0], localmax[0], alpha);
			point[1] = usrExts[1]+Lerp(localmin[1], localmax[1], alpha);
			point[2] = usrExts[2]+Lerp(localmin[2], localmax[2], alpha);
		
		
		
			float mag = fData->getFieldMag(point);
			if (mag < 0.f) continue;  //Point is out of range
			//Insert the point:
			seedSorter->setPoint(insertionPosn++,mag,point);
			totalPointsInserted++;
			if(insertionPosn >= seedSorterSize || totalPointsInserted >= numPointsToInsert) break;
		}
		//OK, now sort the seeds in the sorter:
		int endPosn = Min(seedSorterSize-1, insertionPosn-1);
		
		seedSorter->sortPoints(0, endPosn, (bias < 0.f));
		//After the first sort, we insert following the totalNum points
		startInsertPosn = totalNum;
		//test for completion:
		if (totalPointsInserted >= numPointsToInsert) break;
	}
	//Now the desired seeds are in the first totalNum positions:
	
	for (int i = 0; i< totalNum; i++){
		pSeed[stride*i] = seedSorter->getPoint(i,0);
		pSeed[stride*i+1] = seedSorter->getPoint(i,1);
		pSeed[stride*i+2] = seedSorter->getPoint(i,2);
	}
	delete seedSorter;
	return true;
	
}


void LineRake::GenSeedRegular(const vector<double>&usrExts, 
							  const size_t numSeeds[3], 
							 const double localmin[3], 
							 const double localmax[3], 
							 float* pSeed,
							 int stride)
{
	int totalNum;
	float unitRatio;
	float alpha;

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	unitRatio = (float)1.0/(float)(totalNum+1);
	for(int iFor = 0; iFor < totalNum; iFor++)
	{
		alpha = (float)(iFor+1) * unitRatio;
		pSeed[stride*iFor + 0] = usrExts[0]+Lerp(localmin[0], localmax[0], alpha);
		pSeed[stride*iFor + 1] = usrExts[1]+Lerp(localmin[1], localmax[1], alpha);
		pSeed[stride*iFor + 2] = usrExts[2]+Lerp(localmin[2], localmax[2], alpha);
	}
}

/************************************************************************/
/*					 		PlaneRake Class                             */
/************************************************************************/
PlaneRake::PlaneRake()
{
}


void PlaneRake::GenSeedRandom(const vector<double>&usrExts, 
							  const size_t numSeeds[3], 
							  const double localmin[3], 
							  const double localmax[3], 
							  float* pSeed,
							  unsigned int randomSeed,
							  int stride)
{
	int totalNum;
	// eight corners
	float ll[3], hl[3], hh[3], lh[3];
	int iFor;

	for(iFor = 0; iFor < 3; iFor++)
	{
		ll[iFor] = localmin[iFor];
		hh[iFor] = localmax[iFor];
	}

	hl[0] = hh[0];	hl[1] = ll[1];	hl[2] = ll[2];
	lh[0] = ll[0];	lh[1] = hh[1];	lh[2] = hh[2];

	// initialize random number generator
	srand(randomSeed);

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	for(iFor = 0; iFor < totalNum; iFor++)
	{
		float coeff[2];
		coeff[0] = (float)rand()/(float)RAND_MAX;
		coeff[1] = (float)rand()/(float)RAND_MAX;
		
		pSeed[stride*iFor + 0] = usrExts[0]+BiLerp(ll[0], hl[0], lh[0], hh[0], coeff);
		pSeed[stride*iFor + 1] = usrExts[1]+BiLerp(ll[1], hl[1], lh[1], hh[1], coeff);
		pSeed[stride*iFor + 2] = usrExts[2]+BiLerp(ll[2], hl[2], lh[2], hh[2], coeff);
	}
}
bool PlaneRake::GenSeedBiased(const vector<double>&usrExts, float bias, float fieldMin, float fieldMax, FieldData* fData, 
		const size_t numSeeds[3], const double localmin[3], const double localmax[3], float* pSeed, 
		unsigned int randomSeed, int stride)
{
	int totalNum;
	// eight corners
	float ll[3], hl[3], hh[3], lh[3];
	int iFor;

	int sameCoord[3];
	for(iFor = 0; iFor < 3; iFor++)
	{
		ll[iFor] = localmin[iFor];
		hh[iFor] = localmax[iFor];
		if (localmin[iFor] == localmax[iFor]) sameCoord[iFor] = 1;
		else sameCoord[iFor] = 0;
	}
	assert(sameCoord[0] + sameCoord[1] + sameCoord[2] == 1);
	//There should be two coords that are not the same.  hl and lh take opposite values for those
	bool firstDiff = true;
	for (int i = 0; i< 3; i++){
		if (sameCoord[i]) {
			hl[i] = lh[i] = ll[i];
		} else if (firstDiff){
			lh[i] = ll[i];
			hl[i] = hh[i];
			firstDiff = false;
		} else {
			lh[i] = hh[i];
			hl[i] = ll[i];
		}
	}

	//Note:  The following code is more or less replicated in solid rake and line rake.
	// initialize random number generator with a negative value
	long rseed = randomSeed;
	if(rseed > 0) rseed = -rseed;

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	
	// Generate biased seeds by selecting largest or smallest seeds from
	// a number of trials determined by the seed bias
	// Number of trials is 2**abs(bias) per seed point output
	int numTrials = (int)(totalNum*pow(2.f,abs(bias)));
	//We shall allocate a PointSorter that holds at most totalNum+MAX_SORT_MBYTES/16 seeds
	//since each seed occupies 16 bytes
	int seedSorterSize = totalNum + numTrials;
	if (seedSorterSize > (totalNum + (int)(MAX_SORT_MBYTES*1.e06f/16.f)))
		seedSorterSize = totalNum + (int)(MAX_SORT_MBYTES*1.e06f/16.f);
	//We need to have at least twice as much space as seeds:
	if (seedSorterSize < totalNum*2) return false;
	PointSorter* seedSorter = new PointSorter(seedSorterSize);
	if (!seedSorter->isValid()) return false;

	int insertionPosn = 0;
	int startInsertPosn = 0;
	int numPointsToInsert = totalNum + numTrials;
	int totalPointsInserted = 0;
	while(1){ //outer loop fills seedSorter and sorts seeds
		insertionPosn = startInsertPosn;
		while(1) { //Inner loop fills seedSorter
			//Fill up the array from insertion point to seedSorterSize
			float coeff[2], point[3];
			coeff[0] = (float)ran1(&rseed);
			coeff[1] = (float)ran1(&rseed);
		
			point[0] = usrExts[0]+BiLerp(ll[0], hl[0], lh[0], hh[0], coeff);
			point[1] = usrExts[1]+BiLerp(ll[1], hl[1], lh[1], hh[1], coeff);
			point[2] = usrExts[2]+BiLerp(ll[2], hl[2], lh[2], hh[2], coeff);
		
			float mag = fData->getFieldMag(point);
			if (mag < 0.f) continue;  //Point is out of range
			//Insert the point:
			seedSorter->setPoint(insertionPosn++,mag,point);
			totalPointsInserted++;
			if(insertionPosn >= seedSorterSize || totalPointsInserted >= numPointsToInsert) break;
		}
		//OK, now sort the seeds in the sorter:
		int endPosn = Min(seedSorterSize-1, insertionPosn-1);
		
		seedSorter->sortPoints(0, endPosn, (bias < 0.f));
		//After the first sort, we insert following the totalNum points
		startInsertPosn = totalNum;
		//test for completion:
		if (totalPointsInserted >= numPointsToInsert) break;
	}
	//Now the desired seeds are in the first totalNum positions:
	
	for (int i = 0; i< totalNum; i++){
		pSeed[stride*i] = seedSorter->getPoint(i,0);
		pSeed[stride*i+1] = seedSorter->getPoint(i,1);
		pSeed[stride*i+2] = seedSorter->getPoint(i,2);
	}
	delete seedSorter;
	return true;
}
void PlaneRake::GenSeedRegular(const vector<double>&usrExts, 
							   const size_t numSeeds[3], 
							  const double localmin[3], 
							  const double localmax[3], 
							  float* pSeed,
							  int stride)
{
	// eight corners
	float ll[3], hl[3], hh[3], lh[3];
	int iFor, jFor;
	for(iFor = 0; iFor < 3; iFor++)
	{
		lh[iFor] = hl[iFor] = 0.0;
		ll[iFor] = localmin[iFor];
		hh[iFor] = localmax[iFor];
	}
	//hl and lh depend on which coordinate is not changing

	if (numSeeds[2] == 1){
		hl[2] = lh[2] = ll[2];
		hl[0] = hh[0];	hl[1] = ll[1];	
		lh[0] = ll[0];	lh[1] = hh[1];	
	} else if (numSeeds[1] == 1){
		hl[1]=lh[1]=hh[1];
		hl[0] = hh[0];	hl[2] = ll[2];
		lh[0] = ll[0];	lh[2] = hh[2];
	} else if(numSeeds[0] == 1){ 
		hl[0]=lh[0]=hh[0];
		hl[1] = hh[1];	hl[2] = ll[2];
		lh[1] = ll[1];	lh[2] = hh[2];
	} else { assert(0);}


	// generate seeds
	float widthUnit, heightUnit;
	int numPerRow, numPerCol;
	if(numSeeds[0] != 1)
		numPerRow = numSeeds[0];
	else 
		numPerRow = numSeeds[1];

	if (numSeeds[2] != 1) 
		numPerCol = numSeeds[2];
	else 
		numPerCol = numSeeds[1]; 
	

	widthUnit = (float)1.0/float(numPerRow+1);
	heightUnit = (float)1.0/float(numPerCol+1);
	int index = 0;
	for(jFor = 0; jFor < numPerCol; jFor++)
		for(iFor = 0; iFor < numPerRow; iFor++)
		{
			float coeff[2];
			coeff[0] = (float)(iFor+1) * widthUnit;
			coeff[1] = (float)(jFor+1) * heightUnit;

			pSeed[index] = usrExts[0]+BiLerp(ll[0], hl[0], lh[0], hh[0], coeff);
			pSeed[index+1] = usrExts[1]+BiLerp(ll[1], hl[1], lh[1], hh[1], coeff);
			pSeed[index+2] = usrExts[2]+BiLerp(ll[2], hl[2], lh[2], hh[2], coeff);
			index += stride;
		}
}

/************************************************************************/
/*					 		SolidRake Class                             */
/************************************************************************/
SolidRake::SolidRake()
{
}



void SolidRake::GenSeedRandom(const vector<double>&usrExts, 
							  const size_t numSeeds[3], 
							  const double localmin[3], 
							  const double localmax[3], 
							  float* pSeed,
							  unsigned int randomSeed,
							  int stride)
{
	int totalNum;
	// eight corners
	float lll[3], hll[3], lhl[3], hhl[3], llh[3], hlh[3], lhh[3], hhh[3];
	int iFor;

	for(iFor = 0; iFor < 3; iFor++)
	{
		//shrink slightly, to ensure seeds lie entirely inside edge cells
		float edgeWidth = (localmax[iFor]-localmin[iFor])*0.03;
		lll[iFor] = localmin[iFor]+edgeWidth;
		hhh[iFor] = localmax[iFor]-edgeWidth;
	}

	hll[0] = hhh[0];	hll[1] = lll[1];	hll[2] = lll[2];
	lhl[0] = lll[0];	lhl[1] = hhh[1];	lhl[2] = lll[2];
	hhl[0] = hhh[0];	hhl[1] = hhh[1];	hhl[2] = lll[2];
	llh[0] = lll[0];	llh[1] = lll[1];	llh[2] = hhh[2];
	hlh[0] = hhh[0];	hlh[1] = lll[1];	hlh[2] = hhh[2];
	lhh[0] = lll[0];	lhh[1] = hhh[1];	lhh[2] = hhh[2];

	// initialize random number generator
	srand(RAND_MAX-randomSeed);

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	
	for(iFor = 0; iFor < totalNum; iFor++)
	{
		float coeff[3];
		coeff[0] = (float)rand()/(float)RAND_MAX;
		coeff[1] = (float)rand()/(float)RAND_MAX;
		coeff[2] = (float)rand()/(float)RAND_MAX;

		pSeed[stride*iFor + 0] = usrExts[0]+TriLerp(lll[0], hll[0], lhl[0], hhl[0], llh[0], hlh[0], lhh[0], hhh[0], coeff);
		pSeed[stride*iFor + 1] = usrExts[1]+TriLerp(lll[1], hll[1], lhl[1], hhl[1], llh[1], hlh[1], lhh[1], hhh[1], coeff);
		pSeed[stride*iFor + 2] = usrExts[2]+TriLerp(lll[2], hll[2], lhl[2], hhl[2], llh[2], hlh[2], lhh[2], hhh[2], coeff);
	}
	
}
bool SolidRake::GenSeedBiased(const vector<double>&usrExts, float bias, float fieldMin, float fieldMax, FieldData* fData, 
		const size_t numSeeds[3], const double localmin[3], const double localmax[3], float* pSeed, 
		unsigned int randomSeed, int stride){
	assert( bias >= -15.f && bias <= 15.f);
	int totalNum;
	// eight corners
	float lll[3], hll[3], lhl[3], hhl[3], llh[3], hlh[3], lhh[3], hhh[3];
	
	
	for(int iFor = 0; iFor < 3; iFor++)
	{
		//shrink slightly, to ensure seeds lie entirely inside edge cells
		float edgeWidth = (localmax[iFor]-localmin[iFor])*0.03;
		lll[iFor] = localmin[iFor]+edgeWidth;
		hhh[iFor] = localmax[iFor]-edgeWidth;
	}

	hll[0] = hhh[0];	hll[1] = lll[1];	hll[2] = lll[2];
	lhl[0] = lll[0];	lhl[1] = hhh[1];	lhl[2] = lll[2];
	hhl[0] = hhh[0];	hhl[1] = hhh[1];	hhl[2] = lll[2];
	llh[0] = lll[0];	llh[1] = lll[1];	llh[2] = hhh[2];
	hlh[0] = hhh[0];	hlh[1] = lll[1];	hlh[2] = hhh[2];
	lhh[0] = lll[0];	lhh[1] = hhh[1];	lhh[2] = hhh[2];

	//Note:  The following code is more or less replicated in plane rake and line rake.
	// initialize random number generator with a negative value
	long rseed = randomSeed;
	if(rseed > 0) rseed = -rseed;

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	
	// Generate biased seeds by selecting largest or smallest seeds from
	// a number of trials determined by the seed bias
	// Number of trials is 2**abs(bias) per seed point output
	int numTrials = (int)(totalNum*pow(2.f,abs(bias)));
	//We shall allocate a PointSorter that holds at most totalNum+MAX_SORT_MBYTES/16 seeds
	//since each seed occupies 16 bytes
	int seedSorterSize = totalNum + numTrials;
	if (seedSorterSize > (totalNum + (int)(MAX_SORT_MBYTES*1.e06f/16.f)))
		seedSorterSize = totalNum + (int)(MAX_SORT_MBYTES*1.e06f/16.f);
	//We need to have at least twice as much space as seeds:
	if (seedSorterSize < totalNum*2) return false;
	PointSorter* seedSorter = new PointSorter(seedSorterSize);
	if (!seedSorter->isValid()) return false;

	//This works by putting seeds into the seedSorter, then sorting them
	//so that the largest (or smallest) keys (magnitude) go to the start.
	//The seedSorter is repeatedly filled and sorted, keeping the largest mag point
	//until the desired number of random seeds has been generated.
	//Repeatedly:  
	//   1.  Insert seeds 
	//		(first time insert from beginning, after that insert from position
	//      totalNum+1 to end
	//   2.  Sort all the points in the array, decreasing for pos bias, increasing for neg bias
	//   3.  Test if we are done, if so exit loop 
	int insertionPosn = 0;
	int startInsertPosn = 0;
	int numPointsToInsert = totalNum + numTrials;
	int totalPointsInserted = 0;
	while(1){ //outer loop fills seedSorter and sorts seeds
		insertionPosn = startInsertPosn;
		while(1) { //Inner loop fills seedSorter
			//Fill up the array from insertion point to seedSorterSize
			float coeff[3], point[3];
		
			coeff[0] = (float)ran1(&rseed);
			coeff[1] = (float)ran1(&rseed);
			coeff[2] = (float)ran1(&rseed);
			assert(coeff[0] > 0.f && coeff[0] < 1.f);

			point[0] = usrExts[0]+TriLerp(lll[0], hll[0], lhl[0], hhl[0], llh[0], hlh[0], lhh[0], hhh[0], coeff);
			point[1] = usrExts[1]+TriLerp(lll[1], hll[1], lhl[1], hhl[1], llh[1], hlh[1], lhh[1], hhh[1], coeff);
			point[2] = usrExts[2]+TriLerp(lll[2], hll[2], lhl[2], hhl[2], llh[2], hlh[2], lhh[2], hhh[2], coeff);

			float mag = fData->getFieldMag(point);
			if (mag < 0.f) continue;  //Point is out of range
			//Insert the point:
			seedSorter->setPoint(insertionPosn++,mag,point);
			totalPointsInserted++;
			if(insertionPosn >= seedSorterSize || totalPointsInserted >= numPointsToInsert) break;
		}
		//OK, now sort the seeds in the sorter:
		int endPosn = Min(seedSorterSize-1, insertionPosn-1);
		
		seedSorter->sortPoints(0, endPosn, (bias < 0.f));
		//After the first sort, we insert following the totalNum points
		startInsertPosn = totalNum;
		//test for completion:
		if (totalPointsInserted >= numPointsToInsert) break;
	}
	//Now the desired seeds are in the first totalNum positions:
	
	for (int i = 0; i< totalNum; i++){
		pSeed[stride*i] = seedSorter->getPoint(i,0);
		pSeed[stride*i+1] = seedSorter->getPoint(i,1);
		pSeed[stride*i+2] = seedSorter->getPoint(i,2);
	}
	delete seedSorter;
	return true;

}

void SolidRake::GenSeedRegular(const vector<double>&usrExts, 
							   const size_t numSeeds[3], 
							  const double localmin[3], 
							  const double localmax[3], 
							  float* pSeed,
							  int stride)
{
	// eight corners
	float lll[3], hll[3], lhl[3], hhl[3], llh[3], hlh[3], lhh[3], hhh[3];
	int iFor, jFor, kFor;
	for(iFor = 0; iFor < 3; iFor++)
	{
		lll[iFor] = localmin[iFor];
		hhh[iFor] = localmax[iFor];
	}
	hll[0] = hhh[0];	hll[1] = lll[1];	hll[2] = lll[2];
	lhl[0] = lll[0];	lhl[1] = hhh[1];	lhl[2] = lll[2];
	hhl[0] = hhh[0];	hhl[1] = hhh[1];	hhl[2] = lll[2];
	llh[0] = lll[0];	llh[1] = lll[1];	llh[2] = hhh[2];
	hlh[0] = hhh[0];	hlh[1] = lll[1];	hlh[2] = hhh[2];
	lhh[0] = lll[0];	lhh[1] = hhh[1];	lhh[2] = hhh[2];

	int numPerX, numPerY, numPerZ;
	numPerX = numSeeds[0];
	numPerY = numSeeds[1];
	numPerZ = numSeeds[2];

	float xUnit, yUnit, zUnit;
	int index;
	xUnit = (float)1.0/(float)(numPerX+1);
	yUnit = (float)1.0/(float)(numPerY+1);
	zUnit = (float)1.0/(float)(numPerZ+1);
	index = 0;
	for(kFor = 0; kFor < numPerZ; kFor++)
		for(jFor = 0; jFor < numPerY; jFor++)
			for(iFor = 0; iFor < numPerX; iFor++)
			{
				float coeff[3];
				coeff[0] = (float)(iFor+1) * xUnit;
				coeff[1] = (float)(jFor+1) * yUnit;
				coeff[2] = (float)(kFor+1) * zUnit;

				pSeed[index] = usrExts[0]+TriLerp(lll[0], hll[0], lhl[0], hhl[0], llh[0], hlh[0], lhh[0], hhh[0], coeff);
				pSeed[index+1] = usrExts[1]+TriLerp(lll[1], hll[1], lhl[1], hhl[1], llh[1], hlh[1], lhh[1], hhh[1], coeff);
				pSeed[index+2] = usrExts[2]+TriLerp(lll[2], hll[2], lhl[2], hhl[2], llh[2], hlh[2], lhh[2], hhh[2], coeff);
				index += stride;
			}
}

Rake::PointSorter::PointSorter(int numPoints){
	pointHolder = new float[numPoints*4];
}
Rake::PointSorter::~PointSorter(){
	delete [] pointHolder;
}
void Rake::PointSorter::sortPoints(int first, int last, bool increasing){
	if (increasing)
		qsort(pointHolder+4*first,last-first+1,16,compLT);
	else
		qsort(pointHolder+4*first,last-first+1,16,compGT);
	
}
//ordering C functions used for quicksorting
int  VAPoR::compLT(const void* val1, const void* val2) { 
	if (*((float*)val1) < (*((float*)val2))) return -1;
	if (*((float*)val1) > (*((float*)val2))) return 1;
	return 0;
}
int  VAPoR::compGT(const void* val1, const void* val2) { 
	if (*((float*)val1) > (*((float*)val2))) return -1;
	if (*((float*)val1) < (*((float*)val2))) return 1;
	return 0;
}
