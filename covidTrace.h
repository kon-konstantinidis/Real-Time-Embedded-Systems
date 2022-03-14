#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
//  ***MACROS***
#define COVID_CHANCE 1
#define MAC_ARRAY_SIZE  100
#define RUNTIME_NSECS 25920000000000   // 7.2 hours: 25920000000000 ns

//  ***Structs***
// A struct using uint8_t type to simulate a MAC address-holding variable
struct MACaddress
{
    uint8_t hexs[6];
};

// A struct to hold the information of a contact (MAC address and timestamp)
struct contact
{
    struct MACaddress MAC;
    struct timespec timestamp;
};

//  ***Globals***
// Array of type "struct contact", to hold all contacts, initialized to its max size (max 120 returned MACs in 12 secs, then we delete 1-1)
struct contact contacts[120];

int contacts_writeIndex = 0;
// Timespec structs necessary for clock_nanosleep() to run on the two threads bellow
struct timespec threadBTnearMe_sleepTime, threadBTnearMe_remainingTime,threadTestCOVID_sleepTime,threadTestCOVID_remainingTime;

struct MACaddress* MACsArray; // Pointer to an array with all the generated MAC addresses
pthread_mutex_t contacts_mutex;

//  ***Functions***
void initializeMACs()
{
    // Static array of all 1 byte hexs, from 0x00 to 0xFF (0-255)
    static uint8_t hexArray[256];
    for (int i=0x0; i<=0xFF; i++)
    {
        hexArray[i] = i;
    }

    // Assign to every hex of every MACaddress in MACarray a random hexArray element
    static struct MACaddress MACarray[MAC_ARRAY_SIZE];
    struct timespec ts;
    int randInt = 0;
    for (int i=0; i<MAC_ARRAY_SIZE; i++)
    {
        for (int j=0; j<6; j++)
        {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            srand((time_t)ts.tv_nsec);
            randInt = rand()%256; // randInt is a random, nanoseconds time-seeded integer from 0 to 255
            MACarray[i].hexs[j] = hexArray[randInt];
        }
    }
    // Print the MAC addresses generated
    /*
    for (int i=0; i<MAC_ARRAY_SIZE; i++)
    {
       printf("Mac Address i:%d is %02X:%02X:%02X:%02X:%02X:%02X \n", i, MACarray[i].hexs[0], MACarray[i].hexs[1],MACarray[i].hexs[2],MACarray[i].hexs[3],MACarray[i].hexs[4],MACarray[i].hexs[5]);
    }
    */
    MACsArray = MACarray;
}

// Logs the time of the function call and returns a random MAC address from an array of #MAC_ARRAY_SIZE of them
struct MACaddress BTnearMe()
{
    // Get the time of the function call
    struct timeval timeOfCall;
    gettimeofday(&timeOfCall,NULL);
    // Append the time to the bin file
    FILE *write_ptr;
    write_ptr = fopen("BTcalls.bin","ab");  // a for append, b for binary
    fwrite(&timeOfCall,4,2,write_ptr);
    fclose(write_ptr);

    // Return a random MAC address from the MACsArray
    struct timespec ts;
    int randInt = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((time_t)ts.tv_nsec);
    randInt = rand()%MAC_ARRAY_SIZE; // Chance is a random, nanoseconds time-seeded integer from 0 to 19
    struct MACaddress rand_mac = MACsArray[randInt];
    return rand_mac;
};

// Enters the newContact in the contacts array
// If the array is not yet full, then we add the newContact in the contacts[contact_write_index]
// If the array if filled (one MAC deletion from the start is in order since time has passed), then
void addToContacts(struct contact newContact)
{
    if (contacts_writeIndex<120)  // contacts array not yet full
    {
        contacts[contacts_writeIndex] = newContact;
        contacts_writeIndex++;
    }
    // contacts array full, so we delete the first element contacts[0] (more than 12 secs have passed since its addition),
    // by shifting all other elements to the start by one element, then add the newContact as the last element
    else if (contacts_writeIndex == 120)
    {
        for (int array_index=1; array_index<120; array_index++)
        {
            contacts[array_index-1] = contacts[array_index]; // shift all elements to the start by 1, tossing the first element
        }
        contacts[119] = newContact;
    }
    else
    {
        printf("Something went wrong, contacts_write_index out of bounds, value: %d \n", contacts_writeIndex);
    }
}

