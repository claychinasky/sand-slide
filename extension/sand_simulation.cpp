#include "sand_simulation.h"
#include "elements/element.h"
#include "elements/all_elements.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

SandSimulation::SandSimulation() {
    AllElements::fill_elements(&elements);

    draw_data = PackedByteArray();
    draw_data.resize(width * height * 4);

    visited.resize(width * height);
    cells.resize(width * height);
    chunks.resize(chunk_width * chunk_height);
}

SandSimulation::~SandSimulation() {}

// Run the simulation `iterations` times
void SandSimulation::step(int iterations) {
    for (int i = 0; i < iterations; i++) {
        for (int chunk = chunks.size() - 1; chunk >= 0; chunk--) {
            if (chunks.at(chunk) == 0)
                continue;
            for (int row = chunk_size - 1; row >= 0; row--) {
                for (int col = 0; col < chunk_size; col++) {
                    int r_row = (chunk / chunk_width) * chunk_size + row;
                    int r_col = (chunk % chunk_width) * chunk_size + col;
                    if (r_row >= height || r_col >= width)
                        continue;
                    if (visited.at(r_row * width + r_col)) 
                        visited.at(r_row * width + r_col) = false;
                    else {
                        // Elements (including custom) are stored using numbers 0 - 4096
                        // Taps share the ID of their spawn material, but offset by 4097
                        if (cells.at(r_row * width + r_col) > 4096 && randf() < 1.0 / 16) {
                            int x = cells.at(r_row * width + r_col) - 4097;
                            // Grow in all 8 directions
                            grow(r_row + 1, r_col, 0, x); grow(r_row + 1, r_col + 1, 0, x); grow(r_row + 1, r_col - 1, 0, x); grow(r_row - 1, r_col, 0, x); 
                            grow(r_row - 1, r_col + 1, 0, x); grow(r_row - 1, r_col - 1, 0, x); grow(r_row, r_col + 1, 0, x); grow(r_row, r_col - 1, 0, x);
                        } else {
                            elements.at(get_cell(r_row, r_col))->process(this, r_row, r_col);
                        }
                    }
                }
            }
        }
    }
}

// Swap the elements at the two cells if the first cell has a higher density
void SandSimulation::move_and_swap(int row, int col, int row2, int col2) {
    if (!in_bounds(row, col) || !in_bounds(row2, col2)) 
        return;
    if (elements.at(get_cell(row2, col2))->get_state() == 0)
        return;
    if (elements.at(get_cell(row, col))->get_state() != 0)
        if (elements.at(get_cell(row, col))->get_density() <= elements.at(get_cell(row2, col2))->get_density()) 
            if (get_cell(row, col) != get_cell(row2, col2))
                return;
    int old = get_cell(row, col);
    set_cell(row, col, get_cell(row2, col2));
    set_cell(row2, col2, old);
}

// Move the `replacer` element into the given cell if it is of type `food`
// A `food` value of -1 is equivalent to all elements
void SandSimulation::grow(int row, int col, int food, int replacer) {
    if (!in_bounds(row, col)) 
        return;
    if (food == -1) {
        // Since only explosions/lasers grow into all cells, we run a check for explosion resistance
        if (randf() >= (1.0 - elements.at(get_cell(row, col))->get_explode_resistance())) 
            return;
    } else if (get_cell(row, col) != food) 
        return;
    set_cell(row, col, replacer);
}

void SandSimulation::liquid_process(int row, int col, int fluidity) {
    for (int i = 0; i < fluidity; i++) {
        int new_col = col + (randf() < 0.5 ? 1 : -1);
        if (randf() < 1.0 / 32)
            new_col = col;
        int new_row = row + (is_swappable(row, col, row + 1, new_col) && randf() > 0.2 ? 1 : 0);
        if (is_swappable(row, col, new_row, new_col) && (randf() < 0.3 || !is_swappable(row, col, new_row + 1, new_col))) {
            move_and_swap(row, col, new_row, new_col);
            row = new_row;
            col = new_col;
        }
    }
}

// Returns the amount of cells of `type` surrounding the given cell
int SandSimulation::touch_count(int row, int col, int type) {
    int touches = 0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0 || !in_bounds(row + y, col + x)) 
                continue;
            if (get_cell(row + y, col + x) == type) 
                touches++;
        }
    }
    return touches;
}

// Returns the amount of cells of `type` surrounding the given cell, but only checking
// immediate four closest cells
int SandSimulation::cardinal_touch_count(int row, int col, int type) {
    int touches = 0;
    if (in_bounds(row - 1, col) && get_cell(row - 1, col) == type)
        touches++;
    if (in_bounds(row + 1, col) && get_cell(row + 1, col) == type)
        touches++;
    if (in_bounds(row, col - 1) && get_cell(row, col - 1) == type)
        touches++;
    if (in_bounds(row, col + 1) && get_cell(row, col + 1) == type)
        touches++;
    return touches;
}

bool SandSimulation::in_bounds(int row, int col) {
    return row >= 0 && col >= 0 && row < height && col < width;
}

