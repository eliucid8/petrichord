#include "chord_controller.h"

void ChordController::update_note(uint8_t plate_number) {
    note_number = plate_number;
    playing_note.velocity = 127; // TODO: make velocity adjustable via IMU later

    if(playing_note.pitch != 0) {
        // turn off previous note
        midi->send_midi_note_off(playing_note);
    }

    printf("Strum plate %d selected\n", plate_number);
    if(plate_number < chord.size()) {
        // valid note
        playing_note.pitch = chord[plate_number].pitch;
        midi->send_midi_note_on(playing_note);
    } else {
        playing_note.pitch = 0;
    }
}

// TODO: figure out what happens when you're still holding a strum down but you change the chord
void ChordController::update_chord(std::vector<Note> chord) {
    this->chord = chord;
}

std::string ChordController::print_note() {
    std::string ret;
    ret.append(std::to_string(playing_note.pitch));
    ret.append(", ");
    ret.append(std::to_string(playing_note.velocity));
    ret.append("\n");
    return ret;
}

void ChordController::update_key_state(const std::vector<std::vector<bool>>& keys) {
    this->keys = keys;
    auto chord_info = get_chord_intervals();

    // prevents key releases from changing the chord.
    if(!chord_info.second.empty()) {
        chord = generate_chord(
            chord_info.first + 36, // the root + 3 octaves = 36   
            chord_info.second, // the chord intervals
            4
        );
        // FIXME: add this to keypress logging
        // printf("new chord set: ");
        // for(auto note : chord) {
        //     printf("%d ", note.pitch);
        // }
        // printf("\n");
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
            std::vector<uint8_t> chord_intervals = CHORD_INTERVALS[chord_type];
            
            // FLAT AND SHARP MODIFIERS
            // OPTIMIZE is there somewhere cleaner to put these?
            if(keys[3][0]) {
                ++letter;
            }
            if(keys[3][1]) {
                --letter;
            }
            // printf("letter: %d, chord_type: %d\n", letter, chord_type);
            return {letter, chord_intervals};
        }
    }
    return {};
}