#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

/* Variables globales con datos compartidos */
int x, y;
int shmid;
int *tabla_pids = NULL; /* Los primeros 'x' son la cadena, los siguientes 'y' son las hojas */

/* Control de fork con salida en caso de error */
pid_t safe_fork(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera a un hijo reintentando si llega una señal */
void esperar_hijo(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

/* Valida argumentos */
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

/* Crea la zona de memoria compartida para almacenar (x + y) PIDs */
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

/* Libera y destruye el segmento de memoria compartida */
void liberar_memoria_compartida(void)
{
    shmdt(tabla_pids);
    shmctl(shmid, IPC_RMID, NULL);
}

/* Crea de forma recursiva los 'y' procesos hoja */
void crear_hojas_recursivo(int indice)
{
    if (indice == y)
        return;

    pid_t pid = safe_fork();

    if (pid == 0) {
        /* Proceso hoja: guarda su PID en la segunda seccion del array */
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

/* Crea de forma recursiva los 'x' procesos de la cadena vertical */
void crear_cadena_recursivo(int nivel)
{
    pid_t pid = safe_fork();

    if (pid == 0) {
        /* Guarda su PID en la primera seccion del array */
        tabla_pids[nivel] = getpid();

        /* Si es el ultimo de la cadena, crea las hojas; si no, sigue bajando */
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

    /* El superpadre imprime cuando todos los hijos y hojas han terminado */
    printf("Soy el superpadre (%d) : mis hijos finales son: ", getpid());
    imprimir_lista_pids(&tabla_pids[x], y);
    printf("\n");

    liberar_memoria_compartida();
    return EXIT_SUCCESS;
}