// Check if the cell is touching an element intended to destroy life, such as acid
bool SandSimulation::is_poisoned(int row, int col) {
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0 || !in_bounds(row + y, col + x)) 
                continue;
            int c = get_cell(row + y, col + x);
            if (elements.at(get_cell(row + y, col + x))->get_toxicity() == 1) 
                return true;
        }
    } 
    return false; 
}

// Check if a cell is touching any hot elements
bool SandSimulation::is_on_fire(int row, int col) {
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0 || !in_bounds(row + y, col + x))
                continue;
            if (elements.at(get_cell(row + y, col + x))->get_temperature() == 1) 
                return true;
        }
    } 
    return false;
}

// Check if a cell is touching any cold elements
bool SandSimulation::is_cold(int row, int col) {
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0 || !in_bounds(row + y, col + x))
                continue;
            if (elements.at(get_cell(row + y, col + x))->get_temperature() == -1) 
                return true;
        }
    } 
    return false;
}


bool SandSimulation::is_swappable(int row, int col, int row2, int col2) {
    if (!in_bounds(row, col) || !in_bounds(row2, col2)) 
        return false;

    if (elements.at(get_cell(row2, col2))->get_state() == 0)
        return false;

    if (elements.at(get_cell(row, col))->get_state() != 0 && elements.at(get_cell(row, col))->get_density() <= elements.at(get_cell(row2, col2))->get_density()) 
        return false;

    return true;
}

inline float SandSimulation::randf() { 
    g_seed = (214013 * g_seed + 2531011); 
    return ((g_seed>>16) & 0x7FFF) / (double) 0x7FFF; 
} 

int SandSimulation::get_cell(int row, int col) {
    // Taps should be mostly indestructible, so treat them as the wall element for processing
    if (cells.at(row * width + col) > 4096) {
        return 15;
    }

    return cells.at(row * width + col);
}

void SandSimulation::set_cell(int row, int col, int type) {
    if (cells.at(row * width + col) == 0 && type != 0) 
        chunks.at((row / chunk_size) * chunk_width + (col / chunk_size))++;
    else if (cells.at(row * width + col) != 0 && type == 0) 
        chunks.at((row / chunk_size) * chunk_width + (col / chunk_size))--;
    
    visited.at(row * width + col) = type != 0;
    cells.at(row * width + col) = type;
}

void SandSimulation::draw_cell(int row, int col, int type) {
    set_cell(row, col, type);
    visited.at(row * width + col) = false;
}


int SandSimulation::get_chunk(int c) {
    return chunks.at(c);
}

int SandSimulation::get_width() {
    return width;
}

int SandSimulation::get_height() {
    return height;
}

void SandSimulation::resize(int new_width, int new_height) {
    std::vector<int> temp(cells);
    
    cells.clear();
    cells.resize(new_width * new_height);
    
    visited.clear();
    visited.resize(new_width * new_height);

    chunk_width = (int) std::ceil(new_width / (float) chunk_size);
    chunk_height = (int) std::ceil(new_height / (float) chunk_size);
    
    chunks.clear();
    chunks.resize(chunk_width * chunk_height);

    draw_data.resize(new_width * new_height * 4);

    // Data has to be copied cell-by-cell since the dimensions of the vectors changed
    for (int row = height - 1, new_row = new_height - 1; row >= 0 && new_row >= 0; row--, new_row--) {
        for (int col = 0, new_col = 0; col < width && new_col < new_width; col++, new_col++) {
            cells.at(new_row * new_width + new_col) = temp.at(row * width + col);
            if (cells.at(new_row * new_width + new_col) != 0) 
                chunks.at((new_row / chunk_size) * chunk_width + (new_col / chunk_size))++;
        }
    }

    width = new_width;
    height = new_height;   
}

void SandSimulation::set_chunk_size(int new_size) {
    chunk_size = new_size;
    resize(width, height);
}

PackedByteArray SandSimulation::get_data() {
    PackedByteArray data = PackedByteArray();
    data.resize(width * height * 4);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int id = cells.at(y * width + x);
            int idx = (y * width + x) * 4;
            data.set(idx + 0, (id & 0xFF000000) >> 24);
            data.set(idx + 1, (id & 0x00FF0000) >> 16);
            data.set(idx + 2, (id & 0x0000FF00) >> 8);
            data.set(idx + 3, (id & 0x000000FF) >> 0);
        }
    }
    return data;
}

// Graphic methods

void SandSimulation::initialize_textures(Array images) {
    textures.resize(images.size());
    for (int i = 0; i < images.size(); i++) {
        Ref<Image> img = images[i];
        PackedByteArray bytes = img->get_data();

        GameTexture g;
        g.width = img->get_width();
        g.height = img->get_height();
        g.pixels = new std::vector<uint32_t>();

        g.pixels->resize(g.width * g.height);

        for (int px = 0; px < g.width * g.height; px++) {
            int idx = px * 4;
            
            uint32_t re = bytes[idx + 0] << 24;
            uint32_t gr = bytes[idx + 1] << 16;
            uint32_t bl = bytes[idx + 2] << 8;
            g.pixels->at(px) = re | gr | bl | 0xFF;
        }

        textures.at(i) = g;
    }
}

