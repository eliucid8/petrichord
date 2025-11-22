#pragma once

#include <cstdint>
#include <vector>

#include "theory/note.h"

// the number of half-steps between each note in a chord
const std::vector<uint8_t> MAJOR_INTERVALS = {4, 3, 5};
const std::vector<uint8_t> MINOR_INTERVALS = {3, 4, 5};
const std::vector<uint8_t> DIMINISHED_INTERVALS = {3, 3, 3, 3};
const std::vector<uint8_t> AUGMENTED_INTERVALS = {3, 5, 4};
const std::vector<uint8_t> DOMINANT_INTERVALS = {4, 3, 3, 2};
const std::vector<uint8_t> M7_INTERVALS = {4, 3, 4, 1};

// TODO: typedef vector<Note> as a chord?
inline std::vector<Note> generate_chord(
    uint8_t root, 
    const std::vector<uint8_t> chord_intervals, 
    uint8_t extent = 0, 
    uint8_t inversion = 0, 
    uint8_t velocity = 64
) {
    std::vector<Note> chord;
    // default size is 1 octave.
    if(extent == 0) {
        extent = chord_intervals.size() - 1;
    }
    
    // set root of chord to the correct inversion
    if(inversion > 0) {
        inversion %= chord_intervals.size();
        for(int i = 0; i < inversion; i++) {
            root += chord_intervals[i];
        }
    }

    uint8_t pitch = root;
    for(int i = 0; i < extent; i++) {
        if(pitch > 127) { break; }
        chord.push_back(Note(pitch, velocity));
        pitch += chord_intervals[i % chord_intervals.size()];
    }
    
    return chord;
}