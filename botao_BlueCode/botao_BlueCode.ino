#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient


////////////////////////////////////// DEFINIÇÕES //////////////////////////////////////////

// Definições da topologia Pub/Sub
// Botões -> SA,SB e SC publicam cada um em um tópico MQTT
// Os botões estarão inscritos nos canais dos status de cada lâmpada
// SA -> Publica em buttonSA e checa o status da LA e LB em 2 canais
// SB -> Publica em buttonSB e //
// SC -> Publica em buttonSC e //


// Lâmpadas -> LA e LB dão Sub em cada um desses 3 tópicos MQTT
// As Lâmpadas vão publicar os status delas em canais diferentes também
// LA -> Publica em ledLA e ouve os 3 tópicos dos botões;
// LA vai publicar LSA para acender o LED em SA, DSA para apagar.
// Mas ele só vai fazer isso depois de checar o status no pino do LAA.
// Analogamente funciona para os outros componentes.
// LB -> Publica em ledLB e ouve os 3 tópicos dos botões



// BEZIN:
// Se quiser programar para os botões A,B ou C, deixe apenas os
// #defines que quer salvar.



//#define ID_MQTT  "buttonSA"     //id mqtt (para identificação de sessão)
//#define ID_MQTT  "buttonSB"     //id mqtt (para identificação de sessão)
#define ID_MQTT  "buttonSC"     //id mqtt (para identificação de sessão)

//#define TOPICO_PUBLISH "buttonSA"     //tópico MQTT para enviar o comando do SA
//#define TOPICO_PUBLISH "buttonSB"     //tópico MQTT para enviar o comando do SB
#define TOPICO_PUBLISH "buttonSC"     //tópico MQTT para enviar o comando do SC

//#define ACENDE "LSA"
//#define ACENDE "LSB"
#define ACENDE "LSC"

//#define APAGA "DSA"
//#define APAGA "DSB"
#define APAGA "DSC"


// Esse daqui é para qualquer botão, não precisa comentar nada.
#define buttonSubA  "ledLA" //tópico MQTT de envio de informações da lampada para Broker
#define buttonSubB  "ledLB" //tópico MQTT de envio de informações da lampada para Broker



// Esse daqui é para qualquer botão, não precisa comentar nada.
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2 // PINO DO LED ONBOARD
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1
#define LE    14 // LED EXTERNO
#define pino  5 // Vai acabar sendo o D1 de qualquer jeito.

// WIFI
const char* SSID = ""; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = ""; // Senha da rede WI-FI que deseja se conectar

// MQTT
const char* BROKER_MQTT = "broker.emqx.io"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída
bool statusAnt = 0;

//////////////////////////////////////   FUNÇÕES  //////////////////////////////////////////

// Inicializa comunicação serial com baudrate 115200
void initSerial(){ 
  Serial.begin(115200);
}

//Inicializa e conecta-se na rede WI-FI desejada
void initWiFi(){ 
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconectWiFi();
}

//Reconecta-se ao WiFi
void reconectWiFi(){  
 //se já está conectado a rede WI-FI, nada é feito. 
 //Caso contrário, são efetuadas tentativas de conexão
 if (WiFi.status() == WL_CONNECTED)
      return;
  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
  while (WiFi.status() != WL_CONNECTED){
      delay(100);
      Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
} 

//inicializa parâmetros de conexão MQTT
void initMQTT() {
  Serial.println("Inicializando o MQTT");
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
  MQTT.subscribe(buttonSubA); 
  MQTT.subscribe(buttonSubB);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length){
    Serial.println("callback chamada");
    String msg;
    
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) {
       char c = (char)payload[i];
       msg += c;
    }
    // Se uma das lâmpadas envia que recebeu o sinal, então o LE acende
    if (msg.equals(ACENDE)){
        digitalWrite(LE, HIGH);
        //EstadoSaida = '1';
    }
    
    // Se uma das lâmpadas desligou e enviou o status dela, então o LE desliga
    Serial.print(msg);Serial.println(" - recebido no canal");
    if (msg.equals(APAGA)){
        digitalWrite(LE, LOW);
        //EstadoSaida = '0';
    }
}

void reconnectMQTT() {
    while (!MQTT.connected()) {
      Serial.print("* Tentando se conectar ao Broker MQTT: ");
      Serial.println(BROKER_MQTT);
      if (MQTT.connect(ID_MQTT)) {
          Serial.println("Conectado com sucesso ao broker MQTT!");
          MQTT.subscribe(buttonSubA); 
          MQTT.subscribe(buttonSubB);
          Serial.println("Conectado aos tópicos dos status das lâmpadas"); 
      } 
      else{
          Serial.println("Falha ao reconectar no broker.");
          Serial.println("Havera nova tentatica de conexao em 2s");
          delay(2000);
      }
    }
}

void VerificaConexoesWiFIEMQTT(void){
    if (!MQTT.connected()) 
      reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
    reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

void publicaL(void){
    MQTT.publish(TOPICO_PUBLISH, "L");
    Serial.print("- L enviado ao tópico ");
    Serial.println(TOPICO_PUBLISH);
    delay(1000);
}
void publicaD(void){
      MQTT.publish(TOPICO_PUBLISH, "D");
      Serial.print("- D enviado ao tópico ");
      Serial.println(TOPICO_PUBLISH);
    //Serial.println("- Estado da saida D0 enviado ao broker!");
    delay(1000);
}

// Inicializando pinos
void InitInput() {
pinMode(pino, INPUT_PULLUP);    // Fazendo o pino ser de entrada.
pinMode(LE,OUTPUT);             // Setando o Pino do LED EXTERNO
}

// Timer variables
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 10000;  

void setup() {
  initSerial();
  InitInput();
  initWiFi();
  initMQTT();  
}

void loop() {
  VerificaConexoesWiFIEMQTT();
  delay(100);
   // Se o botão estiver pressionado, vai fazer o que tem que fazer
   unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      //envia o status de todos os outputs para o Broker no protocolo esperado
     if(digitalRead(pino) == LOW){
        publicaL();
        delay(1000);
      }
       else{
        publicaD();
        delay(1000);
      }
    }

  MQTT.loop();
  delay(500);
}
