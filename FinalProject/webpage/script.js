document.addEventListener('DOMContentLoaded', () => {
    const pwmSlider = document.getElementById('pwmSlider');
    const pwmPercentageDisplay = document.getElementById('pwm-percentage');
    const pwmBar = document.getElementById('pwmBar');

    // Function to update the PWM display and bar
    const updatePWM = (value) => {
        pwmPercentageDisplay.textContent = `${value}%`;
        pwmBar.style.width = `${value}%`;
    };

    // Initialize the PWM display and bar with the slider's initial value
    // This ensures it matches the HTML's initial 'value' attribute
    updatePWM(pwmSlider.value);

    // Add an event listener to the slider for real-time updates
    pwmSlider.addEventListener('input', (event) => {
        updatePWM(event.target.value);
    });

    // --- Optional: Simulate Sensor Updates (for demonstration purposes) ---
    // If you were connecting to an MCU, this part would be replaced by actual data fetching.

    const temperatureDisplay = document.getElementById('temperature');
    const humidityDisplay = document.getElementById('humidity');
    const airPressureDisplay = document.getElementById('air-pressure');

    const simulateSensorData = () => {
        // DHT11 Sensor Simulation
        const temp = (Math.random() * (30 - 20) + 20).toFixed(1); // 20.0 to 30.0 °C
        const humidity = (Math.random() * (70 - 50) + 50).toFixed(1); // 50.0 to 70.0 %

        // Ionic Thruster Air Pressure Simulation (km/h)
        // Let's make it slightly responsive to PWM for fun
        const currentPwm = parseInt(pwmSlider.value);
        const basePressure = 500; // Base km/h at 0% PWM
        const maxAdditionalPressure = 1500; // Max additional km/h at 100% PWM
        const pressure = (basePressure + (maxAdditionalPressure * (currentPwm / 100))).toFixed(0);

        temperatureDisplay.textContent = `${temp} \u00B0C`; // \u00B0 is the degree symbol
        humidityDisplay.textContent = `${humidity} %`;
        airPressureDisplay.textContent = `${pressure} km/h`;
    };

    // Update sensor data every 2 seconds
    setInterval(simulateSensorData, 2000);

    // Call it once immediately so there's data on load
    simulateSensorData();

});