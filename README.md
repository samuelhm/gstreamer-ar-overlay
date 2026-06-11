# AR Overlay — Visualizador Multimedia Reactivo

Reproductor de video con efectos visuales sobre la imagen que reaccionan en tiempo real a las frecuencias del audio. El video se divide en 16 columnas verticales; cada columna se "ilumina" proporcionalmente a la amplitud de su banda de frecuencia correspondiente, creando un efecto donde el video baila al ritmo de la música.

<video src="assets/2026-06-11%2014-49-30.mp4" controls width="100%"></video>

## Stack Tecnológico

| Capa | Tecnología |
|---|---|
| Pipeline multimedia | GStreamer 1.x (C API) |
| Análisis de audio | Elemento `spectrum` — FFT en tiempo real |
| Transferencia a GPU | Elemento `glupload` |
| Renderizado visual | OpenGL / GLSL (fragment shader) |
| Lógica de control | C++20 |
| Interfaz de ventana | GTK4 (`gtk4paintablesink`) |
| Build system | Meson + Ninja |

## Arquitectura del Pipeline GStreamer

```
┌──────────┐
│  Archivo │
│  .mp4    │
└────┬─────┘
     │
     ▼
┌──────────────┐
│ uridecodebin │  ← demuxing + decodificación (audio/video)
└──┬───────┬───┘
   │       │
   │       │  pad "audio/x-raw"
   │       ▼
   │  ┌──────────────┐
   │  │ audioconvert │  ← conversión de formato PCM
   │  └──────┬───────┘
   │         ▼
   │  ┌──────────────┐
   │  │   spectrum   │  ← FFT: calcula 16 bandas de frecuencia (~30ms)
   │  └──────┬───────┘
   │         ▼
   │  ┌──────────────┐
   │  │ autoaudiosink│  ← salida de audio a altavoces
   │  └──────────────┘
   │
   │  pad "video/x-raw"
   ▼
┌──────────────┐
│ videoconvert │  ← conversión de espacio de color (→ RGB)
└──────┬───────┘
       ▼
┌──────────────┐
│   glupload   │  ← sube textura del frame a la GPU (VRAM)
└──────┬───────┘
       ▼
┌──────────────┐
│   glshader   │  ← aplica el fragment shader GLSL con uniforms de audio
└──────┬───────┘
       ▼
┌──────────────┐
│  glsinkbin   │  ← wrapper de sink OpenGL
│      │       │
│  ┌───┴────┐  │
│  │gtk4    │  │  ← GdkPaintable → GtkPicture → ventana GTK4
│  │paintable│  │
│  │sink    │  │
│  └────────┘  │
└──────────────┘
```

> **Nota:** `uridecodebin` expone pads dinámicos. La aplicación conecta cada pad según su tipo (`audio/` o `video/`) en el callback `onPadAdded`. El primer pad de audio recibe la cadena `audioconvert → spectrum → autoaudiosink`; los pads de audio adicionales van directo a `autoaudiosink`. El pad de video pasa por `videoconvert → glupload → glshader → glsinkbin → gtk4paintablesink`.

## Flujo de Datos: del Audio al Shader

El ciclo completo de procesamiento de cada frame sigue estos pasos:

