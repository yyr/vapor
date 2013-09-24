#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>
//#include "vdfcreate.h"

using namespace std;
using namespace VAPoR;

int main(int argc, char **argv) {
	std::string command = "roms";
	launchVdfCreate(argc, argv, command);
	exit(0);	
}
