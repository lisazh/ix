"""
"""


import os
import subprocess
import time
import numpy 
import matlibplot.pyplot as plot


MODES = ['0', '1']
BENCH_PARAMS = []
RESULT_DIR = ""
#BASE_DIR = '/home/'
IX_CMD = BASE_DIR + 'dp/ix'
BENCH_CMD = BASE_DIR + 'apps/iobench'

WAIT = 5

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


def run_benchmark(m):
	cwd = os.getcwd()
	benchmark_singles(m, cwd)
	benchmark_batches(m, cwd)



def main():

	##for m in MODES:
		##run_benchmark(m)

	proc = subprocess.Popen(
		['sudo', cwd + IX_CMD, '--', cwd + BENCH_CMD, mode, '1', arg], \
		stdout=PIPE)

	#output = proc.communicate()[0]


	for i in range(WAIT):
		if proc.poll() is not None:
			break
		time.sleep(1)

	#kill process anyway..
	proc.kill()

if __name__=="__main__":
	main()
