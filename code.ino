#include <SPI.h>
#include <WiFi101.h>

#define DEBUG

char apssid[] = "ZEV";
WiFiServer server(80);

boolean needCredentials = true;
boolean needWiFi = false;
String xnetwork;
String xpassword;

struct fields {
  byte type; // 0=combo, 1=radio, 2=text, 3+=custom
  String fieldPrompt;
  String fieldName;
  String textDefault; // used for text field
  byte numDefault; // default element for dropdown
  String heading; // displays before field
  String fieldPrompts[11]; // Only 10 alpha prompts are allowed, should be enough. If not specified, valid[] value is prompt. For fieldPrompts[], use valid[] range.
  byte valid[100]; // if valid[2] = 255, valid[0] and valid[1] are low and high range. otherwise, valid[0]-valid[100] specify individual selections as 1,4,7,12, etc., and set last element to 254
  bool returnPrompts; // if true, return value is fieldPrompt
};

const int numberFields = 12; // add one for zero element
fields settings[numberFields];

WiFiClient client;

void setup() {


  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

#ifdef DEBUG
  Serial.print("Creating access point named: ");
  Serial.println(apssid);
#endif

  // following is for testing ********************************************

  settings[0].fieldPrompt = "Weather Station Setup";
  settings[1].type = 0; // dropdown select
  settings[1].fieldPrompt = "Network SSID";
  settings[1].fieldName = "ssid";
  settings[1].numDefault = 0;
  settings[1].returnPrompts = true;
  int numSsid = WiFi.scanNetworks();
  if (numSsid > 9) numSsid = 9;
  for (byte x = 0; x < numSsid; x++) {
    String ss = WiFi.SSID(x);
    if (ss.length() == 0) settings[5].valid[x] = 254;
    else settings[1].fieldPrompts[x] = ss;
  }
  settings[1].valid[9] = 254;
  settings[2].type = 2;
  settings[2].fieldPrompt = "Password";
  settings[2].fieldName = "password";
  settings[3].type = 2;
  settings[3].fieldPrompt = "Weather Underground Key";
  settings[3].fieldName = "wukey";
  settings[4].type = 2;
  settings[4].fieldPrompt = "Postal Code";
  settings[4].fieldName = "pcode";
  settings[5].type = 0;
  settings[5].fieldPrompt = "Primary Wind Display";
  settings[5].fieldName = "pWind";
  settings[5].numDefault = 0;
  settings[5].valid[0] = 0;
  settings[5].valid[1] = 1;
  settings[5].valid[2] = 255;
  settings[5].fieldPrompts[0] = "Anemometer";
  settings[5].fieldPrompts[1] = "Weather Underground";
  settings[6].type = 0;
  settings[6].fieldPrompt = "Secondary Wind Display";
  settings[6].fieldName = "sWind";
  settings[6].numDefault = 0;
  settings[6].valid[0] = 0;
  settings[6].valid[1] = 2;
  settings[6].valid[2] = 255;
  settings[6].fieldPrompts[0] = "Weather Underground";
  settings[6].fieldPrompts[1] = "Anemometer";
  settings[6].fieldPrompts[2] = "None";
  settings[7].type = 0;
  settings[7].fieldPrompt = "Inside Temperature Sensor";
  settings[7].fieldName = "iTempSensor";
  settings[7].numDefault = 0;
  settings[7].valid[0] = 0;
  settings[7].valid[1] = 1;
  settings[7].valid[2] = 255;
  settings[7].fieldPrompts[0] = "Original";
  settings[7].fieldPrompts[1] = "BME280";
  settings[8].type = 0;
  settings[8].fieldPrompt = "Outside Temperature Sensor";
  settings[8].fieldName = "oTempSensor";
  settings[8].numDefault = 0;
  settings[8].valid[0] = 0;
  settings[8].valid[1] = 1;
  settings[8].valid[2] = 255;
  settings[8].fieldPrompts[0] = "Original";
  settings[8].fieldPrompts[1] = "BME280";
  settings[9].type = 0;
  settings[9].fieldPrompt = "Inside Humidity Display (BME280 Only)";
  settings[9].fieldName = "iHumidity";
  settings[9].numDefault = 0;
  settings[9].valid[0] = 0;
  settings[9].valid[1] = 1;
  settings[9].valid[2] = 255;
  settings[9].fieldPrompts[0] = "No";
  settings[9].fieldPrompts[1] = "Yes";
  settings[10].type = 0;
  settings[10].fieldPrompt = "Outside Humidity Display (BME280 Only)";
  settings[10].fieldName = "oHumidity";
  settings[10].numDefault = 0;
  settings[10].valid[0] = 0;
  settings[10].valid[1] = 1;
  settings[10].valid[2] = 255;
  settings[10].fieldPrompts[0] = "No";
  settings[10].fieldPrompts[1] = "Yes";
  settings[11].type = 0;
  settings[11].fieldPrompt = "Set Time from NTP Server (CPU Ugrade Only)";
  settings[11].fieldName = "iTime";
  settings[11].numDefault = 0;
  settings[11].valid[0] = 0;
  settings[11].valid[1] = 1;
  settings[11].valid[2] = 255;
  settings[11].fieldPrompts[0] = "No";
  settings[11].fieldPrompts[1] = "Yes";
  if (WiFi.beginAP(apssid) != WL_AP_LISTENING) {
#ifdef DEBUG
    Serial.println("Creating access point failed");
#endif
    while (true);
  }
  delay(1000);
  server.begin();
  printAPStatus();
}