```
1. DEMUX + DECODE
   uridecodebin descompone el contenedor y decodifica audio (→ PCM) y video (→ RGB).

2. FFT DEL AUDIO
   spectrum analiza el buffer de audio y emite un mensaje "spectrum" en el GstBus
   con un array de 16 floats en dB (magnitudes por banda de frecuencia).
   Intervalo de emisión: ~30 ms.

3. LECTURA DE MAGNITUDES                          ← CPU (SpectrumAnalyzer)
   ┌─────────────────────────────────────────┐
   │ SpectrumAnalyzer::processMessage(msg)   │
   │ Extrae el array "magnitude" del mensaje  │
   │ y almacena los 16 valores dB en         │
   │ magnitudes_ (std::vector<float>).       │
   └─────────────────────────────────────────┘

4. CONVERSIÓN Y ENVÍO A GPU                     ← CPU (GLRenderer)
   ┌─────────────────────────────────────────┐
   │ GLRenderer::updateAmplitudes(dBValues)  │
   │                                         │
   │  dBtoLinear(dB) → [0.0, 1.0]           │
   │  Empaqueta en GstStructure:             │
   │    u_a0  = amplitud banda 0             │
   │    u_a1  = amplitud banda 1             │
   │    ...                                  │
   │    u_a15 = amplitud banda 15            │
   │    u_texel_x = 1/width                  │
   │    u_texel_y = 1/height                 │
   │    u_blur = 15.0                        │
   │                                         │
   │  g_object_set(glshader, "uniforms", s)  │
   │  g_object_set(glshader, "update-shader",│
   │               TRUE)                     │
   └─────────────────────────────────────────┘

5. RENDERIZADO EN GPU                            ← GPU (Fragment Shader)
   ┌─────────────────────────────────────────┐
   │ Por cada píxel del frame de video:       │
   │                                         │
   │  a) Calcula a qué columna pertenece     │
   │     según v_texcoord.x (16 columnas).   │
   │                                         │
   │  b) Si el píxel está en el margen entre │
   │     columnas (gap del 8%), muestra el   │
   │     video original (separador).         │
   │                                         │
   │  c) Lee la amplitud u_aN de esa banda.  │
   │     La altura visible = amp * altura.   │
   │                                         │
   │  d) Si yb >= amp → video original.      │
   │     Si yb <  amp → efecto neón:         │
   │       - Color neón asignado a la banda  │
   │       - Blur de vecindario 5×5 píxeles  │
   │       - Tinte del color de banda (12%)  │
   │       - Bordes con mezcla blur↔neón     │
   └─────────────────────────────────────────┘
```

### Diagrama de flujo entre componentes

```
  GstBus                  SpectrumAnalyzer           GLRenderer              GPU/Shader
  ──────                  ────────────────           ──────────              ──────────
     │                          │                        │                       │
     │  mensaje "spectrum"      │                        │                       │
     ├─────────────────────────►│                        │                       │
     │                          │                        │                       │
     │                          │  magnitudes()          │                       │
     │                          ├───────────────────────►│                       │
     │                          │                        │  updateAmplitudes()   │
     │                          │                        │  dB→linear            │
     │                          │                        │  pack uniforms        │
     │                          │                        ├──────────────────────►│
     │                          │                        │                       │
     │                          │                        │         GLSL exec     │
     │                          │                        │         16 columnas   │
     │                          │                        │         neon + blur   │
     │                          │                        │                       │
     │                          │                        │   frame renderizado   │
     │                          │                        │◄──────────────────────┤
```

## El Fragment Shader en detalle

El shader (`eq_columns.frag` / código inline en `renderer.cpp`) implementa el algoritmo de enmascarado por columnas:

### 1. División en columnas
```glsl
float colW = 1.0 / 16.0;
int band = int(floor(v_texcoord.x / colW));
```
La pantalla se divide en 16 franjas verticales iguales. Cada píxel se asigna a una banda `[0..15]`.

### 2. Márgenes entre columnas
```glsl
float gapMargin = 0.08;  // 8% de margen a cada lado
float posInCol = (v_texcoord.x - float(band) * colW) / colW;
if (posInCol < gapMargin || posInCol > (1.0 - gapMargin)) {
    gl_FragColor = texture2D(tex, v_texcoord);  // video original
    return;
}
```
Un 16% del ancho de cada columna (8% a cada lado) se reserva como separador donde se ve el video sin modificar.

### 3. Máscara por amplitud
```glsl
float amp = clamp(a, 0.0, 1.0);
float yb = 1.0 - v_texcoord.y;  // coordenada Y invertida
if (yb >= amp) {
    gl_FragColor = texture2D(tex, v_texcoord);  // fuera de la barra: video original
    return;
}
```
Si la coordenada Y del píxel está por encima de la altura proporcional a la amplitud de esa banda, se muestra el video sin tocar. Si está dentro de la barra, se aplica el efecto.

### 4. Efecto neón con blur
```glsl
// Blur del vecindario 5×5
for (float x = -2.0; x <= 2.0; x += 1.0) {
    for (float y = -2.0; y <= 2.0; y += 1.0) {
        vec2 off = vec2(x * u_texel_x * u_blur, y * u_texel_y * u_blur);
        blurred += texture2D(tex, v_texcoord + off);
    }
}
blurred /= 25.0;
```
Se toma un promedio de 25 píxeles del video original con un radio de blur controlado por `u_blur`.

### 5. Color neón por banda
```glsl
vec3 neonColor(int band) {
    if (band == 0)  return vec3(0.0, 1.0, 1.0);   // cian
    if (band == 1)  return vec3(0.2, 0.8, 1.0);   // azul claro
    // ... 14 colores más
}
```
Cada banda tiene un color neón asignado (cian, azul, verde, amarillo, naranja, rojo, magenta, violeta...).

