#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>

#include <vapor/MetadataVDC.h>
#include <vapor/CFuncs.h>
#include <vapor/WaveCodecIO.h>

using namespace std;
using namespace VetsUtil;
using namespace VAPoR;
MetadataVDC *file;
vector<size_t> cratios;
vector<string> variables;
size_t global[3];
size_t bsize[3];
char *wname = "bior3.3";
char *wmode = "symh";
string *file_path;
#define CREATE_F90 FC_FUNC(CreateVDF)
#define DEFINE_F90 FC_FUNC(DefVDFVar)
#define ENDDEF_F90 FC_FUNC(EndVDFDef)
#define WRITE_F90 FC_FUNC(WriteVDC2Var)
#define READ_F90 FC_FUNC(ReadVDC2Var)
extern "C" void CREATE_F90(int *griddims, int *bs, char * path, int path_len)
{
  cratios.push_back(1);
  cratios.push_back(10);
  cratios.push_back(100);
  cratios.push_back(500);
  global[0] = griddims[0];
  global[1] = griddims[1];
  global[2] = griddims[2];
  bsize[0] = bs[0];
  bsize[1] = bs[1];
  bsize[2] = bs[2];
  file_path = new string(path);
  file = new MetadataVDC(global, bsize, cratios, wname, wmode);
}
extern "C" void DEFINE_F90(char * name, int name_len)
{
  string var(name);
  variables.push_back(name);
  int pos = std::find(variables.begin(), variables.end(), name);
  *varid = pos;
}
extern "C" void ENDDEF_F90()
{
  file->SetVariables3D(variables);
  file->write(file_path);
  delete file;
}
extern "C" void WRITE_F90(float *array, int *start, int *len, int *IO_Comm_f, int *ts, int *lod, int *reflevel, char *name, int name_len, int num_iotasks) //AIX no name mangling
{
  
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Comm IO_Comm = MPI_Comm_f2c(*IO_Comm_f);
#ifdef PIOVDC_DEBUG
  std::cout << "@example data write_vdc2 " << array[0] << ":" << array[1] << std::endl;
#endif
  MyBase::SetErrMsgFilePtr(stderr);

  MyBase::SetDiagMsg("@ndims: %d array[0]: %f\n", ndims, array[0]);
  for(int i=0; i<*ndims; i++)
    MyBase::SetDiagMsg("@start[%d]: %d, len[%d]: %d\n", i, start[i], i, len[i]);
  int start_block[3];
  int end_block[3];

  end_block[0] = start[0] + len[0] - 2 ;

  end_block[1] = start[1] + len[1] - 2;

  end_block[2] = start[2] + len[2] - 2;

#ifdef DEBUG
  std::cout << "VDC2 rank: " << rank << " start: " << start_block[0] << "," << start_block[1] << "," << start_block[2] << " end: " << end_block[0] << "," << end_block[1] << "," << end_block[2] << " start2: " << start[0] << "," << start[1] << "," << start[2] << " len: " << len[0] << "," << len[1] << "," << len[2] <<  " bsize: " << bsize[0] << "," << bsize[1] << "," << bsize[2] << std::endl;
#endif
  MyBase::SetDiagMsg("@st: %d %d %d, en: %d %d %d\n", 
		     start_block[0], start_block[1], start_block[2],
		     end_block[0], end_block[1], end_block[2]);
  xform_and_write(array, start, end_block, len, IO_Comm, ts, lod, bsize, name, num_iotasks);

#ifdef PIOVDC_DEBUG
  if(rank == 0)
    std::cout << "VDC2 transforms complete" << std::endl;
#endif
}

extern "C" void READ_F90(float *array, int *start, int *len, int *IO_Comm_f, int *ts, int *lod, int *reflevel, char *name, int name_len, int num_iotasks) //AIX no name mangling
{
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Comm IO_Comm = MPI_Comm_f2c(*IO_Comm_f);

  MyBase::SetErrMsgFilePtr(stderr);
  int start_block[3];
  int end_block[3];

  end_block[0] = start[0] + len[0] - 2 ;

  end_block[1] = start[1] + len[1] - 2;

  end_block[2] = start[2] + len[2] - 2;

#ifdef DEBUG
  std::cout << "VDC2 read_vdc2_var rank: " << rank << " start: " << start_block[0] << "," << start_block[1] << "," << start_block[2] << " end: " << end_block[0] << "," << end_block[1] << "," << end_block[2] << " start2: " << start[0] << "," << start[1] << "," << start[2] << " len: " << len[0] << "," << len[1] << "," << len[2] <<  " bsize: " << bsize[0] << "," << bsize[1] << "," << bsize[2] << " vdf: " << vdf << " name: " << name << std::endl;
#endif
  MyBase::SetDiagMsg("@st: %d %d %d, en: %d %d %d\n", 
		     start_block[0], start_block[1], start_block[2],
		     end_block[0], end_block[1], end_block[2]);
  inverse_and_read(array, start, end_block, len, IO_Comm, ts, lod, reflevel, bsize, name);

#ifdef DEBUG
  if(rank == 0)
    std::cout << "VDC2 transforms complete" << std::endl;
#endif
}

