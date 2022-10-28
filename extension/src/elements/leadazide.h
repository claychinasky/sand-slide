#ifndef LEAD_AZIDE_H
#define LEAD_AZIDE_H

class SandSimulation;

#include "element.h"

class LeadAzide: public Element {
public:
    const double FLAME = 0.15;

    void process(SandSimulation *sim, int row, int col) override {
        // Explode
        if (randf() < FLAME && sim->is_on_fire(row, col)) {
            sim->set_cell(row, col, 9);
        }
    }

    double get_density() override {
        return 16.0;
    }

    double get_explode_resistance() override {
        return 0.0;
    }

    double get_acid_resistance() override {
        return 0.65;
    }
};

#endif // LEAD_AZIDE_H