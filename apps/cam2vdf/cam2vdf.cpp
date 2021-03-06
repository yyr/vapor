#include <cstdio>
#include <cstdlib>
#include <vapor/Copy2VDF.h>

using namespace VAPoR;
using namespace VetsUtil;

int main(int argc, char **argv) {
		
	MyBase::SetErrMsgFilePtr(stderr);
	Copy2VDF copy2vdf;
//	string command = "ROMS";
	string command = "CAM";
	int rc = copy2vdf.launch2vdf(argc, argv, command);

	if (rc == 0) exit(0);    
	else exit(1);
}
