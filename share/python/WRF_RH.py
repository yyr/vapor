#Python program to Relative humidity using WRF variables
#Inputs:  P, PB, T, QVAPOR
#Output: WRF_RH
#Formula is from wrf_user.f
c = 2.0/7.0
SVP1 = 0.6112
SVP2 = 17.67
SVPT0 = 273.15
SVP3 = 29.65
EP_3 = 0.622
TH = T+300.0
PRESS = P+PB
TK = TH*numpy.power(PRESS*.00001,c)
ES = 10*SVP1*numpy.exp(SVP2*(TK-SVPT0)/(TK-SVP3))
QVS = EP_3*ES/(0.01*PRESS - (1.-EP_3)*ES)
WRF_RH = 100.0*numpy.maximum(numpy.minimum(QVAPOR/QVS,1.0),0)