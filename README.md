# Taller SIMD  
## CE-4302 Arquitectura de Computadores II  
**Autores:** Abraham Venegas & Sebastián Hidalgo  

---

## Descripción

Este proyecto implementa una comparación de rendimiento entre dos enfoques para contar caracteres en una cadena:  
- **Versión serial**  
- **Versión SIMD** (alineación a 16 y 32 bytes, así como sin alineación)

Se utiliza Python para automatizar las pruebas de rendimiento y generar una gráfica comparativa de tiempos normalizados.

---

## Requisitos

- Compilador `g++` con soporte para `-msse4.2`  
- Python 3  
- Paquetes: `matplotlib`, `numpy`

Instalación de paquetes requeridos:
```bash
pip install matplotlib numpy
