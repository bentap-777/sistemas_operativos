#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

/* Variables globales */
int g_seconds;
pid_t g_pid_ejec, g_pid_a, g_pid_b;
pid_t g_pid_x, g_pid_y, g_pid_z;

volatile sig_atomic_t g_b_must_destroy = 0;
volatile sig_atomic_t g_leaf_must_die = 0;

/* ---------- Utilidades del sistema ---------- */

/* fork con comprobación de errores */
pid_t safe_fork(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera bloqueante de un hijo tolerante a interrupciones por señal */
void wait_for_child(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

/* Envía SIGUSR2 a una hoja y espera su terminación */
void kill_and_wait(pid_t pid)
{
    kill(pid, SIGUSR2);
    wait_for_child(pid);
}

/* Validación del argumento único de tiempo */
void parse_args(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <segundos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    g_seconds = atoi(argv[1]);
    if (g_seconds <= 0) {
        fprintf(stderr, "Error: Los segundos deben ser mayores que 0\n");
        exit(EXIT_FAILURE);
    }
}

/* ---------- Manejadores de señales ---------- */

/* ejec: Inicia la cascada de destrucción hacia A */
void handler_start_destruction(int sig)
{
    (void)sig;
    kill(g_pid_a, SIGUSR2);
}

/* A: Propaga la orden de destrucción hacia B */
void handler_A_destroy_and_propagate(int sig)
{
    (void)sig;
    kill(g_pid_b, SIGUSR2);
}

/* B: Habilita el apagado de sus hojas */
void handler_B_destroy_and_propagate(int sig)
{
    (void)sig;
    g_b_must_destroy = 1;
}

/* X, Y, Z: Permite salir del bucle de espera activa */
void handler_leaf_destroy(int sig)
{
    (void)sig;
    g_leaf_must_die = 1;
}

/* Z: Vence el temporizador y ordena la ejecución a A */
void handler_Z_alarm(int sig)
{
    (void)sig;
    kill(g_pid_a, SIGUSR1);
}

/* A: Ejecuta pstree mediante un proceso efímero y notifica a ejec */
void handler_A_exec_task(int sig)
{
    (void)sig;
    pid_t pid = safe_fork();
    if (pid == 0) {
        execlp("pstree", "pstree", (char *)NULL);
        perror("Error en exec");
        exit(EXIT_FAILURE);
    }
    wait_for_child(pid);
    kill(g_pid_ejec, SIGUSR2);
}

/* ---------- Procesos hoja (X, Y, Z) ---------- */

void print_leaf_identity(char name)
{
    printf("Soy el proceso %c: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n",
           name, getpid(), g_pid_b, g_pid_a, g_pid_ejec);
}

void run_leaf_process(char name)
{
    print_leaf_identity(name);
    signal(SIGUSR2, handler_leaf_destroy);

    if (name == 'Z') {
        signal(SIGALRM, handler_Z_alarm);
        alarm(g_seconds);
    }

    /* Espera activa permitida (sin sleep ni pause) */
    while (!g_leaf_must_die)
        ;

    printf("Soy %c (%d) y muero\n", name, getpid());
}

pid_t create_leaf_process(char name)
{
    pid_t pid = safe_fork();
    if (pid == 0) {
        run_leaf_process(name);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

/* ---------- Proceso B ---------- */

void run_process_B(void)
{
    g_pid_b = getpid();
    printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n",
           g_pid_b, g_pid_a, g_pid_ejec);

    signal(SIGUSR2, handler_B_destroy_and_propagate);

    g_pid_x = create_leaf_process('X');
    g_pid_y = create_leaf_process('Y');
    g_pid_z = create_leaf_process('Z');

    while (!g_b_must_destroy)
        ;

    /* Destrucción secuencial en orden estricto Z -> Y -> X */
    kill_and_wait(g_pid_z);
    kill_and_wait(g_pid_y);
    kill_and_wait(g_pid_x);

    printf("Soy B (%d) y muero\n", g_pid_b);
}

void create_B_process(void)
{
    g_pid_b = safe_fork();
    if (g_pid_b == 0) {
        run_process_B();
        exit(EXIT_SUCCESS);
    }
    wait_for_child(g_pid_b);
}

/* ---------- Proceso A ---------- */

void run_process_A(void)
{
    g_pid_a = getpid();
    printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n",
           g_pid_a, g_pid_ejec);

    signal(SIGUSR1, handler_A_exec_task);
    signal(SIGUSR2, handler_A_destroy_and_propagate);

    create_B_process();
    printf("Soy A (%d) y muero\n", g_pid_a);
}

void create_A_process(void)
{
    g_pid_a = safe_fork();
    if (g_pid_a == 0) {
        run_process_A();
        exit(EXIT_SUCCESS);
    }
    wait_for_child(g_pid_a);
}

/* ---------- Proceso Raíz (ejec) ---------- */

int main(int argc, char *argv[])
{
    parse_args(argc, argv);
    setvbuf(stdout, NULL, _IONBF, 0);

    g_pid_ejec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", g_pid_ejec);

    signal(SIGUSR2, handler_start_destruction);

    create_A_process();

    printf("Soy ejec (%d) y muero\n", g_pid_ejec);
    return EXIT_SUCCESS;
}