void loop() {
  if (needCredentials) getCredentials();
  if (needWiFi) getWiFi();
}

void getCredentials() {
  client = server.available();
  if (client) {
    Serial.println("new client");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            sendRequestHeaders();
            sendHTMLHead();
            sendHTMLBody();
            sendHTMLFooter();
            break;
          }
          else currentLine = "";
        }
        else if (c != '\r') currentLine += c;
        if (c == ',' && currentLine.substring(0, 3) == "GET") {
          currentLine.replace("%20", "");
          for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
            String fieldName = settings[fieldIndex].fieldName;
            byte startPosition = currentLine.indexOf("?" + fieldName + "=") + fieldName.length() + 2;
            if (startPosition != -1) {
              byte endPosition = currentLine.indexOf("&", startPosition);
              String fieldValue = currentLine.substring(startPosition, endPosition);
              Serial.println();
              Serial.println(fieldValue);
            }
          }
          client.stop();
          WiFi.end();
          needCredentials = false;
          needWiFi = true;
        }
      }
    }
    client.stop();
#ifdef DEBUG
    Serial.println("client disconnected");
    Serial.println();
#endif
  }
}

void sendRequestHeaders() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
}

void sendHTMLHead() {
  client.println(F("<html>"));
  client.println(F("<head>"));
  // client.println(F("<style type=\"text/css\"> body {font-family: sans-serif; margin:50px; padding:20px; line-height: 250% } </style>"));
  client.println(F(".navbar,.table{margin-bottom:20px}.nav>li,.nav>li>a,article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary{display:block}.btn,.form-control,.navbar-toggle{background-image:none}.table,label{max-width:100%}.sub-header{padding-bottom:10px;border-bottom:1px solid #eee}.h3,h3{font-size:24px}.table{width:100%}table{background-color:transparent;border-spacing:0;border-collapse:collapse}.table-striped>tbody>tr:nth-of-type(2n+1){background-color:#f9f9f9}.table>caption+thead>tr:first-child>td,.table>caption+thead>tr:first-child>th,.table>colgroup+thead>tr:first-child>td,.table>colgroup+thead>tr:first-child>th,.table>thead:first-child>tr:first-child>td,.table>thead:first-child>tr:first-child>th{border-top:0}.table>thead>tr>th{border-bottom:2px solid #ddd}.table>tbody>tr>td,.table>tbody>tr>th,.table>tfoot>tr>td,.table>tfoot>tr>th,.table>thead>tr>td,.table>thead>tr>th{padding:8px;line-height:1.42857143;vertical-align:top;border-top:1px solid #ddd}th{text-align:left}td,th{padding:0}.navbar>.container .navbar-brand,.navbar>.container-fluid .navbar-brand{margin-left:-15px}.container,.container-fluid{padding-right:15px;padding-left:15px;margin-right:auto;margin-left:auto}.navbar-inverse .navbar-brand{color:#9d9d9d}.navbar-brand{float:left;height:50px;padding:15px;font-size:18px;line-height:20px}a{color:#337ab7;text-decoration:none;background-color:transparent}.navbar-fixed-top{border:0;top:0;border-width:0 0 1px}.navbar-inverse{background-color:#222;border-color:#080808}.navbar-fixed-bottom,.navbar-fixed-top{border-radius:0;position:fixed;right:0;left:0;z-index:1030}.nav>li,.nav>li>a,.navbar,.navbar-toggle{position:relative}.navbar{border-radius:4px;min-height:50px;border:1px solid transparent}.container{width:750px}.navbar-right{float:right!important;margin-right:-15px}.navbar-nav{float:left;margin:7.5px -15px}.nav{padding-left:0;margin-bottom:0;list-style:none}.navbar-nav>li{float:left}.navbar-inverse .navbar-nav>li>a{color:#9d9d9d}.navbar-nav>li>a{padding-top:10px;padding-bottom:10px;line-height:20px}.nav>li>a{padding:10px 15px}.navbar-inverse .navbar-toggle{border-color:#333}.navbar-toggle{display:none;float:right;padding:9px 10px;margin-top:8px;margin-right:15px;margin-bottom:8px;background-color:transparent;border:1px solid transparent;border-radius:4px}button,select{text-transform:none}button{overflow:visible}button,html input[type=button],input[type=reset],input[type=submit]{-webkit-appearance:button;cursor:pointer}.btn-primary{color:#fff;background-color:#337ab7;border-color:#2e6da4}.btn{display:inline-block;padding:6px 12px;margin-bottom:0;font-size:14px;font-weight:400;line-height:1.42857143;text-align:center;white-space:nowrap;vertical-align:middle;-ms-touch-action:manipulation;touch-action:manipulation;cursor:pointer;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;border:1px solid transparent;border-radius:4px}button,input,select,textarea{font-family:san-serif;font-size:inherit;line-height:inherit}input{line-height:normal}button,input,optgroup,select,textarea{margin:0;font:inherit;color:inherit}.form-control,body{font-size:14px;line-height:1.42857143}.form-horizontal .form-group{margin-right:-15px;margin-left:-15px}.form-group{margin-bottom:15px}.form-horizontal .control-label{padding-top:7px;margin-bottom:0;text-align:right}.form-control{display:block;width:100%;height:34px;padding:6px 12px;color:#555;background-color:#fff;border:1px solid #ccc;border-radius:4px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075);box-shadow:inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition:border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s}.col-xs-8{width:66.66666667%}.col-xs-3{width:25%}.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{float:left}.col-lg-1,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-md-1,.col-md-10,.col-md-11,.col-md-12,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-sm-1,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{position:relative;min-height:1px;padding-right:15px;padding-left:15px}label{display:inline-block;margin-bottom:5px;font-weight:700}*{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}body{font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;color:#333}html{font-size:10px;font-family:sans-serif;-webkit-text-size-adjust:100%"));
  client.println("<title>" + settings[0].fieldPrompt + "</title>");
  client.println(F("</head>"));
}

