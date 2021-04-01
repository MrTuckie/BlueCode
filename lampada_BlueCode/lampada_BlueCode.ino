#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient


// BEZIN: Defina qual é o ID_MQTT e em qual tópico ele vai publicar.
 
#define ID_MQTT  "ledLA"     //id mqtt (para identificação de sessão)
//#define ID_MQTT  "ledLB"     //id mqtt (para identificação de sessão)

#define TOPICO_PUBLISH "ledLA"     //tópico MQTT para enviar o comando do LA
//#define TOPICO_PUBLISH "ledLB"     //tópico MQTT para enviar o comando do LB


// Esse daqui é comum para as duas lâmpadas
#define ledSubSA  "buttonSA" //tópico MQTT de envio de informações da lampada para Broker
#define ledSubSB  "buttonSB" //tópico MQTT de envio de informações da lampada para Broker
#define ledSubSC  "buttonSC" //tópico MQTT de envio de informações da lampada para Broker
 
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
#define LEDA  14
#define LEDB  12
#define LEDC  13

/*
 *    TODO: 
 *    - ADICIONAR TIMER PARA A LAMPADA DESLIGAR DEPOIS DE UM CERTO TEMPO SEM RECEBER
 *    ALGUMA MENSAGEM PELO CANAL
 *    -
 */
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
char enviado = '0';
bool enviadoBool = false;

//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
 

// Inicializa comunicação serial com baudrate 115200
void initSerial(){ 
  Serial.begin(115200);
}

 
//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
     
    reconectWiFi();
}
  
//Função: inicializa parâmetros de conexão MQTT(endereço do 
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
    MQTT.subscribe(ledSubSA);
    MQTT.subscribe(ledSubSB); 
    MQTT.subscribe(ledSubSC);  
}
  
void mqtt_callback(char* topic, byte* payload, unsigned int length){
    Serial.println("callback chamada");
    String msg;
    String sTopic = topic;
    int teste = 13;
    if (sTopic.equals("buttonSA")){
      teste = LEDA;
    }
    if (sTopic.equals("buttonSB")){
      teste = LEDB;
    }
    if (sTopic.equals("buttonSC")){
      teste = LEDC;
    }
    Serial.println(teste);
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) {
       char c = (char)payload[i];
       msg += c;
    }
    // Se uma das lâmpadas envia que recebeu o sinal, então o LE acende
    if (msg.equals("L")){
        digitalWrite(teste, HIGH);
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }
    // Se uma das lâmpadas desligou e enviou o status dela, então o LE desliga
    Serial.print(msg);Serial.println(" - recebido no canal");
    if (msg.equals("D")){
        digitalWrite(teste, LOW);
        if (!(digitalRead(LEDA) or digitalRead(LEDB) or digitalRead(LEDC))){
          digitalWrite(D4, LOW);
        }
        
        EstadoSaida = '0';
    }
}

  
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(ledSubSA);
            MQTT.subscribe(ledSubSB); 
            MQTT.subscribe(ledSubSC);  
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
void VerificaConexoesWiFIEMQTT(void){
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeitA
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

// talvez eu use uma variável para salvar a variação do status
void EnviaEstadoOutputMQTT(void){
    if (digitalRead(LEDA) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "LSA");
      Serial.println("LAMPADA LIGADA por SA");
      enviadoBool = true;
    }
    else if (!digitalRead(LEDA) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "DSA");
      Serial.println("LAMPADA DESLIGADA por SA");
      enviadoBool = true; 
    }    
    if (digitalRead(LEDB) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "LSB");
      Serial.println("LAMPADA LIGADA por SB");
      enviadoBool = true;
    }
    else if (!digitalRead(LEDB) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "DSB");
      Serial.println("LAMPADA DESLIGADA por SB");
      enviadoBool = true; 
    }
    if (digitalRead(LEDC) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "LSC");
      Serial.println("LAMPADA LIGADA por SC");
      enviadoBool = true;
    }
    else if (!digitalRead(LEDC) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "DSC");
      Serial.println("LAMPADA DESLIGADA por SC");
      enviadoBool = true; 
    }
    if (!digitalRead(D4) and enviado == '0'){
      MQTT.publish(TOPICO_PUBLISH, "D");
      Serial.println("LAMPADA DESLIGADA");
      enviadoBool = true;
    }
    if (enviadoBool){ 
      //Serial.println("ENVIADO");
      enviado = '1';
    }
    
}
 
void InitOutput(void){
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    pinMode(LEDA, OUTPUT);
    pinMode(LEDB, OUTPUT);
    pinMode(LEDC, OUTPUT);
    pinMode(D4, OUTPUT);

    digitalWrite(LEDA, LOW);
    digitalWrite(LEDB, LOW);
    digitalWrite(LEDC, LOW);
    digitalWrite(D4, LOW);          
}

// Timer variables
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 30000;  

void setup() {
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
}
  
//programa principal
void loop() {   
    //garante funcionamento das conexões WiFi e ao broker MQTT
    VerificaConexoesWiFIEMQTT();
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      //envia o status de todos os outputs para o Broker no protocolo esperado
      enviado = '0'; // reseta a variável de envio depois de um tempo
    }
    EnviaEstadoOutputMQTT();
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
    delay(500);
}
