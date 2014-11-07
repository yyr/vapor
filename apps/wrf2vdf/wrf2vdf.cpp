#include <cstdio>
#include <cstdlib>
#include <vapor/WrfVDCcreator.h>

using namespace VAPoR;
using namespace VetsUtil;

int main(int argc, char **argv) {

    MyBase::SetErrMsgFilePtr(stderr);
    Wrf2vdf w2v = Wrf2vdf();
    int rc = w2v.launchWrf2Vdf(argc, argv);

    if (rc == 0) return(0);
    else return(1);

}
