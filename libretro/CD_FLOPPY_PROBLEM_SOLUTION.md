# Problema: Juegos que Requieren CD + Floppy Simultáneamente

**Fecha**: 2026-04-27  
**Ejemplo**: The Queen of Duellist Gaiden Alpha (tqodga)

---

## El Problema en MAME Standalone

### Cómo Funciona MAME Standalone

MAME standalone utiliza **software lists XML** para determinar automáticamente qué medios necesita cada juego:

**Archivo**: `hash/fmtowns_cd.xml`

```xml
<software name="tqodga">
    <description>Queen of Duellist Gaiden, The + Gaiden Alpha (Japan)</description>
    <year>1995</year>
    <publisher>Cocktail Soft</publisher>
    
    <!-- CD-ROM principal -->
    <part name="cdrom" interface="fmtowns_cd">
        <diskarea name="cdrom">
            <disk name="queen of duellist gaiden, the + gaiden alpha (japan)" sha1="..."/>
        </diskarea>
    </part>
    
    <!-- Floppy requerido -->
    <part name="flop1" interface="fmtowns_flop_orig">
        <dataarea name="flop" size="1261568">
            <rom name="qdlight.bin" size="1261568" crc="..." sha1="..."/>
        </dataarea>
    </part>
</software>
```

### Proceso Automático de MAME

Cuando ejecutas:
```bash
mame fmtowns -cdrom tqodga
```

MAME:
1. **Lee el XML** y detecta que `tqodga` tiene 2 partes (`cdrom` + `flop1`)
2. **Monta automáticamente**:
   - CD en slot `cdrom`
   - Floppy en slot `floppydisk1`
3. **Busca los archivos** en los rompath configurados
4. **Inicia el juego** con ambos medios disponibles

---

## El Problema en el Core Libretro

### Limitaciones Actuales

El core libretro **NO tiene acceso a los software lists XML** porque:

1. Los archivos XML están en `hash/` pero no se incluyen en el core compilado
2. No hay infraestructura para parsear XML en runtime
3. La API libretro espera que el frontend (RetroArch) pase **un solo archivo** como contenido

### Comportamiento Actual

Cuando cargas un juego en RetroArch:

```cpp
// libretro/mame_bridge.cpp (línea ~800)
if (extension == "chd" || extension == "cue" || extension == "iso") {
    m_options->image_option("cdrom").specify(config.content_path);
}
```

**Resultado**: Solo se monta el CD, el floppy nunca se carga → el juego no arranca.

---

## La Solución Propuesta: M3U Multi-File

### Concepto

Usar archivos **M3U playlist** como "contenedor" que lista todos los medios necesarios:

**Archivo**: `tqodga.m3u`
```
qdlight.bin
queen of duellist gaiden, the + gaiden alpha (japan).chd
```

### Ventajas

1. **Formato estándar**: M3U ya se usa en otros cores (Beetle PSX, Genesis Plus GX)
2. **Sin dependencias XML**: No requiere parsear software lists
3. **Control del usuario**: El usuario crea el M3U con los archivos necesarios
4. **Retrocompatible**: Archivos individuales (.chd, .bin) siguen funcionando

---

## Implementación Técnica

### 1. Función para Leer Todos los Archivos del M3U

```cpp
// libretro/mame_bridge.cpp (después de línea 690)
std::vector<std::string> all_m3u_entries(const std::string &playlist)
{
	std::vector<std::string> entries;
	std::ifstream file(playlist);
	if (!file.is_open())
		return entries;

	std::string base_dir = playlist.substr(0, playlist.find_last_of("/\\") + 1);
	std::string line;
	
	while (std::getline(file, line)) {
		// Eliminar espacios y comentarios
		size_t comment = line.find('#');
		if (comment != std::string::npos)
			line = line.substr(0, comment);
		
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
		
		if (!line.empty()) {
			// Convertir ruta relativa a absoluta
			std::string full_path = (line[0] == '/' || line[0] == '\\' || 
			                         (line.length() > 1 && line[1] == ':'))
			                        ? line : base_dir + line;
			entries.push_back(full_path);
		}
	}
	return entries;
}
```

### 2. Modificar el Procesamiento de M3U

**Ubicación**: `libretro/mame_bridge.cpp` líneas 804-830