void xform_and_write(const float *data, int *start, int *end, int *count,MPI_Comm IOComm, int *ts, int *lod, int *reflevel,  char* name, int num_iotasks){
  //Obtain MPI Rank for timing

  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#ifdef PIOVDC_DEBUG
  double starttime = MPI_Wtime();
#endif
  MetadataVDC metadata(file_path.c_str());
  WaveCodecIO *wcwriter = new WaveCodecIO(metadata, 1);

  wcwriter->SetIOComm(IOComm);

  bool collective = true;
  
  int proc_count[3];
  int comm_size = num_iotasks;

  MPI_Allreduce(count, proc_count, 3, MPI_INT, MPI_SUM, IOComm);


  proc_count[0] = proc_count[0] / comm_size;
  proc_count[1] = proc_count[1] / comm_size;
  proc_count[2] = proc_count[2] / comm_size;

  if(proc_count[0] != count[0] || proc_count[1] != count[1] || proc_count[2] != count[2])
    collective = false;

  wcwriter->SetCollectiveIO(collective);

  size_t temp_count[3] = {count[0], count[1], count[2]};

  wcwriter->EnableBuffering(temp_count, 1, rank);
  //Write out data array
  if (wcwriter->GetErrCode() != 0) perror("wcwriter ctor failed\n");
#ifdef PIOVDC_DEBUG
  double openbegin = MPI_Wtime();
#endif

  if (wcwriter->OpenVariableWrite((size_t)*ts, name, *reflevel, *lod) < 0) perror("openVar failed\n");
#ifdef PIOVDC_DEBUG
  double opentime = MPI_Wtime() - openbegin;
#endif
  int start_block[3], end_block[3];
  
  start_block[0]=( start[0] -1  )/ bsize[0];
  start_block[1] =( start[1] -1 ) / bsize[1];
  start_block[2] = (start[2] -1 ) / bsize[2];

  end_block[0]  =   end[0]  / bsize[0] ;
  end_block[1] =  end[1] / bsize[1] ;
  end_block[2] =  end[2]  / bsize[2] ;

  int local_start[3] = {0, 0, 0};
  int local_end[3] = {count[0] / bsize[0], count[1] / bsize[1] , count[2] / bsize[2]};

  //iterate through data block
  int blk_counter =0, data_counter=0, total = 0;
  wcwriter->xformMPI = 0.0;
  wcwriter->ioMPI = 0.0;
  wcwriter->methodTimer = 0.0;
  wcwriter->methodThreadTimer = 0.0;
  float *buffer = (float *)malloc(sizeof(float) * bsize[0] * bsize[1] * bsize[2]);

  if(collective)
    {
      const size_t sblock[3] = { start_block[0], start_block[1], start_block[2]};
      const size_t eblock[3] = { end_block[0] , end_block[1]  , end_block[2]};
      if (wcwriter->BlockWriteRegion(data, sblock, eblock) < 0) {
	std::string fail ("writeRegion failed, rank: ");
	fail += rank;
	fail += " \n";
	perror(fail.c_str());
      }
    }
  else
    {
      for(int k = start_block[2]; k <= end_block[2]; k++){
	for(int j = start_block[1]; j <= end_block[1]; j++){
	  for(int i = start_block[0]; i <= end_block[0]; i++){
	    const size_t sblock[3] = { i, j, k };
	    const size_t eblock[3] = { i , j  , k};
	    //copy block into temp memory

	    for(int z = 0; z < bsize[2]; z++){
	      for(int y = 0; y < bsize[1]; y++){
		for(int x = 0; x < bsize[0]; x++){
		  int idex = i * bsize[0];
		  int jdex = j * bsize[1];
		  int kdex = k * bsize[2];
		  if(idex+x  < global[0] && jdex + y < global[1] && kdex + z < global[2]){
		    buffer[x + y * bsize[0] + z * bsize[0] * bsize[1]] =  (float)data[local_start[0] + x + (local_start[1] + y)  * count[0] + (local_start[2] + z) * count[0] * count[1]];
		  }
		  else{
		    buffer[x + y * bsize[0] + z * bsize[0] * bsize[1]] = -1.0;
		  } 

		}
	      }
	    }
	    double block = MPI_Wtime();
	    if (wcwriter->BlockWriteRegion(buffer, sblock, eblock) < 0) {
	      std::string fail ("writeRegion failed, rank: ");
	      fail += rank;
	      fail += " \n";
	      perror(fail.c_str());
	    }
	    local_start[0] += bsize[0];
	  }
	  local_start[1] += bsize[1];
	  local_start[0] = 0;
	}
	local_start[2] += bsize[2];
	local_start[1] = 0;
	local_start[0] = 0;
      }
    }
#ifdef PIOVDC_DEBUG
  std::cout << "@Rank" << rank << " finished writing " << std::endl;
#endif

#ifdef PIOVDC_DEBUG
  double close_begin = MPI_Wtime();  
#endif
  wcwriter->CloseVariable();
#ifdef PIOVDC_DEBUG
  double closetime = MPI_Wtime() - close_begin;
#endif
  if (wcwriter->GetErrCode() != 0) perror("closeVar failed\n");
#ifdef PIOVDC_DEBUG
  double vdftime = MPI_Wtime() - starttime;
  std::cout << "rank: " << rank << " VDC2 VDF aggregate time: " << vdftime  << " xformMPI: " << wcwriter->xformMPI << " ioMPI: " << wcwriter->ioMPI << " BlockWriteRegionTimer: " << wcwriter->methodTimer << " BlockWriteRegionThreadTimer: " << wcwriter->methodThreadTimer << " Pure Block Writing: " << onlyBlocks <<  " OpenVarWriteTime: " << opentime << " CloseVarWriteTime: " << closetime << std::endl;
#endif
  delete wcwriter;
}

