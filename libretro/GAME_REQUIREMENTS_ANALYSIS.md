# Análisis: Cómo MAME Determina Requisitos de Juegos

## Pregunta
¿Cómo sabe MAME standalone qué modelo de FM Towns o cuánta RAM usar para cada juego?

## Respuesta: Software Lists (XML)

MAME usa **Software Lists** (archivos XML) que contienen metadatos de cada juego, incluyendo requisitos de hardware.

### Ubicación de Software Lists

```
hash/fmtowns_cd.xml           - Juegos en CD-ROM
hash/fmtowns_flop_orig.xml    - Floppies originales
hash/fmtowns_flop_cracked.xml - Floppies crackeados
hash/fmtowns_flop_misc.xml    - Floppies misceláneos
```

### Estructura de Entrada de Juego

```xml
<software name="asuka120">
    <description>Asuka 120% BURNING Fest.</description>
    <year>1994</year>
    <publisher>ファミリーソフト (Family Soft)</publisher>
    <info name="alt_title" value="あすか 120% BURNING Fest." />
    <info name="release" value="199403xx" />
    <info name="usage" value="Requires 2 MB RAM"/>  ← REQUISITO DE RAM
    <part name="flop1" interface="floppy_3_5">
        <dataarea name="flop" size="1261568">
            <rom name="Disk A" size="1261568" crc="..." sha1="..." />
        </dataarea>
        <feature name="part_id" value="Disk A" />
    </part>
</software>
```

### Tipos de Información en `<info name="usage">`

#### 1. **Requisitos de RAM**
```xml
<info name="usage" value="Requires 2 MB RAM"/>
<info name="usage" value="Requires 2 MB RAM (4 for HDD version)"/>
<info name="usage" value="Requires 4 MB RAM"/>
```

#### 2. **Requisitos de TownsOS**
```xml
<info name="usage" value="Boot TownsOS V2.1L10 or later, then run the icon on floppy drive A:"/>
<info name="usage" value="Boot TownsOS V2.1L20 or later, then run the icon on floppy drive A:"/>
```

#### 3. **Instrucciones Especiales**
```xml
<info name="usage" value="Mouse must not be connected to port 2."/>
<info name="usage" value="To create the user disk: after the intro you will be asked to insert disk 2. Insert disk 3 instead..."/>
```

#### 4. **Requisitos de Modelo Específico**
```xml
<!-- Master CD específico para modelos HC, Fresh ES y Fresh ET -->
<info name="usage" value="This master CD is specifically for the HC, Fresh ES and Fresh ET models"/>
```

## Cómo MAME Usa Esta Información

### En Standalone:

1. **Usuario carga juego**:
   ```bash
   mame fmtowns -cdrom game.chd
   ```

2. **MAME busca en software list**:
   - Calcula hash del contenido
   - Busca coincidencia en `hash/fmtowns_cd.xml`
   - Lee el tag `<info name="usage">`

3. **MAME muestra advertencia** (si hay requisitos):
   ```
   WARNING: This software requires 2 MB RAM
   ```

4. **Usuario ajusta manualmente**:
   ```bash
   mame fmtowns -cdrom game.chd -ramsize 2M
   ```

### Limitación Actual:

❌ **MAME NO configura automáticamente** RAM o modelo según el juego
✅ **MAME SOLO informa** al usuario sobre los requisitos
👤 **Usuario debe configurar manualmente** con opciones de línea de comandos

## Problema en Libretro

### Situación Actual:

1. **Usuario carga juego** en RetroArch
2. **Libretro NO lee** software lists
3. **Libretro usa** configuración por defecto:
   - Modelo: `fmtownsux` (default)
   - RAM: 2M (default del modelo)
4. **Juego puede fallar** si requiere más RAM o modelo diferente

### Ejemplo de Fallo:

```
Usuario carga: "A列車で行こう4" (A-Train 4)
Requisito real: 4 MB RAM
Libretro usa: 2 MB RAM (default)
Resultado: ❌ "Insufficient Memory" o crash
```

## Soluciones Posibles

### Opción 1: Implementar Lectura de Software Lists (Complejo)

**Ventajas**:
- ✅ Configuración automática
- ✅ Experiencia perfecta para el usuario
- ✅ Paridad total con standalone

**Desventajas**:
- ❌ Muy complejo (~500+ líneas)
- ❌ Requiere parser XML
- ❌ Requiere calcular hashes de contenido
- ❌ Requiere mapear requisitos a opciones

