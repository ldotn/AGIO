import csv
from scipy.stats import shapiro, normaltest, norm, ks_2samp
import numpy as np

species_data = {}
species_types = {}

baseline = {}
species_count = 0
with open('bin/baseline.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        baseline[sid] = [float(x) for x in data[:-1]]
species_count = len(baseline)

node_descs = []
with open('bin/species_ref.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        species_types[sid] = data[1]

        if(species_types[sid] == 'Omnivore'):
            node_descs.append('{0}[shape="diamond",label=""];'.format(sid))
        elif(species_types[sid] == 'Carnivore'):
            node_descs.append('{0}[shape="square",label=""];'.format(sid))
        else:
            node_descs.append('{0}[label=""];'.format(sid))   

relations = []
relevant_species = set()

with open('bin/interrelations.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    dataIter = iter(freader)
    for sidB in range(species_count):
        for sidA in range(species_count):
            data = next(dataIter)[:-1]

            if sidA == sidB:
                continue

            data = [float(x) for x in data[:-1]]

            testP = ks_2samp(data, baseline[sidA]).pvalue
            
            if testP < 0.05:
                # The data are not from the same distribution so assume the species are related
                relevant_species.add(sidA)
                relevant_species.add(sidB)

                # TODO : Optimize! I'm computing the mean for the baseline a bunch of times
                mean_fA = np.mean(baseline[sidA])
                mean_fAB = np.mean(data)
                r_ppc = 100*(mean_fAB - mean_fA) / mean_fA

                print("{0} & {1} & {2} & {3} & {4:.2f}\\%\\\\".format(sidA,species_types[sidA], sidB, species_types[sidB], r_ppc))
                relations.append((sidA, sidB,  mean_fAB > mean_fA))

print('')
'''
relations = []
relevant_species = set()
node_descs = []
with open('bin/interrelations.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        data = [float(x) for x in data[:-1]]
        mean, dev, p_shapiro = species_data[sid]

        if(species_types[sid] == 'Omnívoro'):
            node_descs.append('{0}[shape="diamond",label=""];'.format(sid))
        elif(species_types[sid] == 'Carnívoro'):
            node_descs.append('{0}[shape="square",label=""];'.format(sid))
        else:
            node_descs.append('{0}[label=""];'.format(sid))   

        if p_shapiro < 0.05:
            continue
        
        for second_id, val in enumerate(data):
            if sid == second_id:
                continue
            r = abs(val - mean) / dev
            r_ppc = 100*(val - mean) / mean

            if r >= 2:
                relevant_species.add(sid)
                relevant_species.add(second_id)

                prob = 1 - (norm.cdf(r) - norm.cdf(-r))
                relations.append((sid, second_id, r_ppc > 0))
                print("{0} & {1} & {2} & {3} & {4:.2f} & {5:.4f} & {6:.2f}\\%\\\\".format(sid,species_types[sid], second_id, species_types[second_id],r, prob, r_ppc))

'''
print('')
print('digraph relations {')
for sid, desc in enumerate(node_descs):
    if sid in relevant_species:
        print(desc)
for x,y,increased in relations:
    if increased:
        print("{0}->{1}".format(x,y))
    else:
        print('{0}->{1}[style="dashed"]'.format(x,y))

print('}')