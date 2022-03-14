fileID = fopen('C:/Users/konko/Desktop/RPi/BTcalls.bin');
A = fread(fileID,'int32');
fclose(fileID);

% Separating the call times of BTnearMe() to rows, each with two columns(secs and msecs)
callTimes = zeros(ceil(size(A,2)/2),2);
callTimesWriteIndex = 1;
for i=1:2:length(A)
    callTimes(callTimesWriteIndex,1) = A(i);
    callTimes(callTimesWriteIndex,2) = A(i+1);
    callTimesWriteIndex = callTimesWriteIndex + 1;
end

% Time difference between each call of BTnearMe()
timeDiff = [];
for i=2:(size(callTimes,1))
    timeDiff(end+1) = callTimes(i,1) - callTimes(i-1,1);
    timeDiff(end) = timeDiff(end)*1000000;
    timeDiff(end) = timeDiff(end) + callTimes(i,2) - callTimes(i-1,2);
    timeDiff(end) = timeDiff(end)/1000000;
end

% Different useful metrics
meanTimeDiff = mean(timeDiff);
timeOffset = timeDiff - 0.1;
meanTimeOffset = mean(timeOffset);
meanTimeOffsetPercent = meanTimeOffset*100;

% Constructing a time with the timestamp of when BTnearMe() should have
% been called, in theory
theoryTimes = zeros(size(callTimes,1),1);
theoryTimes(1) = 0;
theoryTimes(2) = 0.1;
for i = 3 : size(theoryTimes,1)
    theoryTimes(i) = theoryTimes(i-1) + 0.1;
end

realTimes = zeros(size(callTimes,1),1);
realTimes(1) = 0;
for i=2:size(realTimes,1)
    realTimes(i) = (callTimes(i,1) - callTimes(1,1))*1000000 + (callTimes(i,2) - callTimes(1,2)); % time from first call (0.0)
    realTimes(i) = realTimes(i)/1000000; % into seconds
end

figure;
plot(realTimes,theoryTimes);
xlabel("real call values");
ylabel("theoretical call values");
title("Real vs theoretical BTnearMe() call values");