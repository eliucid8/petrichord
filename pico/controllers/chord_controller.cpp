#include "chord_controller.h"

// TODO: check to make sure we're not duplicating notes (because we are right now)
void ChordController::handle_strum(std::vector<bool> strum_state) {
    // // DEBUG
    // for(bool state : strum_state) {
    //     printf("%b", state);
    // }
    // printf("\n");

    // figure out what notes are now being held down
    std::vector<Note> held_down;
    for(int i = 0; i < strum_state.size(); ++i) {
        if(strum_state[i] && i < chord.size()) {
            held_down.push_back(chord[i]);
        }
    }
    // DEBUG
    for(Note n : held_down) {
        printf("%d ", n.pitch);
    }
    printf("\n");
    
    // send note ons/offs for everything that changed
    // N^2 algorithm. sue me.
    // OPTIMIZE: I know the optimal algorithm uses hashmaps gah
    for(int i = 0; i < notes.size(); ++i) {
        Note note = notes[i];
        auto it = std::find(held_down.begin(), held_down.end(), note);
        if(it == held_down.end()) {
            // no match found, remove this from the vector.
            std::swap(note, notes.back());
            midi->send_midi_note_off(note);
            notes.pop_back();
        } else {
            // match found, remove this from held_down.
            notes[i].pitch = it->pitch; // FIX: is the note struct mutable?
            std::swap(*it, held_down.back());
            held_down.pop_back();
        }
    }
    
    for(Note new_note : held_down) {
        midi->send_midi_note_on(new_note);
        notes.push_back(new_note);
    }
    printf(print_notes().c_str());
}

// TODO: figure out what happens when you're still holding a strum down but you change the chord
void ChordController::update_chord(std::vector<Note> chord) {
    this->chord = chord;
}

std::string ChordController::print_notes() {
    std::string ret;
    for(Note n : notes) {
        ret.append(std::to_string(n.pitch));
        ret.append(" ");
    }
    ret.append("\n");
    return ret;
}