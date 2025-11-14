#pragma once

#include <cstdint>
#include <vector>

#include "theory/note.h"

const std::vector<uint8_t> MAJOR = {2, 2, 1, 2, 2, 2, 1};
const std::vector<uint8_t> NATURAL_MINOR = {2, 1, 2, 2, 1, 2, 2};
const std::vector<uint8_t> HARMONIC_MINOR = {2, 1, 2, 2, 1, 3, 1};
const std::vector<uint8_t> MAJOR_PENTATONIC = {2, 2, 3, 2, 3};

enum SCALE_MODES {
    IONIAN,
    DORIAN,
    PHRYGIAN,
    LYDIAN,
    MIXOLYDIAN,
    AEOLIAN,
    LOCRIAN
};

inline std::vector<Note> make_scale(
    uint8_t root,
    std::vector<uint8_t> scale_intervals,
    uint8_t octaves = 1,
    uint8_t mode = 0,
    uint8_t velocity = 64
)  {
    std::vector<Note> scale;
    uint8_t displacement = 0;
    mode %= scale_intervals.size();
    for(int i = 0; i < octaves * scale_intervals.size(); i++) {
        scale.push_back(Note(root + displacement, velocity));
        displacement += scale_intervals[mode];
        ++mode;
        if(mode >= scale_intervals.size()) {
            mode -= scale_intervals.size();
        }
    }
    return scale;
}