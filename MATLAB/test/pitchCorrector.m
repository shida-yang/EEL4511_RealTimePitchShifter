%%Primitive pitch correction function. Requires input STFT, its frequency
%%space, and a table of pitches to compare to. Y can bt the HPS of S or any
%%other peak-accentuated sequence.
function S_corrected = pitchCorrector(S, F, Y, pitchtable)

bins = length(S(1,:));
S_len = length(S(:,1));

%%Pre-allocates memory for arrays
maxpeaks = zeros(1,bins);
indices = zeros(1,bins);
pitches = zeros(1,bins);
correctedpitches = zeros(1,bins);
ratio = zeros(1,bins);
S_corrected = zeros(S_len,bins);

%%Finds the maximum amplitudes and their corresponding frequencies in the
%%Fourier spectrum. correctedpitches stores the closest exact pitch to
%%each of these maximum pitches. ratio stores the factor by which each
%%ST-spectrum will have to be shifted in order to obtain the correct
%%pitch.
for k = 1:bins
    maxpeaks(k) = max(Y(:,k));
    temp=find(Y(:,k) == maxpeaks(k),k,'first');
    indices(k) = temp;
    pitches(k) = F(indices(k));
    correctedpitches(k) = compareToPitches(pitches(k), pitchtable);
    ratio(k) = correctedpitches(k)/pitches(k);

end

%%This for-loop is separate from the previous for debugging purposes.
%%Utilizes the fact that the frequencies in F are spaced linearly, starting at 0, so that
%%the ratio between frequencies is equivalent to the ratios between sample
%%indices. If ratio > 1, S_corrected will end with a number of zeros. If
%%ratio < 1, S_corrected will contain many repeated values. Either way,
%%high-frequency information will be lost.

for k = 1:bins
        for j = 1:S_len
          y = round(j/ratio(k));
          
          %%Prevents negative indices
          if y < 1 
              y = 1;
          end
          
          %%Non-constant frequency scaling
          if y <= S_len
            S_corrected(j,k) = S((y),k);
          end
        end
end

end


