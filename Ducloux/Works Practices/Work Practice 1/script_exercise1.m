# Generate a sinusoidal digital signal specifying the corresponding parameters 
# and its amplitude at different resolutions. Graph the signal.
clear; # Eliminate variables
clc; # Cleans console
close all; # Closes previous graphics

% Exercise 1
# Parameters of the signal fs, t, f, x
fs = 1000;  % Sampling frequency

# Begin:Step:End ==> 0:1/1000=0.001:1 ==> 1 second of signal with steps of 1ms
# 0, 0.001, 0.002, 0.003, ...., 1
t = 0:1/fs:1;  % Time Vector

# Signal completes 5 cycles per second
f = 5;  % Signal frecuency
x = sin(2 * pi * f * t);  % Sinusoidal signal

# Cuantization
# Amplitud will be represented with 3 bits
bits = 3;  % Cuantization bits
# 8 levels of cuantization
# Amplitud will take only 8 possible values
levels = 2^bits;  % Cuantization levels

# (x+1) = before: [-1,1] => after: [0,2] avoid negative values to cuantify
# * (levels/2 - 1) = 3 => [0,2] to [0,6]
# round(....) => (0,1,2,3,4,5,6) discrete levels
# / (levels/2 - 1) => [0,6] to [0,2]
# -1 => [0,2] to [-1,1]
# This formula transforms continual signal in an aproximation with few levels
# With 3 bits, signal can't take any amplitud, just some.
# Cuantized signal it's like a type of stairs
x_quantized = round((x + 1) * (levels/2 - 1)) / (levels/2 - 1) - 1;

# Make graph
plot(t, x, 'b', 'LineWidth', 1); # Graph original´ signal x in blue ('b')
hold on; # Wait until other graph is draw inside this actual graph. Don't borrow actual signal
plot(t, x_quantized, 'r', 'LineWidth', 1.5); # Draws cuantizied signal in red ('r') and a little bit more width
xlabel('Time [s]'); # X Axis
ylabel('Amplitud'); # Y Axis
title(['Cuantizied Signal to ', num2str(bits), ' bits']);
legend('Original', 'Cuantizied'); # Adds leyend
grid on; # Activates grid