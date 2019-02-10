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

    ('MinSpeciesAge', [10, 20, 40]),
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

eval_repeats = 1

for batch_idx, batch in enumerate(batches(combs,workers_num)):
    for idx, params in enumerate(batch):
        for name,value in params:
            cfg.set("AGIO", name, str(value)) # no need to store the old value as you change all the values every time
        with open("parametric_config/pass1/{0}/cfg/main.ini".format(idx),'w') as fp:
            cfg.write(fp)

    # Do it several times
    for eval_idx in range(eval_repeats):
        print('Doing evaluation {0} of batch {1}'.format(eval_idx+1,batch_idx+1))
        processes = [subprocess.Popen(root + "parametric_config/pass1/{0}/bin/diversity_plot.exe ../cfg/main.ini ../../results/{1}.txt ../assets/worlds/world0.txt ".format(n,batch_idx) , shell=True, cwd=root + "parametric_config/pass1/{0}/bin/".format(n)) for n,_ in enumerate(batch)]
        for p in processes: 
            p.wait()

    print('Batch {0} of {1} done'.format(batch_idx + 1,math.ceil(len(combs) / workers_num)))

#armar un mapa con las especies, y de que setting y que fitness se llego
# y despues pones cuantas especies llego ese setting y a cuantas veces tuvo la mejor fitness
results = {}
species_counts = []
for idx in range(len(combs)):
    species = parse_file('parametric_config/results/{0}.txt'.format(idx))
    for tag,avg_fit in species:
        try:
            results[str(tag)].append((avg_fit,idx)
        except:
            results[str(tag)] = [(avg_fit,idx]
    species_counts.append(len(species))

# Find on how many species each parameter is the best
bests_counts = [0] * len(combs)
for _,vals in results.items():
    _,idx = sorted(vals,reverse=True)[0]
    bests_counts[idx] += 1

for sc, bc in zip(species_counts, bests_counts)
    print(sc,bc)