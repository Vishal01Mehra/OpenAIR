#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "credentials.h"

// Pin definitions
#define BOOT_BUTTON 0
#define FAN_PIN 4

// Global variables
WebServer server(80);
WiFiConfig wifiConfig;
bool isConfigMode = false;
unsigned long buttonPressTime = 0;
bool buttonPressed = false;

// AQI simulation variables
float currentAQI = 50.0;
float aqiHistory[24]; // Last 24 hours data
unsigned long lastAQIUpdate = 0;
bool fanAutoMode = true;
float fanThreshold = 100.0;

// Function declarations
void checkResetButton();
void checkBootButton();
void loadWiFiConfig();
void startConfigMode();
void setupWebServer();
void updateAQISimulation();
void handleGetAQI();
void handleGetHistory();
void handleFanControl();
void handleSettings();
void handleWiFiConfig();
void handleNotFound();
void handleRoot();

// HTML content - COMPLETELY CLEAN VERSION
const char* INDEX_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OpenFilter</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --primary: #1d1d1f;
            --secondary: #2c2c2e;
            --accent: #0a84ff;
            --text-primary: #ffffff;
            --text-secondary: #98989d;
            --success: #30d158;
            --warning: #ff9f0a;
            --danger: #ff453a;
            --card-bg: rgba(28, 28, 30, 0.8);
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: 
                linear-gradient(135deg, rgba(0, 0, 0, 0.9) 0%, rgba(29, 29, 31, 0.9) 100%),
                url('https://images.unsplash.com/photo-1504608524841-42fe6f032b4b?ixlib=rb-4.0.3&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D&auto=format&fit=crop&w=2065&q=80');
            background-size: cover;
            background-position: center;
            background-attachment: fixed;
            color: var(--text-primary);
            min-height: 100vh;
            padding: 20px;
            position: relative;
        }
        
        body::before {
            content: '';
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: 
                url('https://images.unsplash.com/photo-1504608524841-42fe6f032b4b?ixlib=rb-4.0.3&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D&auto=format&fit=crop&w=2065&q=80');
            background-size: cover;
            background-position: center;
            background-attachment: fixed;
            opacity: 0.5;
            z-index: -1;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            position: relative;
            z-index: 1;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding-top: 20px;
        }
        
        .header h1 {
            font-size: 2rem;
            font-weight: 600;
            margin-bottom: 8px;
            background: linear-gradient(135deg, #ffffff, #98989d);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        
        .header p {
            color: var(--text-secondary);
            font-size: 1.1rem;
        }
        
        .status-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding: 10px 20px;
            background: rgba(28, 28, 30, 0.8);
            border-radius: 12px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .connection-status {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 0.9rem;
        }
        
        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: var(--text-secondary);
            animation: pulse 2s infinite;
        }
        
        .status-dot.connected {
            background: var(--success);
        }
        
        .status-dot.disconnected {
            background: var(--danger);
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        
        .dashboard {
            display: grid;
            grid-template-columns: 1fr;
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .card {
            background: var(--card-bg);
            border-radius: 20px;
            padding: 25px;
            backdrop-filter: blur(20px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .card.controls {
            background: rgba(28, 28, 30, 0.95);
        }
        
        .current-reading {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .gauge-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin-bottom: 20px;
        }
        
        .gauge {
            width: 180px;
            height: 180px;
            position: relative;
            margin-bottom: 15px;
        }
        
        .gauge-bg {
            fill: none;
            stroke: rgba(255, 255, 255, 0.1);
            stroke-width: 12;
        }
        
        .gauge-fill {
            fill: none;
            stroke: var(--success);
            stroke-width: 12;
            stroke-linecap: round;
            transform: rotate(-90deg);
            transform-origin: 50% 50%;
            transition: all 1s ease;
        }
        
        .gauge-value {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 2.5rem;
            font-weight: 300;
        }
        
        .gauge-label {
            font-size: 1.1rem;
            color: var(--text-secondary);
            margin-bottom: 10px;
        }
        
        .aqi-level {
            font-size: 1.3rem;
            font-weight: 600;
            margin-bottom: 20px;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin-top: 25px;
        }
        
        .status-item {
            text-align: center;
            padding: 15px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 12px;
        }
        
        .status-value {
            font-size: 1.4rem;
            font-weight: 600;
            margin-bottom: 5px;
        }
        
        .status-label {
            font-size: 0.85rem;
            color: var(--text-secondary);
        }
        
        .controls {
            display: flex;
            flex-direction: column;
            gap: 15px;
        }
        
        .btn {
            padding: 16px 24px;
            border: none;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            background: rgba(255, 255, 255, 0.1);
            color: var(--text-primary);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .btn.active {
            background: var(--accent);
            border-color: var(--accent);
        }
        
        .btn:hover {
            background: rgba(255, 255, 255, 0.15);
        }
        
        .btn.active:hover {
            background: var(--accent);
            opacity: 0.9;
        }
        
        .btn-group {
            display: flex;
            gap: 10px;
        }
        
        .btn-group .btn {
            flex: 1;
        }
        
        .slider-container {
            margin: 20px 0;
        }
        
        .slider-label {
            display: flex;
            justify-content: space-between;
            margin-bottom: 12px;
            color: var(--text-secondary);
            font-weight: 500;
        }
        
        .slider {
            width: 100%;
            height: 6px;
            border-radius: 3px;
            background: rgba(255, 255, 255, 0.1);
            outline: none;
            -webkit-appearance: none;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--accent);
            cursor: pointer;
            border: 2px solid var(--primary);
        }
        
        .chart-container {
            height: 200px;
            margin-top: 20px;
        }
        
        .config-panel {
            background: var(--card-bg);
            border-radius: 20px;
            padding: 25px;
            margin-top: 20px;
            display: none;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        
        .form-label {
            display: block;
            margin-bottom: 8px;
            color: var(--text-secondary);
            font-weight: 500;
        }
        
        .form-input {
            width: 100%;
            padding: 14px 16px;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            font-size: 1rem;
            color: var(--text-primary);
            transition: border-color 0.3s ease;
        }
        
        .form-input:focus {
            outline: none;
            border-color: var(--accent);
        }
        
        .form-input::placeholder {
            color: var(--text-secondary);
        }
        
        /* AQI Level Colors */
        .aqi-excellent { color: var(--success); }
        .aqi-good { color: #30d158; }
        .aqi-moderate { color: var(--warning); }
        .aqi-poor { color: #ff6b35; }
        .aqi-very-poor { color: var(--danger); }
        .aqi-severe { color: #bf5af2; }
        
        .fan-on { color: var(--success); }
        .fan-off { color: var(--text-secondary); }
        .mode-auto { color: var(--accent); }
        .mode-manual { color: var(--warning); }
        
        .connected-text { color: var(--success); }
        .disconnected-text { color: var(--danger); }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>OpenFilter</h1>
            <p>Air Quality Monitoring System</p>
        </div>
        
        <div class="status-bar">
            <div class="connection-status">
                <div class="status-dot" id="statusDot"></div>
                <span id="statusText">Connecting...</span>
            </div>
            <div class="last-update" id="lastUpdate">
                Last update: --
            </div>
        </div>
        
        <div class="dashboard">
            <div class="card current-reading">
                <div class="gauge-container">
                    <div class="gauge-label">AIR QUALITY INDEX</div>
                    <div class="gauge">
                        <svg viewBox="0 0 120 120" class="gauge">
                            <circle cx="60" cy="60" r="54" class="gauge-bg"></circle>
                            <circle cx="60" cy="60" r="54" class="gauge-fill" id="gaugeFill"></circle>
                        </svg>
                        <div class="gauge-value" id="aqiValue">--</div>
                    </div>
                    <div class="aqi-level" id="aqiLevel">Loading</div>
                </div>
                
                <div class="status-grid">
                    <div class="status-item">
                        <div class="status-value" id="fanStatus">OFF</div>
                        <div class="status-label">FAN</div>
                    </div>
                    <div class="status-item">
                        <div class="status-value" id="autoStatus">AUTO</div>
                        <div class="status-label">MODE</div>
                    </div>
                    <div class="status-item">
                        <div class="status-value" id="thresholdValue">100</div>
                        <div class="status-label">THRESHOLD</div>
                    </div>
                </div>
            </div>
            
            <div class="card controls">
                <h3 style="margin-bottom: 20px; color: var(--text-primary);">Fan Control</h3>
                
                <div class="btn-group">
                    <button class="btn active" id="btnAuto" onclick="setFanAuto(true)">AUTO</button>
                    <button class="btn" id="btnOn" onclick="setFanManual(true)">ON</button>
                    <button class="btn" id="btnOff" onclick="setFanManual(false)">OFF</button>
                </div>
                
                <div class="slider-container">
                    <div class="slider-label">
                        <span>Auto Threshold</span>
                        <span id="thresholdDisplay">100 AQI</span>
                    </div>
                    <input type="range" min="0" max="300" value="100" class="slider" id="thresholdSlider" oninput="updateThreshold(this.value)">
                </div>
                
                <button class="btn" onclick="toggleConfig()" style="margin-top: 10px;">WiFi Settings</button>
            </div>
            
            <div class="card">
                <h3 style="margin-bottom: 20px; color: var(--text-primary);">24-Hour History</h3>
                <div class="chart-container">
                    <canvas id="historyChart"></canvas>
                </div>
            </div>
        </div>
        
        <div class="config-panel" id="configPanel">
            <h3 style="margin-bottom: 20px; color: var(--text-primary);">Network Configuration</h3>
            <div class="form-group">
                <label class="form-label">Network Name</label>
                <input type="text" class="form-input" id="wifiSsid" placeholder="Enter WiFi SSID">
            </div>
            <div class="form-group">
                <label class="form-label">Password</label>
                <input type="password" class="form-input" id="wifiPassword" placeholder="Enter WiFi password">
            </div>
            <div class="btn-group">
                <button class="btn active" onclick="saveWiFiConfig()">Save & Restart</button>
                <button class="btn" onclick="toggleConfig()">Cancel</button>
            </div>
        </div>
    </div>

    <script>
        let aqiChart;
        let currentAqi = 0;
        let currentFanAuto = true;
        let isConnected = false;
        let lastUpdateTime = null;
        
        // Update connection status
        function updateConnectionStatus(connected, error = null) {
            const statusDot = document.getElementById('statusDot');
            const statusText = document.getElementById('statusText');
            
            if (connected) {
                statusDot.className = 'status-dot connected';
                statusText.textContent = 'Connected';
                statusText.className = 'connected-text';
                isConnected = true;
            } else {
                statusDot.className = 'status-dot disconnected';
                statusText.textContent = error || 'Disconnected';
                statusText.className = 'disconnected-text';
                isConnected = false;
            }
        }
        
        // Update last update time
        function updateLastUpdateTime() {
            const lastUpdateElement = document.getElementById('lastUpdate');
            lastUpdateTime = new Date();
            lastUpdateElement.textContent = 'Last update: ' + lastUpdateTime.toLocaleTimeString();
        }
        
        // Initialize chart
        function initializeChart() {
            const ctx = document.getElementById('historyChart').getContext('2d');
            aqiChart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: Array.from({length: 24}, (_, i) => {
                        const hour = i === 0 ? 12 : i > 12 ? i - 12 : i;
                        const period = i < 12 ? 'AM' : 'PM';
                        return `${hour}${period}`;
                    }),
                    datasets: [{
                        label: 'AQI',
                        data: [],
                        borderColor: '#0a84ff',
                        backgroundColor: 'rgba(10, 132, 255, 0.1)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4,
                        pointBackgroundColor: '#0a84ff',
                        pointBorderColor: '#000000',
                        pointBorderWidth: 1,
                        pointRadius: 2
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        y: {
                            beginAtZero: true,
                            max: 300,
                            grid: {
                                color: 'rgba(255, 255, 255, 0.1)'
                            },
                            ticks: {
                                color: 'rgba(255, 255, 255, 0.6)'
                            }
                        },
                        x: {
                            grid: {
                                display: false
                            },
                            ticks: {
                                color: 'rgba(255, 255, 255, 0.6)'
                            }
                        }
                    },
                    plugins: {
                        legend: {
                            display: false
                        }
                    }
                }
            });
        }
        
        // Update gauge
        function updateGauge(aqi) {
            const gaugeFill = document.getElementById('gaugeFill');
            const aqiValue = document.getElementById('aqiValue');
            const aqiLevel = document.getElementById('aqiLevel');
            
            // Calculate gauge rotation (0-270 degrees for 0-500 AQI)
            const rotation = (aqi / 500) * 270;
            gaugeFill.style.transform = `rotate(${rotation - 90}deg)`;
            
            // Update value
            aqiValue.textContent = Math.round(aqi);
            
            // Update color and level based on AQI
            let level = 'Excellent';
            let color = '#30d158';
            
            if (aqi <= 50) {
                level = 'Excellent';
                color = '#30d158';
            } else if (aqi <= 100) {
                level = 'Good';
                color = '#30d158';
            } else if (aqi <= 150) {
                level = 'Moderate';
                color = '#ff9f0a';
            } else if (aqi <= 200) {
                level = 'Poor';
                color = '#ff6b35';
            } else if (aqi <= 300) {
                level = 'Very Poor';
                color = '#ff453a';
            } else {
                level = 'Severe';
                color = '#bf5af2';
            }
            
            gaugeFill.style.stroke = color;
            aqiValue.className = 'gauge-value ' + getAqiClass(aqi);
            aqiLevel.textContent = level;
            aqiLevel.className = 'aqi-level ' + getAqiClass(aqi);
        }
        
        function getAqiClass(aqi) {
            if (aqi <= 50) return 'aqi-excellent';
            if (aqi <= 100) return 'aqi-good';
            if (aqi <= 150) return 'aqi-moderate';
            if (aqi <= 200) return 'aqi-poor';
            if (aqi <= 300) return 'aqi-very-poor';
            return 'aqi-severe';
        }
        
        // Update button states
        function updateButtonStates(fanAuto, fanState) {
            const btnAuto = document.getElementById('btnAuto');
            const btnOn = document.getElementById('btnOn');
            const btnOff = document.getElementById('btnOff');
            
            // Remove active class from all buttons
            btnAuto.classList.remove('active');
            btnOn.classList.remove('active');
            btnOff.classList.remove('active');
            
            // Add active class to current mode
            if (fanAuto) {
                btnAuto.classList.add('active');
            } else if (fanState) {
                btnOn.classList.add('active');
            } else {
                btnOff.classList.add('active');
            }
        }
        
        // API functions
        async function fetchAQI() {
            try {
                const response = await fetch('/api/aqi');
                if (!response.ok) throw new Error('Network response was not ok');
                
                const data = await response.json();
                
                currentAqi = data.aqi;
                currentFanAuto = data.fanAuto;
                updateGauge(data.aqi);
                updateButtonStates(data.fanAuto, data.fanState);
                
                document.getElementById('fanStatus').textContent = data.fanState ? 'ON' : 'OFF';
                document.getElementById('fanStatus').className = 'status-value ' + (data.fanState ? 'fan-on' : 'fan-off');
                document.getElementById('autoStatus').textContent = data.fanAuto ? 'AUTO' : 'MANUAL';
                document.getElementById('autoStatus').className = 'status-value ' + (data.fanAuto ? 'mode-auto' : 'mode-manual');
                document.getElementById('thresholdValue').textContent = data.threshold;
                document.getElementById('thresholdDisplay').textContent = data.threshold + ' AQI';
                document.getElementById('thresholdSlider').value = data.threshold;
                
                updateConnectionStatus(true);
                updateLastUpdateTime();
            } catch (error) {
                console.error('Error fetching AQI:', error);
                updateConnectionStatus(false, 'Connection Error');
            }
        }
        
        async function fetchHistory() {
            try {
                const response = await fetch('/api/history');
                if (!response.ok) throw new Error('Network response was not ok');
                
                const data = await response.json();
                
                if (aqiChart) {
                    aqiChart.data.datasets[0].data = data.history;
                    aqiChart.update();
                }
                
                updateConnectionStatus(true);
            } catch (error) {
                console.error('Error fetching history:', error);
                updateConnectionStatus(false, 'Connection Error');
            }
        }
        
        async function setFanAuto(auto) {
            try {
                const response = await fetch('/api/fan', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({auto: auto})
                });
                
                if (!response.ok) throw new Error('Network response was not ok');
                
                fetchAQI();
                updateConnectionStatus(true);
            } catch (error) {
                console.error('Error setting fan mode:', error);
                updateConnectionStatus(false, 'Connection Error');
            }
        }
        
        async function setFanManual(state) {
            try {
                const response = await fetch('/api/fan', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({auto: false, state: state})
                });
                
                if (!response.ok) throw new Error('Network response was not ok');
                
                fetchAQI();
                updateConnectionStatus(true);
            } catch (error) {
                console.error('Error setting fan state:', error);
                updateConnectionStatus(false, 'Connection Error');
            }
        }
        
        async function updateThreshold(value) {
            document.getElementById('thresholdDisplay').textContent = value + ' AQI';
            
            try {
                const response = await fetch('/api/settings', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({threshold: parseFloat(value)})
                });
                
                if (!response.ok) throw new Error('Network response was not ok');
                
                updateConnectionStatus(true);
            } catch (error) {
                console.error('Error updating threshold:', error);
                updateConnectionStatus(false, 'Connection Error');
            }
        }
        
        function toggleConfig() {
            const panel = document.getElementById('configPanel');
            panel.style.display = panel.style.display === 'none' ? 'block' : 'none';
        }
        
        async function saveWiFiConfig() {
            const ssid = document.getElementById('wifiSsid').value;
            const password = document.getElementById('wifiPassword').value;
            
            if (!ssid) {
                alert('Please enter network name');
                return;
            }
            
            try {
                const response = await fetch('/api/wifi', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ssid: ssid, password: password})
                });
                
                if (!response.ok) throw new Error('Network response was not ok');
                
                const result = await response.json();
                alert(result.message);
                
                if (result.status === 'success') {
                    setTimeout(() => {
                        window.location.reload();
                    }, 3000);
                }
                
                updateConnectionStatus(true);
            } catch (error) {
                console.error('Error saving WiFi config:', error);
                alert('Error saving configuration');
                updateConnectionStatus(false, 'Connection Error');
            }
        }
        
        // Initialize and start updates
        initializeChart();
        fetchAQI();
        fetchHistory();
        
        // Update data every 2 seconds
        setInterval(() => {
            fetchAQI();
        }, 2000);
        
        // Update history every 30 seconds
        setInterval(() => {
            fetchHistory();
        }, 30000);
        
        // Initial connection status
        updateConnectionStatus(false, 'Connecting...');
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // Initialize pins
    pinMode(BOOT_BUTTON, INPUT_PULLUP);
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Initialize AQI history
    for(int i = 0; i < 24; i++) {
        aqiHistory[i] = 50.0 + random(-10, 10);
    }
    
    // Check for reset button press
    checkResetButton();
    
    // Load WiFi configuration and connect
    loadWiFiConfig();
    
    // Setup web server routes
    setupWebServer();
    
    Serial.println("AQI Monitoring Server Started");
}

