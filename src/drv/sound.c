#include "sound.h"
#include "../libk/ports.h"
#include "../libk/string.h"
#include "../libk/debug/log.h"
#include "../libk/math.h"
#include "stdbool.h"
#include "stddef.h"
#include "../cpu/isr.h"

static const double NOTES[NUM_OCTAVES * OCTAVE_SIZE] = {
    // O1
    32.703195662574764,
    34.647828872108946,
    36.708095989675876,
    38.890872965260044,
    41.203444614108669,
    43.653528929125407,
    46.249302838954222,
    48.99942949771858,
    51.913087197493056,
    54.999999999999915,
    58.270470189761156,
    61.735412657015416,

    // O2
    65.406391325149571,
    69.295657744217934,
    73.416191979351794,
    77.781745930520117,
    82.406889228217381,
    87.307057858250872,
    92.4986056779085,
    97.998858995437217,
    103.82617439498618,
    109.99999999999989,
    116.54094037952237,
    123.4708253140309,

    // O3
    130.8127826502992,
    138.59131548843592,
    146.83238395870364,
    155.56349186104035,
    164.81377845643485,
    174.61411571650183,
    184.99721135581709,
    195.99771799087452,
    207.65234878997245,
    219.99999999999989,
    233.08188075904488,
    246.94165062806198,

    // O4
    261.62556530059851,
    277.18263097687202,
    293.66476791740746,
    311.12698372208081,
    329.62755691286986,
    349.22823143300383,
    369.99442271163434,
    391.99543598174927,
    415.30469757994513,
    440,
    466.16376151808993,
    493.88330125612413,

    // O5
    523.25113060119736,
    554.36526195374427,
    587.32953583481526,
    622.25396744416196,
    659.25511382574007,
    698.456462866008,
    739.98884542326903,
    783.99087196349899,
    830.60939515989071,
    880.00000000000034,
    932.32752303618031,
    987.76660251224882,

    // O6
    1046.5022612023952,
    1108.7305239074892,
    1174.659071669631,
    1244.5079348883246,
    1318.5102276514808,
    1396.9129257320169,
    1479.977690846539,
    1567.9817439269987,
    1661.2187903197821,
    1760.000000000002,
    1864.6550460723618,
    1975.5332050244986,

    // O7
    2093.0045224047913,
    2217.4610478149793,
    2349.3181433392633,
    2489.0158697766506,
    2637.020455302963,
    2793.8258514640347,
    2959.9553816930793,
    3135.9634878539991,
    3322.437580639566,
    3520.0000000000055,
    3729.3100921447249,
    3951.0664100489994,
};

#define MIXER_IRQ       0x5
#define MIXER_IRQ_DATA  0x2

// SB16 ports
#define DSP_MIXER       0x224
#define DSP_MIXER_DATA  0x225
#define DSP_RESET       0x226
#define DSP_READ        0x22A
#define DSP_WRITE       0x22C
#define DSP_READ_STATUS 0x22E
#define DSP_ACK_8       DSP_READ_STATUS
#define DSP_ACK_16      0x22F

// TODO: ???
#define DSP_PROG_16     0xB0
#define DSP_PROG_8      0xC0
#define DSP_AUTO_INIT   0x06
#define DSP_PLAY        0x00
#define DSP_RECORD      0x08
#define DSP_MONO        0x00
#define DSP_STEREO      0x20
#define DSP_UNSIGNED    0x00
#define DSP_SIGNED      0x10

#define DMA_CHANNEL_16  5
#define DMA_FLIP_FLOP   0xD8
#define DMA_BASE_ADDR   0xC4
#define DMA_COUNT       0xC6

// commands for DSP_WRITE
#define DSP_SET_TIME    0x40
#define DSP_SET_RATE    0x41
#define DSP_ON          0xD1
#define DSP_OFF         0xD3
#define DSP_OFF_8       0xD0
#define DSP_ON_8        0xD4
#define DSP_OFF_16      0xD5
#define DSP_ON_16       0xD6
#define DSP_VERSION     0xE1

// commands for DSP_MIXER
#define DSP_VOLUME  0x22
#define DSP_IRQ     0x80

#define SAMPLE_RATE     48000
#define BUFFER_MS       30

#define BUFFER_SIZE ((size_t) (SAMPLE_RATE * (BUFFER_MS / 1000.0)))
static short buffer[BUFFER_SIZE];
static bool buffer_flip = false;

static uint64_t sample = 0;

static uint8_t volume_master;
static uint8_t volumes[NUM_NOTES];
static uint8_t notes[NUM_NOTES];
static uint8_t waves[NUM_NOTES];

void sound_note(uint8_t index, uint8_t octave, uint8_t note) {
    notes[index] = (octave << 4) | note;
}

void sound_volume(uint8_t index, uint8_t v) {
    volumes[index] = v;
}

void sound_master(uint8_t v) {
    volume_master = v;
}

void sound_wave(uint8_t index, uint8_t wave) {
    waves[index] = wave;
}

