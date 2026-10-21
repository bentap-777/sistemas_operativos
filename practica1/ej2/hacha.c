/* EJERCICIO 2 */

#include <stdlib.h> //atol(), exit(), constantes EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h> //read, write, close, pipe, fork, STDERR_FILENO
#include <fcntl.h> //open()
#include <string.h> //para el string
#include <sys/wait.h> //waitpid()
/* No podemos usar printf -> no usamos el "<stdio.h>" */

#define TAM_BUFFER 4096
#define MAX_TROZOS 100 /* Fragmentos de .h00 a .h99 */

/* Muestra un error por la salida de error estándar usando write() y sale del programa */
void lanzar_error(const char *mensaje)
{
    write(STDERR_FILENO, mensaje, strlen(mensaje));
    write(STDERR_FILENO, "\n", 1);
    exit(EXIT_FAILURE);
}

/* Comprueba que los dos argumentos pasados por consola sean correctos */
void parse_args(int argc, char *argv[], char **archivo, long *tam_trozo)
{
    if (argc != 3) {
        lanzar_error("Uso: ./hacha <archivo> <tamano>");
    }

    *archivo = argv[1];
    *tam_trozo = atol(argv[2]);

    if (*tam_trozo <= 0) {
        lanzar_error("El tamano debe ser mayor que 0");
    }
}

/* Construye la extension del fichero: .h00, .h01, etc. sin usar sprintf */
void construir_nombre(char *destino, const char *origen, int indice)
{
    int len;

    strcpy(destino, origen);
    strcat(destino, ".h");
    len = strlen(destino);

    /* Calculamos las dos cifras con division y resto */
    destino[len]     = '0' + (indice / 10);
    destino[len + 1] = '0' + (indice % 10);
    destino[len + 2] = '\0';
}

/* Codigo del proceso hijo: lee de la tuberia y vuelca los bytes en su archivo .hXX */
void proceso_hijo(int tub_lectura, const char *nombre_fragmento)
{
    char buffer[TAM_BUFFER];
    ssize_t leidos;
    int fd_destino;

    fd_destino = open(nombre_fragmento, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd_destino == -1) {
        lanzar_error("Error al crear el archivo fragmento");
    }

    /* Leemos de la tuberia hasta que el padre cierra el extremo y read devuelve 0 (EOF) */
    while ((leidos = read(tub_lectura, buffer, sizeof(buffer))) > 0) {
        if (write(fd_destino, buffer, leidos) == -1) {
            lanzar_error("Error al escribir en el fragmento");
        }
    }

    if (leidos == -1) {
        lanzar_error("Error al leer de la tuberia");
    }

    close(fd_destino);
    close(tub_lectura);
}

/* Codigo del proceso padre: lee del fichero original y escribe en la tuberia */
void transferir_a_hijo(int fd_origen, int tub_escritura, long tam_trozo)
{
    char buffer[TAM_BUFFER];
    long restantes = tam_trozo;
    long a_leer;
    ssize_t leidos;

    while (restantes > 0) {
        /* Leemos en bloques de como maximo el tamano del buffer */
        a_leer = (restantes < TAM_BUFFER) ? restantes : TAM_BUFFER;

        leidos = read(fd_origen, buffer, a_leer);
        if (leidos == -1) {
            lanzar_error("Error al leer archivo original");
        }
        if (leidos == 0) {
            break; /* Ya hemos llegado al final del archivo original */
        }

        if (write(tub_escritura, buffer, leidos) == -1) {
            lanzar_error("Error al volcar datos en la tuberia");
        }

        restantes -= leidos;
    }
}

/* Crea la tuberia y el hijo para un solo fragmento */
void crear_fragmento(int fd_origen, const char *archivo, int indice, long tam_trozo)
{
    int tub[2];
    pid_t pid;
    char nombre_fragmento[512];

    construir_nombre(nombre_fragmento, archivo, indice);

    if (pipe(tub) == -1) {
        lanzar_error("Error al crear pipe");
    }

    pid = fork();
    if (pid == -1) {
        lanzar_error("Error en fork");
    }

    if (pid == 0) {
        /* Proceso hijo: solo necesita leer de la tuberia */
        close(tub[1]);
        close(fd_origen);
        proceso_hijo(tub[0], nombre_fragmento);
        exit(EXIT_SUCCESS);
    }

    /* Proceso padre: solo necesita escribir en la tuberia */
    close(tub[0]);
    transferir_a_hijo(fd_origen, tub[1], tam_trozo);
    close(tub[1]); /* Clave: al cerrar aqui, el hijo recibe EOF y puede salir del bucle */

    waitpid(pid, NULL, 0); /* Espera a que el hijo termine antes de pasar al siguiente trozo */
}

int main(int argc, char *argv[])
{
    char *archivo;
    long tam_trozo, tam_total, trozos;
    int fd_origen;

    parse_args(argc, argv, &archivo, &tam_trozo);

    fd_origen = open(archivo, O_RDONLY);
    if (fd_origen == -1) {
        lanzar_error("No se puede abrir el archivo original");
    }

    /* Calculamos el tamano total del archivo con lseek a nivel de kernel */
    tam_total = lseek(fd_origen, 0, SEEK_END);
    lseek(fd_origen, 0, SEEK_SET);

    /* Calculamos cuantos trozos necesitamos */
    trozos = (tam_total == 0) ? 1 : (tam_total + tam_trozo - 1) / tam_trozo;

    if (trozos > MAX_TROZOS) {
        lanzar_error("Demasiados fragmentos (>100): aumente el tamano");
    }

    /* Creamos los fragmentos de forma secuencial para no colapsar la memoria */
    for (int i = 0; i < trozos; i++) {
        crear_fragmento(fd_origen, archivo, i, tam_trozo);
    }

    close(fd_origen);
    return EXIT_SUCCESS;
}