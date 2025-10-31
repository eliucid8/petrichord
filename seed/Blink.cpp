#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

//---------------------------------------------------------------------
// Hardware and constants
//---------------------------------------------------------------------
DaisySeed hw;

constexpr int kNumVoices = 32;
constexpr float kVelocityScale = 1.0f / 127.0f;

uint8_t numVoicesActive = 0;

//---------------------------------------------------------------------
// Voice definition
//---------------------------------------------------------------------
struct Voice
{
    Oscillator osc;
    Adsr       env;
    bool       active;
    bool       gate;
    uint8_t    note;
    float      velocity;

    void Init(float samplerate)
    {
        osc.Init(samplerate);
        osc.SetWaveform(Oscillator::WAVE_SAW);

        env.Init(samplerate);
        env.SetTime(ADSR_SEG_ATTACK, 0.01f);
        env.SetTime(ADSR_SEG_DECAY, 0.1f);
        env.SetSustainLevel(0.7f);
        env.SetTime(ADSR_SEG_RELEASE, 0.3f);

        active   = false;
        gate     = false;
        note     = 0;
        velocity = 0.0f;
    }

    void NoteOn(uint8_t note_in, uint8_t vel)
    {
        float freq = mtof(note_in);
        osc.SetFreq(freq);
        velocity = vel * kVelocityScale;
        gate     = true;
        note     = note_in;
        active   = true;
    }

    void NoteOff()
    {
        gate = false;
    }

    float Process()
    {
        if(!active)
            return 0.0f;

        float env_out = env.Process(gate);
        float sig     = osc.Process() * env_out * velocity;

        // Voice cleanup when envelope is nearly 0
        if(!gate && env_out < 0.0001f)
        {
            active = false;
        }

        // hw.SetLed(true);

        // hw.SetLed(true);
        return sig;
    }
};

//---------------------------------------------------------------------
// Polyphony management
//---------------------------------------------------------------------
Voice voices[kNumVoices];

Voice* FindFreeVoice()
{
    for(int i = 0; i < kNumVoices; i++)
        if(!voices[i].active)
            return &voices[i];
    return &voices[0]; // voice stealing
}

Voice* FindVoiceByNote(uint8_t note)
{
    for(int i = 0; i < kNumVoices; i++)
        if(voices[i].active && voices[i].note == note) {
            // hw.SetLed(true);
            return &voices[i];
        }
    return nullptr;
}

MidiUartHandler midi_uart;

void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            auto msg = m.AsNoteOn();
            if(msg.velocity > 0)
            {
                Voice* v = FindFreeVoice();
                v->NoteOn(msg.note, msg.velocity);
            }
            else
            {
                Voice* v = FindVoiceByNote(msg.note);
                if(v)
                    v->NoteOff();
            }
            break;
        }

        case NoteOff:
        {
            auto msg = m.AsNoteOff();
            // hw.SetLed(true);
            Voice* v = FindVoiceByNote(msg.note);
            if(v)
                v->NoteOff();
            break;
        }

        default: break;
    }
}

//---------------------------------------------------------------------
// Audio callback
//---------------------------------------------------------------------
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t                     size)
{
    for(size_t i = 0; i < size; i++)
    {
        float mix = 0.0f;
        int active_voices = 0;
        for(int v = 0; v < kNumVoices; v++) {
            if(voices[v].active) {
                mix += voices[v].Process();
                active_voices++;
            }
        }

        mix *= 1.0f / sqrtf(static_cast<float>(active_voices));

        mix *= 2.0f;
        out[0][i] = mix;
        out[1][i] = mix;
    }
}

struct RhythmTrack
{
    uint32_t period_ms;     // interval between note-ons
    uint32_t duration_ms;   // note length
    uint8_t  note;
    float    velocity;
    uint32_t last_on;
    bool     playing;
    Voice*   v;
};

constexpr int kNumTracks = 4;
RhythmTrack tracks[kNumTracks] = {
    {200, 100, 60, 0.9f, 0, false, nullptr}, // 5Hz
    {300, 100, 64, 0.9f, 0, false, nullptr}, // 3.3Hz
    {500, 200, 67, 0.9f, 0, false, nullptr}, // 2Hz
    {700, 250, 72, 0.9f, 0, false, nullptr}, // 1.4Hz
};

//---------------------------------------------------------------------
// Main
//---------------------------------------------------------------------
int main(void)
{
    hw.Init();
    hw.StartLog();

    float sr = hw.AudioSampleRate();
    for(int i = 0; i < kNumVoices; i++)
        voices[i].Init(sr);

    // Initialize MIDI UART
    MidiUartHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = UartHandler::Config::Peripheral::USART_1;
    // midi_cfg.transport_config.rx.pin = seed::D15; // UART1 RX pin
    // midi_cfg.transport_config.tx.pin = seed::D16; // optional TX for MIDI Thru
    midi_uart.Init(midi_cfg);
    midi_uart.StartReceive();
    // midi_uart.SetReceiveCallback(HandleMidiMessage);

    // Start audio
    hw.StartAudio(AudioCallback);
    hw.SetLed(false);
    hw.PrintLine("UART MIDI PolySynth Ready! 32 voices active.");
    // v1->NoteOff();

    Voice *v1 = FindFreeVoice();
    Voice *v2 = FindFreeVoice();
    v1->NoteOn(60, 127);
    v2->NoteOn(66, 127);

    while(1)
    {
        midi_uart.Listen();

        while(midi_uart.HasEvents()) {
            
            HandleMidiMessage(midi_uart.PopEvent());
        }

        System::Delay(1);
    }
}
