#include "covidTrace.h"

void addToTimeval(struct timeval* t_spec, struct timeval addend)
{
    t_spec->tv_sec += addend.tv_sec;
    t_spec->tv_usec += addend.tv_usec;

    // Check if the nsecs of t_spec exceed 10^9 and need to be wrapped
    if (t_spec->tv_usec >= 1000000)
    {
        t_spec->tv_usec -= 1000000;
        t_spec->tv_sec += 1;
    }
}

int main()
{
    struct timeval timeOfCall, newInterval,firstCall;

    FILE *write_ptr;

    struct timespec ts;
    int randInt = 0;
    float percent = 0;


    write_ptr = fopen("C:/Users/konko/Desktop/RPi/BTcallsTest.bin","ab");  // a for append, b for binary
    timeOfCall.tv_usec = 448719;
    timeOfCall.tv_sec = 1633886591;

    firstCall.tv_sec = 1633877001;
    firstCall.tv_usec = 48751;

    double meanTimeOffset = 0.0000000003336670056084977 * 1000000;
    long duration_msecs = 0;

    while(duration_msecs < 25920000)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        srand((time_t)ts.tv_nsec);
        randInt = rand()%101;
        percent = (float)(randInt)/100 + 0.50;

        newInterval.tv_sec=0;
        newInterval.tv_usec = 100000 + meanTimeOffset*percent;
        addToTimeval(&timeOfCall,newInterval);

        //printf("New time of call: %ld secs and %ld usecs. \n",timeOfCall.tv_sec,timeOfCall.tv_usec);
        fwrite(&timeOfCall,4,2,write_ptr);
        duration_msecs = (timeOfCall.tv_sec - firstCall.tv_sec)*1000 + (timeOfCall.tv_usec - firstCall.tv_usec)/1000;
        printf("Duration: %ld out of 25920000. \n",duration_msecs);
    }

    fclose(write_ptr);
    return 1;
}
