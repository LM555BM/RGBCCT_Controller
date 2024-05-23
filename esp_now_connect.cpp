#include <array>
#include "esp_now.h"
#include "esp_now_connect.hpp"
#include <cstring>
#include <algorithm>
#include <hardwareSerial.h>

ESP_NOW_BASE::ESP_NOW_BASE(uint8_t* loopState, std::vector<LED>* ledVector) : main_state{loopState},lights{ledVector}
{
  
}

ESP_NOW_BASE::~ESP_NOW_BASE()
{
  esp_now_deinit();
}

void ESP_NOW_BASE::printMAC(const uint8_t * mac_addr)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

bool ESP_NOW_BASE::addPeer(const uint8_t *peer_addr) 
{      
  esp_now_peer_info_t pairedDevice;
  memset(&pairedDevice, 0, sizeof(pairedDevice));
  memcpy(pairedDevice.peer_addr, peer_addr, 6);
  
  pairedDevice.channel = 0; // pick a channel
  pairedDevice.encrypt = 0; // no encryption

  // check if the peer exists
  if(esp_now_is_peer_exist(pairedDevice.peer_addr)) {
    Serial.println("Already Paired");
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(&pairedDevice);

    if (addStatus == ESP_OK) {
      //The Broadcast Adress shall not land in the connection vector, as it would make problem while sending Data
      if(memcmp(&pairedDevice.peer_addr, &broadcastAddressX, sizeof(broadcastAddressX)) != 0)
      {
        connections.push_back(pairedDevice);
        printMAC(peer_addr);
        Serial.println(" : Added to connection Vector");
      }
      return true;
    }
    else 
    {
      Serial.println("addPair failed:");
      printMAC(peer_addr);
      return false;
    }
  }
} 






//Master declaration
ESP_NOW_MASTER* ESP_NOW_MASTER::ptr = nullptr;

ESP_NOW_MASTER::ESP_NOW_MASTER(uint8_t* loopState, std::vector<LED>* ledVector) : ESP_NOW_BASE(loopState,ledVector)
{
  stateSelf = MASTER;

  if (esp_now_init() == ESP_OK) 
  {
    //ptr will be needed for the Callback fuckery
    ESP_NOW_MASTER::ptr = this;

    esp_now_register_send_cb([](const uint8_t *mac_addr, esp_now_send_status_t status){ptr->OnDataSent(mac_addr, status);});
    esp_now_register_recv_cb([](const uint8_t * mac_addr, const uint8_t *incomingData, int len){ptr->OnDataRecv(mac_addr, incomingData, len);});
  }
  else
  {
    Serial.println("Error initializing ESP-NOW");
  }
}


void ESP_NOW_MASTER::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}

void ESP_NOW_MASTER::OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len)
{ 
  // first message byte is the type of message 
  uint8_t type = incomingData[0];       
  
  switch (type)
  {
  case LIGHTING_DATA :
    sendingData_t<std::array<uint8_t, 5>> lightingData;
    memcpy(&lightingData, incomingData, sizeof(lightingData));

    for(const auto& ledValue : lightingData.data)
    {
      Serial.print(ledValue);
      Serial.print(" ");
    }
    Serial.println();
    break;

  case SAVE:
    Serial.println("Save Lighting Conditon");
    break;

  case PAIRING:
    pairing_t pairingData;

    memcpy(&pairingData, incomingData, sizeof(pairingData));
    
    pairingData.senderType = MASTER;
    Serial.println("send response");

    addPeer(mac_addr);

    esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &pairingData, sizeof(pairingData));
    if(result == ESP_OK)
    {
      Serial.println("Sent Pairing Respons");
    }
    else
    {
      //Maybe remove peer againg
      Serial.println("Error while sending Pairing Respons");
    } 
    break; 
  }
}

//Make it usable
uint8_t ESP_NOW_MASTER::autoPairing()
{
  return PAIR_PAIRED;
}





