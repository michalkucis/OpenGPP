uniform sampler2D texColor;
uniform sampler2D texDepth;

uniform float inM;
uniform float b1m;
uniform float s;

void main ()  
{
	float x = inM * texture2D(texDepth, gl_TexCoord[0].st).r;
	float y = b1m * abs(s-x) / x;
	gl_FragColor = vec4(y);
}