#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define numR 12
#define numC 20
#define NUMT 4

int A[numR][numC];
int B[numR][numC];

pthread_mutex_t lock;
pthread_barrier_t bar;

int x;
int y;
int currNum;
int numTasks;
bool incomplete;
int totalChecks = 0;

void fillGrid(int G[numR][numC], int seed, int high)
{
        srand(seed);
        int r = 0;
        //so ... while G isn't full ...
        for (int i = 0; i < numR; i++)
        {
                for (int j = 0; j < numC; j++)
                {
                        r = rand() % high;
                        //randomly generate a num from seed (mod high) and put that num in the next slot
                        G[i][j] = r;
                }
        }
}

void printGrid(int G[numR][numC])
{
        for (int i = 0; i < numR; i++)
        {
                for (int j = 0; j < numC; j++)
                {
                        printf("%3i ", G[i][j]);
                }
                printf("\n");
        }
}

void *compute(void *vargp)
{
        //while all tasks haven't been completed, keep looping
        int k = 0;

        pthread_mutex_lock(&lock);
        long thisid = (long)vargp;
        int thisy = y;
        int thisx = x;
        pthread_mutex_unlock(&lock);

        while (incomplete)
        {
                pthread_mutex_lock(&lock);
                long thisid = (long)vargp;
                int thisy = y;
                int thisx = x;
                //update y and x
                if ((x + 1) == numC && (y + 1) == numR)
                {
                        incomplete = false;
                }
                else if ((x + 1) == numC)
                {
                        y = y + 1;
                        x = 0;
                }
                else
                {
                        x = x + 1;
                }
                pthread_mutex_unlock(&lock);
                //printf("Got id %i, y = %i, x = %i\n", thisid, thisy, thisx); // testing line
                int final = A[thisy][thisx];
                int totalSum = final;

                if ((thisy - 1) >= 0)
                {
                        pthread_mutex_lock(&lock);
                        totalSum = totalSum + A[thisy - 1][thisx];
                        if ((thisx - 1) >= 0)
                        {
                                totalSum = totalSum + A[thisy - 1][thisx - 1];
                        }
                        if ((thisx + 1) < numC)
                        {
                                totalSum = totalSum + A[thisy - 1][thisx + 1];
                        }
                        pthread_mutex_unlock(&lock);
                }

                if ((thisy + 1) < numR)
                {
                        pthread_mutex_lock(&lock);
                        totalSum = totalSum + A[thisy + 1][thisx];
                        if ((thisx - 1) >= 0)
                        {
                                totalSum = totalSum + A[thisy + 1][thisx - 1];
                        }
                        if ((thisx + 1) < numC)
                        {
                                totalSum = totalSum + A[thisy + 1][thisx + 1];
                        }
                        pthread_mutex_unlock(&lock);
                }

                if ((thisx - 1) >= 0)
                {
                        pthread_mutex_lock(&lock);
                        totalSum = totalSum + A[thisy][thisx - 1];
                        pthread_mutex_unlock(&lock);
                }

                if ((thisx + 1) < numC)
                {
                        pthread_mutex_lock(&lock);
                        totalSum = totalSum + A[thisy][thisx + 1];
                        pthread_mutex_unlock(&lock);
                }

                usleep(totalSum%11*1500);
                //printf("sum for %d (y = %d, x = %d): %d \n", final,thisy,thisx, totalSum); //testing line

                if (totalSum % 10 == 0)
                {
                        final = 0;
                }
                else if (totalSum < 50)
                {
                        final = final + 3;
                }
                else if (totalSum < 150 && totalSum > 50)
                {
                        if ((final - 3) > 0)
                        {
                                final = final - 3;
                        }
                        else
                        {
                                final = 0;
                        }
                }
                else if (totalSum > 150)
                {
                        final = 1;
                }

                pthread_mutex_lock(&lock);
                B[thisy][thisx] = final;
                totalChecks++;
                //printf("Thread %i, B[%i][%i] = %d\n", thisid, thisy, thisx, B[thisy][thisx]); //testing line
                thisy = y;
                thisx = x;
                pthread_mutex_unlock(&lock);
                k++;
                //pthread_mutex_lock(&lock);

                //update y and x
        /*      if ((x + 1) == numC && (y + 1) == numR)
                {
                        incomplete = false;
                }
                else if ((x + 1) == numC)
                {
                        y = y + 1;
                        x = 0;
                }
                else
                {
                        x = x + 1;
                }
                pthread_mutex_unlock(&lock);*/
        }
        //printf("Thread %i exiting...\n", thisid); //testing line
        pthread_barrier_wait(&bar);
        pthread_mutex_lock(&lock);
        incomplete = true;
        pthread_mutex_unlock(&lock);
        pthread_exit(NULL);
}




int main()
{
        pthread_mutex_init(&lock, NULL);
        pthread_barrier_init(&bar, NULL, NUMT);
        printf("Using %i threads: \n", NUMT);
        fillGrid(A, 2, 20);

        printf("A: \n");
        pthread_mutex_lock(&lock);
        printGrid(A);
        incomplete = true;
        pthread_mutex_unlock(&lock);
        long k = 0;

        pthread_t tids1[NUMT];
        void *status;

        //printf("Creating threads\n"); //testing line
        clock_t startTime = clock();

        int p = 0;
        int gen = 10;
        while (p < gen)
        {
                pthread_mutex_lock(&lock);
                y = 0;
                x = 0;
                pthread_mutex_unlock(&lock);
                k = 0;

                void *status;
                while (k < NUMT)
                {
                        pthread_create(&tids1[k], NULL, compute, (void *)k);
                        //printf("p = %d, thread made = %d\n", p, k); //testing line
                k++;
                }
                //printf("Summing done\n"); //testing line
                for (int i = 0; i < NUMT; i++)
                {
                        pthread_join(tids1[i], &status);
                }

                printf("Gen %d: \n", (p+1));
                pthread_mutex_lock(&lock);
                printGrid(B);
                memcpy(A[0], B[0], numR*numC*sizeof(B[0][0]));
                pthread_mutex_unlock(&lock);
                p++;
        }

        clock_t stopTime = clock();
        double total = (double) (stopTime - startTime)/CLOCKS_PER_SEC;
        printf("Expected number of checks = %i ... Actual number of checks = %i \n", (numR*numC*gen), totalChecks);
        printf("Time taken for %i threads: %f\n", NUMT, total);

        pthread_exit(NULL);
        return 0;
}
