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

# I assume that this file is in the root of the repo
root = os.path.dirname(os.path.abspath(__file__))
root += '/'

# Number of workers to use
workers_num = int(sys.argv[1])

# Clean from previous runs
try:
    shutil.rmtree('parametric_config/sensibility', ignore_errors=True)
except FileNotFoundError:
    pass

# Create folders
os.mkdir('parametric_config/sensibility')
os.chdir('parametric_config/sensibility')
os.mkdir('results')
for i in range(workers_num):
    os.mkdir(str(i))

    shutil.copytree('../../bin',str(i) + '/bin')
    shutil.copytree('../../assets',str(i) + '/assets')
    shutil.copytree('../../cfg',str(i) + '/cfg')
os.chdir('../..')

# Sensibility analysis
print('Running sensibility analysis')

#   Open base config file
cfg = configparser.ConfigParser(inline_comment_prefixes='#')
cfg.read('src/Tests/DiversityPlot/Config.cfg')

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

eval_repeats = 8

for batch_idx, batch in enumerate(batches(flat_ranges,workers_num)):
    for idx, (name, value) in enumerate(batch):
        old_value = cfg.get("AGIO",name)
        cfg.set("AGIO", name, str(value))
        with open("parametric_config/sensibility/{0}/cfg/main.ini".format(idx),'w') as fp:
            cfg.write(fp)
        cfg.set("AGIO", name, old_value)

    # Do it several times
    for eval_idx in range(eval_repeats):
        print('Doing evaluation {0} of batch {1}'.format(eval_idx+1,batch_idx+1))
        processes = [subprocess.Popen(root + "parametric_config/sensibility/{0}/bin/diversity_plot.exe ../cfg/main.ini ../../results/{1}_{2}_{3}.txt ../assets/worlds/world0.txt ".format(n,name,value,eval_idx) , shell=True, cwd=root + "parametric_config/sensibility/{0}/bin/".format(n)) for n,(name,value) in enumerate(batch)]
        for p in processes: 
            p.wait()

    print('Batch {0} of {1} done'.format(batch_idx + 1,math.ceil(len(flat_ranges) / workers_num)))

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
    
    cv_vals = []
    for _, fit_vals in results.items():
        if statistics.mean(fit_vals) <= 0:
            continue
        fit_cv = statistics.pstdev(fit_vals) / statistics.mean(fit_vals)
        cv_vals.append(fit_cv)

    plt.hist(cv_vals, np.linspace(0,2,9), density=False, facecolor='b')
    plt.xlabel('Coeficiente de Variación')
    plt.ylabel('Cantidad')
    plt.axis([0,2, 0, 20])
    plt.yticks(np.arange(0, 21, step=5))
    #plt.title(clean_names[name])
    plt.show()
    snum_cv = statistics.pstdev(species_nums) / statistics.mean(species_nums)
    print(snum_cv)


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