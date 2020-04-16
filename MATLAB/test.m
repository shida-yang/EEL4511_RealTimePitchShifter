currFreq=141;
octave=floor(log(currFreq/16.35)/log(2));

notes=[1 1 5 5 6 6 5];
%notes=[1 2 3 4 5 6 7];

lookup=[1 3 5 6 8 10 12 13];

ratioLookup=[1 21/20 11/10 6/5 5/4 4/3 7/5 3/2 8/5 5/3 9/5 19/10 2];

lookup2=[16.35 17.32 18.35 19.45 20.6 21.83 23.12 24.5 25.96 27.5 29.14 30.87];

output=[];

noteLen=0.5;
input_file="C.mp3";
[input fs]=audioread(input_file);
input=input(:,1);
overlap=int32(noteLen*fs);
output=zeros(1,overlap*7+length(input));
for i=1:length(notes)
    %freq=lookup2(lookup(notes(i)))*2^octave;
    %[sound fs]=syn_sound("gong.mp3",freq,noteLen);
    
%   sound=pitchShift(input, 1024, 256, lookup(notes(i))-1);
    
    targetSemiNote=lookup(notes(i));
    ratio=ratioLookup(targetSemiNote);
    pvocOut=pvoc(input,1/ratio,512);
    sound=interp1(1:length(pvocOut),pvocOut,1:ratio:length(pvocOut));
    
%     env=envelope(fs,length(sound)/fs);
%     sound=sound.*env;
    starting=(i-1)*overlap+1;
    output(starting:starting+length(sound)-1)=output(starting:starting+length(sound)-1)+sound;
end

soundsc(output, fs);