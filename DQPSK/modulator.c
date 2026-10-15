#include "modulator.h"
#include<math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct dqpsk_demod* create_dqpsk_demod(float carrier_freq,int tx_rate,int sample_rate){
    struct dqpsk_demod* dqpsk = malloc(sizeof(struct dqpsk_demod));
    dqpsk->dcount = 0;
    dqpsk->dcount_max = sample_rate/tx_rate;

    dqpsk->delta_captured=0;


    dqpsk->phase_history = malloc(sizeof(float)*dqpsk->dcount_max);

    memset(dqpsk->phase_history,0,dqpsk->dcount_max*sizeof(float));

    dqpsk->phase=0;
    float rate_f = sample_rate;
    float txrate_f = tx_rate;
    dqpsk->shifter = 0;
    float spread = 4;

    float tightness = 1.1;

    float freq1 = carrier_freq;

    dqpsk->offset = (freq1/rate_f)*(M_PI*2);
    float filter_freq = txrate_f*tightness;

    dqpsk->frame = 0;

    dqpsk->lpf_q_1 = create_Bessel(filter_freq,rate_f);
    dqpsk->lpf_i_1 = create_Bessel(filter_freq,rate_f);

    dqpsk->bit_count = 0;
    dqpsk->packet_size = 0;

    dqpsk->preamb_manchester = 0;
    dqpsk->pr=0.0f;
    dqpsk->pi=0.0f;
    dqpsk->prev_symbol=0;

    return dqpsk;
}

struct dqpsk_mod* create_dqpsk_mod(float carrier_freq,int tx_rate,int sample_rate,float drive){
    struct dqpsk_mod* dqpsk = malloc(sizeof(struct dqpsk_mod));

    float rate_f = sample_rate;
    float txrate_f = tx_rate;
    dqpsk->shifter = 0;
    dqpsk->phase=0;
    dqpsk->offset = (carrier_freq/rate_f)*(M_PI*2);
    float filter_freq = txrate_f*1.5;

    dqpsk->lpf_mod = create_Bessel(filter_freq,rate_f);

    float spread = 4;

    dqpsk->bit_count = 0;

    dqpsk->data = 0;
    dqpsk->d_end = 0;
    dqpsk->value = -1;
    dqpsk->d_index = 0;


    dqpsk->count = 0;
    dqpsk->dcount_max = sample_rate/tx_rate;

    dqpsk->baseline = drive;

    dqpsk->preamb_manchester = 0;
    return dqpsk;
}
void free_dqpsk_mod(struct dqpsk_mod* dqpsk){
    free_IIR(dqpsk->lpf_mod);
    if(dqpsk->data){
        free(dqpsk->data);
    }
    free(dqpsk);

}

void free_dqpsk_demod(struct dqpsk_demod* dqpsk){
    free_IIR(dqpsk->lpf_q_1);
    free_IIR(dqpsk->lpf_i_1);
    free(dqpsk->phase_history);
    free(dqpsk);
}
int _on_rx(uint16_t data,struct dqpsk_demod* dqpsk){
    unsigned char* as_str = (unsigned char*)&data;
    unsigned char delim1 = as_str[0];
    unsigned char delim2 = as_str[1];
    if(delim1 == ((~delim2)&255) && dqpsk->bit_count >= 16){
        dqpsk->bit_count = 2;
        //printf("Got %c\n",delim2);
        return delim2;

    }
    dqpsk->bit_count+=2;
    return -1;
}
int _on_rx_amb(uint16_t data,struct dqpsk_demod* dqpsk){
    unsigned char* as_str = (unsigned char*)&data;
    unsigned char delim1 = as_str[0];
    unsigned char delim2 = as_str[1];
    if((delim1 ==  ((~delim2)&255))&& dqpsk->packet_size == -1 && dqpsk->bit_count >= 16){
        dqpsk->bit_count = 2;
        dqpsk->packet_size = delim2;
        return -1;
    }else if((delim1 ==  0xAA && delim2 == 0xF9)&& dqpsk->packet_size == 0){
        dqpsk->bit_count = 2;
        dqpsk->packet_size = -1;
        return -1;

    }else if(dqpsk->bit_count >= 16){
        dqpsk->bit_count=2;
        if(dqpsk->packet_size == 1){
            dqpsk->packet_size = 0;
            return delim2;
        }else if(dqpsk->packet_size > 0){
            dqpsk->packet_size = dqpsk->packet_size-2;
            return -2;
        }
        if(dqpsk->packet_size == -1){
            dqpsk->packet_size = 0;
        }
        return -1;
    }
    dqpsk->bit_count+=2;
    if(dqpsk->bit_count == 1600){
            dqpsk->bit_count = 16;
    }

    return -1;
}

