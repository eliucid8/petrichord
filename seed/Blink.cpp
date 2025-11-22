#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisy::seed;
using namespace daisysp;

//---------------------------------------------------------------------
// Hardware and constants
//---------------------------------------------------------------------
DaisySeed hw;

constexpr int kNumVoices = 32;
constexpr float kVelocityScale = 1.0f / 127.0f;

uint8_t numVoicesActive = 0;

// OPTIMIZE ugly global variables
float attack = 0.01f;
float decay = 0.1f;
float sustain = 0.7f;
float release = 0.3f;

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
        env.SetTime(ADSR_SEG_ATTACK, attack);
        env.SetTime(ADSR_SEG_DECAY, decay);
        env.SetSustainLevel(sustain);
        env.SetTime(ADSR_SEG_RELEASE, release);
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
                // first check to see if the note is already being played. deduplicates note ons
                Voice* v = FindVoiceByNote(msg.note);
                if(v == nullptr) {
                    v = FindFreeVoice();
                }
                v->NoteOn(msg.note, msg.velocity);
                hw.PrintLine("ON\t%d\t%d", msg.note, msg.velocity);
            }
            else
            {
                Voice* v = FindVoiceByNote(msg.note);
                if(v) {
                    v->NoteOff();
                    hw.PrintLine("OFF\t%d", msg.note);
                }
            }
            break;
        }

        case NoteOff:
        {
            auto msg = m.AsNoteOff();
            // hw.SetLed(true);
            Voice* v = FindVoiceByNote(msg.note);
            if(v) {
                v->NoteOff();
                hw.PrintLine("OFF\t%d", msg.note);
            }
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
    // ----Hardware init----
    hw.Init();
    hw.StartLog();

    // Create an array of AdcChannelConfig objects
    const int NUM_ADC_CHANNELS = 3;
    AdcChannelConfig my_adc_config[NUM_ADC_CHANNELS];
    // Initialize.
    // OPTIMIZE put this into functions or something
    my_adc_config[0].InitSingle(A0);
    my_adc_config[1].InitSingle(A1);
    my_adc_config[2].InitSingle(A2);
    my_adc_config[3].InitSingle(A3);
    // Initialize the ADC peripheral with that configuration
    hw.adc.Init(my_adc_config, NUM_ADC_CHANNELS);
    // Start the ADC
    hw.adc.Start();


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


    while(1)
    {
        midi_uart.Listen();

        while(midi_uart.HasEvents()) {
            HandleMidiMessage(midi_uart.PopEvent());
        }

        attack = 2 * hw.adc.GetFloat(0);
        decay = hw.adc.GetFloat(1);
        sustain = hw.adc.GetFloat(2);
        release = 3 * hw.adc.GetFloat(3);

        System::Delay(1);
    }
}