static void fill(short *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        double f = 0.0;

        for (size_t j = 0; j < NUM_NOTES; j++) {
            uint8_t octave = (notes[j] >> 4) & 0xF,
               note = notes[j] & 0xF;

            if (note == NOTE_NONE) {
                continue;
            }

            double note_freq = NOTES[octave * OCTAVE_SIZE + note],
                   freq = note_freq / (double) SAMPLE_RATE,
                   d = 0.0,
                   offset = 0.0;

            switch (waves[j]) {
                case WAVE_SIN:
                    d = sin(2.0 * PI * sample * freq);
                    break;
                case WAVE_SQUARE:
                    d = sin(2.0 * PI * sample * freq) >= 0.0 ? 1.0 : -1.0;
                    break;
                case WAVE_TRIANGLE:
                    d = fabs(fmod(4 * (sample * freq) + 1.0, 4.0) - 2.0) - 1;
                    break;
                case WAVE_NOISE:
                    offset = (freq * 128.0) * ((rand() / 4294967295.0) - 0.5);
                    d = fabs(fmod(4 * (sample * freq + offset) + 1.0, 4.0) - 2.0) - 1;
                    break;
            }

            d *= (volumes[j] / 255.0);
            f += d;
        }

        buf[i] = (short) (((volume_master / 255.0) * 4096.0) * f);

        sample++;

        // avoid double overflow errors, instead just mess up one note every
        // few minutes
        sample %= (1 << 24);
    }
}

static void dsp_write(uint8_t b) {
    while (inportb(DSP_WRITE) & 0x80);
    outportb(DSP_WRITE, b);
}

static void dsp_read(uint8_t b) {
    while (inportb(DSP_READ_STATUS) & 0x80);
    outportb(DSP_READ, b);
}

static void reset() {
    char buf0[128], buf1[128];

    outportb(DSP_RESET, 1);

    // TODO: maybe not necessary
    // ~3 microseconds?
    for (size_t i = 0; i < 1000000; i++);

    outportb(DSP_RESET, 0);

    uint8_t status = inportb(DSP_READ_STATUS);
    if (~status & 128) {
        goto fail;
    }

    status = inportb(DSP_READ);
    if (status != 0xAA) {
        goto fail;
    }

    outportb(DSP_WRITE, DSP_VERSION);
    uint8_t major = inportb(DSP_READ),
       minor = inportb(DSP_READ);

    if (major < 4) {
        status = (major << 4) | minor;
        goto fail;
    }

    return;
fail:
    strncpy(buf0, "FAILED TO RESET SB16: ", 128);
    itoa(status, buf1);
    strncat(buf0, buf1, 128);
    log(buf0, 3, 1);
}

static void set_sample_rate(uint16_t hz) {
    dsp_write(DSP_SET_RATE);
    dsp_write((uint8_t) ((hz >> 8) & 0xFF));
    dsp_write((uint8_t) (hz & 0xFF));
}

static void transfer(void *buf, uint32_t len) {
    uint8_t mode = 0x48;

    // disable DMA channel
    outportb(DSP_ON_8, 4 + (DMA_CHANNEL_16 % 4));

    // clear byte-poiner flip-flop
    outportb(DMA_FLIP_FLOP, 1);

    // write DMA mode for transfer
    outportb(DSP_ON_16, (DMA_CHANNEL_16 % 4) | mode | (1 << 4));

    // write buffer offset (div 2 for 16-bit)
    uint16_t offset = (((uintptr_t) buf) / 2) % 65536;
    outportb(DMA_BASE_ADDR, (uint8_t) ((offset >> 0) & 0xFF));
    outportb(DMA_BASE_ADDR, (uint8_t) ((offset >> 8) & 0xFF));

    // write transfer length
    outportb(DMA_COUNT, (uint8_t) (((len - 1) >> 0) & 0xFF));
    outportb(DMA_COUNT, (uint8_t) (((len - 1) >> 8) & 0xFF));

    // write buffer
    outportb(0x8B, ((uintptr_t) buf) >> 16);

    // enable DMA channel
    outportb(0xD4, DMA_CHANNEL_16 % 4);
}

static void sb16_irq_handler() {
    buffer_flip = !buffer_flip;

    fill(
        &buffer[buffer_flip ? 0 : (BUFFER_SIZE / 2)],
        (BUFFER_SIZE / 2)
    );

    inportb(DSP_READ_STATUS);
    inportb(DSP_ACK_16);
}

static void configure() {
    register_interrupt_handler(MIXER_IRQ, sb16_irq_handler, "SB16 IRQ");
    outportb(DSP_MIXER, DSP_IRQ);
    outportb(DSP_MIXER_DATA, MIXER_IRQ_DATA);

    uint8_t v = MIXER_IRQ;
    if (v != MIXER_IRQ) {
        char buf0[128], buf1[128];
        itoa(v, buf0);
        strncpy(buf1, "SB16 HAS INCORRECT IRQ: ", 128);
        strncat(buf1, buf0, 128);
        log(buf1, 3, 1);
    }
}

void sound_init() {
    register_interrupt_handler(MIXER_IRQ, sb16_irq_handler, "SB16 IRQ");
    reset();
    configure();

    transfer(buffer, BUFFER_SIZE);
    set_sample_rate(SAMPLE_RATE);

    uint16_t sample_count = (BUFFER_SIZE / 2) - 1;
    dsp_write(DSP_PLAY | DSP_PROG_16 | DSP_AUTO_INIT);
    dsp_write(DSP_SIGNED | DSP_MONO);
    dsp_write((uint8_t) ((sample_count >> 0) & 0xFF));
    dsp_write((uint8_t) ((sample_count >> 8) & 0xFF));

    dsp_write(DSP_ON);
    dsp_write(DSP_ON_16);

    memset(&notes, NOTE_NONE, sizeof(notes));
    memset(&waves, WAVE_SIN, sizeof(waves));
}