void inverse_and_read(float *data, int *start, int *end, int *count,MPI_Comm IOComm, int *ts, int *lod, int reflevel, char* vdf,  char* name){
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#ifdef PIOVDC_DEBUG
  double starttime = MPI_Wtime();
#endif
  MetadataVDC metadata(vdf);
  WaveCodecIO *wcwriter = new WaveCodecIO(metadata, 1);

  wcwriter->SetIOComm(IOComm);

  bool collective = true;

  if(global[0] % bsize[0] != 0 || global[1] % bsize[1] != 0 || global[2] % bsize[2] !=0)
    collective = false;

  wcwriter->SetCollectiveIO(collective);

  wcwriter->SetCollectiveIO(collective);
  size_t temp_count[3] = {count[0], count[1], count[2]};

  //Write out data array
  if (wcwriter->GetErrCode() != 0) perror("wcwriter ctor failed\n");
#ifdef PIOVDC_DEBUG
  double openbegin = MPI_Wtime();
#endif
  if (wcwriter->OpenVariableRead((size_t)*ts, name, *reflevel, *lod) < 0) perror("openVar failed\n");
#ifdef PIOVDC_DEBUG
  double opentime = MPI_Wtime() - openbegin;
#endif

  int start_block[3], end_block[3];
  
  start_block[0]=( start[0] -1  )/ bsize[0];
  start_block[1] =( start[1] -1 ) / bsize[1];
  start_block[2] = (start[2] -1 ) / bsize[2];

  end_block[0]  =   end[0]  / bsize[0] ;
  end_block[1] =  end[1] / bsize[1] ;
  end_block[2] =  end[2]  / bsize[2] ;

  int local_start[3] = {0, 0, 0};
  int local_end[3] = {count[0] / bsize[0], count[1] / bsize[1] , count[2] / bsize[2]};

  //iterate through data block
  wcwriter->xformMPI = 0.0;
  wcwriter->ioMPI = 0.0;
  wcwriter->methodTimer = 0.0;
  wcwriter->methodThreadTimer = 0.0;


  const size_t sblock[3] = { start_block[0], start_block[1], start_block[2] };
  const size_t eblock[3] = { end_block[0] , end_block[1]  , end_block[2]};
  if (wcwriter->BlockReadRegion(sblock, eblock, data) < 0) {
    std::string fail ("BlockReadRegion failed, rank: ");
    fail += rank;
    fail += " \n";
    perror(fail.c_str());
  }

#ifdef PIOVDC_DEBUG
  double close_begin = MPI_Wtime();  
#endif
  wcwriter->CloseVariable();
#ifdef PIOVDC_DEBUG
  double closetime = MPI_Wtime() - close_begin;
#endif
  if (wcwriter->GetErrCode() != 0) perror("closeVar failed\n");
#ifdef PIOVDC_DEBUG
  double vdftime = MPI_Wtime() - starttime;
  std::cout << "rank: " << rank << " VDC2 VDF aggregate time: " << vdftime  << " xformMPI: " << wcwriter->xformMPI << " ioMPI: " << wcwriter->ioMPI << " BlockWriteRegionTimer: " << wcwriter->methodTimer << " BlockWriteRegionThreadTimer: " << wcwriter->methodThreadTimer <<  " OpenVarReadTime: " << opentime << " CloseVarReadTime: " << closetime << std::endl;
#endif
  delete wcwriter;
}

