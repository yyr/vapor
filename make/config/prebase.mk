SHARED = 1

include $(TOP)/make/config/arch.mk
include $(TOP)/make/config/$(ARCH).mk
include $(TOP)/options.mk


ifndef	QT_LIB_PATH
ifdef   QTDIR
QT_LIB_PATH = $(QTDIR)/lib
endif
endif

QTLIB = -lQtOpenGL -lQtGui -lQtCore -lQt3Support 

EXPATLIB = -lexpat
NETCDFLIB = -lnetcdf


