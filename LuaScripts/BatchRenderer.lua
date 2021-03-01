description = [[
********************************************************************************

Brief:  Example ImageVis3D batch renderer.
Author: James Hughes
Date:   December 2012

********************************************************************************
]]

print(description)

-- Program arguments?
dataset = "/Users/jhughes/sci/datasets/c60.uvf"
outputDir = "/Users/jhughes/sci/output/BatchRenderer"
shadersDir = "/Users/jhughes/sci/imagevis3d/Tuvok/Shaders"

-- Build 3D slice based volume renderer.
-- Parameters are: renderer type, use only power of two textures, downsample to 8 bits,
--                 disable border, bias and scale TF.
print("Initializing renderer")
renderer = tuvok.renderer.new(tuvok.renderer.types.OpenGL_SBVR, false, false, false, false, false)

-- Both load dataset and add shader path must be done before passing the context.
renderer.loadDataset(dataset)
renderer.addShaderPath(shadersDir)

-- tuvok.createContext() is a function that is bound in BatchRenderer.
-- Parameters are: Framebuffer width and height, color bits, depth bits, stencil 
--                 bits, double buffer, and if visible.
context = tuvok.createContext(640,480, 32,24,8, true, false)
renderer.initialize(context)
renderer.resize({640, 480})
renderer.setRendererTarget(tuvok.renderer.types.RT_Headless)
renderer.paint()

renderer.setRendererTarget(tuvok.renderer.types.RT_Capture)
renderer.captureSingleFrame(outputDir .. '/render.png', true)

renderer.cleanup()
deleteClass(renderer)

