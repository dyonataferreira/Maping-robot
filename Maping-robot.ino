#include <esp_arduino_version.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "variaveis.h"
#include "pagina_web.h"
#include "mpu_controle.h"
#include "movimento_robo.h"
#include "ultrassom_mapa.h"

// --- INSTANCIAÇÃO DAS VARIÁVEIS GLOBAIS ---
int matrizMapa[MAP_SIZE][MAP_SIZE];
const int TRIGS[4] = { 18, 5, 23, 19 };
const int ECHOS[4] = { 26, 25, 33, 32 };
float distanciasAtuais[4] = { 0, 0, 0, 0 };

float roboTheta = -PI / 2.0;
float roboX_cm = 250.0;
float roboY_cm = 250.0;
int roboX = 50;
int roboY = 50;

float fatorOdometria = 50.0;
int rangeUltrassom = 100;  // Valor padrão em cm
int qtdAmostras = 5;
int raioBorracha = 5;

int velE_global = 0;
int velD_global = 0;
uint32_t ultimoTempoOdom = 0;

SemaphoreHandle_t xMutexMapa;
VespaMotors motores;
VespaBattery vbat;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
const uint8_t PIN_LED = 15;
uint32_t timeout_vbat, timeout_sensores, timeout_mapa;
const uint32_t TEMPO_ATUALIZACAO_VBAT = 5000;

TaskHandle_t xTaskMapeamentoHandle;
TaskHandle_t xTaskMpuHandle;

const char *ALIAS_ANGULO = "angulo";
const char *ALIAS_VELOCIDADE = "velocidade";
const char *ALIAS_VBAT = "vbat";

EstadoRobo estadoAtual = ESTADO_IDLE;

char bufferMapa[10200];

int cmdVelocidade = 0;
int cmdAngulo = 0;
volatile bool comandoPendente = false;

// --- CAIXA DE CORREIO SEGURA PARA LOGS ---
char logPendenteId[10] = "";
char logPendenteMsg[64] = "";
bool temLogPendente = false;

void mudarEstado(EstadoRobo novoEstado) {
  if (estadoAtual == novoEstado) return;
  estadoAtual = novoEstado;
  if (estadoAtual == ESTADO_EMERGENCY) {
    motores.stop();
    velE_global = 0;
    velD_global = 0;
  }
}

