#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <array>

#include "theory/chords.h"
#include "io/midi_messenger.h"
#include "io/key_matrix.h"

/**
 * calculates note on/off messages from given inputs.
 */
class ChordController {
public:
    inline static const std::array<std::vector<uint8_t>, 8> CHORD_INTERVALS = {
        MAJOR_INTERVALS,
        MAJOR_INTERVALS, // Mxx
        MINOR_INTERVALS, // xmx
        DIMINISHED_INTERVALS, // Mmx
        DOMINANT_INTERVALS, // xx7
        M7_INTERVALS, // Mx7
        MINOR7_INTERVALS, // xm7
        MINOR_MAJOR_INTERVALS, // Mm7
    };
    
    
    // F C G D A E B
    // the base midi numbers for each key. Add +12 for each extra octave you want--this is currently octave -1.
    inline static const std::array<uint8_t, 7> LETTER_LAYOUT = {
        17, 12, 19, 14, 21, 16, 23
    };

    ChordController(MidiMessenger* midi): midi(midi) {
        chord.push_back(Note(0, 0));
    }

    /**
     * called on strum interrupt. updates state of current notes down
     * and sends to midi_messenger
     */
    void handle_strum(std::vector<bool> state);   

    /**
     * DEPRECATED called on chord change.
     */
    void update_chord(std::vector<Note> chord);
    
    /**
     * pass chord key matrix state info
     * whatever keys might be a different array structure who gives a shit anymore
     */
    void update_key_state(const std::vector<std::vector<bool>>& keys) {
        this->keys = keys;
    }

    /**
     * print the state of the notes table
     */
    std::string print_notes();

private:
    /**
     * scan through keys, figure out what chord is being keyed
     * returns empty vector if no keys pressed at all.
     */
    std::pair<uint8_t, std::vector<uint8_t>> get_chord_intervals();

    std::vector<Note> notes;
    std::vector<Note> chord;
    std::vector<std::vector<bool>> keys; // OPTIMIZE size should be defined in key_matrix?
    MidiMessenger* midi; //NOTE: make uniq_ptr?
};