#include "chord_controller.h"

// TODO: check to make sure we're not duplicating notes (because we are right now)
void ChordController::handle_strum(std::vector<bool> strum_state) {
    // figure out what notes are now being held down
    std::vector<Note> held_down;
    for(int i = 0; i < strum_state.size(); ++i) {
        if(strum_state[i] && i < chord.size()) {
            held_down.push_back(chord[i]);
        }
    }
    
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
    // printf(print_notes().c_str());
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


void ChordController::update_key_state(const std::vector<std::vector<bool>>& keys) {
    this->keys = keys;

    // for (auto row : keys) {
    //     for (bool col : row) {
    //         if (col) {
    //             printf("1");
    //         } else {
    //             printf("0");
    //         }
    //     }
    //     printf("\n");
    // }

    auto chord_info = get_chord_intervals();

    // FIXME: wait until all keys are released before setting next chord.
    // prevents key releases from changing the chord.
    if(!chord_info.second.empty()) {
        chord = generate_chord(
            chord_info.first + 36, // the root + 3 octaves = 36   
            chord_info.second, // the chord intervals
            4
        );
        for(auto note : chord) {
            printf("%d ", note.pitch);
        }
        printf("\n");
    }

}


std::pair<uint8_t, std::vector<uint8_t>> ChordController::get_chord_intervals() {
    for(int i = 0; i < keys[0].size(); i++) {
        // read the keys pressed in this column as a number
        int chord_type = 0;
        const int CHORD_ROWS = 3; // there are 3 rows on the board dedicated to chord keys (Major, minor, 7th)
        // reading from bottom up
        for(int j = CHORD_ROWS-1; j >= 0; j--) {
            // FIXME: add logging
            chord_type <<= 1;
            if(keys[j][i]) {
                chord_type |= 1;
            }
        }
        if(chord_type > 0) { // if keys are pressed down
            int letter = LETTER_LAYOUT[i];
            // std::vector<uint8_t> chord_intervals = CHORD_INTERVALS[chord_type];
            // printf("letter: %d, chord_type: %d\n", letter, chord_type);
            return {letter,chord_intervals};
        }
    }
    return {};
}