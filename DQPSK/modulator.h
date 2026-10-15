#ifndef dqpskMOD
#define dqpskMOD
#include "./IIR/lpf.h"
#include<stdint.h>


#define M_2PI 2 * M_PI
#define M_PIH M_PI / 2.0

struct dqpsk_demod{

    float phase;
    float offset;
    float shifter;

    float pr;
    float pi;

    float* phase_history;

    int prev_symbol;
    int symbol_list[8];
    int delta_captured;



    struct IIR* lpf_q_1;
    struct IIR* lpf_q_2;
    struct IIR* lpf_i_1;
    struct IIR* lpf_i_2;
    int dcount;
    int dcount_max;


    float p_i;
    float p_q;


    uint16_t frame;

    int packet_size;

    int bit_count;

    int preamb_manchester;//0 for preamble, 1 for manchester encoding


};

struct dqpsk_mod{
    float shifter;
    float phase;
    float offset;

    struct IIR* lpf_mod;
    struct IIR* lpf_mod_2;


    unsigned char* data;
    unsigned char* d_end;
    unsigned char* d_index;

    int bit_count;

    int count;
    int dcount_max;

    float value;

    int preamb_manchester;//0 for preamble, 1 for manchester encoding
    float baseline;
};

//dqpsk demodulator
struct dqpsk_demod* create_dqpsk_demod(float carrier_freq,int tx_rate,int sample_rate);
void free_dqpsk_demod(struct dqpsk_demod* dqpsk);
//dqpsk modulator
struct dqpsk_mod* create_dqpsk_mod(float carrier_freq,int tx_rate,int sample_rate,float drive);
void free_dqpsk_mod(struct dqpsk_mod* dqpsk);



int run_dqpsk_demod(struct dqpsk_demod* dqpsk,short* in,char* obuffer,int size);//return the number of characters received
int run_dqpsk_mod(struct dqpsk_mod* dqpsk,short* audio,int size);//return 1 if the entirity of data has been transmitter
    //for generating SSB on IQ modulators
    //if flip is set then the 90 degree and zero degree channels are flipped
    //offset is the phase offset for best rejection
int run_dqpsk_mod_IQ(struct dqpsk_mod* dqpsk,short* audio,int size,float offset,int flip);//return 1 if the entirity of data has been transmitter

void set_dqpsk_data(struct dqpsk_mod* dqpsk, unsigned char* data,int length);//run after dqpsk_mod returns 1 to set new data
#endif
