#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

// total number of sharks and divers
const int SHARK_COUNT = 6;
const int DIVER_COUNT = 2;

// capacity of the reef (this many sharks OR divers)
const int MAX_IN_REEF = 2;

//wait time scaling factor
const int SCALING_FACTOR = 1;

// max time a shark waits before getting hungry (in microseconds)
const int SHARK_WAITING_TIME = SCALING_FACTOR * 2000000;

// max time a shark spends feeding in the reef
const int SHARK_FISHING_TIME = SCALING_FACTOR * 2000000;

// max time a diver waits before wanting to fish
const int DIVER_WAITING_TIME = SCALING_FACTOR * 2000000;

// max time a diver spends fishing in the reef
const int DIVER_FISHING_TIME = SCALING_FACTOR * 2000000;

// total time the simulation should run (in seconds)
const int TOTAL_SECONDS = 60;

const int initial_count = 7;

const double sratio = SHARK_COUNT / DIVER_COUNT;
const double dratio = DIVER_COUNT / SHARK_COUNT;

// whether or not each shark/diver is currently in the reef
bool *divers_fishing;
bool *sharks_feeding;


// declare synchronization variables here
sem_t mutex, sharkcanfish, divercanfish;
int sharks_in_reef, divers_in_reef, sharksfed, diversfed = 0;

// end synchronization variables


void report(void) {
    // shark report
    int total_sharks = 0;
    char shark_report[100];
    shark_report[0] = 0;

    for (int i = 0; i < SHARK_COUNT; i++) {
        if (sharks_feeding[i]) {
            strcat(shark_report, "+");
            total_sharks++;
        } else {
            strcat(shark_report, " ");
        }
    }

    // diver report
    int total_divers = 0;
    char diver_report[100];
    diver_report[0] = 0;

    for (int i = 0; i < DIVER_COUNT; i++) {
        if (divers_fishing[i]) {
            strcat(diver_report, "*");
            total_divers++;
        } else {
            strcat(diver_report, " ");
        }
    }

    // reef report
    char reef_report[100];
    reef_report[0] = 0;
    for (int i = 0; i < total_sharks; i++)
        strcat(reef_report, "+");
    for (int i = 0; i < total_divers; i++)
        strcat(reef_report, "*");
    for (int i = strlen(reef_report); i < MAX_IN_REEF; i++)
        strcat(reef_report, " ");

    printf("[%s] %s [%s]\n", shark_report, reef_report, diver_report);
    if (total_sharks > 0 && total_divers > 0)
        printf("!!! ERROR: diver getting eaten\n");

    fflush(stdout);
}


// function to simulate a single shark
void *shark(void *arg) {
    int k = *(int *) arg;
    printf("starting shark #%d\n", k);
    
    for (;;) {
        // sleep for some time
        usleep(random() % SHARK_WAITING_TIME);


        //start feeding
        sem_wait(&mutex);
        bool handler = false;
        if(SHARK_COUNT > DIVER_COUNT){
            handler = sharksfed <= (sratio*diversfed+1);
        } else {
            handler = true;
        }
        if (sharks_in_reef < 2 && !divers_in_reef && handler){
            sem_wait(&sharkcanfish);
            sharks_in_reef++;
            sharksfed++;
            sharks_feeding[k] = true;
            report();
            sem_post(&mutex);
        } else {
            sem_post(&mutex);
            continue;
        }


        usleep(random() % SHARK_FISHING_TIME);


        sem_wait(&mutex);
        sharks_in_reef--;
        sharks_feeding[k] = false;
        report();
        if (sharks_in_reef < 2 && !divers_in_reef)
            sem_post(&sharkcanfish);
        sem_post(&mutex);        
    }

    return NULL;
}

// function to simulate a single diver
void *diver(void *arg) {
    int k = *(int *) arg;
    printf("starting diver #%d\n", k);
    fflush(stdout);

    for (;;) {
        // sleep for some time
        usleep(random() % DIVER_WAITING_TIME);


        sem_wait(&mutex);
        bool handler = false;
        if(SHARK_COUNT > DIVER_COUNT){
            handler = true;
        } else {
            handler = diversfed <= (dratio*sharksfed+1);
        }
        if (divers_in_reef < 2 && !sharks_in_reef && handler){
            sem_wait(&divercanfish);
            divers_in_reef++;
            diversfed++;
            divers_fishing[k] = true;
            report();
            sem_post(&mutex);
        } else {
            sem_post(&mutex);
            continue;
        }


        usleep(random() % DIVER_FISHING_TIME);


        sem_wait(&mutex);
        divers_in_reef--;
        divers_fishing[k] = false;
        report();
        if (divers_in_reef < 2 && !sharks_in_reef)
            sem_post(&divercanfish);
        sem_post(&mutex);        
    }

    return NULL;
}

int main(int argc, char **argv) {
    //
    // initialize synchronization variables here
    //
    sem_init(&mutex, 0, 1);
    sem_init(&sharkcanfish, 0, 2);
    sem_init(&divercanfish, 0, 2);
    //
    // end of synchronization variable initialization
    //
    // initialize shared state
    sharks_feeding = malloc(sizeof(bool) * SHARK_COUNT);
    assert(sharks_feeding != NULL);
    for (int i = 0; i < SHARK_COUNT; i++)
        sharks_feeding[i] = false;

    divers_fishing = malloc(sizeof(bool) * SHARK_COUNT);
    assert(divers_fishing != NULL);
    for (int i = 0; i < DIVER_COUNT; i++)
        divers_fishing[i] = false;

    pthread_t sharks[SHARK_COUNT];
    pthread_t divers[DIVER_COUNT];

    // spawn the sharks
    int shark_counts[SHARK_COUNT];
    for (int i = 0; i < SHARK_COUNT; i++) {
        // create a new thread for this shark
        shark_counts[i] = i;
        int s = pthread_create(&sharks[i], NULL, shark, &shark_counts[i]);
        assert(s == 0);
        s = pthread_detach(sharks[i]);
        assert(s == 0);
    }

    // spawn the divers
    int diver_counts[DIVER_COUNT];
    for (int i = 0; i < DIVER_COUNT; i++) {
        // create a new thread for this diver
        diver_counts[i] = i;
        int s = pthread_create(&divers[i], NULL, diver, &diver_counts[i]);
        assert(s == 0);
        s = pthread_detach(divers[i]);
        assert(s == 0);
    }

    // let the simulation run for a while
    sleep(TOTAL_SECONDS);
    fflush(stdout);

    return 0;
}
