#ifndef GLASS_H
#define GLASS_H



#include "../element.h"

class Glass: public Element {
public:
    void process(SandSimulation *sim, int row, int col) override {
        
    }

    double get_density() override {
        return 3.0;
    }

    double get_explode_resistance() override {
        return 0.2;
    }

    double get_acid_resistance() override {
        return 0.95;
    }

    int get_state() override {
        return 0;
    }
};

#endif // GLASS_H