void loop() {
    server.handleClient();
    
    // Check boot button for reset
    checkBootButton();
    
    // Update AQI simulation every 2 seconds
    if(millis() - lastAQIUpdate > 2000) {
        updateAQISimulation();
        lastAQIUpdate = millis();
    }
    
    // Auto control fan
    if(fanAutoMode) {
        digitalWrite(FAN_PIN, currentAQI > fanThreshold ? HIGH : LOW);
    }
}

void checkResetButton() {
    // Check if reset flag is set
    bool resetFlag;
    EEPROM.get(RESET_FLAG_ADDR, resetFlag);
    if(resetFlag) {
        EEPROM.put(RESET_FLAG_ADDR, false);
        EEPROM.commit();
        startConfigMode();
    }
}

void checkBootButton() {
    if(digitalRead(BOOT_BUTTON) == LOW) {
        if(!buttonPressed) {
            buttonPressed = true;
            buttonPressTime = millis();
        } else if(millis() - buttonPressTime > 5000) {
            // Button held for 5 seconds - enter config mode
            // Set reset flag and reboot
            EEPROM.put(RESET_FLAG_ADDR, true);
            EEPROM.commit();
            Serial.println("Reset flag set, rebooting...");
            delay(1000);
            ESP.restart();
        }
    } else {
        buttonPressed = false;
    }
}

