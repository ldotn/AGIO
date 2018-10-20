#include <iostream>
#include "PreyPredatorEvolution.cpp"
#include "PreyPredator.cpp"

int main() {
    cout << "1 - Run Evolution" << endl;
    cout << "2 - Run Simulation" << endl;
    cout << "Select an option:";

    int op;
    cin >> op;
    if (op == 1)
        runEvolution();
    if (op == 2)
        runSimulation();

}