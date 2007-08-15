//-- -----------------------------------------------------------
//   
// $Id$
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
// backface shader - stores depth in w component of color 
//----------------------------------------------------------------------------
static char fragment_shader_backface[] =
  "  "
  "  "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  gl_FragColor.xyz = gl_Color.xyz;\n"
  "  gl_FragColor.w = gl_FragCoord.z;\n"
  "}\n";

//----------------------------------------------------------------------------
// Isosurface with light
//----------------------------------------------------------------------------
static char fragment_shader_iso_lighting[] =
  "//const int maxiso = 8;\n"
  "uniform sampler2D backface_buffer;\n"
  "uniform sampler3D volumeTexture;\n"
  "//uniform float isovalues[maxiso];\n"
  "//uniform vec4 isocolors[maxiso];\n"
  "uniform float isovalues;\n"
  "uniform vec4 isocolors;\n"
  "//uniform int numiso;\n"
  "uniform float delta;\n"
  "uniform vec3 lightDirection;\n"
  "uniform float kd;\n"
  "uniform float ka;\n"
  "uniform float ks;\n"
  "uniform float expS;\n"
  "uniform vec3 dimensions;\n"
  " "
  "varying vec4 position;\n"
  "varying vec3 view;\n"
  " "
  "vec3 Gradient(in vec3, in vec3);\n"
  " "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  //if (isovalues >= 0.0) { \n"
  "  //  gl_FragColor = vec4(0.0, 1.0, 0.0,1.0);\n"
  "  //  return;\n"
  "  //}\n"
  "  vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
  "  vec2 tex_crd_2d = ((position.xy / position.w) + 1.0) / 2.0;\n"
  "  vec3 stop = texture2D(backface_buffer, tex_crd_2d).xyz;\n"
  "  vec3 start = gl_TexCoord[0].xyz;\n"
  "  vec3 dir = stop - start;\n"
  "  vec3 dir_unit = normalize(dir);\n"
  "  float len = length(dir);\n"
  "  int nsegs = int(len / delta);\n"
  "  float delta1 = len / float(nsegs);\n"
  "  vec3 delta_dir = dir_unit * delta1;\n"
  "  float zlen = texture2D(backface_buffer, tex_crd_2d).w - gl_FragCoord.z;\n"
  "  float zdelta = delta1 / len * zlen;\n"
  "  "
  "  vec3 texCoord0 = start.xyz;\n"
  "  vec3 texCoord1 = texCoord0+delta_dir;\n"
  "  float s0 = texture3D(volumeTexture, texCoord0).x;\n"
  "  float s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  "
  "  gl_FragDepth = gl_FragCoord.z;\n"
  "  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
  "  "
  "  "
  "  for (int i = 0; i<nsegs; i++) {\n"
  "    "
  "    // If sign changes we have an isosurface\n"
  "    "
  "    //for (int ii=0; ii<numiso; ii++) {\n"
  "      // If sign changes we have an isosurface\n"
  "      // if (((isovalues[ii]-s1) * (isovalues[ii]-s0)) < 0.0) {\n"
  "       if (((isovalues-s1) * (isovalues-s0)) < 0.0) {\n"
  "        "
  "        //vec4 color = isocolors[ii];\n"
  "        vec4 color = isocolors;\n"
  "        "
  "        // find texture coord of isovalue with linear interpolation\n"
  "        // vec3 isoTexCoord = texCoord0 + (((isovalues[ii]-s0) / (s1-s0)) * delta_dir);\n"
  "         vec3 isoTexCoord = texCoord0 + (((isovalues-s0) / (s1-s0)) * delta_dir);\n"
  "        vec3 grad_dd = 0.5 / dimensions;\n"
  "        vec3 gradient = gl_NormalMatrix * Gradient(grad_dd, isoTexCoord);\n"
  "        "
  "        vec3 ambient = ka * color.xyz;\n"
  "        vec3 diffuse;\n"
  "        vec3 specular;\n"
  "        "
  "        if (length(gradient) > 0.0) {\n"
  "          vec3 gradient_unit = normalize(gradient);\n"
  "          vec3 light      = normalize(lightDirection);\n"
  "          vec3 halfv      = normalize(light + gradient_unit);\n"
  "          "
  "          diffuse  = kd * abs(dot(light, gradient_unit)) * lightColor * color.xyz;\n"
  "          specular = ks * pow(abs(dot(halfv, normalize(view))), expS) * lightColor;\n"
  "          "
  "        }\n"
  "        else {\n"
  "          diffuse  = vec3(0.0);\n"
  "          specular = vec3(0.0);\n"
  "        }\n"
  "        "
  "        color.xyz = ambient + diffuse + specular;\n"
  "        // blend fragment\n"
  "        gl_FragColor = (color * vec4(color.a)) + (gl_FragColor * (vec4(1.0) - vec4(color.a)));\n"
  "        gl_FragDepth = gl_FragCoord.z + (float(i) * zdelta);\n"
  "      }\n"
  "     "
  "    //}\n"
  "    texCoord0 = texCoord1;\n"
  "    texCoord1 = texCoord1+delta_dir;\n"
  "    s0 = s1;\n"
  "    s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  }\n"
  "  if (gl_FragColor.a == 0.0) discard;\n"
  "}\n"
  ""
  "//-----------------------------------------------------------------------\n"
  "// Compute the gradient \n"
  "//-----------------------------------------------------------------------\n"
  "vec3 Gradient(in vec3 delta, in vec3 tc)\n"
  "{\n"
  "  vec3 gradient;\n"
  "  "
  "  "
  "  float dx = delta.x;\n"
  "  float dy = delta.y;\n"
  "  float dz = delta.z;\n"
  "  vec3 a0;\n"
  "  vec3 a1;\n"
  "  "
  "  a0.x = texture3D(volumeTexture, tc + vec3(dx,0,0)).x;\n"
  "  a1.x = texture3D(volumeTexture, tc - vec3(dx,0,0)).x;\n"
  "  a0.y = texture3D(volumeTexture, tc + vec3(0,dy,0)).x;\n"
  "  a1.y = texture3D(volumeTexture, tc - vec3(0,dy,0)).x;\n"
  "  a0.z = texture3D(volumeTexture, tc + vec3(0,0,dz)).x;\n"
  "  a1.z = texture3D(volumeTexture, tc - vec3(0,0,dz)).x;\n"
  "  "
  "  //gradient = (a1-a0)/(2.0*delta);\n"
  "  gradient = (a1-a0)/2.0;\n"
  "  "
  "  return gradient;\n"
  "}\n";

