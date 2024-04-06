#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <mpi.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define TAG_PGSETUP 2
#define TAG_PG 3
#define TAG_PA 4
#define TAG_PES 5

#define PESO_MIM        10000000  // >,=,< 

double pgWtime(void) {
    struct timeval tv;
    if(gettimeofday(&tv, 0) < 0) {
        perror("oops");
    }
    return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

void fuerza_espera_pg(unsigned long peso){
        for (unsigned long i=1; i<1*peso; i++) sqrt(i);
}

void pg(int id){

        MPI_Status status;
        int npa;
        int wishNum;
        // Pa libre/ocupado, preguntas desde Pa, tiempo total, tiempo de calculo(entre una pregunta de pa y su respuesta), n adivinado,id
        double wishNumStadistics[6] = {0, 0, 0, 0, 0, id};
        // tiempo total, tiempo total de calculo, llamadas MPI
        double totalStadistics[3] = {0, 0, 0};
        int * pa_list;
        int answer = 0;
        int idPaAssigned = -1;
        int wishNumPa = -1;
        bool wishNumFound = false;
        //send int de pa, si ese char es "="/-2,"<"/-3 o ">"/-4
        int response[3] = {-2, -3, -4};
        double initial_time = pgWtime();
        double initial_parcial_time = 0;
        double total_calc_time = 0;
        double parcial_calc_time = 0;
        double initial_time_pa = 0;
        double mpi_calls = 0;

        MPI_Recv(&npa, 1, MPI_INT, 0, TAG_PGSETUP, MPI_COMM_WORLD, &status);
        mpi_calls++;

        pa_list = (int *)malloc(npa * sizeof(int));
        if (pa_list == NULL) {
                fprintf(stderr, "Error: couldn't get memory\n");
                return;
        }
        MPI_Recv(pa_list, npa, MPI_INT, 0, TAG_PGSETUP, MPI_COMM_WORLD, &status);
        mpi_calls++;

        while(1){
                MPI_Recv(&wishNum, 1, MPI_INT, 0, TAG_PG, MPI_COMM_WORLD, &status);
                initial_parcial_time = pgWtime();
                mpi_calls++;
                wishNumFound = false;
                if(wishNum != -1){
                        while(answer != 1){
                                for(int i = id%npa; i < npa; i++){
                                        MPI_Send(&id, 1, MPI_INT, pa_list[i], TAG_PA, MPI_COMM_WORLD);
                                        mpi_calls++;
                                        MPI_Recv(&answer, 1, MPI_INT, pa_list[i], TAG_PG, MPI_COMM_WORLD, &status);
                                        mpi_calls++;
                                        wishNumStadistics[0] ++;
                                        if(answer == 1){
                                                idPaAssigned = pa_list[i];
                                                break;
                                        }
                                }
                        }
                        while(!wishNumFound){
                                MPI_Recv(&wishNumPa, 1, MPI_INT, idPaAssigned, TAG_PG, MPI_COMM_WORLD, &status);
                                initial_time_pa = pgWtime();
                                mpi_calls++;
                                wishNumStadistics[1]++;
                                fuerza_espera_pg(PESO_MIM);
                                if(wishNumPa < wishNum){
                                        MPI_Send(&response[2], 1, MPI_INT, idPaAssigned, TAG_PA, MPI_COMM_WORLD);
                                        parcial_calc_time += (pgWtime() - initial_time_pa);
                                        mpi_calls++;
                                }else if(wishNumPa > wishNum){
                                        MPI_Send(&response[1], 1, MPI_INT, idPaAssigned, TAG_PA, MPI_COMM_WORLD);
                                        parcial_calc_time += (pgWtime() - initial_time_pa);
                                        mpi_calls++;
                                }else{
                                        MPI_Send(&response[0], 1, MPI_INT, idPaAssigned, TAG_PA, MPI_COMM_WORLD);
                                        mpi_calls++;
                                        parcial_calc_time += (pgWtime() - initial_time_pa);
                                        total_calc_time += parcial_calc_time;
                                        wishNumStadistics[2] = (pgWtime()-initial_parcial_time);
                                        wishNumStadistics[3] = (parcial_calc_time);
                                        wishNumStadistics[4] = (wishNum);
                                        MPI_Send(wishNumStadistics, 6, MPI_DOUBLE, 0, TAG_PES, MPI_COMM_WORLD);
                                        mpi_calls++;
                                        wishNumStadistics[0] = 0;
                                        wishNumStadistics[1] = 0;
                                        parcial_calc_time = 0;
                                        answer = 0;
                                        wishNumFound = true;
                                }
                        }
                }else{
                        break;
                }
        }
        
        totalStadistics[0] = (pgWtime()-initial_time);
        totalStadistics[1] = total_calc_time;
        totalStadistics[2] = mpi_calls + 1;
        MPI_Send(totalStadistics, 3, MPI_DOUBLE, 0, TAG_PES, MPI_COMM_WORLD);
        free(pa_list);
}