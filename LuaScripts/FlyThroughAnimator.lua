description = [[
*************************************************************************************************************

Brief:  ImageVis3D Lua script to interactively generate a key frame sequence for an animated fly through
Author: Alexander Schiewe
Date:   October 2012

HowTo:  - press 'F'-key to enable first person mode and use first person mode navigation only
        - use '0'-key to capture key frame and insert it at currently active key frame position 
        - use ','/'.'-key to delete active key frame
        - use '1'/'2'/'3'-keys to jump to previous/active/next key frames
        - use 'Enter'-key to print captured key frames
        - use '/'-key to write captured key frames to file
        - use '*'-key to generate animation preview data structures
        - use '-'/'+'-keys to step along the last generated animation path

TODOs:  - automatically update animation data structure when manipulating key points

*************************************************************************************************************
]]

print(description)
iv3d.setStayOpen(true)

inputDir = 'S:/Share/Datasets/Octrees/ImageVis/'

--filename = inputDir ..'mandelbulb1024-20s-bs32-8.uvf'
--filename = inputDir ..'mandelbulb2048-20s-bs32-8.uvf'
--filename = inputDir ..'mandelbulb4096-20s-bs32-8.uvf'
--filename = inputDir ..'mandelbulb8192-20s-bs32-8.uvf'
--filename = inputDir ..'mandelbulb16384-20s-bs64-8.uvf'
--filename = inputDir ..'RichtmyerMeshkov-2048x2048x1920-8bit-64.uvf'
--filename = inputDir ..'VisualHuman-512x512x1884-16bit-32-median-clamp-zlib.uvf'
filename = inputDir ..'wholebody-32.uvf'

data = iv3d.renderer.new(filename)
--data.lighting(false)
--data.resize({1920, 1080})
--data.resize({960, 540})
data.resize({480, 270})

rw = data.getRawRenderer()
rw.setBGColors({0, 0, 255}, {0, 255, 255})
--rw.setBGColors({255, 255, 255}, {255, 255, 255})
--rw.setBGColors({0, 0, 0}, {0, 0, 0})

region = rw.getFirst3DRenderRegion()

-- key point capturing **************************************************************************************

outputSequence = 0
activeKeyPoint = 0
keyPoints = {}

function getCamera()
	return {eye={x=rw.getViewPos()[1], y=rw.getViewPos()[2], z=rw.getViewPos()[3]},
				  ref={x=rw.getViewDir()[1], y=rw.getViewDir()[2], z=rw.getViewDir()[3]},
				  vup={x=rw.getUpDir()[1],   y=rw.getUpDir()[2],   z=rw.getUpDir()[3]}}
end

function setCamera(cam)
	rw.setViewPos({cam.eye.x, cam.eye.y, cam.eye.z})
	rw.setViewDir({cam.ref.x, cam.ref.y, cam.ref.z})
	rw.setUpDir(  {cam.vup.x, cam.vup.y, cam.vup.z})
	iv3d.processUI()
end

function cameraToString(cam)
	return "{eye={x=".. cam.eye.x ..", y=".. cam.eye.y ..", z=".. cam.eye.z .."}, ref={x=".. cam.ref.x ..", y=".. cam.ref.y ..", z=".. cam.ref.z .."}, vup={x=".. cam.vup.x ..", y=".. cam.vup.y ..", z=".. cam.vup.z .."}}"
end

function matrixToString(mat)
  return "{{"..mat[1][1]..", "..mat[1][2]..", "..mat[1][3]..", "..mat[1][4].."}, {"..mat[2][1]..", "..mat[2][2]..", "..mat[2][3]..", "..mat[2][4].."}, {"..mat[3][1]..", "..mat[3][2]..", "..mat[3][3]..", "..mat[3][4].."}, {"..mat[4][1]..", "..mat[4][2]..", "..mat[4][3]..", "..mat[4][4].."}}"
end

