"""
"""

import sys
import os
import signal
import subprocess
import time
import mmap
import fnmatch
import random
from datetime import datetime
from graphing import *
#import numpy ##TODO find module 
#import matlibplot.pyplot as plot

#PARAMS
MODES = [1, 0]  # ordering is to ensure writes go before reads..
IO_SZS = [100, 500, 1000]
BT_SZS = [10, 100, 1000, 10000]
DEF_ITER = 100
DEF_IOSZ = 128

#LOGISTICS
SRESULT_DIR = "resultsingle"
BRESULT_DIR = "resultsbatch"
IX_CMD = 'dp/ix'
BENCH_CMD = 'apps/iobench'

#MISC
TRIES = 5
WAIT_BASE = 10 #scientifically determined minimum wait time for IX startup...
ONE_M = 1000000


def graph_results():
	pass

def mv_results(whichdir):

	cwd = os.getcwd()

	topdir = os.path.join(cwd, whichdir)
	if not os.path.exists(topdir):
		os.mkdir(topdir)
	
	if whichdir == BRESULT_DIR:
		resultdir = os.path.join(topdir, datetime.now().strftime("%d%m%y_%H%M"))
		os.mkdir(resultdir)	
	else:
		resultdir = topdir

	for f in os.listdir('.'):
    		if fnmatch.fnmatch(f, 'results_*.ix'):
        		os.rename(os.path.join(cwd, f), os.path.join(resultdir, f))	


def runner(mode, batchsz, itern, iosize):

	cwd = os.getcwd()

	outfile = open('out.ix', 'w+')
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
		else: 
			print 'benchmark completed\n'
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

	if os.path.exists(os.path.join('/proc', str(pid))):
		print "Benchmark process not killed.."
	else:
		print "Benchmark process killed successfully."


def benchmark_batches(mode):

	ntimes = random.randint(1, 10)

	for i in range(ntimes):
		for arg in BT_SZS:
			runner(mode, arg, 1, DEF_IOSZ)

	mv_results(BRESULT_DIR)


def benchmark_singles(mode):

	for arg in IO_SZS:
		runner(mode, 1, DEF_ITER, arg)

	mv_results(SRESULT_DIR)

		
def run_benchmarks(m):
	#benchmark_singles(m)
	#benchmark_batches(m)

	#allresdir = os.path.join(cwd, 'resultsall', datetime.now().isoformat('-')) #NVM..
	graph_iosizes(SRESULT_DIR, IO_SZS)


def main():

	for m in MODES:
		print "running benchmark in mode {}".format(m)
		run_benchmarks(m)

if __name__=="__main__":
	main()