//Slave declaration
ESP_NOW_SLAVE* ESP_NOW_SLAVE::ptr = nullptr;

ESP_NOW_SLAVE::ESP_NOW_SLAVE(uint8_t* loopState, std::vector<LED>* ledVector) : ESP_NOW_BASE(loopState,ledVector)
{
  stateSelf = SLAVE;

  if (esp_now_init() == ESP_OK) 
  {
    //ptr will be needed for the Callback fuckery
    ESP_NOW_SLAVE::ptr = this;

    esp_now_register_send_cb([](const uint8_t *mac_addr, esp_now_send_status_t status){ptr->OnDataSent(mac_addr, status);});
    esp_now_register_recv_cb([](const uint8_t * mac_addr, const uint8_t *incomingData, int len){ptr->OnDataRecv(mac_addr, incomingData, len);});
  }
  else
  {
    Serial.println("Error initializing ESP-NOW");
  }

}

void ESP_NOW_SLAVE::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  //Maybe Remove to save time?
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}


void ESP_NOW_SLAVE::OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len)
{ 
  // first message byte is the type of message 
  uint8_t type = incomingData[0];
  Serial.print("Type: ");
  Serial.println(type);

  switch (type) 
  {
  case LIGHTING_DATA:                           // the message is data type
    sendingData_t<std::array<uint8_t, 5>> lightingData;
    memcpy(&lightingData, incomingData, sizeof(lightingData));

    for(const auto& ledValue : lightingData.data)
    {
      Serial.print(ledValue);
      Serial.print(" ");
    }
    Serial.println();
    break;

  case SAVE:
    Serial.println("Save Lighting Conditon");
    break;

  case PAIRING:                            // the message is a pairing request 
    pairing_t pairingData;

    memcpy(&pairingData, incomingData, sizeof(pairingData));
    
    //As all the AutoParing Requst are Broadcastet and will be heared by all slaves, check if Information came Peer to Peer from a Master
    if(pairingData.senderType == MASTER)
    {
      //If the Master Responded accept him as the Connection

      if(addPeer(mac_addr))
      {
        pairingStatus = PAIR_PAIRED;
      }
    }
    break; 
  }
}

uint8_t ESP_NOW_SLAVE::autoPairing()
{
  pairing_t pairingData;
  uint8_t tries = 0;
  uint32_t currentMillis = 0;
  uint32_t previousMillis = 0;
  
  while(tries < 6)
  {
    switch(pairingStatus) 
    {
      case PAIR_REQUEST:   
        //Try again with a new ESP-NOW initialization
        esp_now_deinit();

        if (esp_now_init() != 0) 
        {
          Serial.println("Error initializing ESP-NOW");
          break;
        }

        esp_now_register_send_cb([](const uint8_t *mac_addr, esp_now_send_status_t status){ptr->OnDataSent(mac_addr, status);});
        esp_now_register_recv_cb([](const uint8_t * mac_addr, const uint8_t *incomingData, int len){ptr->OnDataRecv(mac_addr, incomingData, len);});

        addPeer(broadcastAddressX);
      
        // set pairing data to send to the server
        pairingData.msgType = PAIRING;
        pairingData.senderType = SLAVE;
        previousMillis = millis();

        //Make itself kown to any master Device out there
        esp_now_send(broadcastAddressX, (uint8_t *) &pairingData, sizeof(pairingData));
        pairingStatus = PAIR_REQUESTED;
        break;

      case PAIR_REQUESTED:
        // time out to allow receiving response from server
        currentMillis = millis();
        if(currentMillis - previousMillis > 200) 
        {
          previousMillis = currentMillis;
          // time out expired,  try again
          tries++;

          pairingStatus = PAIR_REQUEST;           
        }
        break;

      case PAIR_PAIRED:
        tries = 6;
        break;
    }   
  }

  return pairingStatus;
}