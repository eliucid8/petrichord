#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>

#include "theory/chords.h"
#include "io/midi_messenger.h"

/**
 * calculates note on/off messages from given inputs.
 */
class ChordController {
public:
    ChordController(MidiMessenger* midi): midi(midi) {
        chord.push_back(Note(0, 0));
    }

    /**
     * called on strum interrupt. updates state of current notes down
     * and sends to midi_messenger
     */
    void handle_strum(std::vector<bool> state);   

    /**
     * called on chord change. 
     */
    void update_chord(std::vector<Note> chord);

    /**
     * print the state of the notes table
     */
    std::string print_notes();

private:
    std::vector<Note> notes;
    std::vector<Note> chord;
    MidiMessenger* midi; //NOTE: make uniq_ptr?
};