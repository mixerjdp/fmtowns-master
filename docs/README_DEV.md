# FM Towns libretro core — Guía para desarrolladores

Core libretro basado en **MAME 0.287** que emula **FM Towns / FM Towns Marty**. A diferencia de MAME vanilla, no incluye UI, debugger ni plugins; corre in-process en RetroArch como `.dll`/`.so`.

---

## Estructura del repositorio

```
libretro/          → Wrapper libretro (fmtowns_libretro.cpp, osd_libretro.cpp, mame_bridge.cpp)
src/               → Árbol MAME filtrado (drivers, emu, devices, mess)
3rdparty/          → zlib, zstd, flac, expat, lua, asio, genie, etc.
build/             → Artefactos de compilación (.o, .a, .dll/.so)
build32/           → Artefactos para build x86
scripts/           → Scripts GENIE (*.lua) para generar makefiles
docs/              → Documentación técnica y de usuario
hash/              → Software lists XML (cdtowns.xml, floptowns.xml, etc.)
cfg/               → Configuraciones por defecto del core
nvram/             → Archivos NVRAM generados en tiempo de ejecución
regtests/          → Pruebas de regresión
libretro/          → Código del wrapper libretro
```

---

## Arquitectura del sistema

```
RetroArch
    │
    ├── Retro API (libretro.h)
    │       │
    │       ▼
    │   libretro/fmtowns_libretro.cpp  ← Punto de entrada libretro
    │       │
    │       ▼
    │   libretro/mame_bridge.cpp       ← Inicializa running_machine sin frontend
    │       │
    │       ▼
    │   libretro/osd_libretro.cpp      ← Video, audio, input mínimos
    │       │
    │       ▼
    │   MAME static libs (~20 .a)      ← src/ compilado como bibliotecas
    │       │
    │       ▼
    │   emu/machine.cpp                ← Ciclo principal de emulación
    │   emu/video.cpp, emu/audio.cpp, emu/input.cpp
```

### Flujo de arranque

1. **retro_init()** — `fmtowns_libretro.cpp`: Inicializa el bridge MAME, registra dispositivos y carga la configuración base.
2. **retro_load_game(super_info)** — Carga la ROM/software list indicada. `mame_bridge.cpp` abre el driver correspondiente con `machine_config`.
3. **retro_run()** — Por cada frame, llama a `machine->run()` que ejecuta tantos ciclos de CPU como sean necesarios para alcanzar el próximo refresh de video.
4. **osd_libretro_update_video()** — Toma el buffer de video de MAME, lo convierte al formato solicitado por RetroArch (RGB565 o XRGB8888) y lo entrega vía `retro_video_refresh_t`.
5. **osd_libretro_update_audio()** — Convierte el audio generado por MAME (frecuencia nativa del device, típicamente 44100 o 48000 Hz) al formato de 16-bit estéreo y lo entrega vía `retro_audio_sample_batch_t`.
6. **osd_libretro_handle_input()** — Traduce los eventos de RetroArch (teclado, mando, ratón) a los dispositivos de entrada de MAME (joypad, mouse, teclado FM Towns).

### Diferencias con MAME vanilla

| Aspecto | MAME vanilla | Este core |
|---------|-------------|-----------|
| UI | SDL/Win32 + in-game OSD | Ninguna. Sin menú, sin debugger |
| Plugins | Lua plugins completos | No incluidos |
| Audio | Múltiples backends (SDL, WASAPI, ALSA) | Solo entrega a RetroArch |
| Video | Ventana nativa con draw() | Solo entrega buffer a RetroArch |
| Input | Teclado/ratón/joystick nativos | Traducido desde RetroArch |
| Software lists | Carga directa de CHD/ZIP | Carga de formatos sueltos |

---

## Build system

### GENIE

