import csv
from scipy.stats import shapiro, normaltest, norm
import numpy as np

species_data = {}
with open('bin/baseline.csv','r') as csvfile:
    freader = csv.reader(csvfile)
    for sid,data in enumerate(freader):
        data = [float(x) for x in data[:-1]]
        stat, p = shapiro(data)

        # interpret
        alpha = 0.05
        if p > alpha:
            print('Sample looks Gaussian (fail to reject H0)')
        else:
            print('Sample does not look Gaussian (reject H0)')
        stat, p = normaltest(data)

        # interpret
        alpha = 0.05
        if p > alpha:
            print('Sample looks Gaussian (fail to reject H0)')
        else:
            print('Sample does not look Gaussian (reject H0)')
        print(np.mean(data))
        print(np.std(data))
        species_data[sid] = (np.mean(data),np.std(data))
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
            #print(r, end=' ')
            if r >= 2:
                ppc = 100*(norm.cdf(r) - norm.cdf(-r))
                print("Species {0} depends on {1} [outside of {3:.2f}% of the expected values]".format(sid, second_id, r, ppc))
        #print('')

