# Reportes y salidas HLS

Esta carpeta almacenará los artefactos generados por el flujo de HLS:

- reportes de síntesis,
- logs de co-simulación,
- archivos RAW de salida del kernel HLS.

## Archivo de salida esperado

```text
reports/output_hls.raw
```

Este nombre se utiliza para que el script de comparación cruzada pueda consumir la salida de la implementación HLS.

## Validación de fase 2

El comando:

```bash
./hls/scripts/run_tb.sh
```

debe generar este archivo y dejar listo el resultado para la comparación contra el prototipo virtual y la referencia.