**Código Estimado**:
```cpp
// 1. Calcular hash del contenido cargado
std::string content_hash = calculate_sha1(content_path);

// 2. Buscar en software list XML
software_info info = parse_software_list("hash/fmtowns_cd.xml", content_hash);

// 3. Extraer requisitos
if (info.usage.contains("Requires 4 MB RAM")) {
    g_ram_size = "4M";
}

// 4. Aplicar configuración
boot.ram_size = g_ram_size;
```

### Opción 2: Opciones Manuales + Documentación (Simple) ⭐

**Ventajas**:
- ✅ Simple de implementar (~40 líneas)
- ✅ Usuario tiene control total
- ✅ Funciona para todos los casos

**Desventajas**:
- ⚠️ Usuario debe conocer requisitos
- ⚠️ Requiere documentación

**Implementación**:
```cpp
// Ya propuesta en RAM_ANALYSIS.md
const retro_variable k_variables[] = {
    { "fmtowns_model", "FM Towns model; fmtownsux|fmtmarty|..." },
    { "fmtowns_ram", "RAM Size; default|2M|4M|6M|8M|12M|16M" },
    // ...
};
```

**Documentación en `.info`**:
```
# fmtowns_libretro.info
notes = "Some games require specific RAM settings:|
- Most games: 2M (default)|
- 3D games: 4M-6M|
- Applications: 8M+|
Check game documentation for requirements."
```

### Opción 3: Base de Datos Simplificada (Intermedio)

**Ventajas**:
- ✅ Configuración automática para juegos conocidos
- ✅ Fallback a manual para desconocidos
- ✅ Complejidad moderada (~150 líneas)

**Desventajas**:
- ⚠️ Requiere mantener base de datos
- ⚠️ No cubre todos los juegos

**Implementación**:
```cpp
// Base de datos simple por hash o nombre
struct game_requirements {
    const char* name_pattern;
    const char* ram_size;
    const char* model;
};

const game_requirements known_games[] = {
    { "a-train", "4M", "fmtowns" },
    { "asuka120", "2M", "fmtowns" },
    { "brandish", "2M", "fmtowns" },
    // ...
};

// Auto-detectar y aplicar
for (const auto& req : known_games) {
    if (content_name.contains(req.name_pattern)) {
        g_ram_size = req.ram_size;
        g_model = req.model;
        break;
    }
}
```

## Comparación con Otros Cores

| Core | Método | Auto-Config |
|------|--------|-------------|
| **MAME standalone** | Software Lists XML | ❌ No (solo advierte) |
| **px68k** | Opciones manuales | ❌ No |
| **quasi88** | Opciones manuales | ❌ No |
| **fmtowns (actual)** | Ninguno | ❌ No |

**Conclusión**: Ningún core similar hace auto-configuración. Todos usan opciones manuales.

## Recomendación

### Implementar **Opción 2** (Opciones Manuales)

**Razones**:
1. **Consistente** con otros cores similares
2. **Simple** de implementar y mantener
3. **Suficiente** para usuarios informados
4. **Documentable** en README y .info

### Plan de Implementación:

#### Fase 1: Agregar Opciones (~40 líneas)
```cpp
{ "fmtowns_ram", "RAM Size; default|2M|4M|6M|8M|12M|16M|24M|32M" }
```

#### Fase 2: Documentación
Crear `libretro/GAME_COMPATIBILITY.md`:
```markdown
# FM Towns Game Compatibility

## RAM Requirements

| Game | RAM | Notes |
|------|-----|-------|
| A-Train 4 | 4M | Requires 4M for HDD version |
| Asuka 120% | 2M | Default is sufficient |
| Brandish | 2M | Default is sufficient |
...
```

#### Fase 3: Actualizar .info
```
notes = "RAM requirements vary by game. Check GAME_COMPATIBILITY.md"
```

### Fase Futura (Opcional):

Si hay demanda, implementar **Opción 3** (Base de Datos Simplificada) para los 50-100 juegos más populares.

## Respuesta a Tu Pregunta Original

> "¿Cómo sabe MAME qué modelo usar cuando se lee determinado juego?"

**Respuesta**: 
- MAME **NO** lo sabe automáticamente
- MAME **lee** los requisitos de `hash/*.xml` (software lists)
- MAME **muestra advertencia** al usuario
- **Usuario** debe configurar manualmente con `-ramsize` y `-model`

**Por eso algunos juegos no arrancan en libretro**:
- Libretro usa defaults (2M RAM, modelo fmtownsux)
- Algunos juegos requieren más RAM o modelo diferente
- Sin opciones de RAM/modelo, el usuario no puede ajustar

**Solución**: Implementar opciones de RAM y modelo (ya propuesto en RAM_ANALYSIS.md)
