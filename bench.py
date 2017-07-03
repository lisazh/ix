"""
"""


import os
import signal
import subprocess
import time
#import numpy ##TODO find module 
#import matlibplot.pyplot as plot


MODES = ['0', '1']
BENCH_PARAMS = []
RESULT_DIR = ""
#BASE_DIR = '/home/'
IX_CMD = 'dp/ix'
BENCH_CMD = 'apps/iobench'

WAIT = 20

"""
"""
def graph_results():
	pass


def benchmark_singles(mode, cwd):

	for arg in BENCH_PARAMS:
		proc = subprocess.Popen(
			['sudo', cwd + IX_CMD, '--', cwd + BENCH_CMD, mode, '1', arg], \
			stdout=PIPE)

		for i in range(WAIT):
			time.sleep(1)

		
		#kill 	

def benchmark_batches(mode, cwd):
	pass

def run_benchmark(m):
	cwd = os.getcwd()
	benchmark_singles(m, cwd)
	benchmark_batches(m, cwd)



def main():

	##for m in MODES:
		##run_benchmark(m)
	#os.setpgrp()
	outfile = os.open("out.ix", os.O_CREAT | os.O_TRUNC |os.O_WRONLY)
	cwd = os.getcwd()
	proc = subprocess.Popen(
		['sudo', cwd + '/' +  IX_CMD, '--', cwd + '/' + BENCH_CMD, '1', '1', '10'], \
		stdout=outfile)

	#out = proc.communicate()[0]
	#print out

	for i in range(WAIT):
		time.sleep(1)
	"""
		if proc.poll() is not None:
			print "Process exited\n"
			break
		else: 
			print "Waiting for process exit\n"
			time.sleep(1)
	"""
	
	#kill process anyway..
	print "Process group {} and PID {}".format(os.getpgrp(), proc.pid) 
	##os.killpg(0, signal.SIGKILL)
	p = subprocess.Popen(['sudo', 'killall', 'ix'])
	
	time.sleep(1)	
	os.close(outfile)
	#check it was killed..
	p1 = subprocess.Popen(['ps', 'aux'], stdout=subprocess.PIPE)
	p2 = subprocess.Popen(['grep', 'ix'], stdin=p1.stdout, stdout=subprocess.PIPE)
	p1.stdout.close()
	output = p2.communicate()[0]
	print output

if __name__=="__main__":
	main()
