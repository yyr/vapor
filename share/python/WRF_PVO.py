# Script to calculate Potential Vorticity
# Based on wrf_pvo.f in NCL WRF library
# Input 3D variables T, P, PB, U, V
# Input 2D variable F
# Output 3D variable WRF_PVO
# Results are not exactly the same as wrf_pvo.f because this is
# calculated in cartesian coords, not model layers.

#  helper routine: use 6th order differences for DA/DP (vertical direction)
#  based on deriv_findiff code
#  finds da/dp where deltap is a 3d differences array for p
def deriv_vert(a,deltap):
	s = shape(a)	#size of the input array
	aprime = array(a)	#output has the same size than input
	for i in range(3):
		aprime[i,:,:] = (-11*a[i,:,:]+18*a[i+1,:,:]-9*a[i+2,:,:]+2*a[i+3,:,:]) /(6*deltap[i,:,:])     #forward differences near the first boundary
	for i in range(3,s[0]-3):
		aprime[i,:,:] = (-a[i-3,:,:]+9*a[i-2,:,:]-45*a[i-1,:,:]+45*a[i+1,:,:] -9*a[i+2,:,:]+a[i+3,:,:])/(60*deltap[i,:,:]) #centered differences
	for i in range(s[0]-3,s[0]):
		aprime[i,:,:] = (-2*a[i-3,:,:]+9*a[i-2,:,:]-18*a[i-1,:,:]+11*a[i,:,:]) /(6*deltap[i,:,:])     #backward differences near the second boundary
	return aprime

PR = 0.01*(P+PB)
s = shape(P)	#size of the input array
# Need to find DX, DY, DZ
# Note that these are in Python coords, so DX is vertical 
ext = (__BOUNDS__[3]-__BOUNDS__[0], __BOUNDS__[4]-__BOUNDS__[1],__BOUNDS__[5]-__BOUNDS__[2])
usrmax = vapor.MapVoxToUser([__BOUNDS__[3],__BOUNDS__[4],__BOUNDS__[5]],__REFINEMENT__)
usrmin = vapor.MapVoxToUser([__BOUNDS__[0],__BOUNDS__[1],__BOUNDS__[2]],__REFINEMENT__)
dx = (usrmax[2]-usrmin[2])/ext[2]	# grid delta in Python coord system
dy =  (usrmax[1]-usrmin[1])/ext[1]
dz =  (usrmax[0]-usrmin[0])/ext[0]

DP = empty(s,float32)
# set up the P differences.  Avoid divide by zero
DP[0,:,:] = PR[1,:,:]-PR[0,:,:]
DP[1:s[0]-2,:,:] = PR[2:s[0]-1,:,:] - PR[0:s[0]-3,:,:]
DP[s[0]-1,:,:] = PR[s[0]-1,:,:]-PR[s[0]-2,:,:]
dpzeros = equal(DP, 0.0)
DP[dpzeros] = .0000001

DTHDP = deriv_vert(T,DP)
DTHDX = deriv_findiff(T,1,dx)
DTHDY = deriv_findiff(T,2,dy)
DUDP = deriv_vert(U,DP)
DVDP = deriv_vert(V,DP)
DUDY = deriv_findiff(U,2,dy)
DVDX = deriv_findiff(V,1,dx)
AVORT = DVDX - DUDY + F
WRF_PVO = float32(-9.81*(DTHDP*AVORT-DVDP*DTHDX+DUDP*DTHDY)*10000.)