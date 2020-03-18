#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include "FS.h"

const byte analogica = 0; //AO
const byte botonReset = 16; //D0
const byte ledRed = 5; // D1
const byte ledGreen = 4; //D2
const byte ledBlue = 0; //D3
bool resetSensor = true;
bool wifiConectado = false;
int contador = 0;
int red = 10;
int green = 150;
int blue = 22;

const char *filename = "/datos.json";
char SERVER[50] = "IPSERVER";
char PASSWORD[50] = "PASS";
String USERNAME = "USER";
int SERVERPORT = 12345;
char COMANDO[255];
char arrayValor[512];
char PLACA[50];
//Agregar switch ON/OFF para encender el esp que corte la alimentacion
//Agregar boton de encendido y de reset
//Agregar led rgb para controlar los estatus

/*Para enviar datos de la pagina la ESP debe ser en formato JSON ejemplo 
{"OPCION":1} -> Buscar redes
{"OPCION":2,"SSID":"","PASS":""} -> Enviar credenciales
*/
/*Para obtener los datos del ESP en la pagina debe ser en formato JSON ejemplo
{"OPCION":1,"DATA":[{"SSID":"IZZI"},{"SSID":"BIOSINSA"}]} -> Redes obtenidas
{"OPCION":2,"DATA": "Conexion a la red exitosa! Ahora nos conectaremos al servidor para hacer un test de conexion..."} -> Conectado a la red
{"OPCION":3,"DATA": "Error al conectarse a la red"} -> No se pudo conectar a la red
{"OPCION":4,"DATA": "Conexion el servidor exitosa! Se reiniciará el dispositivo."} -> Test de conexion a MQTT
{"OPCION":5,"DATA": "Error al conectarse al servidor, por favor contacte al administrador del server."} -> Error en el test de conexion a MQTT
*/
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
static const char PROGMEM INDEX_HTML[] = R"(
<!DOCTYPE html>
    <html lang="es">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, user-scalable=no">
        <meta http-equiv="X-UA-Compatible" content="ie=edge">
        <title>TrapEbio Configuración</title>
    </head>
    <style>
        *{
            font-family: Arial, Helvetica, sans-serif;
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        header{
            background-color: rgb(59, 59, 59 );
            color: rgb(250, 250, 250);
            padding: 20px 20px;
        }
        .contenedor{
            width: 400px;
            margin: 0 auto;
            margin-top: 30px;
        }
        .card{
            background-color: rgb(250, 250, 250);
            border: 1px solid rgb(229, 229, 229);
            padding: 15px;
        }
        .campo{
            width: 100%;
            height: 35px;
            border-radius: 7px;
            border: 1px solid rgb(238, 238, 238);
            margin-top: 25px;
            padding: 5px;
        }
        .fila{
            display: flex;
            flex-wrap: wrap;
        }
        .modal{
            left: 0px;
            top: 0px;
            background-color: rgb(59, 59, 59, 0.5 );
            position: fixed;
            z-index: 1000;
            width: 100%;
            height: 100%;
            transition: 0.5s;
            opacity: 0;
            visibility: hidden;
        }
        .listaWifi{
            list-style: none;
            text-align: center;
            margin-top: 10px;
        }
        li{
            font-size: 13px;
            padding: 8px;
            background-color: rgb(234, 234, 234);
            margin-top: 5px;
        }
    </style>
    <body> 
    <body>
        <header>
            <h3>Conectarse a una red</h3>
        </header>
        <div class="fila">
            <div class="contenedor">
                <form id="formulario" class="card">
                    <h3 style='color:rgb(37, 37, 37)'>Agregar nombre de la red(SSID) y contraseña</h3>
                    <input class="campo" type="text" id="ssid" placeholder="Nombre de la red" required>
                    <input class="campo" type="password" id="password" placeholder="Contraseña" required autocomplete="off">
                    <button class="campo" type="submit">Conectar</button>
                </form>
            </div>
            <div class="contenedor">
                <button class="campo" id="escanear" type="button">Escanear redes</button>
                <div id="respuesta">
                </div>
            </div>
        </div>
        <div id="modal" class="modal">
            <div class="contenedor">
                <div class="card">
                    <p id="mensaje"></p>
                    <button class="campo" id="cerrarModal" type="button" style="display: none">Aceptar</button>
                </div>
            </div>
        </div>
        <script>
            const ssid = document.querySelector('#ssid');
            const password = document.querySelector('#password');
            const conectar = document.querySelector('#formulario');
            const escanear = document.querySelector('#escanear');
            const respuesta = document.querySelector('#respuesta');
            const modal = document.querySelector('#modal');
            const mensaje = document.querySelector('#mensaje');
            const cerrarModal = document.querySelector('#cerrarModal');

            escanear.addEventListener('click', ()=>{
                let dato = {};
                mensaje.textContent = "Escaneando redes...";
                modal.style.opacity = 1;
                modal.style.visibility = 'visible';
                dato.OPTION = 1;
                ajax('EscanearRedes', dato);
                console.log(JSON.stringify(dato));
            });

            cerrarModal.addEventListener('click', ()=>{
                modal.style.opacity = 0;
                modal.style.visibility = 'hidden';
                cerrarModal.style.display = 'none';
            });

            conectar.addEventListener('submit', (e)=>{
                let dato = {};   
                mensaje.textContent = "Conectando a la red espere...";     
                modal.style.opacity = 1;
                modal.style.visibility = 'visible';
                e.preventDefault();
                dato.OPTION = 2;
                dato.SSID = ssid.value;
                dato.PASS = password.value;
                ajax('ConectarRed', dato);
                console.log(JSON.stringify(dato));
            });

            function agregarRed(red){
                ssid.value = red.textContent;
            }

            function ajax(url, datos) {
                if(window.XMLHttpRequest) {
                    connection = new XMLHttpRequest();
                }
                else if(window.ActiveXObject) {
                    connection = new ActiveXObject("Microsoft.XMLHTTP");
                }                
                connection.onreadystatechange = response;
                connection.open('POST', url, true);
                connection.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
                connection.send(JSON.stringify(datos));
            }
                
            function response() {
                if(connection.readyState == 4) {
                    try{
                        let data = connection.responseText;
                        if(JSON.parse(data)){
                            let respuestaJSON = JSON.parse(data);
                            console.log(respuestaJSON);
                            if(respuestaJSON.OPTION == 1){
                                if(respuestaJSON.DATA.length > 0){
                                    let lista = "<ul class='listaWifi'>";
                                    for(i in respuestaJSON.DATA){
                                        lista += `<li onclick='agregarRed(this)'>${respuestaJSON.DATA[i]}</li>`;
                                    }
                                    lista += "</ul>";
                                    respuesta.innerHTML = lista;
                                }
                                modal.style.opacity = 0;
                                modal.style.visibility = 'hidden';           
                            }else if(respuestaJSON.OPTION == 2 || respuestaJSON.OPTION == 4){
                                mensaje.textContent = respuestaJSON.DATA;
                                cerrarModal.style.display = 'block';
                            }else if(respuestaJSON.OPTION == 3){
                                mensaje.textContent = respuestaJSON.DATA;
                            }                        
                        }
                    }catch(error){
                        mensaje.textContent = "Error al conectarse a la red";
                        cerrarModal.style.display = 'block';
                    }
                }
            }       
        </script>
    </body>
</html>
)";

