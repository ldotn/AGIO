import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
import math
from scipy.optimize import differential_evolution

plt.ion()
board_bounds = np.array([49,49])
board = np.zeros((board_bounds[0]+1,board_bounds[1]+1),dtype=int)
cell_values = [4,-2,-10]
def build_init_board():
    for x,y in np.ndindex(board.shape):
        w0 = 1.0/(0.1*((x-40.0)**2 + (y-10.0)**2) + 1.0)
        w1 = 1.0/(0.1*((x-10.0)**2 + (y-35.0)**2) + 1.0)
        w2 = 1.0/(0.5*((x-27.0)**2 + (y-32.0)**2) + 1.0)

        a0 = -10.5
        a1 = -10.1
        a2 = 3
        
        v = a0*w0 + a1*w1 + a2*w2

        if v > 0.5:
            board[x,y] = 0
        elif v > -0.5 and v <= 0.5:
            board[x,y] = 1
        else:
            board[x,y] = 2
build_init_board()


# Un problema con usar una distribucion normal es que eso asume que para cada variable de entrada y cada accion solo hay un "pico" de interes, cuando esto no es verdad
#   Es bastante posible que varios posibles valores de una variable de entrada correspondan con una alta probabilidad de hacer la accion
#   Esto se puede solucionar teniendo varias normales, o si no discretizando, cuando estas trabajando con valores discretos no pasa
def GaussianP(MeanAndVariance,x):
    return 1.0/math.sqrt(2*math.pi*MeanAndVariance[1])*math.exp(-(x-MeanAndVariance[0])**2/(2*MeanAndVariance[1]))

class Organism:
    def __init__(self,pos,init_dna = True):
        self.pos = pos
        self.life = 100
        self.age = 0
        # 4 actions, 25 inputs, 3 cell types
        if init_dna:
            self.dna = np.random.uniform(0.0,1.0,(4,25,3)) 
        #self.dna /= np.sum(self.dna)

    def step(self):
        # Read board
        '''xl = [None,None,None,None]
        xl[0] = board[tuple(np.clip(self.pos + np.array([1,0]),0,board_bounds))]
        xl[1] = board[tuple(np.clip(self.pos + np.array([-1,0]),0,board_bounds))]
        xl[2] = board[tuple(np.clip(self.pos + np.array([0,1]),0,board_bounds))]
        xl[3] = board[tuple(np.clip(self.pos + np.array([0,-1]),0,board_bounds))]'''
        xl = np.zeros(25,dtype=int)
        current_i = 0
        for x in [-2,-1,0,1,2]:
            for y in [-2,-1,0,1,2]:
                xl[current_i] = board[tuple(np.clip(self.pos + np.array([x,y]),0,board_bounds))]
                current_i += 1

        # Compute probabilites of each action
        probs = np.array([1.0,1.0,1.0,1.0])

        for action_idx in range(self.dna.shape[0]):
            for input_idx in range(self.dna.shape[1]):
                probs[action_idx] *= self.dna[action_idx,input_idx,xl[input_idx]]
                #probs[pidx] *= GaussianP(self.dna[pidx,iidx],xl[iidx])

        # Normalize probabilities
        probs /= np.sum(probs)

        # Select action to do
        action = np.random.choice(4,p=probs)
        if action == 0:
            self.pos[0] += 1
        elif action == 1:
            self.pos[0] -= 1
        elif action == 2:
            self.pos[1] += 1
        else:
            self.pos[1] -= 1

        self.pos = np.clip(self.pos,0,board_bounds)
        self.life += cell_values[board[self.pos[0],self.pos[1]]]
        self.age += 1

def org_sim(dna):
    org = Organism(np.array([20,20]),False)
    org.dna = np.reshape(dna,(4,25,3))
    for i in range(1000):
        if org.life <= 0:
            break
        org.step()
    return -org.age

