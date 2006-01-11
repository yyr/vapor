//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

uniform sampler1D colormap;
uniform sampler3D volumeTexture;
uniform vec3 dimensions;

uniform bool shading;
uniform float kd;
uniform float ka;
uniform float ks;
uniform float expS;

vec3 alphaGradient();

//----------------------------------------------------------------------------
// Fragment shader main
//----------------------------------------------------------------------------
void main(void)
{
  vec4 intensity = vec4 (texture3D(volumeTexture, gl_TexCoord[0].xyz));
  vec4 color     = vec4 (texture1D(colormap, intensity.x));

  if (shading)
  {
    vec3 lightPosition = vec3 (0, 0, 4.0);
    vec3 position      = vec3 (gl_ModelViewMatrix * gl_TexCoord[0].xyzw);
    vec3 light         = normalize(lightPosition - position);

    vec3 gradient = alphaGradient();

    if (length(gradient) == 0)
    {
      gradient = vec3 (0,0,1);
    }
    else
    {
      gradient = normalize(gl_NormalMatrix * gradient);
    }

    vec3 reflection = reflect(-light, gradient);
    vec3 view       = normalize(-position);

    float diffuse  = kd * abs(dot(light,gradient));
    float specular = ks * pow(abs(dot(reflection, view)), expS);

    gl_FragColor = color * (diffuse + specular + ka);
  }
  else
  {
    gl_FragColor = color;
  }
}


//----------------------------------------------------------------------------
// Compute the gradient using the alpha channel
//----------------------------------------------------------------------------
vec3 alphaGradient()
{
  vec3 gradient;

  float dx = 1.0/dimensions.x;
  float dy = 1.0/dimensions.y;
  float dz = 1.0/dimensions.z;
 
  float x0 = texture1D(colormap, 
                       texture3D(volumeTexture, 
                                 gl_TexCoord[0].xyz + vec3(dx,0,0)).x).r;
  float x1 = texture1D(colormap, 
                       texture3D(volumeTexture, 
                                 gl_TexCoord[0].xyz + vec3(-dx,0,0)).x).r;

  float y0 = texture1D(colormap, 
                       texture3D(volumeTexture, 
                                 gl_TexCoord[0].xyz + vec3(0,dy,0)).x).r;
  float y1 = texture1D(colormap, 
                       texture3D(volumeTexture, 
                                 gl_TexCoord[0].xyz + vec3(0,-dy,0)).x).r;

  float z0 = texture1D(colormap, 
                       texture3D(volumeTexture, 
                                 gl_TexCoord[0].xyz + vec3(0,0,dz)).x).r;
  float z1 = texture1D(colormap, 
                       texture3D(volumeTexture, 
                                 gl_TexCoord[0].xyz + vec3(0,0,-dz)).x).r;

  gradient.x = (x1-x0)/2.0;
  gradient.y = (y1-y0)/2.0;
  gradient.z = (z1-z0)/2.0;

  return gradient;
}
