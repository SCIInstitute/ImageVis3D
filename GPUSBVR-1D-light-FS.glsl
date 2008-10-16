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
  \file    GPUSBVR-1D-light-FS.glsl
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    October 2008
*/

uniform sampler3D texVolume;  ///< the data volume
uniform sampler1D texTrans1D; ///< the 1D Transfer function
uniform float fTransScale;    ///< scale for 1D Transfer function lookup
uniform float fStepScale;   ///< quotient of nyquist and actual stepsize
uniform vec3 vVoxelStepsize;  ///< Stepsize (in texcoord) to get to the next voxel

uniform vec3 vLightAmbient;
uniform vec3 vLightDiffuse;
uniform vec3 vLightSpecular;
uniform vec3 vLightDir;

void main(void)
{
  /// get volume value
	float fVolumVal = texture3D(texVolume, gl_TexCoord[0].xyz).x;	
  vec4  vTransVal = texture1D(texTrans1D, fVolumVal*fTransScale);

  // compute the gradient/normal
	float fVolumValXp = texture3D(texVolume, gl_TexCoord[0].xyz+vec3(+vVoxelStepsize.x,0,0)).x;
	float fVolumValXm = texture3D(texVolume, gl_TexCoord[0].xyz+vec3(-vVoxelStepsize.x,0,0)).x;
	float fVolumValYp = texture3D(texVolume, gl_TexCoord[0].xyz+vec3(0,-vVoxelStepsize.y,0)).x;
	float fVolumValYm = texture3D(texVolume, gl_TexCoord[0].xyz+vec3(0,+vVoxelStepsize.y,0)).x;
	float fVolumValZp = texture3D(texVolume, gl_TexCoord[0].xyz+vec3(0,0,+vVoxelStepsize.z)).x;
	float fVolumValZm = texture3D(texVolume, gl_TexCoord[0].xyz+vec3(0,0,-vVoxelStepsize.z)).x;
  vec3  vGradient = vec3(fVolumValXm-fVolumValXp, fVolumValYp-fVolumValYm, fVolumValZm-fVolumValZp); 
  vec3  vNormal = normalize(gl_NormalMatrix * vGradient);

  // compute lighting
  vec3 vPosition   = (gl_ModelViewMatrix * vec4(gl_TexCoord[0].xyz*2.0-1.0,1)).xyz;
  vec3 vViewDir    = normalize(vec3(0,0,0)-vPosition);
  vec3 vReflection = reflect(vViewDir, vNormal);
  vec3 vLightColor = vLightAmbient+clamp(dot(vNormal, -vLightDir),0.0,1.0)*vTransVal.xyz*vLightDiffuse+pow(clamp(dot(vReflection, vLightDir),0.0,1.0),8.0)*vLightSpecular;

  /// apply opacity correction
  vTransVal.a = 1.0 - pow(1.0 - vTransVal.a, fStepScale);

  /// write result to fragment color
	gl_FragColor    = vec4(vLightColor.x, vLightColor.y, vLightColor.z, vTransVal.a);
}