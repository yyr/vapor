from numpy import *
import math
umod = cos(angleRAD)*u + sin(angleRAD)*v
vmod = -sin(angleRAD)*u + cos(angleRAD)*v
umod = umod/cos(latDEG*math.pi/180.)