clc; % limpia la pantalla
clear; % borra el espacio de trabajo

% ejercicio 1
fs = 1000;  % frecuencia de muestreo
t = 0:1/fs:1;  % vector de tiempo
f = 5;  % frecuencia de la se#al
x = sin(2 * pi * f * t);  % se#al sinusoidal
bits = 3;  % bits de cuantización
levels = 2^bits;  % niveles de cuantización
x_quantized = round((x + 1) * (levels/2 - 1)) / (levels/2 - 1) - 1;
plot(t, x, 'b', 'LineWidth', 1);
hold on;
plot(t, x_quantized, 'r', 'LineWidth', 1.5);
xlabel('Tiempo [s]');
ylabel('Amplitud');
title(['Señal Cuantizada a ', num2str(bits), ' bits']);
legend('Original', 'Cuantizada');
grid on;


% ejercicio 2
signal_freq=100;
signal_amp=100;
sampling_freq=8000;
sample_number = 2000;
for i=1:sample_number
 signal(i) = (signal_amp*sin(2*pi*(i-1)*signal_freq/sampling_freq));
end
plot(signal);
plot(signal, 'o-b');
signal1 = signal;


% ejercicio 3
signal_freq=300;
signal_amp=100;
sampling_freq=8000;
sample_number = 2000;
for i=1:sample_number
 signal(i) = (signal_amp*sin(2*pi*(i-1)*signal_freq/sampling_freq));
end
plot(signal);
plot(signal, 'o-b');
signal2 = signal;

signal3 = signal1 + signal2;
plot(signal3, 'o-b');


% ejercicio 4
X=signal3;
L=length(X); % longitud de la se#al
T = 1/sampling_freq; % periodo de muestreo     
t = (0:L-1)*T; % vector de tiempo
Y = fft(X); % calcula la FFT.
P2 = abs(Y/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);
f = sampling_freq*(0:(L/2))/L; % vector de frecuencias.
figure;
plot(f,P1) % grafica la se#al en el dominio de la frecuencia.
title('Amplitud en el dominio de la frecuencia')
xlabel('f (Hz)')
ylabel('|P1(f)|')

plot(f,20*log10(P1)) % grafica con salida en dBs.



% ejercicio 5
% carga de datos
array = signal3;
min_index = 1;
max_index = 100;
file_name = 'test_signal.h';
const_name = 'ELEMENTS';
array_name = ['short test_signal[' const_name ']={'];
% crea el archivo de cabecera
fid=fopen(file_name,'w');
fprintf(fid,'#define ');
fprintf(fid, const_name);
fprintf(fid, ' %d\n\n', max_index);
fprintf(fid, array_name);
fprintf(fid,'%d, ' , array(min_index:(max_index-1)));
fprintf(fid,'%d' , array(max_index));
fprintf(fid,'};\n');
fclose(fid);