void addToTimespec(struct timespec* t_spec, struct timespec addend)
{
    t_spec->tv_sec += addend.tv_sec;
    t_spec->tv_nsec += addend.tv_nsec;

    // Check if the nsecs of t_spec exceed 10^9 and need to be wrapped
	if (t_spec->tv_nsec >= 1000000000)
	{
		t_spec->tv_nsec -= 1000000000;
		t_spec->tv_sec += 1;
	}
}

// The returned address is stored in a contact struct, along with the timestamp
void *threadBTnearMe()
{
    // Thread runtime
    struct timespec threadBTnearMeStart;
    clock_gettime(CLOCK_REALTIME, &threadBTnearMeStart);
    long threadBTnearMe_seconds = 0;
    long long threadBTnearMe_nanos = 0;
    // Sleep time between BT searches: 0.1 secs
    threadBTnearMe_sleepTime.tv_sec = 0;
    threadBTnearMe_sleepTime.tv_nsec = 100000000;

    struct timespec nextBTsearchTime = threadBTnearMeStart;
    while(1)
    {
        addToTimespec(&nextBTsearchTime,threadBTnearMe_sleepTime);
        struct MACaddress newMAC = BTnearMe(); // Call BTnearMe()

        // Make the new contact
        struct timespec timeOfContact;
        clock_gettime(CLOCK_REALTIME, &timeOfContact);
        struct contact newContact;
        newContact.MAC = newMAC;
        newContact.timestamp = timeOfContact;

        // Add the new contact to the contacts array
        pthread_mutex_lock(&contacts_mutex);
        addToContacts(newContact);
        pthread_mutex_unlock(&contacts_mutex);
        // While loop (thread) termination

        // Calculate total thread runtime
        threadBTnearMe_seconds = (timeOfContact.tv_sec - threadBTnearMeStart.tv_sec);
        threadBTnearMe_nanos = ((threadBTnearMe_seconds * 1000000000) + timeOfContact.tv_nsec) - (threadBTnearMeStart.tv_nsec);
        printf("\rProgram runtime: %03d minutes and %02d seconds", threadBTnearMe_seconds/60,threadBTnearMe_seconds%60);
        if (threadBTnearMe_nanos >= RUNTIME_NSECS)
        {
            break;
        }

        // Sleep for 0.1 sec, the repeat all over
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &nextBTsearchTime, NULL);
    }
    return NULL;

}

// Function that simulates the user taking a COVID test
// Returns the boolean 1 for a positive result and the boolean 0 for a negative result
// The chance of a positive result is COVID_CHANCE
bool testCOVID()
{

    // Seeds the rand() function with the time in nanoseconds, gets different values for same second rand() calls
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((time_t)ts.tv_nsec);

    int chance = rand()%101; // Chance is a random, nanoseconds time-seeded integer from 0 to 100
    //printf("COVID TEST\nResult: %s \n",(chance<COVID_CHANCE)?"Positive":"Negative");
    if (chance<COVID_CHANCE)
    {
        return true;   // Chance is [0,100], so (chance-1)% for true and (100-chance+1)% for false
    }
    else return false;

};

// Appends the contents of the closeContacts[arraySize] array to the "closeContacts.txt" file
void uploadContacts(struct MACaddress* closeContacts, int arraySize)
{

    // Append close contacts to "closeContacts.txt"
    FILE *f;
    f = fopen("closeContacts.txt", "a");
    if (f == NULL)
    {
        printf("Error opening .txt file for close contacts upload! errno is %d and error is: %s .\n",errno,strerror(errno));
        exit(1);
    }
    fprintf(f, "Close contacts: \n");
    for (int i=0; i<arraySize; i++)
    {
        fprintf(f, "Contact %d: %02X:%02X:%02X:%02X:%02X:%02X \n",i+1,closeContacts[i].hexs[0],closeContacts[i].hexs[1],
                closeContacts[i].hexs[2],closeContacts[i].hexs[3],closeContacts[i].hexs[4],closeContacts[i].hexs[5]);
    }
    fprintf(f, "\n");
    fclose(f);
}

