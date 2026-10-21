/* EJERCICIO 1A */

#include <stdio.h> //para imprimir mensajes
#include <stdlib.h> //para convertir argumentos con el "Atoi" y salir "exit()"
#include <unistd.h> //para el "fork()" y el "sleep()"
#include <errno.h> //para las interrupciones de señal
#include <sys/wait.h> //para el "waitpid()"

#define ESPERA 20 /* Segundos que aguantan las hojas vivas para ver el arbol con pstree */

/* Funcion auxiliar para hacer fork y controlar si da error */
pid_t safe_fork()
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error al hacer fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera a que termine un hijo concreto. Si una senal interrumpe la espera, reintenta */
void wait_for_child(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

/* Comprobamos que el usuario nos pase los dos argumentos requeridos */
void parse_args(int argc, char *argv[], int *rows, int *cols)
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <filas> <columnas>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    *rows = atoi(argv[1]);
    *cols = atoi(argv[2]);
    if (*rows < 1 || *cols < 1) {
        fprintf(stderr, "Las filas y las columnas deben ser mayores que 0\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Baja recursivamente en vertical creando una fila por llamada.
 * Al llegar a la ultima fila (la hoja), duerme un tiempo para permitir el pstree.
 */
void run_row(int rows_below)
{
    pid_t pid;

    /* Caso base: si ya no quedan filas por debajo, somos la hoja */
    if (rows_below == 0) {
        sleep(ESPERA);
        return;
    }

    pid = safe_fork();
    if (pid == 0) {
        /* El hijo continua bajando en profundidad */
        run_row(rows_below - 1);
        exit(EXIT_SUCCESS);
    }

    /* El padre espera a que su hijo vertical termine antes de morir */
    wait_for_child(pid);
}

/*
 * Se expande recursivamente en horizontal creando el primer proceso de cada columna.
 * No usamos ningun bucle for ni while para cumplir las normas de la practica.
 */
void create_columns(int cols, int rows)
{
    pid_t pid;

    /* Caso base: cuando ya hemos creado todas las columnas requeridas */
    if (cols == 0)
        return;

    pid = safe_fork();
    if (pid == 0) {
        /* Cada cabeza de columna empieza a descender sus filas */
        run_row(rows - 1);
        exit(EXIT_SUCCESS);
    }

    /* El proceso raiz engendra la siguiente columna antes de esperar */
    create_columns(cols - 1, rows);
    wait_for_child(pid);
}

int main(int argc, char *argv[])
{
    int rows, cols;

    parse_args(argc, argv, &rows, &cols);
    create_columns(cols, rows);

    return 0;
}