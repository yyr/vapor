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
// This code provides shaders that implement a ray-caster for sampled
// data (3D texture volumes). The algorithm employs a two-pass 
// renderer. The first pass calculates the texture coordinates
// and depth of the back facing quads of a rectilinear, axis-aligned box 
// bounding the scene. The fixed function pipeline execute the first
// rendering pass. The second pass provides the depth and texture 
// coordinaetes of the front faces of the box. From this information the
// shaders traverse rays between the front and back faces. The algorithm 
// is based on the paper: 
// Acceleration Techniques for GPU-based Volume Rendering, by Kruger
// and Westermann
//
//----------------------------------------------------------------------------

#ifdef	DEAD
//----------------------------------------------------------------------------
// backface shader - stores depth in w component of color 
//----------------------------------------------------------------------------
static char fragment_shader_backface[] =
  "#version 120\n"
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  gl_FragColor.xyz = gl_Color.xyz;\n"
  "  gl_FragColor.w = gl_FragCoord.z;\n"
  "}\n";
#endif

//----------------------------------------------------------------------------
// Isosurface with light
//----------------------------------------------------------------------------
static char fragment_shader_iso_lighting[] =
  "\n"
  "\n"
  "#version 120\n"
  "uniform sampler2D texcrd_buffer;	// tex coords for back facing quads\n"
  "uniform sampler2D depth_buffer;	// depth back facing quads\n"
  "uniform sampler3D volumeTexture;	// sampled data to ray cast\n"
  "uniform float isovalue;\n"
  "uniform vec4 isocolor;	// base color for isosurface\n"
  " "
  "uniform float delta;	// sampling distance along ray (eye coordinates)\n"
  "uniform vec2 winsize;	// Window width and height \n"
  " "
  "// Lighting parameters\n"
  " "
  "uniform vec3 lightDirection;\n"
  "uniform float kd;\n"
  "uniform float ka;\n"
  "uniform float ks;\n"
  "uniform float expS;\n"
  " "
  "uniform vec3 dimensions;	// ijk dimensions of 3D texture\n"
  " "
  "varying vec4 position;	// interpolated gl_Position\n"
  "varying vec3 view;	// normalized, negative position (view vector)\n"
  " "
  "vec3 Gradient(in vec3, in vec3);\n"
  " "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
  "  "
  "  // Normalized window coordinates of this frament\n"
  "  vec2 texCoord2D = ((position.xy / position.w) + 1.0) / 2.0;\n"
  "  "
  "  // Starting and finishing texture coordinate of ray\n"
  "  vec3 texStop = texture2D(texcrd_buffer, texCoord2D).xyz;\n"
  "  vec3 texStart = gl_TexCoord[0].xyz;\n"
  "  "
  "  // Ray direction, texture coordinates\n"
  "  vec3 texDir = texStop - texStart;\n"
  "  vec3 texDirUnit = normalize(texDir);\n"
  "  //float len = length(texDir);\n"
  "  "
  "  float n = gl_DepthRange.near;\n"
  "  float f = gl_DepthRange.far;\n"
  "  "
  "  // Starting and stopping Z (window, NDC and eye coordinates)\n"
  "  // We need eye coordinates to interpolate depth (Z) because\n"
  "  // NDC and Window space is non-linear\n"
  "  // N.B. Inverse transform of window coords only works for perspective\n"
  "  "
  "  float zStopWin = texture2D(depth_buffer, texCoord2D).x;\n"
  "  float zStopNDC = (2*zStopWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStopEye = -gl_ProjectionMatrix[3].z / (zStopNDC + gl_ProjectionMatrix[2].z);\n"
  "  float zStartWin = gl_FragCoord.z;\n"
  "  float zStartNDC = (2*zStartWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStartEye = -gl_ProjectionMatrix[3].z / (zStartNDC + gl_ProjectionMatrix[2].z);\n"
  "  vec2 posNDC = gl_FragCoord.xy * 2.0 / winsize - 1.0;\n"
  "  "
  "  // Compute number of samples based on samplin distance, delta \n"
  "  //\n"
  "  int nsegs = int(min((zStopEye-zStartEye / delta), 256.0));\n"
  "  "
  "  // Ugh. Hard-wire number of samples. Bad things happen when view point \n"
  "  // is inside the view volume using above code \n"
  "  if (nsegs>0) nsegs = 256;\n"
  "  vec3 deltaVec = texDirUnit * (length(texDir) / float(nsegs));\n"
  "  float deltaZ = (zStopEye-zStartEye) / float(nsegs);\n"
  "  "
  "  // texCoord{0,1} are the end points of a ray segment in texture coords\n"
  "  // s{0,1} are the sampled values of the texture at ray segment endpoints\n"
  "  vec3 texCoord0 = texStart;\n"
  "  vec3 texCoord1 = texCoord0+deltaVec;\n"
  "  float s0 = texture3D(volumeTexture, texCoord0).x;\n"
  "  float s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  "
  "  // Current Z value along ray and current (accumulated) color \n"
  "  float fragDepth = zStartEye;\n"
  "  vec4 fragColor =  vec4(0.0, 0.0, 0.0, 0.0);\n"
  "  "
  "  // Make sure gl_FragDepth is set for all execution paths\n"
  "  gl_FragDepth = zStopWin;\n"
  "  "
  "  // Composite from front to back\n"
  "  "
  "  // false after first isosurface interesected \n"
  "  bool first = true;\n"
  "  for (int i = 0; i<nsegs; i++) {\n"
  "    "
  "    // If sign changes we have an isosurface\n"
  "    "
  "       if (((isovalue-s1) * (isovalue-s0)) < 0.0) {\n"
  "        "
  "        float weight = (isovalue-s0) / (s1-s0);\n"
  "        vec4 color = isocolor;\n"
  "        "
  "        // find precise texture coord of isovalue with linear \n"
  "        // interpolation current segment end points \n"
  "        vec3 isoTexCoord = texCoord0 + (weight * deltaVec);\n"
  "        "
  "        // compute surface gradient at ray's intersection with isosurface\n"
  "        vec3 grad_dd = 0.5 / dimensions;\n"
  "        vec3 gradient = gl_NormalMatrix * Gradient(grad_dd, isoTexCoord);\n"
  "        "
  "        float diffuse = 0.0;\n"
  "        float specular = 0.0;\n"
  "        "
  "        if (length(gradient) > 0.0) {\n"
  " "
  "          gradient = normalize(gradient);\n"
  "          // Ugh. Need to convert ray intersection point to eye coords \n"
  "          vec4 posClip = vec4( \n"
  "            posNDC, \n"
  "           -gl_ProjectionMatrix[3].z / fragDepth - gl_ProjectionMatrix[2].z,"
  "           1.0);\n"
  "          vec4 eyePos = gl_ProjectionMatrixInverse * posClip;\n"
  "          eyePos /= eyePos.w;\n"
  "          "
  "          //use Phong illumination if non-zero gradient\n"
  "          "
  "          vec3 lightVec      = normalize(lightDirection);\n"
  "          vec3 halfv      = reflect(-lightVec, gradient);\n"
  "          vec3 viewVec      = normalize(-eyePos.xyz);\n"
  "          "
  "          diffuse  = abs(dot(lightVec, gradient));\n"
  "          if (diffuse > 0.0) {\n"
  "            specular = pow(abs(dot(halfv, normalize(viewVec))), expS);\n"
  "          }\n"
  "          "
  "        }\n"
  "        diffuse = kd * diffuse;\n"
  "        specular = ks * specular;\n"
  "        "
  "        color.xyz = color.xyz * (ka+diffuse) + vec3(specular*lightColor);\n"
  "        "
  "        // blend fragment color with front to back compositing operator \n"
  "        fragColor = (vec4(1.0)- vec4(fragColor.a))*color + fragColor;\n"
  " "
  "        // The depth buffer value will be the first ray-isosurface \n"
  "        // intersection. N.B. may not be best choice \n"
  " "
  "        if (first) {\n"
  "          fragDepth = zStartEye + (float(i) * deltaZ) + (deltaZ*weight);\n"
  "          first = false;\n"
  "        }\n"
  "      }\n"
  "     "
  "    texCoord0 = texCoord1;\n"
  "    texCoord1 = texCoord1+deltaVec;\n"
  "    s0 = s1;\n"
  "    s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  }\n"
  "  "
  "  if (fragColor.a == 0.0) discard;\n"
  "  "
  "  // Convert depth from eye coordinates back to window coords \n"
  "  "
  "  fragDepth = (fragDepth * gl_ProjectionMatrix[2].z + gl_ProjectionMatrix[3].z) / -fragDepth;\n" 
  "  gl_FragDepth = fragDepth * ((f-n)/2) + (n+f)/2;\n"
  "  gl_FragColor = fragColor;\n"
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
  "#version 120\n"
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
  "#version 120\n"
  "uniform sampler2D texcrd_buffer;	// tex coords for back facing quads\n"
  "uniform sampler2D depth_buffer;	// depth back facing quads\n"
  "uniform sampler3D volumeTexture;	// sampled data to ray cast\n"
  "uniform float isovalue;\n"
  "uniform vec4 isocolor;	// base color for isosurface\n"
  " "
  "uniform float delta;	// sampling distance along ray (eye coordinates)\n"
  " "
  "varying vec4 position;	// interpolated gl_Position\n"
  " "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
  "  "
  "  // Normalized window coordinates of this frament\n"
  "  vec2 texCoord2D = ((position.xy / position.w) + 1.0) / 2.0;\n"
  "  "
  "  // Starting and finishing texture coordinate of ray\n"
  "  vec3 texStop = texture2D(texcrd_buffer, texCoord2D).xyz;\n"
  "  vec3 texStart = gl_TexCoord[0].xyz;\n"
  "  "
  "  // Ray direction, texture coordinates\n"
  "  vec3 texDir = texStop - texStart;\n"
  "  vec3 texDirUnit = normalize(texDir);\n"
  "  //float len = length(texDir);\n"
  "  "
  "  float n = gl_DepthRange.near;\n"
  "  float f = gl_DepthRange.far;\n"
  "  "
  "  // Starting and stopping Z (window, NDC and eye coordinates)\n"
  "  // We need eye coordinates to interpolate depth (Z) because\n"
  "  // NDC and Window space is non-linear\n"
  "  // N.B. Inverse transform of window coords only works for perspective\n"
  "  "
  "  float zStopWin = texture2D(depth_buffer, texCoord2D).x;\n"
  "  float zStopNDC = (2*zStopWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStopEye = -gl_ProjectionMatrix[3].z / (zStopNDC + gl_ProjectionMatrix[2].z);\n"
  "  float zStartWin = gl_FragCoord.z;\n"
  "  float zStartNDC = (2*zStartWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStartEye = -gl_ProjectionMatrix[3].z / (zStartNDC + gl_ProjectionMatrix[2].z);\n"
  "  "
  "  // Compute number of samples based on samplin distance, delta \n"
  "  //\n"
  "  int nsegs = int(min((zStopEye-zStartEye / delta), 256.0));\n"
  "  "
  "  // Ugh. Hard-wire number of samples. Bad things happen when view point \n"
  "  // is inside the view volume using above code \n"
  "  if (nsegs>0) nsegs = 256;\n"
  "  vec3 deltaVec = texDirUnit * (length(texDir) / float(nsegs));\n"
  "  float deltaZ = (zStopEye-zStartEye) / float(nsegs);\n"
  "  "
  "  // texCoord{0,1} are the end points of a ray segment in texture coords\n"
  "  // s{0,1} are the sampled values of the texture at ray segment endpoints\n"
  "  vec3 texCoord0 = texStart;\n"
  "  vec3 texCoord1 = texCoord0+deltaVec;\n"
  "  float s0 = texture3D(volumeTexture, texCoord0).x;\n"
  "  float s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  "
  "  // Current Z value along ray and current (accumulated) color \n"
  "  float fragDepth = zStartEye;\n"
  "  vec4 fragColor =  vec4(0.0, 0.0, 0.0, 0.0);\n"
  "  "
  "  // Make sure gl_FragDepth is set for all execution paths\n"
  "  gl_FragDepth = zStopWin;\n"
  "  "
  "  // Composite from front to back\n"
  "  "
  "  // false after first isosurface interesected \n"
  "  bool first = true;\n"
  "  for (int i = 0; i<nsegs; i++) {\n"
  "    "
  "    // If sign changes we have an isosurface\n"
  "    "
  "       if (((isovalue-s1) * (isovalue-s0)) < 0.0) {\n"
  "        "
  "        float weight = (isovalue-s0) / (s1-s0);\n"
  "        vec4 color = isocolor;\n"
  "        "
  "        // blend fragment color with front to back compositing operator \n"
  "        fragColor = (vec4(1.0)- vec4(fragColor.a))*color + fragColor;\n"
  " "
  "        // The depth buffer value will be the first ray-isosurface \n"
  "        // intersection. N.B. may not be best choice \n"
  " "
  "        if (first) {\n"
  "          fragDepth = zStartEye + (float(i) * deltaZ) + (deltaZ*weight);\n"
  "          first = false;\n"
  "        }\n"
  "      }\n"
  "     "
  "    texCoord0 = texCoord1;\n"
  "    texCoord1 = texCoord1+deltaVec;\n"
  "    s0 = s1;\n"
  "    s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  }\n"
  "  "
  "  if (fragColor.a == 0.0) discard;\n"
  "  "
  "  // Convert depth from eye coordinates back to window coords \n"
  "  "
  "  fragDepth = (fragDepth * gl_ProjectionMatrix[2].z + gl_ProjectionMatrix[3].z) / -fragDepth;\n" 
  "  gl_FragDepth = fragDepth * ((f-n)/2) + (n+f)/2;\n"
  "  gl_FragColor = fragColor;\n"
  "}\n";



