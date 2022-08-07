import os
from pathlib import Path
from datetime import datetime


def longest_line(vec: list) -> int:
    out = 0
    for j in vec:
        if len(j) > out:
            out = len(j)
    return out


def stars(size: int) -> str:
    o = "// " + xof(size, "*")
    return o


def xof(size: int, x) -> str:
    o = ""
    for _ in range(0, size):
        o += x
    return o


def boxed(s: str) -> str:
    length = longest_line(s.split("\n"))
    out = stars(length + 4) + "\n"

    for ss in s.split("\n"):
        out += "// * " + ss + xof(length - len(ss), " ") + " *\n"

    out += stars(length + 4) + "\n"
    return out


def find_files(folder: str, depth: int = 4) -> list:
    if depth == 0:
        return []
    out = []
    for item in os.listdir(folder):
        p = folder + "/" + item
        if os.path.isfile(p):
            out.append(p)
        elif os.path.isdir(p):
            out += find_files(p, depth - 1)

    return out


path = "src"
outfile = "src/task_manager.hpp"
exclude = ["main.cpp"]

files = find_files(path)

headers = [x for x in files if x.endswith(".h")]
sources = [x for x in files if x.endswith(".cpp") if "main.cpp" not in x]

includes = ""
h_and_s = ""

for i in headers + sources:
    h_and_s += "\n" + boxed("Start of " + i) + "\n"
    with open(i, "r") as file:
        for line in file:
            if "#include <" in line and line not in includes:
                includes += line
            elif "#include \"" in line:
                continue
            else:
                h_and_s += line

am = boxed("\nAn amalgamation of the task_manager library\nBy Christian\n"
           + str(len(headers)) + " .h\n"
           + str(len(sources)) + " .cpp\n") + "\n" + includes + "\n" + h_and_s
mode = "x"

if Path(outfile).is_file():
    mode = "w"

with open(outfile, mode) as file:
    file.write(am)

now = datetime.now()
print("done: ", now.strftime("%b/%m/%Y %H:%M:%S"))
