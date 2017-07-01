#include <SoftwareSerial.h>
#include <XBee.h>
#include <AESLib.h>

// Define SoftSerial TX/RX pins
// Connect Arduino pin 8 to TX of usb-serial device
uint8_t ssRX = 3;
// Connect Arduino pin 9 to RX of usb-serial device
uint8_t ssTX = 2;
// Remember to connect all devices to a common Ground: XBee, Arduino and USB-Serial device
SoftwareSerial nss(ssRX, ssTX);

XBee xbee = XBee();

int ledPin = 13;                      //Pin donde conectaremos el LED

int PIRpin = 12;                     //Pin donde recibiremos la info del PIR                //Varibale sincronizar escritura puerto Serial.
int calibrationTime = 30;             //Tiempo de calibración del sensor.
int auth = 0;
int msgcount = 0;
int timeHello = 5000;


//Data
uint8_t data        = 0;
uint8_t dataLength  = 0;
uint8_t challange[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0'}; // 0-8 posiciones ( length = 9 )
uint8_t nonce[]     = {'0', '0', '0', '0', '0', '0', '0', '0', '0'}; // 0-8 posiciones ( length = 9 )
uint8_t iv[]        = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}; // 9-24 posiciones ( length = 16 )
uint8_t MAC[]       = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}; //( length = 16 )

//Valores preconfigurados Sensor-Router
uint8_t key[] = {'9','7','5','2','5','9','1','5','5','8','1','2','3','5','8','4'}; //Key preconfigurada para el MAC
//uint8_t key[] = {57, 55, 53, 50, 53, 57, 49, 53, 53, 56, 49, 50, 51, 53, 56, 52}; 
uint8_t KEY_MASTER[] = {'7','6','5','4','2','1','0','8','5','8','1','2','3,','5','8','4'}; //KEY preconfigurada para los challange del primer hanshake
//uint8_t KEY_MASTER[] = {55, 54, 53, 52, 50, 49, 48, 56, 53, 56, 49, 50, 51, 53, 56, 52};
uint8_t IV_MASTER[] = {'5','6','8','1','9','7','2','8','1','5','7','9','6','3','8','2'}; //IVpreconfigurada para el cifrado
//uint8_t IV_MASTER[] = {53, 54, 56, 49, 57, 55, 50, 56, 49, 53, 55, 57, 54, 51, 56, 50};

int     nonceInt [] = { 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 };// 0-8 posiciones ( length = 9 )
int movimiento = 0;
int val = 0;
int valBefore = 0;

XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle
Rx64Response rx64 = Rx64Response();
Rx16Response rx16 = Rx16Response();
Tx16Request tx16 = Tx16Request();

void setup() {
  Serial.begin(9600);
  nss.begin(9600);
  xbee.begin(nss);
  delay(5000);

  movimiento = 0;
  pinMode(ledPin, OUTPUT);
  pinMode(PIRpin, INPUT);

  calibrationSensor();
  Serial.flush();

}

void loop() {
  if (auth == 0) dataSend(3, nonce);
  else {
    val = digitalRead(PIRpin);
    Serial.print("Val: ");
    Serial.println(val);
    Serial.print("BeforeVal: ");
    Serial.println(valBefore);
    if (val == 0) {
      Serial.println("Listening channel...");
      listenChannel(1000, 1000, 5); // We Send timeACk and timePacket integers to function
    }
    if (val == HIGH) {
      digitalWrite(ledPin, HIGH);
      if (movimiento == 0) {
        movimiento = 1;
        Serial.print("Movimiento Change:  ");
        Serial.print(movimiento);
        Serial.print('\n');
        dataSend(1, nonce ); //MOVE!
        valBefore = val;
      }

    } else {
      digitalWrite(ledPin, LOW);
      if (movimiento == 1) {
        movimiento = 0;
        Serial.print("Movimiento Change:  ");
        Serial.print(movimiento);
        Serial.print('\n');
        dataSend(2, nonce); // Not Move!
        valBefore = val;
      }

    }
  }
}

