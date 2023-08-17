package bgu.spl.net.srv;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.api.StompMessagingProtocol;
import java.util.Map;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<Map<String, String>>{
    public Connections<Map<String, String>> connections;
    private int connectionId;
    private String login;
    private boolean shouldTerminate = false;
    String[] VALID_COMMANDS = {"CONNECT", "SEND", "SUBSCRIBE", "UNSUBSCRIBE", "DISCONNECT"};

    
    /**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    public void start(int connectionId, Connections<Map<String, String>> connections){
        this.connections = connections;
        this.connectionId = connectionId;
    }
    
    public void process(Map<String, String> msg ){
        String command = msg.get("command");
        if(command.equals("CONNECT") || verifyConnection(msg)){
            if(command.equals("CONNECT")){
                this.handleConnect(msg);
            }
            else if(command.equals("SUBSCRIBE")){
                this.handleSubscribe(msg);
            }
            else if(command.equals("UNSUBSCRIBE")){
                this.handleUnsubscribe(msg);
            }
            else if(command.equals("SEND")){
                this.handleSend(msg);
            }
            else if(command.equals("DISCONNECT")){
                this.handleDisconnect(msg);
            }
            else{
                sendErrorFrame("INVALID COMMAND!", msg);
            }
        }
        
    }
	
	/**
     * @return true if the connection should be terminated
     */
    public boolean shouldTerminate(){
        return this.shouldTerminate;
    }

    public void sendErrorFrame(String error_msg, Map<String, String> msg){
        Map<String, String> msg_object = new HashMap<String, String>();
        msg_object.put("command", "ERROR");
        msg_object.put("message", error_msg);
        if(msg.get("receipt") != null){
            msg_object.put("receipt-id", msg.get("receipt"));
        }
        String body = "The message:\n--------\n";
        body += Utils.buildString(msg);
        body += "\n--------\n";
        msg_object.put("body", body);
        connections.send(this.connectionId, msg_object);
        connections.disconnect(login);
        terminate();
    }

    private void sendReceiptFrame(Map<String, String> msg){
        Map<String, String> msg_object = new HashMap<String, String>();
        msg_object.put("command", "RECEIPT");
        msg_object.put("receipt-id", msg.get("receipt"));
        connections.send(this.connectionId, msg_object);
    }

    private void handleConnect(Map<String, String> msg){
        String version = msg.get("accept-version");
        String host = msg.get("host");
        String login_ = msg.get("login");
        String pass = msg.get("password");
        if(version == null || host == null || login_ == null || pass == null){
            sendErrorFrame("MISSING REQUIRED HEADERS", msg);
        }
        if(!version.equals("1.2") || !host.equals("stomp.cs.bgu.ac.il")){
            sendErrorFrame("INVALID VERSION OR HOST", msg);
        }
        String answer = this.connections.registerUser(login_, pass, this.connectionId);
        if(answer.equals("Connected".toUpperCase())){
            login = login_;
            sendConnectedFrame(msg);
        }
        else{
            sendErrorFrame(answer, msg);

        }
    }

    private void handleSubscribe(Map<String, String> msg){
        String channel = msg.get("destination");
        String sub_id = msg.get("id");
        connections.subscribe(sub_id,this.connectionId,channel);
        String receipt = msg.get("receipt");
        if(receipt != null){
            sendReceiptFrame(msg);
        }
    }

    private void handleUnsubscribe(Map<String, String> msg){
        String sub_id = msg.get("id");
        connections.unsubscribe(sub_id,this.connectionId,msg);
        String receipt = msg.get("receipt");
        if(receipt != null){
            sendReceiptFrame(msg);
        }
    }

    private void handleSend(Map<String, String> msg ){
        String channel = msg.get("destination");
        Map<String, String> response = new HashMap<String, String>();
        response.put("command", "MESSAGE");
        response.put("destination", channel);
        response.put("body", msg.get("body"));
        // @TODO: what to do if body is empty?
        connections.send(channel, response);
    }

    private void handleDisconnect(Map<String, String> msg ){
        connections.disconnect(this.login);
        sendReceiptFrame(msg);
        terminate();
    }

    public void sendConnectedFrame(Map<String, String> msg ){
        Map<String, String> msg_object = new HashMap<String, String>();
        msg_object.put("command", "CONNECTED");
        msg_object.put("version", msg.get("accept-version"));
        connections.send(this.connectionId, msg_object);
        if(msg.get("receipt") != null){
            sendReceiptFrame(msg);
        }
    }

    public Boolean checkIfErrorFrame(Map<String, String> msg){
        if(msg.get("command").equals("Error".toUpperCase())){
            this.shouldTerminate = true;
        }
        return this.shouldTerminate;
    }

    public void terminate(){
        this.shouldTerminate = true;
    }

    public boolean verifyConnection(Map<String, String> msg){
        if (!connections.isConnected(this.connectionId)){
            sendErrorFrame("YOU MUST CONNECT FIRST", msg);
            return false;
        }
        return true;
    }

    public String getLogin(){
        return login;
    }
    
}
