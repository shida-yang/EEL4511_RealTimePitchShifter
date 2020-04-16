function output = sineGen(A, fw, phase, fs, length)
    t=1:length;
    spp=fs/fw;
    output=A*cos(2*pi*t/spp+phase);
end