//----------------------------------------------------------------------------
// Standard vertex shader for the isosurface model
//----------------------------------------------------------------------------
static char vertex_shader_iso_lighting[] =
  "//-----------------------------------------------------------------------\n"
  "// Vertex shader main\n"
  "//-----------------------------------------------------------------------\n"
  "#version 110\n"
  "uniform mat4 glModelViewProjectionMatrix;\n"
  "varying vec3 view;\n"
  "varying vec4 position;\n"
  "void main(void)\n"
  "{\n"
  "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
  "  gl_Position    = ftransform();\n"
  "  position       = gl_Position;\n"
  "  view           = normalize(-gl_Position.xyz);\n"
  "}\n";




//----------------------------------------------------------------------------
// Isosurface without lighting
//----------------------------------------------------------------------------
static char fragment_shader_iso[] =
  "//const int maxiso = 8;\n"
  " "
  "uniform sampler2D backface_buffer;\n"
  "uniform sampler3D volumeTexture;\n"
  "//uniform float isovalues[maxiso];\n"
  "//uniform vec4 isocolors[maxiso];\n"
  "uniform float isovalues;\n"
  "uniform vec4 isocolors;\n"
  "//uniform int numiso;\n"
  "uniform float delta;\n"
  " "
  "varying vec4 position;\n"
  " "
  " "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  //if (isovalues >= 0.0) { \n"
  "  //  gl_FragColor = vec4(0.0, 0.0, 1.0,1.0);\n"
  "  //  return;\n"
  "  //}\n"
  "  vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
  "  vec2 tex_crd_2d = ((position.xy / position.w) + 1.0) / 2.0;\n"
  "  vec3 stop = texture2D(backface_buffer, tex_crd_2d).xyz;\n"
  "  vec3 start = gl_TexCoord[0].xyz;\n"
  "  vec3 dir = stop - start;\n"
  "  vec3 dir_unit = normalize(dir);\n"
  "  float len = length(dir);\n"
  "  int nsegs = int(len / delta);\n"
  "  float delta1 = len / float(nsegs);\n"
  "  vec3 delta_dir = dir_unit * delta1;\n"
  "  float zlen = texture2D(backface_buffer, tex_crd_2d).w - gl_FragCoord.z;\n"
  "  float zdelta = delta1 / len * zlen;\n"
  "  "
  "  vec3 texCoord0 = start.xyz;\n"
  "  vec3 texCoord1 = texCoord0+delta_dir;\n"
  "  float s0 = texture3D(volumeTexture, texCoord0).x;\n"
  "  float s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  "
  "  gl_FragDepth = gl_FragCoord.z;\n"
  "  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
  "  "
  "  "
  "  for (int i = 0; i<nsegs; i++) {\n"
  "    //for (int ii=0; ii<numiso; ii++) {\n"
  "      // If sign changes we have an isosurface\n"
  "      //if (((isovalues[ii]-s1) * (isovalues[ii]-s0)) < 0.0) {\n"
  "      if (((isovalues-s1) * (isovalues-s0)) < 0.0) {\n"
  "        "
  "        //vec4 color = isocolors[ii];\n"
  "        vec4 color = isocolors;\n"
  "        "
  "        // blend fragment\n"
  "        gl_FragColor = (color * vec4(color.a)) + (gl_FragColor * (vec4(1.0) - vec4(color.a)));\n"
  "        gl_FragDepth = gl_FragCoord.z + (float(i) * zdelta);\n"
  "        "
  "      }\n"
  "      "
  "    //}\n"
  "    texCoord0 = texCoord1;\n"
  "    texCoord1 = texCoord1+delta_dir;\n"
  "    s0 = s1;\n"
  "    s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  }\n"
  "  if (gl_FragColor.a == 0.0) discard;\n"
  "}\n";

//----------------------------------------------------------------------------
// Standard vertex shader for the isosurface model w/out lighting
//----------------------------------------------------------------------------
static char vertex_shader_iso[] =
  "//-----------------------------------------------------------------------\n"
  "// Vertex shader main\n"
  "//-----------------------------------------------------------------------\n"
  "#version 110\n"
  "uniform mat4 glModelViewProjectionMatrix;\n"
  "varying vec4 position;\n"
  "void main(void)\n"
  "{\n"
  "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
  "  gl_Position    = ftransform();\n"
  "  position       = gl_Position;\n"
  "}\n";

