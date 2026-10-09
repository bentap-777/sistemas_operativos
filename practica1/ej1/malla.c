#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define ESPERA 20  /* Tiempo que las hojas permanecen vivas para poder ver el arbol. */

/* ---------- Validación y espera de procesos ---------- */

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

/* Espera al hijo indicado; si una señal interrumpe la espera, vuelve a intentarlo. */
void wait_for_child(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

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

/* ---------- Construcción de las filas y columnas ---------- */

/* Cada llamada baja una fila. Al llegar a la última, el proceso espera un rato
    para que se pueda consultar el árbol con pstree. */
void run_row(int rows_below)
{
    pid_t pid;

    if (rows_below == 0) {
        sleep(ESPERA);
        return;
    }
    pid = safe_fork();
    if (pid == 0) {
        run_row(rows_below - 1);
        exit(EXIT_SUCCESS);
    }
    wait_for_child(pid);
}

/* Crea una columna por hijo y termina de lanzarlas todas antes de esperarlas.
    Cada columna tiene la cantidad de filas indicada. */
void create_columns(int cols, int rows)
{
    pid_t pid;

    if (cols == 0)
        return;
    pid = safe_fork();
    if (pid == 0) {
        run_row(rows - 1);
        exit(EXIT_SUCCESS);
    }
    create_columns(cols - 1, rows);
    wait_for_child(pid);
}

/* ---------- Inicio del proceso malla ---------- */

int main(int argc, char *argv[])
{
    int rows, cols;

    parse_args(argc, argv, &rows, &cols);
    create_columns(cols, rows);
    return 0;
}