El build de las bibliotecas MAME se genera con [GENIE](https://github.com/bkaradzic/genie). Los scripts están en `scripts/`:

- `scripts/genie.lua` — Punto de entrada principal
- `scripts/src/3rdparty.lua` — Dependencias externas
- `scripts/src/mess.lua` — Drivers de MESS (computadoras)
- `scripts/src/osd.lua` — Módulos OSD

Para regenerar los makefiles:
```bash
# Windows
genie.exe --gcc=gcc --vs=2022 vs2022

# Linux
genie --gcc=gcc gmake
```

### Compilación paso a paso

#### Windows x64 (MSYS2 UCRT64)

```bash
./build64.bat
```

Esto ejecuta:
1. `make SUBTARGET=fmtowns -j$(nproc)` — Compila todas las `src/` como `.a`
2. `make -f libretro/Makefile.libretro -j$(nproc)` — Linkea contra las `.a` → `fmtowns_libretro.dll`

#### Windows x86 (MSYS2 MINGW32)

```bash
./build32.bat
```

#### Linux nativo

```bash
make SUBTARGET=fmtowns -j$(nproc)
./build_linux.sh     # Compila wrapper y linkea
```

#### Linux cross desde Windows (MSYS2 → VM Linux vía SSH)

```bash
./buildlinux.bat
```

### Requisitos por plataforma

| Plataforma | Entorno | Comando |
|-----------|---------|---------|
| Windows x64 | MSYS2 UCRT64 + GCC 13+ | `build64.bat` |
| Windows x86 | MSYS2 MINGW32 + GCC | `build32.bat` |
| Linux | GCC/Clang C++20 + pkg-config + SDL2 + X11 | `make SUBTARGET=fmtowns && ./build_linux.sh` |
| Linux cross | MSYS2 + SSH + VM Linux | `buildlinux.bat` |

### Dependencias de sistema (Linux)

```bash
sudo apt install build-essential pkg-config libsdl2-dev libx11-dev libasound2-dev
```

---

## OSD layer (libretro/osd_libretro.cpp)

Esta capa implementa la interfaz `osd_interface` de MAME con el mínimo necesario para que el core funcione:

### Video

- `osd_libretro.cpp` implementa `renderer_libretro` que hereda de `osd_renderer`.
- El buffer de video se obtiene llamando `machine().render().get_buffer()`, se copia a un `uint32_t*` o `uint16_t*` (según el pixel format solicitado por RetroArch) y se entrega vía `retro_video_refresh_t`.
- Soporta cambio dinámico de resolución: si el juego cambia de modo (ej. 320×240 ↔ 640×480), el core renegocia con RetroArch vía `retro_set_geometry()`.

### Audio

- `sound_manager` simple que captura el stream de audio de MAME y lo acumula en un ring buffer.
- En `retro_run()`, se drena el ring buffer y se entrega a RetroArch en batches de hasta 1024 samples.
- Frecuencia de sampleo: 44100 Hz por defecto (configurable vía core option).

### Input

El mapeo se realiza en `osd_libretro_handle_input()`:

| Dispositivo FM Towns | Entrada RetroArch |
|---------------------|-------------------|
| Teclado (MATRA) | `retro_keyboard_callback` |
| Joypad 1 | Puerto de mando 1 (retro_input_poll) |
| Joypad 2 | Puerto de mando 2 |
| Mouse | Ratón vía `retro_mouse_*` + `retro_lightgun_*` |
| Towns Pad | Mapeo especial desde gamepad digital |

El `fmtowns_pad1` y `fmtowns_pad2` core options permiten seleccionar entre:
- `none` — Desconectado
- `joypad` — Gamepad genérico (2 botones)
- `mouse` — Ratón
- `towns_pad` — Towns Pad con botones específicos

### NVRAM y ahorro de estado

- La NVRAM se guarda/restaura automáticamente en `retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)`.
- Savestates: **no disponibles en v1**. El plan arquitectónico (fases 4-5 en `Libretro.txt`) prevé su implementación serializando `running_machine` completo.

---

## Cómo agregar un nuevo driver FM Towns

1. **Crear el driver** en `src/mess/drivers/` siguiendo el patrón de `towns.cpp`.
2. **Registrar en software lists** en `hash/` (ej. `cdtowns.xml` para CD, `floptowns.xml` para diskettes).
3. **Incluir device** en `src/mess/mess.lua` listándolo en `files()`.
4. **Recompilar** — el GENIE incluirá automáticamente el nuevo `.o`.

### Convenciones de código

- Siguen las convenciones de MAME: clases `device_t`, `machine_config` con `MCFG_*`, y drivers con `GAME()`, `CONS()`, `COMP()`.
- Usar `LOG()` para logging condicional (definir `#define VERBOSE 1` en el driver).
- No usar características de C++ que MAME no soporte (ej. excepciones, RTTI).

---

## Formato de imágenes soportado

| Tipo | Formatos | Notas |
|------|----------|-------|
| CD | chd, cue, toc, nrg, gdi, iso, cdr, bin | CHD recomendado por rendimiento |
| Floppy | mfi, dfi, mfm, td0, imd, 86f, d77, d88, 1dd, cqm, cqi, dsk | D88 y D77 son los más comunes |
| HDD | hdm, hd, hdv, 2mg, hdi, h0 | Imágenes de disco duro |
| Multi | m3u | Lista de pistas para multi-CD |

**Nota**: No se usan archivos ZIP. Los formatos deben estar extraídos.

---

## BIOS

Colocar en `system/fmtowns/`:

| Archivo | MD5 (referencia) | Propósito |
|---------|-----------------|-----------|
| `fmt_dos_a.rom` | — | DOS-C ROM (arranque) |
| `fmt_dic.rom` | — | Diccionario de kanji |
| `fmt_fnt.rom` | — | Fuente de caracteres |
| `fmt_sys_a.rom` | — | ROM de sistema principal |
| `mytownsux.rom` | — | BIOS alternativa ("My Town's UX") |

Ver `README_BIOS.md` para detalles de版本es y ubicaciones alternativas.

---

## Debugging

### Logging

Para habilitar logging del core:

```c
// En fmtowns_libretro.cpp o el archivo relevante:
#define VERBOSE 1
```

O dinámicamente en RetroArch:
```
Log Level: Info (en RetroArch → Ajustes → Registro)
```

Los mensajes de `osd_printf_*()` se redirigen a `retro_log_callback`.

### Tools de análisis

```bash
# Listar todos los drivers FM Towns disponibles
grep -r "GAME(" src/mess/drivers/towns.cpp

# Verificar dependencias de un driver
grep -r "towns" src/mess/mess.lua

# Depurar flujo de arranque con GDB (Linux)
gdb -ex "set pagination off" -ex "run" --args retroarch -L fmtowns_libretro.so
```

### Problemas comunes

| Síntoma | Causa probable | Solución |
|---------|---------------|----------|
| "No driver found" | Faltan ROMs o están mal nombradas | Verificar `system/fmtowns/` y `README_BIOS.md` |
| Video negro sin audio | Core option `fmtowns_model` incorrecta | Cambiar a `marty` o `townsux` |
| Crash al cargar CHD | CHD corrupto o de versión incorrecta | Reextraer con `chdman createcd` |
| Audio entrecortado | Frecuencia de sampleo muy alta | Reducir a 44100 Hz |
| Input no responde | Mapeo incorrecto en `fmtowns_pad1/2` | Cambiar a `joypad` o `mouse` |

---

## Rendimiento

### Perfilado

Para identificar cuellos de botella, compilar con profiling:

```bash
# Linux
make SUBTARGET=fmtowns OPTIMIZE=3 PROFILE=1
make -f libretro/Makefile.libretro OPTIMIZE=3 PROFILE=1
```

### Factores que afectan el rendimiento

1. **Modelo de CPU**: El i386 de FM Towns es más lento de emular que el i386SX del Marty.
2. **CD-ROM**: La emulación del CD-ROM (SCSI CDC) es cara; usar CHD ayuda.
3. **Video**: Modos high-color (16k/16M colores) requieren más conversión de pixel format.
4. **DSP**: Algunos juegos usan el DSP de FM Towns para audio digital; verificar si está habilitado.

---

## Core options

| Opción | Valores | Default | Descripción |
|--------|---------|---------|-------------|
| `fmtowns_model` | `towns`, `marty`, `townsux` | `towns` | Modelo de FM Towns a emular |
| `fmtowns_pad1` | `none`, `joypad`, `mouse`, `towns_pad` | `joypad` | Dispositivo en puerto 1 |
| `fmtowns_pad2` | `none`, `joypad`, `mouse`, `towns_pad` | `none` | Dispositivo en puerto 2 |
| `fmtowns_mouse` | `disabled`, `enabled` | `disabled` | Emulación de ratón por defecto |

---

## Plan arquitectónico (fases 0-5)

Ver `docs/Libretro.txt` para el plan completo. Resumen:

| Fase | Estado | Descripción |
|------|--------|-------------|
| 0 | ✅ Completa | Port a libretro + build funcional |
| 1 | ✅ Completa | Software lists + carga de imágenes |
| 2 | ✅ Completa | Core options + mapeo de input |
| 3 | 🔄 En progreso | Optimización de rendimiento |
| 4 | ⏳ Pendiente | Savestates |
| 5 | ⏳ Pendiente | Netplay |

---

## Notas adicionales

- **No es standalone**: requiere RetroArch para ejecutarse.
- **No usa ZIPs**: todos los formatos deben estar en archivos sueltos.
- **Savestates**: no disponibles en v1. Ver plan fases 4-5.
- **Test de regresión**: en `regtests/` hay scripts para validar que los juegos conocidos booteen correctamente tras cambios.
- **Contribuciones**: seguir el estilo de MAME. Hacer PR contra `main`. Incluir descripción del cambio y, si aplica, captura de pantalla del juego funcionando.