**Cambio**: Reemplazar el bloque que solo carga el primer archivo:

```cpp
// ANTES (solo primer archivo)
if (extension == "m3u") {
	content_path = first_m3u_entry(config.content_path);
	extension = content_path.substr(content_path.find_last_of('.') + 1);
}

// DESPUÉS (todos los archivos)
if (extension == "m3u") {
	std::vector<std::string> m3u_files = all_m3u_entries(config.content_path);
	
	for (const std::string &file_path : m3u_files) {
		std::string ext = file_path.substr(file_path.find_last_of('.') + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		
		// Determinar slot según extensión
		std::string slot;
		if (ext == "bin" || ext == "d77" || ext == "d88") {
			slot = "floppydisk1";
		} else if (ext == "chd" || ext == "cue" || ext == "iso") {
			slot = "cdrom";
		} else if (ext == "hd" || ext == "hdi" || ext == "hdv") {
			slot = "harddisk1";
		}
		
		if (!slot.empty()) {
			m_options->image_option(slot).specify(file_path);
			LOG("[MAME_BRIDGE] Mounted %s in slot %s\n", file_path.c_str(), slot.c_str());
		}
	}
	return true; // Salir después de procesar M3U
}
```

### 3. Mapeo de Extensiones a Slots

| Extensión | Slot MAME | Tipo de Medio |
|-----------|-----------|---------------|
| `.bin`, `.d77`, `.d88` | `floppydisk1` | Floppy Disk |
| `.chd`, `.cue`, `.iso` | `cdrom` | CD-ROM |
| `.hd`, `.hdi`, `.hdv` | `harddisk1` | Hard Disk |

---

## Comparación: Standalone vs Libretro

| Aspecto | MAME Standalone | Core Libretro (Propuesto) |
|---------|-----------------|---------------------------|
| **Fuente de info** | Software lists XML | M3U playlist |
| **Detección** | Automática | Manual (usuario crea M3U) |
| **Montaje** | Automático | Automático (desde M3U) |
| **Flexibilidad** | Limitada a XML | Total (cualquier combinación) |
| **Mantenimiento** | Requiere actualizar XML | No requiere cambios en core |

---

## Ejemplo de Uso

### Paso 1: Crear M3U

**Archivo**: `D:\Emulation\ROMs\FmTowns\tqodga\tqodga.m3u`
```
qdlight.bin
queen of duellist gaiden, the + gaiden alpha (japan).chd
```

### Paso 2: Cargar en RetroArch

```
RetroArch → Load Content → tqodga.m3u
```

### Paso 3: Verificar en Log

```
[MAME_BRIDGE] Mounted D:\...\qdlight.bin in slot floppydisk1
[MAME_BRIDGE] Mounted D:\...\queen of duellist gaiden, the + gaiden alpha (japan).chd in slot cdrom
```

---

## Ventajas de Esta Solución

1. **Simplicidad**: ~30 líneas de código
2. **Sin dependencias**: No requiere libxml2 ni parsers complejos
3. **Estándar libretro**: Otros cores usan M3U de la misma forma
4. **Documentable**: Fácil explicar a usuarios cómo crear M3U
5. **Extensible**: Soporta CD+Floppy, CD+HDD, múltiples floppies, etc.

---

## Casos de Uso Adicionales

### Juego con Múltiples Floppies
```m3u
disk1.bin
disk2.bin
disk3.bin
```

### Juego con CD + HDD
```m3u
game.chd
userdata.hdi
```

### Comentarios en M3U
```m3u
# User disk (required)
qdlight.bin
# Game CD
queen of duellist gaiden, the + gaiden alpha (japan).chd
```

---

## Estado de Implementación

- ✅ Función `all_m3u_entries()` creada
- ✅ M3U de prueba creado (`tqodga.m3u`)
- ⏳ Modificación del procesamiento M3U (pendiente compilación)
- ⏳ Testing con juego real

---

## Próximos Pasos

1. Completar modificación en `mame_bridge.cpp` líneas 804-830
2. Compilar con `build64.bat`
3. Probar con `tqodga.m3u` en RetroArch
4. Documentar en README para usuarios
5. Crear lista de juegos que requieren M3U multi-file
