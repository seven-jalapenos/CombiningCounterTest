import benchmark
import benchmark_helper
import subprocess

path_to_python = '/home/jjurgenson/code_stuff/CombiningCounterTest/test/.venv/bin/python'

def main():
	subprocess.run([path_to_python, 'benchmark_helper.py', 'prepare', '--cores', '0-11'])

	subprocess.run(['cset', 'shield', '--exec', path_to_python, 'benchmark.py'])	
	subprocess.run([path_to_python, 'benchmark_helper.py', 'restore'])

if __name__ == '__main__':
	main()
