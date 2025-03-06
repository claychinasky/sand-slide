#pragma once


#include "../element.h"

class Smoke: public Element {
public:
    const double DECAY = 1.0 / 130;
    const double UP = 1.0 / 1.5;
    const double UP_BLOCK = 1.0 / 16;

    void process(SandSimulation *sim, int row, int col) override {
        if (sim->randf() < DECAY) {
            sim->set_cell(row, col, 0);
            return;
        }

        if (sim->is_gravity_enabled()) {
            // Side-scroller mode with anti-gravity (smoke rises)
            bool blocked = !sim->in_bounds(row - 1, col) || sim->get_cell(row - 1, col) == 6;
            if (sim->randf() < (blocked ? UP_BLOCK : UP)) {
                sim->move_and_swap(row, col, row - 1, col);
            } else {
                sim->move_and_swap(row, col, row, col + (sim->randf() < 0.5 ? 1 : -1));
            }
        } else {
            // Top-down mode without gravity
            // In top-down view, smoke spreads in all directions equally
            if (sim->randf() < UP) {
                // Choose a random direction with equal probability
                int direction = static_cast<int>(sim->randf() * 4);
                int new_row = row;
                int new_col = col;
                
                switch (direction) {
                    case 0: // Up
                        new_row = row - 1;
                        break;
                    case 1: // Right
                        new_col = col + 1;
                        break;
                    case 2: // Down
                        new_row = row + 1;
                        break;
                    case 3: // Left
                        new_col = col - 1;
                        break;
                }
                
                if (sim->in_bounds(new_row, new_col) && sim->get_cell(new_row, new_col) != 6) {
                    sim->move_and_swap(row, col, new_row, new_col);
                }
            }
        }
    }

    double get_density() override {
        return 0.35;
    }

    double get_explode_resistance() override {
        return 0.6;
    }

    double get_acid_resistance() override {
        return 0.1;
    }

    int get_state() override {
        return 2;
    }

    int get_temperature() override {
        return 0;
    }

    int get_toxicity() override {
        return 0;
    }
};