function insertKeyFrame(index)
	table.insert(keyPoints, index, getCamera())
	print("Captured key frame at index ".. index .."/".. #keyPoints ..": ".. cameraToString(keyPoints[index]))
end

function deleteKeyFrame(index)
	if index < 1 or index > #keyPoints then
		print("There is no captured key frame with index ".. index .. " to be deleted")
		return
	end
	print("Deleting key frame at index ".. index .."/".. #keyPoints ..": ".. cameraToString(keyPoints[index]))
	table.remove(keyPoints, index)
	if activeKeyPoint > #keyPoints then
		activeKeyPoint = #keyPoints
	end
end

function jumpToKeyFrame(index)
	if #keyPoints == 0 then
		print("There are no captured key frames to jump to")
		return
	end
	if index < 1 then
		index = #keyPoints
		print("WRAPPED AROUND to the last key frame")
	elseif index > #keyPoints then
		index = 1
		print("WRAPPED AROUND to the first key frame")
	end
	print("Jumping to key frame: ".. index)
	local cam = keyPoints[index]
	activeKeyPoint = index
	setCamera(cam)
end

function printKeyFrames()
	if #keyPoints == 0 then
		print("There are no captured key frames to print")
		return
	end
	for i = 1, #keyPoints do
		print(string.format("%02d", i) ..": ".. cameraToString(keyPoints[i]))
	end
	print("Active key frame: ".. activeKeyPoint)
end

function writeKeyFramesToFile(filename)
  io.output(filename)

  -- would be very useful if we could read those values into a matrix easily
  --io.write("translation = ".. matrixToString(region.getTranslation4x4()) .."\n")
  --io.write("rotation = "..    matrixToString(region.getRotation4x4()) .."\n")

  local t = region.getTranslation4x4()
  io.write("local t=matrix.identity()\n")
  io.write("t[1][1]="..t[1][1].."; t[1][2]="..t[1][2].."; t[1][3]="..t[1][3].."; t[1][4]="..t[1][4].."\n");
  io.write("t[2][1]="..t[2][1].."; t[2][2]="..t[2][2].."; t[2][3]="..t[2][3].."; t[2][4]="..t[2][4].."\n");
  io.write("t[3][1]="..t[3][1].."; t[3][2]="..t[3][2].."; t[3][3]="..t[3][3].."; t[3][4]="..t[3][4].."\n");
  io.write("t[4][1]="..t[4][1].."; t[4][2]="..t[4][2].."; t[4][3]="..t[4][3].."; t[4][4]="..t[4][4].."\n");
  io.write("region.setTranslation4x4(t)\n");
  local r = region.getRotation4x4()
  io.write("local r=matrix.identity()\n")
  io.write("r[1][1]="..r[1][1].."; r[1][2]="..r[1][2].."; r[1][3]="..r[1][3].."; r[1][4]="..r[1][4].."\n");
  io.write("r[2][1]="..r[2][1].."; r[2][2]="..r[2][2].."; r[2][3]="..r[2][3].."; r[2][4]="..r[2][4].."\n");
  io.write("r[3][1]="..r[3][1].."; r[3][2]="..r[3][2].."; r[3][3]="..r[3][3].."; r[3][4]="..r[3][4].."\n");
  io.write("r[4][1]="..r[4][1].."; r[4][2]="..r[4][2].."; r[4][3]="..r[4][3].."; r[4][4]="..r[4][4].."\n");
  io.write("region.setRotation4x4(r)\n");

	if #keyPoints == 0 then
		print("There are no captured key frames but dataset transformation parameters were written to file: "..filename)
		return true
	end

  io.write("keyPoints = {}\n")
	for i = 1, #keyPoints do
		io.write("keyPoints[".. i .."] = ".. cameraToString(keyPoints[i]) .."\n")
	end
  io.flush()
  io.close()
  print("Wrote ".. #keyPoints .." captured key frames to file: ".. filename)
  return true
end

-- animation stuff ******************************************************************************************

-- basic vector calculus
function vminus(a, b)
  return {x=a.x-b.x, y=a.y-b.y, z=a.z-b.z}
end
function vadd(a, b)
  return {x=a.x+b.x, y=a.y+b.y, z=a.z+b.z}
end
function vlength(v)
  return math.sqrt(v.x*v.x + v.y*v.y + v.z*v.z)
end
function vnormalize(v)
  local len = vlength(v)
  return {x=v.x/len, y=v.y/len, z=v.z/len}
end
function vscale(v, scale)
  return {x=v.x*scale, y=v.y*scale, z=v.z*scale}
end

-- scale and bias a 'value' from the 'i' range to the 'o' range
function remap(value, imin, imax, omin, omax)
  return omin + (value-imin) * ((omax-omin)/(imax-imin))
end

-- linear interpolation between two values
function lerp(a, b, u)
  assert(0.0 <= u and u <= 1.0)
  return (1-u) * a + u * b
end

-- linear interpolation between two vectors
function vlerp(v0, v1, u)
  return {x=lerp(v0.x, v1.x, u),
          y=lerp(v0.y, v1.y, u),
          z=lerp(v0.z, v1.z, u)}
end

-- evaluate decasteljau algorithm for given curve parameter u [0, 1]
function decasteljau(p00, p01, p02, p03, u)
  -- first gen points
  local p10 = vlerp(p00, p01, u)
  local p11 = vlerp(p01, p02, u)
  local p12 = vlerp(p02, p03, u)
  -- second gen points
  local p20 = vlerp(p10, p11, u)
  local p21 = vlerp(p11, p12, u)
  -- third gen points
  local p30 = vlerp(p20, p21, u)
  -- return both bezier segments
  -- [3] p30 is the sampled point
  return {p10, p20, p30, p21, p12}
end

-- compute tangents for bezier curve from key points
function computeTangents(p0, p1, p2, p3)
  local c = vlength(vminus(p2, p1))
  local alpha = c / 2
  if (alpha == 0.0) then
    --print('KEY POINTS ARE IDENTICAL')
    return p1, p2
  end

  local q1 = vadd(p1, vscale(vnormalize(vminus(p2, p0)), alpha))
  local q2 = vadd(p2, vscale(vnormalize(vminus(p1, p3)), alpha))
  return q1, q2
end

-- adaptively sample arc length parameterization of bezier curve
function adaptivelySampleBezierCurve(p1, q1, q2, p2, uA, uB, epsilon, samples)
  local segments = decasteljau(p1, q1, q2, p2, 0.5)
  local p = segments[3]
  local a = vlength(vminus(p, p1))
  local b = vlength(vminus(p2, p))
  local c = vlength(vminus(p2, p1))

  -- subdivide curve segment if triangle is too large and recurse
  if math.abs(c - (a + b)) > epsilon then
    local u = (uB - uA) / 2
    adaptivelySampleBezierCurve(p1, segments[1], segments[2], p,  uA - u, uA, epsilon, samples)
    adaptivelySampleBezierCurve(p,  segments[4], segments[5], p2, uB - u, uB, epsilon, samples)
  else
    local lastEntry = samples[#samples]
    local newEntryA = {distance = lastEntry.distance + a, parameter = uA}
    local newEntryB = {distance = newEntryA.distance + b, parameter = uB}
    table.insert(samples, newEntryA)
    table.insert(samples, newEntryB)
  end
end

-- initialize arc length parametrization for given segment defined by four key points
function sampleArcLengthForSegment(p0, p1, p2, p3, u, epsilon, samples)
  local q1, q2 = computeTangents(p0, p1, p2, p3)
  adaptivelySampleBezierCurve(p1, q1, q2, p2, u, u+0.5, epsilon, samples)
end

-- compute arc length lookup table
function sampleArcLength(keyPoints, frameCount)
  local distance = 0
  for i = 4,#keyPoints do
    local d = vlength(vminus(keyPoints[i-1].eye, keyPoints[i-2].eye))
    --print("distance " .. i-2 .. " to " .. i-1 .. ": " .. d)
    distance = distance + d
  end
  print("Linear distance:  ".. distance)

  -- sample curve according to epsilon threshold
  local epsilon = distance / frameCount
  print("Split epsilon:    ".. epsilon)
  local samples = {}
  samples[1] = {distance=0, parameter=2}
  for i = 4,#keyPoints do
    sampleArcLengthForSegment(keyPoints[i-3].eye, keyPoints[i-2].eye, keyPoints[i-1].eye, keyPoints[i].eye, i-1.5, epsilon, samples)
  end

  --for i = 1, #samples do print(i .."\t".. samples[i].distance .."\t".. samples[i].parameter) end
  print("Sampled distance: ".. samples[#samples].distance .." (".. #samples .." samples)")

  return samples
end

-- interpolate camera properties for given traveled distance s
function animateCamera(keyPoints, lookup, s, indexHint)
  assert(lookup[1].distance == 0.0)
  if s < 0.0 then
    s = 0.0
    print("CLAMPED S TO MIN VALUE")
  elseif s > lookup[#lookup].distance then
    s = lookup[#lookup].distance
    print("CLAMPED S TO MAX VALUE")
  end

  -- find first index that distance is equal or larger than s
  if indexHint < 2 then
  	indexHint = 2
  elseif indexHint > #lookup then
  	indexHint = #lookup
  end
  local idx = indexHint -- default should be 2
  while lookup[idx].distance < s do
    idx = idx + 1
  end
  
  -- interpolate u
  local u = remap(s, lookup[idx-1].distance,  lookup[idx].distance,
                     lookup[idx-1].parameter, lookup[idx].parameter)

  local i = math.floor(u) -- key point index
  local w = u - i         -- local curve parameter
  --print("u: ".. u .." i: ".. i .." w: ".. w)
  if i == u then
    print("EARLY EXIT")
    return keyPoints[i], idx -- early exit, we hit exactly a key point
  end  

  local p0, p1, p2, p3 = keyPoints[i-1], keyPoints[i], keyPoints[i+1], keyPoints[i+2]
  local q1, q2 = {}, {}
  q1.eye, q2.eye = computeTangents(p0.eye, p1.eye, p2.eye, p3.eye)
  q1.ref, q2.ref = computeTangents(p0.ref, p1.ref, p2.ref, p3.ref)
  q1.vup, q2.vup = computeTangents(p0.vup, p1.vup, p2.vup, p3.vup)
  local Eye = decasteljau(p1.eye, q1.eye, q2.eye, p2.eye, w)[3]
  local Ref = decasteljau(p1.ref, q1.ref, q2.ref, p2.ref, w)[3]
  local Vup = decasteljau(p1.vup, q1.vup, q2.vup, p2.vup, w)[3]

  -- now we can construct our new camera and return the last used lookup index to speed up next search
  return {eye=Eye, ref=Ref, vup=Vup}, idx
end

-- http://lua-users.org/wiki/CopyTable
function deepcopy(object)
    local lookup_table = {}
    local function _copy(object)
        if type(object) ~= "table" then
            return object
        elseif lookup_table[object] then
            return lookup_table[object]
        end
        local new_table = {}
        lookup_table[object] = new_table
        for index, value in pairs(object) do
            new_table[_copy(index)] = _copy(value)
        end
        --return setmetatable(new_table, getmetatable(object))
        return setmetatable(new_table, _copy( getmetatable(object)))
    end
    return _copy(object)
end

-- hot keys *************************************************************************************************

function key0Pressed()
	--print("key0Pressed()")
	-- capture key frame
	activeKeyPoint = activeKeyPoint + 1
	insertKeyFrame(activeKeyPoint)
end

function key1Pressed()
	--print("key1Pressed()")
	-- jump to previous key frame
	jumpToKeyFrame(activeKeyPoint - 1)
end

function key2Pressed()
	--print("key2Pressed()")
	-- jump to current key frame
	jumpToKeyFrame(activeKeyPoint)
end

function key3Pressed()
	--print("key3Pressed()")
	-- jump to next key frame
	jumpToKeyFrame(activeKeyPoint + 1)
end

function keyCommaOrPeriodPressed()
	--print("keyCommaOrPeriodPressed()")
	-- delete key frame
	deleteKeyFrame(activeKeyPoint)
end

function keyEnterPressed()
	--print("keyEnterPressed()")
	-- print key frames
	printKeyFrames()
end

function keySlashPressed()
  --print("keySlashPressed()")
  -- write key frames to file
  if writeKeyFramesToFile(filename .."-KeyFrames".. string.format("%02d", outputSequence) ..".txt") then
    outputSequence = outputSequence + 1
  end
end

-- parameters for the animation preview
loopAnimation = false -- do we want a looped animation preview?
animation = {}        -- prepared key points
lookup = {}           -- arc length lookup table
indexHint = 1         -- (currently not used)
distance = 0          -- total arc length for the whole animation path (will be computed)
delta = 0.000         -- distance that should be traveled on keyPlusPressed()/keyMinusPressed() (good starting value is 0.005)
frameCount = 20       -- used frame count if delta is set to zero
s = 0                 -- current arc length

function keyAsteriskPressed()
	--print("keyAsteriskPressed()")
	-- prepare animation preview
	animation = deepcopy(keyPoints)

	if loopAnimation then
	  -- prepend last point and append first point to form a perfect loop (first and last frame will be identical)
	  table.insert(animation, 1, animation[#animation]) -- we need the last point here to have the right tangent for the first point
	  table.insert(animation, animation[2])             -- we need the original first point to have the right tangent for the last point
	  table.insert(animation, animation[3])             -- we need one more point to make sure to end up at the first point again
	else
	  -- just duplicate first and last key point
	  table.insert(animation, 1, animation[1])
	  table.insert(animation, animation[#animation])
	end

	lookup = sampleArcLength(animation, 100000)
	distance = lookup[#lookup].distance
  if delta == 0 then
    delta = distance / frameCount
  else
    frameCount = distance / delta
  end
	print("Animation delta is set to: ".. delta .." (resulting in ".. frameCount .." frames)")

	local idx = 1
  while lookup[idx].parameter < (activeKeyPoint + 1) do
    idx = idx + 1
  end
  s = lookup[idx].distance

  collectgarbage()
  print("Prepared animation. Set current arc length according to active key point (".. activeKeyPoint ..") to: ".. s)
end

function keyMinusPressed()
	--print("keyMinusPressed()")
	s = s - delta
	if s < 0 then
		print("WRAPPED AROUND to the last animation frame")
		s = distance
	end
	local cam, hint = animateCamera(animation, lookup, s, indexHint)
	activeKeyPoint = math.floor(lookup[hint].parameter) - 1
	print("Animated. Set active key point according to current arc length (".. s ..") to: ".. activeKeyPoint)
	indexHint = 1
	setCamera(cam)
end

function keyPlusPressed()
	--print("keyPlusPressed()")
	s = s + delta
	if s > distance then
		print("WRAPPED AROUND to the first animation frame")
		s = 0
	end
	local cam, hint = animateCamera(animation, lookup, s, indexHint)
	activeKeyPoint = math.floor(lookup[hint].parameter) - 1
	print("Animated. Set active key point according to current arc length (".. s ..") to: ".. activeKeyPoint)
	indexHint = 1
	setCamera(cam)
end
