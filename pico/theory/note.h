#pragma once

#include <cstdint>
#include <algorithm>

struct Note {
    uint8_t pitch;    // MIDI pitch (0-127)
    uint8_t velocity; // MIDI velocity (0-127)

    Note() : pitch(0), velocity(0) {}
    Note(uint8_t pitch, uint8_t velocity) : pitch(pitch), velocity(velocity) {}

    bool operator==(const Note& other) const {
        return (pitch == other.pitch);
    }

    // // Add semitones to pitch
    // Note operator+(int semitones) const {
    //     int newPitch = std::clamp(static_cast<int>(pitch) + semitones, 0, 127);
    //     return Note(static_cast<uint8_t>(newPitch), velocity);
    // }

    // // Subtract semitones from pitch
    // Note operator-(int semitones) const {
    //     int newPitch = std::clamp(static_cast<int>(pitch) - semitones, 0, 127);
    //     return Note(static_cast<uint8_t>(newPitch), velocity);
    // }

    // // Compound assignment operators
    // Note& operator+=(int semitones) {
    //     pitch = static_cast<uint8_t>(std::clamp(static_cast<int>(pitch) + semitones, 0, 127));
    //     return *this;
    // }

    // Note& operator-=(int semitones) {
    //     pitch = static_cast<uint8_t>(std::clamp(static_cast<int>(pitch) - semitones, 0, 127));
    //     return *this;
    // }
    
    // // Compound assignment operators
    // Note& operator+=(int semitones) {
    //     pitch = static_cast<uint8_t>(std::clamp(static_cast<int>(pitch) + semitones, 0, 127));
    //     return *this;
    // }

    // Note& operator-=(int semitones) {
    //     pitch = static_cast<uint8_t>(std::clamp(static_cast<int>(pitch) - semitones, 0, 127));
    //     return *this;
    // }
};