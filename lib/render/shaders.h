//--DVRShader.cpp -----------------------------------------------------------
//   
//                   Copyright (C) 2006
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
// GLSL shader code. 
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Standard fragment shader
//----------------------------------------------------------------------------
static char fragment_shader_default[] =
  "uniform sampler1D colormap;\n"
  "uniform sampler3D volumeTexture;\n"
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "vec4 intensity = vec4 (texture3D(volumeTexture, gl_TexCoord[0].xyz));\n"
  "vec4 color     = vec4 (texture1D(colormap, intensity.x));\n"
  "gl_FragColor = color;\n"
  "}\n";

//----------------------------------------------------------------------------
// Standard vertex shader for the lighting model 
//----------------------------------------------------------------------------
static char vertex_shader_lighting[] =
  "//-----------------------------------------------------------------------\n"
  "// Vertex shader main\n"
  "//-----------------------------------------------------------------------\n"
  "uniform vec3 lightPosition;\n"
  "varying vec3 position;\n"
  "varying vec3 light;\n"
  "varying vec3 view;\n"
  "void main(void)\n"
  "{\n"
  "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
  "  gl_Position    = ftransform();\n"
  "  position       = gl_Position.xyz;\n"
  "  light          = normalize(lightPosition - position);\n"
  "  view           = normalize(-position);\n"
  "}\n";

//----------------------------------------------------------------------------
// Standard fragment shader w/ lighting model (on-the-fly gradient calculation)
//----------------------------------------------------------------------------
static char fragment_shader_lighting[] =
  "uniform sampler1D colormap;\n"
  "uniform sampler3D volumeTexture;\n"
  "uniform vec3 dimensions;\n"
  "uniform float kd;\n"
  "uniform float ka;\n"
  "uniform float ks;\n"
  "uniform float expS;\n"
  "varying vec3 position;\n"
  "varying vec3 light;\n"
  "varying vec3 view;\n"
  ""
  "vec3 alphaGradient();\n"
  ""
  "//-----------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//-----------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  vec4 intensity = vec4 (texture3D(volumeTexture, gl_TexCoord[0].xyz));\n"
  "  vec4 color     = vec4 (texture1D(colormap, intensity.x));\n"
  "  \n"
  "  "
  "  vec3 gradient = alphaGradient();\n"
  "  "
  "  if (length(gradient) == 0.0)\n"
  "  {\n"
  "    gradient = vec3 (0,0,1);\n"
  "  }\n"
  "  else\n"
  "  {\n"
  "    gradient = normalize(gl_NormalMatrix * gradient);\n"
  "  }\n"
  "  "
  "  vec3 reflection = reflect(-light, gradient);\n"
  "  "
  "  float diffuse  = kd * abs(dot(light,gradient));\n"
  "  float specular = ks * pow(abs(dot(reflection, view)), expS);\n"
  "  "
  "  gl_FragColor = color * (diffuse + specular + ka);\n"
  "}\n"
  "" 
  "//-----------------------------------------------------------------------\n"
  "// Compute the gradient using the alpha channel\n"
  "//-----------------------------------------------------------------------\n"
  "vec3 alphaGradient()\n"
  "{\n"
  "  vec3 gradient;\n"
  "  "
  "  float dx = 0.5/(dimensions.x);\n"
  "  float dy = 0.5/(dimensions.y);\n"
  "  float dz = 0.5/(dimensions.z);\n"
  "  "
  "  float x0 = texture1D(colormap,\n" 
  "                       texture3D(volumeTexture,\n" 
  "                                gl_TexCoord[0].xyz + vec3(dx,0,0)).x).a;\n"
  "  float x1 = texture1D(colormap,\n" 
  "                       texture3D(volumeTexture,\n" 
  "                                gl_TexCoord[0].xyz + vec3(-dx,0,0)).x).a;\n"
  "  "
  "  float y0 = texture1D(colormap,\n" 
  "                       texture3D(volumeTexture,\n" 
  "                               gl_TexCoord[0].xyz + vec3(0,dy,0)).x).a;\n"
  "  float y1 = texture1D(colormap,\n" 
  "                       texture3D(volumeTexture," 
  "                                gl_TexCoord[0].xyz + vec3(0,-dy,0)).x).a;\n"
  "  "
  "  float z0 = texture1D(colormap,\n" 
  "                       texture3D(volumeTexture,\n" 
  "                                 gl_TexCoord[0].xyz + vec3(0,0,dz)).x).a;\n"
  "  float z1 = texture1D(colormap,\n" 
  "                       texture3D(volumeTexture,\n" 
  "                                gl_TexCoord[0].xyz + vec3(0,0,-dz)).x).a;\n"
  "  "
  "  gradient.x = (x1-x0)/2.0;\n"
  "  gradient.y = (y1-y0)/2.0;\n"
  "  gradient.z = (z1-z0)/2.0;\n"
  "  "
  "  return gradient;\n"
  "}\n";
