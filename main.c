#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

int simulationDays;
int numOfPassengers;
int numOfAgents;
int numOfSeats;
int seed;
int numOfTours;


int busList[5000];
int busIndex = 0;
int busSeats[5000];
int seatsAvailable;
int seatsReserveAvailable;
int busSeatStatus[5000];
int reservationAllowance[5000];

int day = 0;

bool finish = false;

time_t rawtime;
struct tm * timeinfo;

pthread_mutex_t wMutex;
pthread_mutex_t fullMutex;
pthread_mutex_t logMutex;
pthread_mutex_t mutexArray[5000];
pthread_mutex_t mutexP[5000];
pthread_cond_t condArray[5000];


FILE *log_file;

void reserveTicket(int PID, int AID) {
    
    if(reservationAllowance[PID-1] == 0){
        //printf("Daily reservation limit exceeded for passenger %d", PID);
    }
    else{
        
        /*if(seatsAvailable == 0 && AID == 0) {
         printf("Wait ediyor %d seatsAvailable %d\n", PID, seatsAvailable);
         pthread_cond_wait(&condArray[0], &mutexArray[499]);
         }*/
        
        if(seatsReserveAvailable <= 0 && AID == 0) {
            seatsReserveAvailable = 0;
            pthread_mutex_lock(&wMutex);
            busList[busIndex] = PID;
            busIndex++;
            pthread_mutex_unlock(&wMutex);
            //printf("Reserve Wait ediyor %d\n", PID);
            pthread_cond_wait(&condArray[PID-1], &mutexP[PID-1]);
            //printf("Reserve Wait ediyor %d\n", PID);
        }
        
        if(!finish) {
            time_t rawtimeR;
            struct tm * timeinfoR;
            time ( &rawtimeR );
            timeinfoR = localtime ( &rawtimeR );
            bool searchSeat = true;
            int seatNo = 0;
            
            while(searchSeat) {
                
                seatNo++;
                
                pthread_mutex_lock(&mutexArray[seatNo-1]);
                //critical section for individual seat for buy operation
                int tour = ((seatNo-1)/numOfSeats)+1;
                int seat = ((seatNo-1)%numOfSeats)+1;
                if(busSeats[seatNo-1] == 0){
                    busSeats[seatNo-1] = PID;
                    busSeatStatus[seatNo-1] = 1;
                    seatsReserveAvailable--;
                    searchSeat = false;
                    reservationAllowance[PID-1]--;
                    pthread_mutex_lock(&logMutex);
                    fprintf(log_file, "%d:%d:%d        %d       %d       R       %d        %d\n", timeinfoR->tm_hour, timeinfoR->tm_min, timeinfoR->tm_sec, PID, AID, seat, tour);
                    //printf("%d:%d:%d      Reserved TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoR->tm_hour, timeinfoR->tm_min, timeinfoR->tm_sec, seat, tour, PID, AID);
                    pthread_mutex_unlock(&logMutex);
                }
                
                if(seatNo >= numOfSeats*numOfTours) {
                    searchSeat = false;
                }
                
                pthread_mutex_unlock(&mutexArray[seatNo-1]);
                
            }
        }
        else {
            //printf("Devam etti %d\n", PID);
        }
    }
    
}

