FLAGS = -march=native -mtune=native -O3 -funroll-loops -fomit-frame-pointer -flto -fno-signed-zeros -fno-trapping-math -fno-fast-math
DSP = ./DQPSK/modulator.c ./DQPSK/IIR/lpf.c

all:
	$(CC) ./dqpsk_rx.c $(FLAGS) $(DSP) ./alsa/alsa.c -lm -lasound -o dqpsk_rx
	$(CC) ./rtlsdr_data_rx.c $(FLAGS) $(DSP) ./alsa/alsa.c -lm -o sdr_rx
	$(CC) ./dqpsk_gen.c $(FLAGS) $(DSP) ./alsa/alsa.c -lm -lasound -o dqpsk_gen
	$(CC) ./dqpsk_gen_IQ.c $(FLAGS) $(DSP) ./alsa/alsa.c -lm -lasound -o dqpsk_IQ
