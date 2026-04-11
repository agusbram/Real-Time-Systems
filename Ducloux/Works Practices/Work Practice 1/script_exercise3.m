# Add two signals like those in point 1 with equal amplitude, but
# of different frequencies. Graph the resulting signal.

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

