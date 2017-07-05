
import os 
import fnmatch
import numpy as np
import matplotlib.pyplot as plot

DIRNAME='resultest'


def compute_iops(perfus, numreq):
	return numreq/(perfus/1000000)



def getavg(fname):
	darr = np.loadtxt(fname, delimiter=',', usecols=(1,), ndmin=0)
	#print "testing {}".format(fname)
	#print darr

	weights = np.ones(len(darr))
	weights[0] = 0 #hack to remove first entry..

	ret = np.average(darr, weights=weights)
	#print "ret is {}".format(ret)

	return ret
	

"""

def getdata(sz):
	ret = np.empty(len(sz))

	for i in range(len(sz)):
		rdir = os.path.join(os.getcwd(), DIRNAME)
		dirlist = os.listdir(rdir)
		for fname in dirlist:
			if fnmatch.fnmatch(fname, "results*{}.ix".format(str(sz[i]))):
				 ret[i] = getavg(os.path.join(rdir,fname))
	return ret			 

"""

def getdatal(sz, dirname):
	fullpath = os.path.join(os.getcwd(), dirname)
	for fname in os.listdir(fullpath):
			if fnmatch.fnmatch(fname, "results*{}.ix".format(str(sz))):
				 return getavg(os.path.join(fullpath, fname))




def graph_iosizes(dirname, iosizes):

	vect = np.vectorize(getdatal)
	perf = vect(iosizes, dirname)

	print perf

	plot.plot(iosizes, perf)
	plot.xlabel('IO Size (bytes)')
	plot.ylabel('Latency (us)')
	plot.title('Performance over IO sizes')

	plot.savefig('perf_iosizes.png')


if __name__=="__main__":

	"""
	for fname in os.listdir(dirname):
		f = open(fname, 'r')
		darr = np.loadtxt(f, delimiter='', usecols = (), ndmin=0)
	"""

	iosizes = [100, 500, 1000]
	#perf = [112, 120, 170]
	vect = np.vectorize(getdatal)
	perf = vect(iosizes, DIRNAME)

	print perf

	plot.plot(iosizes, perf)
	plot.xlabel('IO Size (bytes)')
	plot.ylabel('Latency (us)')
	plot.title('Performance over IO sizes')

	#dmin = np.amin(darr)
	#dmax = np.amax(darr)

	"""
	worklds = ('min ', 'avg', 'max')

	x_pos = np.arange(len(worklds))
	data = np.array([dmin, davg, dmax])	
	print data

	fig, ax = plot.subplots()

	ax.bar(x_pos, data, align='center')
	ax.set_xticks(x_pos)
	ax.set_xticklabels(worklds)
	ax.set_ylabel('Performance')
	"""

	#plot.show()
	plot.savefig('sample.png')

	print "works"