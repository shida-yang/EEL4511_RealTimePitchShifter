function S_out = pitchShiftS(S, ratio)
    bins = length(S(1,:));
    S_len = length(S(:,1));
    S_out = zeros(S_len,bins);
    for k = 1:bins
        for j = 1:S_len
            y = round(j/ratio);
          
            %%Prevents negative indices
            if y < 1 
              y = 1;
            end

            %%Non-constant frequency scaling
            if y <= S_len
            S_out(j,k) = S((y),k);
            end
        end
    end
end