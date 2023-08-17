#pragma once

#include "event.h"
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <map>
#include <mutex>

using boost::asio::ip::tcp;

struct event_data {
    std::string game;
    std::string team_a;
    std::string team_b;
    std::map<std::string,std::string> general_stats;
    std::map<std::string,std::string> a_stats;
    std::map<std::string,std::string> b_stats;
    std::string description;

};

class StompClientClass {
private:
	std::map<std::string,int> game_to_subid;
    std::map<std::string,std::map<std::string,event_data>> user_to_events;
    
	int subid;
	int receipt_id;
    std::string user;
    volatile int connected;
    std::map<int,std::map<std::string,std::string>> receipt_id_to_frame;
    std::mutex summary_lock;
    std::mutex receipt_id_to_frame_lock;
    std::mutex game_to_subid_lock;
    ConnectionHandler connectionHandler;

public:
	StompClientClass();

	int init();

    void parseMessage(std::vector<std::string>);
    void buildFrame(std::string line);
    void send(std::string msg);
    void updateData(Event event,std::string user, std::string game);
    std::vector<std::string> splitLine(std::string line,std::string deli);


    void handleConnect(std::vector<std::string> input);
    void handleJoin(std::vector<std::string> input);
    void handleExit(std::vector<std::string> input);
    void handleReport(std::vector<std::string> input);
    void handleSummary(std::vector<std::string> input);
    void handleLogout(std::vector<std::string> input);

    std::string frameToString(std::map<std::string,std::string> frame);

    void updateStruct(event_data &game_events, Event event);
    void readSocketHandler();
    void handleServerResponse(std::string answer);


}; 

