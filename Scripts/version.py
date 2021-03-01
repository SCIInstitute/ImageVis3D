# A replacement for "grep IV3D_MAJOR StdDefines.h | awk "{print $3}"
# because windows' shell is worthless.
import subprocess
import sys

p1 = subprocess.Popen(["grep", "%s" % sys.argv[1], "ImageVis3D/StdDefines.h"],
                      stdout=subprocess.PIPE)
output = p1.communicate()[0]
ver = output.split()
print ver[2]
