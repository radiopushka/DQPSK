#ifndef LPF_H
#define LPF_H

struct IIR{
    int size;
    float* input_ring;
    float* input_ring_end;
    float* input_ring_ctr;
    float* output_ring;
    float* output_ring_end;
    float* output_ring_ctr;
    float* feedback;
    float* feedback_end;
    float* feedfwd;
    float* feedfwd_end;
    float b0;
};

struct IIR* create_IIR(int sizea,int sizeb,float fwd_cf,float bk_cf);

struct IIR* create_Butterworth(float ftarg,float fs);

struct IIR* create_Bessel(float ftarg,float fs);

float calculate_lpf(struct IIR* lpf, float in);

void set_lpf_coeff(struct IIR* lpf, float* fwd_cf,float* bk_cf,float b0);

void free_IIR(struct IIR* lpf);
void reset_lpf(struct IIR* lpf,float value);
#endif
