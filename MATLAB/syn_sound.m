function [output fs] = syn_sound(input_file, freq, timeLen)
    peakDiffThres=0.01;
    outputFreq=freq;
    timeLength=timeLen;
    
    [input fs]=audioread(input_file);
    left=input(:,1);
    clear output;
    leftFFT=fft(left.*hann(length(left)));
    %leftFFT=fft(left);
    leftFFT=leftFFT(1:length(leftFFT)/2);
    leftMag=abs(leftFFT);
    [maxL maxIndex]=max(leftMag);
    leftPhase=angle(leftFFT);
    [pksL,locsL,widthsL,promsL]= findpeaks(leftMag,1:length(leftMag),'MinPeakProminence',maxL*peakDiffThres);
    
    freqRatioL=[];
    for i=1:length(locsL)
        freqRatioL(i)=locsL(i)/maxIndex;
    end
    
    magListL=leftMag(locsL);
    phaseListL=leftPhase(locsL);

    output=zeros(1,round(timeLength*fs));

    for i=1:length(locsL)
        output=output+sineGen(magListL(i),outputFreq*freqRatioL(i),phaseListL(i),fs,length(output));
    end
end