def evo_callback(dna,convergence):
    print('>',end='')
    '''org = Organism(np.array([20,20]),False)
    org.dna = np.reshape(dna,(4,4,3))
    for i in range(10):
        if org.life <= 0:
            break
        org.step()
    print(org.age)'''
# popsize is a multiplier, real pop size is 4*25*3*popsize
result = differential_evolution(org_sim,[(0,1) for _ in range(4*25*3)],popsize=1,maxiter=200,callback=evo_callback)
print(result)

while True:
    best_org = Organism(np.array([20,20]),False)
    best_org.dna = np.reshape(result.x,(4,25,3))
    for i in range(1000):
        if best_org.life <= 0:
            break

        plt.clf()
        plt.imshow([[cell_values[c] for c in l] for l in board],cmap = plt.get_cmap('gist_gray'))
        plt.plot([best_org.pos[0]],[best_org.pos[1]],'ro')
        plt.pause(0.01)

        best_org.step()
        # Random changes in the enviroment
        if i % 2 == 0:
            new_cidx = (np.random.choice(board_bounds[0]),np.random.choice(board_bounds[1]))
            board[new_cidx] = (board[new_cidx] + 1) % 3

    print(best_org.age)
    input('')
'''
population_size = 200
population = [Organism(np.array([20,20])) for _ in range(population_size)]

# Do evolution
for g in range(1000):
    print('Generation ' + str(g))
    build_init_board()

    # Simulate population
    for i in range(1000):
        all_dead = True
        for org in population:
            if org.life > 0:
                all_dead = False
                org.step()

        # Random changes in the enviroment
        #if i % 2 == 0:
           # new_cidx = (np.random.choice(board_bounds[0]),np.random.choice(board_bounds[1]))
            #board[new_cidx] = (board[new_cidx] + 1) % 3

        if all_dead:
            break

    print(np.mean(np.array([org.age for org in population])))

    # Find best and show it
    if g % 100 == 0:
        population = sorted(population,key=lambda org : -org.age)
        population[0].life = 100
        population[0].pos = np.array([20,20])
       
        population[0].age = 0
        for i in range(500):
            if population[0].life <= 0:
                break

            plt.clf()
            plt.imshow([[cell_values[c] for c in l] for l in board],cmap = plt.get_cmap('gist_gray'))
            plt.plot([population[0].pos[0]],[population[0].pos[1]],'ro')
            plt.pause(0.01)

            population[0].step()

            # Random changes in the enviroment
            #if i % 2 == 0:
                #new_cidx = (np.random.choice(board_bounds[0]),np.random.choice(board_bounds[1]))
                #board[new_cidx] = (board[new_cidx] + 1) % 3

    # Generate childs, replacing the worst 30% of the population
    # Parents are selected randomly with an exponential distribution
    new_pop = population
    for idx in range(int(0.3*len(population))):
        # Some childs are random mutants
        if np.random.uniform() < 0.1:
            new_pop[-(idx + 1)] = Organism(np.array([20,20]))
        else:
            parents_indices = [int(x) for x in np.random.exponential(10,2)]
            p0 = population[parents_indices[0]]
            p1 = population[parents_indices[1]]

            while parents_indices[0] == parents_indices[1]: # Both parents can't be the same organism
                parents_indices = [int(x) for x in np.random.exponential(10,2)]
                p0 = population[parents_indices[0]]
                p1 = population[parents_indices[1]]
            child = Organism(np.array([20,20]))
            lerp_f = np.random.uniform()
            child.dna = p0.dna * lerp_f + (1.0 - lerp_f)*p1.dna
            #child.dna = (p0.dna*p0.age + p1.dna*p1.age) / (p0.age + p1.age)
            #child.dna /= np.sum(child.dna)
            new_pop[-(idx + 1)] = child

    # Swap population
    population = new_pop

    # Reset organisms 
    for idx in range(len(population)):
        population[idx].life = 100
        population[idx].pos = np.array([20,20])
        population[idx].age = 0
'''