#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#define DEBUG_PRINT true

int conn;
char sbuf[512];

char *nick = "printer";
char *channel = "#test";
char *host = "10.13.0.229";
uint16_t port = 6667;

char *ssid = "i3detroit";
char *password = "";
char *host_name = "printer";

WiFiClient client;

void setup() {
    Serial.begin(115200);
    while(!Serial) ;
    Serial.println("hi");

    WiFi.mode(WIFI_STA);

    WiFi.hostname(host_name);
    WiFi.begin(ssid, password);

//    ArduinoOTA.onStart([]() {
//            if(DEBUG_PRINT) Serial.print("Start\n|");
//            for(int i=0; i<80; ++i){
//            if(DEBUG_PRINT) Serial.print(" ");
//            }
//            if(DEBUG_PRINT) Serial.print("|\n ");
//
//            });
//    ArduinoOTA.onEnd([]() {
//            if(DEBUG_PRINT) Serial.println("\nEnd");
//            });
//    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//            //Static is fine becuase we restart after this. Or crash. Either way.
//            static int curProg = 0;
//            int x = map(progress,0,total,0,80);
//            for(int i=curProg; i<x; ++i){
//            if(DEBUG_PRINT) Serial.print("*");
//            }
//            });
//    ArduinoOTA.onError([](ota_error_t error) {
//            if(DEBUG_PRINT) Serial.printf("Error[%u]: ", error);
//            if (error == OTA_AUTH_ERROR) if(DEBUG_PRINT) Serial.println("Auth Failed");
//            else if (error == OTA_BEGIN_ERROR) if(DEBUG_PRINT) Serial.println("Begin Failed");
//            else if (error == OTA_CONNECT_ERROR) if(DEBUG_PRINT) Serial.println("Connect Failed");
//            else if (error == OTA_RECEIVE_ERROR) if(DEBUG_PRINT) Serial.println("Receive Failed");
//            else if (error == OTA_END_ERROR) if(DEBUG_PRINT) Serial.println("End Failed");
//            });
//    ArduinoOTA.begin();
}
void reconnect() {
    while(WiFi.status() != WL_CONNECTED) {
        //Wifi is disconnected
        if(DEBUG_PRINT) Serial.print(".");
        delay(1000);
    }
    //We have wifi
    if(DEBUG_PRINT) Serial.println("WiFi connected");
    if(DEBUG_PRINT) Serial.println("IP address: ");
    if(DEBUG_PRINT) Serial.println(WiFi.localIP());
    //We have wifi
    while(true) {
        if(client.connect(host, port)) {
            Serial.println("connected");
            return;
        } else {
            Serial.println("connection failed");
            delay(2000);
        }
    }

}
void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
      reconnect();
  }
  handle_irc();
}

void raw(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    Serial.print("<< ");
    Serial.println(sbuf);
    client.print(sbuf);
}

void intHandler(int dummy) {
    raw("QUIT :coming back someday\r\n");
    exit(0);
}

void processMessage(char* from, char* where, char* target, char* message){
    if(!strncmp(message, nick, strlen(nick)) && message[strlen(nick)] == ':') {
        char* action = NULL;
        char* args = NULL;
        //Addressed to us (in format 'nick: some message'
        //strip out our name
        message += strlen(nick)+1;
        //There might be a space after :
        if(message[0] == ' ') {
            message++;
        }
        action = args = message;
        while(args[0] != ' ' && args[0] != '\0') {
            args++;
        }
        if(args[0] != '\0') {
            args[0] = '\0';
            args++;
        } else {
            args = NULL;
        }
        //action now is the first word, args is the action arguments or null
        //Now we have the message addressed at us.
        printf("action: [%s]; args: [%s]\n", action, args);

        raw("PRIVMSG %s :%s\r\n", target, args);
    }
}


int read_until(char abort_c, char buffer[], int size) {
  int bytes_read = 0;
  memset(buffer, 0, sizeof(buffer));
  for(int i = 0; i < size - 1; i++) {
    if (client.available()) {
      char c = client.read();
      bytes_read++;
      buffer[i] = c;
      if(c == abort_c) {
        return bytes_read;
      }
    }
  }
  return bytes_read;
}

int handle_irc() {

    char *user, *command, *where, *message, *sep, *target;
    int i, j, l, sl, o = -1, start, wordcount;
    char buf[513];
    //TODO: remove serialBuf
    char serialBuf[512];

    raw("USER %s 0 0 :%s\r\n", nick, nick);
    raw("NICK %s\r\n", nick);

    //while ((sl = read(conn, sbuf, 512))) {
    while(true) {
        ArduinoOTA.handle();

        if(!client.available()) {
            continue;
        }
        sl = read_until('\n', sbuf, 512);
        for (i = 0; i < sl; i++) {
            //We read till one message is done, and process that buffer.
            o++;
            buf[o] = sbuf[i];
            if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == 512) {
                buf[o + 1] = '\0';
                l = o;
                o = -1;
                //We hit message length, or \n\r, so we have a full message. Process it.

                Serial.print(">> ");
                Serial.println(buf);

                if (!strncmp(buf, "PING", 4)) {
                    //Is "PING and some message" we return "PONG and some message"
                    buf[1] = 'O';
                    raw(buf);
                } else if (buf[0] == ':') {
                    // there is a prefix, so we handle it.
                    wordcount = 0;
                    user = command = where = message = NULL;
                    //process message
                    for (j = 1; j < l; j++) {
                        if (buf[j] == ' ') {
                            buf[j] = '\0';
                            wordcount++;
                            switch(wordcount) {
                                case 1: user = buf + 1; break;
                                case 2: command = buf + start; break;
                                case 3: where = buf + start; break;
                            }
                            if (j == l - 1) continue;
                            start = j + 1;
                        } else if (buf[j] == ':' && wordcount == 3) {
                            if (j < l - 1) message = buf + j + 1;
                            break;
                        }
                    }//end process message

                    if (wordcount < 2) continue;

                    if (!strncmp(command, "001", 3) && channel != NULL) {
                        raw("JOIN %s\r\n", channel);
                    } else if (!strncmp(command, "PRIVMSG", 7) ) {
                        if (where == NULL || message == NULL) {
                            Serial.println("where or message null");
                            continue;
                        }
                        if ((sep = strchr(user, '!')) != NULL) {
                            user[sep - user] = '\0';
                        }
                        //If it is one of the 4 channel types, where is the channel, else it's the user
                        if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') {
                            target = where;
                        }else{
                            target = user;
                        }
                        //process \n\r out of message
                        //printf("strlen: %d, -1: %d, -2: %d\n", strlen(message), message[strlen(message)-1], message[strlen(message)-2]);
                        if(strlen(message) > 2) {
                            message[strlen(message)-2] = '\0';
                        }
                        sprintf(serialBuf, "[from: %s] [where: %s] [reply-to: %s] %s\n", user, where, target, message);
                        Serial.print(serialBuf);
                        processMessage(user, where, target, message);
                    }
                }//else the message isn't PING, and has no prefix. Not sure messages like this exist in the wild
            }//end if we have entire thing in buffer
        }//end for
    }//end while we read (forever)
}
