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


MODES = ['0', '1']
IO_SZS = [100, 500, 1000]
BT_SZS = []
RESULT_DIR = ""
#BASE_DIR = '/home/'
IX_CMD = 'dp/ix'
BENCH_CMD = 'apps/iobench'

WAIT_BASE = 5 #scientifically determined minimum wait time for IX startup...

"""
"""
def graph_results():
	pass


def benchmark_singles(mode, cwd):

	for arg in IO_SZS:
		outfile = open('out.ix', 'w+')
		mfile = mmap.mmap(outfile.fileno(), 0)
		proc = subprocess.Popen(
			['sudo', cwd + '/' +  IX_CMD, '--', cwd + '/' + BENCH_CMD, mode, '1', arg], \
			stdout=outfile)

		time.sleep(WAIT_BASE)

		for i in range(WAIT_BASE * 10): #TODO: replace with some value based on size of benchmark..
			if mfile.find('clean up complete') == -1:
				time.sleep(1)
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
	os.mkdir(resultsdir)

	
	for f in os.listdir('.'):
    	if fnmatch.fnmatch(f, 'results_*.ix'):
        	os.rename(os.path.join(cwd, f), os.path.join(resultsdir, f))
		
def run_benchmark(m):
	cwd = os.getcwd()
	if sys.argv[1] == 's':
		print "Running singles benchmark\n"
		benchmark_singles(m, cwd)
	else if sys.argv[1] == 'b':
		print "Running batches benchmark\n"
		benchmark_batches(m, cwd)
	else:
		print "No benchmark type specified..exiting.\n"


def main():

	for m in MODES:
		run_benchmark(m)

	"""
	outfile = open('out.ix', 'w+')
	mfile = mmap.mmap(outfile.fileno(), 0)
	cwd = os.getcwd()
	proc = subprocess.Popen(
		['sudo', cwd + '/' +  IX_CMD, '--', cwd + '/' + BENCH_CMD, '1', '1', '10'], \
		stdout=outfile)

	#out = proc.communicate()[0]
	#print out

	time.sleep(WAIT_BASE)
	for i in range(WAIT_BASE): #TODO: replace with some value based on size of benchmark..
		if mfile.find('clean up complete') == -1:
			time.sleep(1)
		else: 
			print 'benchmark completed\n'
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
