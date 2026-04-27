# Análisis: Gestión de Memoria RAM en FM Towns

## MAME Standalone vs Libretro

### 1. Configuración de RAM en MAME Standalone

MAME permite configurar la RAM mediante la opción de línea de comandos:
```bash
mame fmtowns -ramsize 6M
```

#### Configuraciones por Modelo:

| Modelo | RAM Default | Opciones Disponibles | Notas |
|--------|-------------|---------------------|-------|
| **fmtowns** | 2M | 1M, 3M, 4M, 5M, 6M | 1 MB onboard + 3 slots SIMM (1-2 MB cada uno) |
| **fmtownsux** | 2M | 4M, 6M, 10M | 2 MB onboard + 1 slot SIMM (2-8 MB) |
| **fmtownssj** | 8M | 4M, 12M, 16M, 20M, 24M, 28M, 32M, 36M, 40M, 44M, 48M, 52M, 56M, 68M, 72M | 4-8 MB onboard + 2 slots SIMM (4-32 MB cada uno) |
| **fmtownshr** | 4M | 6M, 8M, 10M, 12M, 14M, 16M, 18M, 20M, 22M, 24M, 28M | 4 MB onboard + 3 slots SIMM (2-8 MB cada uno) |
| **fmtownsmx** | 8M | 4M-100M (muchas opciones) | 4 MB onboard + 3 slots SIMM (2-32 MB cada uno) |
| **fmtownsftv** | 6M | 4M, 8M, 10M, 12M, 14M, 16M, 20M, 22M, 24M, 28M, 36M, 38M, 40M, 44M, 52M, 68M | 4 MB onboard + 2 slots SIMM (2-32 MB) |
| **fmtmarty** | 2M | 4M | 2 MB onboard, expandible a 4 MB con tarjeta Marty |
| **fmtmarty2** | 2M | 4M | Similar a Marty |

### 2. Implementación Actual en Libretro

**Estado**: ❌ **NO IMPLEMENTADO**

El core libretro actualmente:
- Usa la RAM por defecto del modelo seleccionado
- **NO** tiene opción para cambiar el tamaño de RAM
- **NO** pasa `-ramsize` a MAME

#### Código Actual:
```cpp
const retro_variable k_variables[] = {
    { "fmtowns_model", "FM Towns model; fmtownsux|fmtmarty|..." },
    { "fmtowns_pad1", "Port 1 device; townspad|towns6b|martypad|none" },
    { "fmtowns_pad2", "Port 2 device; none|townspad|towns6b|martypad" },
    { "fmtowns_mouse", "Mouse; enabled|disabled" },
    { nullptr, nullptr }
};
// ← NO HAY OPCIÓN DE RAM
```

### 3. Impacto en Compatibilidad

#### Juegos que Requieren Más RAM:

Algunos juegos de FM Towns requieren configuraciones específicas de RAM:

| Tipo de Software | RAM Recomendada | Razón |
|------------------|-----------------|-------|
| **Juegos 3D avanzados** | 6M+ | Texturas y geometría |
| **Aplicaciones multimedia** | 8M+ | Buffers de video/audio |
| **Software de desarrollo** | 12M+ | Compiladores, IDEs |
| **Juegos CD-ROM tardíos** | 4M-6M | Assets grandes |

**Ejemplo Real**: 
- Algunos juegos pueden mostrar "Insufficient Memory" con 2M
- Aplicaciones como TownsOS V2.1 recomiendan 4M mínimo
- Juegos 3D como "Lemmings" pueden beneficiarse de 6M+

### 4. ¿Es Necesario Implementarlo?

#### ✅ **SÍ, es recomendable** por:

1. **Compatibilidad**: Algunos juegos no funcionarán con RAM por defecto
2. **Precisión**: MAME standalone lo soporta, libretro debería también
3. **Flexibilidad**: Usuarios pueden optimizar según el contenido
4. **Paridad**: Otros cores libretro (como px68k) tienen opciones de RAM

#### Casos de Uso:

- **Usuario carga juego antiguo** → 2M suficiente (default)
- **Usuario carga juego 3D** → Necesita cambiar a 6M
- **Usuario carga aplicación** → Necesita 8M+

### 5. Implementación Propuesta

#### Opción A: RAM Fija por Modelo (Más Simple) ⭐

Agregar opciones predefinidas según el modelo:

