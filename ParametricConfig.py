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

    ('ProgressMetricsFalloff', [0.001, 0.025, 0.1]),
    ('MinIndividualsPerSpecies', [25, 50, 75]),
    ('ProgressThreshold', [0.001, 0.005, 0.02])
]
    #('ProgressMetricsIndividuals', [5, 10, 20]),
'''
ranges = [
    ('PopSizeMultiplier', [30]),
    ('GenerationsCount', [400]),
    ('MaxSimulationSteps', [300]),

    ('MinSpeciesAge', [10, 15, 20, 25]),
    ('MinIndividualsPerSpecies', [15, 20 , 25]),
    ('ProgressThreshold', [0.0005, 0.001, 0.002])
]
'''
combs = []
def all_comb(ranges_list, final_list):
    if ranges_list == []:
        combs.append(final_list)
    else:
        name, values = ranges_list[0]
        for v in values:
            all_comb(ranges_list[1:], final_list + [(name,v)])

all_comb(ranges, [])

# I assume that this file is in the root of the repo
root = os.path.dirname(os.path.abspath(__file__))
root += '/'

# Number of workers to use
workers_num = int(sys.argv[1])

# Clean from previous runs
try:
    shutil.rmtree('parametric_config/pass1', ignore_errors=True)
except FileNotFoundError:
    pass

# Create folders
os.mkdir('parametric_config/pass1')
os.chdir('parametric_config/pass1')
os.mkdir('results')
for i in range(workers_num):
    os.mkdir(str(i))

    shutil.copytree('../../bin',str(i) + '/bin')
    shutil.copytree('../../assets',str(i) + '/assets')
    shutil.copytree('../../cfg',str(i) + '/cfg')
os.chdir('../..')

# Store the parameters to know what each index corresponds to
with open('parametric_config/pass1/parameters.txt','w') as fp:
    for params in combs:
        fp.write(str(params) + '\n')

print('Running parametric config')

#   Open base config file
cfg = configparser.ConfigParser(inline_comment_prefixes='#')
cfg.read('src/Tests/DiversityPlot/Config.cfg')

eval_repeats = 3

for batch_idx, batch in enumerate(batches(combs,workers_num)):
    for idx, params in enumerate(batch):
        for name,value in params:
            cfg.set("AGIO", name, str(value)) # no need to store the old value as you change all the values every time
        with open("parametric_config/pass1/{0}/cfg/main.ini".format(idx),'w') as fp:
            cfg.write(fp)

    # Do it several times
    for eval_idx in range(eval_repeats):
        for world_idx in range(5):
            print('Doing evaluation {0} of batch {1} for world {2}'.format(eval_idx+1,batch_idx+1,world_idx+1))
            processes = [subprocess.Popen(root + "parametric_config/pass1/{0}/bin/diversity_plot.exe ../cfg/main.ini ../../results/{1}_{2}_{3}.txt ../assets/worlds/world{3}.txt ".format(n,len(batch) * batch_idx + n,eval_idx,world_idx) , shell=True, cwd=root + "parametric_config/pass1/{0}/bin/".format(n)) for n,_ in enumerate(batch)]
            for p in processes: 
                p.wait()

    print('Batch {0} of {1} done'.format(batch_idx + 1,math.ceil(len(combs) / workers_num)))

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