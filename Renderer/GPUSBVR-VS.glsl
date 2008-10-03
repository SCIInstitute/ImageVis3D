
/*
 * Simple pass throught vertex shader, generates the texcoords automatically
*/

//varying vec3 texCoordYPos;

void main(void)
{
   gl_Position = vec4( gl_Vertex.xy, 0.0, 1.0 );
    
   // Texture coordinate for screen aligned (in correct range):
   gl_TexCoord[0].xy = (vec2( gl_Position.x, gl_Position.y ) + vec2( 1.0 ) ) / vec2( 2.0 );
   gl_TexCoord[0].zw = vec2(0.0,0.0);
}