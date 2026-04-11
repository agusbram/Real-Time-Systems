% Exercise 5
# 5) Use the simulation environment to generate a header file (file with the extension “.h”) with the stored samples of a signal.

signal_freq=100; # Frecuency of the signal. 100 cycles per second
signal_amp=100; # Amplitud signal. [-100,100]
sampling_freq=8000; # 8000 samples per second
sample_number = 2000; # Quantity of samples to be generated

# Generates the sinusoidal signal sample by sample
# x[n] = A * sin(2*pi * f/fs * n)
for i=1:sample_number 
 signal(i) = (signal_amp*sin(2*pi*(i-1)*signal_freq/sampling_freq));
end

# Copied to use it later in other calculus
signal1 = signal;

# Make signal 2 to be additioned with signal 1
signal_freq=300; # Different frecuency that signal 1
signal_amp=100; # Same Amplitud that signal 1
sampling_freq=8000; # Same that signal 1
sample_number = 2000; # Same that signal 1
for i=1:sample_number
 signal(i) = (signal_amp*sin(2*pi*(i-1)*signal_freq/sampling_freq));
end

# Copied actual signal to signal2 to addition it with signal 1 
signal2 = signal;

# Sum signal 1 with signal 2
signal3 = signal1 + signal2;

# Uses signal3 of previous exercise
X=signal3;
L=length(X); % Signal Lenght. Quantity of signal samples
T = 1/sampling_freq; % Samples Period. Time interval between samples
t = (0:L-1)*T; % Creates a time vector for every sample. You need it to graph signal in the domain of time
Y = fft(X); % Transform signal time to frecuency. Y = complex number vector. Every element represents amplitud and phase of a frecuency present in signal
P2 = abs(Y/L); # Normalized magnitud of every frecuency component
P1 = P2(1:L/2+1); # FFT signal is simetric. You only need positive half of spectrum
# Multiplies by 2 all of the amplitud minus the first and the last one, because energy divides into half positive and half negative
P1(2:end-1) = 2*P1(2:end-1); # This ensures that magnitud represents correctly the amplitud of the original signal
# (0:(L/2)) generates indexs of half positive of spectrum
f = sampling_freq*(0:(L/2))/L; % Contains correspondient frecuencies of every P1 value

# ---------------------------------- HERE BEGINS EXERCISE 5 -------------------------------------

% Data charge
array = signal3; # Saves samples of signal3 of exercise 4

# Range of samples of array to be saved into header h
min_index = 1;
max_index = 100;
# Other useful variables to insert correctly into header h
file_name = 'test_signal.h';
const_name = 'ELEMENTS';
# This shows into header file next content: short test_signal[ELEMENTS]={
array_name = ['short test_signal[' const_name ']={'];

% Creates header file with write permissions
fid=fopen(file_name,'w');

# Insert into header file created 
# Three lines below writes this: #define ELEMENTS 100
fprintf(fid,'#define ');
fprintf(fid, const_name);
fprintf(fid, ' %d\n\n', max_index);
# short test_signal[ELEMENTS]={
fprintf(fid, array_name);

# short test_signal[ELEMENTS]={0, 10, 20, ..., 5};
fprintf(fid,'%d, ' , array(min_index:(max_index-1)));
fprintf(fid,'%d' , array(max_index));
# Closes declaration of array in C
fprintf(fid,'};\n');
fclose(fid);

# Result of header file:
#define ELEMENTS 100

#short test_signal[ELEMENTS] = {0, 10, 20, 30, ..., 5};

