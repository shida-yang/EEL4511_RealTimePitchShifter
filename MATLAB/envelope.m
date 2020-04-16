%create an envelope function based on duration and sampling rate
function xx = envelope(fs,duration)
    %initialize the function vector
    xx=zeros(1,int32(duration*fs));
    %start index
    n1=1;
    %length of A: 5%
    n2=int32(length(xx)*0.05);
    %length of D: 5%
    n3=int32(length(xx)*0.05);
    %length of S: 50%
    n4=int32(length(xx)*0.5);
    %length of R: 40%
    n5=length(xx)-n2-n3-n4;
    %put different parts into vector xx
    %each part is a linear function
    xx(n1:n1+n2-1)=linspace(0,1,n2);
    xx(n1+n2:n1+n2+n3-1)=linspace(1,0.8,n3);
    xx(n1+n2+n3:n1+n2+n3+n4-1)=linspace(0.8,0.7,n4);
    xx(n1+n2+n3+n4:n1+n2+n3+n4+n5-1)=linspace(0.7,0,n5);
end