import os
import itertools
import subprocess
import shutil
import sys
import configparser
import math
import pickle
import json
import statistics

#   Parameters ranges
ranges = [
    ('MinIndividualsPerSpecies', [5, 10, 50, 100]),
    ('MinSpeciesAge', [1, 10, 50, 100]),
    ('ProgressMetricsIndividuals', [1, 5, 40]), # Cant be more than MinIndividualsPerSpecies, starts at 50
    ('ProgressMetricsFalloff', [0.001, 0.025, 0.1, 0.9]),
    ('ProgressThreshold', [0.0001, 0.005, 0.1, 0.9]),
    ('SpeciesStagnancyChances', [1, 10, 50, 100]),
    ('MorphologyTries', [1, 10, 50, 100]),
]

flat_ranges = []
for name, values in ranges:
    for v in values:
        flat_ranges.append((name,v))

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
'''
print('Processing results')
results = {}
for name,value in flat_ranges:
    species = parse_file('parametric_config/results/sensibility/{0}_{1}.txt'.format(name,value))
    for tag,avg_fit in species:
        try:
            results[str(tag)].append((avg_fit,(name,value)))
        except:
            results[str(tag)] = [(avg_fit,(name,value))]

# Sort by fitness and report best
for tag, vals in results.items():
    results[tag] = sorted(vals,reverse=True)
    print("{0} | {1}".format(tag,vals[0]))

with open('parametric_config/results/sensibility.json','w') as fp:
    json.dump(results,fp,indent='\t')
'''

eval_repeats = 8

# The idea of the sensibility test is to see if the parameter affects or not the results
for name, values in ranges:
    species_nums = []
    results = {}
    for v in values:
        avg_num_species = 0
        tmp_results = {}
        for eval_idx in range(eval_repeats):
            species = parse_file('parametric_config/results/sensibility/{0}_{1}_{2}.txt'.format(name,v,eval_idx))
            for tag,avg_fit in species:
                try:
                    tmp_results[str(tag)].append(avg_fit)
                except:
                    tmp_results[str(tag)] = [avg_fit]
            avg_num_species += len(species)
        for key,repeats_vals in tmp_results.items():
            try:
                results[key].append(statistics.mean(repeats_vals))
            except:
                results[key] = [statistics.mean(repeats_vals)]
        avg_num_species /= eval_repeats
        species_nums.append(avg_num_species)

    '''fit_cv = 0
    fit_cv_acc = 0
    for _, fit_vals in results.items():
        if len(fit_vals) > 3 :
            fit_cv += statistics.pstdev(fit_vals) / statistics.mean(fit_vals)
            fit_cv_acc += 1
    fit_cv /= fit_cv_acc
    print(fit_cv_acc)
    snum_cv = statistics.pstdev(species_nums) / statistics.mean(species_nums)
    print((name, 100*fit_cv, 100*snum_cv))'''
    print(name)
    for _, fit_vals in results.items():
        if statistics.mean(fit_vals) <= 0:
            continue
        fit_cv = statistics.pstdev(fit_vals) / statistics.mean(fit_vals)
        print('{0:3.2f},'.format(100*fit_cv),end='')
        #print('({0},{1}) '.format(statistics.pstdev(fit_vals),statistics.mean(fit_vals)),end=',')
    print('\n')