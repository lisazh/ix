"""
"""

import sys
import os
import signal
import subprocess
import time
import mmap
import fnmatch
#import numpy ##TODO find module 
#import matlibplot.pyplot as plot


MODES = ['1', '0']
IO_SZS = [100, 500, 1000]
BT_SZS = [1, 10, 100, 1000, 10000]
DEF_ITER = 100
RESULT_DIR = ""
#BASE_DIR = '/home/'
IX_CMD = 'dp/ix'
BENCH_CMD = 'apps/iobench'


WAIT_BASE = 5 #scientifically determined minimum wait time for IX startup...
1M = 1000000
TRIES = 3
"""
"""
def graph_results():
	pass


def benchmark_singles(mode, cwd):

	for arg in IO_SZS:
		outfile = open('out.ix', 'w+')
		##mfile = mmap.mmap(outfile.fileno(), 0)
		proc = subprocess.Popen(
			['sudo', cwd + '/' +  IX_CMD, '--', cwd + '/' + BENCH_CMD, mode, '1', str(DEF_ITER), str(arg)], \
			stdout=outfile)

		print "running benchmark for io size of {}".format(arg)
		time.sleep(WAIT_BASE)
		mfile = mmap.mmap(outfile.fileno(),0)
		wait = (arg *100/1M)  if mode == '0' else (arg/1M)

		for i in range(TRIES): #TODO: replace with some value based on size of benchmark..
			if mfile.find('clean up complete') == -1:
				time.sleep(wait)
			else: 
				print 'benchmark completed\n'
				break
	
		p = subprocess.Popen(['sudo', 'killall', 'ix'])
	
		time.sleep(1)	

		#TODO grep and do smthg with index building time...

		mfile.close()
		outfile.close()
	
	#check it was killed..
		p1 = subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE)
		p2 = subprocess.Popen(['grep', 'ix'], stdin=p1.stdout, stdout=subprocess.PIPE)
		p1.stdout.close()
		output = p2.communicate()[0]
		print output

	##TODO move files somewhere

	resultdir = os.path.join(cwd, 'resultsraw')
	if not os.path.exists(resultdir):
		os.mkdir(resultdir)

	
	for f in os.listdir('.'):
    		if fnmatch.fnmatch(f, 'results_*.ix'):
        		os.rename(os.path.join(cwd, f), os.path.join(resultdir, f))
		
def run_benchmark(m):
	cwd = os.getcwd()
	benchmark_singles(m, cwd)
		
"""
if sys.argv[1] == 's':
		print "Running singles benchmark\n"
		benchmark_singles(m, cwd)
	elif sys.argv[1] == 'b':
		print "Running batches benchmark\n"
		benchmark_batches(m, cwd)
	else:
		print "No benchmark type specified..exiting.\n"

"""
def main():

	for m in MODES:
		print "running benchmark in mode {}".format(m)
		run_benchmark(m)

	"""
	outfile = open('out.ix', 'w+')
	#mfile = mmap.mmap(outfile.fileno(), 0)
	cwd = os.getcwd()
	
	proc = subprocess.Popen(
		['sudo', cwd + '/' +  IX_CMD, '--', cwd + '/' + BENCH_CMD, '1', '1', '10'], \
		stdout=outfile)

	#out = proc.communicate()[0]
	#print out
	time.sleep(WAIT_BASE)
	mfile = mmap.mmap(outfile.fileno(), 0)
	
	for i in range(20): #TODO: replace with some value based on size of benchmark..
		if mfile.find('clean up complete') == -1:
			time.sleep(1)
		else: 
			#TODO learn how to format strings in python...
			print 'benchmark completed at' 
			print i 
			print '\n'
			break
	
	#kill process anyway..
	#print "Process group {} and PID {}".format(os.getpgrp(), proc.pid) 
	##os.killpg(0, signal.SIGKILL)
	p = subprocess.Popen(['sudo', 'killall', 'ix'])
	
	time.sleep(1)	

	mfile.close() #close mmap first..
	outfile.close()
	
	#check it was killed..
	p1 = subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(['grep', 'ix'], stdin=p1.stdout, stdout=subprocess.PIPE)
	p1.stdout.close()
	output = p2.communicate()[0]
	print output

	"""

if __name__=="__main__":
	main()
