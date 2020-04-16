%%Computes the Harmonic Product Spectrum of a magnitude spectrum Y.
%%Works by downsampling the input sequence by 2, zero-padding it to its
%%original length, and then dotting it with the original sequence. This should
%%theoretically accentuate the peak at the fundamental frequency.
%%Including a downsampling by 3 can be done by uncommenting the commented
%%code. Absolute amplitude values of the HPS will be meaningless, but
%%the HPS is only used to obtain frequency information anyway.

function S = hps(Y)

down2 = Y(1:2:length(Y));
padding = zeros((length(Y)-length(down2)), 1);
down2 = [down2; padding];

%%down3 = Y(1:3:length(Y));
%%padding = zeros((length(Y)-length(down3)), 1);
%%down3 = [down3; padding];

S = Y.*down2;

%%S = Y.*down2.*down3;


for i=1:length(S)
   S(i) = S(i)^(1/2);
end

end