uniform sampler2D tex;
void main ()  
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
	gl_FragColor = vec4(0,0,color.b,0);
}