// Given two structs MACaddress mac1 and mac2, checks if mac1 == mac2
// Returns true for equal, false for non equal
bool equalMACs(struct MACaddress mac1, struct MACaddress mac2)
{
    bool isEqual = true;
    for (int i=0; i<6; i++)
    {
        if (mac1.hexs[i] != mac2.hexs[i])
        {
            isEqual = false;
        }
    }
    return isEqual;
}

// Prints the contacts array to the "allContacts.txt" file, for post-verification
void printContactsArray()
{
    // Append all contacts to "allContacts.txt"
    FILE *write_pointer_AllContacts;
    write_pointer_AllContacts = fopen("allContacts.txt", "a");
    if (write_pointer_AllContacts == NULL)
    {
        printf("Error opening .txt file for contacts array print! errno is %d and error is: %s .\n",errno,strerror(errno));
        exit(1);
    }
    fprintf(write_pointer_AllContacts, "All contacts: \n");
    for (int i=0; i<contacts_writeIndex; i++)
    {
        fprintf(write_pointer_AllContacts, "Contact %02d: %02X:%02X:%02X:%02X:%02X:%02X and timestamp: %ld seconds and %ld nanoseconds.\n",
        i,contacts[i].MAC.hexs[0],contacts[i].MAC.hexs[1],contacts[i].MAC.hexs[2],contacts[i].MAC.hexs[3],contacts[i].MAC.hexs[4],
        contacts[i].MAC.hexs[5],contacts[i].timestamp.tv_sec, contacts[i].timestamp.tv_nsec);
    }
    fprintf(write_pointer_AllContacts, "\n");
    fclose(write_pointer_AllContacts);
}

