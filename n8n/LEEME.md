# Flujo de n8n

`asesor_ia.json` implementa el asesor de IA: pide a Node-RED el resumen de 24 h, arma el prompt, consulta a Ollama (`qwen2.5:3b`) y devuelve el diagnóstico a Node-RED. Probado con n8n 2.34.6.

El disparador manual usa datos de ejemplo; el programado (cada 3 h) y el que viene del dashboard usan los datos reales. El flujo se importa **inactivo**: actívelo para que funcionen el disparador programado y el botón del dashboard.

Las direcciones usan `127.0.0.1` porque n8n puede resolver `localhost` a IPv6 y fallar (error ECONNREFUSED sobre `::1`).