void paginaPrincipal(){
    server.send(200, "text/html", INDEX_HTML);
}

void setup() {
    Serial.begin(115200);
    pinMode(botonReset, INPUT);
    pinMode(ledRed, OUTPUT);
    pinMode(ledGreen, OUTPUT);
    pinMode(ledBlue, OUTPUT);
    setColorBlank();

    //Si entra aqui es porque se encendio con el switch
    if(analogRead(0) > 100){
        resetSensor = false;
    }

    if(!SPIFFS.begin()){
        Serial.println("Fallo montando el documento");
    }
    //Codigo para formatear la memoria spiffs
    while(digitalRead(botonReset)){ 
        ++contador;
        delay(500);
        if(contador == 10){
            //Led RGB en Celeste
            setColor(1023, 747, 957);
            Serial.println("Formatear SPIFFS...");
            guardarDatos(filename, "{\"SSID\":\"\",\"PASS\":\"\"}");
            delay(500);
            Serial.println("Resetear placa...");
            ESP.reset();
        }       
    }
    
    String datosMemoria = obtenerDatosMemoria(filename);
    Serial.println(datosMemoria);
    DynamicJsonDocument doc(1024);
    JsonObject obj = doc.to<JsonObject>();
    DeserializationError error = deserializeJson(doc, datosMemoria);
    if(error){
        Serial.println("Fallo al leer el archivo para editar");
    }
    if(obj["SSID"] != "" && obj["PASS"] != ""){
        WiFi.mode(WIFI_STA);        
        if(conectarWifi(obj["SSID"], obj["PASS"], 50)){            
            if(resetSensor){
                Serial.println("Se activo con sensor");
                wifiConectado = true;
                client.setServer(SERVER, SERVERPORT);
                client.setCallback(callback);
                String comando = "Comando"; 
                comando.toCharArray(COMANDO, 255);
            }else{
                Serial.println("Se activo normal");
                ESP.deepSleep(0, WAKE_RF_DEFAULT);
            }
        }else{
            //Led RGB en Rojo
            setColor(851, 1023, 896);
            Serial.println("Error al conectarse a una red");
            delay(5000);
            ESP.deepSleep(0, WAKE_RF_DEFAULT);
        }
    }else{        
        WiFi.mode(WIFI_AP_STA);
        //Led RGB en Azul
        setColor(1020, 988, 321);
        WiFi.softAP("BIOSMART-1", "12345678");
        IPAddress myIP = WiFi.softAPIP();
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        Serial.print("Direccion IP del acceso: ");
        Serial.println(myIP);
        server.on("/", paginaPrincipal);
        server.on("/EscanearRedes", escanearRedes);
        server.on("/ConectarRed", conectarRed);
        server.begin();
        Serial.println("WebServer iniciado...");
    }
}

