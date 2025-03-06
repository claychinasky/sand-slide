#pragma once


#include "../element.h"

class Sand: public Element {
public:
    const double FLAME = 1 / 64.0;
    const double POWDER = 1 / 1.05;

    void process(SandSimulation *sim, int row, int col) override {
        if (sim->randf() >= POWDER)
            return;
        if (sim->randf() < FLAME && sim->is_on_fire(row, col)) {
            sim->set_cell(row, col, 53);
            return;
        }
        
        if (sim->is_gravity_enabled()) {
            // Side-scroller mode with gravity
            bool bot_left = sim->is_swappable(row, col, row + 1, col - 1);
            bool bot = sim->is_swappable(row, col, row + 1, col);
            bool bot_right = sim->is_swappable(row, col, row + 1, col + 1);
            if (bot) {
                sim->move_and_swap(row, col, row + 1, col);
            } else if (bot_left && bot_right) {
                sim->move_and_swap(row, col, row + 1, col + (sim->randf() < 0.5 ? 1 : -1));
            } else if (bot_left) {
                sim->move_and_swap(row, col, row + 1, col - 1);
            } else if (bot_right) {
                sim->move_and_swap(row, col, row + 1, col + 1);
            }
        } else {
            // Top-down mode without gravity
            // Sand will only move if disturbed (e.g., by other elements or player actions)
            // This makes it behave more like a static element in a top-down game
            
            // We'll keep a small chance for sand to settle/spread to simulate some minor movement
            if (sim->randf() < 0.05) {
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
                
                if (sim->is_swappable(row, col, new_row, new_col)) {
                    sim->move_and_swap(row, col, new_row, new_col);
                }
            }
        }
    }

    double get_density() override {
        return 2.0;
    }

    double get_explode_resistance() override {
        return 0.85;
    }

    double get_acid_resistance() override {
        return 0.2;
    }

    int get_state() override {
        return 0;
    }

    int get_temperature() override {
        return 0;
    }

    int get_toxicity() override {
        return 0;
    }
};
