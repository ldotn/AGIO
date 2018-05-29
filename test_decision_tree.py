import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
import math
import copy

plt.ion()
board_bounds = np.array([59,59],dtype=int)
board = np.zeros((board_bounds[0]+1,board_bounds[1]+1),dtype=int)
cell_values = [7,-2,-9]
cell_types = [i for i in range(len(cell_values))]

def build_init_board():
    for x,y in np.ndindex(board.shape):
        w0 = 1.0/(0.1*((x-40.0)**2 + (y-10.0)**2) + 1.0)
        w1 = 1.0/(0.1*((x-10.0)**2 + (y-35.0)**2) + 1.0)
        w2 = 1.0/(0.5*((x-27.0)**2 + (y-32.0)**2) + 1.0)

        a0 = -10.5
        a1 = -10.1
        a2 = 10
        
        v = a0*w0 + a1*w1 + a2*w2

        if v > 0.5:
            board[x,y] = 0
        elif v > -0.5 and v <= 0.5:
            board[x,y] = 1
        else:
            board[x,y] = 2
build_init_board()

# Using wrapper classes as a way to have pointers
class Node:
    def __init__(self,param_idx,branches_map):
        self.param_idx = param_idx
        self.branches_map = branches_map

    def __str__(self):
        return ('( ' + str(self.param_idx) + ':' + str([str(v) + '->' + str(b) for v,b in self.branches_map.items()]) + ' )').replace('\\','')

class Leaf:
    def __init__(self,value):
        self.value = value
    
    def __str__(self):
        return str(self.value)

def tree_traverse(tree,callback,leaf_callback):
    if type(tree) is Leaf:
        return leaf_callback(tree)
    for value,branch in tree.branches_map.items():
        if callback(tree.param_idx,value,branch):
            break
        tree_traverse(branch,callback,leaf_callback)

class Organism:
    def __init__(self,init_dna = True):
        self.pos = np.zeros(2,dtype=int)
        self.pos[0] = int(np.random.choice(board_bounds[0]))
        self.pos[1] = int(np.random.choice(board_bounds[1]))
        self.life = 100
        self.age = 0

        if init_dna:
            def build_random_tree(params_list):
                if len(params_list) == 0 or np.random.uniform() > 0.55:
                    return Leaf(np.random.choice(5)) # 5 actions
                else:
                    branches_map = {}
                    param = np.random.choice(np.array(params_list)) # 9 inputs

                    #for v in cell_types:
                    for v in np.random.choice(cell_types,size=int(np.floor(np.random.uniform(1,len(cell_types))))):
                        branches_map[v] = build_random_tree([p for p in params_list if p != param])
                    
                    return Node(param,branches_map)
            self.dna = build_random_tree([x for x in range(9)])

    def step(self):
        if self.life <= 0:
            return

        # Read board
        xl = np.zeros(9,dtype=int)
        current_i = 0
        for x in [-1,0,1]:
            for y in [-1,0,1]:
                xl[current_i] = board[tuple(np.clip(self.pos + np.array([x,y]),0,board_bounds))]
                current_i += 1

        # Walk tree to find the action to do
        def tree_walk(tree):
            if type(tree) is Leaf:
                return tree.value
            try:
                return tree_walk(tree.branches_map[xl[tree.param_idx]])
            except KeyError:
                return None

        action = tree_walk(self.dna)

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
        
            self.pos = np.clip(self.pos,0,board_bounds)
        self.life += cell_values[board[self.pos[0],self.pos[1]]]

        self.age += 1

'''
test_tree = Node(0,{0 : Node(1,{4 : Node(3,{2 : Leaf(-2), 1 : Leaf(42)})}), 1 : Leaf(-1)})
tree_traverse(test_tree,print,print)
def test_replace_leaf(leaf : Leaf):
    leaf.value = -1
tree_traverse(test_tree,lambda x0,x1,x2 : False,test_replace_leaf)
tree_traverse(test_tree,print,print)

max_depth = np.random.choice(5) 
def build_random_tree(l):
    if l >= max_depth or np.random.uniform() > 0.75:
        return Leaf(np.random.choice(4)) # 4 actions
    else:
        branches_map = {}
        param_idx = np.random.choice(9) # 9 inputs

        for v in np.random.choice(cell_values,size=int(np.floor(np.random.uniform(1,len(cell_values))))):
            branches_map[v] = build_random_tree(l + 1)
        
        return Node(param_idx,branches_map)
r0 = build_random_tree(0)
r1 = build_random_tree(0)
r2 = build_random_tree(0)
print('------------')
tree_traverse(r0,print,print)
print('------------')
tree_traverse(r1,print,print)
print('------------')
tree_traverse(r2,print,print)

exit()
'''







