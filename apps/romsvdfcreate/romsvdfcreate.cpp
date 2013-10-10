#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>
//#include "vdfcreate.h"

using namespace std;
using namespace VAPoR;

int main(int argc, char **argv) {
	
	vdfcreate vdfc;
	std::string command = "roms";
	vdfc.launchVdfCreate(argc, argv, command);
	exit(0);	
}
