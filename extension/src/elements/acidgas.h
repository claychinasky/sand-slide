#ifndef ACID_GAS_H
#define ACID_GAS_H

#include "element.h"

class AcidGas: public Element {
public:
    const double DECAY = 1.0 / 160;

    void process(SandSimulation *sim, int row, int col) override {
        if (randf() < DECAY) {
            sim->set_cell(row, col, 0);
        }

        int dir = (std::rand() % 3) - 1;
        if (dir != 0) {
            sim->move_and_swap(row, col, row, col + dir);
        } else {
            sim->move_and_swap(row, col, row - 1, col);
        }
    }

    double get_density() override {
        return 0.35;
    }

    double get_explode_resistance() override {
        return 0.05;
    }

    double get_acid_resistance() override {
        return 0.05;
    }
};

#endif // ACID_GAS_H