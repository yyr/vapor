#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>
//#include "vdfcreate.h"

using namespace std;
using namespace VAPoR;

int main(int argc, char **argv) {
	
	vdfcreate vdfc;
	std::string command = "roms";
	int rc = vdfc.launchVdfCreate(argc, argv, command);

	if (rc == 0) exit(0);
	else exit(1);
}
