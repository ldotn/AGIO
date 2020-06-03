# AGIO : Automatic Generation of Interrelated Organisms

This is a reference C++17 implementation for the organism generation method AGIO.

## Building
1) Clone using --recurse-submodules to pull all the submodules.

### Windows
2) Open the folder with Visual Studio 2017 or later (the Community versions work fine).
3) Wait for CMake to create the solution files. The first time it will take a while as it fetches boost.
4) Select the experiment you want and compile as usual.

Note : When switching between debug and release, you'll need to do a clean first, otherwise you get (at least with VS 2019) linker errors.

### Unix (or Mac probably)
2) Install SMFL. On ubuntu you can do 

        sudo apt-get install libsfml-dev
3) Run cmake to configure the project. Need CMake 3.12 or higher. The first time it will take a while as it fetches boost. Supported configurations are Release, Debug and RelWithDebInfo.

        cmake -DCMAKE_BUILD_TYPE=Release . 
3.1) You can turn off the build_tests option to not build the experiments.
4) Run make. You can build all the experiments or just the one you want. A couple experiments are exclusive to windows.

        make

## Organization
AGIO is built as a static library, which is later linked by the different experiments. All source files are located on the src folder.
As a note, while the paper uses the term _"Organism"_ to refer to AGIO's agents and _"Individual"_ for NEAT agents, in the code is the other way around. I'm sorry about that.

### Library
All code is under the _AGIO_ namespace, and is organized on the following folders.

* _**Core**_ : Global configuration parameters and support for loading it from files.

* _**Utils**_ : General helper functions for math, SFML support, threading, and others.

* _**Evolution**_ : Code for organism and population generation, and the public interface
    - _**PublicInterface**_: The **PublicInteface** class, defined on _Globals.h_, is the way AGIO interacts with a specific problem. The user must specialize the class and assign an instance to the **Interface** global variable before using any AGIO method or object. This class is responsible of specifying the component groups, manage the state of the organisms and the environment, and compute the fitness.

* _**Interface**_ : Contains the base individual definition, which is the base class for organisms during evolution and evolved organisms.

* _**Serialization**_ : Support for saving a population to disk and reading it back, with a thinner organism implementation. 
This is useful for using the evolved individuals in your games/apps.

### Experiments
There are several experiments all on the _Tests_ folder. The folders are separated by what interface implementation they use. After compilation the binaries for the different experiments with their lowercase underscored names will be on the _bin_ folder.

When running the experiments it's important that the working directory is the _bin_ folder, otherwise it'll fail to find the files it needs to work.

* _**3D Experiment**_ : The experiment here is not reported on the paper as it's an unfinished test of integrating AGIO organisms into an Unreal Engine 4 project.

* _**Greedy**_ : A greedy comparison algorithm for the prey-predator system. Currently unused.

* _**PreyPredator**_ : This folder contains two experiments, prey_predator (_main.cpp_) and sim_size_test (_SimSizeTest.cpp_). 

    - _**prey_predator**_ : Verification experiment with two species, one of carnivores and the other of herbivores. The experiment has two modes, selected at startup. One runs the evolution and stores several CSV files with per generation values like fitness or species age. The other presents a 2D visualization of the simulation, with round sprites being herbivores, star-like sprites carnivores and green "smudges" food. The evolution needs to be run before the visualization, otherwise it won't have any evolved individuals to load.

        _Note: For some reason I never managed to fix (not that I dedicated much time trying tbh), the SFML visualizations only seems to work on Windows on Debug builds._ Outputs the following CSV files, where _XXXX_ is a timestamp value :

      - _XXXX\_age\_herbivore.csv_
      - _XXXX\_age\_carnivore.csv_
      - _XXXX\_avg\_eaten\_herbivore.csv_
      - _XXXX\_avg\_eaten\_carnivore.csv_
      - _XXXX\_avg\_failed\_herbivore.csv_
      - _XXXX\_avg\_failed\_carnivore.csv_
      - _XXXX\_avg\_coverage\_herbivore.csv_
      - _XXXX\_avg\_coverage\_carnivore.csv_
      - _XXXX\_min\_eaten\_herbivore.csv_
      - _XXXX\_min\_failed\_herbivore.csv_
      - _XXXX\_min\_coverage\_herbivore.csv_
      - _XXXX\_max\_eaten\_herbivore.csv_
      - _XXXX\_max\_failed\_herbivore.csv_
      - _XXXX\_max\_coverage\_herbivore.csv_
      - _XXXX\_min\_eaten\_carnivore.csv_
      - _XXXX\_min\_failed\_carnivore.csv_
      - _XXXX\_min\_coverage\_carnivore.csv_
      - _XXXX\_max\_eaten\_carnivore.csv_
      - _XXXX\_max\_failed\_carnivore.csv_
      - _XXXX\_max\_coverage\_carnivore.csv_
            
    - _**sim_size_test**_ : Does multiple evolutions testing different simulation sizes but keeping the population size constant. Meant to show the impact the simulation size has on the behavior, as a justification for the decoupling of simulation and population sizes.
    
        Outputs a CSV (_sim\_size.csv_) with the per species metrics on the last generation of each evolution.

