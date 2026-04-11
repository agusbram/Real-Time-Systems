# Exercise 4
# Review the fft() function documentation for the simulation environment to obtain the frequency spectra of the signals from points 1 and 2.
# Plot each frequency spectrum.

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

# Graph signal
# Graphs signal using index of sample as x axis
# x axis: 1,2,3,...,200
# y axis: signal amplitud
plot(signal);

# Graphs same signal but with circular shape in every sample in color blue aligned with '-'
plot(signal, 'o-b');

# Copied actual signal to signal2 to addition it with signal 1 
signal2 = signal;

# Sum signal 1 with signal 2
signal3 = signal1 + signal2;

# Graph additioned signal 3
plot(signal3, 'o-b');


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

# Shows spectrum frecuency with real amplitud
figure;
plot(f,P1) % grafica la se#al en el dominio de la frecuencia.
title('Amplitud en el dominio de la frecuencia')
xlabel('f (Hz)')
ylabel('|P1(f)|')

# Converts magnitud to dB
# This is useful to see signals with differences of high amplitud. Logaritmic scale represents better little components
plot(f,20*log10(P1)) % grafica con salida en dBs.

