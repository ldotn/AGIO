#include <matplotlibcpp.h>

#include "DiversityPlot.h"

namespace plt = matplotlibcpp;

void runSimulation() {
    while (true) {
        plt::clf();
        plt::plot(1, 1, "ob");
        plt::plot(3, 3, "ob");
        plt::pause(0.1);
    }
}