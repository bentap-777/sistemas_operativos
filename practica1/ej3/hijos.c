/* EJERCICIO 3 */
#include <stdio.h> //el de siempre
#include <stdlib.h> //atoi, exit
#include <unistd.h> //fork(), getpid()
#include <errno.h> //controlar interrupciones por señales
#include <sys/types.h> //tipos de datos -> pid_t, size_t
#include <sys/ipc.h> //constante -> IPC_PRIVATE
#include <sys/shm.h> //gestionar memoria compartida -> shmget, shmat, shmdt, shmctl
#include <sys/wait.h> //waitpid()

/* Variables globales: x e y definen el arbol; tabla_pids apunta a la memoria compartida */
int x, y;
int shmid;
int *tabla_pids = NULL; /* Primeros 'x' son los padres; siguientes 'y' son las hojas */

/* Envoltorio de fork para controlar errores del sistema */
pid_t safe_fork()
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera a que termine un hijo reintentando si llega una senal */
void esperar_hijo(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

/* Validacion de los dos argumentos numericos */
void parse_args(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <x> <y>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    x = atoi(argv[1]);
    y = atoi(argv[2]);
    if (x < 1 || y < 1) {
        fprintf(stderr, "Error: x e y deben ser mayores que 0\n");
        exit(EXIT_FAILURE);
    }
}

/* Muestra un vector de PIDs separados por comas */
void imprimir_lista_pids(const int *pids, int cantidad)
{
    for (int i = 0; i < cantidad; i++) {
        if (i > 0)
            printf(", ");
        printf("%d", pids[i]);
    }
}

/* Reserva la zona de memoria compartida para almacenar (x + y) PIDs */
void crear_memoria_compartida()
{
    size_t tamano = (x + y) * sizeof(int);

    /* Usamos IPC_PRIVATE porque todos los procesos son de la misma familia (fork) */
    shmid = shmget(IPC_PRIVATE, tamano, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    /* Vinculamos el segmento a nuestro espacio de direcciones */
    tabla_pids = (int *)shmat(shmid, NULL, 0);
    if (tabla_pids == (int *)-1) {
        perror("Error al vincular memoria compartida");
        exit(EXIT_FAILURE);
    }
}

/* Desvincula el segmento y lo borra del sistema operativo */
void liberar_memoria_compartida()
{
    shmdt(tabla_pids);
    shmctl(shmid, IPC_RMID, NULL); /* Clave para no dejar memoria colgada en ipcs -m */
}

/* Crea de forma recursiva los 'y' procesos hoja (subhijos) */
void crear_hojas_recursivo(int indice)
{
    if (indice == y)
        return;

    pid_t pid = safe_fork();

    if (pid == 0) {
        /* Cada hoja guarda su propio PID en la segunda seccion del array */
        tabla_pids[x + indice] = getpid();

        /* Imprime su mensaje leyendo los PIDs de sus padres en la primera seccion */
        printf("Soy el subhijo %d, mi padres son: ", getpid());
        imprimir_lista_pids(tabla_pids, x);
        printf("\n");

        shmdt(tabla_pids);
        exit(EXIT_SUCCESS);
    }

    crear_hojas_recursivo(indice + 1);
    esperar_hijo(pid);
}

/* Crea de forma recursiva la cadena vertical de 'x' procesos */
void crear_cadena_recursivo(int nivel)
{
    pid_t pid = safe_fork();

    if (pid == 0) {
        /* Guarda su PID en la posicion correspondiente a su nivel */
        tabla_pids[nivel] = getpid();

        /* Si es el ultimo de la cadena, da el salto y crea a las hojas; si no, baja otro nivel */
        if (nivel == x - 1) {
            crear_hojas_recursivo(0);
        } else {
            crear_cadena_recursivo(nivel + 1);
        }

        shmdt(tabla_pids);
        exit(EXIT_SUCCESS);
    }

    esperar_hijo(pid);
}

int main(int argc, char *argv[])
{
    parse_args(argc, argv);
    crear_memoria_compartida();

    /* Arranca la creacion vertical desde el nivel 0 */
    crear_cadena_recursivo(0);

    /* El superpadre solo llega aqui cuando todos sus descendientes han terminado */
    printf("Soy el superpadre (%d) : mis hijos finales son: ", getpid());
    imprimir_lista_pids(&tabla_pids[x], y);
    printf("\n");

    liberar_memoria_compartida();
    return EXIT_SUCCESS;
}