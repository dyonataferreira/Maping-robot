#pragma once

#ifndef PAGINA_WEB_H
#define PAGINA_WEB_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Vespa Nav</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover"/>
    <meta name="apple-mobile-web-app-capable" content="yes">
    <style>
        * { box-sizing: border-box; }
        body { background: #1a1a1a; color: white; font-family: 'Gill Sans', Calibri, sans-serif; margin: 0; padding: 0; display: flex; flex-direction: column; height: 100vh; overflow: hidden; touch-action: none; }
        
        /* TOPO */
        .header { background: black; padding: 0 15px; height: 50px; display: flex; justify-content: space-between; align-items: center; width: 100%; flex-shrink: 0; z-index: 10;}
        .battery-container { display: flex; align-items: center; }
        .battery { height: 16px; width: 35px; border: 2px solid #F1F1F1; border-radius: 4px; padding: 1px; position: relative; margin-right: 8px; }
        .battery::after { content: ''; position: absolute; height: 10px; width: 3px; background: #F1F1F1; right: -5px; top: 1px; border-radius: 0 2px 2px 0; }
        .part { background: #0F0; height: 100%; border-radius: 1px; transition: width 0.3s; }
        .status-pill { background: red; color: white; padding: 3px 8px; border-radius: 12px; font-size: 12px; font-weight: bold; margin-left: 10px; transition: 0.3s; }

        /* MAPA E BOTÕES EM LINHA */
        .map-wrapper { flex: 0 0 48vh; display: flex; flex-direction: column; background: #000; border-bottom: 2px solid #333; }
        .map-canvas-container { flex: 1; display: flex; justify-content: center; align-items: center; position: relative; padding: 5px; }
        #mapaCanvas { width: 100%; height: 100%; max-width: 450px; max-height: 450px; aspect-ratio: 1/1; background: #111; image-rendering: pixelated; object-fit: contain; }
        
        .btn-panel { display: flex; flex-direction: row; justify-content: center; gap: 8px; padding: 8px; background: #111; }
        .btn-acao { background: #f0be00; color: black; border: none; padding: 8px 10px; font-weight: bold; border-radius: 5px; cursor: pointer; box-shadow: 0px 2px 4px rgba(0,0,0,0.5); font-size: 12px; flex: 1;}
        .btn-limpar { background: #d9534f; color: white; }
        .btn-console { background: #17a2b8; color: white; }

        /* CONTROLES E SENSORES */
        .control-section { flex: 1; display: flex; flex-direction: column; background: #1a1a1a; position: relative;}
        .painel-sensores { background: #222; text-align: center; padding: 12px 5px; font-size: 15px; border-bottom: 2px solid #333; display: flex; justify-content: space-evenly; align-items: center; flex-shrink: 0;}
        .sensor-val { font-weight: bold; color: #f0be00; }
        
        /* JOYSTICK */
        .joystick-wrapper { flex: 1; display: flex; align-items: center; justify-content: center; position: relative; width: 100%; padding: 10px; }
        #canvas_joystick { touch-action: none; } 

        /* OVERLAY DO CONSOLE & CONFIGURAÇÕES OCULTAS */
        .console-overlay { 
            display: none; 
            position: fixed; /* <-- Mudou de absolute para fixed */
            top: 0; left: 0; 
            width: 100vw; height: 100vh; /* <-- Garante 100% da tela física */
            background: rgba(0,0,0,0.95); 
            z-index: 9999; /* <-- Joga por cima de tudo */
            flex-direction: column; 
            padding: 15px;
            box-sizing: border-box;
        }
        .console-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #444; padding-bottom: 10px; margin-bottom: 10px;}
        .console-header span { color: #f0be00; font-weight: bold; font-family: monospace; }
        .btn-close { background: #dc3545; color: white; border: none; padding: 5px 15px; border-radius: 4px; font-weight: bold; cursor: pointer;}
        
        .hidden-settings { background: #222; padding: 10px; border-radius: 5px; margin-bottom: 10px; display: flex; flex-direction: column; gap: 8px;}
        .setting-row { display: flex; align-items: center; justify-content: space-between; font-size: 12px;}
        .setting-row input[type=range] { flex: 1; margin-left: 10px; }
        .setting-val { color: #0F0; font-weight: bold; width: 35px; text-align: right; }

        .seq-logger { flex: 1; overflow-y: auto; font-family: 'Courier New', Courier, monospace; font-size: 11px; color: #0f0; border-top: 1px solid #444; padding-top: 10px;}
        .log-entry { margin: 4px 0; border-bottom: 1px dotted #333; padding-bottom: 4px; line-height: 1.2;}
        .log-id { color: #f0be00; font-weight: bold; margin-right: 5px; }
    </style>
</head>
<body>

    <div id="consoleOverlay" class="console-overlay">
        <div class="console-header">
            <span>>_ PAINEL DE DESENVOLVEDOR</span>
            <button class="btn-close" onclick="toggleConsole()">Fechar</button>
        </div>
        
        <div class="hidden-settings">
            <div class="setting-row">
                <span>Vel. Joystick:</span>
                <input type="range" id="speedSlider" min="10" max="100" value="90" oninput="document.getElementById('lblVel').innerText = this.value + '%'">
                <span id="lblVel" class="setting-val">90%</span>
            </div>
            <div class="setting-row">
                <span>Calib. Odom:</span>
                <input type="range" id="odomSlider" min="10" max="120" value="50" onchange="send_calib('calib_odom', this.value)" oninput="document.getElementById('lblOdom').innerText = this.value">
                <span id="lblOdom" class="setting-val">60</span>
            </div>
            <div class="setting-row">
                <span>Amostras Filtro:</span>
                <input type="range" id="filterSlider" min="3" max="15" value="5" step="2" onchange="send_calib('calib_amostras', this.value)" oninput="document.getElementById('lblFilter').innerText = this.value">
                <span id="lblFilter" class="setting-val">9</span>
            </div>
            <div class="setting-row">
                <span>Raio Borracha:</span>
                <input type="range" id="eraserSlider" min="1" max="10" value="5" onchange="send_calib('calib_borracha', this.value)" oninput="document.getElementById('lblEraser').innerText = this.value">
                <span id="lblEraser" class="setting-val">2</span>
            </div>
            <div class="setting-row">
                <span>Range Ultrassom:</span>
                <input type="range" id="rangeSlider" min="50" max="200" value="100" onchange="send_calib('calib_range', this.value)" oninput="document.getElementById('lblRange').innerText = this.value + ' cm'">
                <span id="lblRange" class="setting-val">100 cm</span>
</div>
        </div>

        <div id="seqLogger" class="seq-logger">
            <div style="color: #666;">-- Aguardando Conexão --</div>
        </div>
    </div>

    <div class="header">
        <div style="display: flex; align-items: center;">
            <span style="font-weight:bold; color:#f0be00; font-size: 18px;">Vespa Nav</span>
            <span id="connStatus" class="status-pill">Desconectado</span>
        </div>
        <div class="battery-container">
            <div class="battery"><div id="lbat" class="part" style="width: 100%;"></div></div>
            <span id="vbat" style="font-size: 16px;">0</span>V
        </div>
    </div>

    <div class="map-wrapper">
        <div class="map-canvas-container">
            <canvas id="mapaCanvas" width="500" height="500"></canvas>
        </div>
        <div class="btn-panel">
            <button class="btn-acao btn-console" onclick="toggleConsole()">Ver Console</button>
            <button class="btn-acao btn-limpar" onclick="limparMapa()">Limpar Mapa</button>
            <button class="btn-acao" onclick="baixarMapa()">Baixar Mapa</button>
        </div>
    </div>

    <div class="control-section">
        <div class="painel-sensores">
            F:<span id="val_F" class="sensor-val">-</span> | 
            T:<span id="val_T" class="sensor-val">-</span> | 
            D:<span id="val_D" class="sensor-val">-</span> | 
            E:<span id="val_E" class="sensor-val">-</span>
        </div>
        
        <div class="joystick-wrapper" id="joy-wrap">
            <canvas id="canvas_joystick"></canvas>
        </div>
    </div>

    <script>
        var connection = new WebSocket(`ws://${window.location.hostname}/ws`);
        var statusUI = document.getElementById('connStatus');
        var loggerUI = document.getElementById('seqLogger');
        var consoleOverlay = document.getElementById('consoleOverlay');
        let ultimoEnvioJoystick = 0;
        
        function toggleConsole() {
            if (consoleOverlay.style.display === "flex") {
                consoleOverlay.style.display = "none";
            } else {
                consoleOverlay.style.display = "flex";
                loggerUI.scrollTop = loggerUI.scrollHeight;
            }
        }

        // Função universal para envio de pacotes de calibração
        function send_calib(chave, valor) {
            if(connection.readyState === WebSocket.OPEN) {
                var data = {};
                data[chave] = parseInt(valor);
                connection.send(JSON.stringify(data));
            }
        }

        connection.onopen = function() {
            statusUI.innerText = "Conectado";
            statusUI.style.background = "#0F0";
            statusUI.style.color = "black";
        };

        connection.onclose = function() {
            statusUI.innerText = "Desconectado";
            statusUI.style.background = "red";
            statusUI.style.color = "white";
        };

        function limparMapa() {
            if(connection.readyState === WebSocket.OPEN) {
                connection.send(JSON.stringify({"comando":"limpar"}));
                var canvasMapa = document.getElementById('mapaCanvas');
                var ctxMapa = canvasMapa.getContext('2d');
                ctxMapa.clearRect(0, 0, canvasMapa.width, canvasMapa.height);
            }
        }

        var canvasMapa = document.getElementById('mapaCanvas');
        var ctxMapa = canvasMapa.getContext('2d');

        connection.onmessage = function (e) {
            if (typeof e.data === "string" && e.data.startsWith("MAP:")) {
                atualizarMapaCanvasWS(e.data.substring(4));
                return;
            }

            try {
                const data = JSON.parse(e.data);
                
                if (data["seq_id"] !== undefined) {
                    var newLog = document.createElement("div");
                    newLog.className = "log-entry";
                    newLog.innerHTML = "<span class='log-id'>[" + data["seq_id"] + "]</span>" + data["seq_msg"];
                    loggerUI.appendChild(newLog);
                    if (consoleOverlay.style.display === "flex") {
                        loggerUI.scrollTop = loggerUI.scrollHeight;
                    }
                }

                if (data["vbat"]){
                    document.getElementById("vbat").innerText = (data["vbat"] / 1000).toFixed(1);
                    var lbat = (data["vbat"] * 100 / 9000).toFixed(0);
                    if(lbat > 100) lbat = 100;
                    if(lbat < 2) lbat = 2;
                    document.getElementById("lbat").style.width = lbat + '%';
                    if (lbat < 20) document.getElementById("lbat").style.backgroundColor = "#F00";
                    else if (lbat < 70) document.getElementById("lbat").style.backgroundColor = "orange";
                    else document.getElementById("lbat").style.backgroundColor = "#0F0";
                }
                if (data["F"] !== undefined) {
                    document.getElementById("val_F").innerText = data["F"];
                    document.getElementById("val_T").innerText = data["T"];
                    document.getElementById("val_D").innerText = data["D"];
                    document.getElementById("val_E").innerText = data["E"];
                }
            } catch(err) {}
        };

        var ultimoTempoEnvio = 0; // Variável para controlar o fluxo de dados

        function send_joystick(speed, angle){
            if(connection.readyState === WebSocket.OPEN) {
                
                // FILTRO ANTI-SPAM (Throttle)
                // Se for ordem para andar, limita o envio a cada 50 milissegundos (20Hz max)
                // Se for ordem para parar (speed == 0), ignora o bloqueio e envia na hora!
                var tempoAtual = Date.now();
                if (speed > 0 && (tempoAtual - ultimoTempoEnvio < 50)) {
                    return; // Aborta o envio se foi muito rápido
                }
                ultimoTempoEnvio = tempoAtual;

                var limit = document.getElementById('speedSlider').value / 100.0;
                var final_speed = Math.round(speed * limit);
                var data = {"velocidade":final_speed, "angulo":angle};
                connection.send(JSON.stringify(data));
            }
        }

        function atualizarMapaCanvasWS(textoMapa) {
            const gridSize = 100;
            const cellSize = canvasMapa.width / gridSize; 
            ctxMapa.clearRect(0, 0, canvasMapa.width, canvasMapa.height);
            
            let i = 0;
            for(let y = 0; y < gridSize; y++) {
                for(let x = 0; x < gridSize; x++) {
                    const char = textoMapa[i];
                    if (char === 'R') {
                        ctxMapa.fillStyle = 'red'; 
                        ctxMapa.fillRect(x * cellSize - 2, y * cellSize - 2, cellSize + 4, cellSize + 4);
                    } else if (char === '#') {
                        ctxMapa.fillStyle = '#0F0'; 
                        ctxMapa.fillRect(x * cellSize, y * cellSize, cellSize, cellSize);
                    }
                    i++;
                }
                i++; 
            }
        }

        function baixarMapa() {
            const link = document.createElement('a');
            link.download = 'mapa_vespa.png';
            link.href = canvasMapa.toDataURL('image/png');
            link.click();
            var data = {"comando": "baixar_imagem"};
            connection.send(JSON.stringify(data));
        }
    </script>

    <script>
        var canvas_joystick, ctx_joystick;
        window.addEventListener('load', () => {
            canvas_joystick = document.getElementById('canvas_joystick');
            ctx_joystick = canvas_joystick.getContext('2d');
            resize();
            
            canvas_joystick.addEventListener('mousedown', startDrawing, {passive: false});
            canvas_joystick.addEventListener('mouseup', stopDrawing, {passive: false});
            canvas_joystick.addEventListener('mousemove', Draw, {passive: false});
            canvas_joystick.addEventListener('touchstart', startDrawing, {passive: false});
            canvas_joystick.addEventListener('touchend', stopDrawing, {passive: false});
            canvas_joystick.addEventListener('touchcancel', stopDrawing, {passive: false});
            canvas_joystick.addEventListener('touchmove', Draw, {passive: false});
            
            window.addEventListener('resize', resize);
        });

        var width, height, radius;
        let origin_joystick = { x: 0, y: 0};
        const width_to_radius_ratio = 0.09; 
        const radius_factor = 3.5;
            
        function resize() {
            var container = document.getElementById('joy-wrap');
            width = container.clientWidth;
            height = container.clientHeight;
            let minDim = Math.min(width, height);
            
            radius = width_to_radius_ratio * minDim;
            ctx_joystick.canvas.width = width;
            ctx_joystick.canvas.height = height;
            origin_joystick.x = width / 2;
            origin_joystick.y = height / 2;
            joystick(origin_joystick.x, origin_joystick.y);
        }

        function joystick_background() {
            ctx_joystick.clearRect(0, 0, canvas_joystick.width, canvas_joystick.height);
            ctx_joystick.beginPath();
            ctx_joystick.arc(origin_joystick.x, origin_joystick.y, radius * radius_factor, 0, Math.PI * 2, true);
            ctx_joystick.fillStyle = '#ECE5E5';
            ctx_joystick.fill();
        }

        function joystick(x, y) {
            joystick_background();
            ctx_joystick.beginPath();
            ctx_joystick.arc(x, y, radius*1.5, 0, Math.PI * 2, true);
            ctx_joystick.fillStyle = '#f0be00';
            ctx_joystick.fill();
            ctx_joystick.strokeStyle = 'gray';
            ctx_joystick.lineWidth = 2;
            ctx_joystick.stroke();
        }

        let coord = { x: 0, y: 0 };
        let paint = false;
        var movimento = 0;

        function getPosition_joystick(event) {
            var rect = canvas_joystick.getBoundingClientRect();
            var mouse_x = event.clientX || event.touches[0].clientX || (event.touches[1] ? event.touches[1].clientX : 0);
            var mouse_y = event.clientY || event.touches[0].clientY || (event.touches[1] ? event.touches[1].clientY : 0);
            coord.x = mouse_x - rect.left;
            coord.y = mouse_y - rect.top;
        }

        function in_circle() {
            var current_radius = Math.sqrt(Math.pow(coord.x - origin_joystick.x, 2) + Math.pow(coord.y - origin_joystick.y, 2));
            return (radius * radius_factor) >= current_radius;
        }

        function startDrawing(event) {
            event.preventDefault(); 
            paint = true;
            getPosition_joystick(event);
            if (in_circle()) { joystick(coord.x, coord.y); Draw(event); }
        }

        function stopDrawing(event) {
            event.preventDefault();
            paint = false; 
            joystick(origin_joystick.x, origin_joystick.y);
            if (movimento == 1) { 
                send_joystick(0, 0); 
                movimento = 0; 
            }
        }

        function Draw(event) {
            event.preventDefault(); 
            if (paint) {
                getPosition_joystick(event);
                var angle = Math.atan2((coord.y - origin_joystick.y), (coord.x - origin_joystick.x));
                var x, y;
                if (in_circle()) {
                    x = coord.x; y = coord.y; 
                } else {
                    x = radius * radius_factor * Math.cos(angle) + origin_joystick.x; 
                    y = radius * radius_factor * Math.sin(angle) + origin_joystick.y; 
                }
                var speed = Math.round(100 * Math.sqrt(Math.pow(x - origin_joystick.x, 2) + Math.pow(y - origin_joystick.y, 2)) / (radius * radius_factor)); 
                if (speed > 100) speed = 100;
                var angle_in_degrees = Math.sign(angle) == -1 ? Math.round(-angle * 180 / Math.PI) : Math.round(360 - angle * 180 / Math.PI);
                joystick(x, y);
                send_joystick(speed, angle_in_degrees);
                movimento = 1;
            }
        }
    </script>
</body>
</html>
)rawliteral";

#endif