void SandSimulation::initialize_flat_color(Dictionary dict) {
    flat_color.clear();
    
    Array ids = dict.keys();
    for (int i = 0; i < dict.size(); i++) {
        int id = ids[i];
        flat_color[id] = dict[id];
    }
}

void SandSimulation::initialize_gradient_color(Dictionary dict) {
    gradient_color.clear();
    
    Array ids = dict.keys();
    for (int i = 0; i < dict.size(); i++) {
        int id = ids[i];
        Gradient g;

        Array arr = dict[id];

        g.colors[0] = arr[0];
        g.colors[1] = arr[1];
        g.colors[2] = arr[2];
        g.colors[3] = arr[3];
        g.colors[4] = arr[4];

        g.offsets[0] = arr[5];
        g.offsets[1] = arr[6];
        g.offsets[2] = arr[7];

        gradient_color[id] = g;
    }
}

uint32_t SandSimulation::lerp_color(uint32_t a, uint32_t b, double x) {
    uint32_t re = uint32_t(double((a & 0xFF000000) >> 24) * (1.0 - x) + double((b & 0xFF000000) >> 24) * (x)) << 24;
    uint32_t gr = uint32_t(double((a & 0x00FF0000) >> 16) * (1.0 - x) + double((b & 0x00FF0000) >> 16) * (x)) << 16;
    uint32_t bl = uint32_t(double((a & 0x0000FF00) >> 8) * (1.0 - x) + double((b & 0x0000FF00) >> 8) * (x)) << 8;
    return re | gr | bl | 0xFF;
}

// https://thebookofshaders.com/glossary/?search=smoothstep
double SandSimulation::smooth_step(double edge0, double edge1, double x) {
    // Scale, bias and saturate x to 0..1 range
    x = (x - edge0) / (edge1 - edge0);
    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;
    // Evaluate polynomial
    return x * x * (3.0 - 2.0 * x);
}

uint32_t SandSimulation::sample_texture(GameTexture t, int x, int y) {
    x %= t.width;
    y %= t.height;
    return t.pixels->at(y * t.width + x);
}

uint32_t SandSimulation::get_color(int row, int col) {
    int id = cells.at(row * width + col);

    if (flat_color.find(id) != flat_color.end()) {
        return flat_color[id];
    }

    if (gradient_color.find(id) != gradient_color.end()) {
        Gradient g = gradient_color[id];
        double x = double(touch_count(row, col, id)) / 8.0;

        uint32_t out = lerp_color(g.colors[0], g.colors[1], smooth_step(0.0, g.offsets[0], x));
        out = lerp_color(out, g.colors[2], smooth_step(g.offsets[0], g.offsets[1], x));
        out = lerp_color(out, g.colors[3], smooth_step(g.offsets[1], g.offsets[2], x));
        out = lerp_color(out, g.colors[4], smooth_step(g.offsets[2], 1.0, x));

        return out;
    }

    return 0xFFFFFFFF;
}

PackedByteArray SandSimulation::get_color_image() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t col = get_color(y, x);
            
            int idx = (y * width + x) * 4;
            draw_data.set(idx + 0, (col & 0xFF000000) >> 24);
            draw_data.set(idx + 1, (col & 0x00FF0000) >> 16);
            draw_data.set(idx + 2, (col & 0x0000FF00) >> 8);
            draw_data.set(idx + 3, 0xFF);
        }
    }
    return draw_data;
}

void SandSimulation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("step"), &SandSimulation::step);
    ClassDB::bind_method(D_METHOD("in_bounds"), &SandSimulation::in_bounds);
    ClassDB::bind_method(D_METHOD("get_cell"), &SandSimulation::get_cell);
    ClassDB::bind_method(D_METHOD("set_cell"), &SandSimulation::set_cell);
    ClassDB::bind_method(D_METHOD("draw_cell"), &SandSimulation::draw_cell);
    ClassDB::bind_method(D_METHOD("get_data"), &SandSimulation::get_data);
    ClassDB::bind_method(D_METHOD("get_width"), &SandSimulation::get_width);
    ClassDB::bind_method(D_METHOD("get_height"), &SandSimulation::get_height);
    ClassDB::bind_method(D_METHOD("resize"), &SandSimulation::resize);
    ClassDB::bind_method(D_METHOD("set_chunk_size"), &SandSimulation::set_chunk_size);
    ClassDB::bind_method(D_METHOD("get_color_image"), &SandSimulation::get_color_image);

    ClassDB::bind_method(D_METHOD("initialize_textures"), &SandSimulation::initialize_textures);
    ClassDB::bind_method(D_METHOD("initialize_flat_color"), &SandSimulation::initialize_flat_color);
    ClassDB::bind_method(D_METHOD("initialize_gradient_color"), &SandSimulation::initialize_gradient_color);
}