int dqpsk_p_delta_map(float delta_p){
  float M_PIH_half=M_PIH/2.0f;
  if(delta_p > M_PIH + M_PIH_half ){
    return 3;
  }
  if(delta_p > M_PIH_half ){
    return 1;
  }
  if(delta_p < M_PIH_half && delta_p > -M_PIH_half ){
    return 0;
  }
  return 2;



}
int pre_amble[]={3,1,3,1,3,1,3,1};
int run_dqpsk_demod(struct dqpsk_demod* dqpsk,short* in,char* obuffer,int size){

    int chars = 0;
    for(short* i = in;i<in+size;i++){
        float in = *i;

        float aq = sinf(dqpsk->shifter)*in;
        float ai = cosf(dqpsk->shifter)*in;
        ai = calculate_lpf(dqpsk->lpf_i_1,ai);
        aq = calculate_lpf(dqpsk->lpf_q_1,aq);


        
        float deltaph = atan2f(ai*dqpsk->pr - aq*dqpsk->pi,   // cross  (sin Δθ)
        ai*dqpsk->pi + aq*dqpsk->pr);  // dot    (cos Δθ)
        dqpsk->phase=dqpsk->phase+deltaph;

        dqpsk->pr=aq;
        dqpsk->pi=ai;

        dqpsk->shifter +=dqpsk->offset;
        if(dqpsk->shifter >= M_2PI){
            dqpsk->shifter -= M_2PI;

        }
    ///phase tracker array and update the coefficients
        float* end_ptr=dqpsk->phase_history+dqpsk->dcount_max;
        float exiled_value=*(end_ptr-1);
        /*if(exiled_value > M_2PI){
            exiled_value=M_2PI;
        }else if(-exiled_value > M_2PI){
            exiled_value=-M_2PI;
        }else{
          exiled_value=0.0f;
        }*/

        for(float* i=end_ptr-1;i>dqpsk->phase_history;i--){
          float prev=*(i-1);
          *i=prev-exiled_value;
          

        }

        dqpsk->phase=dqpsk->phase-exiled_value;
        *(dqpsk->phase_history)=dqpsk->phase;
        //float curr_delta= dqpsk->phase - *(end_ptr-1);
        float curr_delta=  dqpsk->phase - *(end_ptr-1);
        
     



        int symbol_preamb=0;
        //detect preamble
        symbol_preamb=dqpsk_p_delta_map(curr_delta);
    /*
     * 3 1 3 1 3 1 3 1      
     */
        if(dqpsk->prev_symbol!=symbol_preamb){

          for(int i=7;i>0;i--){
            dqpsk->symbol_list[i]=dqpsk->symbol_list[i-1];
          }
          dqpsk->symbol_list[0]=symbol_preamb;

        }
        if(memcmp(dqpsk->symbol_list,pre_amble,sizeof(int)*8)==0){
          for(int i=7;i>=0;i--){
            dqpsk->symbol_list[i]=0;
          }
          dqpsk->dcount=-dqpsk->dcount_max/2;

        }

        dqpsk->prev_symbol=symbol_preamb; 
        
        if(dqpsk->dcount>=dqpsk->dcount_max){
            dqpsk->frame = ((dqpsk->frame)<<2)|symbol_preamb;
            int out = 0;
            if(dqpsk->preamb_manchester){
                out = _on_rx(dqpsk->frame,dqpsk);
            }else{
                out = _on_rx_amb(dqpsk->frame,dqpsk);
            }
            if(out == -2){
                unsigned char* as_str = (unsigned char*)&dqpsk->frame;
                unsigned char delim1 = as_str[0];
                unsigned char delim2 = as_str[1];
                *obuffer = delim2;
                obuffer++;
                chars++;
                *obuffer = delim1;
                obuffer++;
                chars++;

            }else if(out!=-1){
                *obuffer = out;
                obuffer++;
                chars++;
            }
            dqpsk->dcount = 0;

        }
        dqpsk->dcount++;




    }
    return chars;
}

