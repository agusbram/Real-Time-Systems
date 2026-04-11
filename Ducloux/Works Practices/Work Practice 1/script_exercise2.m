% Exercise 2
# Generate a digital sinusoidal signal specifying its amplitude and 
# frequency, the sampling frequency, and the number of samples 
# in the recording as parameters. Graph the signal.

signal_freq=100; # Frecuency of the signal. 100 cycles per second
signal_amp=100; # Amplitud signal. [-100,100]
sampling_freq=8000; # 8000 samples per second
sample_number = 2000; # Quantity of samples to be generated

# Generates the sinusoidal signal sample by sample
# x[n] = A * sin(2*pi * f/fs * n)
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

# Copied to use it later in other calculus
signal1 = signal;
