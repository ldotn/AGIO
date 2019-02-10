import os
import itertools
import subprocess
import shutil
import sys
import configparser
import math
import json
import statistics
import numpy as np
import matplotlib.pyplot as plt

def batches(l, n):
    """Yield successive n-sized batches from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]

def parse_file(path : str):
    species = []
    with open(path,'r') as f:
        species_num = int(f.readline())
        for i in range(species_num):
            tag, avg_fit = tuple(str(f.readline()).split(':'))
            tag = [tuple(x.split(',')) for x in tag.split(' ')[0:-1]]
            avg_fit = float(avg_fit)
            species.append((tag,avg_fit))
    return species