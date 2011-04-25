# Script to calculate Potential Vorticity
# Based on wrf_pvo.f in NCL WRF library
# Note that U and V have already been unstaggered
# Uses 3D variables T, P, PB, U, V
# uses 2D variables MAPFAC_U, MAPFAC_V, MAPFAC_M, F

PR = P+PB
s = shape(P)	#size of the input array
ss = [1,s[1],s[2]] # shape of 2d arrays
# Need to find DX, DY, DZ
# Note that these are in Python coords, so DX is vertical 
ext = (__BOUNDS__[3]-__BOUNDS__[0], __BOUNDS__[4]-__BOUNDS__[1],__BOUNDS__[5]-__BOUNDS__[2])
usrmax = vapor.MapVoxToUser([__BOUNDS__[3],__BOUNDS__[4],__BOUNDS__[5]],__REFINEMENT__)
usrmin = vapor.MapVoxToUser([__BOUNDS__[0],__BOUNDS__[1],__BOUNDS__[2]],__REFINEMENT__)
dx = (usrmax[2]-usrmin[2])/ext[2]	# grid delta in Python coord system
dy =  (usrmax[1]-usrmin[1])/ext[1]
dz =  (usrmax[0]-usrmin[0])/ext[0]
	
DUDP = empty(s,float32)
DVDP = empty(s,float32)
DUDY = empty(s,float32)
DVDX = empty(s,float32)
DTHDP = empty(s,float32)
DTHDX = empty(s,float32)
DTHDY = empty(s,float32)
DP = empty(s,float32)
# set up the P differences.  Avoid divide by zero
DP[0,:,:] = PR[1,:,:]-PR[0,:,:]
DP[1:s[0]-2,:,:] = PR[2:s[0]-1,:,:] - PR[0:s[0]-3,:,:]
DP[s[0]-1,:,:] = PR[s[0]-1,:,:]-PR[s[0]-2,:,:]
dpzeros = equal(DP, 0.0)
DP[dpzeros] = -0.01
#Reshape the 2d vars so they are compatible with 3d vars

MFU = ones(ss,float32)
MFU[0,:,:] = MAPFAC_U[:,:]
MFV = ones(ss,float32)
MFV[0,:,:] = MAPFAC_V[:,:]
FF = zeros(ss,float32)
FF[0,:,:] = F[:,:]

MM=ones(ss,float32)
MM[0,:,:] = MAPFAC_M[:,:]*MAPFAC_M[:,:]

DTHDP[0,:,:]= (T[1,:,:]-T[0,:,:])/DP[0,:,:]
DTHDP[s[0]-1,:,:]= (T[s[0]-1,:,:]-T[s[0]-2,:,:])/DP[s[0]-1,:,:]
DTHDP[1:s[0]-2,:,:] = (T[2:s[0]-1,:,:]-T[0:s[0]-3,:,:])/DP[1:s[0]-2,:,:]

DUDP[0,:,:]= (U[1,:,:]-U[0,:,:])/DP[0,:,:]
DUDP[s[0]-1,:,:]= (U[s[0]-1,:,:]-U[s[0]-2,:,:])/DP[s[0]-1,:,:]
DUDP[1:s[0]-2,:,:] = (U[2:s[0]-1,:,:]-U[0:s[0]-3,:,:])/DP[1:s[0]-2,:,:]

DVDP[0,:,:]= (V[1,:,:]-V[0,:,:])/DP[0,:,:]
DVDP[s[0]-1,:,:]= (V[s[0]-1,:,:]-V[s[0]-2,:,:])/DP[s[0]-1,:,:]
DVDP[1:s[0]-2,:,:] = (V[2:s[0]-1,:,:]-V[0:s[0]-3,:,:])/DP[1:s[0]-2,:,:]

DUDY[:,0,:]= (U[:,1,:]/MFU[:,1,:]-U[:,0,:]/MFU[:,0,:])/(MM[:,0,:]*dy)
DUDY[:,s[1]-1,:]= (U[:,s[1]-1,:]/MFU[:,s[1]-1,:]-U[:,s[1]-2,:]/MFU[:,s[1]-2,:])/(MM[:,s[1]-1,:]*dy)
DUDY[:,1:s[1]-2,:] = (U[:,2:s[1]-1,:]/MFU[:,2:s[1]-1,:]-U[:,0:s[1]-3,:]/MFU[:,0:s[1]-3,:])/(MM[:,1:s[1]-2,:]*dy)

DVDX[:,:,0]= (V[:,:,1]/MFV[:,:,1]-V[:,:,0]/MFV[:,:,0])/(MM[:,:,0]*dx)
DVDX[:,:,s[2]-1]= (V[:,:,s[2]-1]/MFV[:,:,s[2]-1]-V[:,:,s[2]-2]/MFV[:,:,s[2]-2])/(MM[:,:,s[2]-1]*dx)
DVDX[:,:,1:s[2]-2] = (V[:,:,2:s[2]-1]/MFV[:,:,2:s[2]-1]-V[:,:,0:s[2]-3]/MFV[:,:,0:s[2]-3])/(MM[:,:,1:s[2]-2]*dx)

AVORT = DVDX - DUDY + FF
WRF_PV = float32(-9.81*(DTHDP*AVORT-DVDP*DTHDX+DUDP*DTHDY)*1000000.)