void buyTicket(int PID, int AID) {
    //printf("seatsAvailable %d\n", seatsAvailable);
    if(seatsAvailable <= 0 && AID == 0) {
        seatsAvailable = 0;
        pthread_mutex_lock(&wMutex);
        busList[busIndex] = PID;
        busIndex++;
        pthread_mutex_unlock(&wMutex);
        //printf("Buy Wait ediyor %d\n", PID);
        pthread_cond_wait(&condArray[PID-1], &mutexP[PID-1]);
        //printf("Buy Devam ediyor %d\n", PID);
    }
    
    if(!finish) {
        time_t rawtimeB;
        struct tm * timeinfoB;
        time ( &rawtimeB );
        timeinfoB = localtime ( &rawtimeB );
        bool searchSeat = true;
        int seatNo = 0;
        
        while(searchSeat) {
            
            seatNo++;
            
            pthread_mutex_lock(&mutexArray[seatNo-1]);
            //critical section for individual seat for buy operation
            int tour = ((seatNo-1)/numOfSeats)+1;
            int seat = ((seatNo-1)%numOfSeats)+1;
            if(busSeats[seatNo-1] == 0){
                busSeats[seatNo-1] = PID;
                busSeatStatus[seatNo-1] = 2;
                seatsAvailable--;
                seatsReserveAvailable--;
                searchSeat = false;
                pthread_mutex_lock(&logMutex);
                fprintf(log_file, "%d:%d:%d        %d       %d       B       %d        %d\n", timeinfoB->tm_hour, timeinfoB->tm_min, timeinfoB->tm_sec, PID, AID, seat, tour);
                //printf("%d:%d:%d      Bought TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoB->tm_hour, timeinfoB->tm_min, timeinfoB->tm_sec, seat, tour, PID, AID);
                pthread_mutex_unlock(&logMutex);
            }
            else if((busSeats[seatNo-1] == PID) && (busSeatStatus[seatNo-1] == 1)){
                busSeatStatus[seatNo-1] = 2;
                searchSeat = false;
                seatsAvailable--;
                reservationAllowance[PID-1]++;
                pthread_mutex_lock(&logMutex);
                fprintf(log_file, "%d:%d:%d        %d       %d       B       %d        %d\n", timeinfoB->tm_hour, timeinfoB->tm_min, timeinfoB->tm_sec, PID, AID, seat, tour);
                //printf("%d:%d:%d      Bought reserved TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoB->tm_hour, timeinfoB->tm_min, timeinfoB->tm_sec, seat, tour, PID, AID);
                pthread_mutex_unlock(&logMutex);
            }
            
            if(seatNo >= numOfSeats*numOfTours) {
                searchSeat = false;
            }
            
            pthread_mutex_unlock(&mutexArray[seatNo-1]);
            
        }
    }
    
}

