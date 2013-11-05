varying vec2 samplePos;

void main ()  
{
	gl_Position = ftransform();
	vec2 half2 = {0.5f, 0.5f};
	samplePos = ftransform().st / 2 + half2;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}