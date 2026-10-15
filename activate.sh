#!/usr/bin/env bash
# ==============================================================================
# Entorno Virtual Aislado para Desarrollo PSP (PSPDEV venv)
# Uso: source activate.sh
# Para salir: deactivate_psp
# ==============================================================================

# Obtener la ruta absoluta del directorio donde reside este script
if [ -n "$BASH_SOURCE" ]; then
    _SCRIPT_PATH="${BASH_SOURCE[0]}"
elif [ -n "$ZSH_VERSION" ]; then
    _SCRIPT_PATH="${(%):-%x}"
else
    _SCRIPT_PATH="$0"
fi

_PSP_ROOT="$(cd "$(dirname "$_SCRIPT_PATH")" && pwd)"
export PSPDEV="$_PSP_ROOT/pspdev"

if [ ! -d "$PSPDEV/bin" ]; then
    echo "❌ Error: No se encontró la carpeta del compilador en $PSPDEV"
    return 1 2>/dev/null || exit 1
fi

# Guardar estado previo para poder desactivar limpiamente
_OLD_PSP_PATH="$PATH"
_OLD_PSP_PS1="$PS1"
export _OLD_PSP_PATH _OLD_PSP_PS1

# Agregar binarios al PATH
export PATH="$PSPDEV/bin:$PATH"

# Definir función para desactivar el entorno
deactivate_psp() {
    if [ -n "$_OLD_PSP_PATH" ]; then
        export PATH="$_OLD_PSP_PATH"
        unset _OLD_PSP_PATH
    fi
    if [ -n "$_OLD_PSP_PS1" ]; then
        export PS1="$_OLD_PSP_PS1"
        unset _OLD_PSP_PS1
    fi
    unset PSPDEV
    unset -f deactivate_psp
    echo "👋 Entorno PSP desactivado."
}

# Modificar el prompt de la terminal
export PS1="(psp-env) $PS1"

echo "=========================================================="
echo "🎮 Entorno PSPDEV activado (Aislado en MandoPSP)"
echo "   Compilador: $(psp-gcc --version 2>/dev/null | head -n 1)"
echo "   Ruta SDK:   $PSPDEV"
echo "   Comando para desactivar: deactivate_psp"
echo "=========================================================="
