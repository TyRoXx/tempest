from sys import argv, exit
from os import path, getcwd, chdir, makedirs
from subprocess import call
import errno

def useful_mkdir(dir):
	try:
		makedirs(dir)
	except OSError as exception:
		if exception.errno != errno.EEXIST:
			raise

def get_c_compiler(cpp_compiler):
	return str.replace(str.replace(cpp_compiler,
		"clang++", "clang"),
		"g++", "gcc")

def test_compiler(cmakelists, compiler_executable, output_parent):
	print "Testing compiler ", compiler_executable
	output = path.join(output_parent, path.basename(compiler_executable))
	useful_mkdir(output)
	chdir(output)

	if 0 != call(["cmake", cmakelists,
	              "-DCMAKE_CXX_COMPILER=" + compiler_executable,
	              "-DCMAKE_C_COMPILER=" + get_c_compiler(compiler_executable),
	              "-DCMAKE_BUILD_TYPE=DEBUG"]):
		print compiler_executable, ": CMake failed"
		return

	if 0 != call(["make"]):
		print compiler_executable, ": make failed"
		return

	if 0 != call(["./test/test"]):
		print compiler_executable, ": test failed"
		return

def main(arguments):
	if len(arguments) < 1:
		print "first argument: path containing CMakeLists.txt"
		return 1
	cmakelists = path.abspath(arguments[0])
	output_parent = getcwd()
	for compiler in arguments[1:]:
		test_compiler(cmakelists, compiler, output_parent)
	return 0

if __name__ == "__main__":
	exit(main(argv[1:]))
