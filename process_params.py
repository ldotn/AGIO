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
from ParametricConfigUtils import *
import itertools


#   Parameters ranges
ranges = [
    ('PopSizeMultiplier', [10, 20, 30]),
    ('GenerationsCount', [100, 250, 400]),
    ('MaxSimulationSteps', [50, 150, 300]),

    ('ProgressMetricsIndividuals', [5, 10, 20]),
    ('MinIndividualsPerSpecies', [25, 50, 75]),
    ('ProgressThreshold', [0.001, 0.005, 0.02])
]

combs = []
def all_comb(ranges_list, final_list):
    if ranges_list == []:
        combs.append(final_list)
    else:
        name, values = ranges_list[0]
        for v in values:
            all_comb(ranges_list[1:], final_list + [(name,v)])

all_comb(ranges, [])

eval_repeats = 3

results = {}
species_counts = []
for idx in range(len(combs)):
    try:
        for eval_idx in range(eval_repeats):
            for world_idx in range(5):
                species = parse_file('parametric_config/pass1/results/{0}_{1}_{2}.txt'.format(idx,eval_idx,world_idx))
                for tag,avg_fit in species:
                    try:
                        results[str(tag)].append((avg_fit,idx))
                    except:
                        results[str(tag)] = [(avg_fit,idx)]
                species_counts.append(len(species))
    except:
        print("Think i'm missing some configs...")

# Find on how many species each parameter is the best
bests_counts = [0] * len(species_counts)
for _,vals in results.items():
    _,idx = sorted(vals,reverse=True)[0]
    bests_counts[idx] += 1

fvals = []
for idx,(sc, bc) in enumerate(zip(species_counts, bests_counts)):
    fvals.append((bc,sc,idx))
# Select top 5 by best counts and then sort by number of species and keep the best 3
fvals = sorted(fvals, reverse=True)[:5]
fvals = sorted(fvals, reverse=True, key=lambda x : x[1])[:3]
print(fvals)

# Show the parameters of the selected params
for bc,sc,idx in fvals:
    print('({0} times best,{1} species found) -> '.format(bc,sc) + str(combs[idx]))



