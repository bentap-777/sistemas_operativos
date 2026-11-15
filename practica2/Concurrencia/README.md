# Concurrencia: productor-consumidor

`productor_consumidor.cm` describe en C-- para JBACI el problema del productor
y el consumidor con un buffer de capacidad limitada. Se producen y consumen
15 elementos usando un buffer circular de 5 posiciones.

## Elementos principales

| Elemento | Función |
|---|---|
| `buffer[N]` | Almacena temporalmente los elementos compartidos. |
| `entrada` y `salida` | Índices de escritura y lectura; avanzan circularmente con el resto de dividir por `N`. |
| `mutex` | Protege el acceso al buffer para que solo un proceso lo modifique o lea a la vez. |
| `vacios` | Cuenta los espacios libres; el productor espera cuando no queda ninguno. |
| `llenos` | Cuenta los elementos disponibles; el consumidor espera cuando no hay ninguno. |
| `productor()` | Produce cada valor, espera un espacio, lo guarda y avisa de que hay un elemento disponible. |
| `consumidor()` | Espera un elemento, lo lee, libera la posición y avisa de que hay espacio libre. |
| `main()` | Inicializa los semáforos y ejecuta productor y consumidor concurrentemente con `cobegin`. |

## Sincronización

Los semáforos comienzan con `mutex = 1`, `vacios = N` y `llenos = 0`. Ambos
procesos esperan primero por el semáforo de disponibilidad correspondiente y
después por `mutex` antes de acceder al buffer. Al terminar el acceso, liberan
`mutex` y actualizan el contador que permite avanzar al otro proceso. Así se
evitan tanto las lecturas de un buffer vacío como las escrituras cuando está
lleno, además de proteger la sección crítica.

## Ejecución

Abre `productor_consumidor.cm` en JBACI y ejecuta la simulación. La interfaz
permite observar la alternancia entre producción y consumo; el programa también
imprime el valor y la posición usados en cada operación.
