#include "lpf.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>


struct IIR* create_IIR(int sizea,int sizeb,float fwd_cf,float bk_cf){
    struct IIR* bw = malloc(sizeof(struct IIR));

    bw->input_ring = malloc(sizeof(float)*sizea);
    bw->output_ring = malloc(sizeof(float)*sizeb);
    bw->feedback = malloc(sizeof(float)*sizeb);
    bw->feedfwd = malloc(sizeof(float)*sizea);

    for(int i = 0;i<sizeb;i++){
        bw->feedback[i] = bk_cf;
    }
    for(int i = 0;i<sizea;i++){

        bw->feedfwd[i] = fwd_cf;
    }
    bw->b0 = fwd_cf;
    memset(bw->input_ring,0,sizeof(float)*sizea);
    memset(bw->output_ring,0,sizeof(float)*sizeb);

    bw->feedback_end = bw->feedback + sizeb;
    bw->feedfwd_end = bw->feedfwd + sizea;

    bw->output_ring_ctr = bw->output_ring;
    bw->input_ring_ctr = bw->input_ring;
    bw->output_ring_end = bw->output_ring + sizeb;
    bw->input_ring_end = bw->input_ring + sizea;

    return bw;
}
struct IIR* create_Butterworth(float ftarg,float fs){
    float omega = 2.0f * M_PI * ftarg / fs;
    float sn = sinf(omega);
    float cs = cosf(omega);
    float alpha = sn / (2.0f * 0.707106781f);  // Q = 1/√2 ≈ 0.707

    float b0 = (1.0f - cs) / 2.0f;
    float b1 = 1.0f - cs;
    float b2 = (1.0f - cs) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cs;
    float a2 = 1.0f - alpha;
    struct IIR* bw = create_IIR(2,2,1,1);
    bw->feedback[1] = a1/a0;
    bw->feedback[0] = a2/a0;

    bw->feedfwd[1] = b1/a0;
    bw->feedfwd[0] = b2/a0;
    bw->b0 = b0/a0;
    return bw;
}
struct IIR* create_Bessel(float ftarg,float fs){
      // This design ensures unity DC gain and no peaking

    float omega = 2.0f * M_PI * ftarg / fs;
    float K = tanf(omega / 2.0f);

    // Bessel polynomial factors
    float t1 = 1.0f + sqrtf(3.0f);
    float t2 = 1.0f - sqrtf(3.0f);

    float d = K * K + K * t1 + 1.0f;

    // Original unnormalized coefficients
    float b0_temp = 1.0f / d;
    float b1_temp = 2.0f / d;
    float b2_temp = 1.0f / d;

    float a1_temp = (2.0f * K * K - 2.0f) / d;
    float a2_temp = (K * K - K * t1 + 1.0f) / d;

    // Calculate DC gain and normalize
    float dc_gain = (b0_temp + b1_temp + b2_temp) /
                    (1.0f + a1_temp + a2_temp);

    float scale = 1.0f / dc_gain;

    struct IIR* bw = create_IIR(2,2,1,1);
    bw->b0 = b0_temp * scale;
    bw->feedfwd[1] = b1_temp * scale;
    bw->feedfwd[0] = b2_temp * scale;

    bw->feedback[1] = a1_temp;
    bw->feedback[0] = a2_temp;
    return bw;
}
void reset_lpf(struct IIR* lpf,float value){
    float* input_ittr = lpf->input_ring_ctr;
    for(float* i = lpf->feedfwd;i<lpf->feedfwd_end;i++){
        *input_ittr=value;
        input_ittr++;
        if(input_ittr == lpf->input_ring_end){
            input_ittr = lpf->input_ring;
        }
    }
    //calculate output_sum
    float* output_ittr = lpf->output_ring_ctr;
    for(float* i = lpf->feedback;i<lpf->feedback_end;i++){
        *output_ittr=value;
        output_ittr++;
        if(output_ittr == lpf->output_ring_end){
            output_ittr = lpf->output_ring;
        }
    }

}
float calculate_lpf(struct IIR* lpf, float in){


    //calculate input_sum
    float* input_ittr = lpf->input_ring_ctr;
    float output = 0;
    for(float* i = lpf->feedfwd;i<lpf->feedfwd_end;i++){
        output += ((*i)*(*input_ittr));
        input_ittr++;
        if(input_ittr == lpf->input_ring_end){
            input_ittr = lpf->input_ring;
        }
    }
    //calculate output_sum
    float* output_ittr = lpf->output_ring_ctr;
    for(float* i = lpf->feedback;i<lpf->feedback_end;i++){
        output -= ((*i)*(*output_ittr));
        output_ittr++;
        if(output_ittr == lpf->output_ring_end){
            output_ittr = lpf->output_ring;
        }
    }

    output += in*lpf->b0;
    //update input and output rings
    *lpf->output_ring_ctr = output;
    lpf->output_ring_ctr++;
    if(lpf->output_ring_ctr == lpf->output_ring_end){
        lpf->output_ring_ctr = lpf->output_ring;
    }
    *lpf->input_ring_ctr = in;
    lpf->input_ring_ctr++;
    if(lpf->input_ring_ctr == lpf->input_ring_end){
        lpf->input_ring_ctr = lpf->input_ring;
    }
    return output;
}
void free_IIR(struct IIR* lpf){
    free(lpf->input_ring);
    free(lpf->output_ring);
    free(lpf->feedback);
    free(lpf->feedfwd);

}
void set_lpf_coeff(struct IIR* lpf, float* fwd_cf,float* bk_cf,float b0){
  lpf->b0 = b0;

  for(float* i = lpf->feedfwd;i<lpf->feedfwd_end;i++){
      *i = *fwd_cf;
      fwd_cf++;
  }
  for(float* i = lpf->feedback;i<lpf->feedback_end;i++){
      *i = *bk_cf;
      bk_cf++;
  }


}

