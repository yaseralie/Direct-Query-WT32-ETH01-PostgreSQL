/*
  by Yaser Ali Husen
  This code for sending data temperature and humidity to postresql database
  code based on pgconsole in SimplePgSQL example code.

  Board : W32-ETH01
  Uploading steps: https://www.youtube.com/watch?v=0avosBsQpis
  Wiring DHT use GPIO 2

  Library for ethernet connection: https://github.com/khoih-prog/WebServer_WT32_ETH01
  Use Arduino Library Manager Installation:
      The best and easiest way is to use Arduino Library Manager. Search for WebServer_WT32_ETH01, 
      then select / install the latest version
  Note:
      Sometimes, serial is not working but data sent succesfully
*/
#include <SPI.h>
#include <SimplePgSQL.h>
#define DEBUG_ETHERNET_WEBSERVER_PORT       Serial

// Debug Level from 0 to 4
#define _ETHERNET_WEBSERVER_LOGLEVEL_       3
#include <WebServer_WT32_ETH01.h>
WebServer server(80);

// Select the IP address according to your local network
IPAddress myIP(192, 168, 0, 232);
IPAddress myGW(192, 168, 0, 1);
IPAddress mySN(255, 255, 255, 0);

// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

//Setup for DHT======================================
#include <DHT.h>
#define DHTPIN 2  //GPIO2 atau D4
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT11     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
//Setup for DHT======================================

//Setup connection and Database=====================
IPAddress PGIP(192, 168, 0, 101);       // your PostgreSQL server IP
const char user[] = "postgres";         // your database user
const char password[] = "123";          // your database password
const char dbname[] = "postgres";       // your database name

WiFiClient    client;

char buffer[1024];
PGconnection conn(&client, 0, 1024, buffer);
char inbuf[128];
int pg_status = 0;
//Setup connection and Database=====================

//millis================================
//Set every 10 sec read DHT
unsigned long previousMillis = 0;  // variable to store the last time the task was run
const long interval = 10000;        // time interval in milliseconds (eg 1000ms = 1 second)
//======================================

void setup() {
  delay(5000);
  Serial.begin(115200);
  while (!Serial);

  delay(5000);
  //For DHT sensor
  dht.begin();

  // To be called before ETH.begin()
  WT32_ETH01_onEvent();
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

  // Static IP, leave without this line to get IP via DHCP
  //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
  ETH.config(myIP, myGW, mySN, myDNS);

  WT32_ETH01_waitForConnect();
  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
}

void doPg(void)
{
  char *msg;
  int rc;
  if (!pg_status) {
    conn.setDbLogin(PGIP,
                    user,
                    password,
                    dbname,
                    "utf8");
    pg_status = 1;
    return;
  }

  if (pg_status == 1) {
    rc = conn.status();
    if (rc == CONNECTION_BAD || rc == CONNECTION_NEEDED) {
      char *c = conn.getMessage();
      if (c) Serial.println(c);
      pg_status = -1;
    }
    else if (rc == CONNECTION_OK) {
      pg_status = 2;
      Serial.println("Starting query");
    }
    return;
  }

  if (pg_status == 2 && strlen(inbuf) > 0) {
    if (conn.execute(inbuf)) goto error;
    Serial.println("Working...");
    pg_status = 3;
    memset(inbuf, 0, sizeof(inbuf));
  }

  if (pg_status == 3) {
    rc = conn.getData();
    int i;
    if (rc < 0) goto error;
    if (!rc) return;
    if (rc & PG_RSTAT_HAVE_COLUMNS) {
      for (i = 0; i < conn.nfields(); i++) {
        if (i) Serial.print(" | ");
        Serial.print(conn.getColumn(i));
      }
      Serial.println("\n==========");
    }
    else if (rc & PG_RSTAT_HAVE_ROW) {
      for (i = 0; i < conn.nfields(); i++) {
        if (i) Serial.print(" | ");
        msg = conn.getValue(i);
        if (!msg) msg = (char *)"NULL";
        Serial.print(msg);
      }
      Serial.println();
    }
    else if (rc & PG_RSTAT_HAVE_SUMMARY) {
      Serial.print("Rows affected: ");
      Serial.println(conn.ntuples());
    }
    else if (rc & PG_RSTAT_HAVE_MESSAGE) {
      msg = conn.getMessage();
      if (msg) Serial.println(msg);
    }
    if (rc & PG_RSTAT_READY) {
      pg_status = 2;
      Serial.println("Waiting query");
    }
  }
  return;
error:
  msg = conn.getMessage();
  if (msg) Serial.println(msg);
  else Serial.println("UNKNOWN ERROR");
  if (conn.status() == CONNECTION_BAD) {
    Serial.println("Connection is bad");
    pg_status = -1;
  }
}

void loop() {
  delay(50);
  doPg();
  unsigned long currentMillis = millis();  // get the time now
  // Checks whether it is time to run the task
  if (currentMillis - previousMillis >= interval) {
    // Save the last time the task was run
    previousMillis = currentMillis;
    //read DHT-11---------------------------------------
    t = dht.readTemperature();
    h = dht.readHumidity();
    Serial.print("Humidity = ");
    Serial.print(h);
    Serial.print("% ");
    Serial.print("Temperature = ");
    Serial.print(t);
    Serial.println(" C ");
    //read DHT-11---------------------------------------
    //Send data to PostgreSQL
    // Using sprintf to format strings with numeric values
    // and save it in inbuf
    sprintf(inbuf, "insert into dht_data (temp,humidity) values(%.2f,%.2f)", t, h);
  }
}
