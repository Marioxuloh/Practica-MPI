#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <mpi.h>
#include <time.h>

#define TAG_PA 4
#define TAG_PG 3
#define TAG_PES 5

#define PESO_MEDIO      100000 

double paWtime(void) {
    struct timeval tv;
    if(gettimeofday(&tv, 0) < 0) {
        perror("oops");
    }
    return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

void fuerza_espera_pa(unsigned long peso){
        for (unsigned long i=1; i<1*peso; i++) sqrt(i);
}

void pa(int id, int min, int max){

        int msg;
        MPI_Status status;
        bool free = true; //Variable boolean controla si el pa esta ocupado o libre
        int id_pg;
        int guess;
        int bussyStatus = 0;
        int freeStatus = 1;
        int low;
        int high;
        double totalStadisticsPa[3] = {0,0,0}; //1-> tiempo total, 2-> tiempo total de calculo, 3-> llamadas MPI
        double mpiCalls = 0;
        double startTime = paWtime();
        double startComputeTime, totalComputeTime = 0;



        while(1){
                //recv un int de pg, si ese char es "="/-2,"<"/-3 o ">"/-4
                //si es "-1"/-1 para que termine y si es "id" un pg q pregunta si esta libre o no
                MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, TAG_PA, MPI_COMM_WORLD, &status);
                startComputeTime = paWtime();     //Iniciamos el tiempo de calculo
                mpiCalls++;     //Incremento el contador de llamadas mpi

                if(msg == -1){   //Codigo para terminar
                        break;
                } else {  
                        //Comunicacion con PG
                        //Si Pa esta libre mandamos un mensaje de vuelta a PG para establecer la comunicacion
                        if (free == true && msg > 0) {
                                id_pg = msg; 
                                free = false;
                                low = min;
                                high = max;
                                //le mando a pg que estoy libre
                                MPI_Send(&freeStatus, 1, MPI_INT, id_pg, TAG_PG, MPI_COMM_WORLD);
                                mpiCalls++;
                                guess = (low + high) / 2;
                                MPI_Send(&guess, 1, MPI_INT, id_pg, TAG_PG, MPI_COMM_WORLD); 
                                mpiCalls++;

                        } else if(free == false && msg > 0) { //PA esta ocupado
                                //Envio a PG que PA esta ocupado
                                MPI_Send(&bussyStatus, 1, MPI_INT, msg, TAG_PG, MPI_COMM_WORLD);
                                mpiCalls++;
                        }else {
                                //Iniciamos el proceso de adivinacion

                                //Busqueda binaria del elemento
                                fuerza_espera_pa(PESO_MEDIO);
                                if(msg == -2){  //(=) Hemos adivinado el numero
                                        free = true;
                                        totalComputeTime += paWtime() - startComputeTime;
                                }
                                else if (msg == -3){    //(<) Ajustamos el valor mas alto como la mitad -1
                                        high = guess - 1;
                                        guess = (low + high) / 2;
                                        MPI_Send(&guess, 1, MPI_INT, id_pg, TAG_PG, MPI_COMM_WORLD);
                                        mpiCalls++;
                                        totalComputeTime += paWtime() - startComputeTime;
                                }
                                else if (msg == -4){    //(>) Ajustamos el valor minimo como la mitad +1
                                        low = guess + 1;
                                        guess = (low + high) / 2;
                                        MPI_Send(&guess, 1, MPI_INT, id_pg, TAG_PG, MPI_COMM_WORLD);
                                        mpiCalls++;
                                        totalComputeTime += paWtime() - startComputeTime;
                                }
                        }
       
                }
        }

        // Calcula estadisticas finales
        mpiCalls++;
        totalStadisticsPa[0] = (paWtime() - startTime); // Tiempo total
        totalStadisticsPa[1] = totalComputeTime; // Tiempo de calculo
        totalStadisticsPa[2] = mpiCalls; // Llamadas MPI

        // Envia estadisticas a PES
        MPI_Send(totalStadisticsPa, 3, MPI_DOUBLE, 0, TAG_PES, MPI_COMM_WORLD);
        
}