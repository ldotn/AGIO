# AGIO

AGIO : Automatic Generation of Interrelated Organisms

This is a reference C++17 implementation for the organism generation method AGIO.

## Building
1) Clone using --recurse-submodules to pull all the submodules.

### Windows
2) Open the folder with Visual Studio 2017 or later (the Community versions work fine).
3) Wait for CMake to create the solution files. The first time it will take a while as it fetches boost.
4) Select the experiment you want and compile as usual.

### Unix
2) Install SMFL. On ubuntu you can do 

        sudo apt-get install libsfml-dev
3) Run cmake to configure the project. Need CMake 3.12 or higher. The first time it will take a while as it fetches boost. Supported builds are Release, Debug and RelWithDebInfo

        cmake -DCMAKE_BUILD_TYPE=Release . 
3.1 You can turn off the build_tests option to not build the experiments.
4) Run make. It will build all the tests, though a couple are exclusive to windows.

        make

## Organization
AGIO is built as a static library, which is later linked by the different experiments. All source files are located on the src folder.
As a note, while the paper uses the term _"Organism"_ to refer to AGIO's agents and _"Individual"_ for NEAT agents, in the code is the other way around. I'm sorry about that.

### Library
All code is under the _AGIO_ namespace, and is organized on the following folders.

#### Core
Global configuration parameters and support for loading it from files.

#### Utils
General helper functions for math, SFML support, threading, and others.

#### Evolution
Code for organism and population generation, and the public interface

##### PublicInterface
The **PublicInteface** class, defined on _Globals.h_, is the way AGIO interacts with a specific problem. The user must specialize the class and assign an instance to the **Interface** global variable before using any AGIO method or object.
This class is responsible of specifying the component groups, manage the state of the organisms and the environment, and compute the fitness.

#### Interface
Contains the base individual definition, which is the base class for organisms during evolution and evolved organisms.

#### Serialization
Support for saving a population to disk and reading it back, with a thinner organism implementation. 
This is useful for using the evolved individuals in your games/apps.

### Experiments
There are several experiments all on the _Tests_ folder. The folders are separated by what interface implementation they use

#### 3D Experiment
The experiment here is not reported on the paper as it's an unfinished test of integrating AGIO organisms into an Unreal Engine 4 project.

#### Greedy
A comparison greedy algorithm for the prey-predator system. Currently unused.

#### PreyPredator