void loadWiFiConfig() {
    EEPROM.get(WIFI_CONFIG_ADDR, wifiConfig);
    
    // Check if configuration is valid
    if(wifiConfig.ssid[0] == '\0' || strlen(wifiConfig.ssid) == 0) {
        Serial.println("No WiFi config found, starting config mode");
        startConfigMode();
        return;
    }
    
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifiConfig.ssid);
    
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    
    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        isConfigMode = false;
    } else {
        Serial.println("\nFailed to connect to WiFi, starting config mode");
        startConfigMode();
    }
}

void startConfigMode() {
    Serial.println("Starting Configuration Mode");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
    
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    isConfigMode = true;
}

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void setupWebServer() {
    // Serve main page
    server.on("/", HTTP_GET, handleRoot);
    
    // API endpoints
    server.on("/api/aqi", HTTP_GET, handleGetAQI);
    server.on("/api/history", HTTP_GET, handleGetHistory);
    server.on("/api/fan", HTTP_POST, handleFanControl);
    server.on("/api/settings", HTTP_POST, handleSettings);
    server.on("/api/wifi", HTTP_POST, handleWiFiConfig);
    
    // Handle not found routes
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("HTTP server started");
}

void handleGetAQI() {
    StaticJsonDocument<200> doc;
    doc["aqi"] = currentAQI;
    doc["fanAuto"] = fanAutoMode;
    doc["fanState"] = digitalRead(FAN_PIN);
    doc["threshold"] = fanThreshold;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleGetHistory() {
    StaticJsonDocument<1024> doc;
    JsonArray history = doc.createNestedArray("history");
    
    for(int i = 0; i < 24; i++) {
        history.add(aqiHistory[i]);
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleFanControl() {
    if(server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        deserializeJson(doc, server.arg("plain"));
        
        if(doc.containsKey("auto")) {
            fanAutoMode = doc["auto"];
            if(!fanAutoMode && doc.containsKey("state")) {
                digitalWrite(FAN_PIN, doc["state"] ? HIGH : LOW);
            }
        }
        
        server.send(200, "application/json", "{\"status\":\"success\"}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
}

void handleSettings() {
    if(server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        deserializeJson(doc, server.arg("plain"));
        
        if(doc.containsKey("threshold")) {
            fanThreshold = doc["threshold"];
        }
        
        server.send(200, "application/json", "{\"status\":\"success\"}");
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
}

void handleWiFiConfig() {
    if(server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        deserializeJson(doc, server.arg("plain"));
        
        String ssid = doc["ssid"];
        String password = doc["password"];
        
        // Save to EEPROM
        strncpy(wifiConfig.ssid, ssid.c_str(), sizeof(wifiConfig.ssid));
        strncpy(wifiConfig.password, password.c_str(), sizeof(wifiConfig.password));
        
        EEPROM.put(WIFI_CONFIG_ADDR, wifiConfig);
        EEPROM.commit();
        
        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved. Rebooting...\"}");
        
        delay(2000);
        ESP.restart();
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
}

void handleNotFound() {
    Serial.print("404 - Not found: ");
    Serial.print(server.uri());
    Serial.print(" - Method: ");
    Serial.println(server.method() == HTTP_GET ? "GET" : "POST");
    
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    server.send(404, "text/plain", message);
}

void updateAQISimulation() {
    // Simulate AQI changes
    float change = (random(-100, 100)) / 10.0;
    currentAQI += change;
    
    // Keep AQI in reasonable range
    if(currentAQI < 0) currentAQI = 0;
    if(currentAQI > 500) currentAQI = 500;
    
    // Update history (shift array)
    for(int i = 23; i > 0; i--) {
        aqiHistory[i] = aqiHistory[i-1];
    }
    aqiHistory[0] = currentAQI;
}