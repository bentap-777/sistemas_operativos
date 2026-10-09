#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

/* x e y definen el tamaño del árbol; la tabla compartida guarda los PID. */
int x, y;
int shmid;
int *tabla_pids = NULL; /* Primero van los PID de la cadena y después los de las hojas. */

/* Crea un hijo y termina con un mensaje claro si fork falla. */
pid_t safe_fork(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera al hijo indicado y reintenta si una señal interrumpe la espera. */
void esperar_hijo(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

/* Lee las cantidades de procesos y comprueba que ambas sean positivas. */
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

/* Imprime los PID de la tabla separados por comas. */
void imprimir_lista_pids(const int *pids, int cantidad)
{
    for (int i = 0; i < cantidad; i++) {
        if (i > 0)
            printf(", ");
        printf("%d", pids[i]);
    }
}

/* Reserva una tabla compartida para los PID de la cadena y de las hojas. */
void crear_memoria_compartida(void)
{
    size_t tamano = (x + y) * sizeof(int);

    shmid = shmget(IPC_PRIVATE, tamano, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    tabla_pids = (int *)shmat(shmid, NULL, 0);
    if (tabla_pids == (int *)-1) {
        perror("Error al vincular memoria compartida");
        exit(EXIT_FAILURE);
    }
}

/* Desvincula la tabla y marca el segmento compartido para eliminarlo. */
void liberar_memoria_compartida(void)
{
    shmdt(tabla_pids);
    shmctl(shmid, IPC_RMID, NULL);
}

/* Crea las hojas una a una y espera a cada hijo antes de volver. */
void crear_hojas_recursivo(int indice)
{
    if (indice == y)
        return;

    pid_t pid = safe_fork();

    if (pid == 0) {
        /* Cada hoja guarda su PID después de los PID de la cadena. */
        tabla_pids[x + indice] = getpid();

        printf("Soy el subhijo %d, mi padres son: ", getpid());
        imprimir_lista_pids(tabla_pids, x);
        printf("\n");

        shmdt(tabla_pids);
        exit(EXIT_SUCCESS);
    }

    crear_hojas_recursivo(indice + 1);
    esperar_hijo(pid);
}

/* Crea la cadena vertical; el último proceso será el padre de todas las hojas. */
void crear_cadena_recursivo(int nivel)
{
    pid_t pid = safe_fork();

    if (pid == 0) {
        /* La posición corresponde al nivel que ocupa este proceso en la cadena. */
        tabla_pids[nivel] = getpid();

        /* Al final de la cadena crea las hojas; los demás niveles crean el siguiente. */
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

    /* Empieza la cadena desde su primer nivel. */
    crear_cadena_recursivo(0);

    /* Cuando termina la cadena, el superpadre muestra los PID de las hojas. */
    printf("Soy el superpadre (%d) : mis hijos finales son: ", getpid());
    imprimir_lista_pids(&tabla_pids[x], y);
    printf("\n");

    liberar_memoria_compartida();
    return EXIT_SUCCESS;
}