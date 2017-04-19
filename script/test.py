import os
import time
for i in range(1,20):
	print "Test "+str(i)+" "+str(os.system("bash test"+str(i)))

