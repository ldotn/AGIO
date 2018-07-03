import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
import math
import copy
from scipy.ndimage.filters import gaussian_filter

population = []

plt.ion()
board_bounds = np.array([59,59],dtype=int)
board = np.zeros((board_bounds[0]+1,board_bounds[1]+1),dtype=int)
cell_values = [30,-0.5,-15]
cell_types = [i for i in range(len(cell_values))]
org_positions = np.zeros(board.shape)
org_density = np.zeros(board.shape)
org_types = np.zeros(board.shape)
food_count = np.zeros(board.shape)

def build_init_board():
    for x,y in np.ndindex(board.shape):
        if x == 0 or x == board_bounds[0] or y == board_bounds[1] or y == 0 or x == 1 or x == board_bounds[0] - 1 or y == board_bounds[1] - 1 or y == 1:
            board[x,y] = 2
            continue
        w0 = 1.0/(0.1*((x-40.0)**2 + (y-10.0)**2) + 1.0)
        w1 = 1.0/(0.1*((x-10.0)**2 + (y-35.0)**2) + 1.0)
        w2 = 1.0/(0.5*((x-(27.0))**2 + (y-(32.0 ))**2) + 1.0)
        w3 = 1.0/(0.5*((x-(4.0))**2 + (y-(5.0 ))**2) + 1.0)
        
        a0 = -10.5
        a1 = -10.1
        a2 = 10
        a3 = 8

        v = a0*w0 + a1*w1 + a2*w2 + a3*w3

        if v > 0.5:
            food_count[x,y] = 4 # Can eat 4 times the same food
            board[x,y] = 0
        elif v > -0.5 and v <= 0.5:
            board[x,y] = 1
        else:
            board[x,y] = 2
build_init_board()

# Using wrapper classes as a way to have pointers
class Node:
    def __init__(self,param_idx,value,branch_if_true,branch_if_false):
        self.param_idx = param_idx
        self.target_value = value
        self.branch_if_true = branch_if_true
        self.branch_if_false = branch_if_false

    def __str__(self):
        return ('( ' + str(self.param_idx) + ' = ' + str(self.target_value) + '->' + str(self.branch_if_true) + '|' + str(self.branch_if_false)).replace('\\','')

class Leaf:
    def __init__(self,action):
        self.action = action
    
    def __str__(self):
        return str(self.action)

base_starting_life = 100

