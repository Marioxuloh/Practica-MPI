#include <mpi.h>
#include <stdio.h>
#include <time.h>

#define TAG_PA 4
#define TAG_PG 3
#define TAG_PES 5
#define TAG_PI 6

double piWtime(void) {
    struct timeval tv;
    if (gettimeofday(&tv, 0) < 0) {
        perror("oops");
    }
    return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

void pi(int id) {
  
        MPI_Status status;
        int terminationSignal;
        double mpiCalls = 0;
        double startTime = piWtime();
        double startComputeTime, totalComputeTime = 0;
        // tiempo total, tiempo total de calculo, llamadas MPI
        double totalStadistics[3] = {0, 0, 0};

        //Esperamos a que nos diga PES que nuestro amor se termino
        MPI_Recv(&terminationSignal, 1, MPI_INT, 0, TAG_PI, MPI_COMM_WORLD, &status);
        mpiCalls++;
        

        double end = piWtime();
        double total_time = end - startTime;
   
        //estadistica del tiempo al PES
        totalStadistics[2]=mpiCalls+1;
        totalStadistics[1]=0;
        totalStadistics[0]=total_time;
        MPI_Send(totalStadistics, 3, MPI_DOUBLE, 0, TAG_PES, MPI_COMM_WORLD);


}

