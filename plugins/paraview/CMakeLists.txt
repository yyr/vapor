PROJECT(VDFReader)

# Plugin that uses the Vapor libraries to read VDF files into ParaView

cmake_minimum_required(VERSION 2.8)
IF( COMMAND CMAKE_POLICY )
  CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )

IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

#find where all vapor header files live (especially vaporinternal/common.h)
FIND_PATH (VAPOR_HEADERS
  vaporinternal/common.h
  PATHS /ThirdPartyLibraries/VAPOR/source/vapor-1.5.2/vapor/include
  DOC "NCAR Vapor source directory, where include/vaporinternal/common.h can be found"
  )

#include the parts of vapor that we need to compile the reader
#both vapor/ and vaporinternal/ are underneath this
INCLUDE_DIRECTORIES(${VAPOR_HEADERS})

#link to the parts of vapor lib that we need to run/compile the reader
FIND_LIBRARY(VAPOR_VDF_LIB vdf DOC "NCAR Vapor Data Format library")
FIND_LIBRARY(VAPOR_COMMON_LIB common DOC "NCAR Vapor common libary")

#find expat and netcdf libs that vapor library itself uses
FIND_PACKAGE(EXPAT REQUIRED)
INCLUDE_DIRECTORIES(${EXPAT_INCLUDE_DIRS})
FIND_LIBRARY(VAPOR_NETCDF_LIB netcdf DOC "NetCDF library required for VAPOR plugin")

ADD_PARAVIEW_PLUGIN(VDFReaderPlugin "1.0"
  SERVER_MANAGER_XML VDFReader.xml
  SERVER_MANAGER_SOURCES vtkVDFReader.cxx
  GUI_RESOURCE_FILES VDFReaderGUI.xml)

TARGET_LINK_LIBRARIES(VDFReaderPlugin
  ${VAPOR_VDF_LIB} ${VAPOR_COMMON_LIB}
  ${EXPAT_LIBRARIES} ${VAPOR_NETCDF_LIB} )

