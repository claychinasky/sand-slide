#ifndef INFO_PISTON_DOWN_CHARGED_H
#define INFO_PISTON_DOWN_CHARGED_H



#include "../element.h"

class InfoPistonDownCharged: public Element {
public:
    const double MOVE = 1.0 / 32;
    const double LOSE_CHARGE = 1.0 / 3;

    void process(SandSimulation *sim, int row, int col) override {
        if (sim->randf() < LOSE_CHARGE) {
            sim->set_cell(row, col, 105);
            return;
        }
        if (sim->randf() < MOVE) {
            int end_row = row;
            while (end_row < sim->get_height() && sim->get_cell(end_row, col) != 0 && sim->get_cell(end_row, col) != 15) {
                end_row++;
            }
            if (end_row == sim->get_height() || sim->get_cell(end_row, col) == 15) {
                return;
            }

            while (end_row > row) {
                if (end_row != sim->get_height())
                    sim->set_cell(end_row, col, sim->get_cell(end_row - 1, col));
                end_row--;
            }
        }
    }

    double get_density() override {
        return 32.0;
    }

    double get_explode_resistance() override {
        return 0.05;
    }

    double get_acid_resistance() override {
        return 0.99;
    }

    int get_state() override {
        return 0;
    }
};

#endif // INFO_PISTON_DOWN_CHARGED_H