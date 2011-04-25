#Python program to calculate Sea Level Pressure
#Inputs:  P, PB, T, QVAPOR, ELEVATION
#Output: WRF_SLP
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
PR = P+PB
s = shape(P)	#size of the input array
ss = [s[1],s[2]] # shape of 2d arrays
#line 21
WRF_SLP = empty(ss,float32)
LEVEL = empty(ss,int32)
LEVEL[:,:] = -1
for K in range(s[0]):
	LEVNEED = logical_and(less(LEVEL,0), less(PR[K,:,:] , PR[0,:,:] - PCONST))
	LEVEL[LEVNEED] = K

# seem to need a loop for this:
for J in range(s[2]):
	for I in range(s[1]):
		KLO = maximum(LEVEL[I,J],0)
		KHI = minimum(KLO+1, s[0]-1)
		PLO = PR[KLO,I,J]
		PHI = PR[KHI,I,J]
		TLO = TK[KLO,I,J]*(1.+0.608*QVAPOR[KLO,I,J])
		THI = TK[KHI,I,J]*(1.+0.608*QVAPOR[KHI,I,J])
		ZLO = ELEVATION[KLO,I,J]
		ZHI = ELEVATION[KHI,I,J]
		P_AT_PCONST = PR[0,I,J]-PCONST
		T_AT_PCONST = THI - (THI-TLO)*log(P_AT_PCONST/PHI)*log(PLO/PHI)
		Z_AT_PCONST = ZHI - (ZHI-ZLO)*log(P_AT_PCONST/PHI)*log(PLO/PHI)
		T_SURF = T_AT_PCONST*power((PR[0,I,J]/P_AT_PCONST),(GAMMA*R/G))
		T_SEA_LEVEL = T_AT_PCONST + GAMMA*Z_AT_PCONST
#Ridiculous MM5 test:
		if(T_SURF <= TC and TC >= T_SEA_LEVEL):
			T_SEA_LEVEL = TC
		else:
			T_SEA_LEVEL = TC - .005*(T_SURF -TC)**2

		Z_HALF_LOWEST=ELEVATION[0,I,J]
		WRF_SLP[I,J] = 0.01*(PR[0,I,J]*exp(2.*G*Z_HALF_LOWEST/(R*(T_SEA_LEVEL+T_SURF))))