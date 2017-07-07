import re
import os 
import fnmatch
import numpy as np
import matplotlib.pyplot as plot
from bench import *


NODELAYDIR = 'resultsbatchnodelay'

def compute_iops(perfus, numreq):
	#print "Params are {0} (num I/Os) {1} (performance)".format(numreq, perfus)
	return numreq/(perfus/ONE_M)



def getperc(p, fname):
	darr = np.loadtxt(fname, delimiter=',', usecols=(1,), ndmin=0, skiprows=2)
	#print "testing {}".format(fname)
	#print darr

	#weights = np.ones(len(darr))
	#weights[0] = 0 #hack to remove first entry..

	#ret = np.average(darr, weights=weights)
	#ret = np.mean(darr)
	#print "ret is {}".format(ret)
	altret = np.percentile(darr, p)
	#print "altret is {}".format(ret)

	return altret
	
def getfiodata(fiores):
	arr = []
	rg = re.compile('.*(?<=iops\=)\d+(?=, runt)')

	f = open(os.path.join(os.getcwd(), fiores))
	for line in f:
		if rg.match(line):
			#print "Matched at line {}".format(line)
			arr.append(int(re.search('(?<=iops\=)\d+', line).group()))

	return arr

def grepdata(fpath):
	#print "DEBUG path is {}".format(fpath)
	f = open(fpath, 'r')
	d = f.readline()
	f.close()
	m = re.search('\d+(?= microseconds)', d)
	#print "Time was {}".format(m.group(0))
	return int(m.group(0))

def wgetbatchdata(sz, dirname):

	arr = []
	for root, dirs, fs in os.walk(os.path.join(os.getcwd(), dirname)):
		for d in dirs:
			for fname in os.listdir(os.path.join(root, d)):
				if fnmatch.fnmatch(fname, "results_w_{}_*.ix".format(str(sz))):
					arr.append(grepdata(os.path.join(root, d, fname)))

	#print "Times for {0} requests were {1}".format(sz, arr)
	return compute_iops(np.mean(arr), sz)			

def rgetbatchdata(sz, dirname):

	arr = []
	for root, dirs, fs in os.walk(os.path.join(os.getcwd(), dirname)):
		for d in dirs:
			for fname in os.listdir(os.path.join(root, d)):
				if fnmatch.fnmatch(fname, "results_r_{}_*.ix".format(str(sz))):
					arr.append(grepdata(os.path.join(root, d, fname)))

	#print "Times for {0} requests were {1}".format(sz, arr)
	return compute_iops(np.mean(arr), sz)			



def getavgdata(sz, dirname):

	rg = re.compile('results_.*(?<=_){}.ix$'.format(sz))
	fullpath = os.path.join(os.getcwd(), dirname)	
	for fname in os.listdir(fullpath):
			if (rg.match(fname)):
			#if fnmatch.fnmatch(fname, "results_*{}.ix".format(str(sz))):
				#print "Matched file {0} for io size {1}".format(fname, sz)
				return getperc(50, os.path.join(fullpath, fname))

def getnndata(sz, dirname):
	rg = re.compile('results_.*(?<=_){}.ix$'.format(sz))
	fullpath = os.path.join(os.getcwd(), dirname)	
	for fname in os.listdir(fullpath):
			if (rg.match(fname)):
			#if fnmatch.fnmatch(fname, "results_*{}.ix".format(str(sz))):
				#print "Matched file {0} for io size {1}".format(fname, sz)
				return getperc(99, os.path.join(fullpath, fname))


def getindexdata(sz, dirname):

	tmp = []
	rg = re.compile('(?:DEBUG: index building took )\d+(?= milliseconds)')
	rf = re.compile('out_{}.ix'.format(str(sz)))

	fullpath = os.path.join(os.getcwd(), dirname)
	for fname in os.listdir(fullpath):
		if (rf.match(fname)):
			f = open(os.path.join(fullpath, fname), 'r')
			for line in f:
				if rg.match(line):
					tmp.append(int(re.search('\d+(?= milliseconds)', line).group()))

	#print "Times for creating index of size {0} were {1}".format(sz, tmp)

	return np.mean(tmp)


