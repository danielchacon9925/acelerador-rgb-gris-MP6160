# Código fuente HLS

Aquí irán los archivos C/C++ sintetizables para el kernel HLS.

## Directrices preliminares

- Mantener la lógica de conversión BT.601 en aritmética entera.
- Evitar float/double/ap_fixed en el kernel funcional.
- Usar la misma fórmula que el prototipo virtual y la referencia:

```c
gris = (77 * R + 150 * G + 29 * B) >> 8;
```
