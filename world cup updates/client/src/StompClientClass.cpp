#include <stdlib.h>
#include "../include/ConnectionHandler.h"
#include <thread>
#include <vector>
#include "../include/StompClientClass.h"
#include "../include/event.h"
#include <cctype>
#include <iostream>
#include <fstream>



StompClientClass::StompClientClass(): subid(0), receipt_id(0), game_to_subid(std::map<std::string,int>()), 
user_to_events(std::map<std::string,std::map<std::string,event_data>>()), user(),
 receipt_id_to_frame(std::map<int,std::map<std::string,std::string>>()), summary_lock(),
    receipt_id_to_frame_lock(), connectionHandler("0",0), connected(0), game_to_subid_lock()
 {}


	
int StompClientClass::init(){
	//From here we will see the rest of the ehco client implementation:
    while (1) {
        const short bufsize = 1024;
        char buf[bufsize];
        std::cin.getline(buf, bufsize);
		std::string line(buf);
		buildFrame(line);
    }
    return 0;
}

void StompClientClass::readSocketHandler(){
    while (1) {
        int len;
        // We can use one of three options to read data from the server:
        // 1. Read a fixed number of characters
        // 2. Read a line (up to the newline character using the getline() buffered reader
        // 3. Read up to the null character
        std::string answer;
        // Get back an answer: by using the expected number of bytes (len bytes + newline delimiter)
        // We could also use: connectionHandler.getline(answer) and then get the answer without the newline char at the end
        if (!connectionHandler.getLine(answer)) {
            std::cout << "Disconnected. Exiting...\n" << std::endl;
            connectionHandler.close();
            break;
        }
		len=answer.length();
		// A C string must end with a 0 char delimiter.  When we filled the answer buffer from the socket
		// we filled up to the \n char - we must make sure now that a 0 char is also present. So we truncate last character.
        answer.resize(len-1);
        handleServerResponse(answer);
        if (answer == "bye") {
            std::cout << "Exiting...\n" << std::endl;
            break;
        }
    }
}

void StompClientClass::handleServerResponse(std::string answer){
    size_t pos = 0;
	std::vector<std::string> input;
    while((pos = answer.find("\n")) != std::string::npos){
		input.push_back(answer.substr(0,pos));
		answer.erase(0,pos + 1);
	}
    input.push_back(answer);
    std::string command = input[0];
    if (command == "CONNECTED"){
        connected = 1;
        std::cout<<"Login successful\n";
    }
    else if(command == "MESSAGE"){
        parseMessage(input);

    }
    else if(command == "RECEIPT"){
        pos = input[1].find(":");
        int receipt_id = std::stoi(input[1].substr(pos+1,input[1].length()));
        try{
            std::lock_guard<std::mutex> lock(receipt_id_to_frame_lock);
            std::map<std::string,std::string> frame = receipt_id_to_frame.at(receipt_id);
            std::string receipt_command = frame.at("command");
            if (receipt_command == "SUBSCRIBE"){
                std::string game = frame.at("destination");
                game = game.substr(1,game.length());
                int id = std::stoi(frame.at("id"));
                std::unique_lock<std::mutex> lck(game_to_subid_lock);
                game_to_subid[game] = id;
                lck.unlock(); 
                std::cout<<"Joined channel " + game +"\n";
            }
            if (receipt_command == "UNSUBSCRIBE"){
                std::string game = frame.at("game");
                std::unique_lock<std::mutex> lck(game_to_subid_lock);
                game_to_subid.erase(game);
                lck.unlock(); 
                std::cout<<"Exited channel " + game +"\n";

            }
            if (receipt_command == "DISCONNECT"){
                connected = 0;
                game_to_subid.clear();
                receipt_id_to_frame.clear();
                connectionHandler.close();
            }

        }
        catch(...){
            std::cout << "Unknown receipt id"<<std::endl;
        }
    }
    else if(command == "ERROR"){
        int index = 1;
        while(input[index] != ""){
            pos = input[index].find(":");
            std::string header = input[index].substr(0,pos);
            if(header == "message"){
                std::cerr << input[index].substr(pos+1,input[index].length())+"\n";
                break;
            }
            else{
                index++;
            }
        }
    }
}