void sendHTMLBody() {
  byte xLow;
  byte xHigh;
  byte xFlag;
  String idVal;
  String xOpt;
  String sel;

  client.println("<body>");
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // note that 0 is used for title
    String fieldName = settings[fieldIndex].fieldName;
    if (settings[fieldIndex].heading != "") client.println("<h2>" + settings[fieldIndex].heading + "</h2>");
    switch (settings[fieldIndex].type) {
      case 0: // dropdown
        {
          client.print(settings[fieldIndex].fieldPrompt + "<br>");
          client.println("<select id=" + String("\"") + settings[fieldIndex].fieldName + "\">");
          if (settings[fieldIndex].valid[2] == 255) { // valid[0] and valid[1] specifiy low and high range
            xLow = settings[fieldIndex].valid[0];
            xHigh = settings[fieldIndex].valid[1];
            xFlag = true;
          }
          else // use all valid[] for specific options
          {
            xLow = 0;
            xHigh = 99;
            xFlag = false;
          }
          for (int option = xLow; option <= xHigh; option++) {
            if (xFlag) { // range
              if (option < 10 && settings[fieldIndex].fieldPrompts[option] != "") xOpt = settings[fieldIndex].fieldPrompts[option]; //only 10 alpha options allowed
              else xOpt = String(option); // after 10 or null value, use option as prompt
              idVal = option;
            }
            else { // specific options
              if (option < 10 && settings[fieldIndex].fieldPrompts[option] != "") xOpt = settings[fieldIndex].fieldPrompts[option]; //only 10 alpha options allowed
              else  xOpt = settings[fieldIndex].valid[option];
              if (settings[fieldIndex].returnPrompts == false) idVal = xOpt;
              else if (settings[fieldIndex].fieldPrompts[option] == "") break;
                else idVal = settings[fieldIndex].fieldPrompts[option]; // return fieldPrompt when options vary such as with SSID
              if (settings[fieldIndex].valid[option] == 254) break; // leave if last option
            }
            if (option == settings[fieldIndex].numDefault) sel = "selected"; // set default selection
            else sel = "";
            client.println("<option value=\"" + idVal + "\"" + sel + ">" + xOpt + "</option>");
          }
          client.println(F("</select>"));
          client.print(F("<br><br>"));
          break;
        }
      case 1: // radio
        { client.println("<form id = \"test\">");
          client.print(settings[fieldIndex].fieldPrompt + "<br>");
          for (int opt = 0; opt < 11; opt++) {
            if (settings[fieldIndex].fieldPrompts[opt] == "") break;
            //      client.print("<input type=\"radio\"" + String("name=\"r1\"")  + "value=\"" + String(opt) + "\">" + settings[fieldIndex].fieldPrompts[opt] + "<br>");
            client.print("<input type=\"radio\"name=\"" + settings[fieldIndex].fieldName + "\""  + "value=\"" + opt + "\"" + "id=\"" + settings[fieldIndex].fieldPrompts[opt] + "\">" + settings[fieldIndex].fieldPrompts[opt] + "<br>");
          }
          // client.print("<script>" + settings[fieldIndex].fieldName + "=document.getElementById('" + settings[fieldIndex].fieldName + "').value;</script>;");
          //  client.print(String("<script>") + settings[fieldIndex].fieldName + "=test.elements.namedItem('" + "sportscar" + "').value;</script>;");

          client.print(F("<br>"));
          client.println("</form>");

          break;
        }
      case 2: // text
        {
          client.print(settings[fieldIndex].fieldPrompt + "<br>");
          client.print("<input id=\"" + settings[fieldIndex].fieldName + "\"" + "value=\"" + settings[fieldIndex].textDefault + "\"><br><br>");
          break;
        }
    }
  }
  client.print(F("<br>"));
  client.print(F("<button type=\"button\" onclick=\"SendText()\">Save</button>"));
  client.println(F("</body>"));
}

