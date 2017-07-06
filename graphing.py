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



def getavg(fname):
	darr = np.loadtxt(fname, delimiter=',', usecols=(1,), ndmin=0)
	#print "testing {}".format(fname)
	#print darr

	weights = np.ones(len(darr))
	weights[0] = 0 #hack to remove first entry..

	ret = np.average(darr, weights=weights)
	#print "ret is {}".format(ret)

	return ret
	
def getfiodata(fiores):
	arr = []
	rg = re.compile('(?:\s*write\:).*(?<=iops\=)\d+(?=, runt)')

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

def getbatchdata(sz, dirname):

	arr = []
	for root, dirs, fs in os.walk(os.path.join(os.getcwd(), dirname)):
		for d in dirs:
			for fname in os.listdir(os.path.join(root, d)):
				if fnmatch.fnmatch(fname, "results_w_{}_*.ix".format(str(sz))):
					arr.append(grepdata(os.path.join(root, d, fname)))

	#print "Times for {0} requests were {1}".format(sz, arr)
	return compute_iops(np.mean(arr), sz)			




def getdatal(sz, dirname):

	rg = re.compile('results_.*(?<=_){}.ix$'.format(sz))
	fullpath = os.path.join(os.getcwd(), dirname)	
	for fname in os.listdir(fullpath):
			if (rg.match(fname)):
			#if fnmatch.fnmatch(fname, "results_*{}.ix".format(str(sz))):
				print "Matched file {0} for io size {1}".format(fname, sz)
				return getavg(os.path.join(fullpath, fname))




def graph_iosizes(iosizes):

	vect = np.vectorize(getdatal)
	wperf = vect(iosizes, os.path.join(SRESULT_DIR, "writes"))
	rperf = vect(iosizes, os.path.join(SRESULT_DIR, "reads"))

	print wperf
	print rperf
	#TODO different lines.. how label
	plot.plot(iosizes, wperf, label='Writes')
	plot.plot(iosizes, rperf, label='Reads')
	plot.xlabel('IO Size (bytes)')
	plot.ylabel('Average Latency (us)')
	plot.title('Performance over IO sizes')

	plot.legend()
	plot.savefig('perf_iosizes.png')


def graph_indexrebuild():


	stor_sz = []
	rebuildtime = 0

	plot.plot(stor_sz, rebuildtime)
	plot.xlabel()
	plot.ylabel()
	plot.title()

	plot.savefig('.png')


def graph_tpoh():
	pass

def graph_batch():
	
	vect = np.vectorize(getbatchdata)
	res_delay = vect(BT_SZS, BRESULT_DIR)
	res_nodelay = vect(BT_SZS, NODELAYDIR)
	res_fio = getfiodata('fio_res.ix')


	#print res_delay
	#print res_nodelay
	#print res_fio

	ind = np.arange(1,5)
	width = 0.25

	fig, ax = plot.subplots()
	rects1 = ax.bar(ind, res_delay, width, color='r')
	rects2 = ax.bar(ind + width, res_fio, width, color='y')
	rects3 = ax.bar(ind + 2*width, res_nodelay, width, color='g')

	# add some text for labels, title and axes ticks
	ax.set_ylabel('IOPS')
	ax.set_xlabel('Batch size (# requests)')
	ax.set_title('IOPS or smthg')
	ax.set_xticks(ind + width * 3/2)
	ax.set_xticklabels(BT_SZS) #TODO LOG SCALE

	ax.legend((rects1[0], rects2[0], rects3[0]), ('IX with delay', 'fio on ramdisk', 'IX w/o delay'), loc='upper left')


	plot.savefig('sampleiops.png')



if __name__=="__main__":

	"""
	for fname in os.listdir(dirname):
		f = open(fname, 'r')
		darr = np.loadtxt(f, delimiter='', usecols = (), ndmin=0)
	"""

	graph_iosizes(IO_SZS)
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