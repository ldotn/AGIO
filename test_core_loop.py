import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
import math
import copy

population = []

plt.ion()
board_bounds = np.array([59,59],dtype=int)
board = np.zeros((board_bounds[0]+1,board_bounds[1]+1),dtype=int)
cell_values = [4,-0.5,-8]
cell_types = [i for i in range(len(cell_values))]
org_positions = np.zeros(board.shape)
org_density = np.zeros(board.shape)
org_types = np.zeros(board.shape)

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

class Organism:
    def __init__(self,init_dna = True):
        self.pos = np.zeros(2,dtype=int)
        self.pos[0] = int(np.random.choice(board_bounds[0]))
        self.pos[1] = int(np.random.choice(board_bounds[1]))
        self.life = 100
        self.age = 0
        self.accumulated_life = 0
        self.is_predator = np.random.uniform() < 0.3
        if self.is_predator:
            self.life = 300 

        if init_dna:
            # Using a binary decision tree
            # That way you always get an answer
            def build_random_tree(level):
                if level == 0 or np.random.uniform() > 0.55:
                    return Leaf(np.random.choice(5)) # 5 actions
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
            if action == 0:
                self.pos[0] += 1
            elif action == 1:
                self.pos[0] -= 1
            elif action == 2:
                self.pos[1] += 1
            elif action == 3:
                self.pos[1] -= 1
            elif action == 4: # Random move
                self.pos[1 if np.random.uniform() < 0.5 else 0] += 1 if np.random.uniform() < 0.5 else 1
            #elif action == 5:
                #None#idle

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
                    if org.pos[0] == self.pos[0] and org.pos[1] == self.pos[1] and not org.is_predator:
                        self.life += 25
                        population[idx].life -= 50
            else:
                if cell_type == 0:
                    self.life += cell_values[0] / (org_density[self.pos[0],self.pos[1]] + 1)
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
    
    def accept_partner(self,partner):
        # Predators can't mate with prey
        if self.is_predator != partner.is_predator:
            return False

        # Condition 1 : Want partners that aren't wounded
        if partner.life < np.random.exponential(0.01):
            return False

        # Condition 2 : Want partners that are not too young or too old
        peak_age = 150 # Arbitrary, the 'best' value depends on the scene and actions
        if np.abs(partner.age - peak_age) > np.abs(np.random.normal(peak_age,4.0)):
            return False

        # Condition 3 : Want partners that had a high average life
        #if (partner.accumulated_life / partner.age) > np.random.exponential(0.05):
            #return False

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

        return child

# Initial population
base_pop_size = 100
population = [Organism() for _ in range(base_pop_size)]

# Build world
build_init_board()

# Do simulation
for sim_iter in range(100000):
    np.random.shuffle(population)
    org_density = np.zeros(board.shape)
    org_types = np.zeros(board.shape)
    num_prey,num_predator = 0,0
    for org in population:
        org_density[org.pos[0],org.pos[1]] += 1
        org_types[org.pos[0],org.pos[1]] = 1 if org.is_predator else 2
        if org.is_predator:
            num_predator += 1
        else:
            num_prey += 1

    # Step
    for org in population:
        org.step()

    # Filter dead organisms
    population = [org for org in population if org.life > 0]
    print(str(len(population)) + ' | Prey : ' + str(num_prey) + ' , Predator : ' + str(num_predator))

    # First version : Keep population size constant
    if len(population) < base_pop_size:
        candidate_parents = set()

        # Find suitable matches
        for p0 in population:
            for p1 in population:
                if p0 is not p1 and np.linalg.norm(p0.pos - p1.pos,ord=np.inf) <= 2.0 and p0.accept_partner(p1) and p1.accept_partner(p0):
                    candidate_parents.add((p0,p1))
        print(len(candidate_parents))
        print('---------')
        # Produce candidate childs
        candidate_childs = []
        for pA,pB in candidate_parents:
            # If they passed the acceptance tests, both tuples are in the list
            # So the two childs will be generated
            candidate_childs.append(pA.cross(pB)) 
        
        # Replace the dead individuals
        while len(population) < base_pop_size and len(candidate_childs) > 0:
            if np.random.uniform() < 0.25:
                population.append(Organism()) # Introduce diversity by creating a fully new organism
            else:
                cidx = np.random.choice(len(candidate_childs))
                population.append(candidate_childs[cidx])
                del candidate_childs[cidx]

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



        

        