int run_dqpsk_mod(struct dqpsk_mod* dqpsk,short* audio,int size){



    int done = (dqpsk->d_index >= dqpsk->d_end);
    for(short* i = audio;i<audio+size;i++){
        dqpsk->count++;
        if(dqpsk->count >= dqpsk->dcount_max){
            if(!done){
                int datav = *(dqpsk->d_index);
                datav=datav>>(6-dqpsk->bit_count);
                int cmpr = 3&datav;
                switch(cmpr){
                    case 1:
                      dqpsk->value=dqpsk->value+M_PIH;
                      break;
                    case 2:
                      dqpsk->value=dqpsk->value-M_PIH;
                      break;
                    case 3:
                      dqpsk->value=dqpsk->value+M_PI;
                      break;
    
                }


                dqpsk->bit_count+=2;
            }          
            if(dqpsk->bit_count >= 8){

                dqpsk->bit_count = 0;
                dqpsk->d_index++;
                if(dqpsk->d_index >= dqpsk->d_end){
                    done = 1;
                }
            }
            dqpsk->count = 0;
        }

        float shifter = dqpsk->value;
        float filter = calculate_lpf(dqpsk->lpf_mod,shifter);




            float osc = sinf(dqpsk->shifter+filter);
            dqpsk->shifter +=(dqpsk->offset);

            if(dqpsk->shifter >= M_2PI){
                dqpsk->shifter -= M_2PI;
            }
            *i = (short)((osc)*(dqpsk->baseline));

    }
    return done;
}
int run_dqpsk_mod_IQ(struct dqpsk_mod* dqpsk,short* audio,int size,float offset,int flip){



    int done = (dqpsk->d_index >= dqpsk->d_end);
    for(short* i = audio;i<audio+(size<<1);i = i+2){
        dqpsk->count++;
        if(dqpsk->count >= dqpsk->dcount_max){
            if(!done){
                int datav = *(dqpsk->d_index);
                datav=datav>>(6-dqpsk->bit_count);
                int cmpr = 3&datav;
                switch(cmpr){
                    case 1:
                      dqpsk->value=dqpsk->value+M_PIH;
                      break;
                    case 2:
                      dqpsk->value=dqpsk->value-M_PIH;
                      break;
                    case 3:
                      dqpsk->value=dqpsk->value+M_PI;
                      break;
    
                }


                dqpsk->bit_count+=2;
            }
            if(dqpsk->bit_count >= 8){
                dqpsk->bit_count = 0;
                dqpsk->d_index++;
                if(dqpsk->d_index >= dqpsk->d_end){
                    done = 1;
                }
            }
            dqpsk->count = 0;
        }

        float shifter = dqpsk->value;
        float filter = calculate_lpf(dqpsk->lpf_mod,shifter);


            float osc = sinf(dqpsk->shifter + filter);
            float osc_90 = cosf(dqpsk->shifter + offset + filter);
            dqpsk->shifter +=(dqpsk->offset);
            if(dqpsk->shifter >= M_2PI){
                dqpsk->shifter -= M_2PI;
            }
            if(flip){
                *i = (short)((osc_90)*(dqpsk->baseline));
                *(i+1) = (short)((osc)*(dqpsk->baseline));
            }else{
                *i = (short)((osc)*(dqpsk->baseline));
                *(i+1) = (short)((osc_90)*(dqpsk->baseline));
            }

    }
    return done;
}

void _set_dqpsk_data_amb(struct dqpsk_mod* dqpsk, unsigned char* data,int length){

    if(dqpsk->data)
    {
        free(dqpsk->data);
    }
    int len = length+8;
    dqpsk->data = malloc(sizeof(char)*len);
    unsigned char* d = dqpsk->data;
    //need the receiver to stabilize
    *d = 0x00;
    d++;

    *d = 0x00;
    d++;
    *d = 0xDD;
    d++;
    *d = 0xDD;
    d++;
    *d = 0xF9;
    d++;
    *d = 0xAA;
    d++;


    *d = length;
    d++;
    *d = ~length;
    d++;

    for(unsigned char* i = data;i<data+length;i++){
        *d = *i;
        d++;
       }
    dqpsk->d_index = dqpsk->data;
    dqpsk->d_end = dqpsk->data+len;
    //dqpsk->value=0.0f;

    while(dqpsk->value>M_2PI){
      dqpsk->value-=M_2PI;
    }
    while(dqpsk->value<-M_2PI){
      dqpsk->value+=M_2PI;
    }


   
    reset_lpf(dqpsk->lpf_mod,dqpsk->value);


}


void set_dqpsk_data(struct dqpsk_mod* dqpsk, unsigned char* data,int length){

    if(!dqpsk->preamb_manchester){
        _set_dqpsk_data_amb(dqpsk,data,length);
        return;
    }
    if(dqpsk->data)
    {
        free(dqpsk->data);
    }
    int len = length<<1;
    dqpsk->data = malloc(sizeof(char)*len);
    unsigned char* d = dqpsk->data;
    for(unsigned char* i = data;i<data+length;i++){
        *d = *i;
        d++;//manchester encoding
        *d = ~(*i);
        d++;
    }
    dqpsk->d_index = dqpsk->data;
    dqpsk->d_end = dqpsk->data+len;


}


