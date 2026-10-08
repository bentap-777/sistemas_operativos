#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define ESPERA 20  /* segundos que viven los procesos de la ultima fila */

/* ---------- Utilidades ---------- */

/* fork con comprobacion de error */
pid_t safe_fork(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera a un hijo concreto (y reintenta si una señal la interrumpe) */
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

/* ---------- Construccion del arbol ---------- */

/* El proceso actual cuelga 'rows_below' procesos mas por debajo, en vertical.
   Si ya no quedan filas, es la ultima fila: se queda esperando para que
   nos de tiempo a hacer pstree. */
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

/* Crea 'cols' columnas, una detras de otra, cada una de 'rows' procesos.
   Todas las columnas se crean antes de esperar a ninguna. */
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

/* ---------- main (proceso malla) ---------- */

int main(int argc, char *argv[])
{
    int rows, cols;

    parse_args(argc, argv, &rows, &cols);
    create_columns(cols, rows);
    return 0;
}