void escanearRedes(){
    StaticJsonDocument<1024> jsonSend;
    String datosObtenidos = server.arg("plain");
    Serial.println(datosObtenidos);
    int numeroRedes = WiFi.scanNetworks();
    if(numeroRedes > 0){                    
        jsonSend["OPTION"] = 1;
        JsonArray data = jsonSend.createNestedArray("DATA");                    
        for(int i = 0; i < numeroRedes; ++i){
            StaticJsonDocument<200> objeto;
            objeto["SSID"] = WiFi.SSID(i);
            data.add(objeto["SSID"]);
            delay(10);
        }                    
    }else{
        jsonSend["OPTION"] = 2;
        jsonSend["DATA"] = "No hay redes cercanas";
    }  
    String respuesta;
    serializeJson(jsonSend, respuesta);
    Serial.println(respuesta);
    server.send(200, "text/plain", respuesta);
}

void conectarRed(){
    String datosObtenidos = server.arg("plain");
    DynamicJsonDocument jsonGet(1024);
    JsonObject obj = jsonGet.to<JsonObject>();
    DeserializationError error = deserializeJson(jsonGet, datosObtenidos);
    if(error){
        Serial.println("Fallo al leer el archivo para editar");
    }
    Serial.println("Conectando a "+obj["SSID"].as<String>()+" con la contraseña "+obj["PASS"].as<String>());
    StaticJsonDocument<1024> jsonSend;
    if(conectarWifi(obj["SSID"], obj["PASS"], 20)){
        jsonSend["OPTION"] = 3;
        jsonSend["DATA"] = "Conexion a la red exitosa! La red se ha guardado correctamente, por favor reinicie el dispositivo...";
        guardarDatos(filename, "{\"SSID\":\""+obj["SSID"].as<String>()+"\",\"PASS\":\""+obj["PASS"].as<String>()+"\"}");
    }else{
        jsonSend["OPTION"] = 4;
        jsonSend["DATA"] = "Error al conectarse a la red";
    }
    String respuesta;
    serializeJson(jsonSend, respuesta);
    Serial.println(respuesta);
    server.send(200, "text/plain", respuesta);
}