```cpp
const retro_variable k_variables[] = {
    { "fmtowns_model", "FM Towns model; fmtownsux|fmtmarty|..." },
    { "fmtowns_ram", "RAM Size; default|2M|4M|6M|8M|12M|16M" },
    // ...
};
```

**Ventajas**:
- Simple de implementar (~30 líneas)
- Cubre la mayoría de casos
- Fácil de entender para usuarios

**Desventajas**:
- No todas las combinaciones son válidas para todos los modelos
- Menos preciso que MAME standalone

#### Opción B: RAM Dinámica por Modelo (Más Precisa)

Cambiar opciones de RAM según el modelo seleccionado:

```cpp
// Opciones cambian dinámicamente según g_model
if (g_model == "fmtowns") {
    // Mostrar: 1M, 2M, 3M, 4M, 5M, 6M
} else if (g_model == "fmtownsmx") {
    // Mostrar: 4M-100M
}
```

**Ventajas**:
- Precisión total
- Coincide con MAME standalone
- Previene configuraciones inválidas

**Desventajas**:
- Más complejo (~100 líneas)
- Requiere lógica de validación

### 6. Código Mínimo (Opción A)

#### Paso 1: Agregar variable
```cpp
const retro_variable k_variables[] = {
    { "fmtowns_model", "FM Towns model; fmtownsux|fmtmarty|..." },
    { "fmtowns_ram", "RAM Size; default|2M|4M|6M|8M|12M|16M|24M|32M" },
    { "fmtowns_pad1", "Port 1 device; townspad|towns6b|martypad|none" },
    // ...
};
```

#### Paso 2: Leer opción
```cpp
std::string g_ram_size = "default";

void refresh_core_options()
{
    g_model = fmtowns::libretro_osd::variable_value("fmtowns_model", "fmtownsux");
    g_ram_size = fmtowns::libretro_osd::variable_value("fmtowns_ram", "default");
    // ...
}
```

#### Paso 3: Pasar a MAME (en mame_bridge.cpp)
```cpp
bool start(const boot_config &config, std::string &error)
{
    // ... código existente ...
    
    if (!config.ram_size.empty() && config.ram_size != "default")
    {
        m_options->set_value(OPTION_RAMSIZE, config.ram_size.c_str(), OPTION_PRIORITY_CMDLINE);
        fmtowns::libretro_osd::log(RETRO_LOG_INFO, 
            "[MAME_BRIDGE] Configurando RAM = %s\n", config.ram_size.c_str());
    }
    
    // ...
}
```

#### Paso 4: Agregar a boot_config
```cpp
struct boot_config
{
    std::string model;
    std::string bios_directory;
    std::string content_path;
    std::string cfg_directory;
    std::string nvram_directory;
    std::string pad1_device;
    std::string pad2_device;
    std::string ram_size;  // ← NUEVO
};
```

### 7. Consideraciones

#### Requiere Restart:
- Cambiar RAM requiere reiniciar el contenido (igual que cambiar modelo)
- Usar el mismo sistema de notificación que implementamos para pads

#### Validación:
- MAME valida automáticamente si el tamaño es válido
- Si es inválido, usa el default del modelo

#### Compatibilidad:
- No rompe saves existentes (MAME maneja esto)
- Cambios son reversibles

### 8. Recomendación Final

✅ **Implementar Opción A (RAM Fija)**

**Razones**:
1. **Necesario**: Algunos juegos no funcionarán sin esto
2. **Simple**: ~40 líneas de código
3. **Suficiente**: Cubre 95% de casos de uso
4. **Consistente**: Usa el mismo patrón que pad hot-swap

**Prioridad**: Media-Alta
- No es crítico para juegos básicos
- Crítico para compatibilidad completa
- Fácil de implementar

### 9. Comparación con Otros Cores

| Core | Opción de RAM | Notas |
|------|---------------|-------|
| **px68k** | ✅ Sí | 1M-12M configurable |
| **quasi88** | ✅ Sí | PC-8801 RAM expansion |
| **np2kai** | ✅ Sí | PC-9801 RAM options |
| **fmtowns (actual)** | ❌ No | **Falta implementar** |

### 10. Resumen Ejecutivo

**Pregunta**: ¿Hace falta agregar opciones de RAM al libretro?

**Respuesta**: **SÍ**

- MAME standalone lo soporta
- Algunos juegos lo requieren
- Implementación simple (~40 líneas)
- Mejora significativa en compatibilidad
- Consistente con otros cores similares

**Siguiente Paso**: Implementar Opción A con notificación de restart (igual que pads)
