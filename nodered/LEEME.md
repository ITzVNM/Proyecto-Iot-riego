# Flujo de Node-RED

Un solo archivo, cuatro pestañas: **Agrivoltaico** (ingesta, respaldo JSON, CSV crudo y de promedios), **Riego** (regla determinista y botones manuales), **Dashboard** y **IA** (servicios HTTP para n8n).

Probado con Node-RED 5.0.4 y `@flowfuse/node-red-dashboard` 1.34.1.

Antes de importar: instale el paquete del dashboard. Si una función de CSV muestra un error sobre el módulo `fs`, active `functionExternalModules` en la configuración de Node-RED.

Después de importar: edite el nodo de configuración del broker (viene como `ws://127.0.0.1:8083/mqtt`, con el cliente `nodered-agri`) y, si no usa la ruta por defecto, la carpeta de datos en los nodos de archivo y en las funciones de CSV.

El botón "Pedir diagnóstico ahora" del dashboard llama al webhook de producción de n8n, que solo responde si el flujo de n8n está **activo**. Para probar sin activarlo, cambie la ruta `webhook` por `webhook-test` y pulse antes "Execute workflow" en n8n.
