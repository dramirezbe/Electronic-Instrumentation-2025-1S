# 1. Interrupciones

Implementar el conteo de pulsos de un circuito anti-rebote hardware y software:

**Hardware:** Capacitor en serie con botón, pull-up 5v, 
**Software**  No contar botón a partir de ciertos ms, dentro del código

# 2. Timer + UART

Implementar timer que cuente cada segundo (horas,minutos,segundos), y mostrarlo por UART (Implementar conteo por interrupción)

Implementar PWM por GPIO x, activar interrupciones para x, modificar el timer.

# 3. ADC

Leer el voltaje de un potenciómetro con un ADC. Mostrar por UART.

# 4. PWM

Implementar salida análogica con pwn y filtro pasa-bajos, y luego medirla con ADC del mismo embebido (usar graficador serial), implementar un "Fade" a un led utilizando la señal PWM.