// --- LOG DE EVENTOS BLINDADO (NÃO COLIDE COM O WI-FI) ---
// --- LOG DE EVENTOS BLINDADO PARA O PAINEL WEB ---
void logEvento(String id, String acao) {
  // 1. Mantém no cabo USB para backup e debug
  Serial.print("[");
  Serial.print(id);
  Serial.print("] ");
  Serial.println(acao);

  // 2. Monta o JSON rápido e envia direto para a página web
  if (ws.count() > 0 && ws.availableForWriteAll()) {
    StaticJsonDocument<128> jsonLog;
    jsonLog["seq_id"] = id;
    jsonLog["seq_msg"] = acao;
    
    size_t len = measureJson(jsonLog);
    char msgLog[len + 1];
    serializeJson(jsonLog, msgLog, len + 1);
    msgLog[len] = '\0'; // Garante o fim do texto para não corromper memória
    
    ws.textAll(msgLog);
  }
}
/*
void logEvento(String id, String acao) {
  Serial.print("[");
  Serial.print(id);
  Serial.print("] ");
  Serial.println(acao);
  // Apenas guarda na caixa de correio estática de forma segura
  //strncpy(logPendenteId, id.c_str(), sizeof(logPendenteId) - 1);
  //strncpy(logPendenteMsg, acao.c_str(), sizeof(logPendenteMsg) - 1);
  //temLogPendente = true;
}
*/
void TaskMPU(void *pvParameters) {
  for (;;) {
    mpu_loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t length) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == length && info->opcode == WS_TEXT) {
    
    // Leitura direta blindada, SEM corromper a memória
    StaticJsonDocument<128> json; 
    DeserializationError error = deserializeJson(json, data, length);

    if (!error) {
      // Comandos de Ação
      if (json.containsKey("comando")) {
        if (strcmp(json["comando"], "limpar") == 0) {
          if (xSemaphoreTake(xMutexMapa, pdMS_TO_TICKS(10)) == pdTRUE) {
            for (int i = 0; i < MAP_SIZE; i++) {
              for (int j = 0; j < MAP_SIZE; j++) matrizMapa[i][j] = 0;
            }
            xSemaphoreGive(xMutexMapa);
          }
          roboX_cm = 250.0; roboY_cm = 250.0; roboX = 50; roboY = 50;
          resetarAnguloMpu();
          logEvento("A06.5", "Sensores e Mapa reiniciados");
        } 
        else if (strcmp(json["comando"], "baixar_imagem") == 0) {
          logEvento("A06.6", "Download do Mapa solicitado");
        }
      }

      // Comandos de Calibração (Sliders)
      if (json.containsKey("calib_odom")) {
        fatorOdometria = json["calib_odom"];
        logEvento("A07.1", "Fator de Odometria: " + String(fatorOdometria));
      }
      if (json.containsKey("calib_amostras")) {
        int val = json["calib_amostras"];
        if (val >= 3 && val <= 15) qtdAmostras = val;
        logEvento("A07.2", "Filtro Ultrassom: " + String(qtdAmostras) + " amostras");
      }
      if (json.containsKey("calib_borracha")) {
        int val = json["calib_borracha"];
        if (val >= 1 && val <= 10) raioBorracha = val;
        logEvento("A07.3", "Raio da Borracha: " + String(raioBorracha));
      }
      if (json.containsKey("calib_range")) {
        int val = json["calib_range"];
        rangeUltrassom = val;
        logEvento("A07.4", "Range Ultrassom: " + String(rangeUltrassom) + "cm");
      }

      // Comando do Joystick (Levanta a bandeira para o loop agir com segurança)
      if (json.containsKey("velocidade")) {
        cmdAngulo = json["angulo"];
        cmdVelocidade = json["velocidade"];
        comandoPendente = true; 
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t length) {
  switch (type) {
    case WS_EVT_CONNECT:
      digitalWrite(PIN_LED, HIGH);
      if (ws.count() > 1) ws.close(client->id());
      mudarEstado(ESTADO_IDLE);
      logEvento("A01", "Dispositivo Conectado");
      break;
    case WS_EVT_DISCONNECT:
      if (ws.count() == 0) digitalWrite(PIN_LED, LOW);
      mudarEstado(ESTADO_EMERGENCY);
      logEvento("A01", "Dispositivo Desconectado");
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, length);
      break;
    default: break;
  }
}

void configurar_servidor_web(void) {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
}

void setup() {
  Serial.begin(115200);
  mpu_begin();
  mpu_calibrate(1000);
  currentTimeMPU = micros();

  xTaskCreatePinnedToCore(TaskMPU, "TaskMPU", 2048, NULL, 1, &xTaskMpuHandle, 0);

  xMutexMapa = xSemaphoreCreateMutex();
  for (int i = 0; i < 4; i++) {
    pinMode(TRIGS[i], OUTPUT);
    pinMode(ECHOS[i], INPUT);
  }

  if (xSemaphoreTake(xMutexMapa, portMAX_DELAY) == pdTRUE) {
    for (int i = 0; i < MAP_SIZE; i++) {
      for (int j = 0; j < MAP_SIZE; j++) matrizMapa[i][j] = 0;
    }
    xSemaphoreGive(xMutexMapa);
  }

  xTaskCreatePinnedToCore(TaskMapeamento, "TaskMapa", 4096, NULL, 1, &xTaskMapeamentoHandle, 0);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAPdisconnect();
  delay(100);
  WiFi.softAP("Vespa", "12345");

  const char *mac = WiFi.softAPmacAddress().c_str();
  char ssid[] = "Vespa-xxxxx";
  char *senha = "robocore";
  for (uint8_t i = 6; i < 11; i++) ssid[i] = mac[i + 6];

  WiFi.softAP(ssid, senha);

  configurar_servidor_web();
  server.begin();

  ultimoTempoOdom = millis();
  mudarEstado(ESTADO_IDLE);
}

void loop() {
  atualizarOdometria();

  // O núcleo principal executa os motores com segurança
  if (comandoPendente) {
    processarComandoJoystick(cmdVelocidade, cmdAngulo);
    comandoPendente = false; // Baixa a bandeira
  }

  if (WiFi.softAPgetStationNum() == 0 && estadoAtual != ESTADO_EMERGENCY) {
    mudarEstado(ESTADO_EMERGENCY);
  }

  // --- ATUALIZAÇÃO SENSORES ---
  if (millis() > timeout_sensores) {
    if (ws.count() > 0) {
      StaticJsonDocument<128> jsonSens;
      jsonSens["F"] = (int)distanciasAtuais[0];
      jsonSens["T"] = (int)distanciasAtuais[1];
      jsonSens["D"] = (int)distanciasAtuais[2];
      jsonSens["E"] = (int)distanciasAtuais[3];

      size_t len = measureJson(jsonSens);
      char msgSens[len + 1];
      serializeJson(jsonSens, msgSens, len + 1);
      msgSens[len] = 0;
      ws.textAll(msgSens);
    }
    timeout_sensores = millis() + 100;
  }

  // --- ENVIO DO MAPA ---
  // --- ENVIO DO MAPA (3Hz) À PROVA DE QUEDAS ---
  if (millis() > timeout_mapa) {
    ws.cleanupClients();

    if (ws.count() > 0) {
      if (xSemaphoreTake(xMutexMapa, pdMS_TO_TICKS(10)) == pdTRUE) {

        int idx = 0;
        bufferMapa[idx++] = 'M';
        bufferMapa[idx++] = 'A';
        bufferMapa[idx++] = 'P';
        bufferMapa[idx++] = ':';

        for (int i = 0; i < MAP_SIZE; i++) {
          for (int j = 0; j < MAP_SIZE; j++) {
            if (j == roboX && i == roboY) bufferMapa[idx++] = 'R';
            else if (matrizMapa[j][i] > 10) bufferMapa[idx++] = '#';
            else bufferMapa[idx++] = '.';
          }
          bufferMapa[idx++] = '\n';
        }
        xSemaphoreGive(xMutexMapa);
        bufferMapa[idx] = '\0';

        // CORREÇÃO: Usamos '& client' para criar uma referência ao objeto original
        // e '.' em vez de '->' porque 'client' agora é o próprio objeto, não um ponteiro.
        for (auto &client : ws.getClients()) {
          if (client.status() == WS_CONNECTED && client.canSend()) {
            client.text(bufferMapa, idx);
          }
        }
      }
    }
    timeout_mapa = millis() + 333;
  }

  // --- ATUALIZAÇÃO BATERIA ---
  if (millis() > timeout_vbat) {
    uint32_t tensao = vbat.readVoltage();
    if (tensao > 1000 && tensao < 6000 && estadoAtual != ESTADO_EMERGENCY) {
      mudarEstado(ESTADO_EMERGENCY);
      delay(2000);
      ESP.restart();
    }

    if (ws.count() > 0) {
      StaticJsonDocument<JSON_OBJECT_SIZE(1)> jsonBat;
      jsonBat[ALIAS_VBAT] = tensao;
      size_t len = measureJson(jsonBat);
      char msgBat[len + 1];
      serializeJson(jsonBat, msgBat, len + 1);
      msgBat[len] = 0;
      ws.textAll(msgBat);
    }
    timeout_vbat = millis() + TEMPO_ATUALIZACAO_VBAT;
  }

  vTaskDelay(pdMS_TO_TICKS(1));
}
