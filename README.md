# Taller SIMD  
## CE-4302 Arquitectura de Computadores II  
**Autores:** Abraham Venegas & Sebastián Hidalgo  

---

## Descripción

Este taller implementa una comparación de rendimiento entre dos enfoques para contar caracteres en una cadena:  
- **Versión serial**  
- **Versión SIMD** (alineación a 16 y 32 bytes, así como sin alinear)


## Requisitos

- Compilador `g++` con soporte para `-msse4.2`  
- Python 3  
- Paquetes: `matplotlib`, `numpy`

Instalación de paquetes requeridos:
```bash
pip install matplotlib numpy

## Instrucciones de uso

1. **Compilación de binarios**

   Se ejecuta el siguiente comando para compilar los ejecutables:
   ```bash
   make
2. **Ejecución del script comparativo**
    ```bash
    python3 comparison_plot.py

El script realizará las pruebas de rendimiento para distintos tamaños de cadena y alineaciones (16B, 32B y sin alinear), y luego generará una gráfica en el directorio /comparison_plots con el nombre: normalized_time_comparison.png
