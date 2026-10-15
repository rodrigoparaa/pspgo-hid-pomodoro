# 🎮 PSP Go Modern Lab: USB HID Gamepad & Desk Clock Pomodoro

<div align="center">

![Platform](https://img.shields.io/badge/Platform-Sony_PSP_Go_(N1000)-003791?style=for-the-badge&logo=playstation&logoColor=white)
![Host](https://img.shields.io/badge/Host-macOS_Apple_Silicon_(M1/M2/M3)-000000?style=for-the-badge&logo=apple&logoColor=white)
![Toolchain](https://img.shields.io/badge/Toolchain-GCC_15.2.0_MIPS-E65B00?style=for-the-badge&logo=gnu&logoColor=white)
![Language](https://img.shields.io/badge/Language-Pure_C99-00599C?style=for-the-badge&logo=c&logoColor=white)
![Framerate](https://img.shields.io/badge/Graphics-60_FPS_GU_Double--Buffer-2EA44F?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-blue?style=for-the-badge)

<p align="center">
  <b>Un laboratorio integral de ingeniería de software, explotación de hardware y desarrollo nativo para PlayStation Portable Go (PSP-N1000) bajo macOS Apple Silicon.</b>
</p>

</div>

---

## 📌 Visión General del Proyecto

Este repositorio documenta e implementa dos vertientes técnicas complementarias para la **Sony PSP Go**:

1. **Controlador USB HID de Baja Latencia para macOS:** Configuración de la consola como mando físico Plug-and-Play para macOS, resolviendo la compatibilidad con motores de videojuegos modernos (como Unity en *Cult of the Lamb*) mediante la intermediación de Steam Input.
2. **Suite de Escritorio Nativa (Desk Clock & Pomodoro):** Aplicación en C puro compilada para arquitectura MIPS que convierte la consola plegada en un reloj minimalista y temporizador Pomodoro de sobremesa, operable exclusivamente mediante los gatillos físicos superiores **L** y **R** con doble búfer por hardware a 60 FPS estables.
3. **Entorno de Desarrollo Aislado (Estilo `venv`):** Infraestructura local en macOS que permite compilar proyectos de PSP mediante un toolchain MIPS moderno (GCC 15.2.0) sin alterar librerías globales del sistema.

---

## 🏗️ Arquitectura del Repositorio

```text
.
├── activate.sh                     # Script de entorno virtual tipo venv (source activate.sh)
├── .gitignore                      # Reglas de exclusión para control de versiones
├── projects/
│   ├── 00_HelloWorld/              # Banco de pruebas inicial de compilación y lectura de pads
│   ├── 00_Probe/                   # Suite de telemetría de hardware (Fase 0 de diagnóstico)
│   └── 01_DeskClock_Pomodoro/      # Aplicación principal de escritorio y productividad
│       ├── core/                   # Lógica pura desacoplada (Pomodoro, gestos, enrutador)
│       ├── ui/                     # Pipeline gráfico GU, fuente bitmap y pantallas
│       ├── platform/               # Módulos de audio sintetizado y telemetría de batería
│       ├── tests/                  # Suite de pruebas unitarias ejecutables en el host con clang
│       ├── Makefile                # Reglas de compilación y empaquetado EBOOT.PBP
│       ├── DESIGN.md               # Especificación funcional detallada
│       ├── INFORME_TECNICO.md      # Informe exhaustivo de ingeniería de bajo nivel
│       └── EBOOT.PBP               # Binario de producción listo para ejecutar
└── README.md                       # Documentación principal del repositorio
```

---

## 🕹️ Vertiente 1: PSP Go como Mando USB en macOS

La PSP Go cuenta con conectividad USB 2.0 y factor de forma slider. Mediante el driver **FuSa GamePad v0.3**, la consola se enumera como un dispositivo genérico de entrada USB HID.

```mermaid
flowchart LR
    A["PSP Go (ef0:)"] -- "USB HID Driver (FuSa)" --> B["macOS Host (Apple Silicon)"]
    B -- "HID Raw Input" --> C["Steam Input Layer"]
    C -- "Virtual Gamepad" --> D["Unity / Native Games\n(ej. Cult of the Lamb)"]
```

### Protocolo de Conexión
1. **Arranque:** Iniciar `FuSa GamePad` desde el menú de la consola **antes** de conectar el cable USB para evitar que monte el almacenamiento masivo.
2. **Enlace:** Conectar el cable al Mac. La pantalla de la consola confirmará el estado `:: CONNECTED ::`.
3. **Salida limpia:** Desconectar el cable USB, pulsar el botón físico **`PS / HOME`** y confirmar con **`X`** para volver al menú XMB.

### Compatibilidad con macOS y Motores Modernos (Unity)
Los motores de juego modernos en macOS que utilizan el framework `GameController` de Apple suelen descartar mandos HID genéricos. Para solventarlo, se activa **Steam Input para mandos genéricos**:
* Steam traduce en tiempo real las llamadas HID a un mando virtual estándar.
* Se obtiene soporte inmediato para sticks analógicos, cruceta y botones de acción sin retraso apreciable de entrada (*input lag*).

---

## ⏱️ Vertiente 2: Desk Clock & Pomodoro (`01_DeskClock_Pomodoro`)

Una aplicación de sobremesa diseñada específicamente para el factor de forma de la PSP Go. Cuando la consola descansa plegada en la mesa o en su dock, los gatillos superiores **L** y **R** quedan orientados hacia arriba, permitiendo operar un temporizador Pomodoro completo sin necesidad de abrir la pantalla.

```text
┌─────────────────────────────────────────────────────────────┐
│  DESK CLOCK  •  PSP GO                              90% [■] │  <- Barra superior sutil
├─────────────────────────────────────────────────────────────┤
│                                                             │
│                    15 DE OCTUBRE DE 2026                    │
│                                                             │
│                    2 0 : 4 5 : 1 2                          │  <- Dígitos de 7 segmentos sólidos
│                                                             │
│       ┌─────────────────────────────────────────────┐       │
│       │  POMODORO EN CURSO       24:18    Activo    │       │  <- Tarjeta de estado
│       │  [■■■■■■■■■■■□□□□□□□□□□□□□□□□□□□□□□□□□□□□□]  │       │
│       └─────────────────────────────────────────────┘       │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  [L+R] Siguiente Pantalla                  PSP Go Desk App  │  <- Barra de navegación
└─────────────────────────────────────────────────────────────┘
```

### Características Principales
* **Motor Gráfico Double-Buffered a 60 FPS Oficial (libpspgu):** Inicialización del coprocesador gráfico oficial de Sony (`sceGuInit`). Los búferes de dibujo y visualización (512x272 a 32 bits RGBA) se intercambian mediante `sceGuSwapBuffers()` en sincronía estricta con el barrido vertical físico (`sceDisplayWaitVblankStart`). Cero tearing, cero artefactos y cero caídas de frames.
* **Tipografía Digital Sólida y Continua:** Trazos de segmentos conectados sin interrupciones ni huecos de píxeles, maximizando la legibilidad en la pantalla LCD de la consola.
* **Temporizador Pomodoro Inmune a Desfase Temporal:** El motor calcula el tiempo transcurrido utilizando el reloj de tiempo real del hardware (`sceRtcGetCurrentTick`) en milisegundos, garantizando que el cronómetro se mantenga matemáticamente exacto independientemente de variaciones en la tasa de refresco.
* **Alertas Acústicas Sintetizadas:** Doble tono armónico generado por software en tiempo real mediante el hardware de audio (`libpspaudio`) sin necesidad de incluir archivos WAV/MP3 externos.
* **Telemetría de Batería con Filtrado de Sensores:** Consulta directa a la API de energía (`libpsppower`) mostrando porcentaje, voltaje exacto en milivoltios y estado de carga, saneando lecturas anómalas de temperatura no soportadas por la arquitectura PSP Go.
* **Keep-Awake de Sistema:** Llamada periódica a `scePowerTick()` para evitar que el sistema operativo atenúe la iluminación o suspenda la consola por inactividad.

---

## 🎮 Esquema de Controles

La aplicación cuenta con una **Guía Rápida de Controles (Cheatsheet)** integrada a la que se accede en cualquier momento pulsando **`SELECT`**.

| Acción | Consola Cerrada (Slider Plegado) | Consola Abierta (Desplegada) |
| :--- | :--- | :--- |
| **Play / Pausar Pomodoro** | Pulsación corta en **`Gatillo L`** | **`Gatillo L`** o botón **`Cruz (X)`** |
| **Saltar Fase (Trabajo/Descanso)** | Pulsación corta en **`Gatillo R`** | **`Gatillo R`** |
| **Reiniciar Ciclo Pomodoro** | Mantener **`Gatillo R`** durante 1.5 s | Mantener **`Gatillo R`** durante 1.5 s |
| **Cambiar Pantalla (Cíclico)** | Mantener **`L + R`** durante 1.0 s | Mantener **`L + R`** o **`Cruceta ◄ / ►`** |
| **Abrir / Cerrar Guía de Ayuda** | — | Botón **`SELECT`** |
| **Volver al Reloj Principal** | — | Botón **`Círculo (O)`** |
| **Salir al Menú XMB de la PSP** | Botón **`PS / HOME`** | Botón **`PS / HOME`** |

> **Nota de ingeniería:** El procesador de entrada incorpora lógica de cancelación por software: al pulsar el combo sostenido `L + R`, el sistema inhibe deliberadamente la emisión de toques sueltos individuales al soltar los gatillos.

---

## 💻 Entorno de Desarrollo y Compilación

Para evitar conflictos de dependencias en macOS (Apple Silicon M-Series), el entorno funciona de manera homóloga a un entorno virtual (`venv`) de Python.

### Requisitos Previos en el Host
* macOS 13+ en Apple Silicon (ARM64).
* Gestor de paquetes Homebrew con las librerías auxiliares de compilador:
  ```bash
  brew install libmpc mpfr gmp
  ```

### Activación del Entorno
Desde el directorio raíz del repositorio:
```bash
# Activar el entorno virtual
source activate.sh

# El prompt se transformará en:
# (psp-env) usuario@host:MandoPSP $
```

### Compilar y Probar

1. **Ejecutar Pruebas Unitarias en el Mac (Host):**
   La lógica pura (`core/`) no contiene dependencias del SDK de PSP y puede verificarse de inmediato con el compilador nativo de macOS:
   ```bash
   cd projects/01_DeskClock_Pomodoro/tests
   clang -Wall -Wextra -O2 -I../core ../core/*.c test_core.c -o run_tests && ./run_tests
   ```

2. **Compilar el Ejecutable Final para PSP Go:**
   ```bash
   cd projects/01_DeskClock_Pomodoro
   make clean && make
   ```
   *Generará el binario oficial empaquetado `EBOOT.PBP` listo para su ejecución.*

3. **Desactivar el Entorno:**
   ```bash
   deactivate_psp
   ```

---

## 🚀 Despliegue en la PSP Go

1. Conectar la PSP Go al Mac mediante cable USB en modo *Conexión USB*.
2. Crear el directorio de la aplicación en el almacenamiento interno:
   ```bash
   mkdir -p "/Volumes/PSP GO/PSP/GAME/DeskClock"
   cp projects/01_DeskClock_Pomodoro/EBOOT.PBP "/Volumes/PSP GO/PSP/GAME/DeskClock/"
   ```
3. Purgar metadatos ocultos creados por macOS (`._*` y `.DS_Store`) para evitar errores de lectura en el menú de la consola:
   ```bash
   dot_clean "/Volumes/PSP GO/PSP/GAME"
   sync
   ```
4. Expulsar el volumen de forma segura desde el Finder y desconectar el cable. La app aparecerá en **Juego ➔ Memoria del sistema**.

---

## 📄 Licencia y Créditos

* **Licencia:** MIT License. Código abierto para la comunidad retro y de desarrollo homebrew.
* **Toolchain:** Proyecto comunitario oficial de preservación [PSPDEV](https://github.com/pspdev).
* **Driver USB:** FuSa GamePad por Andy_maN.
