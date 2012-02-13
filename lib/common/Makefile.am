lib_LIBRARIES = libpiocommon.a 

#setup lib source files

libpiocommon_a_SOURCES = MyBase.cpp OptionParser.cpp EasyThreads.cpp \
		      CFuncs.cpp Base64.cpp Version.cpp PVTime.cpp

libpiocommon_a_CPPFLAGS = -DPARALLEL

if DEBUG

libpiocommon_a_CPPFLAGS += -DDEBUG

endif

if PNETCDF
libpiocommon_a_CPPFLAGS += -DPNETCDF
endif

if NETCDF
libpiocommon_a_CPPFLAGS += -DNETCDF
endif 

all: libpiocommon.a