void StompClientClass::parseMessage(std::vector<std::string> input){
    int index = 1;
    int pos,time;
    std::string team_a, team_b,user,event_name,desc,map_flag;
    bool body = false;
    std::map<std::string,std::string> gen,a,b;
    while(index < input.size()){
        if(input[index] == ""){
            body = true;
            index++;
            continue;
        } 
        else if(body == false)
        {
            pos = input[index].find(":");
            std::string header = input[index].substr(0,pos);
            if(header == "destination"){
                std::string match = input[index].substr(pos+1,input[index].length());
                pos = match.find("_");
                team_a = match.substr(1,pos-1);
                team_b = match.substr(pos+1,match.length());
            }
            index++;
        }
        else{
            std::vector<std::string> parts = splitLine(input[index]," : ");
            if(parts[0] == "user"){
                user = parts[1];
            }
            else if(parts[0] == "event name"){
                event_name = parts[1];

            }
            else if(parts[0] == "time"){
                time = std::stoi(parts[1]);
            }
            else if(parts[0] == "general game updates"){
                map_flag = "gen";
            }
            else if(parts[0] == "team a updates"){
                map_flag = "a";
            }
            else if(parts[0] == "team b updates"){
                map_flag = "b";
            }
            else if(parts[0] == "description"){
                index++;
                while(input[index] != "\0"){
                    desc+=input[index]+"\n";
                    index++;
                }
                break;
            }
            else{
                if(parts[0].substr(0,1) == " "){
                    try{
                        if(map_flag == "gen"){
                            gen[parts[0].substr(4,parts[0].length())] = parts[1];
                        }
                        else if(map_flag == "a"){
                            a[parts[0].substr(4,parts[0].length())] = parts[1];
                        }
                        else if(map_flag == "b"){
                            b[parts[0].substr(4,parts[0].length())] = parts[1];
                        }
                    }
                    catch(...){}
                }
            }
            index++;
        }
        
    }
    Event event (team_a,team_b,event_name,time,gen,a,b,desc);
    updateData(event,user,team_a+"_"+team_b);


}

std::vector<std::string> StompClientClass::splitLine(std::string line,std::string deli){
    int pos = line.find(deli);
    std::string header = line.substr(0,pos);
    std::string value = line.substr(deli.length()+pos,line.length());
    std::vector<std::string> parts;
    parts.push_back(header);
    parts.push_back(value);
    return parts;
}

void StompClientClass::send(std::string line){
    int len=line.length();
    if (!connectionHandler.sendLine(line)) {
            std::cout << "Disconnected. Exiting...\n" << std::endl;
        }
		// connectionHandler.sendLine(line) appends '\n' to the message. Therefor we send len+1 bytes.
}

void StompClientClass::buildFrame(std::string line){
    size_t pos = 0;
	std::vector<std::string> input;
	while((pos = line.find(" ")) != std::string::npos){
		input.push_back(line.substr(0,pos));
		line.erase(0,pos + 1);
	}
    input.push_back(line);
    if(input[0] != "login" && connected == 0){
        std::cerr<<"you must connect first!\n";
        return;
    }
    else if(input[0] == "login" && connected == 1){
        std::cerr<<"The client is already logged in, log out before trying again\n";
        return;
    }
	if(input[0] == "login"){
		handleConnect(input);
	}
	else if(input[0] == "join"){
		handleJoin(input);
	}
	else if(input[0] == "exit"){
		handleExit(input);
	}
	else if(input[0] == "report"){
		handleReport(input);
	}
	else if(input[0] == "summary"){
		handleSummary(input);
	}
	else if(input[0] == "logout"){
		handleLogout(input);
	}
}

