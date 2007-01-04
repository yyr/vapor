#include "Rake.h"
#include "assert.h"
#include "vapor/VaporFlow.h"
#include "Field.h"
using namespace VetsUtil;
using namespace VAPoR;

/************************************************************************/
/*			 			SeedGenerator Class                              */
/************************************************************************/

SeedGenerator::SeedGenerator(const float min[3], 
							 const float max[3], 
							 const size_t numSeeds[3])
{
	for(int iFor = 0; iFor < 3; iFor++)
	{
		rakeMin[iFor] = min[iFor];
		rakeMax[iFor] = max[iFor];
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
		if(rakeMax[iFor] == rakeMin[iFor])
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
bool SeedGenerator::GetSeeds(VaporFlow* vFlow,
							 float *pSeeds, 
							 const bool bRandom, 
							 const unsigned int randomSeed)
{
	Rake* pRake;

	if(rakeDimension == POINT)
		pRake = new PointRake();
	else if(rakeDimension == LINE)
		pRake = new LineRake();
	else if(rakeDimension == PLANE)
		pRake = new PlaneRake();
	else if(rakeDimension == SOLID)
		pRake = new SolidRake();
	
	if(bRandom){
		if (distribBias == 0.f){ 
			pRake->GenSeedRandom(numSeeds, rakeMin, rakeMax, pSeeds, randomSeed);
		} else {
			//Setup for biased distribution:
			//First calc min/max of field in rake.
			float fieldMin, fieldMax;
			double minExt[3], maxExt[3];
			for (int i = 0; i< 3; i++) {minExt[i] = rakeMin[i]; maxExt[i] = rakeMax[i];}
			bool rc = vFlow->getFieldMagBounds(&fieldMin, &fieldMax, varx, vary, varz, 
				minExt, maxExt, numRefinements, timeStep);
			if (!rc) return false;
			//Then set up the FieldData
			FieldData* fData = vFlow->setupFieldData(varx, vary, varz, minExt, maxExt, numRefinements, timeStep, false);
			if (!fData) {delete pRake; return false;}
			/* check the values...
			float pnt[3];
			for (int i = 0; i< 33; i++){
				pnt[0] = (0.5f+(float)i)/32.f;
				for (int j = 0; j<64; j++) {
					pnt[1] = (0.5f+(float)j)/64.f;
					for (int k = 0; k<64; k++){
						pnt[2] = (0.5f+(float)k)/64.f;
						float mag2 = fData->getFieldMag(pnt);
						if (mag2 > 1.f)
							float mag3 = fData->getFieldMag(pnt);
					}
				}
			}
			*/
			rc = pRake->GenSeedBiased(distribBias,fieldMin,fieldMax, fData,numSeeds,  rakeMin,rakeMax, pSeeds, randomSeed);
			if (!rc) {delete fData; delete pRake; return false;}
		}
	}
	else
		pRake->GenSeedRegular(numSeeds, rakeMin, rakeMax, pSeeds);
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
void PointRake::GenSeedRandom(const size_t numSeeds[3], 
							  const float min[3], 
							  const float max[3], 
							  float* pSeed,
							  unsigned int randomSeed)
{
	for(int iFor = 0; iFor < 3; iFor++)
		pSeed[iFor] = min[iFor];
}
bool PointRake::GenSeedBiased(float , float , float , FieldData* , 
		const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed , unsigned int )
{
	for(int iFor = 0; iFor < 3; iFor++)
		pSeed[iFor] = min[iFor];
	return true;
}
void PointRake::GenSeedRegular(const size_t numSeeds[3], 
							  const float min[3], 
							  const float max[3], 
							  float* pSeed)
{
	for(int iFor = 0; iFor < 3; iFor++)
		pSeed[iFor] = min[iFor];
}
/************************************************************************/
/*					 		LineRake Class                              */
/************************************************************************/
LineRake::LineRake()
{
}



void LineRake::GenSeedRandom(const size_t numSeeds[3], 
							 const float min[3], 
							 const float max[3], 
							 float* pSeed,
							 unsigned int randomSeed)
{
	int totalNum;

	// initialize random number generator
	srand(randomSeed);

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	for(int iFor = 0; iFor < totalNum; iFor++)
	{
		// randomly generate a number between [0, 1]
		float alpha = (float)rand()/(float)RAND_MAX;

		// the random seed is generated by min*alpha + max*(1-alpha)
		pSeed[3*iFor + 0] = Lerp(min[0], max[0], alpha);
		pSeed[3*iFor + 1] = Lerp(min[1], max[1], alpha);
		pSeed[3*iFor + 2] = Lerp(min[2], max[2], alpha);
	}
}

bool LineRake::GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData* fData, 
		const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed)
{
	int totalNum;

	// initialize random number generator
	srand(randomSeed);

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	
	//Repeatedly try to find seeds. Try at most 100000 times
	int seedCount = 0;
	for (int i = 0; i< 100000; i++) 
	{
		float alpha, point[3];
		alpha = (float)rand()/(float)RAND_MAX;

		float randomTest = (float)rand()/(float)RAND_MAX;
		
		point[0] = Lerp(min[0], max[0], alpha);
		point[1] = Lerp(min[1], max[1], alpha);
		point[2] = Lerp(min[2], max[2], alpha);
		
		
		float mag = fData->getFieldMag(point);
		//Now test if it's OK.
		float ratio = (mag-fieldMin)/(fieldMax-fieldMin);
		float acceptProb = pow(ratio,bias);
		//The probability that we accept this point is acceptProb;
		//The value of randomTest is <= P with probability P.
		if (randomTest <= acceptProb) { //Accept this point.
			pSeed[3*seedCount] = point[0];
			pSeed[3*seedCount+1] = point[1];
			pSeed[3*seedCount+2] = point[2];
			seedCount++;
			if (seedCount >= totalNum) break;
		}
	}
	return (seedCount >= totalNum);
}


void LineRake::GenSeedRegular(const size_t numSeeds[3], 
							 const float min[3], 
							 const float max[3], 
							 float* pSeed)
{
	int totalNum;
	float unitRatio;
	float alpha;

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	unitRatio = (float)1.0/(float)(totalNum+1);
	for(int iFor = 0; iFor < totalNum; iFor++)
	{
		alpha = (float)(iFor+1) * unitRatio;
		pSeed[3*iFor + 0] = Lerp(min[0], max[0], alpha);
		pSeed[3*iFor + 1] = Lerp(min[1], max[1], alpha);
		pSeed[3*iFor + 2] = Lerp(min[2], max[2], alpha);
	}
}

/************************************************************************/
/*					 		PlaneRake Class                             */
/************************************************************************/
PlaneRake::PlaneRake()
{
}


void PlaneRake::GenSeedRandom(const size_t numSeeds[3], 
							  const float min[3], 
							  const float max[3], 
							  float* pSeed,
							  unsigned int randomSeed)
{
	int totalNum;
	// eight corners
	float ll[3], hl[3], hh[3], lh[3];
	int iFor;

	for(iFor = 0; iFor < 3; iFor++)
	{
		ll[iFor] = min[iFor];
		hh[iFor] = max[iFor];
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
		
		pSeed[3*iFor + 0] = BiLerp(ll[0], hl[0], lh[0], hh[0], coeff);
		pSeed[3*iFor + 1] = BiLerp(ll[1], hl[1], lh[1], hh[1], coeff);
		pSeed[3*iFor + 2] = BiLerp(ll[2], hl[2], lh[2], hh[2], coeff);
	}
}
bool PlaneRake::GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData* fData, 
		const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed)
{
	int totalNum;
	// eight corners
	float ll[3], hl[3], hh[3], lh[3];
	int iFor;

	for(iFor = 0; iFor < 3; iFor++)
	{
		ll[iFor] = min[iFor];
		hh[iFor] = max[iFor];
	}

	hl[0] = hh[0];	hl[1] = ll[1];	hl[2] = ll[2];
	lh[0] = ll[0];	lh[1] = hh[1];	lh[2] = hh[2];

	// initialize random number generator
	srand(randomSeed);

	totalNum = numSeeds[0] * numSeeds[1] * numSeeds[2];
	//Repeatedly try to find seeds. Try at most 100000 times
	int seedCount = 0;
	for (int i = 0; i< 100000; i++) 
	{
		float coeff[2], point[3];
		coeff[0] = (float)rand()/(float)RAND_MAX;
		coeff[1] = (float)rand()/(float)RAND_MAX;

		float randomTest = (float)rand()/(float)RAND_MAX;
		
		point[0] = BiLerp(ll[0], hl[0], lh[0], hh[0], coeff);
		point[1] = BiLerp(ll[1], hl[1], lh[1], hh[1], coeff);
		point[2] = BiLerp(ll[2], hl[2], lh[2], hh[2], coeff);
		
		float mag = fData->getFieldMag(point);
		//Now test if it's OK.
		float ratio = (mag-fieldMin)/(fieldMax-fieldMin);
		float acceptProb = pow(ratio,bias);
		//The probability that we accept this point is acceptProb;
		//The value of randomTest is <= P with probability P.
		if (randomTest <= acceptProb) { //Accept this point.
			pSeed[3*seedCount] = point[0];
			pSeed[3*seedCount+1] = point[1];
			pSeed[3*seedCount+2] = point[2];
			seedCount++;
			if (seedCount >= totalNum) break;
		}
	}
	return (seedCount >= totalNum);
}
void PlaneRake::GenSeedRegular(const size_t numSeeds[3], 
							  const float min[3], 
							  const float max[3], 
							  float* pSeed)
{
	// eight corners
	float ll[3], hl[3], hh[3], lh[3];
	int iFor, jFor;
	for(iFor = 0; iFor < 3; iFor++)
	{
		ll[iFor] = min[iFor];
		hh[iFor] = max[iFor];
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

			pSeed[index++] = BiLerp(ll[0], hl[0], lh[0], hh[0], coeff);
			pSeed[index++] = BiLerp(ll[1], hl[1], lh[1], hh[1], coeff);
			pSeed[index++] = BiLerp(ll[2], hl[2], lh[2], hh[2], coeff);
		}
}

/************************************************************************/
/*					 		SolidRake Class                             */
/************************************************************************/
SolidRake::SolidRake()
{
}



void SolidRake::GenSeedRandom(const size_t numSeeds[3], 
							  const float min[3], 
							  const float max[3], 
							  float* pSeed,
							  unsigned int randomSeed)
{
	int totalNum;
	// eight corners
	float lll[3], hll[3], lhl[3], hhl[3], llh[3], hlh[3], lhh[3], hhh[3];
	int iFor;

	for(iFor = 0; iFor < 3; iFor++)
	{
		lll[iFor] = min[iFor];
		hhh[iFor] = max[iFor];
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

		pSeed[3*iFor + 0] = TriLerp(lll[0], hll[0], lhl[0], hhl[0], llh[0], hlh[0], lhh[0], hhh[0], coeff);
		pSeed[3*iFor + 1] = TriLerp(lll[1], hll[1], lhl[1], hhl[1], llh[1], hlh[1], lhh[1], hhh[1], coeff);
		pSeed[3*iFor + 2] = TriLerp(lll[2], hll[2], lhl[2], hhl[2], llh[2], hlh[2], lhh[2], hhh[2], coeff);
	}
	
}
bool SolidRake::GenSeedBiased(float bias, float fieldMin, float fieldMax, FieldData* fData, 
		const size_t numSeeds[3], const float min[3], const float max[3], float* pSeed, unsigned int randomSeed){
	int totalNum;
	// eight corners
	float lll[3], hll[3], lhl[3], hhl[3], llh[3], hlh[3], lhh[3], hhh[3];
	

	for(int iFor = 0; iFor < 3; iFor++)
	{
		lll[iFor] = min[iFor];
		hhh[iFor] = max[iFor];
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
	//Repeatedly try to find seeds. Try at most 100000 times
	int seedCount = 0;
	int trials;
	for (trials = 0; trials< 100000; trials++) 
	{
		float coeff[3], point[3];
		float randomTest = 0.f;
		//A value of 0.f or 1.f is unacceptable...
		while(randomTest == 1.f || randomTest == 0.f) randomTest = (float)rand()/(float)RAND_MAX;
	

		coeff[0] = (float)rand()/(float)RAND_MAX;
		coeff[1] = (float)rand()/(float)RAND_MAX;
		coeff[2] = (float)rand()/(float)RAND_MAX;

		
		
		point[0] = TriLerp(lll[0], hll[0], lhl[0], hhl[0], llh[0], hlh[0], lhh[0], hhh[0], coeff);
		point[1] = TriLerp(lll[1], hll[1], lhl[1], hhl[1], llh[1], hlh[1], lhh[1], hhh[1], coeff);
		point[2] = TriLerp(lll[2], hll[2], lhl[2], hhl[2], llh[2], hlh[2], lhh[2], hhh[2], coeff);

		float mag = fData->getFieldMag(point);
		if (mag < 0.f) continue;  //Point is out of range
		//Now test if it's OK.
		float ratio, acceptProb;
		if (bias > 0.f){
			ratio = (mag-fieldMin)/(fieldMax-fieldMin);
			acceptProb = pow(ratio,bias);
		} else {
			ratio = (fieldMax - mag)/(fieldMax-fieldMin);
			acceptProb = pow(ratio,-bias);
		}
		assert (ratio >= 0.f && ratio <= 1.f);
			
		//The probability that we accept this point is acceptProb;
		//The value of randomTest is <= P with probability P.
		if (randomTest <= acceptProb) { //Accept this point.
			pSeed[3*seedCount] = point[0];
			pSeed[3*seedCount+1] = point[1];
			pSeed[3*seedCount+2] = point[2];
			seedCount++;
			if (seedCount >= totalNum) break;
		}
	}
	return (seedCount >= totalNum);
}

void SolidRake::GenSeedRegular(const size_t numSeeds[3], 
							  const float min[3], 
							  const float max[3], 
							  float* pSeed)
{
	// eight corners
	float lll[3], hll[3], lhl[3], hhl[3], llh[3], hlh[3], lhh[3], hhh[3];
	int iFor, jFor, kFor;
	for(iFor = 0; iFor < 3; iFor++)
	{
		lll[iFor] = min[iFor];
		hhh[iFor] = max[iFor];
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

				pSeed[index++] = TriLerp(lll[0], hll[0], lhl[0], hhl[0], llh[0], hlh[0], lhh[0], hhh[0], coeff);
				pSeed[index++] = TriLerp(lll[1], hll[1], lhl[1], hhl[1], llh[1], hlh[1], lhh[1], hhh[1], coeff);
				pSeed[index++] = TriLerp(lll[2], hll[2], lhl[2], hhl[2], llh[2], hlh[2], lhh[2], hhh[2], coeff);
			}
}
