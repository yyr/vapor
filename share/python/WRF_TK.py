#Python program to calculate temperature in kelvin using WRF variables
#3d Inputs:  P, PB, T
#3d Output: WRF_TK
#Formula is (T+300)*((P+PB)*10**(-5))**c, 
#Where c is 287/(7*287*.5) = 2/7
c = 2.0/7.0
TH = T+300.0
WRF_TK = TH*numpy.power((P+PB)*.00001,c)