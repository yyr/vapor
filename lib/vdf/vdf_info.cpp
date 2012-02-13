#include <vapor/MetadataVDC.h>
#include <iostream>
using namespace VAPoR;
using namespace std;
extern "C" void getvdfinfo_(int* bs, int* dims, long* ts, int *rank,  char* vdf, int* vdf_len)
{
     
  MetadataVDC *file = new MetadataVDC(vdf);

  size_t *block = new size_t[3];
  size_t *dimensions = new size_t[3];
  file->GetBlockSize(block, 0);
  file->GetGridDim(dimensions);
  long steps = file->GetNumTimeSteps();
  ts = &steps;      
  for(int i = 0; i < 3; i++){
    bs[i] = block[i];
    dims[i] = dimensions[i];
  }
}
