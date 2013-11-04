uniform sampler2D texColor;
uniform sampler2D texDepth;

uniform float inM;
uniform float b1m;
uniform float s;

uniform vec2 arrCoordOffsets[15];
uniform int sizeArr;

float getCoC ()
{	
	float x = inM * texture2D(texDepth, gl_TexCoord[0].st).x;
	float y = b1m / x * abs(s-x);
	return y;
}

void main ()  
{
	float coc = getCoC ();
	vec4 finalColor = vec4(0);
	for (int i = 0; i < sizeArr; i++)
	{
		vec2 offset = gl_TexCoord[0].st;
		offset += arrCoordOffsets[i] * coc;
		finalColor += texture2D(texColor, offset);
	}
	gl_FragColor = finalColor / sizeArr;
}