# spy reads a descriptive .spy file and plots this using matplotlib
# the spy format is as follows (based on MM format):
# | # information on matrix
# | # ...
# | Title
# | m n z procs
# | i_1 j_1 p_1
# | ...
# | i_z j_z p_z
#
# I think it would be cool if we would use full spectrum of colors, and sample uniformly
# using the tree:
#       1/2
#      /   \
#     1/4   3/4
#    / \    / \
# 1/8 3/8  5/8  7/8
# ..  ..   ..   ..
#
# i.e.
#
# |------------------------------------------------|
#      4    2    5       1    6       3      7
# 
# etc.

import argparse

import matplotlib
matplotlib.use('GTK3Cairo')   # generate postscript output by default

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import matplotlib.ticker as ticker

from math import log, ceil


marker_size = 0.8
marker_offset = 0.5 * (1 - marker_size)

parser = argparse.ArgumentParser(description=
    "This script reads one or multiple .spy file, and outputs"
    " a spy plot to screen or as a .pdf file")

parser.add_argument('spy_file', type=str, help=
    "The .spy file to use as input")

args = parser.parse_args()

def color_of_proc(proc):
    # ith element of has denumerator of 2^ceil(2log(p) + 1)
    #                    numerator is n - whats above, 1 + that
    level = ceil(log(proc + 1, 2))
    denumerator = 2**level
    numerator = 1 + (proc - (denumerator / 2))*2
    color = numerator / denumerator
    color -= 0.5
    if (color < 0.0):
        color += 1.0;
    return matplotlib.colors.hsv_to_rgb([color, 1, 1])

for i in range(0, 100):
    color_of_proc(i)

with open(args.spy_file, 'r') as fin:
    line = fin.readline()
    while (len(line) == 0 or line[0] == '#'):
        line = fin.readline()
    title = line

    m, n, nz = map(int, fin.readline().split(' '))

    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1, aspect='equal')

    # force integer labels
    ax.get_xaxis().set_major_locator(ticker.MaxNLocator(integer=True))
    ax.get_yaxis().set_major_locator(ticker.MaxNLocator(integer=True))

    # set the appropriate limits
    plt.xlim([1, n])
    plt.ylim([m, 1])

    for _ in range(0, nz):
        i, j, p = map(int, fin.readline().split(' '))
        ax.add_patch(patches.Rectangle((j + marker_offset,
            i + marker_offset),
            marker_size, marker_size, color=color_of_proc(p)))

    plt.show()
