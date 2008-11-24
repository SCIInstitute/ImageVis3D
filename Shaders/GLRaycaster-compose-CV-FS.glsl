/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.

   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/**
  \file    GLRaycaster-compose-CV-FS.glsl
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    October 2008
*/

uniform sampler2D texRayHitPos;    ///< the hitposition of the ray (alpha flags hit yes/no)
uniform sampler2D texRayHitNormal; ///< the surface normal at the hit position
uniform sampler2D texRayHitPos2;    ///< the hitposition of the ray (alpha flags hit yes/no)
uniform sampler2D texRayHitNormal2; ///< the surface normal at the hit position

uniform vec3 vLightAmbient;
uniform vec3 vLightDiffuse;
uniform vec3 vLightDiffuse2;
uniform vec3 vLightSpecular;
uniform vec3 vLightDir;

uniform vec2 vScreensize;      ///< the size of the screen in pixels
uniform vec2 vProjParam;       ///< X = far / (far - near)  / Y = (far * near / (near - far))

vec3 Lighting(vec3 vPosition, vec3 vNormal, vec3 vLightAmbient, vec3 vLightDiffuse, vec3 vLightSpecular) {
	vec3 vViewDir    = normalize(vec3(0.0,0.0,0.0)-vPosition);
	vec3 vReflection = normalize(reflect(vViewDir, vNormal));
	return vLightAmbient+
				 vLightDiffuse*clamp(dot(vNormal, -vLightDir),0.0,1.0)+
			   vLightSpecular*pow(clamp(dot(vReflection, vLightDir),0.0,1.0),8.0);
}


void main(void){
  // compute the coordinates to look up the previous pass
  vec2 vFragCoords = vec2(gl_FragCoord.x / vScreensize.x , gl_FragCoord.y / vScreensize.y);

  // get hitposition and check if a isosurface hit for this ray was found
  vec4  vPosition = texture2D(texRayHitPos, vFragCoords);
  
  if (vPosition.a == 0.0) discard;
  
  // get hit normal
  vec3  vNormal  = texture2D(texRayHitNormal, vFragCoords).xyz;  

	// compute lighting
	vec3 vContextColor = Lighting(vPosition.xyz, vNormal, vLightAmbient, vLightDiffuse, vLightSpecular);

  // compute non linear depth from linear eye depth
  gl_FragDepth = vProjParam.x + (vProjParam.y / -vPosition.z);  
  
   // get 2nd hitposition and normal
  vec4  vPosition2 = texture2D(texRayHitPos2, vFragCoords);
  vec3  vNormal2   = texture2D(texRayHitNormal2, vFragCoords).xyz;  

  if (vPosition2.a != 0.0) {
	  vec3 vFocusColor = Lighting(vPosition2.xyz, vNormal2, vLightAmbient, vLightDiffuse2, vLightSpecular); 
	  gl_FragColor    = vec4(vFocusColor.x+vContextColor.x, vFocusColor.y+vContextColor.y, vFocusColor.z+vContextColor.z, 1.0);
  } else {
	  gl_FragColor    = vec4(vContextColor.x, vContextColor.y, vContextColor.z, 1.0);  
  }
}