void cancelTicket(int PID, int AID) {
    time_t rawtimeC;
    struct tm * timeinfoC;
    time ( &rawtimeC );
    timeinfoC = localtime ( &rawtimeC );
    
    if(AID == 0) {
        int seatNo = 0;
        
        for(int i=0; i<numOfSeats*numOfTours; i++){
            if(busSeats[i] == PID){
                seatNo = i + 1;
                break;
            }
        }
        
        if(seatNo != 0 && seatNo<= numOfSeats*numOfTours) {
            
            pthread_mutex_lock(&mutexArray[seatNo-1]);
            //critical section for individual seat for buy operation
            
            if(busSeats[seatNo-1] == PID) {
                int tour = ((seatNo-1)/numOfSeats)+1;
                int seat = ((seatNo-1)%numOfSeats)+1;
                
                if(busSeatStatus[seatNo-1] == 1) {
                    busSeats[seatNo-1] = 0;
                    busSeatStatus[seatNo-1] = 0;
                    seatsReserveAvailable++;
                    reservationAllowance[PID-1]++;
                    pthread_mutex_lock(&logMutex);
                    fprintf(log_file, "%d:%d:%d        %d       %d       C       %d        %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, PID, AID, seat, tour);
                    //printf("%d:%d:%d      Cancelled TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, seat, tour, PID, AID);
                    pthread_mutex_unlock(&logMutex);
                }
                else if(busSeatStatus[seatNo-1] == 2) {
                    busSeats[seatNo-1] = 0;
                    busSeatStatus[seatNo-1] = 0;
                    seatsAvailable++;
                    seatsReserveAvailable++;
                    pthread_mutex_lock(&logMutex);
                    fprintf(log_file, "%d:%d:%d        %d       %d       C       %d        %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, PID, AID, seat, tour);
                    //printf("%d:%d:%d      Cancelled TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, seat, tour, PID, AID);
                    pthread_mutex_unlock(&logMutex);
                }
                
                for(int a=0; a<numOfPassengers; a++) {
                    pthread_cond_signal(&condArray[a]);
                    //printf("gfgfg   %d\n", AID);
                }
                pthread_mutex_lock(&wMutex);
                for(int z=0; z<busIndex; z++) {
                    busList[z] = 0;
                }
                busIndex = 0;
                pthread_mutex_unlock(&wMutex);
            }
            
            pthread_mutex_unlock(&mutexArray[seatNo-1]);
        }
    }
    else {
        int seatNo = (rand()%(numOfSeats*numOfTours))+1;
        
        pthread_mutex_lock(&mutexArray[seatNo-1]);
        //critical section for individual seat for buy operation
        
        int tour = ((seatNo-1)/numOfSeats)+1;
        int seat = ((seatNo-1)%numOfSeats)+1;
        
        if(busSeatStatus[seatNo-1] == 1) {
            int oldOwner = busSeats[seatNo-1];
            busSeats[seatNo-1] = 0;
            busSeatStatus[seatNo-1] = 0;
            seatsReserveAvailable++;
            reservationAllowance[oldOwner-1]++;
            pthread_mutex_lock(&logMutex);
            fprintf(log_file, "%d:%d:%d        %d       %d       C       %d        %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, oldOwner, AID, seat, tour);
            //printf("%d:%d:%d      Cancelled TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, seat, tour, oldOwner, AID);
            pthread_mutex_unlock(&logMutex);
        }
        else if(busSeatStatus[seatNo-1] == 2) {
            int oldOwner = busSeats[seatNo-1];
            busSeats[seatNo-1] = 0;
            busSeatStatus[seatNo-1] = 0;
            seatsAvailable++;
            seatsReserveAvailable++;
            pthread_mutex_lock(&logMutex);
            fprintf(log_file, "%d:%d:%d        %d       %d       C       %d        %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, oldOwner, AID, seat, tour);
            //printf("%d:%d:%d      Cancelled TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoC->tm_hour, timeinfoC->tm_min, timeinfoC->tm_sec, seat, tour, oldOwner, AID);
            pthread_mutex_unlock(&logMutex);
        }
        
        for(int a=0; a<numOfPassengers; a++) {
            pthread_cond_signal(&condArray[a]);
            //printf("gfgfg   %d\n", AID);
        }
        pthread_mutex_lock(&wMutex);
        for(int z=0; z<busIndex; z++) {
            busList[z] = 0;
        }
        busIndex = 0;
        pthread_mutex_unlock(&wMutex);
        pthread_mutex_unlock(&mutexArray[seatNo-1]);
    }
    
    
}

void viewTicket(int PID, int AID) {
    time_t rawtimeV;
    struct tm * timeinfoV;
    time ( &rawtimeV );
    timeinfoV = localtime ( &rawtimeV );
    
    for(int j=0; j<numOfSeats*numOfTours; j++) {
        
        int tour = (j/numOfSeats)+1;
        int seat = (j%numOfSeats)+1;
        
        if ((busSeats[j] == PID) && (busSeatStatus[j] == 1)){
            pthread_mutex_lock(&logMutex);
            fprintf(log_file, "%d:%d:%d        %d       %d       V       %d        %d\n", timeinfoV->tm_hour, timeinfoV->tm_min, timeinfoV->tm_sec, PID, AID, seat, tour);
            //printf("%d:%d:%d      Viewed TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoV->tm_hour, timeinfoV->tm_min, timeinfoV->tm_sec, seat, tour, PID, AID);
            pthread_mutex_unlock(&logMutex);
            break;
        }
        else if ((busSeats[j] == PID) && (busSeatStatus[j] == 2)){
            pthread_mutex_lock(&logMutex);
            fprintf(log_file, "%d:%d:%d        %d       %d       V       %d        %d\n", timeinfoV->tm_hour, timeinfoV->tm_min, timeinfoV->tm_sec, PID, AID, seat, tour);
            //printf("%d:%d:%d      Viewed TicketNo %d in Tour %d for Passenger %d by Agent %d\n", timeinfoV->tm_hour, timeinfoV->tm_min, timeinfoV->tm_sec, seat, tour, PID, AID);
            pthread_mutex_unlock(&logMutex);
            break;
        }
    }
    
}


void *passengerFunction(void *ID) {
    
    int id = (int) ID;
    
    time_t rawtime2;
    struct tm * timeinfo2;
    int secondDifference = 0;
    
    //printf("hjgjgjn\n");
    
    while(secondDifference <= simulationDays*5) {
        
        int op = rand()%10;
        //int op = 11;
        //printf("Op: %d\n", op);
        if((0<=op) && (op<=3)) {
            
            /*if(seatsAvailable == 0 && id != 1) {
             printf("Wait ediyor %d seatsAvailable %d\n", id, seatsAvailable);
             pthread_cond_wait(&condArray[id-1], &mutexP[id-1]);
             printf("Devam ediyor %d seatsAvailable %d\n", id, seatsAvailable);
             printf("Wait index: %d\n", waitIndex);
             waitList[waitIndex] = 2;
             waitIndex++;
             printf("Wait list: %d\n", waitList[waitIndex-1]);
             }*/
            
            reserveTicket(id, 0);
        }
        else if((op == 4) || (op == 5)) {
            buyTicket(id, 0);
        }
        else if((op == 6) || (op == 7)) {
            viewTicket(id, 0);
        }
        else if((op == 8) || (op == 9)) {
            cancelTicket(id, 0);
        }
        
        time ( &rawtime2 );
        timeinfo2 = localtime ( &rawtime2 );
        
        int startHour = timeinfo->tm_hour;
        int startMinute = timeinfo->tm_min;
        int startSecond = timeinfo->tm_sec;
        
        int currentHour = timeinfo2->tm_hour;
        int currentMinute = timeinfo2->tm_min;
        int currentSecond = timeinfo2->tm_sec;
        
        int seconds1 = 3600*startHour + 60*startMinute + startSecond;
        int seconds2 = 3600*currentHour + 60*currentMinute + currentSecond;
        
        secondDifference = seconds2 - seconds1;
        int dayNumber = (secondDifference/5)+1;
        
    }
    
    pthread_exit(0);
}

void *agentFunction(void *ID) {
    int id = (int) ID;
    
    int PID = (rand() % numOfPassengers)+1;
    
    time_t rawtime2;
    struct tm * timeinfo2;
    int secondDifference = 0;
    
    while(secondDifference <= simulationDays*5) {
        
        int op = rand()%10;
        //int op = 6;
        //printf("Op: %d\n", op);
        if((0<=op) && (op<=3)) {
            reserveTicket(PID, id);
        }
        else if((op == 4) || (op == 5)){
            buyTicket(PID, id);
        }
        else if((op == 6) || (op == 7)) {
            viewTicket(PID, id);
        }
        else if((op == 8) || (op == 9)) {
            cancelTicket(PID, id);
        }
        
        time ( &rawtime2 );
        timeinfo2 = localtime ( &rawtime2 );
        
        int startHour = timeinfo->tm_hour;
        int startMinute = timeinfo->tm_min;
        int startSecond = timeinfo->tm_sec;
        
        int currentHour = timeinfo2->tm_hour;
        int currentMinute = timeinfo2->tm_min;
        int currentSecond = timeinfo2->tm_sec;
        
        int seconds1 = 3600*startHour + 60*startMinute + startSecond;
        int seconds2 = 3600*currentHour + 60*currentMinute + currentSecond;
        
        secondDifference = seconds2 - seconds1;
        int dayNumber = (secondDifference/5)+1;
        
        if(dayNumber > day) {
            
            day = dayNumber;
            //printf("Day %d\n", day);
            if(day > 1 && day<simulationDays+1) {
                printf("======================\n");
                printf("Day %d\n", day-1);
                printf("======================\n");
                printf("Reserved Seats: ");
                for(int m=0; m<numOfSeats*numOfTours; m++) {
                    int tour = (m/numOfSeats)+1;
                    int seat = (m%numOfSeats)+1;
                    if(seat == 1) {
                        printf("\nTour %d:", tour);
                    }
                    if(busSeatStatus[m] == 1) {
                        printf(" %d ", seat);
                    }
                }
                printf("\n");
                printf("Bought Seats: ");
                for(int n=0; n<numOfSeats*numOfTours; n++) {
                    int tour2 = (n/numOfSeats)+1;
                    int seat2 = (n%numOfSeats)+1;
                    if(seat2 == 1) {
                        printf("\nTour %d:", tour2);
                    }
                    if(busSeatStatus[n] == 2) {
                        printf(" %d ", seat2);
                    }
                }
                printf("\n");
                pthread_mutex_lock(&wMutex);
                printf("WaitingList Passenger ID:\n");
                for(int z=0; z<busIndex; z++) {
                    printf("%d ", busList[z]);
                }
                printf("\n======================\n");
                pthread_mutex_unlock(&wMutex);
                
            }
            for(int z=0; z<numOfPassengers; z++) {
                reservationAllowance[z] = 2;
            }
        }
    }
    
    finish = true;
    if(id == 1) {
        for(int a=0; a<numOfPassengers; a++) {
            pthread_cond_signal(&condArray[a]);
        }

    }
    
    /*if(id == 1) {
     
     printf("======================\n");
     printf("Day %d\n", simulationDays);
     printf("======================\n");
     printf("Reserved Seats: ");
     for(int m=0; m<numOfSeats*numOfTours; m++) {
     int tour = (m/numOfSeats)+1;
     int seat = (m%numOfSeats)+1;
     if(seat == 1) {
     printf("\nTour %d:", tour);
     }
     if(busSeatStatus[m] == 1) {
     printf(" %d ", m+1);
     }
     }
     printf("\n");
     printf("Bought Seats: ");
     for(int n=0; n<numOfSeats*numOfTours; n++) {
     int tour2 = (n/numOfSeats)+1;
     int seat2 = (n%numOfSeats)+1;
     if(seat2 == 1) {
     printf("\nTour %d:", tour2);
     }
     if(busSeatStatus[n] == 2) {
     printf(" %d ", n+1);
     }
     }
     printf("\n======================\n");
     
     for(int z=0; z<numOfPassengers; z++) {
     reservationAllowance[z] = 2;
     }
     }*/
    
    pthread_exit(0);
}

int main(int argc, char** argv) {
    
    log_file = fopen("tickets.log", "w");
    fprintf(log_file, "  Time         PID     AID  OperationSeat No  Tour No\n");
    
    /*for(int w=0; w<9; w++) {
        printf("SeatNo: %d belongs to Passenger %d\n", w+1, busSeats[w]);
    }*/
    
    
    if(strcmp(argv[1], "-d") == 0) {
        simulationDays = atoi(argv[2]);
    }
    
    if(strcmp(argv[3], "-p") == 0) {
        numOfPassengers = atoi(argv[4]);
    }
    
    if(strcmp(argv[5], "-a") == 0) {
        numOfAgents = atoi(argv[6]);
    }
    
    if(strcmp(argv[7], "-t") == 0) {
        numOfTours = atoi(argv[8]);
        if(strcmp(argv[9], "-s") == 0) {
            numOfSeats = atoi(argv[10]);
        }
        
        if(strcmp(argv[11], "-r") == 0) {
            seed = atoi(argv[12]);
        }
    }
    else {
        numOfTours = 1;
        if(strcmp(argv[7], "-s") == 0) {
            numOfSeats = atoi(argv[8]);
        }
        
        if(strcmp(argv[9], "-r") == 0) {
            seed = atoi(argv[10]);
        }
    }
    
    srand(seed);
    
    seatsAvailable = numOfSeats*numOfTours;
    seatsReserveAvailable = numOfSeats*numOfTours;
    
    /*printf("d: %d\n", simulationDays);
    printf("p: %d\n", numOfPassengers);
    printf("a: %d\n", numOfAgents);
    printf("t: %d\n", numOfTours);
    printf("s: %d\n", numOfSeats);
    printf("r: %d\n", seed);*/
    
    for(int z=0; z<numOfPassengers; z++) {
        reservationAllowance[z] = 2;
    }
    
    //threads
    pthread_t passengerThreads[numOfPassengers];
    pthread_t agentThreads[numOfAgents];
    
    //mutex and cv initialization
    for(int a=0; a<numOfSeats*numOfTours; a++) {
        pthread_mutex_init(&mutexArray[a], NULL);
    }
    
    for(int s=0; s<numOfPassengers; s++) {
        pthread_cond_init(&condArray[s], NULL);
    }
    
    for(int u=0; u<numOfPassengers; u++) {
        pthread_mutex_init(&mutexP[u], NULL);
    }
    
    pthread_mutex_init(&wMutex, NULL);
    pthread_mutex_init(&fullMutex, NULL);
    pthread_mutex_init(&logMutex, NULL);
    pthread_mutex_init(&mutexArray[499], NULL);
    //pthread_cond_init(&condArray[0], NULL);
    
    //time
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    //printf("%d:%d:%d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    for(int i=0; i<numOfPassengers; i++) {
        pthread_create(&passengerThreads[i], NULL, passengerFunction, (void*) i+1);
    }
    
    for(int j=0; j<numOfAgents; j++) {
        pthread_create(&agentThreads[j], NULL, agentFunction, (void*) j+1);
    }
    
    for (int k=0; k<numOfPassengers; k++) {
        pthread_join(passengerThreads[k], NULL);
    }
    
    for (int l=0; l<numOfAgents; l++) {
        pthread_join(agentThreads[l], NULL);
    }
    
    printf("======================\n");
    printf("Day %d\n", simulationDays);
    printf("======================\n");
    printf("Reserved Seats: ");
    for(int m=0; m<numOfSeats*numOfTours; m++) {
        int tour = (m/numOfSeats)+1;
        int seat = (m%numOfSeats)+1;
        if(seat == 1) {
            printf("\nTour %d:", tour);
        }
        if(busSeatStatus[m] == 1) {
            printf(" %d ", seat);
        }
    }
    printf("\n");
    printf("Bought Seats: ");
    for(int n=0; n<numOfSeats*numOfTours; n++) {
        int tour2 = (n/numOfSeats)+1;
        int seat2 = (n%numOfSeats)+1;
        if(seat2 == 1) {
            printf("\nTour %d:", tour2);
        }
        if(busSeatStatus[n] == 2) {
            printf(" %d ", seat2);
        }
    }
    printf("\n");
    pthread_mutex_lock(&wMutex);
    printf("WaitingList Passenger ID:\n");
    for(int z=0; z<busIndex; z++) {
        printf("%d ", busList[z]);
    }
    printf("\n======================\n");
    pthread_mutex_unlock(&wMutex);
    
    
    /* Clean up and exit */
    time_t rawtimeF;
    struct tm * timeinfoF;
    time ( &rawtimeF );
    timeinfoF = localtime ( &rawtimeF );
    //printf ("%d:%d:%d      Cleaned up\n", timeinfoF->tm_hour, timeinfoF->tm_min, timeinfoF->tm_sec);

    for(int m=0; m<numOfSeats*numOfTours; m++) {
        pthread_mutex_destroy(&mutexArray[m]);
    }
    for(int f=0; f<numOfPassengers; f++) {
        pthread_cond_destroy(&condArray[f]);
    }
    for(int y=0; y<numOfPassengers; y++) {
        pthread_mutex_destroy(&mutexP[y]);
    }
    /*for(int o=0; o<numOfSeats*numOfTours; o++) {
        printf("SeatNo: %d belongs to Passenger %d\n", o+1, busSeats[o]);
    }*/
    //printf("SimulationDays: %d\n", simulationDays);
    
    pthread_mutex_destroy(&wMutex);
    pthread_mutex_destroy(&fullMutex);
    pthread_mutex_destroy(&mutexArray[499]);
    pthread_mutex_destroy(&logMutex);
    //pthread_cond_destroy(&condArray[0]);
    pthread_exit(NULL);
    
    return 0;
}