def graph_iosizes():

	avgvect = np.vectorize(getavgdata)
	nnvect = np.vectorize(getnndata)
	writedir = os.path.join(SRESULT_DIR, "writes")
	readsdir = os.path.join(SRESULT_DIR, "reads")
	
	#wperf = vect(IO_SZS, writedir)
	#rperf = vect(IO_SZS, readsdir)

	bwperf_avg = avgvect(IO_BSZS, writedir)
	brperf_avg = avgvect(IO_BSZS, readsdir)

	bwperf_nn = nnvect(IO_BSZS, writedir)
	brperf_nn = nnvect(IO_BSZS, readsdir)

	#print bwperf_nn
	#print brperf_nn
	#TODO different lines.. how label
	#plot.plot(IO_SZS, wperf, 'bo')
	#plot.plot(IO_SZS, wperf, 'b', label='Writes')
	#plot.plot(IO_SZS, rperf, 'go')
	#plot.plot(IO_SZS, rperf, 'g', label='Reads')

	fig, ax = plot.subplots()

	ax.plot(IO_BSZS, bwperf_avg, 'y.')
	ax.plot(IO_BSZS, bwperf_avg, 'y', linestyle="-", label="Writes (average)")
	ax.plot(IO_BSZS, bwperf_nn, 'y.')
	ax.plot(IO_BSZS, bwperf_nn, 'y', linestyle="--", label="Writes (99th)")

	ax.plot(IO_BSZS, brperf_avg, 'b.')
	ax.plot(IO_BSZS, brperf_avg, 'b', linestyle="-", label="Reads (average)")
	ax.plot(IO_BSZS, brperf_nn, 'b.')
	ax.plot(IO_BSZS, brperf_nn, 'b', linestyle="--", label="Reads (99th)")

	ax.set_xlabel('IO Size (bytes)')
	ax.set_ylabel('Latency (us)')
	ax.set_title('Performance over IO sizes')

	ax.legend(loc='upper left')
	plot.savefig('perf_iosizes.png')


def graph_indexrebuild():

	#rebuildtime = [652.0, 646.0, 664.0, 656.0, 651.0, 669.0, 716.0, 749.0, 801.0] 
	#rebuildtime = [866.0, 861.0, 858.0, 867.0, 860.0, 892.0, 875.0, 865.0, 861.0]
	indvect = np.vectorize(getindexdata)

	rebuildtime = indvect(STOR_SZS, IRESULT_DIR)


	fig, ax = plot.subplots()

	ax.plot(STOR_SZS, rebuildtime)
	ax.plot(STOR_SZS, rebuildtime, 'bo')
	ax.set_xlabel("Index size (number of 512B entries)")
	ax.set_ylabel("Time (milliseconds)")
	ax.set_title("Index rebuilding")

	plot.savefig('indexrebuild.png')


def graph_tpoh():
	pass

def graph_wbatch():
	
	vect = np.vectorize(wgetbatchdata)
	res_delay = vect(BT_SZS, BRESULT_DIR)
	res_nodelay = vect(BT_SZS, NODELAYDIR)
	res_fio = getfiodata('wfio_res.ix')


	print res_delay
	print res_nodelay
	print res_fio

	ind = np.arange(1,5)
	width = 0.25

	fig, ax = plot.subplots()
	rects1 = ax.bar(ind, res_delay, width, color='r')
	rects2 = ax.bar(ind + width, res_fio, width, color='y')
	rects3 = ax.bar(ind + 2*width, res_nodelay, width, color='g')

	# add some text for labels, title and axes ticks
	ax.set_ylabel('IOPS')
	ax.set_xlabel('Batch size (# requests)')
	ax.set_title('Write IOPS')
	ax.set_xticks(ind + width * 3/2)
	ax.set_xticklabels(BT_SZS) #TODO LOG SCALE

	ax.legend((rects1[0], rects2[0], rects3[0]), ('IX with delay', 'fio on ramdisk', 'IX w/o delay'), loc='upper left')

	plot.savefig('writeiops.png')

def graph_rbatch():
	
	vect = np.vectorize(rgetbatchdata)
	res_delay = vect(BT_SZS, BRESULT_DIR)
	res_nodelay = vect(BT_SZS, NODELAYDIR)
	res_fio = getfiodata('rfio_res.ix')


	print res_delay
	print res_nodelay
	print res_fio

	ind = np.arange(1,5)
	width = 0.25

	fig, ax = plot.subplots()
	rects1 = ax.bar(ind, res_delay, width, color='r')
	rects2 = ax.bar(ind + width, res_fio, width, color='y')
	rects3 = ax.bar(ind + 2*width, res_nodelay, width, color='g')

	# add some text for labels, title and axes ticks
	ax.set_ylabel('IOPS')
	ax.set_xlabel('Batch size (# requests)')
	ax.set_title('Read IOPS')
	ax.set_xticks(ind + width * 3/2)
	ax.set_xticklabels(BT_SZS) #TODO LOG SCALE

	ax.legend((rects1[0], rects2[0], rects3[0]), ('IX with delay', 'fio on ramdisk', 'IX w/o delay'), loc='upper left')

	plot.savefig('readiops.png')




if __name__=="__main__":

	"""
	for fname in os.listdir(dirname):
		f = open(fname, 'r')
		darr = np.loadtxt(f, delimiter='', usecols = (), ndmin=0)
	"""

	
	graph_iosizes()
	graph_indexrebuild()

	graph_rbatch()
	graph_wbatch()


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