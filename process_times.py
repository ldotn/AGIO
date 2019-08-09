import csv
import numpy as np
import scipy.stats as stats

def process(name):
    print(name)
    times = []
    with open('bin/' + name + '.csv') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            for column in row:
                try:
                    times.append(float(column))
                except:
                    pass

    avg = np.mean(times)
    dev = np.std(times)

    print(avg)
    print(dev)

process('decision_times')
process('total_times')
process('evo_time')