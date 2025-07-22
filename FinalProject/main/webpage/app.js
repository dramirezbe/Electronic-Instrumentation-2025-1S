/**
 * @file app.js
 * @brief Main JavaScript file for the Ionic Thruster Control Interface.
 *
 * Handles fetching sensor data from the ESP32 server and sending control
 * commands back to it.
 */

$(document).ready(function() {

    // --- Element Selectors ---
    const tempValueElement = $('#temperature-value');
    const airSpeedValueElement = $('#air-speed-value');
    const pwmSlider = $('#pwm-slider');
    const pwmPercentageElement = $('#pwm-percentage-value');
    const pwmBarElement = $('#pwm-bar');

    /**
     * @brief Fetches sensor readings from the server and updates the UI.
     *
     * This function makes two GET requests to the ESP32:
     * 1. /lm35Sensor.json for the temperature.
     * 2. /anemoSensor.json for the wind speed.
     */
    function updateSensorReadings() {
        // Fetch Temperature Data
        $.getJSON('/lm35Sensor.json', function(data) {
            if (data && typeof data.temp !== 'undefined') {
                // Update the temperature value, formatted to one decimal place.
                tempValueElement.html(data.temp.toFixed(1) + ' °C');
            }
        }).fail(function() {
            console.error("Error: Could not retrieve temperature data.");
        });

        // Fetch Anemometer (Air Speed) Data
        $.getJSON('/anemoSensor.json', function(data) {
            if (data && typeof data.wind !== 'undefined') {
                // Update the air speed value, formatted to one decimal place.
                airSpeedValueElement.text(data.wind.toFixed(1) + ' km/h');
            }
        }).fail(function() {
            console.error("Error: Could not retrieve air speed data.");
        });
    }

    /**
     * @brief Sends the new PWM value to the server via a POST request.
     * @param {number} pwmValue - The PWM value (0-100) to be sent.
     */
    function sendPwmValue(pwmValue) {
        console.log(`Sending PWM value to server: ${pwmValue}`);

        $.ajax({
            url: '/pwmValues.json',
            type: 'POST',
            contentType: 'application/json',
            // The C code expects a JSON object: {"pwm_val": value}
            data: JSON.stringify({ pwm_val: pwmValue }),
            success: function(response) {
                console.log('Server successfully received PWM value.');
            },
            error: function(xhr, status, error) {
                console.error('Error sending PWM value:', status, error);
            }
        });
    }


    // --- Event Listeners ---

    // Update the UI in real-time as the slider is moved.
    pwmSlider.on('input', function() {
        const value = $(this).val();
        pwmPercentageElement.text(value + '%');
        pwmBarElement.css('width', value + '%');
    });

    // Send the final value to the server only when the user releases the slider.
    // This prevents sending too many requests while dragging.
    pwmSlider.on('change', function() {
        const value = parseInt($(this).val());
        sendPwmValue(value);
    });


    // --- Initialization ---

    // Fetch initial sensor data as soon as the page loads.
    updateSensorReadings();

    // Set an interval to automatically update sensor data every 2 seconds (2000 ms).
    setInterval(updateSensorReadings, 2000);

});