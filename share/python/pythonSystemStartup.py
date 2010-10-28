from numpy import *

def mag3d(a1,a2,a3): 
	return sqrt(a1*a1 + a2*a2 + a3*a3)

def mag2d(a1,a2): 
	return sqrt(a1*a1 + a2*a2)


# NAME:
#       DERIV_FINDIFF
#
# PURPOSE:
#       Computes first order derivatives using sixth order finite
#       differences in regular Cartesian grids
#
# CALLING SEQUENCE:
#       APRIME = DERIV_FINDIFF,IN,DIR,DX
#
# PARAMETERS:
#       A[in]:   3D array with the field component
#       DIR[in]:  direction (1,2, or 3) of the the derivative
#       DX[in]:   spatial step of the grid in the direction DIR
#
# RETURNS:       
#       APRIME[out]: 3D array with the spatial derivative
#
#
def deriv_findiff(a,dir,dx):

	s = shape(a)	#size of the input array
	aprime = array(a)	#output has the same size than input

	#
	# derivative for dir=1
	#
	if dir == 1: 

		#forward differences near the first boundary
		for i in range(3):
			aprime[:,:,i] = (-11*a[:,:,i]+18*a[:,:,i+1]-9*a[:,:,i+2]+2*a[:,:,i+3]) / (6*dx)     

		#centered differences
		for i in range(3,s[2]-3):
			aprime[:,:,i] = (-a[:,:,i-3]+9*a[:,:,i-2]-45*a[:,:,i-1]+45*a[:,:,i+1] -9*a[:,:,i+2]+a[:,:,i+3])/(60*dx) 

		#backward differences near the second boundary
		for i in range(s[2]-3,s[2]):
			aprime[:,:,i] = (-2*a[:,:,i-3]+9*a[:,:,i-2]-18*a[:,:,i-1]+11*a[:,:,i]) /(6*dx)     

	#
	# derivative for dir=2
	#
	if dir == 2: 
		for i in range(3):
			aprime[:,i,:] = (-11*a[:,i,:]+18*a[:,i+1,:]-9*a[:,i+2,:]+2*a[:,i+3,:]) /(6*dx)     #forward differences near the first boundary

		for i in range(3,s[1]-3):
			aprime[:,i,:] = (-a[:,i-3,:]+9*a[:,i-2,:]-45*a[:,i-1,:]+45*a[:,i+1,:] -9*a[:,i+2,:]+a[:,i+3,:])/(60*dx) #centered differences

		for i in range(s[1]-3,s[1]):
			aprime[:,i,:] = (-2*a[:,i-3,:]+9*a[:,i-2,:]-18*a[:,i-1,:]+11*a[:,i,:]) /(6*dx)     #backward differences near the second boundary

	#
	# derivative for dir=3
	#
	if dir == 3:
		for i in range(3):
			aprime[i,:,:] = (-11*a[i,:,:]+18*a[i+1,:,:]-9*a[i+2,:,:]+2*a[i+3,:,:]) /(6*dx)     #forward differences near the first boundary

		for i in range(3,s[0]-3):
			aprime[i,:,:] = (-a[i-3,:,:]+9*a[i-2,:,:]-45*a[i-1,:,:]+45*a[i+1,:,:] -9*a[i+2,:,:]+a[i+3,:,:])/(60*dx) #centered differences

		for i in range(s[0]-3,s[0]):
			aprime[i,:,:] = (-2*a[i-3,:,:]+9*a[i-2,:,:]-18*a[i-1,:,:]+11*a[i,:,:]) /(6*dx)     #backward differences near the second boundary

	return aprime



#
# NAME:
#       CURL_FINDIFF
#
# PURPOSE:
#       Computes the curl of a vector field using sixth
#       order finite differences in regular Cartesian grids
#
# CALLING SEQUENCE:
#       OUTX,OUTY,OUTZ = CURL_FINDIFF,INX,INY,INZ,DX,DY,DZ
#
# PARAMETERS:
#       INX[in]:   3D array with the x component of the field
#       INY[in]:   3D array with the y component of the field
#       INZ[in]:   3D array with the z component of the field
#       DX[in]:    spatial step of the grid in the x direction
#       DY[in]:    spatial step of the grid in the y direction.
#       DZ[in]:    spatial step of the grid in the z direction.
#
# RETURNS:
#       OUTX[out]: 3D array with the x component of the curl
#       OUTY[out]: 3D array with the y component of the curl
#       OUTZ[out]: 3D array with the z component of the curl
#
#
#
def curl_findiff(inx,iny,inz,dx,dy,dz):

	aux1 = deriv_findiff(inz,2,dy)       #x component of the curl
	aux2 = deriv_findiff(iny,3,dz)
	outx = aux1-aux2

	aux1 = deriv_findiff(inx,3,dz)       #y component of the curl
	aux2 = deriv_findiff(inz,1,dx)
	outy = aux1-aux2

	aux1 = deriv_findiff(iny,1,dx)       #z component of the curl
	aux2 = deriv_findiff(inx,2,dy)
	outz = aux1-aux2

	return outx, outy, outz

