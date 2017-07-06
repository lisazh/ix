import re
import os 
import fnmatch
import numpy as np
import matplotlib.pyplot as plot
import * from bench

SZDIR='resultsingle'


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
	

def grepdata(fpath):
	f = open(fpath, 'r')
	d = f.readline()
	f.close()
	m = re.search('\d+(?= microseconds)', d)
	print "Time was {}".format(m.group(0))
	return int(m.group(0))

def getbatchdata(sz):

	arr = []
	for root, dirs, fs in os.walk(os.path.join(os.getcwd(), BRESULT_DIR)):
		for d in dirs:
			for fname in os.listdir(os.path.join(root, d)):
				if fnmatch.fnmatch(fname, "results*")
					arr.append(grepdata(os.path.join(root, d, fname)))

	return np.mean(arr)			


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




def graph_iosizes(iosizes):

	vect = np.vectorize(getdatal)
	perf = vect(iosizes, SZDIR)

	print perf
	#TODO different lines.. how label
	plot.plot(iosizes, perf, label='Writes')
	plot.xlabel('IO Size (bytes)')
	plot.ylabel('Average Latency (us)')
	plot.title('Performance over IO sizes')

	plot.savefig('perf_iosizes.png')


def graph_indexrebuild():


	stor_sz = []
	rebuildtime = 

	plot.plot(stor_sz, rebuildtime)
	plot.xlabel()
	plot.ylabel()
	plot.title()

	plot.savefig('.png')


def graph_tpoh():
	pass

def graph_batch():
	
	res_fio= [20, 21, 22, 23]
	res_nodelay= [10, 11, 12, 13]
	res_delay = (30, 31, 32, 33)

	ind = np.arange(1,5)
	width = 0.25       # the width of the bars

	fig, ax = plot.subplots()
	rects1 = ax.bar(ind, res_nodelay, width, color='r')
	rects2 = ax.bar(ind + width, res_fio, width, color='y')
	rects3 = ax.bar(ind + 2*width, res_delay, width, color='g')

	# add some text for labels, title and axes ticks
	ax.set_ylabel('IOPS')
	ax.set_title('IOPS or smthg')
	ax.set_xticks(ind + width * 3/2)
	ax.set_xticklabels(('G1', 'G2', 'G3', 'G4'))

	plot.savefig('sampleiops.png')



if __name__=="__main__":

	"""
	for fname in os.listdir(dirname):
		f = open(fname, 'r')
		darr = np.loadtxt(f, delimiter='', usecols = (), ndmin=0)
	"""

	graph_batch()

	'''
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

	'''

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
	#plot.savefig('sample.png')

	print "works"