class Organism:
    def __init__(self,init_dna = True):
        self.pos = np.zeros(2,dtype=int)
        self.pos[0] = int(np.random.choice(board_bounds[0]))
        self.pos[1] = int(np.random.choice(board_bounds[1]))
        self.life = base_starting_life
        self.age = 0
        self.accumulated_life = 0
        self.is_predator = np.random.uniform() < 0.5
        self.number_of_childs = 0

        if init_dna:
            # Using a binary decision tree
            # That way you always get an answer
            # Though you do have redundant checks
            # When going down a 'true' branch, you should never ask again for a parameter
            # Because you already know the correct value
            # This is only correct for mutually exclusive conditions
            def build_random_tree(level):
                if level == 0 or np.random.uniform() > 0.55:
                    return Leaf(np.random.choice(6)) # 6 actions
                else:
                    branches_map = {}
                    param = np.random.choice(9*2) # 9x2 inputs
                    value = np.random.choice(cell_types)

                    return Node(param,value, build_random_tree(level - 1),build_random_tree(level - 1))
            self.dna = build_random_tree(np.random.choice(20))

    def step(self):
        if self.life <= 0:
            return

        # Read board
        xl = np.zeros(9*2,dtype=int)
        current_i = 0
        for x in [-1,0,1]:
            for y in [-1,0,1]:
                xl[current_i] = board[tuple(np.clip(self.pos + np.array([x,y]),0,board_bounds))]
                current_i += 1
                # This is technically wrong, because i'm using different value ranges for the inputs
                # So you'll get trees that reference this cell to values that aren't possible
                # But just testing for now
                # UPDATE: Actually, now with the prey/predator the numbers are the same, so should be ok
                xl[current_i] = org_types[tuple(np.clip(self.pos + np.array([x,y]),0,board_bounds))]
                current_i += 1

        # Walk tree to find the action to do
        def tree_walk(tree):
            if type(tree) is Leaf:
                return tree.action
            else:
                if xl[tree.param_idx] == tree.target_value:
                    return tree_walk(tree.branch_if_true)
                else:
                    return tree_walk(tree.branch_if_false)

        action = tree_walk(self.dna)

        org_positions[self.pos[0],self.pos[1]] = 0
        if action is not None:
            # Don't allow two organisms to be on the same exact position
            if action == 0 and org_types[tuple(np.clip(self.pos + np.array([1,0]),0,board_bounds))] == 0:
                self.pos[0] += 1
            elif action == 1 and org_types[tuple(np.clip(self.pos + np.array([-1,0]),0,board_bounds))] == 0:
                self.pos[0] -= 1
            elif action == 2 and org_types[tuple(np.clip(self.pos + np.array([0,1]),0,board_bounds))] == 0:
                self.pos[1] += 1
            elif action == 3 and org_types[tuple(np.clip(self.pos + np.array([0,-1]),0,board_bounds))] == 0: 
                self.pos[1] -= 1
            elif action == 4: # Random move
                offset = np.random.choice([-1,0,1],2)
                if org_types[tuple(np.clip(self.pos + offset,0,board_bounds))] == 0:
                    self.pos += offset
            elif action == 5:
                None#idle

            #self.pos = np.clip(self.pos,0,board_bounds)
            # DRASTIC option : kill everyone that steps out of the board
            if self.pos[0] < 0 or self.pos[1] < 0 or self.pos[0] > board_bounds[0] or self.pos[1] > board_bounds[1]:
                self.life = -9999999
                self.pos = np.clip(self.pos,0,board_bounds)

        if self.life > 0:
            cell_type = board[self.pos[0],self.pos[1]]
            if self.is_predator:
                # Predators only get food by moving to the cell of other organisms
                # In doing so it also hurts them
                # Still affected by the rest of the cell types
                if cell_type != 0:
                    self.life += cell_values[cell_type]

                for idx,org in enumerate(population):
                    if np.linalg.norm(self.pos - org.pos,ord=np.inf) <= 1 and not org.is_predator:
                        self.life += 25
                        population[idx].life -= 50
            else:
                if cell_type == 0:
                    self.life += cell_values[0]# / (org_density[self.pos[0],self.pos[1]] + 1)
                else:
                    self.life += cell_values[cell_type]

            # Age makes the life descent faster
            self.life -= np.sqrt(self.age + 1)

            if self.life > 0:
                org_positions[self.pos[0],self.pos[1]] = self.life
                self.accumulated_life += self.life
                self.age += 1
        if self.life <= 0:
            self.life = 0
    
    def local_fitness(self):
        return (0.1*self.life + self.accumulated_life) / (0.5*self.number_of_childs + 1)

    def accept_partner(self,partner):
        # Predators can't mate with prey
        if self.is_predator != partner.is_predator:
            return False

        # Condition 1 : Want partners that aren't wounded
        if partner.life < np.random.exponential(0.01):
            return False

        # Condition 2 : Want partners that are not too young or too old
        #peak_age = 45 # Arbitrary, the 'best' value depends on the scene and actions
        #if np.abs(partner.age - peak_age) > np.abs(np.random.normal(peak_age,4.0)):
            #return False

        # Condition 3 : Want partners that had a high average life
        #if (partner.accumulated_life / partner.age) > np.random.exponential(0.05):
            #return False

        # Condition 4 : Don't want to have too many childrens
        if 1.0 / (0.5*self.number_of_childs + 1) < np.random.uniform():
            return False

        # All conditions passed, so the partner is accepted
        # That a partner is accepted doesn't directly means that there will be mating
        # It does means that organism will accept mating with the partner
        return True
    
    def cross(self,partner):
        child = Organism(init_dna=False)
        child.is_predator = self.is_predator
        #child.pos[0] = np.floor(np.random.uniform(min(self.pos[0],partner.pos[0]),max(self.pos[0],partner.pos[0])))
        #child.pos[1] = np.floor(np.random.uniform(min(self.pos[1],partner.pos[1]),max(self.pos[1],partner.pos[1])))
        cx = 0.5*(self.pos[0]+partner.pos[0])
        cy = 0.5*(self.pos[1]+partner.pos[1])
        child.pos[0] = np.floor(cx + np.random.uniform(-2.2))
        child.pos[1] = np.floor(cy + np.random.uniform(-2,2))
        while child.pos[0] != self.pos[0] and child.pos[0] != partner.pos[0] and child.pos[1] != self.pos[1] and child.pos[1] != partner.pos[1]:
            child.pos[0] = np.floor(cx + np.random.uniform(-2.2))
            child.pos[1] = np.floor(cy + np.random.uniform(-2,2))

        child.pos = np.clip(child.pos,0,board_bounds)
        child.dna = copy.deepcopy(self.dna)
        def tree_walk(child_dna,partner_dna):
            if type(child_dna) == type(partner_dna) == Node:
                if np.random.uniform() < 0.25:
                    if np.random.uniform() < 0.5:
                        child_dna.branch_if_false = copy.deepcopy(partner_dna.branch_if_false)
                    else:
                        child_dna.branch_if_true = copy.deepcopy(partner_dna.branch_if_true)
                else:
                    tree_walk(child_dna.branch_if_true,partner_dna.branch_if_true)
                    tree_walk(child_dna.branch_if_false,partner_dna.branch_if_false)

        tree_walk(child.dna,partner.dna)

        # Starting life of the child is the average of the lifes of the parents + a %
        # Capped to the base start life
        child.life = 0.5*(self.life + partner.life)
        child.life += 0.1*child.life
        child.life = min(child.life,base_starting_life)
        if np.random.uniform() < 0.25:
            child.mutate()
        return child

    def mutate(self):
        def tree_walk(tree):
            if type(tree) == Node:
                if np.random.uniform() < 0.05:
                    tree.param = np.random.choice(9*2) # 9x2 inputs
                if np.random.uniform() < 0.05:
                    tree.value = np.random.choice(cell_types)
                tree_walk(tree.branch_if_true)
                tree_walk(tree.branch_if_false)
            else:
                if np.random.uniform() < 0.05:
                    tree.action = np.random.choice(6)
            
        tree_walk(self.dna)

