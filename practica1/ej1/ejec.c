/* EJERCICIO 1B */

#include <stdio.h>
#include <stdlib.h> //para el "atoi", "exit" y constantes
#include <unistd.h> //fork(), getpid(), alarm(), execlp()
#include <signal.h> // signal(), kill(), sig_atomit_t
#include <errno.h> //controlar si una señal corta un waitpid -> EINTR
#include <sys/wait.h> //waitpid()

/* Variables globales con el tiempo y los PIDs que los procesos necesitan conocer */
int g_seconds;
pid_t g_pid_ejec, g_pid_a, g_pid_b;
pid_t g_pid_x, g_pid_y, g_pid_z;

/* Banderas para las esperas activas de senales (sig_atomic_t evita problemas con el compilador) */
volatile sig_atomic_t g_b_must_destroy = 0;
volatile sig_atomic_t g_leaf_must_die = 0;

/* Clonamos un proceso comprobando que no haya fallado el sistema */
pid_t safe_fork()
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/* Espera a un hijo concreto reintentando si llega una senal antes de tiempo */
void wait_for_child(pid_t pid)
{
    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
        ;
}

/* Envia la orden de terminar a una hoja y espera a que el sistema confirme su muerte */
void kill_and_wait(pid_t pid)
{
    kill(pid, SIGUSR2);
    wait_for_child(pid);
}

/* Leemos los segundos que debe esperar el proceso Z */
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

/* ---------- Manejadores de senales ---------- */

/* ejec recibe la confirmacion de que pstree termino y manda la orden de cierre hacia A */
void handler_start_destruction(int sig)
{
    kill(g_pid_a, SIGUSR2);
}

/* A recibe la orden de destruccion y la propaga hacia B */
void handler_A_destroy_and_propagate(int sig)
{
    kill(g_pid_b, SIGUSR2);
}

/* B recibe la orden y activa su bandera para empezar a matar a sus hojas */
void handler_B_destroy_and_propagate(int sig)
{
    g_b_must_destroy = 1;
}

/* Las hojas X, Y y Z activan su bandera para salir del bucle de espera */
void handler_leaf_destroy(int sig)
{
    g_leaf_must_die = 1;
}

/* Cuando expira el tiempo de alarm, Z avisa al proceso A mediante SIGUSR1 */
void handler_Z_alarm(int sig)
{
    kill(g_pid_a, SIGUSR1);
}

/* A recibe SIGUSR1, crea un hijo para ejecutar pstree y avisa a ejec al terminar */
void handler_A_exec_task(int sig)
{
    pid_t pid = safe_fork();
    if (pid == 0) {
        /* Usamos execlp para ejecutar el comando del sistema */
        execlp("pstree", "pstree", NULL);
        perror("Error en exec");
        exit(EXIT_FAILURE);
    }
    wait_for_child(pid);
    /* Avisamos al proceso raiz ejec de que la tarea ha terminado */
    kill(g_pid_ejec, SIGUSR2);
}

/* ---------- Codigo de las hojas (X, Y, Z) ---------- */

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
        alarm(g_seconds); /* Programamos el reloj */
    }

    /* Espera activa: no usamos sleep ni pause porque esta prohibido */
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

void run_process_B()
{
    g_pid_b = getpid();
    printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n",
           g_pid_b, g_pid_a, g_pid_ejec);

    signal(SIGUSR2, handler_B_destroy_and_propagate);

    /* B crea a sus tres hijos */
    g_pid_x = create_leaf_process('X');
    g_pid_y = create_leaf_process('Y');
    g_pid_z = create_leaf_process('Z');

    /* Espera activa hasta que le llegue la orden de destruccion */
    while (!g_b_must_destroy)
        ;

    /* Orden estricto de muerte pedido en el PDF: primero Z, luego Y, luego X */
    kill_and_wait(g_pid_z);
    kill_and_wait(g_pid_y);
    kill_and_wait(g_pid_x);

    printf("Soy B (%d) y muero\n", g_pid_b);
}

void create_B_process()
{
    g_pid_b = safe_fork();
    if (g_pid_b == 0) {
        run_process_B();
        exit(EXIT_SUCCESS);
    }
    wait_for_child(g_pid_b);
}

/* ---------- Proceso A ---------- */

void run_process_A()
{
    g_pid_a = getpid();
    printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n",
           g_pid_a, g_pid_ejec);

    signal(SIGUSR1, handler_A_exec_task);
    signal(SIGUSR2, handler_A_destroy_and_propagate);

    create_B_process();
    printf("Soy A (%d) y muero\n", g_pid_a);
}

void create_A_process()
{
    g_pid_a = safe_fork();
    if (g_pid_a == 0) {
        run_process_A();
        exit(EXIT_SUCCESS);
    }
    wait_for_child(g_pid_a);
}

/* ---------- Proceso Raiz: ejec ---------- */

int main(int argc, char *argv[])
{
    parse_args(argc, argv);
    setvbuf(stdout, NULL, _IONBF, 0); /* Para que el texto salga directo sin retrasos */

    g_pid_ejec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", g_pid_ejec);

    signal(SIGUSR2, handler_start_destruction);

    create_A_process();

    printf("Soy ejec (%d) y muero\n", g_pid_ejec);
    return EXIT_SUCCESS;
}