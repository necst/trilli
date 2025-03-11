# MIT License

# Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import argparse
import os


def main():
    parser = argparse.ArgumentParser(description='Generator for the hw-link configuration file')
    parser.add_argument("-in", "--input-template", nargs='?', help='cfg template', default='./xclbin_overlay.template.cfg', type=str)
    parser.add_argument("-out", "--output-cfg", nargs='?', help='generated cfg', default='./xclbin_overlay.cfg', type=str)
    parser.add_argument("-intpe", "--interpolator_pe_number", nargs='?', help='number of AIE interpolator PEs, default 1', default='1', type=int)
    parser.add_argument("-t", "--task", nargs='?', help='task to be built', choices=["TX","STEP"], default='STEP')

    args = parser.parse_args()

    interpolator_pe_number = args.interpolator_pe_number

    config_file_in = open(args.input_template, "r")
    config_file_out = open(args.output_cfg, "w")
    task = args.task
    print("task: ", task)
    if interpolator_pe_number < 1:
        print("Error: interpolator_pe_number must be at least 1")
        exit(1)

    # read all lines from the input file
    lines = config_file_in.readlines()
    
    for (idx, line) in enumerate(lines):
        if line.find("${") != -1:
            var_name = line.split("${")[1].split("}")[0]

            if var_name == "AIE_TO_WRITER":
                lines[idx] = ""
                for i in range(1, interpolator_pe_number + 1):
                    if task == "STEP":
                        lines[idx] += "stream_connect = ai_engine_0.result_" + str(i) + ":setup_mi_0.pixels_in_" + str(i) + " # only for INT_PE>=" + str(i) + "  (automatically placed by config_generator.py)\n"
                    elif task == "TX":
                        lines[idx] += "stream_connect = ai_engine_0.result_" + str(i) + ":writer_0.pixels_in_" + str(i) + " # only for INT_PE>=" + str(i) + "  (automatically placed by config_generator.py)\n"
            elif var_name == "SINT_TO_AIE":
                lines[idx] = ""
                for i in range(1, interpolator_pe_number + 1):
                    lines[idx] += "stream_connect = setup_interpolator_0.out_" + str(i) + ":ai_engine_0.p_ab_" + str(i) + " # only for INT_PE>=" + str(i) + "  (automatically placed by config_generator.py)\n"
                    lines[idx] += "stream_connect = setup_interpolator_1.out_" + str(i) + ":ai_engine_0.p_cd_" + str(i) + " # only for INT_PE>=" + str(i) + "  (automatically placed by config_generator.py)\n"
            else:
                print("Error: variable \"" + var_name + "\" not recognized")
                exit(1)
    
    # rewite all lines to the output file
    config_file_out.writelines(lines)

if __name__ == "__main__":
    main()