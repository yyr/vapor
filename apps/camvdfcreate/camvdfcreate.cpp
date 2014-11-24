#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>
//#include "vdfcreate.h"

using namespace std;
using namespace VAPoR;
using namespace VetsUtil;

int main(int argc, char **argv) {
	
	MyBase::SetErrMsgFilePtr(stderr);
	vdfcreate vdfc;
	std::string command = "ROMS";
	int rc = vdfc.launchVdfCreate(argc, argv, command);

	if (rc == 0) return(0);
	else return(1);
}
