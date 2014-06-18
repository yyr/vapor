#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <time.h>
#include <vector>


#include <vapor/Compressor.h>
#include <vapor/CFuncs.h>

using namespace VAPoR;
using namespace VetsUtil;

const int BX = 64;
const int BY = 64;
const int BZ = 64;
const int CRATIOS[] = {500,100,10,1};
const int NCRATIOS = sizeof(CRATIOS) / sizeof(CRATIOS[0]);
const int LOD = 3;
const string wname = "bior3.3";

double DecTime = 0.0;
double RecTime = 0.0;
double BrickTime = 0.0;

#define TIMER_START(T0)     double (T0) = GetTime();
#define TIMER_STOP(T0, T1)  (T1) += GetTime() - (T0);


void fetch_brick(
	const float *data, int nx, int ny, int nz, int x0, int y0, int z0, 
	float *brick, int bx, int by, int bz
) {

	TIMER_START(t0);

	for (int z = 0; z<bz; z++) {
	for (int y = 0; y<by; y++) {
	for (int x = 0; x<bx; x++) {
		brick[z*bx*by + y*bx + x] = data[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)];
	}
	}
	}
	TIMER_STOP(t0, BrickTime);
}

void put_brick(
	const float *brick, int bx, int by, int bz, 
	float *data, int nx, int ny, int nz, int x0, int y0, int z0
) {

	TIMER_START(t0);
	for (int z = 0; z<bz; z++) {
	for (int y = 0; y<by; y++) {
	for (int x = 0; x<bx; x++) {
		data[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)] = brick[z*bx*by + y*bx + x];
	}
	}
	}
	TIMER_STOP(t0, BrickTime);
}

void compute_error(
	const float *data, const float *cdata, int nx, int ny, int nz,
	double &l1, double &l2, double &lmax, double &rms
) {
	l1 = 0.0;
	l2 = 0.0;
	lmax = 0.0;
	rms = 0.0;
	for (int k=0; k<nz; k++) {
		for (int j=0; j<ny; j++) {
			for (int i=0; i<nx; i++) {
				double delta = fabs (data[k*nx*ny + j*nx + i] - cdata[k*nx*ny + j*nx + i]);
				l1 += delta;
				l2 += delta * delta;
				lmax = delta > lmax ? delta : lmax;
if (delta > 1e6) {
cout << "lmax, i, j, k " << lmax << " " << i << " " << j << " " << k << endl;
}
			}
		}
	}
	l2 = sqrt(l2);
	rms = l2 / (nx*ny*nz);
}

int main(int argc, char **argv) {

	assert(argc == 6);

	string srcfile = argv[1];
	string dstbase = argv[2];
	int nx = atoi(argv[3]);
	int ny = atoi(argv[4]);
	int nz = atoi(argv[5]);

	float *data, *cdata;
	float *brick, *cbrick, *coeff;

	assert(nx%BX == 0);
	assert(ny%BY == 0);
	assert(nz%BZ == 0);

	data = new float[nx*ny*nz];
	cdata = new float[nx*ny*nz];
	brick = new float [BX*BY*BZ];
	cbrick = new float [BX*BY*BZ];

	vector <size_t> dims;
	dims.push_back(nx);
	dims.push_back(ny);
	dims.push_back(nz);

	vector <size_t> bs;
	bs.push_back(BZ);
	bs.push_back(BY);
	bs.push_back(BX);

	coeff = new float [BX*BY*BZ];

	vector <size_t> cratios;
	for (int i=0; i<NCRATIOS; i++) {
		cratios.push_back(CRATIOS[i]);
	}

	FILE *fp;
	fp = fopen(srcfile.c_str(), "r");
	assert(fp != NULL);

	int rc = fread(data, sizeof(data[0]), nx*ny*nz, fp);
	fclose(fp);
	assert (rc == nx*ny*nz);


	vector<string> wnames;
	vector<string> modes;


	TIMER_START(t0);
	double time_compress = 0.0;

	DecTime = 0.0;
	RecTime = 0.0;
	BrickTime = 0.0;

	Compressor cmp(bs, wname);


	//
	// Set up ncoeff vector
	//
	vector <size_t> ncoeffs;
	size_t ntotal = cmp.GetNumWaveCoeffs();
	long naccum = 0;
	for (int i=0; i<cratios.size(); i++) {

		size_t n = ntotal / cratios[i];

		// There is a minumum number of coefficients that must be
		// used in reconstruction that places a lower bound on 
		// the compression rate
		//
		if (n < cmp.GetMinCompression()) {
			n = cmp.GetMinCompression();
		}

		n -= naccum; 	// only account for new coefficients

		if (n<1) n = 1;
		naccum += n;

		ncoeffs.push_back(n);

	}

	vector <SignificanceMap> sigmaps(ncoeffs.size());
	for (int z=0; z<nz; z+= BZ) {
		for (int y=0; y<ny; y+= BY) {
			for (int x=0; x<nx; x+= BX) {

				cout << "Compress " << x << " " << y << " " << z << endl;
				fetch_brick(data, nx, ny, nz, x, y, z, brick, BX, BY, BZ);

				TIMER_START(t1);
				cmp.Decompose(brick, coeff, ncoeffs, sigmaps);
				TIMER_STOP(t1, DecTime);

				TIMER_START(t2);
				cmp.Reconstruct(coeff, cbrick, sigmaps, -1);
				TIMER_STOP(t2, RecTime);

				put_brick(cbrick, BX, BY, BZ, cdata, nx, ny, nz, x, y, z);

			}
		}
	}

	TIMER_STOP(t0, time_compress);


	cout << "total compress time = " << time_compress << endl;
	cout << "	decomposition time = " << DecTime << endl;
	cout << "	reconstruct time = " << RecTime << endl;
	cout << "	brick time = " << BrickTime << endl;
	
	double l1, l2, lmax, rms;
	compute_error(data, cdata, nx, ny, nz, l1, l2, lmax, rms);

	cout << "L1 = " << l1 << endl;
	cout << "L2 = " << l2 << endl;
	cout << "LMax = " << lmax << endl;
	cout << "RMS = " << rms << endl;

}
