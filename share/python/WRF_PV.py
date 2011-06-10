#calculation of WRF_PV from WRF variables
#3D inputs: P,PB,U,V,T
#2D input: F
#similar to DCOMPUTEPV in NCL and RIP, but uses 6th order finite differences for derivatives.
#also ignores horizontal distortion of grid.
ext = (__BOUNDS__[3]-__BOUNDS__[0], __BOUNDS__[4]-__BOUNDS__[1],__BOUNDS__[5]-__BOUNDS__[2])
usrmax = vapor.MapVoxToUser([__BOUNDS__[3],__BOUNDS__[4],__BOUNDS__[5]],__REFINEMENT__)
usrmin = vapor.MapVoxToUser([__BOUNDS__[0],__BOUNDS__[1],__BOUNDS__[2]],__REFINEMENT__)
dx = (usrmax[2]-usrmin[2])/ext[2]	# grid delta in user coord system
dy =  (usrmax[1]-usrmin[1])/ext[1]
dz =  (usrmax[0]-usrmin[0])/ext[0]
PR=P+PB
DPDZ = deriv_findiff(PR,3,dz) 
dpzeros = equal(DPDZ, 0.0)
#avoid divide by zero:
DPDZ[dpzeros] = .00001
DTHDZ = deriv_findiff(T,3,dz)
DTHDX = deriv_findiff(T,1,dx)
DTHDY = deriv_findiff(T,2,dy)
DUDZ = deriv_findiff(U,3,dz)
DVDZ = deriv_findiff(V,3,dz)
DUDY = deriv_findiff(U,2,dy)
DVDX = deriv_findiff(V,1,dx)
AVORT = DVDX - DUDY + F
#use hydrostatic balance assumption:  -rho*g ~= DP/DZ
rho_recip = -9.81e06/DPDZ
WRF_PV = float32((DTHDZ*AVORT-DVDZ*DTHDX+DUDZ*DTHDY)*rho_recip)