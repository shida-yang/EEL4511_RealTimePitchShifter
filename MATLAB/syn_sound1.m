function [output fs] = syn_sound1(input_file, freq, timeLen)
    outputFreq=freq;
    timeLength=timeLen;
    
    [input fs]=audioread(input_file);
    left=input(:,1);
    clear output;
    leftFFT=fft(left.*hann(length(left)),fs);
    leftMag=abs(leftFFT);
    [maxL maxIndex]=max(leftMag);
    leftPhase=angle(leftFFT);
    
    currFreq=maxIndex*fs/length(leftMag);
    freqRatio=freq/currFreq;
    
    shiftedFFT=zeros(1,length(leftMag));
    for i=2:(length(leftMag)/2)
        lowIndex=round((i-1)*freqRatio)+1;
        phasor=leftMag(i)*exp(j*leftPhase(i));
        if leftMag(i)<1
            phasor=0;
        else
            phasor=leftMag(i)*exp(j*leftPhase(i));
        end
        shiftedFFT(lowIndex)=phasor;
    end
    
    for i=2:(length(leftMag)/2)
        highIndex=length(leftMag)+2-i;
        shiftedFFT(highIndex)=conj(shiftedFFT(i));
    end

    output=zeros(1,round(timeLength*fs));
    ifftOut=ifft(shiftedFFT, fs);
    numOfWholes=floor(length(output)/length(ifftOut));
    for i=1:numOfWholes
        output(((i-1)*length(ifftOut)+1):(i*length(ifftOut)))=ifftOut;
    end
    for i=(numOfWholes*length(ifftOut)+1):length(output)
        output(i)=ifftOut(i-numOfWholes*length(ifftOut));
    end
end

