#Python program to calculate Sea Level Pressure
#3D Input variables:  P, PB, T, QVAPOR, ELEVATION
#2D Output variable: WRF_SLP
#Copied (and adapted) from NCL fortran code wrf_user.f
#constants:
R=287.04
G=9.81
GAMMA=0.0065
TC=273.16+17.05
PCONST=10000
c = 2.0/7.0
#calculate TK:
TH = T+300.0
PR = P+PB
TK = (T+300.0)*power(PR*.00001,c)
#Find least z that is PCONST Pa above the surface
#Sweep array from bottom to top
s = shape(P)	#size of the input array
ss = [s[1],s[2]] # shape of 2d arrays
WRF_SLP = empty(ss,float32)
LEVEL = empty(ss,int32)
# Ridiculous MM5 test:
RIDTEST = empty(ss,int32)
PLO = empty(ss, float32)
ZLO = empty(ss,float32)
TLO = empty(ss,float32)
PHI = empty(ss,float32)
ZHI = empty(ss,float32)
THI = empty(ss,float32)
LEVEL[:,:] = -1
for K in range(s[0]):
	KHI = minimum(K+1, s[0]-1)
	LEVNEED = logical_and(less(LEVEL,0), less(PR[K,:,:] , PR[0,:,:] - PCONST))
	LEVEL[LEVNEED]=K
	PLO=where(LEVNEED,PR[K,:,:],PLO[:,:])
	TLO=where(LEVNEED,TK[K,:,:]*(1.+0.608*QVAPOR[K,:,:]), TLO[:,:])
	ZLO=where(LEVNEED,ELEVATION[K,:,:],ZLO[:,:])
	PHI=where(LEVNEED,PR[KHI,:,:],PHI[:,:])
	THI=where(LEVNEED,TK[KHI,:,:]*(1.+0.608*QVAPOR[KHI,:,:]), THI[:,:])
	ZHI=where(LEVNEED,ELEVATION[KHI,:,:],ZHI[:,:])

P_AT_PCONST = PR[0,:,:]-PCONST
T_AT_PCONST = THI - (THI-TLO)*log(P_AT_PCONST/PHI)*log(PLO/PHI)
Z_AT_PCONST = ZHI - (ZHI-ZLO)*log(P_AT_PCONST/PHI)*log(PLO/PHI)
T_SURF = T_AT_PCONST*power((PR[0,:,:]/P_AT_PCONST),(GAMMA*R/G))
T_SEA_LEVEL = T_AT_PCONST + GAMMA*Z_AT_PCONST
RIDTEST = logical_and(T_SURF <= TC, T_SEA_LEVEL >= TC)
T_SEA_LEVEL = where(RIDTEST, TC, TC - .005*(T_SURF -TC)**2)
Z_HALF_LOWEST=ELEVATION[0,:,:]
WRF_SLP = 0.01*(PR[0,:,:]*exp(2.*G*Z_HALF_LOWEST/(R*(T_SEA_LEVEL+T_SURF))))