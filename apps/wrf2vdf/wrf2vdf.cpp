#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <vapor/WrfVDCcreator.h>

using namespace VAPoR;
using namespace VetsUtil;

int main(int argc, char **argv) {

    MyBase::SetErrMsgFilePtr(stderr);
    Wrf2vdf w2v = Wrf2vdf();
	
	std::clock_t Cstart;
	double duration;
	Cstart = std::clock();
    int rc = w2v.launchWrf2Vdf(argc, argv);
	duration = (std::clock() - Cstart) / (double) CLOCKS_PER_SEC;
	cout << "cmd line process time: " << duration << endl;
    if (rc == 0) return(0);
    else return(1);

}
