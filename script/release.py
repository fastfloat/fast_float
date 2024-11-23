#!/usr/bin/env python3
########################################################################
# Generates a new release.
########################################################################
import sys
import re
import subprocess
import io
import os
import fileinput

if sys.version_info < (3, 0):
    sys.stdout.write("Sorry, requires Python 3.x or better\n")
    sys.exit(1)


def colored(r, g, b, text):
    return f"\033[38;2;{r};{g};{b}m{text} \033[38;2;255;255;255m"


def extractnumbers(s):
    return tuple(map(int, re.findall(r"(\d+)\.(\d+)\.(\d+)", str(s))[0]))


def toversionstring(major, minor, rev):
    return f"{major}.{minor}.{rev}"


print("Calling git rev-parse --abbrev-ref HEAD")
pipe = subprocess.Popen(
    ["git", "rev-parse", "--abbrev-ref", "HEAD"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
branchresult = pipe.communicate()[0].decode().strip()

if branchresult != "main":
    print(
        colored(
            255,
            0,
            0,
            f"We recommend that you release on main, you are on '{branchresult}'",
        )
    )

ret = subprocess.call(["git", "remote", "update"])

if ret != 0:
    sys.exit(ret)
print("Calling git log HEAD.. --oneline")
pipe = subprocess.Popen(
    ["git", "log", "HEAD..", "--oneline"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
uptodateresult = pipe.communicate()[0].decode().strip()

if len(uptodateresult) != 0:
    print(uptodateresult)
    sys.exit(-1)

pipe = subprocess.Popen(
    ["git", "rev-parse", "--show-toplevel"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
maindir = pipe.communicate()[0].decode().strip()
scriptlocation = os.path.dirname(os.path.abspath(__file__))

print(f"repository: {maindir}")

pipe = subprocess.Popen(
    ["git", "describe", "--abbrev=0", "--tags"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
versionresult = pipe.communicate()[0].decode().strip()

print(f"last version: {versionresult}")
try:
    currentv = extractnumbers(versionresult)
except:
    currentv = [0, 0, 0]
if len(sys.argv) != 2:
    nextv = (currentv[0], currentv[1], currentv[2] + 1)
    print(f"please specify version number, e.g. {toversionstring(*nextv)}")
    sys.exit(-1)
try:
    newversion = extractnumbers(sys.argv[1])
    print(newversion)
except:
    print(f"can't parse version number {sys.argv[1]}")
    sys.exit(-1)

print("checking that new version is valid")
if newversion[0] != currentv[0]:
    assert newversion[0] == currentv[0] + 1
    assert newversion[1] == 0
    assert newversion[2] == 0
elif newversion[1] != currentv[1]:
    assert newversion[1] == currentv[1] + 1
    assert newversion[2] == 0
else:
    assert newversion[2] == currentv[2] + 1

atleastminor = (currentv[0] != newversion[0]) or (currentv[1] != newversion[1])

newmajorversionstring = str(newversion[0])
newminorversionstring = str(newversion[1])
newpatchversionstring = str(newversion[2])
newversionstring = f"{newversion[0]}.{newversion[1]}.{newversion[2]}"
cmakefile = f"{maindir}{os.sep}CMakeLists.txt"

for line in fileinput.input(cmakefile, inplace=1, backup=".bak"):
    line = re.sub(
        r"project\(fast_float VERSION \d+\.\d+\.\d+ LANGUAGES CXX\)",
        f"project(fast_float VERSION {newversionstring} LANGUAGES CXX)",
        line.rstrip(),
    )
    print(line)

print(f"modified {cmakefile}, a backup was made")

versionfilerel = f"{os.sep}include{os.sep}fast_float{os.sep}float_common.h"
versionfile = f"{maindir}{versionfilerel}"

for line in fileinput.input(versionfile, inplace=1, backup=".bak"):
    line = re.sub(
        r"#define FASTFLOAT_VERSION_MAJOR \d+",
        f"#define FASTFLOAT_VERSION_MAJOR {newmajorversionstring}",
        line.rstrip(),
    )
    line = re.sub(
        r"#define FASTFLOAT_VERSION_MINOR \d+",
        f"#define FASTFLOAT_VERSION_MINOR {newminorversionstring}",
        line.rstrip(),
    )
    line = re.sub(
        r"#define FASTFLOAT_VERSION_PATCH \d+",
        f"#define FASTFLOAT_VERSION_PATCH {newpatchversionstring}",
        line.rstrip(),
    )
    print(line)

print(f"{versionfile} modified")

readmefile = f"{maindir}{os.sep}README.md"

for line in fileinput.input(readmefile, inplace=1, backup=".bak"):
    line = re.sub(
        r"https://github.com/fastfloat/fast_float/releases/download/v(\d+\.\d+\.\d+)/fast_float.h",
        f"https://github.com/fastfloat/fast_float/releases/download/v{newversionstring}/fast_float.h",
        line.rstrip(),
    )
    line = re.sub(
        r"GIT_TAG tags/v(\d+\.\d+\.\d+)",
        f"GIT_TAG tags/v{newversionstring}",
        line.rstrip(),
    )
    line = re.sub(
        r"GIT_TAG v(\d+\.\d+\.\d+)\)", f"GIT_TAG v{newversionstring})", line.rstrip()
    )
    print(line)

print(f"modified {readmefile}, a backup was made")

print("running amalgamate.py")
with open(f"{maindir}{os.sep}fast_float.h", "w") as outfile:
    cp = subprocess.run(
        [f"python3", f"{maindir}{os.sep}script{os.sep}amalgamate.py"], stdout=outfile
    )

if cp.returncode != 0:
    print("Failed to run amalgamate")
else:
    print("amalgamate.py ran successfully")
    print(f"You should upload {maindir}{os.sep}fast_float.h")

print("Please run the tests before issuing a release.\n")
print(
    f'to issue release, enter\n git commit -a && git push && git tag -a v{toversionstring(*newversion)} -m "version {toversionstring(*newversion)}" && git push --tags\n'
)