# Initial population
base_pop_size = 75
population = [Organism() for _ in range(base_pop_size)]

# Build world
build_init_board()

# Do simulation
num_predator_list = []
num_prey_list = []
seed_predators = [org for org in population if org.is_predator][:5]
seed_preys = [org for org in population if not org.is_predator][:5]
try:
    for sim_iter in range(100000):
        # Every 75 iterations, reset the enviroment
        if sim_iter % 75 == 0:
            food_count = np.zeros(board.shape)
            build_init_board()

        np.random.shuffle(population)
        #org_density = np.zeros(board.shape)
        org_types = np.zeros(board.shape)
        num_prey,num_predator = 0,0
        for org in population:
            #org_density[org.pos[0],org.pos[1]] += 1
            org_types[org.pos[0],org.pos[1]] = 1 if org.is_predator else 2
            if org.is_predator:
                num_predator += 1
            else:
                num_prey += 1
        #org_density = gaussian_filter(org_density, sigma=2)
        num_prey_list.append(num_prey)
        num_predator_list.append(num_predator)

        # Step
        for org in population:
            org.step()
        
        # Update seed organisms
        '''
        for org in population:
            if org.is_predator:
                for idx,(v,_) in enumerate(best_predators):
                    if org.accumulated_life > v:
                        best_predators[idx] = (org.accumulated_life,org)
            else:
                for idx,(v,_) in enumerate(best_preys):
                    if org.accumulated_life > v:
                        best_preys[idx] = (org.accumulated_life,org)
        '''
        predators = [org for org in population if org.is_predator]
        if len(predators) > 5:
            w_predator = [org.accumulated_life for org in predators]
            w_predator /= sum(w_predator)
            new_seeds = np.random.choice(predators,size=5,replace=False,p=w_predator)
            seed_predators = [old if old.accumulated_life > new.accumulated_life else new for old,new in zip(seed_predators,new_seeds)]
        preys = [org for org in population if not org.is_predator]
        if len(preys) > 5:
            w_prey = [org.accumulated_life for org in preys]
            w_prey /= sum(w_prey)
            new_seeds = np.random.choice(preys,size=5,replace=False,p=w_prey)
            seed_preys = [old if old.accumulated_life > new.accumulated_life else new for old,new in zip(seed_preys,new_seeds)]

        # Filter dead organisms
        population = [org for org in population if org.life > 0]
        print(str(len(population)) + ' | Prey : ' + str(num_prey) + ' , Predator : ' + str(num_predator))

        # Remove part of the food from food cells where organisms are
        for org in population:
            if not org.is_predator and board[tuple(org.pos)] == 0: # food cell
                food_count[tuple(org.pos)] -= 1
                if food_count[tuple(org.pos)] <= 0:
                    board[tuple(org.pos)] = 1 # cell is now empty

        # First version : Keep population size constant
        #if len(population) < base_pop_size:
        # Second version : Use a normal distribution and try to keep distance
        candidate_parents = set()

        # Find suitable matches
        for p0 in population:
            partners = []
            partners_w = []
            w_total = 0
            for p1 in population:
                if p0 is not p1 and p0.is_predator == p1.is_predator and np.linalg.norm(p0.pos - p1.pos,ord=np.inf) <= 2.0:
                    partners.append(p1)
                    w = p1.local_fitness()
                    partners_w.append(w)
                    w_total += w
                #if p0 is not p1 and np.linalg.norm(p0.pos - p1.pos,ord=np.inf) <= 2.0 and p0.accept_partner(p1) and p1.accept_partner(p0):
                    #candidate_parents.add((p0,p1))
            # Select random partner based on local fitness
            if len(partners) > 0:
                partner = np.random.choice(partners,p=[w/w_total for w in partners_w])
                candidate_parents.add((p0,partner))

        candidate_parents = [parents for parents in candidate_parents]
        np.random.shuffle(candidate_parents)

        # Replace the dead individuals
        for pA,pB in candidate_parents:
            if len(population) < base_pop_size + np.random.normal(0.0,5.0)**2:
                population.append(pA.cross(pB))
                pA.number_of_childs += 1
                pB.number_of_childs += 1
                '''if np.random.uniform() < 0:#0.25:
                    population.append(Organism()) # Introduce diversity by creating a fully new organism
                else:
                    population.append(pA.cross(pB))
                    pA.number_of_childs += 1
                    pB.number_of_childs += 1'''

        # Visualize organisms
        plt.clf()
        plt.figure(1)
        plt.imshow([[cell_values[c] for c in l] for l in board],cmap = plt.get_cmap('gist_gray'))
        plt.imshow(org_types,alpha=0.5)

        '''plt.figure(2)
        org_ages = np.zeros(board.shape)
        for org in population:
            org_ages[org.pos[0],org.pos[1]] = org.age
        plt.imshow([[cell_values[c] for c in l] for l in board],cmap = plt.get_cmap('gist_gray'))
        plt.imshow(org_ages,alpha=0.5)

        plt.figure(3)
        plt.imshow([[cell_values[c] for c in l] for l in board],cmap = plt.get_cmap('gist_gray'))
        plt.imshow(org_density,alpha=0.5)'''

        plt.pause(0.01)

        # If all the organisms died, need to re-create everything
        # Instead of starting randomly again, start from the seed organisms
        # Do include some new that are fully random
        if len(population) == 0:
            for _ in range(base_pop_size):
                org = Organism(init_dna=False)
                if np.random.uniform() < 0.75:
                    if np.random.uniform() < 0.5:
                        org.dna = copy.deepcopy(np.random.choice(seed_predators).dna)
                    else:
                        org.dna = copy.deepcopy(np.random.choice(seed_preys).dna)
                    org.mutate()
                else:
                    org = Organism()
                population.append(org)
except KeyboardInterrupt:
    try:
        plt.clf()
        plt.ioff()
        plt.figure(1)
        plt.plot(num_predator_list,label='Predators')
        plt.plot(num_prey_list,label='Prey')

        plt.figure(2)
        plt.plot([x + y for x,y in zip(num_predator_list,num_prey_list)],label='Prey')
        plt.show()
    except KeyboardInterrupt:
        plt.show()



#cuando colapsa el ecosistema, vas a arrancar de nuevo
#pero en vez de arrancar desde cero, podes irte guardando los mejores y arrancar con eso + random        

        