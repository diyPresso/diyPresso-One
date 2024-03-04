 /*
 * EasyWiFi
 * Created by John V. - 2020 V 1.4.1
 * 
 *  RGB LED INDICATOR on uBlox nina Module
 *  GREEN: Connected
 *  
 *  BLUE: (Stored) Credentials found, connecting         <<<<--_
 *  YELLOW: No Stored Credentials found, connecting             \
 *  PURPLE: Can't connect, opening AP for credentials input      |
 *  CYAN: Client connected to AP, wait for credentials input >>--/
 * 
 *  RED: Not connected / Can't connect, wifi.start is stopped, return to program
 * 
 * Released into the public domain on github: https://github.com/javos65/EasyWifi-for-MKR1010
 */
 
#include "EasyWiFi.h"
extern void menu_wifi(char *msg);

#define DBGON    // Debug option  -serial print
//#define DBGON_X   // Debug option - incl packets

char ACCESPOINTNAME[SSIDBUFFERSIZE] = APNAME;       // AP name, dynamic adaptable
char G_SSIDList[MAXSSID][SSIDBUFFERSIZE];                     // Store of available SSID's
int G_APStatus = WL_IDLE_STATUS, G_APInputflag;               // global AP flag to use
int G_ssidCounter = 0;                //Gloabl counter for number of found SSID's
char G_ssid[32] = SECRET_SSID;        // optional init: your network SSID (name) 
char G_pass[32] = SECRET_PASS;        // optional init: your network password 
WiFiServer G_APWebserver(80);         // Global Acces Point Web Server
WiFiUDP G_UDPAP_DNS;                  // A UDP instance to let us send and receive packets over UDP
IPAddress G_APip;                     // Global Acces Point IP adress 
IPAddress G_APDNSclientip;
int G_DNSClientport;
int G_DNSRqstcounter=0;
int SEED=4;
boolean G_useAP=1; // use AP after loging failure, or quit with no AP service
boolean G_ledon=1; // leds on or of
byte G_UDPPacketbuffer[UDP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets
byte G_DNSReplyheader[DNSHEADER_SIZE] = { 
  0x00,0x00,   // ID, to be filled in #offset 0
  0x81,0x80,   // answer header Codes
  0x00,0x01,   //QDCOUNT = 1 question
  0x00,0x01,   //ANCOUNT = 1 answer
  0x00,0x00,   //NSCOUNT / ignore
  0x00,0x00    //ARCOUNT / ignore
  };
byte G_DNSReplyanswer[DNSANSWER_SIZE] = {   
  0xc0,0x0c,  // pointer to pos 12 : NAME Labels
  0x00,0x01,  // TYPE
  0x00,0x01,  // CLASS
  0x00,0x00,  // TTL
  0x18,0x4c,  // TLL 2 days
  0x00,0x04,   // RDLENGTH = 4
  0x00,0x00,   // IP adress octets to be filled #offset 12
  0x00,0x00    // IP adress octeds to be filled
  } ;

// ***************************************


EasyWiFi::EasyWiFi()
{
}

// Login to local network  //
void EasyWiFi::start(){
int  noconnect=0,totalconnect=0;;
WiFi.disconnect();delay(2000);
NINAled(BLUE); // Starting to connect: Set Blue  
int G_Wifistatus = WiFi.status();
if ( ( G_Wifistatus != WL_CONNECTED) || (WiFi.RSSI() <= -90) ||(WiFi.RSSI() ==0) ) { // check if connected
// Read SSId File
  if (Read_Credentials(G_ssid,G_pass)==0) {  // read credentials, if not possible, re-use the old-already loaded credentials
  NINAled(ORANGE); // no credentials found SET YELLOW
#ifdef DBGON
      Serial.println("* Using old credentials");
#endif
  } 
  while ( (G_Wifistatus != WL_CONNECTED) || (WiFi.RSSI() <= -90) || (WiFi.RSSI() ==0) ) {   // attempt to connect to WiFi network:
      noconnect=0;
      menu_wifi("connecting");
      while ( ((G_Wifistatus != WL_CONNECTED) || (WiFi.RSSI() <= -90) || (WiFi.RSSI() ==0) ) && noconnect<MAXCONNECT) {   // attempt to connect to WiFi network 3 times
        menu_wifi(G_ssid);
#ifdef DBGON
          Serial.print("* Attempt#");Serial.print(noconnect);Serial.print(" to connect to Network: ");Serial.println(G_ssid);                // print the network name (SSID);
#endif
           G_Wifistatus = WiFi.begin(G_ssid, G_pass);     // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
           delay(2000);                                   // wait 2 seconds for connection:
           noconnect++;                                   // try-counter
           }
      totalconnect=totalconnect+noconnect;                // count total failed connects     
      if ( G_Wifistatus == WL_CONNECTED ) {    
       NINAled(GREEN); // Set Green   
#ifdef DBGON
      printWiFiStatus();                        // you're connected now, so print out the status anmd break while loop
#endif
      break;
      }
      else if ( (totalconnect > ESCAPECONNECT) || (G_useAP==false) ){ // quite login service ?
      NINAled(RED); // Set red 
#ifdef DBGON
      Serial.println("* Connection not possible after too many retries, quit wifi.start process");                   
#endif
      break;
            }
      else{                                          // no connection possible : exit without server started
#ifdef DBGON
      Serial.println("* Connection not possible after several retries, opening Access Point");                   
#endif
       // start direct-Wifi connect to manualy input Wifi credentials
        NINAled(PURPLE); // no network, : RED
        listNetworks();                         // load avaialble networks in a list
        APSetup();
        NINAled(PURPLE); // start AP, : Purple
        G_APInputflag=0;
        while(!G_APInputflag) {                 // Keep AP open till input is received or till 30 seconds are over
                              // Check AP status - new client on or of ?
              menu_wifi("CONFIG-AP mode");
              if (G_APStatus != WiFi.status()) {
                  G_APStatus = WiFi.status();                                 // it has changed update the variable
                  if (G_APStatus == WL_AP_CONNECTED) {                         // a device has connected to the AP
#ifdef DBGON                     
                    Serial.println("Device connected to AP\n"); 
#endif                 
                    NINAled(CYAN); // Client on AP : purple
                    G_DNSRqstcounter=0;                                           // reset DNS counter
                  }
                  else {                                                     // a device has disconnected from the AP, and we are back in listening mode
#ifdef DBGON                  
                    Serial.println("Device disconnected from AP\n");
#endif                    
                  }
              } // end if loop changed G_APStatus                              
              if (G_APStatus == WL_AP_CONNECTED)  // IF client connected to AP, start DNS and check Webserver
              {
                 APDNSScan();          // check DNS requests
                 APWiFiClientCheck();  // check HTTP server Client
              }
            }
        G_UDPAP_DNS.stop();        // Close UDP connection
        WiFi.end(); 
        WiFi.disconnect();
        NINAled(BLUE); // new credentials : BLUE
        delay(2000);
      }
  } // ever while loop till connected
} // end if not connected
else{
    NINAled(GREEN); // Set Green  
#ifdef DBGON
    Serial.println("* Already connected.");                     // you're already connected
    printWiFiStatus(); 
#endif
    }
}

// SERIALPRINT Wifi Status - only for debug
void EasyWiFi::printWiFiStatus() {
#ifdef DBGON
    // print the SSID of the network you're attached to:
    Serial.print("* SSID: "); Serial.print(WiFi.SSID());
    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP(); Serial.print(" - IP Address: "); Serial.print(ip);
    // print your WiFi gateway:
    IPAddress ip2 = WiFi.gatewayIP(); Serial.print(" - IP Gateway: ");Serial.print(ip2);    
    // print the received signal strength:
    long rssi = WiFi.RSSI(); Serial.print("- Rssi: "); Serial.print(rssi); Serial.println(" dBm");
    static char buf[64];
    sprintf(buf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    menu_wifi(buf);
#endif
}

// SErase credentials from disk file
byte EasyWiFi::erase() {
  return Erase_Credentials();
}

// Set Seed of the Cypher, should be positive
void EasyWiFi::seed(int value) {
  if (value >= 0) SEED=value;
}

// Set Name of AccessPoint
byte EasyWiFi::apname(char *name) {
  int t=0;
  while(name[t]!=0)
  {
    ACCESPOINTNAME[t]=name[t];
    t++;
    if (t>=SSIDBUFFERSIZE) break;
   }
ACCESPOINTNAME[t]=0; // close string
return t;
}

// Scan for available Wifi Networks and place is Glovbal SSIDList
void EasyWiFi::listNetworks() {
int t;
String tmp;
    // scan for nearby networks:
    int numSsid = WiFi.scanNetworks();
    if (numSsid == -1)
    {
#ifdef DBGON        
      Serial.println("* Couldn't get a Wifi List");
#endif
    }
    else {
#ifdef DBGON    
    Serial.print("* Found total ");Serial.print(numSsid);Serial.println(" Networks.");
#endif      
    G_ssidCounter = 0;
    // print the network number and name for each network found:
    for (int thisNet = 0; thisNet < numSsid; thisNet++) {
        tmp =  WiFi.SSID(thisNet);
        if(G_ssidCounter < MAXSSID ) { // store only maximum of <SSIDMAX> SSDI's with high dB > -80        && WiFi.RSSI(thisNet)>-81
          for(t=0;t<tmp.length();++t)  G_SSIDList[G_ssidCounter][t] = tmp[t];
          G_SSIDList[G_ssidCounter][t]=0;
#ifdef DBGONP           
          Serial.print(G_ssidCounter);
          Serial.print(". ");
          Serial.print(G_SSIDList[G_ssidCounter]);
          Serial.print("\t\tSignal: ");
          Serial.print(WiFi.RSSI(thisNet));
          Serial.println(" dBm");
          Serial.flush();
#endif        
          G_ssidCounter = G_ssidCounter+1; 
          }
        } // end for list loop
  }
}


/* Wifi Acces Point Initialisation */
void EasyWiFi::APSetup(){
int tr=5;  // 5 tries to setup AP
#ifdef DBGON
  Serial.print("* Creating access point named: "); Serial.println(ACCESPOINTNAME);
#endif
  G_APip = IPAddress((char) random(11,172),(char) random(0,255),(char) random(0,255),0x01); // Generate random IP adress in Privit IP range
  WiFi.end();                                                       // close Wifi - juist to be suire
  delay(3000);                                                      // Wait 3 seconds
  WiFi.config(G_APip,G_APip,G_APip,IPAddress(255,255,255,0));       // Setup config
while(tr>0) {
    G_APStatus = WiFi.beginAP(ACCESPOINTNAME,APCHANNEL);             // setup AccessPoint
    if (G_APStatus != WL_AP_LISTENING)  {                           // retry
#ifdef DBGON
        Serial.print(".");
#endif        
        --tr;
        //WiFi.disconnect();
        WiFi.config(G_APip,G_APip,G_APip,IPAddress(255,255,255,0));
        }
    else break; // break while loop when AP is connected
    }
if (tr==0){  // not possible to connect in 5 retries
#ifdef DBGON  
   Serial.println("* Creating access point failed");
#endif     
   }
else
  { 
  delay(2000);
  printWiFiStatus();           // you're connected now, so print out the status
  G_UDPAP_DNS.begin(UDPPORT);        // start the UDP server
  G_APWebserver.begin();       // start the AP web server on port 80
  }
}


/* DNS Routines via UDP, act on DSN requests on Port 53*/
/* assume wifi UDP connection has been set up */
void EasyWiFi::APDNSScan()
{
int t=0;  // generic loop counter
int r,p;  // reply and packet counters
unsigned int packetSize=0;
unsigned int replySize=0;
byte G_DNSReplybuffer[UDP_PACKET_SIZE];       // buffer to hold the send DNS reply

  packetSize = G_UDPAP_DNS.parsePacket();
  if ( packetSize ) {                           // We've received a packet, read the data from it
    G_UDPAP_DNS.read(G_UDPPacketbuffer, packetSize);  // read the packet into the buffer
    G_APDNSclientip = G_UDPAP_DNS.remoteIP();
    G_DNSClientport = G_UDPAP_DNS.remotePort();
//  if ( (G_APDNSclientip != G_APip) && (G_DNSRqstcounter<=DNSMAXREQUESTS) )       // skip own requests - ie ntp-pool time requestfrom Wifi module
    if ( (G_APDNSclientip != G_APip)  )       // skip own requests - ie ntp-pool time requestfrom Wifi module

    {
#ifdef DBGON_X  
    Serial.print("DNS-packets (");Serial.print(packetSize);
    Serial.print(") from ");Serial.print(G_APDNSclientip);
    Serial.print(" port ");Serial.println(G_DNSClientport);
      for (t=0;t<packetSize;++t){
      Serial.print(G_UDPPacketbuffer[t],HEX);Serial.print(":");
      }
      Serial.println(" ");
      for (t=0;t<packetSize;++t){
      Serial.print( (char) G_UDPPacketbuffer[t]);//Serial.print("");
      }
    Serial.println("");
#endif   
    //Copy Packet ID and IP into DNS header and DNS answer
    G_DNSReplyheader[0] = G_UDPPacketbuffer[0];G_DNSReplyheader[1] = G_UDPPacketbuffer[1]; // Copy ID of Packet offset 0 in Header
    G_DNSReplyanswer[12] = G_APip[0];G_DNSReplyanswer[13] = G_APip[1];G_DNSReplyanswer[14] = G_APip[2];G_DNSReplyanswer[15] = G_APip[3]; // copy AP Ip adress offset 12 in Answer
    r=0; // set reply buffer counter
    p=12; // set packetbuffer counter @ QUESTION QNAME section
    // copy Header into reply
    for (t=0;t<DNSHEADER_SIZE;++t) G_DNSReplybuffer[r++]=G_DNSReplyheader[t];
    // copy Qusetion into reply:  Name labels till octet=0x00
    while (G_UDPPacketbuffer[p]!=0) G_DNSReplybuffer[r++]=G_UDPPacketbuffer[p++];
    // copy end of question plus Qtype and Qclass 5 octets
    for(t=0;t<5;++t)  G_DNSReplybuffer[r++]=G_UDPPacketbuffer[p++];
    //copy Answer into reply
    for (t=0;t<DNSANSWER_SIZE;++t) G_DNSReplybuffer[r++]=G_DNSReplyanswer[t];
    replySize=r;    
#ifdef DBGON_X  
        Serial.print("* DNS-Reply (");Serial.print(replySize);
        Serial.print(") from ");Serial.print(G_APip);
        Serial.print(" port ");Serial.println(UDPPORT);
        for (t=0;t<replySize;++t){
        Serial.print(G_DNSReplybuffer[t],HEX);Serial.print(":");
        }
        Serial.println(" ");
        for (t=0;t<replySize;++t){
        Serial.print( (char) G_DNSReplybuffer[t]);//Serial.print("");
        }
    Serial.println("");  
#endif      
     // Send DSN UDP packet
     G_UDPAP_DNS.beginPacket(G_APDNSclientip, G_DNSClientport); //reply DNSquestion
     G_UDPAP_DNS.write(G_DNSReplybuffer, replySize);
     G_UDPAP_DNS.endPacket();
     G_DNSRqstcounter++;
   } // end loop correct IP
 } // end loop received packet
}


// Check the AP wifi Client Responses and read the inputs on the main AP web-page.
void EasyWiFi::APWiFiClientCheck() {
String Postline = "";                   // make a String to hold incoming POST-Data
String currentLine = "";                // make a String to hold incoming data from the client
char c;                                 // Character read buffer
int t,u,v,pos1,pos2;                    // loop counter
WiFiClient client = G_APWebserver.available();  // listen for incoming clients
  if (client) {                          // if you get a client,
#ifdef DBGON     
    Serial.println("* New AP webclient");        // print a message out the serial port
#endif    
    while (client.connected()) {         // loop while the client's connected
      if (client.available()) {          // if there's bytes to read from the client,
        c = client.read();               // read a byte, then
#ifdef DBGONP  
        Serial.write(c);                 // print it out the serial monitor
#endif          
        if (c == '\n') {                 // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            // the content of the HTTP response follows the header:
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"); // metaview
            client.println("<body style=\"background-color:SteelBlue\">"); // set color CCS HTML5 style . I | I . I | I .
            client.print( "<p style=\"font-family:verdana; color:GhostWhite\">&nbsp<font size=3> l </font><font size=4> l </font><font size=5> | </font><font size=4> l </font><font size=3> l </font><font size=4> l </font><font size=5> | </font><font size=4> l </font><font size=3> l </font> <br>");
            client.print( "<font size=5>Arduino</font>  <br><font size=5>");client.print(WiFi.SSID());client.println("</font>");
            client.print("<p style=\"font-family:verdana; color:Gainsboro\">");
            for(t=0;t<G_ssidCounter;t++){
                client.print(t);client.print(". [");client.print(G_SSIDList[t]);client.print("]<br>");
                }
                client.println("</font></p>");
            client.print("<p style=\"font-family:verdana; color:Gainsboro\">Enter Wifi-Ssid (Number or name) and Password:<br>");
            client.println("<form method=POST action=\"checkpass.php\">");
            client.println("<input type=text name=XXID><br>");                      // XXID is a key word fo parsing the response
            client.println("<form method=POST action=\"checkpass.php\">");
            client.println("<input type=password name=XXPS><br>");                   //XXPS is a key word for parsing
            client.println("<input type=submit name=action value=Submit>");
            client.println("</form></p>");
            client.print("<meta http-equiv=\"refresh\" content=\"30;url=http://");client.print(WiFi.localIP());client.println("\">");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        // Check to see if the client request was a post on our checkpass.php
        if (currentLine.endsWith("POST /checkpass.php")) {
#ifdef DBGON            
                Serial.println("* Found APServer POST");  
#endif                
                currentLine = ""; 
                while (client.connected()) {                 // loop while the client's connected
                if (client.available()) {                    // if there's bytes to read from the client,
                   c = client.read();                        // read a byte, then
#ifdef DBGON_X                      
                   Serial.write(c);                          // print it out the serial monitor
#endif                   
                    if (c == '\n') {                          // if the byte is a newline character
                       //if (currentLine.length() == 0) break; // no lenght :  end of data request
                       currentLine = "";                      // if you got a newline, then clear currentLine:
                       }
                    else if (c != '\r') currentLine += c;     // if you got anything else but a carriage return character, add to string
               if (currentLine.endsWith("XXID=") ) pos1=currentLine.length();
               if (currentLine.endsWith("&XXPS=") ) pos2=currentLine.length();     
               if (currentLine.endsWith("&action")) {         // Check read line on "data=" start that ends with "&action"
                    t = currentLine.length();
                    if(t<78) {
                      if (pos2-pos1==7) {
                        u= (currentLine[pos1]-48);  if (u>G_ssidCounter) u=0;                                 // one digit - convert to max index
                        for(v=0;G_SSIDList[u][v]!=0;v++) G_ssid[v]=G_SSIDList[u][v]; G_ssid[v]=0;          // copy list name to ssid           
                      }
                      else 
                       {u=0;for(v=pos1;v<(pos2-6);v++) G_ssid[u++]=currentLine[v];G_ssid[u]=0;}           // if not one digit, copy input name to ssid
                      u=0;for(v=pos2;v<(t-7);v++) G_pass[u++]=currentLine[v];G_pass[u]=0;
                      Write_Credentials(G_ssid,sizeof(G_ssid),G_pass,sizeof(G_pass) ); // write credentials to flash 
                    }
                    else
                    {
#ifdef DBGON                        
                    Serial.print("* Invalid input from AP Client"); Serial.println(currentLine);               
#endif                            
                    }
#ifdef DBGON                        
                    Serial.print("\n* AP client input found: "); 
                    Serial.print( G_ssid );Serial.print(",");Serial.println("******");               
#endif                     
                    // copy inbpouts to G_ssid and G_pass
                    G_APInputflag=1;                             // flag Ap input
                    break;
                    }
                 } 
            }
            //delay(1000);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"); // metaview
            client.println("<body style=\"background-color:SteelBlue\">"); // set color CCS HTML5 style
            client.print( "<p style=\"font-family:verdana; color:GhostWhite\">&nbsp<font size=3> l </font><font size=4> l </font><font size=5> | </font><font size=4> l </font><font size=3> l </font><font size=4> l </font><font size=5> | </font><font size=4> l </font><font size=3> l </font> <br>");
            client.print( "<font size=5>Arduino</font>  <br><font size=5>");client.print(WiFi.SSID());client.println("</font></p>");
             client.print("<p style=\"font-family:verdana; color:DarkOrange\"><font size=5>Thank You.....</font><br>");
            client.println("<meta http-equiv=\"refresh\" content=\"6;url=/\" />");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop: 
            delay(1000);   // wait 2 seconds to show the message !                     
            break;
        } // end loop POST check


         if (currentLine.endsWith("GET /generate_204")) {              // HTTP generate_204 HTTP/1.1
             client.println("HTTP/1.1 200 OK");
             client.println("Content-type:text/html");
             client.println();  
             client.print("<meta http-equiv=\"refresh\" content=\"0;url=http://");client.print(WiFi.localIP());client.print("\">");
             client.println();  
#ifdef DBGONP                  
             Serial.println("**generate_204 replyied 200 OK");
#endif             
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:              
          break;
        }
      
/*
        if (currentLine.endsWith("GET /RESET")) {
            client.println("HTTP/1.1 200 OK"); 
            client.println("Content-type:text/html");
            client.println();    
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"); // metaview    
            client.println("<body style=\"background-color:black\">"); // set color CCS HTML5 style
            client.print( "<h2 style=\"font-family:verdana; color:GoldenRod\">Access Router ");client.print(WiFi.SSID());client.println("</h2>");
            client.print("<p style=\"font-family:verdana; color:indianred\">Authentication failed<br>");
            client.print("<meta http-equiv=\"refresh\" content=\"4;url=/\" />");
            G_DNSRqstcounter=0;
            Serial.println("**Reset");
          break;
        }
     
        if (currentLine.endsWith("GET /connecttest")) {              // HTTP redirect for browsers with connect-test
            // client.println("HTTP/1.1 302 found");
            // client.print("Location: http://");client.print(WiFi.localIP());client.println("/");
             client.println("HTTP/1.1 200 OK");
             client.println("Content-type:text/html");
             client.println();  
             client.print("<meta http-equiv=\"refresh\" content=\"1;url=/\" />");
             client.println();  
             Serial.println("**generate_204 replyied 200 OK");
          break;
        }
*/
     
      } // end loop client data avaialbe    
    } // end while loop client connected
    
    // close the connection:
    client.stop();
#ifdef DBGON     
    Serial.println("* AP webclient disconnected");
#endif
  } // end If Client
 }


/********* File Routines **************/

/* Read credentials ID,pass to Flash file , Comma separated style*/
byte EasyWiFi::Read_Credentials(char * buf1,char * buf2)
{
  int u,t,c=0;
  char buf[68],comma=1, zero=0;
  char bufc[68];
  WiFiStorageFile file = WiFiStorage.open(CREDENTIALFILE);
  if (file) {
    file.seek(0);
    if (file.available()) {  // read file buffer into memory, max size is 64 bytes for 2 char-strings
      c= file.read(buf, 68);  //Serial.write(buf, c);
    }
    if (c!=0)
    {
      t=0;u=0;
      while(buf[t] != comma) {  // read ID till comma
        bufc[u++]=buf[t++];
        if (u>31) break;
        }
        bufc[u]=0;
        SimpleDecypher(bufc,buf1);
        u=0;t++;                // move to second part: pass
      while(buf[t] != zero) {   // read till zero
        bufc[u++]=buf[t++];
        if (u>31)  break;
        }
        bufc[u]=0;
        SimpleDecypher(bufc,buf2);
    }
#ifdef DBGON
   Serial.print("* Read Credentials : ");Serial.println(c);
#endif    
   file.close(); return(c);
 }
 else {
#ifdef DBGON
   Serial.println("* Cant read Credentials :");
#endif    
  file.close();return(0);
 }
}

/* Write credentials ID,pass to Flash file , Comma separated style*/
byte EasyWiFi::Write_Credentials(char * buf1,int size1,char * buf2,int size2)
{
  int c=0;
  char comma=1, zero=0;
  char buf[32];
  WiFiStorageFile file = WiFiStorage.open(CREDENTIALFILE);
  if (file) {
    file.erase();     // erase content bnefore writing
  }  
    SimpleCypher(buf1,buf);
    c=c+file.write(buf, size1);
    file.write(&comma, 1); c++;
    SimpleCypher(buf2,buf);
   c=c+file.write(buf, size2);
   file.write(&zero, 1); c++;
   if(c!=0) {
#ifdef DBGON
 Serial.print("* Written Credentials : ");Serial.println(c);
#endif
   file.close(); return(c);
 }
 else {
#ifdef DBGON
   Serial.println("* Cant write Credentials");
#endif  
  file.close(); return(0);
 }
}

/* Erase credentials in flkash file */
byte EasyWiFi::Erase_Credentials()
{
char empty[16]="0empty0o0empty0";  
  WiFiStorageFile file = WiFiStorage.open(CREDENTIALFILE);
  if (file) {
  file.seek(0);
  file.write(empty,16); //overwrite flash
  file.erase();
#ifdef DBGON
 Serial.println("* Erased Credentialsfile : ");
#endif  
  file.close(); return(1);
 }
 else {
  #ifdef DBGON
 Serial.println("* Could not erased Credentialsfile : ");
#endif  
  file.close(); return(0);
 }
}

/* Check credentials file */
byte EasyWiFi::Check_Credentials()
{
  WiFiStorageFile file = WiFiStorage.open(CREDENTIALFILE);
  if (file) {
#ifdef DBGON
 Serial.println("* Found Credentialsfile : ");
#endif  
  file.close(); return(1);
 }
 else {
  #ifdef DBGON
 Serial.println("* Could not find Credentialsfile : ");
#endif  
  file.close(); return(0);
 }
}


/* Simple Cyphering the text code */
void EasyWiFi::SimpleCypher(char * textin, char * textout)
{
int c,t=0;
while(textin[t]!=0) {
   textout[t]=textin[t]+SEED%17-t%7;
   t++;
  }
  textout[t]=0;
#ifdef DBGON
// Serial.print("* Cyphered ");Serial.print(t);Serial.print(" - ");Serial.println(textout);
#endif
}

/* Simple DeCyphering the text code */
void EasyWiFi::SimpleDecypher(char * textin, char * textout)
{
int c,t=0;
while(textin[t]!=0) {
   textout[t]=textin[t]-SEED%17+t%7;
   t++;
  }
  textout[t]=0;
#ifdef DBGON
// Serial.print("* Decyphered ");Serial.print(t);Serial.print(" - ");Serial.println(textout);
#endif
}


/* Set Led indicator active on or off - for low power usage*/
void EasyWiFi::led(boolean value)
{
  G_ledon=value;
}

/* Set AP or no AP service*/
void EasyWiFi::useAP(boolean value)
{
  G_useAP=value;
}

/* Set RGB led on uBlox Module R-G-B , max 128*/
void EasyWiFi::NINAled(char r, char g, char b)
{
  if (G_ledon) {
  // Set LED pin modes to output
  WiFiDrv::pinMode(25, OUTPUT);
  WiFiDrv::pinMode(26, OUTPUT);
  WiFiDrv::pinMode(27, OUTPUT);
  
  // Set all LED color 
  WiFiDrv::analogWrite(25, g%128);    // GREEN
  WiFiDrv::analogWrite(26, r%128);    // RED
  WiFiDrv::analogWrite(27, b%128);    // BLUE
  }
}

