package bgu.spl.net.impl.stomp;
import bgu.spl.net.srv.BaseServer;
import bgu.spl.net.srv.Server;
import bgu.spl.net.srv.StompMessagingProtocolImpl;
import bgu.spl.net.srv.MessageEncoderDecoderImpl;

import java.util.Map;
import java.util.function.Supplier;

public class StompServer{
    public static void main(String[] args) {
        int port = Integer.parseInt(args[0]);
        String server_type = args[1];

        Server<Map<String, String>> server;
        if(server_type.equals("tpc")){
            server = Server.threadPerClient(port, () ->new StompMessagingProtocolImpl(), () ->new MessageEncoderDecoderImpl());
        }
        else{
            server = Server.reactor(3, port,  () ->new StompMessagingProtocolImpl(), () ->new MessageEncoderDecoderImpl());
        }

        server.serve();


    }
}