* _**ComplexSystem**_ : This folder contains several experiments 

    - _**complex_system**_ : Equivalent to the prey_predator experiment but for the complex system. On the visualizer the two sprites from prey_predator are reused, with the addition of a third one for omnivores. Different species use different colors. This experiment is also used for the parametric configuration.

        This experiment does not generate any output files, it just logs to the console.
    - _**perf_test**_ : Profiles average partial and total (considering sensor and action evaluation) decision time, along with evolution time as a factor of the number of generations and population size multiplier. 

        Evolution times are printed on the console while decision times are stored on two CSVs (_total_times.csv_ and _decision_times.csv_).
    - _**nn_graph_dump**_ : Does an evolution run on the complex system and generates DOT files for the neural networks of the evolved organisms. Squares are inputs, triangles outputs and hexagon bias signals.

    - _**interrelations_test**_ : Evaluates interrelations between species. It outputs three CSV files, _interrelations.csv_, _baseline.csv_ and _species_ref.csv_, which can be processed by the _process_interrel.py_ script.

    - _**human_test**_ (Windows exclusive) : Used to compare AGIO with a player. If no evolved population is found (_human\_test\_evolved\_g400.txt_) it will first evolve the population before proceeding to the experiment, where the player is tasked to control an organism.

        It outputs a timestamped CSV file of the form _history\_\[showing\_descs\]\_XXXX\_.csv_ and also prints to the console.

    - _**memory_test**_ (Windows exclusive) : Profiles memory usage during evolution and of the evolved organisms.

        Outputs two CSV files, _mem\_evo.csv_ and _mem\_org.csv_.

### Scripts
In addition to the library and the experiments, there are a few python scripts to help processing the results and doing the parametric configuration. The scripts require matplotlib, scipy and numpy.

* Parametric configuration : The first step of the parametric configuration is running the _SensibilityTest.py_ script, which will generate the files to select the most relevant parameters. After that's finished running _process\_sensibility.py_ will output the coefficient of variation for each parameter.

  After selecting the parameters to adjust the _ParametricConfig.py_ script needs to be run to generate the results. By default it will test values for the parameters on the _range_ array on line 16, that needs to be manually modified if it doesn't match the selected parameters. Keep in mind that even with a good computer this is a slow process. The script takes as input the number of workers to use, try to balance this with the number of workers used during evolution (12 by default).

  Finally, the _process\_params.py_ script will output the best configurations in importance order (the ones further from the center and closest to the y = x line on the fitness - species count plane).

* Interrelations : The _process\_interrel.py_ script will process the results from the interrelations experiment and generate both an interrelations graph in DOT format and a table in latex format with the statistics. On the graph diamonds represent omnivores, ellipses herbivores and diamonds carnivores. A dashed line from A to B signifies that A's fitness decreased when B was not on the simulation, while a solid line means it increased.

* Timing experiments : _process\_times.py_ is a simple script that computes mean and standard deviation for the values generated by the different timing experiments.