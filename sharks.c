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
const int SCALING_FACTOR = 2;

// max time a shark waits before getting hungry (in microseconds)
const int SHARK_WAITING_TIME = SCALING_FACTOR * 3000000;

// max time a shark spends feeding in the reef
const int SHARK_FISHING_TIME = SCALING_FACTOR * 1000000;

// max time a diver waits before wanting to fish
const int DIVER_WAITING_TIME = SCALING_FACTOR * 500000;

// max time a diver spends fishing in the reef
const int DIVER_FISHING_TIME = SCALING_FACTOR * 500000;

// total time the simulation should run (in seconds)
const int TOTAL_SECONDS = 60;

const int initial_count = 9;

// whether or not each shark/diver is currently in the reef
bool *divers_fishing;
bool *sharks_feeding;


// declare synchronization variables here
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

int reef1;
int reef2;

pthread_cond_t cond1;
pthread_cond_t cond2;

int sharks_in_1;
int sharks_in_2;
int divers_in_1;
int divers_in_2;
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

        //
        // write code here to safely start the shark feeding
        // note: call report() after setting sharks_feeding[k] to true
        //

        //REEF 1
        //acquire lock1
        reef1 = pthread_mutex_lock(&mutex1);
        assert(reef1 == 0);

        //wait for cond1
        while (sharks_in_1 || divers_in_1 || divers_in_2) {
            reef1 = pthread_cond_wait(&cond1, &mutex1);
            assert(reef1 == 0);
        }
        sharks_in_1++;
        sharks_feeding[k] = true;
        report();
       
        //release lock1
        reef1 = pthread_mutex_unlock(&mutex1);
        assert(reef1 == 0);


        /* feed for a while */
        usleep(random() % SHARK_FISHING_TIME);


        //acquire lock1
        reef1 = pthread_mutex_lock(&mutex1);
        assert(reef1 == 0);
        
        //signal that reef1 is free
        sharks_in_1--;
        sharks_feeding[k] = false;
        reef1 = pthread_cond_signal(&cond1);
        assert(reef1 == 0);
        report();

        //release lock1
        reef1 = pthread_mutex_unlock(&mutex1);
        assert(reef1 == 0);


        //***********************************//
        usleep(random() % SHARK_WAITING_TIME);
        //***********************************//


        //REEF 2
        //acquire lock2
        reef2 = pthread_mutex_lock(&mutex2);
        assert(reef2 == 0);

        //wait on cond2
        while (sharks_in_2 || divers_in_1 || divers_in_2){
            reef2 = pthread_cond_wait(&cond2, &mutex2);
            assert(reef2 == 0);
        }
        sharks_in_2++;
        sharks_feeding[k] = true;
        report();

        //release lock2
        reef2 = pthread_mutex_unlock(&mutex2);
        assert(reef2 == 0);


        //wait
        usleep(random() % SHARK_FISHING_TIME);


        //acquire lock2
        reef2 = pthread_mutex_lock(&mutex2);
        assert(reef2 == 0);
        
        //signal reef2 free
        sharks_in_2--;
        sharks_feeding[k] = false;
        reef2 = pthread_cond_signal(&cond2);
        assert(reef2 == 0);
        report();

        //release lock2
        reef2 = pthread_mutex_unlock(&mutex2);
        assert(reef2 == 0);
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

        //
        // write code here to safely start the diver fishing
        // note: call report() after setting divers_fishing[k] to true
        //
        // Start REEF 1
        //acquire lock1
        reef1 = pthread_mutex_lock(&mutex1);
        assert(reef1 == 0);

        //wait on cond1
        while (sharks_in_1 || sharks_in_2 || divers_in_1 ){
            reef1 = pthread_cond_wait(&cond1, &mutex1);
            assert(reef1 == 0);
        }
        divers_in_1++;
        divers_fishing[k] = true;
        report();

        //release lock1
        reef1 = pthread_mutex_unlock(&mutex1);
        assert(reef1 == 0);


        // fish for a while
        usleep(random() % DIVER_FISHING_TIME);


        //STOP REEF 1
        reef1 = pthread_mutex_lock(&mutex1);
        assert(reef1 == 0);
        
        divers_in_1--;
        divers_fishing[k] = false;
        reef1 = pthread_cond_signal(&cond1);
        assert(reef1 == 0);
        report();

        reef1 = pthread_mutex_unlock(&mutex1);
        assert(reef1 == 0);


        usleep(random() % DIVER_WAITING_TIME);


        // Start REEF 2
        reef2 = pthread_mutex_lock(&mutex2);
        assert(reef2 == 0);

        while (sharks_in_1 || sharks_in_2 || divers_in_2){
            reef2 = pthread_cond_wait(&cond2, &mutex2);
            assert(reef2 == 0);
        }
        divers_in_2++;
        divers_fishing[k] = true;
        report();

        reef2 = pthread_mutex_unlock(&mutex2);
        assert(reef2 == 0);



        // fish for a while
        usleep(random() % DIVER_FISHING_TIME);


        //STOP REEF 2
        reef2 = pthread_mutex_lock(&mutex2);
        assert(reef2 == 0);
        
        divers_in_2--;
        divers_fishing[k] = false;
        reef2 = pthread_cond_signal(&cond2);
        assert(reef2 == 0);
        report();

        reef2 = pthread_mutex_unlock(&mutex2);
        assert(reef2 == 0);


    }

    return NULL;
}

int main(int argc, char **argv) {
    //
    // initialize synchronization variables here
    //
    reef1 = pthread_mutex_init(&mutex1, NULL);
    reef2 = pthread_mutex_init(&mutex2, NULL);
    assert(reef1 == 0);
    assert(reef2 == 0);
    reef1 = pthread_cond_init(&cond1, NULL);
    reef2 = pthread_cond_init(&cond2, NULL);
    assert(reef1 == 0);
    assert(reef2 == 0);
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
