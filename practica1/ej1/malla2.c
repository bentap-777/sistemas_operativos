#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * [CHECKLIST: FUNCIÓN PARSE_ARGS]
 * - [x] Paso 1: Verificar que se pasaron exactamente 2 argumentos (argc == 3).
 * - [x] Paso 2: Convertir los textos a números enteros con atoi().
 * - [x] Paso 3: Validar que tanto filas como columnas sean mayores que 0.
 */
void parse_args(int argc, char *argv[], int *filas, int *columnas) {
    // [PASO 1: VALIDACIÓN DE CANTIDAD DE PARÁMETROS]
    if (argc != 3) {
        fprintf(stderr, "Uso correcto: %s <filas (x)> <columnas (y)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // [PASO 2: CONVERSIÓN DE CADENA A ENTERO]
    *filas = atoi(argv[1]);
    *columnas = atoi(argv[2]);

    // [PASO 3: COMPROBACIÓN DE VALORES VÁLIDOS]
    if (*filas <= 0 || *columnas <= 0) {
        fprintf(stderr, "Error: Las filas y columnas deben ser enteros mayores a 0.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * [CHECKLIST: FUNCIÓN CREAR_CADENA_VERTICAL]
 * - [x] Paso 1: Imprimir la identidad del proceso actual p(fila, columna).
 * - [x] Paso 2: Freno/Hoja: si llegamos a la fila final, pausar con sleep() para pstree y retornar con return.
 * - [x] Paso 3: Clonar con fork() para crear el siguiente nivel hacia abajo.
 * - [x] Paso 4: Ruta del hijo: descender recursivamente sumando 1 a fila_actual y salir al terminar.
 * - [x] Paso 5: Ruta del padre: esperar al hijo con wait() y retornar limpiamente.
 */
void crear_cadena_vertical(int fila_actual, int total_filas, int col_actual) {
    // [PASO 1: IDENTIFICACIÓN DEL NODO]
    printf("Proceso p(%d,%d) creado | PID: %d | Padre (PPID): %d\n",
           fila_actual, col_actual, getpid(), getppid());

    // [PASO 2: CONDICIÓN DE PARADA / NODO HOJA]
    if (fila_actual >= total_filas) {
        // Pausa de 15 segundos en las hojas para poder inspeccionar con 'pstree -c'
        sleep(15);
        return; // Retorno normal para que GCC no detecte recursión infinita
    }

    // [PASO 3: CREACIÓN DEL SIGUIENTE ESLABÓN VERTICAL]
    pid_t pid_hijo = fork();
    if (pid_hijo < 0) {
        perror("Error en fork vertical");
        exit(EXIT_FAILURE);
    }

    // [PASO 4: RAMA DEL HIJO (CONTINÚA BAJANDO)]
    if (pid_hijo == 0) {
        crear_cadena_vertical(fila_actual + 1, total_filas, col_actual);
        exit(EXIT_SUCCESS); // El hijo finaliza cuando su subárbol ha terminado
    }

    // [PASO 5: RAMA DEL PADRE (ESPERA CONTROLADA)]
    // Espera a que termine su hijo inferior antes de regresar
    wait(NULL);
    return;
}

/*
 * [CHECKLIST: FUNCIÓN CREAR_COLUMNAS_HORIZONTAL]
 * - [x] Paso 1: Freno: si col_actual supera total_columnas, retornar.
 * - [x] Paso 2: Clonar con fork() para crear la columna actual.
 * - [x] Paso 3: Ruta del hijo: iniciar la cadena vertical llamando a crear_cadena_vertical().
 * - [x] Paso 4: Ruta de la raíz: llamarse recursivamente con col_actual + 1 para crear las demás columnas.
 * - [x] Paso 5: Ruta de la raíz: esperar a que esta columna termine con wait().
 */
void crear_columnas_horizontal(int col_actual, int total_columnas, int total_filas) {
    // [PASO 1: FRENO HORIZONTAL]
    if (col_actual > total_columnas) {
        return;
    }

    // [PASO 2: CREACIÓN DE LA COLUMNA ACTUAL]
    pid_t pid_col = fork();
    if (pid_col < 0) {
        perror("Error en fork horizontal");
        exit(EXIT_FAILURE);
    }

    // [PASO 3: RAMA DEL HIJO (CABEZA DE COLUMNA)]
    if (pid_col == 0) {
        crear_cadena_vertical(1, total_filas, col_actual);
        exit(EXIT_SUCCESS);
    }

    // [PASO 4: RAMA DE LA RAÍZ (CREAR SIGUIENTE COLUMNA)]
    crear_columnas_horizontal(col_actual + 1, total_columnas, total_filas);

    // [PASO 5: ESPERA DE LA RAÍZ]
    wait(NULL);
}

/*
 * [CHECKLIST: FUNCIÓN MAIN]
 * - [x] Paso 1: Validar parámetros recibidos.
 * - [x] Paso 2: Imprimir cabecera con el PID de la raíz.
 * - [x] Paso 3: Disparar la recursividad horizontal.
 * - [x] Paso 4: Mensaje de cierre una vez que todos los hijos murieron.
 */
int main(int argc, char *argv[]) {
    int total_filas = 0;
    int total_columnas = 0;

    // [PASO 1: VALIDACIÓN]
    parse_args(argc, argv, &total_filas, &total_columnas);

    // [PASO 2: CABECERA]
    printf("=== PROCESO RAIZ (PID: %d) ===\n", getpid());
    printf("Generando arbol: %d filas x %d columnas...\n", total_filas, total_columnas);

    // [PASO 3: INICIO DE LA CONSTRUCCIÓN RECURSIVA]
    crear_columnas_horizontal(1, total_columnas, total_filas);

    // [PASO 4: FINALIZACIÓN CONTROLADA]
    printf("=== PROCESO RAIZ (PID: %d) TERMINADO: ARBOL DESTRUIDO SIN HUERFANOS ===\n", getpid());
    return 0;
}