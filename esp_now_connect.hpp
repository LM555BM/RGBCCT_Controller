#pragma once

#include "led.hpp"
#include <esp_now.h>
#include <vector>

//Gloable Typedefs
enum pairingStatus_t{PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED, ALREADY_PAIRED};
enum messageType_t{LIGHTING_DATA, SAVE, PAIRING};
enum stateSelf_t{MASTER, SLAVE};

class ESP_NOW_BASE
{
	public:
		struct pairing_t{
		  messageType_t msgType;
		  stateSelf_t senderType;
		};

    template <typename T>
    struct sendingData_t{
      messageType_t msgType;
      T data;
    };

    //Variables
    stateSelf_t stateSelf;

		//Methodes
    ESP_NOW_BASE(uint8_t* loopState, std::vector<LED>* ledVector);
    ~ESP_NOW_BASE();
		virtual esp_now_peer_info_t* autoPairing() = 0;
    uint8_t addPeer(const uint8_t *peer_addr); 

    template <typename T = char>
    esp_err_t sendLightingData(messageType_t messageType, std::vector<T>* data = nullptr)
    {
      esp_err_t errorMsg;

      if(data != nullptr)
      {
        sendingData_t<T> sendingData;
        sendingData.msgType = messageType;

        //Go through all Paired Slaves and new Data at the same time, until one is at the end
        for(int iterator = 0;  iterator < std::min(data->size(),connections.size()); iterator++)
        {
          sendingData.data = (*data)[iterator];
          errorMsg = esp_now_send(connections[iterator].peer_addr, (uint8_t *) &sendingData, sizeof(sendingData));
        }
      }
      else
      {
        sendingData_t<char> sendingData = {messageType,'\0'};    

        //Send all Paired Slaves the message Type
        for(const esp_now_peer_info_t& reciver : connections)
        {
          errorMsg = esp_now_send(reciver.peer_addr, (uint8_t *) &sendingData, sizeof(sendingData));
        }
      }
      return errorMsg;
    }
	
	protected:
		//Variables
    std::vector<LED>* lights;
    uint8_t* main_state;


		std::vector<esp_now_peer_info_t> connections;   
		uint8_t pairingStatus = PAIR_REQUEST;
		
		const uint8_t broadcastAddressX[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		
		//Methodes
		void printMAC(const uint8_t * mac_addr);
		
		virtual void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) = 0;
		virtual void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len)	= 0;
};



class ESP_NOW_MASTER : public ESP_NOW_BASE
{
	public:
    ESP_NOW_MASTER(uint8_t* loopState, std::vector<LED>* ledVector);
    esp_now_peer_info_t* autoPairing() override;

  protected:
    static ESP_NOW_MASTER* ptr;

    void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) override;
	  void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) override;
};



class ESP_NOW_SLAVE : public ESP_NOW_BASE
{
  public:
    ESP_NOW_SLAVE(uint8_t* loopState, std::vector<LED>* ledVector);
    esp_now_peer_info_t* autoPairing() override;  

  protected:
    static ESP_NOW_SLAVE* ptr;

    void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) override;
	  void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) override;
};