void calibrationSensor() {
  Serial.print("Calibrando...");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  } Serial.print("Calibrado");
  Serial.print('\n');
  flickerLED(3, 50);
}

void flickerLED (int times, int msec) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(msec);
    digitalWrite(ledPin, LOW);
  }
}

void dataSend (int op, uint8_t* data) {
  Serial.println("-----SEND INFO FUNCTION-----------");
  Serial.println("Operation: ");
  Serial.println(op);
  uint8_t datasend[13];

  int j = 0;
  Serial.println("Datasend:");
  if (op == 1) { //moveIt[]!
    uint8_t moveIt[]    = {'/', '0', '1', '/'}; // 0-3 posiciones
    Serial.println("moveIt msg created!");
    j = 0;
    for (int i = 0; i < sizeof(datasend); i++) {
      if (i < sizeof(nonce)) {
        datasend[i] = nonce[i];
        Serial.println(datasend[i]);
      }
      else {
        datasend[i] = moveIt[j];
        Serial.println(datasend[i]);
        j++;
      }
    }
    tx16 =  Tx16Request(0x1234, datasend, sizeof(datasend));
  } else if (op == 2) { //No hay movimiento
    uint8_t notMoveIt[] = {'/', '0', '2', '/'}; // 0-3 posiciones
    Serial.println("notMoveIt msg created!");
    j = 0;
    for (int i = 0; i < sizeof(datasend); i++) {
      if (i < sizeof(nonce)) {
        datasend[i] = nonce[i];
        Serial.println(datasend[i]);
      } else {
        datasend[i] = notMoveIt[j];
        Serial.println(datasend[i]);
        j++;
      }
    }
    tx16 = Tx16Request(0x1234, datasend, sizeof(datasend));
  } else if (op == 3) {
    Serial.println("authXBee msg created with challange!");
    uint8_t authXBee[]  = {'/', '0', '3', '/'}; // 0-3 posiciones
    getRandNumber();
    int j = 0;
    for (int i = 0; i < sizeof(datasend); i++) {
      if (i < sizeof(authXBee)) {
        datasend[i] = authXBee[i];
      } else {
        datasend[i] = challange[i-sizeof(authXBee)];
        j++;
      }
    }
    Serial.println("");
    Serial.println("DATASEND: ");
    for( int i = 0; i < sizeof(datasend); i++){
     Serial.print(datasend[i]); 
    }
    tx16 = Tx16Request(0x1234, datasend, sizeof(datasend));

  } else if (op == 4) { //Send Challange
    Serial.println("Send Challange");
    uint8_t sendChallange[]  = {'/', '0', '4', '/'}; // 0-3 posiciones
    uint8_t dataSendChallange[13];

    for (int i = 0; i < sizeof(dataSendChallange); i++) {
      dataSendChallange[i] = sendChallange[i];
      if (i >= 4)dataSendChallange[i] = data[i - 4];
    }
    tx16 = Tx16Request(0x1234, dataSendChallange, sizeof(dataSendChallange));
  } else if (op == 5) { //Hello packet
    Serial.println("hello msg created!");
    j = 0;
    uint8_t hello[]  = {'/', '0', '5', '/'}; // 0-3 posiciones
    for (int i = 0; i < sizeof(datasend); i++) {
      if (i < sizeof(nonce)) {
        datasend[i] = nonce[i];
        Serial.println('\n');
        Serial.print("Datasend: ");
        Serial.print(datasend[i]);
        Serial.println('\n');
      } else {
        datasend[i] = hello[j];
        Serial.print("Datasend: ");
        Serial.print(datasend[i]);
        Serial.println('\n');
        j++;
      }
    }
    tx16 = Tx16Request(0x1234, datasend, sizeof(datasend));
  }
  if (auth == 0) xbee.send(tx16);
  else if (auth == 1) {
    sendWithMAC(datasend);
  } else {
    Serial.println("ERROR");
  }
  listenChannel(1000, 1000, op); // We Send timeACk and timePacket integers to function
}
void viewDataRecievedx16 (XBee xbee, int lastOP) {
  if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) {

    if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
      xbee.getResponse().getRx16Response(rx16);

      Serial.println("PayloadLength:");
      Serial.println(rx16.getDataLength());
      Serial.println("Payload Data:");
      int i = 0;
      for (i = 0; i < rx16.getDataLength(); i++) {
        Serial.print(rx16.getData(i));
      }
      
      if (rx16.getDataLength() == 4)data = rx16.getData(3);

      else if (rx16.getData(0)==47 && rx16.getData(1)==61 && rx16.getData(2)==61 && rx16.getData(3)==47) {
        int same = 1;
        uint8_t challangeEncrypt [rx16.getDataLength() - 12];
        uint8_t challangeRouter[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0'};

        for (int i = 4; i < rx16.getDataLength() ; i++) {
          if ( i < rx16.getDataLength() - 8){
          challangeEncrypt[i-4] = rx16.getData(i);
          }else challangeRouter[i - rx16.getDataLength() + sizeof(challangeRouter) - 1] = rx16.getData(i);
        }
        Serial.println("\nChallange Encrypt(BEFORE): ");
        for (i = 0; i < sizeof(challangeEncrypt); i++) {
          Serial.print(challangeEncrypt[i]);
        }
        Serial.println("\nChallange Router: ");
        for (i = 0; i < sizeof(challangeRouter); i++) { 
          Serial.print(challangeRouter[i]);
        }
        
        aes128_cbc_dec(KEY_MASTER, IV_MASTER, challangeEncrypt, sizeof(challangeEncrypt));
        
        Serial.println("\nChallange Encrypt AFTER encrypt: ");
        for (i = 0; i < sizeof(challangeEncrypt); i++) {
          Serial.print(challangeEncrypt[i]);
        }
        
        for (int i = 0 ; i < sizeof(challange); i++) {
          if ( challange[i] != challangeEncrypt[i]) same = 0;
          Serial.println("\nChallange: ");
          Serial.println(challange[i]);
          Serial.println("\getData: ");
          Serial.println(challangeEncrypt[i]);
        }
        if (same == 1) {
          aes128_cbc_enc(KEY_MASTER, IV_MASTER, challangeRouter, sizeof(challangeRouter));
          dataSend(4, challangeRouter);
        }
        Serial.println("Same variable:");
        Serial.print(same);

      } else {
        Serial.println("NonceLength:");
        Serial.println(sizeof(nonce));
        Serial.println("Nonce:");
        for (i = 0; i < sizeof(nonce); i++) {
          nonce[i] = rx16.getData(i);
          Serial.println(rx16.getData(i));
        }
        Serial.println("i:");
        Serial.println(i);
        Serial.println("IV:");
        int j = 0;
        for (i = 9; i < sizeof(iv) + sizeof(nonce); i++) {
          iv[j] = rx16.getData(i);
          Serial.println(rx16.getData(i));
        }
        Serial.println("i:");
        Serial.println(i);
        Serial.println("Data: ");
        data = rx16.getData(rx16.getDataLength() - 1);
      }
      Serial.println(data);
      if (data == 49) auth = 1; // 49-> Auth
      if (data == 48) auth = 0; // 48->0 UnAuth
      if (data == 50) {
        dataSend(5, nonce);
      }
      Serial.println("Auth=");
      Serial.println(auth);
      msgcount++;
      Serial.println("msgCount=");
      Serial.println(msgcount);
      if (auth == 1) plusNonce(); //Solo nos hace falta cuando estamos autenticados

    } else {
      // Lo mismo pero poniendo 64 en lugar de 16
    }
  }
}

