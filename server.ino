/*--------------------------------------------------------------
https://startingelectronics.org/tutorials/arduino/ethernet-shield-web-server-tutorial/SD-card-web-server-image/
Requires index.htm, page2.htm and pic.jpg to be on the micro SD card in the Ethernet shield micro SD card socket.
--------------------------------------------------------------*/
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#define REQ_BUF_SZ   20

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 20);
EthernetServer server(80);        // create a server at port 80
File webFile;
char HTTP_req[REQ_BUF_SZ] = {0};  // HTTP rqst stored as null termed string
char req_index = 0;               // index into HTTP_req buffer

void setup() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  Serial.begin(115200);
  Serial.println("Initialize SD card");
  if (!SD.begin(4)) {
    Serial.println("ERROR - SD card fail");
    return;
  }
  if (!SD.exists("index.htm")) {
    Serial.println("ERROR no index.htm");
    return;
  }
  Ethernet.begin(mac, ip);  // initialize Ethernet device
  server.begin();           // start to listen for clients
}

void loop() {
  EthernetClient client = server.available();  // try to get client
  if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
          if (req_index < (REQ_BUF_SZ - 1)) {
            HTTP_req[req_index] = c;
            req_index++;
          }
          Serial.print(c);
          if (c == '\n' && currentLineIsBlank) {
            if (StrContains(HTTP_req, "GET / ") || StrContains(HTTP_req, "GET /index.htm")) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connnection: close");
                client.println();
                webFile = SD.open("index.htm");
            }
            else if (StrContains(HTTP_req, "GET /page2.htm")) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/html");
              client.println("Connnection: close");
              client.println();
              webFile = SD.open("page2.htm");
            }
            else if (StrContains(HTTP_req, "GET /pic.jpg")) {
              webFile = SD.open("pic.jpg");
              if (webFile) {
                client.println("HTTP/1.1 200 OK");
                client.println();
              }
            }
            if (webFile) {
              while(webFile.available()) client.write(webFile.read());
              webFile.close();
            }
            req_index = 0;
            StrClear(HTTP_req, REQ_BUF_SZ);
            break;
          }
          if (c == '\n') currentLineIsBlank = true;
          else if (c != '\r') currentLineIsBlank = false;
        }
      }
      delay(1);
      client.stop();
   }
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length) {
  for (int i = 0; i < length; i++) {
    str[i] = 0;
  }
}

// searches for the string sfind in the string str
// returns 1 if string found / returns 0 if not found
char StrContains(char *str, char *sfind) {
  char found = 0;
  char index = 0;
  char len;
  len = strlen(str);
  if (strlen(sfind) > len) return 0;
  while (index < len) {
    if (str[index] == sfind[found]) {
      found++;
      if (strlen(sfind) == found) return 1;
    }
    else found = 0;
    index++;
  }
  return 0;
}
