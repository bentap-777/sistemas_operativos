# Práctica 2: Comunicaciones en red y concurrencia

Esta práctica reúne dos ejercicios independientes. El primero transmite un
archivo entre procesos mediante sockets TCP; el segundo sincroniza un productor
y un consumidor que comparten un buffer limitado mediante semáforos. El
contexto y el enunciado completos están en
[SO-2026-2027-Práctica2.pdf](SO-2026-2027-Práctica2.pdf).

## Estructura

| Carpeta | Contenido | Explicación |
|---|---|---|
| [Red/](Red/) | `Servidor.c`, `Cliente.c` y `Google.html` | El servidor envía el archivo al cliente, que muestra su contenido por pantalla. |
| [Concurrencia/](Concurrencia/) | `productor_consumidor.cm` | Simulación del problema productor-consumidor con un buffer circular y semáforos en JBACI. |

Cada carpeta contiene un README con una explicación breve de sus archivos y
cómo ejecutar el ejercicio.

## Requisitos

- **Comunicaciones en red:** Linux o Unix y un compilador C como `gcc`.
- **Concurrencia:** entorno JBACI compatible con el fichero `.cm`.

Los ejercicios se compilan o ejecutan por separado. Consulta
[Red/README.md](Red/README.md) y
[Concurrencia/README.md](Concurrencia/README.md) para ver sus comandos y
detalles.

## Entrega y evaluación

El enunciado pide demostrar la ejecución y entregar una memoria que describa
cómo se resolvió cada ejercicio junto con sus fuentes. La práctica valora la
correcta implementación y ejecución; en concurrencia también se considera el
uso de la interfaz gráfica de JBACI.
