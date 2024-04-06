#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define TAG_COORDINATION 1
#define TAG_PGSETUP 2
#define TAG_PG 3
#define TAG_PA 4
#define TAG_PES 5
#define TAG_PI 6

#define N_TYPE 3
#define N_WISH 4

//Funciones para el volcado de datos a fichero
void write_final_stats(double *, int, FILE *);
void write_parcial_stats(double *, FILE *);

//Funcion para obtener la fecha de creacion del fichero de las estadisticas
char* get_current_time() {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        char buffer[100];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", tm);

        return strdup(buffer);
}

double pesWtime(void) {
    struct timeval tv;
    if(gettimeofday(&tv, 0) < 0) {
        perror("oops");
    }
    return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

void pes(int id, int min, int max, int npg, int npa, int tam){   

        //type:  0=pg  1=pa  2=pi
        int nprocs, type, aux;
        int *wish_list = (int *)malloc(tam * sizeof(int));
        double wishNumStadistics [6];
        double wishFinStadisticsPG [3];
        double wishFinStadisticsPA [3];
        double wishFinStadisticsPI [3];

        //variables de mario
        double tiempoTotalCalculo = 0;
        double llamadasMPI = 0;

        int *pg_list = (int *)malloc(npg * sizeof(int));
        int *pa_list = (int *)malloc(npa * sizeof(int));

        int i;
        MPI_Status status;
        double startTime = pesWtime();

        if (pg_list == NULL || pa_list == NULL) {
                fprintf(stderr, "Error: couldn't get memory\n");
                return;
        }

        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        int *pi_list = (int *)malloc((nprocs-(npa+npg)-1) * sizeof(int));

        //control de errores
        if(nprocs < (npg + npa + 1) || npg == 0 || npa == 0){
                printf("Bad arguments try again.\n");
                for(int i = 1; i < nprocs; i++){
                        type = 3;//salen por case 3 en el switch
                        MPI_Send(&type, 1, MPI_INT, i, TAG_COORDINATION, MPI_COMM_WORLD);
                }
                return;
        }

        //asignacion de tipos
        aux = 0;
        type = 0;
        for(int i = 1; i < nprocs; i++){
                if( type == 0 && aux == npg){
                        type = 1;
                        aux = 0; 
                }
                if( type == 1 && aux == npa){
                        type = 2;
                        aux = 0;
                }
                if(type == 0){
                        pg_list[aux] = i;
                }else if(type == 1){
                        pa_list[aux] = i;
                }else{
                        pi_list[aux] = i;
                }
                MPI_Send(&type, 1, MPI_INT, i, TAG_COORDINATION, MPI_COMM_WORLD);
                aux++;
        }

        // Obtener la fecha y hora actual
        char* timestamp = get_current_time();

        // Formar el nombre del archivo
        char filename[256];
        snprintf(filename, sizeof(filename), "types/stats/%s.txt", timestamp);
        // Abrir el archivo en modo escritura
        FILE *fp = fopen(filename, "a");
        if (fp == NULL) {
        return;
        }

                /*##### MOTRAR TABLA DE INFO ######*/
        fprintf(stdout, "**********************************\n");
        fprintf(stdout, "   NUMERO DE PROCESOS    : %d\n", nprocs);
        fprintf(stdout, "   PROCESOS GESTORES     : %d\n", npg);
        fprintf(stdout, "   PROCESOS ADIVINADORES : %d\n", npa);
        fprintf(stdout, "   NUMERO DE TOTAL(+E/S) : %d\n", npg+npa+1);
        fprintf(stdout, "**********************************\n");
        fprintf(stdout, "Guardando resultados parciales y finales en --> types/stats/%s.txt\n", timestamp);
                /*##### MOTRAR TABLA DE INFO ######*/
        fprintf(fp, "**********************************\n");
        fprintf(fp, "   NUMERO DE PROCESOS    : %d\n", nprocs);
        fprintf(fp, "   PROCESOS GESTORES     : %d\n", npg);
        fprintf(fp, "   PROCESOS ADIVINADORES : %d\n", npa);
        fprintf(fp, "   NUMERO DE TOTAL(+E/S) : %d\n", npg+npa+1);
        fprintf(fp, "**********************************\n");

        //Le mandamos a todos los PG el numero de PAs y el setup correspondiente
        for(i = 0; i < npg; i++){
                //numero de pa
                MPI_Send(&npa, 1, MPI_INT, pg_list[i], TAG_PGSETUP, MPI_COMM_WORLD);
                //lista con los ids de todos los pa
                MPI_Send(pa_list, npa, MPI_INT, pg_list[i], TAG_PGSETUP, MPI_COMM_WORLD);
        }

        //Creo la lista con elementos randoms
        srand(time(NULL));
        for(i=0; i<tam; i++){
                wish_list[i] = rand()%(max+1); //relleno el vector 
        }


        //aca envio un numero a los que haya
        int index;
        for(i=0; i<npg; i++){ 
                index=(i % npg);
                //fprintf(stdout,"Envio el num %d al PG %d\n", wish_list[i], pg_list[index]);
                if(i > (tam-1)){
                        break;
                }
                MPI_Send(&wish_list[i], 1, MPI_INT, pg_list[index], TAG_PG, MPI_COMM_WORLD);
        }
        fprintf(fp, "\n*********************************************************************\n");
        fprintf(fp, "RESULTADOS PARCIALES\n");
        fprintf(fp, "*********************************************************************\n");
        int cont = 0;
        int k = i;
        while(1){
                // Esperar a que algun PG termine y recibir el resultado
                MPI_Recv(wishNumStadistics, 6, MPI_DOUBLE, MPI_ANY_SOURCE, TAG_PES, MPI_COMM_WORLD, &status);
                cont++; //para salir del bucle cuando ya muestro todos los parciales
                // Mostrar resultados parciales
                
                write_parcial_stats(wishNumStadistics, fp);
                
                if(cont == tam){
                        break; //salgo del bulce infinito
                }

                if(k<tam){
                        //fprintf(stdout, "envio el num %d al PG: %d\n", wish_list[k],  wishNumStadistics[5] );
                        MPI_Send(&wish_list[k], 1, MPI_INT, wishNumStadistics[5], TAG_PG, MPI_COMM_WORLD); 
                        k++;
                }           
        }
        fprintf(fp, "\n*********************************************************************\n");
        fprintf(fp, "RESULTADOS FINALES\n");
        fprintf(fp, "*********************************************************************\n");
        //envio un -1
        int fin = -1;
        for(i=0; i<npg; i++){
                MPI_Send(&fin, 1, MPI_INT, pg_list[i], TAG_PG, MPI_COMM_WORLD);
        }
        fprintf(stdout,"\n");
        for(i=0; i<npg; i++){

                MPI_Recv(wishFinStadisticsPG, 3, MPI_DOUBLE, pg_list[i], TAG_PES, MPI_COMM_WORLD, &status);
                tiempoTotalCalculo += wishFinStadisticsPG[1];
                llamadasMPI += wishFinStadisticsPG[2];
                write_final_stats(wishFinStadisticsPG, pg_list[i], fp);
        }


        for(i=0; i<npa; i++){
                MPI_Send(&fin, 1, MPI_INT, pa_list[i], TAG_PA, MPI_COMM_WORLD);
        } 
        fprintf(stdout,"\n");
        for(int j=0; j<npa; j++){

                MPI_Recv(wishFinStadisticsPA, 3, MPI_DOUBLE, pa_list[j], TAG_PES, MPI_COMM_WORLD, &status);
                llamadasMPI += wishFinStadisticsPA[2];
                tiempoTotalCalculo += wishFinStadisticsPA[1];
                write_final_stats(wishFinStadisticsPA, pa_list[j], fp);
        }
        

        for(i=0; i<(nprocs-(npa+npg)-1); i++){
                MPI_Send(&fin, 1, MPI_INT, pi_list[i], TAG_PI, MPI_COMM_WORLD);
        } 
        fprintf(stdout,"\n");
        for(int j=0; j<(nprocs-(npa+npg)-1); j++){
                MPI_Recv(wishFinStadisticsPI, 3, MPI_DOUBLE, pi_list[j], TAG_PES, MPI_COMM_WORLD, &status);
                tiempoTotalCalculo += wishFinStadisticsPI[1];
                llamadasMPI += wishFinStadisticsPI[2];
                write_final_stats(wishFinStadisticsPI, pi_list[j], fp);
        }        
        fprintf(fp, "*********************************************************************\n");


        //Mostrar tiempo total de la prueba
        double Ttotal = (pesWtime() - startTime); // Tiempo total
        /*##### MOTRAR TABLA DE INFO ######*/
        fprintf(stdout,"**********************************\n");
        fprintf(stdout,"   NUMERO DE PROCESOS    : %d\n", nprocs);
        fprintf(stdout,"   PROCESOS GESTORES     : %d\n", npg);
        fprintf(stdout,"   PROCESOS ADIVINADORES : %d\n", npa);
        fprintf(stdout,"   NUMERO DE TOTAL(+E/S) : %d\n", npg+npa+1);
        fprintf(stdout,"   TAM WISHLIST          : %d\n", tam);
        fprintf(stdout,"   MIN&MAX               : %d-%d\n", min, max);
        fprintf(stdout,"   TIEMPO TOTAL PRUEBA   : %f\n", Ttotal);
        fprintf(stdout,"   TIEMPO TOTAL CALCULO  : %f\n", tiempoTotalCalculo);
        fprintf(stdout,"   TOTAL LLAMADAS MPI    : %f\n", llamadasMPI);
        fprintf(stdout,"**********************************\n");
        fprintf(fp,"**********************************\n");
        fprintf(fp,"   NUMERO DE PROCESOS    : %d\n", nprocs);
        fprintf(fp,"   PROCESOS GESTORES     : %d\n", npg);
        fprintf(fp,"   PROCESOS ADIVINADORES : %d\n", npa);
        fprintf(fp,"   NUMERO DE TOTAL(+E/S) : %d\n", npg+npa+1);
        fprintf(fp,"   TAM WISHLIST          : %d\n", tam);
        fprintf(fp,"   MIN&MAX               : %d-%d\n", min, max);
        fprintf(fp,"   TIEMPO TOTAL PRUEBA   : %f\n", Ttotal);
        fprintf(fp,"   TIEMPO TOTAL CALCULO  : %f\n", tiempoTotalCalculo);
        fprintf(fp,"   TOTAL LLAMADAS MPI    : %f\n", llamadasMPI);
        fprintf(fp,"**********************************\n");

        // Finalizar
        //fprintf(stdout, " Finalizando perrito malvado..\n");
        fclose(fp);
        free(wish_list);
        free(pg_list);
        free(pa_list);
        free(pi_list);
   
}


//Funciones para el volcado de datos a fichero

void write_parcial_stats(double *stadistics, FILE *fp){

        // Pa libre/ocupado, preguntas desde Pa, tiempo total, tiempo de calculo(entre una pregunta de pa y su respuesta), id
        //double wishNumStadistics[5] = {0, 0, 0, 0, 0, id};
        // Escribir las estadisticas en el archivo
        fprintf(fp, "--------------\n");
        fprintf(fp, "PG_id: %.f\n", stadistics[5]);
        fprintf(fp, "F/O: %.f\n", stadistics[0]);
        fprintf(fp, "Q/A: %.f\n", stadistics[1]);
        fprintf(fp, "TT: %f\n", stadistics[2]);
        fprintf(fp, "CT: %f\n", stadistics[3]);
        fprintf(fp, "NA: %.f\n", stadistics[4]);
        fprintf(fp, "--------------\n");

        return;
}

void write_final_stats(double *stadistics, int id, FILE *fp){

        // tiempo total, tiempo total de calculo, llamadas MPI
        //double totalStadistics[3] = {0, 0, 0};
        // Escribir las estadísticas en el archivo
        fprintf(fp, "--------------\n");
        fprintf(fp, "ID: %d\n", id);
        fprintf(fp, "TT: %f\n", stadistics[0]);
        fprintf(fp, "CT: %f\n", stadistics[1]);
        fprintf(fp, "CL: %f\n", stadistics[2]);
        fprintf(fp, "--------------\n");

        return;
}