void listenChannel(int timeACK, int timePacket, int op) {
  Serial.println("--------RECEIVED INFO FUNCTION-----------");
  // Recieve the message
  if (xbee.readPacket(timeACK)) {
    //Esperamos ACk del XBEE
    Serial.println("received ACK!");
    if (xbee.readPacket(timePacket)) viewDataRecievedx16(xbee, op);
    else Serial.println("No received missage");
  } else {
    for (int i = 0; i < 5; i++) {
      xbee.send(tx16);
      if (xbee.readPacket(timeACK)) {
        if (xbee.readPacket(timePacket)) {
          viewDataRecievedx16 (xbee, op);
          i = 5;
        } else Serial.println("No received missage");
      } else Serial.println("No received ACK");
    }
    Serial.println("No response from radio");
  }
  Serial.println("--------RECEIVED INFO FUNCTION-----------FINISHED");
}


void sendWithMAC(uint8_t datasend[]) {
  Serial.println("Send data with MAC-----------");
  Serial.println("MAC:");
  for (int i = 0; i < sizeof(datasend); i++) {
    MAC[i] = datasend[i];
    Serial.println(MAC[i]);
  }
  aes128_cbc_enc(key, iv, MAC, sizeof(MAC));
  uint8_t datasendWithMAC[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
                              '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 
                              '0', '0', '0', '0', '0', '0', '0', '0', '0'}; //length=29
  Serial.println("datasendWithMAC:");
  for (int i = 0; i < sizeof(datasendWithMAC); i++) {
    if (i < 13) {
      datasendWithMAC[i] = datasend[i];
      Serial.println(datasendWithMAC[i]);
    } else {
      datasendWithMAC[i] = MAC[i - 13];
      Serial.println(datasendWithMAC[i]);
    }
  }
  tx16 = Tx16Request(0x1234, datasendWithMAC, sizeof(datasendWithMAC));
  xbee.send(tx16);
}
void getRandNumber() {
  for (int i = 0; i < sizeof(challange); i++) {
    randomSeed(millis());
    challange[i] = (char)random(48, 57);
    Serial.print(challange[i]);
  }

}
void plusNonce () {
  Serial.println("PLUSNONCE()-------------");
  //Convert nonce from uint8_t to integer.
  Serial.println("Size of NonceInt: ");
  Serial.println(sizeof(nonceInt));
  Serial.println("NonceInt: ");
  int i = 0;
  for (i = 0; i < sizeof(nonce); i++) {
    nonceInt[i] = (int) nonce[i];
    Serial.println(nonceInt[i]);
  }
  Serial.println("i: ");
  Serial.println(i);
  //Plus 1 to nonceInt: 48->0, 49->1, 50->2, 51->3, 52->4, 53->5, 54->6, 55->7, 56->8, 57->9.
  for (int j = sizeof(nonce) - 1; j >= 0; j--) { //Usamos J para movernos en el vector
    Serial.println("j:");
    Serial.println(j);
    int cont9 = 0;                        //Usamos cont9 para contar cuantos 9 hay por a partir de nuestra
    //posición para adelante (derecha a izquierda)
    if (j == sizeof(nonce) - 1 && nonceInt[j] == 57) {          // Localizamos si hay '9'
      for (int k = j; k >= 0; k--) {    //Bucle cuenta 9: Contamos las cifras de adelante son '9' también,
        //las almacenamos en la variable cont9
        if (nonceInt[k] == 57)  cont9++; //Por cada 9 (57 en uint8_), aumentamos cont9
        else break;                     //Si ya no encontramos otro 9, se sale del bucle cuenta 9
      }
      if (cont9 != 0) {                 // Si hay alguna cifra consecutiva más con un 9...
        for (int l = 0; l < cont9; l++) { // la convertimos en 0
          nonceInt[j - l] = 48;           // dicho 9 lo pasamos a 0(48 en uint8_t);
        }
      }
      nonceInt[j - cont9] = nonceInt[j - cont9] + 1; //La siguiente cifra despues del ultimo 9, que ahora es 0,
      //le sumamos 1.
      break;                                //Como ya hemos hecho la suma de todos los 9 -> 0
      //y el siguiente +1 salimos del bucle de sumar.
    }
    nonceInt[j] = nonceInt[j] + 1;           //Si no ha encontrado ningún 9 en las unidades, le suma traquilamente.
    break;   // salimos del rollo este
  }
  i = 0;
  //Convert nonceIntfrom integer to uint8_t
  Serial.println("Nonce: ");
  for (i = 0; i < sizeof(nonce); i++) {
    nonce[i] = (uint8_t) nonceInt[i];
    Serial.println(nonce[i]);
  }
}