void loop() {    
    if(wifiConectado){
        if(!client.connected()) {
            //Led RGB en Morado
            setColor(759, 1023, 896);
            reconnect();
            Serial.println("Sin conexion MQTT");
            delay(2000);
        }
        client.loop();
    }else{
        server.handleClient();
        if(digitalRead(botonReset)){
            Serial.println("Resetear placa...");
            delay(1000);
            ESP.reset();
        }
    }    
}

bool conectarWifi(const char* ssid, const char* pass, int limiteConteo){
    int conteoWifi = 0;
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED) {
        if(conteoWifi == limiteConteo){
            return false;
        }
        ++conteoWifi;
        delay(400);
        Serial.print(".");
    }
    Serial.print("");
    Serial.print("WiFi conectado");
    Serial.print(WiFi.localIP());
    return true;
}

void guardarDatos(const char *filename, String datos){
    Serial.println(datos);
    SPIFFS.format();
    File file2 = SPIFFS.open(filename, "a+");
    if(!file2){
        Serial.println("Fallo al crear archivo");
    }
    file2.print(datos);
    file2.close();
}

String obtenerDatosMemoria(const char *filename){
    String datosSpiffs = "";
    File file = SPIFFS.open(filename, "r");
    if(!file){
        Serial.println("Fallo al crear archivo");
    }
    while(file.available()){
        datosSpiffs += (char)file.read();
    }
    return datosSpiffs;
}

void setColorBlank(){
    digitalWrite(ledRed, 1);
    digitalWrite(ledGreen, 1);
    digitalWrite(ledBlue, 1);
}

void setColor(int r, int g, int b){
    analogWrite(ledRed, r);
    analogWrite(ledGreen, g);
    analogWrite(ledBlue, b);
}

void callback(char* topic, byte* payload, unsigned int length) {
  char PAYLOAD[255] = "    ";
  Serial.print("Mensaje Recibido: [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    PAYLOAD[i] = (char)payload[i];
  }
  Serial.println(PAYLOAD);
}

void reconnect() {
    uint8_t retries = 10;
    while(!client.connected()){
        Serial.print("Intentando conexion MQTT...");
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        USERNAME.toCharArray(PLACA, 50);
        if(client.connect("", PLACA, PASSWORD)) {
            Serial.println("Conectado a mqtt MQTT");
            client.subscribe(COMANDO);
            //Led RGB en Verde
            setColor(1023, 618, 1023);
            delay(2000);
            String MAC = WiFi.macAddress();
            StaticJsonDocument<512> jsonSend;
            jsonSend["ID"] = MAC;
            String respuesta;
            serializeJson(jsonSend, respuesta);
            respuesta.toCharArray(arrayValor, 512);
            client.publish(COMANDO, arrayValor);
            Serial.println(respuesta);            
            ESP.deepSleep(0, WAKE_RF_DEFAULT);
            Serial.println("Dormido");
        }else{
            Serial.print("fallo, rc=");
            Serial.print(client.state());
            Serial.println(" intenta nuevamente en 2 segundos");
            delay(2000);
        }
        retries--;
        if(retries == 0){
            //Led RGB en Naranja
            setColor(638, 984, 1023);
            delay(5000);
            ESP.deepSleep(0, WAKE_RF_DEFAULT);
        }
    }
}