void sendHTMLFooter() {
  client.println(F("<script>"));
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // **************************** apparently this is not necessary because we don't need javascript variables
    String fieldName = settings[fieldIndex].fieldName;
    //   client.println("var " + fieldName + " = document.querySelector('#" + fieldName + "');");
  }
  client.println(F("function SendText() {"));
  client.println("var nocache=\"&nocache=\" + Math.random() * 1000000;");
  client.println(F("var request = new XMLHttpRequest();"));
  client.print(F("var netText = \"&txt="));

  bool first = true;
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) {
    String fieldName = settings[fieldIndex].fieldName;
    if (first) client.print(F("?"));
    else client.print(F("\"&?"));
    first = false;
    client.print(fieldName + "=\"" + "+" + fieldName + ".value" + "+");
  }
  client.println(F("\"&,&end=end\";"));
  client.println(F("request.open(\"GET\",\"ajax_inputs\" +  netText + nocache, true);"));
  client.println(F("request.send(null);"));
  for (int fieldIndex = 1; fieldIndex < numberFields; fieldIndex++) { // clear fields
    String fieldName = settings[fieldIndex].fieldName;
    client.println(fieldName + ".value" + "='';");
  }
  client.println(F("}</script>"));
  client.println(F("</html>"));
  client.println();
}

void getWiFi() {
  if (xnetwork == "" or xpassword == "") {
    Serial.println("Invalid WiFi credentials");
    Serial.println("STOP!!!!!!!!!!");
    while (true);
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    WiFi.begin(xnetwork, xpassword);
    delay(10000);
  }
  Serial.println("WiFi connection successful");
  printWiFiStatus();
  needWiFi = false;
  delay(1000);
}

void printWiFiStatus() {
#ifdef DEBUG
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
#endif
}

void printAPStatus() {
#ifdef DEBUG
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("To connect, open a browser to http://");
  Serial.println(ip);
#endif
}
