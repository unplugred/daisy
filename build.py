import os
import sys
import shlex

codes = {
	"plugin": [],
	"run": ["usb","jtag","no"]
}

if sys.platform == "darwin":
	system = "mac"
	import pty
elif sys.platform == "win32" or sys.platform == "cygwin":
	system = "win"
	import subprocess
else:
	system = "linux"
	import pty

plugins = [{}]
for i in range(len(plugins)):
	codes["plugin"].append(plugins[i]["name"])

def fuzzy_match(term,data):
	lower = str(term).replace(' ','').lower()
	if lower != "":
		for result in data:
			if result.replace(' ','').lower().startswith(lower):
				return result

	return None

def get_plugin(term):
	lower = str(term).replace(' ','').lower()
	if lower != "":
		for plugin in plugins:
			if plugin["name"].replace(' ','').lower().startswith(lower):
				return plugin

	error("Unknown plugin: "+term)


def debug(string):
	print('\033[1m'+string+'\033[0m')

def alert(string):
	print('\033[1m\033[93m'+string+'\033[0m')

def error(string, exit_code=1):
	print('\033[1m\033[91m'+string+'\033[0m')
	#sys.exit(exit_code)
	sys.exit(1)

def run_command(cmd,ignore_errors=False):
	debug("RUNNING COMMAND: "+cmd)

	if system == "win":
		os.environ['SYSTEMD_COLORS'] = '1'
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
		while process.poll() == None:
			print(process.stdout.readline().decode("UTF-8"),end='')
		return_code = process.returncode
	else:
		return_code = pty.spawn(shlex.split(cmd), lambda fd: os.read(fd, 1024))

	if return_code != 0:
		if ignore_errors:
			alert("Exited with return code "+str(return_code))
		else:
			error("Exited with return code "+str(return_code)+", exiting...",return_code)

def join(arr):
	return '/'.join(arr)

def ls(path):
	debug("LISTING FILES IN "+path)
	for file in os.listdir(path):
		debug(file)

def cd(directory=None):
	if directory == None:
		debug("CD .")
		os.chdir(os.path.dirname(os.path.abspath(__file__)))
	else:
		debug("CD "+directory)
		currentdir = os.path.dirname(os.path.abspath(__file__))
		if currentdir.endswith("/") or currentdir.endswith("\\"):
			currentdir = currentdir[:-1]
		os.chdir(join([currentdir,directory]))

cd()


def configure():
	debug("UPDATING DEPENDENCIES")
	run_command("git submodule update --init --recursive")

	debug("BUILDING LIBDAISY")
	cd("libDaisy")
	run_command("make -s clean")
	run_command("make -j -s")

	debug("BUILDING DAISYSP")
	cd("DaisySP")
	run_command("make -s clean")
	run_command("make -j -s")

	cd()

def build(plugin):
	debug("BUILDING "+plugin.upper())
	cd(get_plugin(plugin)["path"])
	run_command("make -s clean")
	run_command("make -s")
	cd()

def run_plugin(plugin,run):
	if run not in codes["run"] or run == "no":
		return
	debug("RUNNING "+plugin.upper()+" ON "+run.upper())
	cd(get_plugin(plugin)["path"])
	if run == "usb":
		run_command("make program-dfu")
	else:
		run_command("make program")
	cd()

def run_program(string):
	if string.strip() == "":
		error("You must specify arguments!")
	args = shlex.split(string)

	if "configure".startswith(args[0]) and ',' not in string and len(args) <= 1:
		configure()
		return

	to_build = {}
	arguments = ["plugin","run"]
	for argument in arguments:
		to_build[argument] = []
	for i in range(len(args)):
		if i >= len(arguments):
			error("Unexpected number of arguments!")
		if i == 0 and "all".startswith(args[i]):
			for n in codes[arguments[i]]:
				to_build[arguments[i]].append(n)
			continue
		if i == 1 and ',' in args[i]:
			error("Argument "+arguments[i]+" can only have one value")
		sub_args = args[i].split(',')
		for n in range(len(sub_args)):
			match = fuzzy_match(sub_args[n],codes[arguments[i]])
			if match == None:
				error("Unknown "+arguments[i]+": "+sub_args[n])
			if match in to_build[arguments[i]]:
				error("Argument "+match+" used more than once!")
			to_build[arguments[i]].append(match)

	if len(to_build["run"]) == 0:
		to_build["run"].append("usb" if len(to_build["plugin"]) <= 1 else "no")

	for plugin in to_build["plugin"]:
		build(plugin)
		if to_build["run"][0] != "no":
			run_plugin(plugin,to_build["run"][0])

if __name__ == "__main__":
	run_program(' '.join(shlex.quote(s) for s in (sys.argv[1:])))
