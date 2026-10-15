#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("HelloWorldPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Manejador de salida limpia al menú XMB con botón HOME / PS */
static int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
}

int main(void) {
    pspDebugScreenInit();
    setup_callbacks();

    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    int count = 0;

    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);

        pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("==================================================\n");
        pspDebugScreenPrintf("       HOLA MUNDO DESDE PSP GO & MAC M2           \n");
        pspDebugScreenPrintf("==================================================\n\n");
        pspDebugScreenPrintf(" Compilado con GCC 15.2.0 (Entorno Aislado venv)\n\n");
        pspDebugScreenPrintf(" Contador de frames: %d\n\n", count++);

        pspDebugScreenPrintf(" [Lectura de Controles]\n");
        pspDebugScreenPrintf(" Stick Analogico X: %3d | Y: %3d\n", pad.Lx, pad.Ly);

        if (pad.Buttons & PSP_CTRL_CROSS) {
            pspDebugScreenPrintf(" Boton presionado:  [ X ] (Cruz)\n");
        } else if (pad.Buttons & PSP_CTRL_CIRCLE) {
            pspDebugScreenPrintf(" Boton presionado:  [ O ] (Circulo)\n");
        } else if (pad.Buttons & PSP_CTRL_TRIANGLE) {
            pspDebugScreenPrintf(" Boton presionado:  [ /\\ ] (Triangulo)\n");
        } else if (pad.Buttons & PSP_CTRL_SQUARE) {
            pspDebugScreenPrintf(" Boton presionado:  [ [] ] (Cuadrado)\n");
        } else {
            pspDebugScreenPrintf(" Boton presionado:  Ninguno\n");
        }

        pspDebugScreenPrintf("\n Pulsa el boton PS / HOME para salir al menu.\n");

        sceDisplayWaitVblankStart();
    }

    return 0;
}
