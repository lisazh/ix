"""
"""
import re
import sys
import os
import signal
import subprocess
import time
import mmap
import fnmatch
import random
from datetime import datetime
#from graphing import *
import numpy as np ##TODO find module 
#import matlibplot.pyplot as plot

#PARAMS
MODES = [0, 1, 2]  # ordering is to ensure writes go before reads..
STRMODES = ['r', 'w', 'rw']
IO_SZS = [100, 250, 500, 750, 1000, 2500, 5000]
IO_BSZS = [128, 256, 512, 768, 1024, 1280, 1536, 1796, 2048, 4096]
BT_SZS = [10, 100, 1000, 10000]
STOR_SZS = [1000, 2500, 5000, 7500, 10000, 25000, 50000, 75000, 100000]
DEF_ITER = 1000
DEF_IOSZ = 384
DEF_STORSZ = 512

#LOGISTICS
SRESULT_DIR = "resultsingle"
BRESULT_DIR = "resultsbatch"
IRESULT_DIR = "resultsindex"
DIRS = [SRESULT_DIR, BRESULT_DIR, IRESULT_DIR]
ALL_DIR = 'results_allbenchmarks'

IX_CMD = 'dp/ix'
BENCH_CMD = 'apps/iobench'

#MISC
TRIES = 5
WAIT_BASE = 10 #scientifically determined minimum wait time for IX startup...
ONE_M = 1000000


def graph_results():
	pass

def mv_results(whichdir, pref=""):

	cwd = os.getcwd()

	topdir = os.path.join(cwd, whichdir)
	if not os.path.exists(topdir):
		os.mkdir(topdir)
	
	if pref:
		pref+= '_'
	if whichdir == BRESULT_DIR:
		resultdir = os.path.join(topdir, pref + datetime.now().strftime("%d%m%y_%H%M%S"))
		os.mkdir(resultdir)	
		
		for f in os.listdir('.'):
	    		if fnmatch.fnmatch(f, 'results_*.ix'):
        			os.rename(os.path.join(cwd, f), os.path.join(resultdir, f))	
	elif whichdir == SRESULT_DIR:
		wresultdir = os.path.join(topdir, 'writes')
		rresultdir = os.path.join(topdir, 'reads')
		if not os.path.exists(wresultdir):
			os.mkdir(wresultdir)
		if not os.path.exists(rresultdir):
			os.mkdir(rresultdir)

		for f in os.listdir('.'):
			if fnmatch.fnmatch(f, 'results_w_*.ix'):
				os.rename(os.path.join(cwd, f), os.path.join(wresultdir, f))
			elif fnmatch.fnmatch(f, 'results_r_*.ix'):
				os.rename(os.path.join(cwd,f), os.path.join(rresultdir,f))	

	elif whichdir == IRESULT_DIR:
		resultdir = topdir
		for f in os.listdir('.'):
			if fnmatch.fnmatch(f, 'out_*.ix'):
				os.rename(os.path.join(cwd,f), os.path.join(resultdir))




def runner(mode, batchsz, itern, iosize, killat=0, outfname='out.ix', apnd=False):

	cwd = os.getcwd()

	fmode = 'a' if apnd else 'w+'
	outfile = open(outfname, fmode)
	proc = subprocess.Popen(
		['sudo', cwd + '/' +  IX_CMD, '--', cwd + '/' + BENCH_CMD, str(mode), str(batchsz), str(itern), str(iosize)], \
		stdout=outfile)

	time.sleep(WAIT_BASE)

	pid = proc.pid
	mfile = mmap.mmap(outfile.fileno(),0)
	if batchsz > 1:
		wait = (batchsz*10/ONE_M) #completely arbitrary
	else:	
		wait = (itern *100/ONE_M)  if mode == 1 else (itern/ONE_M)	

	for i in range(TRIES):
		if mfile.find('clean up complete') == -1:
			time.sleep(wait)
		elif killat:
			if mfile.find("put finished for key {}".format(killat)):
				break #jump to kill..
		else: 
			print 'benchmark completed'
			break

	p = subprocess.Popen(['sudo', 'killall', 'ix'])

	time.sleep(WAIT_BASE/2)	
	mfile.close()
	outfile.close()

#check it was killed.. TODO BETTER CHECK

	#p1 = subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE)
	#p2 = subprocess.Popen(['grep', 'ix'], stdin=p1.stdout, stdout=subprocess.PIPE)
	#p1.stdout.close()
	#output = p2.communicate()[0]
	#print output

	for i in range(TRIES):
		if not os.path.exists(os.path.join('/proc', str(pid))):
			print "Benchmark process killed successfully"
		else:
			time.sleep(0.01)
	if os.path.exists(os.path.join('/proc', str(pid))):
		print "Benchmark process killed successfully."


def benchmark_index():

	rg = re.compile('(?:DEBUG: index building took )\d+(?= milliseconds)')	
	#ntimes = random.randint(1, 10)
	ntimes = 5
	#technically should clean out "device" between runs..whatever
	for s in STOR_SZS:
		runner(1, 1, s, DEF_IOSZ)
		fout = open("out_{0}.ix".format(s), "w")
		for i in range(ntimes):
			outname = "out_{0}_{1}.ix".format(s,i)
			runner(0, 1, 1, DEF_IOSZ, outfname=outname) #dummy read call
	#print "Average times for index building were {0} over {2} runs for indexes of sizes {1}".format(ret, STOR_SZS, ntimes)				
		#write to a new file...
			
			f = open(outname, 'r')
			for line in f:
				if rg.match(line): #copy result line over
					fout.write(line)		
			f.close()
			os.remove(os.path.join(os.getcwd(), outname))	
		
		fout.close()
			
	mv_results(IRESULT_DIR)

def benchmark_batches(mode):

	ntimes = random.randint(5, 10)
	print "Running benchmark in mode {0} for {1} iterations".format(mode, ntimes)

	for i in range(ntimes):
		for arg in BT_SZS:
			runner(mode, arg, 1, DEF_IOSZ) #run twice to 
		mv_results(BRESULT_DIR, STRMODES[mode])


def benchmark_singles():
	#for arg in IO_SZS:
		#Run writes and then immediately followed by read (b/c read depends on existing written IO size)
		#runner(MODES[1], 1, DEF_ITER, arg) 
		#runner(MODES[0], 1, DEF_ITER, arg)
	for barg in IO_BSZS:
		runner(MODES[1], 1, DEF_ITER, barg)
		runner(MODES[0], 1, DEF_ITER, barg)
	mv_results(SRESULT_DIR)


def collect_results():

	cwd = os.getcwd()
	adir = os.path.join(cwd, ALL_DIR)
	if not os.path.exists(adir):
		os.mkdir(adir)
	for d in DIRS:
		os.rename(os.path.join(cwd, d), os.path.join(adir, d))

def main():
	benchmark_singles()

	#writes before reads..
	benchmark_batches(MODES[1]) 
	benchmark_batches(MODES[0])

	#benchmark_index()

	collect_results()

if __name__=="__main__":
	main()