population_size = 50
population = [Organism() for _ in range(population_size)]


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
        for _ in range(1):
            new_cidx = (np.random.choice(board_bounds[0]),np.random.choice(board_bounds[1]))
            board[new_cidx] = (board[new_cidx] + 1) % 3

        if all_dead:
            break

    population = sorted(population,key=lambda org : -org.age)
    print(str(population[0].age) + ' | ' + str((np.array([org.age for org in population]))))

    # Generate childs, replacing the worst 30% of the population
    # Parents are selected randomly with an exponential distribution
    new_pop = population
    for idx in range(int(0.3*len(population))):
        replace = -np.clip(int(np.random.exponential(10)),1,len(population)-1) # reverse exponential to select replacement
        # Some childs are random 
        if np.random.uniform() < 0.2:
            new_pop[-replace] = Organism()
        else:
            parents_indices = [np.clip(int(x),0,len(population)-1) for x in np.random.exponential(10,2)]
            p0 = population[parents_indices[0]]
            p1 = population[parents_indices[1]]

            while parents_indices[0] == parents_indices[1]: # Both parents can't be the same organism
                parents_indices = [np.clip(int(x),0,len(population)-1) for x in np.random.exponential(10,2)]
                p0 = population[parents_indices[0]]
                p1 = population[parents_indices[1]]

            child = Organism(init_dna=False)

            # Start traversing tree of parent 0
            child.dna = copy.deepcopy(p0.dna)
            #hay que mantener una lista de los parameteros visto en esta rama
                                #no podes hacer el swap si en lo que vas a poner hay algun parametro que ya viste
                               # aunque si las comparaciones no son de igualdad en realidad podria estar bien
            def cross(param,value,branch):
                found = False
                def search_param(p1,v1,b1):
                    if param == p1:
                        if np.random.uniform() < 0.5:
                            branch = b1
                            found = True
                            return True
                    return False
                tree_traverse(p1.dna,search_param,lambda x0 : False)
                return found

            tree_traverse(child.dna,cross,lambda x0 : False)
            
            '''
            while type(tree0) is Node:

                # Search for this param in a the other parent
                tree1 = p1.dna
                while type(tree1) is Node:
                    if tree0.param_idx == tree1.param_idx:
                        # Found the parameter in the other parent
                        # With probability p swap
                        if np.random.uniform() < 0.25:
                            tree0.branches_map = tree1.branches_map
                            break
                           tree0 = '''

            new_pop[-replace] = child

    # Find best and show it
    print(population[0].dna)
    if g % 100 == 0:
        build_init_board()
        # Reset organisms 
        for idx in range(len(population)):
            population[idx].life = 100
            population[idx].pos[0] = np.random.choice(board_bounds[0])
            population[idx].pos[1] = np.random.choice(board_bounds[1])
            population[idx].age = 0

        for i in range(500):

            plt.clf()
            plt.imshow([[cell_values[c] for c in l] for l in board],cmap = plt.get_cmap('gist_gray'))
            org_positions = np.zeros(board.shape)
            for org in population:
                org_positions[org.pos[0],org.pos[1]] = org.life
            plt.imshow(org_positions,cmap = plt.cm.hot,alpha=0.5)
            #plt.plot([org.pos[0] for org in population if org.life > 0],[org.pos[1] for org in population if org.life > 0],'ro')
            plt.pause(0.01)
            
            any_alive = False
            for org in population:
                if org.life > 0:
                    org.step()
                    any_alive = True
            if not any_alive:
                break

            # Random changes in the enviroment
            for _ in range(1):
                new_cidx = (np.random.choice(board_bounds[0]),np.random.choice(board_bounds[1]))
                board[new_cidx] = (board[new_cidx] + 1) % 3
    
    # Swap population
    population = new_pop

    # Reset organisms 
    for idx in range(len(population)):
        population[idx].life = 100
        population[idx].pos[0] = np.random.choice(board_bounds[0])
        population[idx].pos[1] = np.random.choice(board_bounds[1])
        population[idx].age = 0
