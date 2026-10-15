#include "./alsa/alsa.h"
#include "DQPSK/modulator.h"
#include<stdio.h>
#include<stdlib.h>

int main(){
    int bsize = 1024;
    if(setup_alsa("default",0,bsize, 48000)<0)
        printf("Failed to setup alsa\n");

    short *frame = malloc(bsize*sizeof(short));

    struct dqpsk_demod* mod = create_dqpsk_demod(19000.0,250,48000);

    char* output = malloc(1024*sizeof(char));
    char* trm = malloc(1024*sizeof(char));

    while(1){

      aread(frame,bsize);
      int chars = run_dqpsk_demod(mod,frame,output,bsize);
      output[chars] = 0;
      printf("%s",output);
    }

    free_dqpsk_demod(mod);

    free_alsa();
}
