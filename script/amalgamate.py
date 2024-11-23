# text parts
processed_files = {}

# authors
for filename in ["AUTHORS", "CONTRIBUTORS"]:
    with open(filename, encoding="utf8") as f:
        text = ""
        for line in f:
            if filename == "AUTHORS":
                text += "// fast_float by " + line
            if filename == "CONTRIBUTORS":
                text += "// with contributions from " + line
        processed_files[filename] = text + "//\n//\n"

# licenses
for filename in ["LICENSE-MIT", "LICENSE-APACHE", "LICENSE-BOOST"]:
    lines = []
    with open(filename, encoding="utf8") as f:
        lines = f.readlines()

    # Retrieve subset required for inclusion in source
    if filename == "LICENSE-APACHE":
        lines = ["   Copyright 2021 The fast_float authors\n", *lines[179:-1]]

    text = ""
    for line in lines:
        line = line.strip()
        if len(line):
            line = "    " + line
        text += "//" + line + "\n"
    processed_files[filename] = text

# code
for filename in [
    "constexpr_feature_detect.h",
    "float_common.h",
    "fast_float.h",
    "ascii_number.h",
    "fast_table.h",
    "decimal_to_binary.h",
    "bigint.h",
    "digit_comparison.h",
    "parse_number.h",
]:
    with open("include/fast_float/" + filename, encoding="utf8") as f:
        text = ""
        for line in f:
            if line.startswith('#include "'):
                continue
            text += line
        processed_files[filename] = "\n" + text

# command line
import argparse

parser = argparse.ArgumentParser(description="Amalgamate fast_float.")
parser.add_argument(
    "--license",
    default="TRIPLE",
    choices=["DUAL", "TRIPLE", "MIT", "APACHE", "BOOST"],
    help="choose license",
)
parser.add_argument("--output", default="", help="output file (stdout if none")

args = parser.parse_args()


def license_content(license_arg):
    result = []
    if license_arg == "TRIPLE":
        result += [
            "// Licensed under the Apache License, Version 2.0, or the\n",
            "// MIT License or the Boost License. This file may not be copied,\n",
            "// modified, or distributed except according to those terms.\n",
            "//\n",
        ]
    if license_arg == "DUAL":
        result += [
            "// Licensed under the Apache License, Version 2.0, or the\n",
            "// MIT License at your option. This file may not be copied,\n",
            "// modified, or distributed except according to those terms.\n",
            "//\n",
        ]

    if license_arg in ("DUAL", "TRIPLE", "MIT"):
        result.append("// MIT License Notice\n//\n")
        result.append(processed_files["LICENSE-MIT"])
        result.append("//\n")
    if license_arg in ("DUAL", "TRIPLE", "APACHE"):
        result.append("// Apache License (Version 2.0) Notice\n//\n")
        result.append(processed_files["LICENSE-APACHE"])
        result.append("//\n")
    if license_arg in ("TRIPLE", "BOOST"):
        result.append("// BOOST License Notice\n//\n")
        result.append(processed_files["LICENSE-BOOST"])
        result.append("//\n")

    return result


text = "".join(
    [
        processed_files["AUTHORS"],
        processed_files["CONTRIBUTORS"],
        *license_content(args.license),
        processed_files["constexpr_feature_detect.h"],
        processed_files["float_common.h"],
        processed_files["fast_float.h"],
        processed_files["ascii_number.h"],
        processed_files["fast_table.h"],
        processed_files["decimal_to_binary.h"],
        processed_files["bigint.h"],
        processed_files["digit_comparison.h"],
        processed_files["parse_number.h"],
    ]
)

if args.output:
    with open(args.output, "wt", encoding="utf8") as f:
        f.write(text)
else:
    print(text)
