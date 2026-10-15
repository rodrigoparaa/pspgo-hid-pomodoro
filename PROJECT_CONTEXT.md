# CONTEXTO TÉCNICO, ARQUITECTURA Y ESTADO DEL PROYECTO (PSP GO & MACOS)

> **Documento de Continuidad y Especificación Técnica para Modelos de Inteligencia Artificial y Desarrolladores.**  
> **Fecha de actualización:** Octubre 2026  
> **Directorio raíz del proyecto:** `MandoPSP/` (o `~/Desktop/MandoPSP`)  
> **Dispositivo objetivo:** Sony PlayStation Portable Go (PSP-N1000)

---

## 1. Ficha Técnica del Hardware y Sistema Operativo

| Componente | Especificación Técnica | Notas de Integración |
| :--- | :--- | :--- |
| **Dispositivo** | Sony PSP Go (Modelo N1000) | Factor de forma slider ultracompacto. |
| **Almacenamiento** | 16 GB eMMC Flash interna (`ef0:`) | **No** se utiliza Memory Stick M2 externa (`ms0:`). Todo reside en `ef0:`. |
| **Arquitectura CPU** | MIPS R4000 (Allegrex) @ 333 MHz | Procesador de 32 bits con coprocesador vectorial VFPU y Media Engine. |
| **Memoria RAM** | 64 MB DDR SDRAM | Los modelos PSP-2000, 3000 y Go disponen de 64 MB (frente a los 32 MB de la 1000). |
| **Pantalla** | TFT LCD 3.8 pulgadas, 480 × 272 píxeles, 60 Hz | Relación de aspecto 16:9. |
| **Sensores físicos** | Sensor Hall magnético de pantalla deslizante | Detecta apertura y cierre del panel deslizable (SYSCON kernel). |
| **Controles físicos** | D-Pad, Stick analógico, `X`, `O`, `[]`, `/\`, `L`, `R`, `PS`, `VOL+/-`, `Display`, `Note`, `WLAN` | Los gatillos **L** y **R** y botones de volumen permanecen accesibles con el slider cerrado. |
| **Conectividad** | Conector multipropósito propietario USB 2.0, Wi-Fi 802.11b, Bluetooth 2.0+EDR | Admite conexión de mandos DualShock 3 y enlace de datos. |
| **Firmware Base** | 6.60 Oficial con marcador Infinity (`6.60∞`) | Marcador permanente de Infinity presente en `flash0`. |
| **Custom Firmware Activo** | **6.60 PROMOD-C2p VCFW** | Instalado en memoria mediante `PROUPDATE` y reactivable vía `FastRecovery`. |
| **Estado del VSH** | **Operativo al 100%** | El menú flotante `PRO VSH MENU` se despliega pulsando `SELECT` en el XMB. |

---

## 2. Proyecto 1: PSP Go como Mando USB HID en macOS (Completado)

### 2.1. Software Utilizado
* **Aplicación:** FuSa GamePad v0.3
* **Ruta de instalación en la consola:** `ef0:/PSP/GAME/FusaGamePad/`
* **Archivos clave:**
  * `EBOOT.PBP` (124.739 bytes): Ejecutable principal en espacio de usuario.
  * `usbgamepad.prx` (3.823 bytes): Driver kernel cifrado (~PSP) que gestiona el stack USB de la consola.
  * `config.ini`: Configuración de mapeo de botones, deadzone analógica y switchers.

### 2.2. Auditoría y Refutación del Mito del Parche Perl (`ms0:` vs `ef0:`)
* **Hipótesis comunitaria previa:** *"En PSP Go se debe parchear el binario con `perl -pi -e 's/ms0:/ef0:/g' EBOOT.PBP` para que cargue el driver desde la memoria interna"*.
* **Hallazgo tras ingeniería inversa del ELF:**
  * Se inspeccionó la sección `.rodata` y `.text` del ejecutable.
  * La cadena `"ms0:"` **no existe** en `EBOOT.PBP` ni en `usbgamepad.prx`.
  * La rutina de carga en `0x08902f50` obtiene la ruta dinámica del directorio de trabajo llamando a `getcwd()` y concatena `/usbgamepad.prx`.
  * **Causa real del fallo previo:** La consola estaba arrancando en firmware oficial (OFW 6.60) debido a instaladores de 6.61 que residían en la memoria. El OFW bloqueaba la ejecución de módulos kernel. Una vez cargado el CFW 6.60 PRO-C, FuSa funciona directamente desde `ef0:` sin modificación binaria alguna.

### 2.3. Integración con macOS Apple Silicon & Steam Input
* **Comportamiento nativo de macOS:**
  * El sistema reconoce la PSP Go como un dispositivo USB HID Gamepad estándar (`VendorID: Sony / FuSaPAD`).
  * Las herramientas web (como `gamepad-tester.com`) reciben los eventos inmediatamente a través de la Gamepad API HTML5.
* **Problema con motores de videojuegos (Unity):**
  * Títulos nativos de macOS como ***Cult of the Lamb*** utilizan el motor Unity con el framework Apple GameController, el cual ignora mandos HID genéricos sin perfiles oficiales.
* **Solución Técnica Implementada (Steam Input):**
  1. Habilitación de *"Steam Input para mandos genéricos"* en los ajustes globales de Steam.
  2. Forzado de *"Habilitar Steam Input"* en las propiedades del juego.
  3. Mapeo verificado y funcional:
     * **Stick analógico:** Movimiento del personaje.
     * **Cruz (`X`):** Interactuar / Confirmar.
     * **Cuadrado (`[]`):** Atacar.
     * **Círculo (`O`):** Rodar / Esquivar.
     * **Triángulo (`/\`):** Maldición / Disparo especial.
     * **Gatillos L / R:** Habilidad / Reliquia.
     * **START / SELECT:** Pausa e inventario.

---

## 3. Proyecto 2: Entorno de Desarrollo Aislado (PSPDEV venv)

Para evitar dependencias globales y permitir la reproducibilidad en macOS Apple Silicon (ARM64), el entorno de compilación se diseñó con una arquitectura aislada tipo virtualenv.

### 3.1. Estructura de Directorios en `MandoPSP`
```text
MandoPSP/
├── activate.sh                    # Script de activación del entorno virtual
├── .gitignore                     # Exclusión de pspdev/ (400 MB) y metadatos macOS
├── README.md                      # Portada oficial de GitHub
├── PROJECT_CONTEXT.md             # Memoria técnica de desarrollo
├── projects/                      # Directorio de aplicaciones caseras
│   ├── 00_HelloWorld/             # Proyecto piloto de validación [COMPILADO CON ÉXITO]
│   │   ├── main.c                 # Código C con lectura de pads y pantalla debug
│   │   ├── Makefile               # Script de compilación con reglas build.mak
│   │   └── EBOOT.PBP              # Binario ejecutable final empaquetado (131 KB)
│   ├── 00_Probe/                  # Suite de telemetría de hardware (Fase 0 de diagnóstico)
│   │   ├── main.c                 # Lógica de muestreo y benchmark
│   │   ├── Makefile               # Reglas de compilación
│   │   ├── EBOOT.PBP              # Binario ejecutable (136 KB)
│   │   └── probe_log.txt          # Informe generado en hardware real
│   └── 01_DeskClock_Pomodoro/     # [100% OPERATIVO EN HARDWARE] Reloj de mesa y Pomodoro
│       ├── core/                  # pomodoro.c, input_logic.c, app_state.c (Lógica pura con tests)
│       ├── ui/                    # font.c, graphics.c (Hardware GU Double-buffer), screens.c
│       ├── platform/              # psp_audio.c (Chimes sintetizados), psp_battery.c
│       ├── tests/                 # Suite de pruebas unitarias test_core.c (100% pasando)
│       ├── Makefile               # Reglas de compilación con -lpspgu
│       ├── DESIGN.md              # Especificación lógica
│       ├── INFORME_TECNICO.md     # Informe exhaustivo de ingeniería
│       └── EBOOT.PBP              # Binario final empaquetado (154 KB)
```

---

## 4. Estado Actual de la Suite de Aplicaciones

### 4.1. Proyecto A: Desk Clock & Pomodoro (`01_DeskClock_Pomodoro`) — ESTADO: COMPLETADO Y VERIFICADO EN HARDWARE
* **Pipeline Gráfico:** 100% libre de tearing. Inicialización oficial del coprocesador gráfico mediante `libpspgu` (`sceGuInit`). Búferes duales (512x272 @ 32-bit RGBA) intercambiados con `sceGuSwapBuffers()` en sincronía estricta con `sceDisplayWaitVblankStart()`.
* **Tipografía Digital Continua:** Glifos de 7 segmentos con grosor $h/8$ y unión perfecta en esquinas, eliminando huecos de píxeles para lograr una estética digital sólida.
* **Operación con Slider Plegado:** Gatillos **L** (Play/Pausa), **R** (Saltar) y **R sostenido** (Reset) funcionando con la pantalla cerrada.
* **Combo de Navegación:** Mantener `L + R` (1s) cicla entre Reloj, Pomodoro, Batería y Guía de Controles.
* **Audio Sintetizado:** Secuencia armónica suave de dos tonos (La5 y Re6) generada matemáticamente en tiempo real por hilo dedicado.
* **Telemetría de Batería:** Porcentaje, milivoltios y estado de carga, saneando códigos negativos del sensor en PSP Go.
* **Keep-Awake:** `scePowerTick(PSP_POWER_TICK_ALL)` ejecutado cada 4 segundos impidiendo que la consola se suspenda o atenúe.

### 4.2. Proyecto B: Lector Distraction-Free de Markdown (`02_MarkdownReader`) — PLANIFICADO
* Lectura técnica de apuntes `.md` desde `ef0:/DOCS/` con scroll analógico a 60 FPS y paso de página con gatillos L/R.

### 4.3. Proyecto C: Caja de Herramientas Offline (`03_PocketToolkit`) — PLANIFICADO
* Conversor Hex/Dec/Bin/Oct con operaciones bitwise, cronómetro de precisión RTC y notas cifradas AES-256.

---

## 5. Instrucciones para Continuidad con Agentes de IA

1. **Entorno de Compilación:**
   * Activar siempre el entorno aislado antes de compilar:
     ```bash
     source activate.sh && cd projects/01_DeskClock_Pomodoro && make
     ```
   * Nunca compilar con el `gcc` o `clang` del sistema anfitrión; invocar las herramientas a través de `psp-gcc` provisto en el entorno.
2. **Despliegue Seguro en Hardware:**
   * Copiar los binarios `EBOOT.PBP` a `/Volumes/PSP GO/PSP/GAME/<NombreApp>/`.
   * Ejecutar siempre `dot_clean "/Volumes/PSP GO"` y `sync` tras cualquier transferencia para evitar "Datos dañados".
   * **Regla crítica de seguridad:** **NUNCA** desplegar ni ejecutar `CIPL_Flasher` en la PSP Go (provoca brick irreversible de la placa base).
