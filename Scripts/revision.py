# A replacement for "svn info . | grep Revision | awk '{print $2}',
# because windows' shell is worthless.
import subprocess
import sys
import os

p1 = subprocess.Popen(["svn", "info", "%s" % sys.argv[1]], stdout=subprocess.PIPE)
p2 = subprocess.Popen(["grep", "Revision"], stdin=p1.stdout, stdout=subprocess.PIPE)
if os.name == "nt":
  p3 = subprocess.Popen(["gawk", "{print $2}"], stdin=p2.stdout, stdout=subprocess.PIPE)
else:
  p3 = subprocess.Popen(["awk", "{print $2}"], stdin=p2.stdout, stdout=subprocess.PIPE)
output = p3.communicate()[0]
print output.rstrip()
