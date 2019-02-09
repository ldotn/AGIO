import os
import itertools
import subprocess
import shutil
import sys
import configparser
import math

# I assume that this file is in the root of the repo
root = os.path.dirname(os.path.abspath(__file__))
root += '/'

# Number of workers to use
workers_num = int(sys.argv[1])

# Clean from previous runs
try:
    shutil.rmtree('parametric_config', ignore_errors=True)
except FileNotFoundError:
    pass

# Create folders
os.mkdir('parametric_config')
os.chdir('parametric_config')
os.mkdir('results')
os.mkdir('results/sensibility')
for i in range(workers_num):
    os.mkdir(str(i))

    shutil.copytree('../bin',str(i) + '/bin')
    shutil.copytree('../assets',str(i) + '/assets')
    shutil.copytree('../cfg',str(i) + '/cfg')

os.chdir('..')

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
    ('ProgressMetricsFalloff', [0.001, 0.025, 0.1, 1]),
    ('ProgressThreshold', [0.0001, 0.005, 0.1, 1]),
    ('SpeciesStagnancyChances', [1, 10, 50, 100]),
    ('MorphologyTries', [1, 10, 50, 100]),
]

flat_ranges = []
for name, values in ranges:
    for v in values:
        flat_ranges.append((name,v))

def batches(l, n):
    """Yield successive n-sized batches from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]

for batch_idx, batch in enumerate(batches(flat_ranges,workers_num)):
    for idx, (name, value) in enumerate(batch):
        cfg.set("AGIO",name,str(value))
        with open("parametric_config/{0}/cfg/main.ini".format(idx),'w') as fp:
            cfg.write(fp)

    processes = [subprocess.Popen(root + "parametric_config/{0}/bin/diversity_plot.exe ../cfg/main.ini ../../results/sensibility/{1}_{2}.txt ../assets/worlds/world0.txt ".format(n,name,value) , shell=True, cwd=root + "parametric_config/{0}/bin/".format(n)) for n,(name,value) in enumerate(batch)]
    for p in processes: 
        p.wait()

    print('Batch {0} of {1} done'.format(batch_idx + 1,math.ceil(len(flat_ranges) / workers_num)))