//----------------------------------------------------------------------------
// Standard vertex shader for the isosurface model w/out lighting
//----------------------------------------------------------------------------
static char vertex_shader_iso[] =
  "//-----------------------------------------------------------------------\n"
  "// Vertex shader main\n"
  "//-----------------------------------------------------------------------\n"
  "#version 120\n"
  "uniform mat4 glModelViewProjectionMatrix;\n"
  "varying vec4 position;\n"
  "void main(void)\n"
  "{\n"
  "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
  "  gl_Position    = ftransform();\n"
  "  position       = gl_Position;\n"
  "}\n";


















//----------------------------------------------------------------------------
// Colored Isosurface with light
//----------------------------------------------------------------------------
static char fragment_shader_iso_color_lighting[] =
  "#version 120\n"
  "uniform sampler1D colormap;\n"
  "uniform sampler2D texcrd_buffer;	// tex coords for back facing quads\n"
  "uniform sampler2D depth_buffer;	// depth back facing quads\n"
  "uniform sampler3D volumeTexture;	// sampled data to ray cast\n"
  "uniform float isovalue;\n"
  " "
  "uniform float delta;	// sampling distance along ray (eye coordinates)\n"
  "uniform vec2 winsize;	// Window width and height \n"
  " "
  "// Lighting parameters\n"
  " "
  "uniform vec3 lightDirection;\n"
  "uniform float kd;\n"
  "uniform float ka;\n"
  "uniform float ks;\n"
  "uniform float expS;\n"
  " "
  "uniform vec3 dimensions;	// ijk dimensions of 3D texture\n"
  " "
  "varying vec4 position;	// interpolated gl_Position\n"
  "varying vec3 view;	// normalized, negative position (view vector)\n"
  " "
  "vec3 Gradient(in vec3, in vec3);\n"
  " "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
  "  "
  "  // Normalized window coordinates of this frament\n"
  "  vec2 texCoord2D = ((position.xy / position.w) + 1.0) / 2.0;\n"
  "  "
  "  // Starting and finishing texture coordinate of ray\n"
  "  vec3 texStop = texture2D(texcrd_buffer, texCoord2D).xyz;\n"
  "  vec3 texStart = gl_TexCoord[0].xyz;\n"
  "  "
  "  // Ray direction, texture coordinates\n"
  "  vec3 texDir = texStop - texStart;\n"
  "  vec3 texDirUnit = normalize(texDir);\n"
  "  //float len = length(texDir);\n"
  "  "
  "  float n = gl_DepthRange.near;\n"
  "  float f = gl_DepthRange.far;\n"
  "  "
  "  // Starting and stopping Z (window, NDC and eye coordinates)\n"
  "  // We need eye coordinates to interpolate depth (Z) because\n"
  "  // NDC and Window space is non-linear\n"
  "  // N.B. Inverse transform of window coords only works for perspective\n"
  "  "
  "  float zStopWin = texture2D(depth_buffer, texCoord2D).x;\n"
  "  float zStopNDC = (2*zStopWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStopEye = -gl_ProjectionMatrix[3].z / (zStopNDC + gl_ProjectionMatrix[2].z);\n"
  "  float zStartWin = gl_FragCoord.z;\n"
  "  float zStartNDC = (2*zStartWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStartEye = -gl_ProjectionMatrix[3].z / (zStartNDC + gl_ProjectionMatrix[2].z);\n"
  "  "
  "  vec2 posNDC = gl_FragCoord.xy * 2.0 / winsize - 1.0;\n"
  "  // Compute number of samples based on samplin distance, delta \n"
  "  //\n"
  "  int nsegs = int(min((zStopEye-zStartEye / delta), 256.0));\n"
  "  "
  "  // Ugh. Hard-wire number of samples. Bad things happen when view point \n"
  "  // is inside the view volume using above code \n"
  "  if (nsegs>0) nsegs = 256;\n"
  "  vec3 deltaVec = texDirUnit * (length(texDir) / float(nsegs));\n"
  "  float deltaZ = (zStopEye-zStartEye) / float(nsegs);\n"
  "  "
  "  // texCoord{0,1} are the end points of a ray segment in texture coords\n"
  "  // s{0,1} are the sampled values of the texture at ray segment endpoints\n"
  "  vec3 texCoord0 = texStart;\n"
  "  vec3 texCoord1 = texCoord0+deltaVec;\n"
  "  float s0 = texture3D(volumeTexture, texCoord0).x;\n"
  "  float s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  "
  "  // Current Z value along ray and current (accumulated) color \n"
  "  float fragDepth = zStartEye;\n"
  "  vec4 fragColor =  vec4(0.0, 0.0, 0.0, 0.0);\n"
  "  "
  "  // Make sure gl_FragDepth is set for all execution paths\n"
  "  gl_FragDepth = zStopWin;\n"
  "  "
  "  // Composite from front to back\n"
  "  "
  "  // false after first isosurface interesected \n"
  "  bool first = true;\n"
  "  for (int i = 0; i<nsegs; i++) {\n"
  "    "
  "    // If sign changes we have an isosurface\n"
  "    "
  "       if (((isovalue-s1) * (isovalue-s0)) < 0.0) {\n"
  "        "
  "        float weight = (isovalue-s0) / (s1-s0);\n"
  "        "
  "        // find precise texture coord of isovalue with linear \n"
  "        // interpolation current segment end points \n"
  "        vec3 isoTexCoord = texCoord0 + (weight * deltaVec);\n"
  "        "
  "        float var2 = texture3D(volumeTexture,isoTexCoord).w;\n"
  "        vec4 color = vec4(texture1D(colormap, var2));\n"
  "        "
  "        // compute surface gradient at ray's intersection with isosurface\n"
  "        vec3 grad_dd = 0.5 / dimensions;\n"
  "        vec3 gradient = gl_NormalMatrix * Gradient(grad_dd, isoTexCoord);\n"
  "        "
  "        float diffuse = 0.0;\n"
  "        float specular = 0.0;\n"
  "        "
  "        if (length(gradient) > 0.0) {\n"
  " "
  "          gradient = normalize(gradient);\n"
  "          // Ugh. Need to convert ray intersection point to eye coords \n"
  "          vec4 posClip = vec4( \n"
  "            posNDC, \n"
  "           -gl_ProjectionMatrix[3].z / fragDepth - gl_ProjectionMatrix[2].z,"
  "           1.0);\n"
  "          vec4 eyePos = gl_ProjectionMatrixInverse * posClip;\n"
  "          eyePos /= eyePos.w;\n"
  "          "
  "          //use Phong illumination if non-zero gradient\n"
  "          "
  "          vec3 lightVec      = normalize(lightDirection);\n"
  "          vec3 halfv      = reflect(-lightVec, gradient);\n"
  "          vec3 viewVec      = normalize(-eyePos.xyz);\n"
  "          "
  "          diffuse  = abs(dot(lightVec, gradient));\n"
  "          if (diffuse > 0.0) {\n"
  "            specular = pow(abs(dot(halfv, normalize(viewVec))), expS);\n"
  "          }\n"
  "          "
  "        }\n"
  "        "
  "        diffuse = kd * diffuse;\n"
  "        specular = ks * specular;\n"
  "        "
  "        color.xyz = color.xyz * (ka+diffuse) + vec3(specular*lightColor);\n"
  "        "
  "        // blend fragment color with front to back compositing operator \n"
  "        fragColor = (vec4(1.0)- vec4(fragColor.a))*color + fragColor;\n"
  " "
  "        // The depth buffer value will be the first ray-isosurface \n"
  "        // intersection. N.B. may not be best choice \n"
  " "
  "        if (first) {\n"
  "          fragDepth = zStartEye + (float(i) * deltaZ) + (deltaZ*weight);\n"
  "          first = false;\n"
  "        }\n"
  "      }\n"
  "     "
  "    texCoord0 = texCoord1;\n"
  "    texCoord1 = texCoord1+deltaVec;\n"
  "    s0 = s1;\n"
  "    s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  }\n"
  "  "
  "  if (fragColor.a == 0.0) discard;\n"
  "  "
  "  // Convert depth from eye coordinates back to window coords \n"
  "  "
  "  fragDepth = (fragDepth * gl_ProjectionMatrix[2].z + gl_ProjectionMatrix[3].z) / -fragDepth;\n" 
  "  gl_FragDepth = fragDepth * ((f-n)/2) + (n+f)/2;\n"
  "  gl_FragColor = fragColor;\n"
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
static char vertex_shader_iso_color_lighting[] =
  "//-----------------------------------------------------------------------\n"
  "// Vertex shader main\n"
  "//-----------------------------------------------------------------------\n"
  "#version 120\n"
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
static char fragment_shader_iso_color[] =
  "#version 120\n"
  "uniform sampler1D colormap;\n"
  "uniform sampler2D texcrd_buffer;	// tex coords for back facing quads\n"
  "uniform sampler2D depth_buffer;	// depth back facing quads\n"
  "uniform sampler3D volumeTexture;	// sampled data to ray cast\n"
  "uniform float isovalue;\n"
  " "
  "uniform float delta;	// sampling distance along ray (eye coordinates)\n"
  " "
  "varying vec4 position;	// interpolated gl_Position\n"
  " "
  "//------------------------------------------------------------------\n"
  "// Fragment shader main\n"
  "//------------------------------------------------------------------\n"
  "void main(void)\n"
  "{\n"
  "  "
  "  vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
  "  "
  "  // Normalized window coordinates of this frament\n"
  "  vec2 texCoord2D = ((position.xy / position.w) + 1.0) / 2.0;\n"
  "  "
  "  // Starting and finishing texture coordinate of ray\n"
  "  vec3 texStop = texture2D(texcrd_buffer, texCoord2D).xyz;\n"
  "  vec3 texStart = gl_TexCoord[0].xyz;\n"
  "  "
  "  // Ray direction, texture coordinates\n"
  "  vec3 texDir = texStop - texStart;\n"
  "  vec3 texDirUnit = normalize(texDir);\n"
  "  //float len = length(texDir);\n"
  "  "
  "  float n = gl_DepthRange.near;\n"
  "  float f = gl_DepthRange.far;\n"
  "  "
  "  // Starting and stopping Z (window, NDC and eye coordinates)\n"
  "  // We need eye coordinates to interpolate depth (Z) because\n"
  "  // NDC and Window space is non-linear\n"
  "  // N.B. Inverse transform of window coords only works for perspective\n"
  "  "
  "  float zStopWin = texture2D(depth_buffer, texCoord2D).x;\n"
  "  float zStopNDC = (2*zStopWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStopEye = -gl_ProjectionMatrix[3].z / (zStopNDC + gl_ProjectionMatrix[2].z);\n"
  "  float zStartWin = gl_FragCoord.z;\n"
  "  float zStartNDC = (2*zStartWin)/(f-n) - (n+f)/(f-n);\n"
  "  float zStartEye = -gl_ProjectionMatrix[3].z / (zStartNDC + gl_ProjectionMatrix[2].z);\n"
  "  "
  "  // Compute number of samples based on samplin distance, delta \n"
  "  //\n"
  "  int nsegs = int(min((zStopEye-zStartEye / delta), 256.0));\n"
  "  "
  "  // Ugh. Hard-wire number of samples. Bad things happen when view point \n"
  "  // is inside the view volume using above code \n"
  "  if (nsegs>0) nsegs = 256;\n"
  "  vec3 deltaVec = texDirUnit * (length(texDir) / float(nsegs));\n"
  "  float deltaZ = (zStopEye-zStartEye) / float(nsegs);\n"
  "  "
  "  // texCoord{0,1} are the end points of a ray segment in texture coords\n"
  "  // s{0,1} are the sampled values of the texture at ray segment endpoints\n"
  "  vec3 texCoord0 = texStart;\n"
  "  vec3 texCoord1 = texCoord0+deltaVec;\n"
  "  float s0 = texture3D(volumeTexture, texCoord0).x;\n"
  "  float s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  "
  "  // Current Z value along ray and current (accumulated) color \n"
  "  float fragDepth = zStartEye;\n"
  "  vec4 fragColor =  vec4(0.0, 0.0, 0.0, 0.0);\n"
  "  "
  "  // Make sure gl_FragDepth is set for all execution paths\n"
  "  gl_FragDepth = zStopWin;\n"
  "  "
  "  // Composite from front to back\n"
  "  "
  "  // false after first isosurface interesected \n"
  "  bool first = true;\n"
  "  for (int i = 0; i<nsegs; i++) {\n"
  "    "
  "    // If sign changes we have an isosurface\n"
  "    "
  "       if (((isovalue-s1) * (isovalue-s0)) < 0.0) {\n"
  "        "
  "        float weight = (isovalue-s0) / (s1-s0);\n"
  "        "
  "        // find texture coord of isovalue with linear interpolation\n"
  "        vec3 isoTexCoord = texCoord0 + (weight * deltaVec);\n"
  "        "
  "        float var2 = texture3D(volumeTexture,isoTexCoord).w;\n"
  "        vec4 color = vec4(texture1D(colormap, var2));\n"
  "        "
  "        // blend fragment color with front to back compositing operator \n"
  "        fragColor = (vec4(1.0)- vec4(fragColor.a))*color + fragColor;\n"
  " "
  "        // The depth buffer value will be the first ray-isosurface \n"
  "        // intersection. N.B. may not be best choice \n"
  " "
  "        if (first) {\n"
  "          fragDepth = zStartEye + (float(i) * deltaZ) + (deltaZ*weight);\n"
  "          first = false;\n"
  "        }\n"
  "      }\n"
  "     "
  "    texCoord0 = texCoord1;\n"
  "    texCoord1 = texCoord1+deltaVec;\n"
  "    s0 = s1;\n"
  "    s1 = texture3D(volumeTexture, texCoord1).x;\n"
  "  }\n"
  "  "
  "  if (fragColor.a == 0.0) discard;\n"
  "  "
  "  // Convert depth from eye coordinates back to window coords \n"
  "  "
  "  fragDepth = (fragDepth * gl_ProjectionMatrix[2].z + gl_ProjectionMatrix[3].z) / -fragDepth;\n" 
  "  gl_FragDepth = fragDepth * ((f-n)/2) + (n+f)/2;\n"
  "  gl_FragColor = fragColor;\n"
  "}\n";

//----------------------------------------------------------------------------
// Standard vertex shader for the isosurface model w/out lighting
//----------------------------------------------------------------------------
static char vertex_shader_iso_color[] =
  "//-----------------------------------------------------------------------\n"
  "// Vertex shader main\n"
  "//-----------------------------------------------------------------------\n"
  "#version 120\n"
  "uniform mat4 glModelViewProjectionMatrix;\n"
  "varying vec4 position;\n"
  "void main(void)\n"
  "{\n"
  "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
  "  gl_Position    = ftransform();\n"
  "  position       = gl_Position;\n"
  "}\n";

