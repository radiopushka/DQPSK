#include "./alsa/alsa.h"
#include "DQPSK/modulator.h"
#include<stdio.h>
#include<stdlib.h>
#include <stdint.h>
#include<string.h>
#include<math.h>

int main(){
    int bsize = 7398*2;
    if(setup_alsa_IQ( "default",bsize, 48000)<0)
        printf("Failed to setup alsa\n");

    short *frame = malloc(bsize*sizeof(short));

    struct dqpsk_mod* mod = create_dqpsk_mod(5000.0,4000,48000,1000);


    char* string = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 - = + []\n";
    //char* string = "Hello World!\n";
    int len_st = strlen(string);
    printf("string len:%d\n",len_st);

    set_dqpsk_data(mod,(unsigned char*)string,len_st);
    int framec = 0;
    int make_array = 1;
    while(1){

        if(run_dqpsk_mod_IQ(mod,frame,bsize>>1,M_PI/5.56,1)){
            set_dqpsk_data(mod,(unsigned char*)string,len_st);
            //printf("frames for data:%d %d\n",(mod->dcount_max*(mod->d_end-mod->data))*8,framec);
            framec = 0;
        }else{
            framec++;
        }
        if(!make_array){
            printf("uint32_t* packet_buffer={");
            for(int i = 0;i<bsize>>1;i = i+2){
                short samp1 = frame[i] + 2047;
                short samp2 = frame[i+1] + 2047;
                uint32_t sample = (samp2<<16)|samp1;
                if(i)
                    printf(",");
                printf("%u",sample);

            }
            printf("};\n");
            printf("\n");
            printf("\n");
            printf("\n");

            make_array = 1;
        }
        //printf("sample\n");
        awrite(frame,bsize>>1);

    }

    free_dqpsk_mod(mod);

    free_alsa();
}
