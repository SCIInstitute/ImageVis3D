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
  \file    GLRaycaster-ISO-CV-FS.glsl
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    October 2008
*/

uniform sampler3D texVolume;   ///< the data volume
uniform sampler2D texRayEntry; ///< the frontface or ray entry point texture
uniform sampler2D texRayEntryPos; ///< the frontface or ray entry point texture
uniform vec3 vVoxelStepsize;   ///< Stepsize (in texcoord) to get to the next voxel
uniform float fRayStepsize;    ///< stepsize along the ray
uniform float fIsoval;         ///< the isovalue
uniform float fIsovalFocus;    ///< the 2nd isovalue
uniform vec2 vScreensize;      ///< the size of the screen in pixels
uniform vec2 vProjParam;       ///< X = far / (far - near)  / Y = (far * near / (near - far))

varying vec3 vEyePos;

vec3 RefineIsosurface(vec3 vRayDir, vec3 vCurrentPos) {
	vRayDir /= 2.0;
	vCurrentPos -= vRayDir;
	for (int i = 0; i < 5; i++) {
		vRayDir /= 2.0;
		float voxel = texture3D(texVolume, vCurrentPos).x;
		if (voxel >= fIsoval) {
			vCurrentPos -= vRayDir;
		} else {
			vCurrentPos += vRayDir;
		}
	}	
	return vCurrentPos;
}


vec3 ComputeNormal(vec3 vHitPosTex) { 
  float fVolumValXp = texture3D(texVolume, vHitPosTex+vec3(+vVoxelStepsize.x,0,0)).x;
  float fVolumValXm = texture3D(texVolume, vHitPosTex+vec3(-vVoxelStepsize.x,0,0)).x;
  float fVolumValYp = texture3D(texVolume, vHitPosTex+vec3(0,-vVoxelStepsize.y,0)).x;
  float fVolumValYm = texture3D(texVolume, vHitPosTex+vec3(0,+vVoxelStepsize.y,0)).x;
  float fVolumValZp = texture3D(texVolume, vHitPosTex+vec3(0,0,+vVoxelStepsize.z)).x;
  float fVolumValZm = texture3D(texVolume, vHitPosTex+vec3(0,0,-vVoxelStepsize.z)).x;
  vec3  vGradient = vec3(fVolumValXm-fVolumValXp, fVolumValYp-fVolumValYm, fVolumValZm-fVolumValZp); 
  vec3 vNormal     = gl_NormalMatrix * vGradient;
  float l = length(vNormal); if (l>0.0) vNormal /= l; // secure normalization
  return vNormal;
}
  

void main(void)
{
  // compute the coordinates to look up the previous pass
  vec2 vFragCoords = vec2(gl_FragCoord.x / vScreensize.x , gl_FragCoord.y / vScreensize.y);

  // compute the ray parameters
  vec3  vRayEntry  = gl_TexCoord[0].xyz;
  vec3  vRayExit   = texture2D(texRayEntry, vFragCoords).xyz;
  vec3  vRayEntryPos  = texture2D(texRayEntryPos, vFragCoords).xyz;  
  vec3  vRayDir    = vRayExit - vRayEntry;
  float fRayLength = length(vRayDir);
  vRayDir /= fRayLength;

  // compute the maximum number of steps before the domain is left
  int iStepCount = int(fRayLength / length(fRayStepsize * vRayDir));

  // do the actual raycasting
  vec3  vCurrentPos = vRayEntry;
  vec4  vHitPosTex     = vec4(0.0,0.0,0.0,0.0);
  int i = 0;
  for (;i<iStepCount+1;i++) {
    float fVolumVal = texture3D(texVolume, vCurrentPos).x;	
    if (fVolumVal >= fIsoval) {
      vHitPosTex = vec4(vCurrentPos.x, vCurrentPos.y, vCurrentPos.z, 1);
      break;
    }
    vCurrentPos    += fRayStepsize * vRayDir;
  }
  
  // store surface hit if one is found
  if (vHitPosTex.a != 0.0) 
    vHitPosTex.xyz = RefineIsosurface(fRayStepsize * vRayDir, vHitPosTex.xyz); 
  else 
    discard;
  
  // interpolate eye space position
  float fInterpolParam = length(vHitPosTex.xyz-vRayEntry)/fRayLength;
  vec3 vHitPos = vRayEntryPos.xyz * (1.0-fInterpolParam) + vEyePos.xyz *  fInterpolParam;

  gl_FragDepth = vProjParam.x + (vProjParam.y / -vEyePos.z);  // as the surface maybe transparent set depth to the ray exit pos, so we get at least the bbox correct
  
  gl_FragData[0] = vec4(vHitPos,1);

  // store normal
  vec3 vNormal   = ComputeNormal(vHitPosTex.xyz);  
  gl_FragData[1] = vec4(vNormal,1);
  

  // do the raycasting for the second isovalue
  vHitPosTex = vec4(0.0,0.0,0.0,0.0);
  for (;i<iStepCount+1;i++) {
    float fVolumVal = texture3D(texVolume, vCurrentPos).x;	
    if (fVolumVal >= fIsovalFocus) {
      vHitPosTex = vec4(vCurrentPos.x, vCurrentPos.y, vCurrentPos.z, 1);
      break;
    }
    vCurrentPos    += fRayStepsize * vRayDir;
  }
    
  if (vHitPosTex.a != 0.0) {
    vHitPosTex.xyz = RefineIsosurface(fRayStepsize * vRayDir, vHitPosTex.xyz); 
     
    // interpolate eye space position
    fInterpolParam = length(vHitPosTex.xyz-vRayEntry)/fRayLength;
    vHitPos = vRayEntryPos.xyz * (1.0-fInterpolParam) + vEyePos.xyz *  fInterpolParam;
    gl_FragData[2] = vec4(vHitPos,1);    
     
    // store normal
    vNormal   = ComputeNormal(vHitPosTex.xyz);  
    gl_FragData[3] = vec4(vNormal,1);
  } else {
    gl_FragData[2] = vec4(0.0,0.0,0.0,0.0);
    gl_FragData[3] = vec4(0.0,0.0,0.0,0.0);
  }
}