// If the test is positive, uploadContacts() is called
void *threadTestCOVID()
{
    // Thread runtime
    struct timespec threadTestCOVID_start;
    clock_gettime(CLOCK_REALTIME, &threadTestCOVID_start);
    struct timespec threadTestCOVID_current;

    long threadTestCOVID_seconds = 0;
    long long threadTestCOVID_nanos = 0;
    // Sleep time between COVID tests: 144 secs = 2.4 minutes
    threadTestCOVID_sleepTime.tv_sec = 144;
    threadTestCOVID_sleepTime.tv_nsec = 0;

    struct timespec nextTestCOVIDTime = threadTestCOVID_start;
    bool covidTest = false;
    while(1)
    {
        addToTimespec(&nextTestCOVIDTime,threadTestCOVID_sleepTime);
        covidTest = testCOVID();
        if (covidTest == true)
        {
            pthread_mutex_lock(&contacts_mutex);

            // Extract the close contacts and feed their MACs to uploadContacts()
            struct contact closeContacts[60]; // Max number of close contacts = #contacts/2
            int closeContacts_writeIndex = 0;
            int current_contact_index = 0;
            struct timespec currentContact_timestamp;
            struct timespec examinedContact_timestamp;
            long long contact_timeDiff = 0;

            // Loop through all the contacts in the contacts array
            while(current_contact_index<120 && contacts[current_contact_index].timestamp.tv_sec > 0)
            {
                currentContact_timestamp = contacts[current_contact_index].timestamp;
                int examined_contact_index = current_contact_index + 1;

                // For every contact, loop through every contact after it and check if they match
                while (examined_contact_index<120 && contacts[examined_contact_index].timestamp.tv_sec > 0)
                {
                    examinedContact_timestamp = contacts[examined_contact_index].timestamp;

                    // Time difference of the two contacts
                    contact_timeDiff = (examinedContact_timestamp.tv_sec-currentContact_timestamp.tv_sec)*1000000000 +
                                       examinedContact_timestamp.tv_nsec - currentContact_timestamp.tv_nsec;

                    // If the matching contacts differ in time by at least 2.4 seconds and no more than 12 seconds
                    if (contact_timeDiff >= 2400000000 && contact_timeDiff <= 12000000000)
                    {
                        // Check whether this close contact is already in the closeContacts array
                        bool isCloseContact = false;
                        for (int i=0; i<closeContacts_writeIndex; i++)
                        {
                            if ( equalMACs(closeContacts[i].MAC,contacts[examined_contact_index].MAC) == true )
                            {
                                isCloseContact = true;
                            }
                        }
                        // If the close contact is not already in the closeContacts array, add it
                        if (isCloseContact == false)
                        {
                            closeContacts[closeContacts_writeIndex] = contacts[examined_contact_index];
                            closeContacts_writeIndex++;
                        }
                        if (closeContacts_writeIndex>=60)
                        {
                            //printf("Inner break contact check. \n");
                            break;
                        }
                    }
                    examined_contact_index++;
                }
                current_contact_index++;
                if (closeContacts_writeIndex>=60)
                {
                    //printf("Break contact check. \n");
                    break;
                }
            }
            pthread_mutex_unlock(&contacts_mutex);

            // Now we have all of the close contacts, we extract their MACs
            //printf("closeContacts_writeIndex: %d\n",closeContacts_writeIndex);
            struct MACaddress closeContacts_MACs[closeContacts_writeIndex];
            for (int j=0; j<closeContacts_writeIndex; j++)
            {
                closeContacts_MACs[j] = closeContacts[j].MAC;
            }
            //print the contacts array to "allContacts.txt" file, for cross-referencing
            printContactsArray();
            // print close contact MACs to the "closeContacts.txt"
            uploadContacts(closeContacts_MACs, closeContacts_writeIndex);
        }

        // While loop (thread) termination
        // Calculate thread runtime
        clock_gettime(CLOCK_REALTIME, &threadTestCOVID_current);
        threadTestCOVID_seconds = (threadTestCOVID_current.tv_sec - threadTestCOVID_start.tv_sec);
        threadTestCOVID_nanos = ((threadTestCOVID_seconds * 1000000000) + threadTestCOVID_current.tv_nsec) - (threadTestCOVID_start.tv_nsec);
        if (threadTestCOVID_nanos >= RUNTIME_NSECS)
        {
            break;
        }

        // Sleep until 2.4 secs later (since from the start of the while loop), then repeat all over
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &nextTestCOVIDTime, NULL);
    }

    return NULL;
}

// Main function to be run from main()
int programFunction(){
   // ****** PROGRAM START ******

    // Generate all MAC addresses of possible contacts
    initializeMACs();

    // Initialize all contacts' timestamps to 0 secs, makes checking for 'NULL' elements easy
    for (int i=0; i<120; i++)
    {
        contacts[i].timestamp.tv_sec=0;
    }

    // Initialize the contacts_mutex
    if (pthread_mutex_init(&contacts_mutex, NULL) != 0)
    {
        printf("\n Contacts mutex initilization has failed. \n");
        return 1;
    }
    // Create and join the threads for the program's runtime
    pthread_t threadBTnearMeId, threadTestCOVIDId;

    int error;
    error = pthread_create(&threadBTnearMeId, NULL, threadBTnearMe, NULL); // BTnearMe() thread
    if (error != 0)
    {
        printf("threadBTnearMe can't be created, error:[%s] \n",strerror(error));
        return 2;
    }

    error = pthread_create(&threadTestCOVIDId, NULL, threadTestCOVID, NULL); // testCOVID() thread
    if (error != 0)
    {
        printf("threadTestCOVID can't be created, error :[%s] \n",strerror(error));
        return 3;
    }

    pthread_join(threadBTnearMeId, NULL);
    pthread_join(threadTestCOVIDId, NULL);
    pthread_mutex_destroy(&contacts_mutex);
    // ****** PROGRAM END ******
    return 0; //all well
}
