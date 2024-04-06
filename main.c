#include <stdio.h>
#include <mpi.h>
#include "types/pes.c"
#include "types/pg.c"
#include "types/pa.c"
#include "types/pi.c"

#define TAG_COORDINATION 1

#define TAM 100
#define MIN 0
#define MAX 999

int main(int argc, char ** argv){

        //type:  0=pg  1=pa  2=pi
        int npg,npa;
        int id, type;
        MPI_Status status;

        MPI_Init(&argc, &argv);

        MPI_Comm_rank(MPI_COMM_WORLD, &id); 
        
        //Leemos de la terminal los parametros para los pg y los pa
        if (argc >= 3){
            npg = atoi(argv[1]);
            npa = atoi(argv[2]);
        }
        else{
            MPI_Finalize();
            return 0;
        }

        if(id == 0){
                //llamamos a pes.c, este proceso es el encargado de coordinar
                pes(id, MIN, MAX, npg, npa, TAM);
        }else{
                //hacemos recv bloqueante para obtener el tipo de proceso
                //int MPI_Recv(void* mensaje, int contador, MPI_Datatype tipo_datos, int origen, int etiqueta, MPI_Comm com, MPI_Status* status);
                MPI_Recv(&type, 1, MPI_INT, 0, TAG_COORDINATION, MPI_COMM_WORLD, &status);
                //fprintf(stderr, "recv type: %d with id: %d\n", type, id);
                switch (type)
                {
                    case 0:
                        //llamamos a pg.c
                        //
                        pg(id);
                        
                        break;
                    case 1:
                        //llamamos a pa.c
                        pa(id, MIN, MAX);
                        break;
                    case 2:
                        //llamamos a pi.c
                        pi(id);
                        break;
                    case 3:
                        //salimos
                        break;
                    default:
                        //no deberia venir ninguno por aqui pero
                        //se pone por precaucion y sacar por pantalla un mensaje
                        printf("Not known type assigned from id=%d\n", id);
                }
        }

        MPI_Finalize();

        return 0;

}