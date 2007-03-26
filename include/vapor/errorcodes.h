//-- errorcodes.h ------------------------------------------------------------
//   
//                   Copyright (C)  2004
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           errorcodes.h
//
//      Author:         Kenny Gruchalla
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2006
//
//      Description:    VAPoR error codes used for message reporting. 
//
//                      The high-order bits of the codes are bit masked 
//                      providing type information: diagonstic, warning, error,
//                      and fatal.
//
//                      For example, if (code & VAPoR::WARNING) then 'code'
//                      is a warning type. 
//
//----------------------------------------------------------------------------

namespace VAPoR
{
  enum errorcodes
  {
    // Diagnostic Codes  0x1001 - 0x1FFF
	//Currently, diagnostic codes are not used
    VAPOR_DIAGNOSTIC = 0x1000,
    
    // Warning Codes     0x2001 - 0x2FFF
    VAPOR_WARNING               = 0x2000,
    VAPOR_WARNING_GL_SHADER_LOG = 0x2001,
	VAPOR_WARNING_FLOW			= 0x2100,
	VAPOR_WARNING_SEEDS			= 0x2101,
	VAPOR_WARNING_DATA_UNAVAILABLE	= 0x2003,
    
    // Error Codes       0x4001 - 0x4FFF
    VAPOR_ERROR                    = 0x4000,
    VAPOR_ERROR_GL_RENDERING       = 0x4001,
    VAPOR_ERROR_GL_UNKNOWN_UNIFORM = 0x4002,
	VAPOR_ERROR_DATA_UNAVAILABLE   = 0x4003,
	VAPOR_ERROR_DATA_TOO_BIG	= 0x4004,

	//Flow errors
	VAPOR_ERROR_FLOW = 0x4100,
	VAPOR_ERROR_SEEDS = 0x4101,
	VAPOR_ERROR_INTEGRATION = 0x4102,
	VAPOR_ERROR_FLOW_DATA = 0x4103,
    
    // Fatal Codes       0x8001 - 0x8FFF
    VAPOR_FATAL = 0x8000
  };   
};
