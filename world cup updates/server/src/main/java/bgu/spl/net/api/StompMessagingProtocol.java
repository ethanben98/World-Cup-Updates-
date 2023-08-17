package bgu.spl.net.api;

import bgu.spl.net.srv.Connections;
import java.util.ArrayList;
import java.util.Map;


public interface StompMessagingProtocol<T> {
	/**
	 * Used to initiate the current client protocol with it's personal connection ID and the connections implementation
	**/
    void start(int connectionId, Connections<T> connections);
    
    void process(T message);
	
	/**
     * @return true if the connection should be terminated
     */
    boolean shouldTerminate();

    public Boolean checkIfErrorFrame(T msg);

    public void terminate();

    public void sendErrorFrame(String error_msg, T msg);

    public String getLogin();

    


    

}
