import csv
from scipy.stats import shapiro, normaltest, norm
import numpy as np

species_data = {}
species_types = {}

with open('bin/species_ref.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        species_types[sid] = data[1]

with open('bin/baseline.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        data = [float(x) for x in data[:-1]]

        _, p_shapiro = shapiro(data)
        _, p_pearson_dagostino = normaltest(data)
        print('{0} & {1:.2f} & {2:.2f}\\\\'.format(sid,p_shapiro,p_pearson_dagostino))

        species_data[sid] = (np.mean(data),np.std(data))
        '''stat, p = shapiro(data)
        print(p)
        # interpret
        alpha = 0.05
        if p > alpha:
            print('Sample looks Gaussian (fail to reject H0)')
        else:
            print('Sample does not look Gaussian (reject H0)')
        stat, p = normaltest(data)
        print(p)

        # interpret
        alpha = 0.05
        if p > alpha:
            print('Sample looks Gaussian (fail to reject H0)')
        else:
            print('Sample does not look Gaussian (reject H0)')
        #print(np.mean(data))
        #print(np.std(data))
        
        species_data[sid] = (np.mean(data),np.std(data))
        print('')'''
print('')
with open('bin/interrelations.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        data = [float(x) for x in data[:-1]]
        mean, dev = species_data[sid]
        if mean == dev == 0:
            continue
        for second_id, val in enumerate(data):
            if sid == second_id:
                continue
            r = abs(val - mean) / dev
            r_ppc = 100*(val - mean) / mean
            #print(r, end=' ')
            if r >= 2:
                prob = 1 - (norm.cdf(r) - norm.cdf(-r))

                print("{0} & {1} & {2} & {3} & {4:.2f} & {5:.4f} & {6:.2f}\\%\\\\".format(sid,species_types[sid], second_id, species_types[second_id],r, prob, r_ppc))