void StompClientClass::handleConnect(std::vector<std::string> input){
    std::vector<std::string> connection_details = splitLine(input[1],":");
    int port = std::stoi(connection_details[1]);
    connectionHandler.override(connection_details[0],port);
    if (!connectionHandler.connect()) {
        std::cerr << "Could not connect to server\n";
        return;
    }
    std::thread socketReader(&StompClientClass::readSocketHandler,this); 
    socketReader.detach();
    user = input[2];
    std::map<std::string,std::string> frame;
    frame.insert({"command","CONNECT"});
    frame.insert({"accept-version","1.2"});
    frame.insert({"host","stomp.cs.bgu.ac.il"});//what is host and port page 12 (input[1])
    frame.insert({"login",input[2]});
    frame.insert({"password",input[3]});
    frame.insert({"body"," "});
    //remember to init user to input
    send(frameToString(frame));
}

void StompClientClass::handleJoin(std::vector<std::string> input){
    std::map<std::string,std::string> frame;
    frame.insert({"command","SUBSCRIBE"});
    frame.insert({"destination","/" + input[1]});
    frame.insert({"id",std::to_string(subid)});
    frame.insert({"receipt",std::to_string(receipt_id)});
    frame.insert({"body",""});
    std::lock_guard<std::mutex> lock(receipt_id_to_frame_lock);
    receipt_id_to_frame.insert({receipt_id,frame});
    subid++;
    receipt_id++;
    send(frameToString(frame)); 
}

void StompClientClass::handleExit(std::vector<std::string> input){
	std::map<std::string,std::string> frame;
    frame.insert({"command","UNSUBSCRIBE"});
    std::string s_id;
    try{
        std::unique_lock<std::mutex> lck(game_to_subid_lock);
        s_id = std::to_string(game_to_subid.at(input[1]));
        lck.unlock();       
        frame.insert({"id",s_id});
        frame.insert({"receipt",std::to_string(receipt_id)});
        frame.insert({"body",""});
        std::string str_frame = frameToString(frame);
        frame.insert({"game",input[1]});
        std::lock_guard<std::mutex> lock(receipt_id_to_frame_lock);
        receipt_id_to_frame.insert({receipt_id,frame});
        receipt_id++;
        send(str_frame);
    }
    catch(...){
        std::cout << "User is not subscribed to this game"<<std::endl;
    }
    
}
void StompClientClass::handleReport(std::vector<std::string> input){
    names_and_events e = parseEventsFile(input[1]);
    std::string game = e.team_a_name + "_" + e.team_b_name;
    try{
        std::unique_lock<std::mutex> lck(game_to_subid_lock);
        int sid = game_to_subid.at(game);
        lck.unlock(); 
    }
    catch(...){
        std::cout << "User is not subscribed to this game"<<std::endl;
        return;
    }
    std::vector<Event> events = e.events;
    for(Event event:events){
        std::string tab = "    ";
        std::map<std::string,std::string> frame;
        frame.insert({"command","SEND"});
        frame.insert({"destination","/"+game});
        std::string body = "user : " + user + '\n';
        body += "event name : " + event.get_name() + "\n";
        body += "time : " + std::to_string(event.get_time()) + "\n";
        body += "general game updates : \n";
        std::map<std::string,std::string> updates = event.get_game_updates();
        for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
            body+= tab + it->first + " : " + it->second + "\n";
        }
        body += "team a updates : \n";
        updates = event.get_team_a_updates();
        for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
            body+= tab + it->first + " : " + it->second + "\n";
        }
        body += "team b updates : \n";
        updates = event.get_team_b_updates();
        for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
            body+= tab + it->first + " : " + it->second + "\n";
        }
        body += "description : \n";
        body += "\"" + event.get_discription() + "\"\n\0";
        frame.insert({"body",body});
        std::string frame_str = frameToString(frame);
        std::lock_guard<std::mutex> lock(summary_lock); 
        std::map<std::string,event_data> game_to_events;
        try{
            game_to_events = user_to_events.at(user);
        }
        catch(...){}
        event_data game_events;
        try{
            game_events = game_to_events.at(game);
        }
        catch(...){
            game_events = {
                event.get_team_a_name() + " vs " + event.get_team_b_name(),
                event.get_team_a_name(),
                event.get_team_b_name(),
                std::map<std::string,std::string>(),
                std::map<std::string,std::string>(),
                std::map<std::string,std::string>(),
                "",
            };
        }
        updateStruct(game_events,event);
        //TODO: check if need to insert game_events back into map !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        send(frame_str);
    }

	
}

