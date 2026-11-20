# Tópicos MQTT y servicios HTTP

| Tópico | Uso |
|---|---|
| `agri/nodo/N/telemetria` | Lectura del nodo N (1 a 3), en JSON |
| `agri/nodo/N/error` | Sin respuesta o sensores caídos |
| `agri/nodo/3/cmd` | Orden de la válvula: `{"accion":"abrir","seg":60}` o `{"accion":"cerrar"}` |
| `agri/nodo/3/evento` | Orden recibida por el gateway y confirmada por el nodo |
| `agri/ia/diagnostico` | Diagnóstico del asesor de IA (mensaje retenido) |
| `agri/gateway/estado` | Estado del gateway |

Servicios HTTP de Node-RED para n8n: `GET /api/resumen24h` (agregue `?demo=1` para datos de ejemplo) y `POST /api/diagnostico`.

Archivos que genera Node-RED en su carpeta de datos: respaldo JSON mensual, CSV crudo por nodo y día, CSV de promedios de 5 minutos por nodo y mes, registro de riego y de diagnósticos.
