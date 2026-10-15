#include "DQPSK/modulator.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


void parse_packet(char* in,int length,int chksum){

    int sum = 0;
    for(char* i = in;i<in+length;i++){
        sum+=*i;
    }
    if(sum!=chksum){
        printf("checksum failed\n");
    }else{
      for(char* i = in;i<in+length;i++){
        printf("%c",*i);
      }

    }

}
int packet_counter = 0;
int packet_chksum = 0;
int ptracker = 0;
char packet[100];
char back_track[64];
void parse_data(char* in,int* count){

    int size = *count;
    *count = 0;
    int buffer_counter = 0;

    for(char* i = in;i<in+size;i++){

        memmove(back_track,back_track+1,63);
        if(packet_counter<=0){
            *back_track = *i;
        }else{
            *back_track = 0;
        }
        if(memcmp(back_track,"SYNCSIZE",8)==0&&packet_counter<=0){
            int tracker = sizeof(long);
            int buffer_location = buffer_counter-63;
            long l;
            int c = 8;
            for(int i = tracker-8;i>=0;i = i-8){
                l |= back_track[c]<<i;
                c++;
            }
            printf("received file size %ld\n",l);
        }
        if(back_track[0]==2&&packet_counter<=0){//transmission start
                int buffer_location = buffer_counter-63;
                packet_counter = back_track[1];
                packet_chksum = (back_track[2]<<8)|back_track[3];
                int size = 60;
                if(size>packet_counter){
                    size = packet_counter;
                    packet_counter = 0;
                    memcpy(packet,back_track+4,size);
                    parse_packet(packet,size,packet_chksum);

                }else{
                    packet_counter -= size;
                    memcpy(packet,back_track+4,size);
                    ptracker = size;
                }

        }else if(packet_counter>0){
            packet[ptracker] = *i;
            ptracker++;
            packet_counter--;
            if(packet_counter==0){
                parse_packet(packet,ptracker,packet_chksum);
            }
        }
        buffer_counter++;
    }
}

int main(){
    int rate_upscale = 2;
    int bsize = 1024;

    short *frame = malloc(bsize*sizeof(short));
    short *frame_moc = malloc((bsize*rate_upscale)*sizeof(short));

    struct dqpsk_demod* mod = create_dqpsk_demod(8000.0,8000,64000);

    char* output = malloc(1024*sizeof(char));
    char* framed = malloc(200*sizeof(char));

    int count = 0;
    short prev = 0;
    while(1){

      int samples =fread(frame, sizeof(short), bsize, stdin);
      short* framet = frame_moc;
      for(int i = 0;i<bsize;i++){
          short point = frame[i];
          for(int i = 0;i<rate_upscale;i++){
            *framet = (point+prev)/2;
            prev = point;
            framet++;
          }
      }


      int chars = run_dqpsk_demod(mod,frame_moc,output,bsize*rate_upscale);

      /*(if(count+chars > 200){
          printf("full\n");
        parse_data(framed,&count);

      }
      memcpy(framed+count,output,chars);
      count +=chars;*/
      output[chars] = 0;
      printf("%s",output);
    }

    free_dqpsk_demod(mod);
    free(output);
    free(frame);
    free(frame_moc);
    free(framed);

}
