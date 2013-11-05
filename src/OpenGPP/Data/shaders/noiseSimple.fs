// not used

uniform sampler2D tex;

vec3 noiseMappingFunction (const in vec3 base, const in vec3 random)
{
    return random / 4;
}


vec2 vecOffsetRed;
vec2 vecOffsetGreen;
vec2 vecOffsetBlue;



float getRandomFloat (const in vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 getNoisedValue (const in vec3 value)
{
    vec2 offset = gl_TexCoord[0].st;
    vec2 offsetRed = offset + vecOffsetRed;
    vec2 offsetGreen = offset + vecOffsetGreen;
    vec2 offsetBlue = offset + vecOffsetBlue;
    vec3 inRandom;
    inRandom.x = getRandomFloat(offsetRed);
    inRandom.y = getRandomFloat(offsetGreen);
    inRandom.z = getRandomFloat(offsetBlue);
    vec3 outRandom = noiseMappingFunction(value, inRandom);
    return outRandom;
}

void main ()  
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
  color.rgb += getNoisedValue(color.rgb);
	gl_FragColor = color;
}