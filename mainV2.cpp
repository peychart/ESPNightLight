#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ESP8266HTTPUpdateServer.h>

short ResetConfig = 4;     //Just change this value to reset current config on the next boot...
#define DEFAULTWIFIPASS    "defaultPassword"
#define MAXWIFIERRORS      10
#define SSIDMax()          3

#define MAXVALUE             128

//Avoid to change the following:
String hostname = "ESP8266";//Can be change by interface
String ssid[SSIDMax()];     //Identifiants WiFi /Wifi idents
String password[SSIDMax()]; //Mots de passe WiFi /Wifi passwords
bool WiFiAP=false;
unsigned short nbWifiAttempts=MAXWIFIERRORS, WifiAPDelay;

enum {blue=0, green, red};
enum {none=-1, roundFlashingBlue, roundFlashingGreen, roundFlashingRed, roundFlashingMulti, allFlashingBlue=6, allFlashingGreen, allFlashingRed};
short prog=none;
#define outputCount()      3
#define ABS(v)  (((v)<0) ? -(v) : (v))

struct {
  unsigned char value[3]={0,0,0};
  bool          incr[3];
  unsigned int  pin[3];
} output[outputCount()];

ESP8266WebServer server(80); //Instanciation du serveur port 80
ESP8266WebServer updater(8081);
ESP8266HTTPUpdateServer httpUpdater;