### 6. Mezcla final con bordes suaves
```glsl
// Dentro de la barra: tinte sutil del color de banda sobre el blur
gl_FragColor = blurred * vec4(mix(vec3(1.0), ncol, 0.12), 1.0);

// En los bordes de la barra: transición blur → neón sólido
if (edgeMax > 0.0) {
    gl_FragColor = mix(blurred, vec4(ncol, 1.0), edgeMax * 0.65);
}
```

## Estructura del Proyecto

```
gstreamer-ar-overlay/
├── assets/                        # Videos demo
│   └── 2026-06-11 14-49-30.mp4
├── shaders/                       # Archivos GLSL
│   ├── default.vert               # Vertex shader (fullscreen quad)
│   └── eq_columns.frag            # Fragment shader (máscara de columnas)
├── src/                           # Código fuente C++20
│   ├── main.cpp                   # Punto de entrada
│   ├── pipeline.hpp / .cpp        # Pipeline GStreamer (RAII)
│   ├── spectrum.hpp / .cpp        # Analizador FFT (lee mensajes "spectrum")
│   ├── renderer.hpp / .cpp        # Controlador GLSL (uniforms, conversión dB)
│   ├── shader_loader.hpp / .cpp   # Carga de archivos .vert/.frag desde disco
│   └── gui.hpp / .cpp             # Ventana GTK4 + GtkPicture
├── meson.build                    # Build system
└── README.md
```

## Dependencias

| Paquete | Descripción |
|---|---|
| `gstreamer-1.0` (≥ 1.20) | Core de GStreamer |
| `gstreamer-gl-1.0` (≥ 1.20) | Elementos OpenGL (`glupload`, `glshader`, `glsinkbin`) |
| `gtk4` (≥ 4.0) | Ventana y `gtk4paintablesink` |
| `gst-plugins-base` | `uridecodebin`, `videoconvert`, `audioconvert` |
| `gst-plugins-good` | `spectrum`, `autoaudiosink` |
| `gst-plugins-bad` | `gtk4paintablesink` |
| `meson` + `ninja` | Sistema de compilación |

### Instalación en Ubuntu/Debian

```bash
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                 libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev \
                 libgstreamer-gl1.0-dev libgtk-4-dev meson ninja-build
```

## Compilación

```bash
meson setup builddir
meson compile -C builddir
```

## Uso

```bash
./builddir/ar-overlay <archivo.mp4>
```

El reproductor abre una ventana GTK4 a 1920×1080. El video se reproduce con el efecto de columnas reactivas al audio. Ctrl+C o cerrar la ventana termina la ejecución.

### Señales soportadas
- `SIGINT` (Ctrl+C) / `SIGTERM`: cierre graceful del pipeline.
- `GST_MESSAGE_EOS`: fin de stream — cierra automáticamente.
- `GST_MESSAGE_ERROR`: error en el pipeline — loguea y cierra.

## Clases principales

| Clase | Responsabilidad |
|---|---|
| `Pipeline` | Construye el pipeline GStreamer, conecta pads dinámicos, gestiona el bus de mensajes y el ciclo de vida (RAII). |
| `SpectrumAnalyzer` | Se adjunta al elemento `spectrum`, intercepta mensajes `"spectrum"` del bus y extrae el array `magnitude` de 16 floats. |
| `GLRenderer` | Configura el `glshader` con el fragment shader inline, convierte amplitudes dB→linear y las empaqueta como uniforms en una `GstStructure`. |
| `ShaderLoader` | Lee archivos `.vert`/`.frag` del disco (usado para referencia; el shader actual se compila inline en `renderer.cpp`). |
| `GUI` | Crea la ventana GTK4, obtiene el `GdkPaintable` del sink y lo muestra en un `GtkPicture`. |

## Roadmap

| Fase | Estado |
|---|---|
| 1. Esqueleto del proyecto | ✅ |
| 2. Pipeline multimedia básico | ✅ |
| 3. Análisis de espectro de audio (FFT) | ✅ |
| 4. Renderizado OpenGL básico | ✅ |
| 5. Shader reactivo al audio (columnas + neón) | ✅ |
| 6. Pulido y robustez | 🔲 |

Ver [ROADMAP.md](ROADMAP.md) para más detalles.
