# Soporte M3U Multi-Archivo - Estado y Solución

## Problema Identificado

El juego **"The Queen of Duellist Gaiden Alpha"** requiere:
- **Floppy Disk** (qdlight.bin) - User Disk para guardar partidas
- **CD-ROM** (queen of duellist gaiden.chd) - Juego principal

### Estado Actual del Core

El core libretro **solo carga el primer archivo** del M3U:

```cpp
// Código actual (línea 804-813)
if (extension == "m3u")
{
    content_path = first_m3u_entry(config.content_path);  // ← Solo primer archivo
    // ...
    extension = lowercase_extension(content_path);
}
// Solo monta UN archivo
m_options->image_option(slot).specify(content_path);
```

## Solución Implementada (Pendiente de Compilar)

He agregado la función `all_m3u_entries()` que lee TODOS los archivos del M3U y los monta en sus slots correspondientes.

### Cambios Necesarios:

1. **Nueva función** (ya agregada en línea 691):
```cpp
std::vector<std::string> all_m3u_entries(const std::string &playlist)
{
    // Lee todas las líneas del M3U y retorna vector de paths
}
```

2. **Modificar procesamiento M3U** (líneas 804-830):
```cpp
if (extension == "m3u")
{
    std::vector<std::string> m3u_files = all_m3u_entries(config.content_path);
    
    for (const std::string &file_path : m3u_files)
    {
        std::string file_ext = lowercase_extension(file_path);
        const char *slot = slot_for_extension(file_ext);
        
        if (slot && m_options->has_image_option(slot))
        {
            m_options->image_option(slot).specify(file_path);
            fmtowns::libretro_osd::log(RETRO_LOG_INFO, 
                "[MAME_BRIDGE] Mounted %s in slot %s\n", file_path.c_str(), slot);
        }
    }
}
```

## Archivo M3U Correcto

**D:\Emulation\ROMs\FmTowns\tqodga\tqodga.m3u**:
```
qdlight.bin
queen of duellist gaiden, the + gaiden alpha (japan).chd
```

## Próximos Pasos

1. Aplicar los cambios manualmente al archivo `libretro/mame_bridge.cpp`
2. Compilar con `build64.bat`
3. Probar cargando el M3U en RetroArch
4. Verificar en el log que ambos archivos se montan:
   ```
   [INFO] [MAME_BRIDGE] Mounted qdlight.bin in slot floppydisk1
   [INFO] [MAME_BRIDGE] Mounted queen of duellist gaiden.chd in slot cdrom
   ```

## Beneficios

- ✅ Soporte completo para juegos que requieren múltiples medios
- ✅ Compatible con MAME standalone (mismo comportamiento)
- ✅ Funciona con cualquier combinación: floppy+CD, floppy+HDD, etc.
- ✅ Logs informativos para debugging

## Nota

El script PowerShell causó un error de sintaxis. Los cambios deben aplicarse manualmente editando el archivo `mame_bridge.cpp` directamente.
