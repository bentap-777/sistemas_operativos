#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define TAM_BUFFER 4096
#define MAX_TROZOS 100 /* La numeración de los fragmentos va de .h00 a .h99. */

/* Escribe el error en stderr y termina el programa. */
void lanzar_error(const char *mensaje)
{
    write(STDERR_FILENO, mensaje, strlen(mensaje));
    write(STDERR_FILENO, "\n", 1);
    exit(EXIT_FAILURE);
}

/* Obtiene el archivo y el tamaño solicitado, y comprueba los argumentos. */
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

/* Forma el nombre del fragmento, por ejemplo archivo.h00 o archivo.h01. */
void construir_nombre(char *destino, const char *origen, int indice)
{
    int len;

    strcpy(destino, origen);
    strcat(destino, ".h");
    len = strlen(destino);

    destino[len]     = '0' + (indice / 10);
    destino[len + 1] = '0' + (indice % 10);
    destino[len + 2] = '\0';
}

/* El hijo copia al fragmento los datos que recibe por la tubería. */
void proceso_hijo(int tub_lectura, const char *nombre_fragmento)
{
    char buffer[TAM_BUFFER];
    ssize_t leidos;
    int fd_destino;

    fd_destino = open(nombre_fragmento, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd_destino == -1) {
        lanzar_error("Error al crear el archivo fragmento");
    }

    /* La lectura termina cuando el padre cierra la tubería y llega el EOF. */
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

/* El padre envía como máximo el tamaño de fragmento indicado. */
void transferir_a_hijo(int fd_origen, int tub_escritura, long tam_trozo)
{
    char buffer[TAM_BUFFER];
    long restantes = tam_trozo;
    long a_leer;
    ssize_t leidos;

    while (restantes > 0) {
        a_leer = (restantes < TAM_BUFFER) ? restantes : TAM_BUFFER;

        leidos = read(fd_origen, buffer, a_leer);
        if (leidos == -1) {
            lanzar_error("Error al leer archivo original");
        }
        if (leidos == 0) {
            break; /* Ya no quedan datos en el archivo original. */
        }

        if (write(tub_escritura, buffer, leidos) == -1) {
            lanzar_error("Error al volcar datos en la tuberia");
        }

        restantes -= leidos;
    }
}

/* Crea un hijo y una tubería para producir un único fragmento. */
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
        close(tub[1]);     /* El hijo solo necesita leer de la tubería. */
        close(fd_origen);   /* El archivo original lo gestiona el padre. */
        proceso_hijo(tub[0], nombre_fragmento);
        exit(EXIT_SUCCESS);
    }

    /* Proceso padre */
    close(tub[0]); /* El padre solo necesita escribir en la tubería. */
    transferir_a_hijo(fd_origen, tub[1], tam_trozo);
    close(tub[1]); /* Al cerrar, el hijo recibe EOF y puede terminar la copia. */

    waitpid(pid, NULL, 0); /* No empieza el siguiente fragmento hasta que este esté listo. */
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

    /* Consulta el tamaño del archivo y deja su posición al principio. */
    tam_total = lseek(fd_origen, 0, SEEK_END);
    lseek(fd_origen, 0, SEEK_SET);

    /* Incluso un archivo vacío produce un fragmento, que también queda vacío. */
    trozos = (tam_total == 0) ? 1 : (tam_total + tam_trozo - 1) / tam_trozo;

    if (trozos > MAX_TROZOS) {
        lanzar_error("Demasiados fragmentos (>100): aumente el tamano");
    }

    /* Se procesa cada fragmento por turno, usando el mismo archivo de origen. */
    for (int i = 0; i < trozos; i++) {
        crear_fragmento(fd_origen, archivo, i, tam_trozo);
    }

    close(fd_origen);
    return EXIT_SUCCESS;
}