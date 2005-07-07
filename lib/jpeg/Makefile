TOP = ../..

include ${TOP}/make/config/arch.mk

ifeq ($(ARCH),WIN32)
MAKEFILE_CFLAGS += /D"JPEG_EXPORTS" 
else
MAKEFILE_CFLAGS += -DJPEG_EXPORTS
endif

SHARED = 0
LIBRARY = vjpeg
FILES = \
	jcapimin   jcmarker jdapimin jdinput  jdtrans  jidctred \
	jcapistd   jcmaster jdapistd jdmainct jerror   jmemmgr \
	jccoefct   jcomapi  jdatadst jdmarker jfdctflt jmemnobs \
	jccolor    jcparam  jdatasrc jdmaster jfdctfst jquant1 \
	jcdctmgr   jcphuff  jdcoefct jdmerge  jfdctint jquant2 \
	jchuff     jcprepct jdcolor  jdphuff  jidctflt jutils \
	jcinit     jcsample jddctmgr jdpostct jidctfst transupp \
	jcmainct   jctrans  jdhuff   jdsample jidctint jfilewrite


include ${TOP}/make/config/base.mk