void StompClientClass::updateData(Event event,std::string user, std::string game){
    std::lock_guard<std::mutex> lock(summary_lock); 
    std::map<std::string,event_data> game_to_events;
    
    if(user_to_events.find(user) == user_to_events.end()){
        game_to_events = std::map<std::string,event_data>();
        user_to_events.insert({user,game_to_events});
    }
    else{
        game_to_events = user_to_events.at(user);
    }
    event_data game_events;
    if(game_to_events.find(game) == game_to_events.end()){
        game_events = {
            event.get_team_a_name() + " vs " + event.get_team_b_name(),
            event.get_team_a_name(),
            event.get_team_b_name(),
            std::map<std::string,std::string>(),
            std::map<std::string,std::string>(),
            std::map<std::string,std::string>(),
            "",
        };
        game_to_events.insert({game,game_events});
    }
    else{
        game_events = game_to_events.at(game);
    }
    updateStruct(game_events,event);
    game_to_events[game] = game_events;
    user_to_events[user] = game_to_events;
}

void StompClientClass::updateStruct(event_data &game_events, Event event){
    std::map<std::string,std::string> updates = event.get_game_updates();
	for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
        game_events.general_stats[it->first] = it->second;    
    }
    updates = event.get_team_a_updates();
    for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
        game_events.a_stats[it->first] = it->second;    
    }
    updates = event.get_team_b_updates();
    for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
        game_events.b_stats[it->first] = it->second;    
    }
    std::string desc = std::to_string(event.get_time()) + " - " + event.get_name() +":\n\n" + event.get_discription() +"\n\n";
    game_events.description += desc;

}


void StompClientClass::handleSummary(std::vector<std::string> input){
    std::string user = input[2];
    std::string game = input[1];
    std::string file = input[3];
    event_data game_data;
    std::map<std::string,event_data> game_to_events;
    try{
        game_to_events = user_to_events.at(user);
        game_data = game_to_events.at(game);
        std::string summary = game_data.game + "\n" + "Game stats:" + "\n";
        summary += "General stats:\n";
        std::map<std::string,std::string> updates = game_data.general_stats;
        for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
            summary += it->first + ": " + it->second + "\n";    
        }
        summary += game_data.team_a + " stats:\n";
        updates = game_data.a_stats;
        for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
            summary += it->first + ": " + it->second + "\n";    
        }
        summary += game_data.team_b + " stats:\n";
        updates = game_data.b_stats;        
        for(std::map<std::string,std::string>:: iterator it = updates.begin(); it != updates.end(); ++it){
            summary += it->first + ": " + it->second + "\n";    
        }
        summary += game_data.description;

        try{
            std::ofstream myFile(file);
            myFile << summary;
            myFile.close();
        } 
        catch(...){
            std::cout<<"Couldn't write to file";           
        }


    }
    catch(...){
        std::cout<<"Error: User does not exist or didn't make a report for this game\n";
    }

	
}
void StompClientClass::handleLogout(std::vector<std::string> input){
	std::map<std::string,std::string> frame;
    frame.insert({"command","DISCONNECT"});
    frame.insert({"receipt",std::to_string(receipt_id)});
    frame.insert({"body",""});
    std::lock_guard<std::mutex> lock(receipt_id_to_frame_lock);
    receipt_id_to_frame.insert({receipt_id,frame});
    receipt_id++;
    send(frameToString(frame));
}

std::string StompClientClass::frameToString(std::map<std::string,std::string> frameMap){
    std::string frame = frameMap.at("command") + '\n';
    for(std::map<std::string,std::string>:: iterator it = frameMap.begin(); it != frameMap.end(); ++it){
        std::string key = it->first;
        if (key == "command" || key == "body"){
            continue;
        }
        frame += it->first +" : " + it->second + "\n";
    }
    frame += "\n";
    frame += frameMap.at("body") +"\n";
    return frame;
}
