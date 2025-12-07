#include "daisy_seed.h"
#include "daisysp.h"
#include <cstdint>
#include <algorithm>

using namespace daisy;
using namespace daisy::seed;
using namespace daisysp;

//---------------------------------------------------------------------
// Hardware and constants
//---------------------------------------------------------------------
DaisySeed hw;

constexpr int kNumVoices = 22;
constexpr float kVelocityScale = 1.0f / 127.0f;

MidiUartHandler midi_uart;
uint8_t numVoicesActive = 0;

// OPTIMIZE ugly global variables
float g_attack = 0.01f;
float g_decay = 0.1f;
float g_sustain = 0.7f;
float g_release = 0.3f;

bool debugLED = false;;

// Effects
Overdrive overdrive;
Autowah autowah;

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
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);

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
        env.SetTime(ADSR_SEG_ATTACK, g_attack);
        env.SetTime(ADSR_SEG_DECAY, g_decay);
        env.SetSustainLevel(g_sustain);
        env.SetTime(ADSR_SEG_RELEASE, g_release);
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


void HandleNoteOn(daisy::NoteOnEvent msg) {
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
}

void HandleNoteOff(NoteOffEvent msg) {
    // hw.SetLed(true);
    Voice* v = FindVoiceByNote(msg.note);
    if(v) {
        v->NoteOff();
        hw.PrintLine("OFF\t%d", msg.note);
    }
}

void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            HandleNoteOn(m.AsNoteOn());
            break;
        }

        case NoteOff:
        {
            HandleNoteOff(m.AsNoteOff());
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
        
        // global effects
        // mix = autowah.Process(mix);

        mix = overdrive.Process(mix);
        
        mix *= 1.0f / sqrtf(static_cast<float>(active_voices));
        // assuming we have a max of 16 voices running at max volume, we will end up with 4x the original range of the oscillator.
        // thus we divide by 4 to get something within output range.
        // mix *= 0.25f;
        // hw.PrintLine(FLT_FMT3, FLT_VAR3(mix));

        out[0][i] = mix;
        out[1][i] = mix;
    }
}

void bendAll(Voice voices[]) {
    for(int i = 0; i < kNumVoices; i++) {
        if(voices[i].gate) {
            float bend = (g_decay - 0.5f) * 16.0f;
            float note = voices[i].note + bend;
            float freq = mtof(note);
            hw.PrintLine(FLT_FMT3, FLT_VAR3(note));
            voices[i].osc.SetFreq(freq);
        }
    }
}

void blink(void) {
    debugLED = !debugLED;
    hw.SetLed(debugLED);
}

/**
 * init global sound effects
 * float fs: sample rate
 */
void init_effects(float fs) {
    // Create an array of AdcChannelConfig objects
    const int NUM_ADC_CHANNELS = 4;
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

    autowah.Init(fs);
    autowah.SetDryWet(80.f);
    autowah.SetLevel(0.1f);
    autowah.SetWah(0.01f);
    
    overdrive.Init();
    overdrive.SetDrive(0.1f);
}


//---------------------------------------------------------------------
// Main
//---------------------------------------------------------------------
int main(void) {
    // ----Hardware init----
    hw.Init();
    hw.StartLog();

    float audioSampleRate = hw.AudioSampleRate();
    for(int i = 0; i < kNumVoices; i++) {
        voices[i].Init(audioSampleRate);
    }
    // Initialize MIDI UART
    MidiUartHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = UartHandler::Config::Peripheral::USART_1;
   // midi_cfg.transport_config.rx.pin = seed::D15; // UART1 RX pin
    // midi_cfg.transport_config.tx.pin = seed::D16; // optional TX for MIDI Thru
    midi_uart.Init(midi_cfg);
    midi_uart.StartReceive();
    // midi_uart.SetReceiveCallback(HandleMidiMessage);

    init_effects(audioSampleRate);

    // Start audio
    hw.StartAudio(AudioCallback);
    hw.SetLed(false);
    System::Delay(1000);
    hw.PrintLine("UART MIDI PolySynth Ready! %d voices active.", kNumVoices);
    // voices[0].NoteOn(60, 40);
    // voices[1].NoteOn(64, 40);
    // voices[2].NoteOn(67, 40);
    

    int loopcounter = 0;

    while(1) {
        // DEBUG
        if(loopcounter == 1000) {
            HandleNoteOn(daisy::NoteOnEvent{0, 60, 100});
            // voices[0].NoteOn(60, 100);
        } else if (loopcounter == 1005) {
            HandleNoteOff(NoteOffEvent{0, 60, 0});
            // voices[0].NoteOff();
        } else if (loopcounter == 1010) {
            HandleNoteOn(daisy::NoteOnEvent{0, 64, 100});
            // voices[1].NoteOn(64, 100);
        } else if (loopcounter == 1015) {
            HandleNoteOff(NoteOffEvent{0, 64, 0});
            // voices[1].NoteOff();
        } else if (loopcounter == 1020) {
            HandleNoteOn(daisy::NoteOnEvent{0, 67, 100});
            // voices[2].NoteOn(67, 100);
        } else if (loopcounter == 1025) {
            HandleNoteOff(NoteOffEvent{0, 67, 0});
            // voices[2].NoteOff();
        } else if (loopcounter >= 2000) {
            loopcounter = 0;
            // voices[0].NoteOff();
            // voices[1].NoteOff();
            // voices[2].NoteOff();
        }
        loopcounter++;

        midi_uart.Listen();

        while(midi_uart.HasEvents()) {
            HandleMidiMessage(midi_uart.PopEvent());
            blink();
        }

        g_attack = 2 * hw.adc.GetFloat(0);
        g_decay = hw.adc.GetFloat(1);
        g_sustain = hw.adc.GetFloat(2);
        g_release = 3 * hw.adc.GetFloat(3);
        
        // bendAll(voices);

        System::Delay(1);
    }
}