String getMyMacAddress(){
  char ret[18];
  uint8_t mac[6]; WiFi.macAddress(mac);
  sprintf(ret, "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return ret;
}

String getValues(){return "0,1,2,3,4,5";}   //Coming soon...

String ultos(unsigned long v){ char ret[11]; sprintf(ret, "%ld", v); return ret; }
String  getPage(){
  String page="<html lang='us-US'><head><meta charset='utf-8'/>\n<title>" + hostname + "</title>\n";
  page += "<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n";
  page += " ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n";
  page += " td{text-align:left; min-width:110px; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n";
  page += " .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n";
  page += " .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n";
  page += " .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n";
  page += " .delayConf{float: left; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n";
  page += " .duration{width:50px;}\n";
  page += "  //see: https://proto.io/freebies/onoff/:\n";
  page += " .onoffswitch {position: relative; width: 90px;-webkit-user-select:none; -moz-user-select:none; -ms-user-select: none;}\n";
  page += " .onoffswitch-checkbox {display: none;}\n";
  page += " .onoffswitch-label {display: block; overflow: hidden; cursor: pointer;border: 2px solid #999999; border-radius: 20px;}\n";
  page += " .onoffswitch-inner {display: block; width: 200%; margin-left: -100%;transition: margin 0.3s ease-in 0s;}\n";
  page += " .onoffswitch-inner:before, .onoffswitch-inner:after {display: block; float: left; width: 50%; height: 30px; padding: 0; line-height: 30px;font-size: 14px; color: white; font-family: Trebuchet, Arial, sans-serif; font-weight: bold;box-sizing: border-box;}\n";
  page += " .onoffswitch-inner:before{content: 'ON';padding-left: 10px;background-color: #34C247; color: #FFFFFF;}\n";
  page += " .onoffswitch-inner:after {content: 'OFF';padding-right: 10px;background-color: #EEEEEE; color: #999999;text-align: right;}\n";
  page += " .onoffswitch-switch{display: block; width: 18px; margin: 6px;background: #FFFFFF;position: absolute; top: 0; bottom: 0;right: 56px;border: 2px solid #999999; border-radius: 20px;transition: all 0.3s ease-in 0s;}\n";
  page += " .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {margin-left: 0;}\n";
  page += " .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {right: 0px;}\n";
  page += "</style></head>\n<body onload='init();'>\n";
  page += "<script>\nthis.timer=0;\n";
  page += "function init(){var e;\n";
  page += "e=document.getElementById('example1');\ne.innerHTML=document.URL+'[" + getValues() + "]'; e.href=e.innerHTML;\n";
  page += "e=document.getElementById('example2');\ne.innerHTML=document.URL+'status'; e.href=e.innerHTML;\n";
  page += "refresh();}\n";
  page += "function refresh(v=30){clearTimeout(this.timer); document.getElementById('about').style.display='none';\n";
  page += "this.timer=setTimeout(function(){getStatus(); refresh(v);}, v*1000);}\n";
  page += "function getStatus(){var jchar, ret, e, req=new XMLHttpRequest(); req.open('GET', document.URL+'status', false); req.send(null); ret=req.responseText;\n";
  page += "if((j=ret.indexOf('[')) >= 0){\n";
  page += "if((e=document.getElementsByClassName('onoffswitch-checkbox')).length && (j=ret.indexOf('['))>=0)\n";
  page += " for(var e,v,i=0,r=ret.substr(j+1); r[0] && r[0]!=']'; i++){\n";
  page += "  j=r.indexOf(',');if(j<0) j=r.indexOf(']'); v=parseInt(r.substr(0,j));\n";
  page += "  if(v>=0) e[i].checked=(v?true:false); r=r.substr(j+1);\n";
  page += " }if((e=document.getElementsByClassName('onoffswitch-checkbox')).length)\n";
  page += "  for(var e,v,i=0,ret=r.substr(j+1); r[0] && r[0]!=']'; i++){\n";
  page += "   j=r.indexOf(',');if(j<0) j=r.indexOf(']'); v=parseInt(r.substr(0,j));\n";
  page += "   if(v>=0) e[i].checked=(v?true:false); r=r.substr(j+1);\n";
  page += "}}}\n";
  page += "function showHelp(){refresh(120); document.getElementById('about').style.display='block';}\n";
  page += "function saveSSID(f){\n";
  page += "if((f=f.parentNode)){var s, p=false;\n";
  page += "for(var i=0; i<f.children.length; i++){\n";
  page += "if(f.children[i].type=='password'){\n";
  page += "if (!p) p=f.children[i];\n";
  page += "else if(p.value!=f.children[i].value) p.value='';\n";
  page += "}else if(f.children[i].type=='text') s=f.children[i];\n";
  page += "}if(s.value==''){\n";
  page += "alert('Empty SSID...'); f.reset(); s.focus();\n";
  page += "}else if(p.value==''){\n";
  page += "var ssid=s.value; f.reset(); s.value=ssid;\n";
  page += "alert('Incorrect password...'); p.focus();\n";
  page += "}else f.submit();\n";
  page += "}}\n";
  page += "function deleteSSID(f){\n";
  page += "if((f=f.parentNode))\n";
  page += "for(var i=0; i<f.children.length; i++)\n";
  page += "if(f.children[i].type=='text')\n";
  page += "if(f.children[i].value!=''){\n";
  page += "if(confirm('Are you sure to remove this SSID?')){\n";
  page += "for(var i=0; i<f.children.length; i++)\n";
  page += "if(f.children[i].type=='password') f.children[i].value='';\n";
  page += "f.submit();\n";
  page += "}}else alert('Empty SSID...');\n";
  page += "}\n";
  page += "function switchSubmit(e){refresh();\n";
  page += " var b=false, l=e.parentNode.parentNode.getElementsByTagName('input');\n";
  page += " for(var i=0; i<l.length; i++) if(l[i].type=='number')\n   b|=(l[i].value!='0');\n";
  page += " e.checked&=b; document.getElementById('switchs').submit();\n";
  page += "}\n";
  page += "var checkDelaySubmit=0;\n";
  page += "function checkDelay(e){refresh();\n";
  page += " if(e.value=='-1'){\n";
  page += "  var l=e.parentNode.getElementsByTagName('input');\n";
  page += "  for(var i=0; i<l.length; i++)if(l[i].className=='duration' && l[i].data-unit!=1)\n  {l[i].style.display='none'; l[i].value='0';}\n";
  page += " }clearTimeout(this.checkDelaySubmit); this.checkDelaySubmit=setTimeout(function(){this.checkDelaySubmit=0; document.getElementById('switchs').submit();}, 1000);\n";
  page += "}\n";
  page += "</script>\n<div id='about' class='modal'><div class='modal-content'>";
  page += "<span class='close' onClick='refresh();'>&times;</span>";
  page += "<h1>About</h1>";
  page += "This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domotics or Jeedom.<br><br>";
  page += "In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). ";
  page += "Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs (Json format), like this:";
  page += "<a id='example1' style='padding:0 0 0 5px;'></a> (-1 -> unchanged)<br><br>";
  page += "The state of the electrical outlets can also be requested from the following URL: ";
  page += "<a id='example2' style='padding:0 0 0 5px;'></a><br><br>";
  page += "The status of the power strip is retained when the power is turned off and restored when it is turned on ; a power-on delay can be set on each output: (-1) no delay, (0) to disable an output and (number of mn) to configure a power-on delay.<br><br>";
  page += "The following allows you to configure some parameters of the Wifi Power Strip (until a SSID is set, the socket works as an access point with its own SSID and default password: \"" + hostname + "/" + DEFAULTWIFIPASS + "\").<br><br>";
  page += "<h2><form method='POST'>";
  page += "Network name: <input type='text' name='hostname' value='" + hostname + "' style='width:110;'>";
  page += " <input type='button' value='Submit' onclick='submit();'>";
  page += "</form></h2>";
  page += "<h2>Network connection:</h2>";
  page += "<table style='width:100%'><tr>";
  for(int i=0; i<SSIDMax(); i++){
   page += "<td><div><form method='POST'>";
   page += "SSID " + String(i+1) + ":<br><input type='text' name='SSID' value='" + ssid[i] + (ssid[i].length() ?"' readonly": "'") + "><br>";
   page += "Password:<br><input type='password' name='password' value='" + String(ssid[i][0] ?password[i] :"") + "'><br>";
   page += "Confirm password:<br><input type='password' name='confirm' value='" + String(ssid[i][0] ?password[i] :"") + "'><br><br>";
   page += "<input type='button' value='Submit' onclick='saveSSID(this);'>";
   page += "<input type='button' value='Remove' onclick='deleteSSID(this);'>";
   page += "</form></div></td>";
  }page += "</tr></table>";
  page += "<h6><a href='update' onclick='javascript:event.target.port=8081'>Firmware update</a>";
  page += " - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>";
  page += "</div></div>\n";
  page += "<table style='border:0; width:100%;'><tbody><tr>";
  page += "<td><h1>" + hostname + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + getMyMacAddress() + "]" + " :</h1></td>";
  page += "<td style='text-align:right; vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>";
  page += "<tr></tbody></table>\n";
  page += "<h3>Status :</h3>\n";
  page += "<form id='switchs' method='POST'><ul>\n";
/*
  for (short i=0; i<outputCount(); i++){ bool display;
   page += "<li><table><tbody>\n<tr><td>" + outputName[i] + "</td><td>";
   page += "<div class='onoffswitch delayConf'>\n";
//   page += "<input type='checkbox' class='onoffswitch-checkbox' id='" + outputName[i] + "' name='" + outputName[i] + "' " + (outputValue[i] ?"checked" :"") + " onClick='switchSubmit(this);'></input>\n";
   page += "<label class='onoffswitch-label' for='" + outputName[i] + "'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n";
   page += "</div>\n<div class='delayConf'> &nbsp; &nbsp; &nbsp; (will be 'ON' during &nbsp;";
//   display=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]/86400);
//   page += "<input type='nu  mainColor  = blue;
mber' name='" + outputName[i] + "-max-duration-d' value='" + (display ?ultos((unsigned long)maxDurationOn[i]/86400) :(String)"0") + "' min='0' max='366' data-unit=86400 class='duration' style='width:60px;display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"d &nbsp;" :"") + "</input>\n";
//   display|=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]%86400/3600);
//   page += "<input type='number' name='" + outputName[i] + "-max-duration-h' value='" + (display ?ultos((unsigned long)maxDurationOn[i]%86400/3600) :(String)"0") + "' min='0' max='24' data-unit=3600 class='duration' style='display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"h &nbsp;" :"") + "</input>\n";
//   display|=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]%86400%3600/60);
//   page += "<input type='number' name='" + outputName[i] + "-max-duration-mn' value='" + (display ?ultos((unsigned long)maxDurationOn[i]%86400%3600/60) :(String)"0") + "' min='0' max='60' data-unit=60 class='duration' style='display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"mn &nbsp;" :"") + "</input>\n";
//   page += "<input type='number' name='" + outputName[i] + "-max-duration-s'  value='" + ((maxDurationOn[i]!=(unsigned int)(-1)) ?ultos((unsigned long)maxDurationOn[i]%86400%3600%60) :(String)"-1") + "' min='-1' max='60' data-unit=1 class='duration' onChange='checkDelay(this);'>" + (String)((maxDurationOn[i]!=(unsigned int)(-1)) ?"s" :"-") + "</input>\n";
//   page += ")</div>\n</td></tr>\n</tbody></table></li>\n";
 }
 */
 page += "</form>\n</ul></body></html>";
  return page;
}

bool WiFiHost(){
  Serial.println();
  Serial.print("No custom SSID defined: ");
  Serial.println("setting soft-AP configuration ... ");
  WiFiAP=WiFi.softAP(hostname.c_str(), password[0].c_str());
  Serial.println(String("Connecting [") + WiFi.softAPIP().toString() + "] from: " + hostname + "/" + password[0]);
  nbWifiAttempts=(nbWifiAttempts==-1 ?1 :nbWifiAttempts); WifiAPDelay=60;
  return WiFiAP;
}

bool WiFiConnect(){
  WiFi.disconnect(); WiFi.softAPdisconnect(); WiFiAP=false;
  delay(10);

  if(!ssid[0][0] || !nbWifiAttempts--)
    return WiFiHost();

  for(short i=0; i<SSIDMax(); i++) if(ssid[i].length()){
    Serial.println(); Serial.println();

    //Connection au reseau Wifi /Connect to WiFi network
    Serial.println(String("Connecting [") + getMyMacAddress() + "] to: " + ssid[i]);
    WiFi.begin(ssid[i].c_str(), password[i].c_str());
    WiFi.mode(WIFI_STA);

    //Attendre la connexion /Wait for connection
    for(short j=0; j<16 && WiFi.status()!=WL_CONNECTED; j++){
      delay(500);
      Serial.print(".");
    } Serial.println();

    if(WiFi.status()==WL_CONNECTED){
      //Affichage de l'adresse IP /print IP address
      Serial.println("WiFi connected");
      Serial.println("IP address: " + WiFi.localIP().toString());
      Serial.println();
      nbWifiAttempts=MAXWIFIERRORS;
      return true;
    }else Serial.println("WiFi Timeout.");
  }
  return false;
}

bool readConfig(bool=true);
void writeConfig(){        //Save current config:
  if(!readConfig(false))
    return;
  File f=SPIFFS.open("/config.txt", "w+");
  if(f){ uint8_t i;
    f.println(ResetConfig);
    f.println(hostname);                //Save hostname
    for(uint8_t j=i=0; j<SSIDMax(); j++){   //Save SSIDs
      if(!password[j].length()) ssid[j]="";
      if(ssid[j].length()){
        f.println(ssid[j]);
        f.println(password[j]);
        i++;
    } }while(i++<SSIDMax()) f.println();
//  f.println(myConfig);
    }f.close();
  if(WiFiAP && ssid[0].length()) WiFiConnect();
}

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }
inline bool getConfig(String        &v, File f, bool w){String r=readString(f);               if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(bool          &v, File f, bool w){bool   r=atoi(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(int           &v, File f, bool w){int    r=atoi(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(long          &v, File f, bool w){long   r=atol(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
bool readConfig(bool w){      //Get config (return false if config is not modified):
  bool ret=false;
  File f=SPIFFS.open("/config.txt", "r");
  if(f && ResetConfig!=atoi(readString(f).c_str())) f.close();
  if(!f){    //Write default config:
    if(w){
      ssid[0]=""; password[0]=DEFAULTWIFIPASS;
      }SPIFFS.format(); writeConfig();      SPIFFS.format(); writeConfig();
    return true;
  }ret|=getConfig(hostname, f, w);
  for(short i=0; i<SSIDMax(); i++){        //Get SSIDs
    ret|=getConfig(ssid[i], f, w);
    if(ssid[i].length())
      ret|=getConfig(password[i], f, w);
    }
  if(!ssid[0].length()) password[0]=DEFAULTWIFIPASS; //Default values
//ret|=getConfig(myConfig, f, w);
  f.close(); return ret;
}

void handleSubmitSSIDConf(){           //Setting:
  int count=0;
  for(short i=0; i<SSIDMax(); i++) if(ssid[i].length()) count++;
  for(short i=0; i<count; i++)
    if(ssid[i]==server.arg("SSID")){
      password[i]=server.arg("password");
      if(!password[i].length())    //Modify password
        ssid[i]=="";               //Delete ssid
      writeConfig();
      if(!ssid[i].length()) readConfig();
      return;
  }if(count<SSIDMax()){            //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
} }

void  handleRoot(){bool ret=true;
    if((ret=server.hasArg("hostname"))){
      hostname=server.arg("hostname");                  //Set host name:
    }else if((ret=server.hasArg("password"))){          //Set WiFi connections:
      handleSubmitSSIDConf();
    }server.send(200, "text/html", getPage());
}

void outputInit(){
  output[0].value[blue]  = MAXVALUE-1;   output[0].value[green] = MAXVALUE/2-1; output[0].value[red]   = 0;
  output[1].value[blue]  = 0;            output[1].value[green] = MAXVALUE-1;   output[1].value[red]   = MAXVALUE/2-1;
  output[2].value[blue]  = MAXVALUE/2-1; output[2].value[green] = 0;            output[2].value[red]   = MAXVALUE-1;
  for(unsigned char n=0; n<outputCount(); n++)for(unsigned char color=0; color<3; color++)output[n].incr[color]=true;
}

inline void incrOutput(unsigned char n, unsigned char color){
  if(output[n].incr[color])
        analogWrite(output[n].pin[color], ++output[n].value[color]);
  else  analogWrite(output[n].pin[color], --output[n].value[color]);
}

inline void clearOutput(unsigned char n, unsigned char color){
  analogWrite(output[n].pin[color], (output[n].value[color]=0));
} inline bool isClear(unsigned char n, unsigned char color){return (output[n].value[color]==0);}

inline void setOutput(unsigned char n, unsigned char color){
  analogWrite(output[n].pin[color], (output[n].value[color]=MAXVALUE-1));
} inline bool isSet(unsigned char n, unsigned char color){return (output[n].value[color]==MAXVALUE-1);}

inline void clearAll() {
  for (unsigned char n=0; n<outputCount(); n++)
    for (unsigned char color=0; color<3; color++)
      clearOutput(n, color);
}
inline bool isAllClear(){
  for(unsigned char n=0; n<outputCount(); n++)
    for(unsigned char color=0; color<3; color++)
      if(!isClear(n, color)) return false;
  return true;
}

inline void reverse(unsigned char n, unsigned char color){output[n].incr[color] = isClear(n, color);}

void sequence(void){
  if (isAllClear()) outputInit();
  for(unsigned char n=0; n<outputCount(); n++)
    for(unsigned char color=0; color<3; color++){
      if(isSet(n, color) || isClear(n, color)) reverse(n, color);
      incrOutput(n, color);
}    }

void roundFlashing(unsigned char color=roundFlashingMulti)  {
  static unsigned char n=0, newcol=0; n%=outputCount(); clearAll();
  if(color!=roundFlashingMulti) newcol=color;
  analogWrite(output[n++].pin[(newcol++)%3], MAXVALUE-1); delay(750); clearAll();
}

void allFlashing(unsigned char color)  {
  static unsigned char n=0; n=n%outputCount(); clearAll();
  for(unsigned char n=0; n<3; n++)
    analogWrite(output[n].pin[color%3], MAXVALUE/2-1);
  delay(750); clearAll();
}

void handleRoundFlashingBlue() {prog=roundFlashingBlue; server.send(200, "text/html", getPage());}
void handleRoundFlashingGreen(){prog=roundFlashingGreen;server.send(200, "text/html", getPage());}
void handleRoundFlashingRed()  {prog=roundFlashingRed;  server.send(200, "text/html", getPage());}
void handleRoundFlashing()     {prog=roundFlashingMulti;server.send(200, "text/html", getPage());}
void handleAllFlashingBlue()   {prog=allFlashingBlue;   server.send(200, "text/html", getPage());}
void handleAllFlashingGreen()  {prog=allFlashingGreen;  server.send(200, "text/html", getPage());}
void handleAllFlashingRed()    {prog=allFlashingRed;    server.send(200, "text/html", getPage());}
void handleSequence()          {prog=none; clearAll();  server.send(200, "text/html", getPage());}

void setup(){
  Serial.begin(115200);
  delay(10);Serial.println("Hello World!");

  //Definition des URL d'entree /Input URL definition
  server.on("/", handleRoot);
  server.on("/clear", handleSequence);
  server.on("/roundFlashingRed",  handleRoundFlashingRed);
  server.on("/roundFlashingGreen",handleRoundFlashingGreen);
  server.on("/roundFlashingBlue", handleRoundFlashingBlue);
  server.on("/roundFlashing",     handleRoundFlashing);
  server.on("/allFlashingRed",    handleAllFlashingRed);
  server.on("/allFlashingGreen",  handleAllFlashingGreen);
  server.on("/allFlashingBlue",   handleAllFlashingBlue);
  //server.on("/about", [](){ server.send(200, "text/plain", getHelp()); });

  //Demarrage du serveur web /Web server start
  server.begin();
  Serial.println("Server started");

  //Open config:
  SPIFFS.begin();

  //initialisation des broches /pins init
  // Define Pins:
  output[0].pin[blue]  = D3;
  output[0].pin[green] = D1;
  output[0].pin[red]   = D4;

  output[1].pin[blue]  = D5;
  output[1].pin[green] = D2;
  output[1].pin[red]   = D0;

  output[2].pin[blue]  = D6;
  output[2].pin[green] = D7;
  output[2].pin[red]   = D8;

  for(unsigned char i=0; i<3; i++)
    for(unsigned char j=0; j<3; j++){
      pinMode     (output[i].pin[j], OUTPUT);
      digitalWrite(output[i].pin[j],  LOW);
  } clearAll();

  handleSequence();

  //Allow OnTheAir updates:
  MDNS.begin(hostname.c_str());
  httpUpdater.setup(&updater);
  updater.begin();
}

#define DELAY 200
unsigned short int count=0;
void loop(){
  updater.handleClient();

  if(!count--){ count=1000/DELAY*900;            //Test connexion/Check WiFi every 15mn
    if(WiFi.status() != WL_CONNECTED && (!WiFiAP || !WifiAPDelay--))
      WiFiConnect();
  }

  server.handleClient();                         //Traitement des requetes /HTTP treatment

  switch(prog){
    case none: sequence();
      break;
    case roundFlashingBlue:
    case roundFlashingGreen:
    case roundFlashingRed:
    case roundFlashingMulti:roundFlashing(prog);
      break;
    case allFlashingBlue:
    case allFlashingGreen:
    case allFlashingRed: allFlashing(prog);
  }

  delay(DELAY);
}
