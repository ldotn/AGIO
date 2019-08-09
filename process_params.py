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

    ('SpeciesStagnancyChances', [1, 3, 7]),
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
config_species = {}
max_scount = 0
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
                    try:
                        config_species[idx].append((str(tag),avg_fit))
                    except:
                        config_species[idx] = [(str(tag),avg_fit)]
                species_counts.append(len(species))
                max_scount = max(max_scount, len(species))
    except:
        print("Think I'm missing some configs...")

best_fit = {}
for tag,vals in results.items():
    best_fit[tag] = max(vals)[0] # Max on a list of tuples takes the tuple with the highest first value

fitness_results = [None for _ in combs]
for idx in range(len(combs)):
    avgf = 0
    species = []
    try:
        species = config_species[idx]
    except:
        continue
    for tag,avg_fit in species:
        avgf += avg_fit / best_fit[tag]
    avgf /= len(species)
    fitness_results[idx]= (avgf,species_counts[idx], idx)

# From the 20 best, sort by species count and keep the top 3
#fvals = sorted(fitness_results, reverse=True)[:20]
#fvals = sorted(fvals, reverse=True, key=lambda x : x[1])[:3]
#print(fvals)
print(max_scount)
fvals = []
for entry in fitness_results:
    if entry is None:
        continue
    avgf, sc, idx = entry
    fvals.append((abs(avgf - 1)/1 + abs(sc - max_scount)/max_scount, idx))
fvals = sorted(fvals)[:3]

# Show the parameters of the selected params
for fval, idx in fvals:
    f, sc,_ = fitness_results[idx]
    print('({0}, {1} avg norm fitness,{2} species found) -> '.format(fval,f,sc) + str(combs[idx]))

# Plot
plot_x, plot_y = [],[]
for entry in fitness_results:
    if entry is None:
        continue
    avgf, sc, _ = entry
    plot_x.append(avgf)
    plot_y.append(sc)

plt.ylabel('Cantidad de especies')
plt.xlabel('Fitness Normalizada Promedio')
plt.scatter(plot_x,plot_y, s=5)
plt.show()
