package bgu.spl.net.srv;
import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ConnectionsImpl<T> implements Connections<T> {
    
    public ConcurrentHashMap<Integer,Integer> uid_connected;  // client is connected
    public ConcurrentHashMap<String,Integer> sub_id_to_uid;
    public ConcurrentHashMap<Integer,ConnectionHandler<T>> uid_to_handler;
    public ConcurrentHashMap<String,ConcurrentLinkedQueue<String>> channel_to_sub_ids;
    public ConcurrentHashMap<String,String> sub_id_to_channel;
    public ConcurrentHashMap<String,String> users;
    public ConcurrentHashMap<String,Integer> login_to_uid;  // user is connected
    public ConcurrentHashMap<Integer,ConcurrentLinkedQueue<String>> uid_to_sub_ids;  
    public Integer uid;
    public Integer msg_uid;




    public ConnectionsImpl(){
        this.uid_to_handler = new ConcurrentHashMap<Integer,ConnectionHandler<T>>();
        this.channel_to_sub_ids = new ConcurrentHashMap<>();
        this.sub_id_to_channel = new ConcurrentHashMap<>();
        this.users = new ConcurrentHashMap<String,String>();
        this.login_to_uid = new ConcurrentHashMap<String,Integer>();
        this.uid_connected = new ConcurrentHashMap<Integer,Integer>();
        this.sub_id_to_uid = new ConcurrentHashMap<String,Integer>();
        this.uid_to_sub_ids = new ConcurrentHashMap<>();

        uid = 0;
        msg_uid = 0;

    }

    public void send(int connectionId, T msg){
        ConnectionHandler<T> handler = this.uid_to_handler.get(connectionId);
        handler.send(msg);
    }

    public void send(String channel, T msg){
        //TO ASK: CAN WE DO THIS CASTING
        //MOVE THIS LOGIC TO STOMP PROTOCOL WITH GETTER
        Map<String,String> msg_obj = (Map<String,String>) msg;
        msg_obj.put("message-id", msg_uid + "");
        msg_uid++;
        ConcurrentLinkedQueue<String> ids = this.channel_to_sub_ids.get(channel);
        Iterator<String> iter = ids.iterator();
        while(iter.hasNext()){
            String[] sub_id = iter.next().split("_");
            msg_obj.put("subscription", sub_id[1]);
            send(this.sub_id_to_uid.get(String.join("_",sub_id)), msg);
        }
    }

    public void disconnect(String login){
        if(login == null){
            return;
        }
        int uid = login_to_uid.get(login);
        ConcurrentLinkedQueue<String> sub_ids = this.uid_to_sub_ids.get(uid);
        if(sub_ids == null){
            sub_ids = new ConcurrentLinkedQueue<>();
        }
        uid_to_sub_ids.remove(uid);
        login_to_uid.remove(login);
        uid_connected.remove(uid);
        Iterator<String> iter = sub_ids.iterator();
        while(iter.hasNext()){
            String sub_id = iter.next();
            sub_id_to_uid.remove(uid+ "_" +sub_id);
            String channel = sub_id_to_channel.get(uid+ "_" +sub_id);
            ConcurrentLinkedQueue<String> ch_sub_ids =  channel_to_sub_ids.get(channel);
            ch_sub_ids.remove(uid+ "_" +sub_id);
            channel_to_sub_ids.put(channel, ch_sub_ids);
            sub_id_to_channel.remove(uid+ "_" +sub_id);
            
        }
    }

    // Utils
    public String registerUser(String login, String pass, int id){
        String uPass = users.get(login);
        if(uPass == null){
            users.put(login, pass);
            login_to_uid.put(login, id);
            uid_connected.put(id, 1);
            return "Connected".toUpperCase();
        }
        if(!uPass.equals(pass)){
            return "Wrong Password";
        }
        if(login_to_uid.get(login) != null){
            // @TODO: if socket is closed by cntrl c, server should disconnect the user?
            return "User already logged in";  
        }
        Integer is_connected = uid_connected.get(id);
        if(is_connected != null){
            return "The Client is already logged in, log out before trying again";
        }
        uid_connected.put(id, 1);
        login_to_uid.put(login, id);
        return "Connected".toUpperCase();
    }

    public void initIdToHandler(ConnectionHandler<T> handler){
        uid_to_handler.put(uid, handler);
        handler.getProtocol().start(uid, this);
        uid++;
    }

    public ConnectionHandler<T> getConnectionHandler(Integer id){
        return uid_to_handler.get(id);
    }

    public boolean isConnected(Integer uid){
        return uid_connected.get(uid) != null;
    }

    public void subscribe(String sub_id,int uid,String channel){
        this.sub_id_to_uid.put(uid+"_"+sub_id, uid);
        ConcurrentLinkedQueue<String> uid_sub_ids = uid_to_sub_ids.get(uid);
        if(uid_sub_ids == null){
            uid_sub_ids = new ConcurrentLinkedQueue<>();
        }
        uid_sub_ids.add(sub_id);
        uid_to_sub_ids.put(uid,uid_sub_ids);
        ConcurrentLinkedQueue<String> sub_ids = this.channel_to_sub_ids.get(channel);
        if(sub_ids == null){
            sub_ids = new ConcurrentLinkedQueue<>();
        }
        sub_ids.add(uid+"_"+sub_id);
        this.channel_to_sub_ids.put(channel, sub_ids);
        this.sub_id_to_channel.put(uid+"_"+sub_id, channel);
        this.sub_id_to_uid.put(uid+"_"+sub_id,uid);
    }

    public void unsubscribe(String sub_id,int uid,T msg){
        ConcurrentLinkedQueue<String> uid_sub_ids = uid_to_sub_ids.get(uid);
        uid_sub_ids.remove(sub_id);
        uid_to_sub_ids.put(uid,uid_sub_ids);
        this.sub_id_to_uid.remove(uid+"_"+sub_id);
        String channel = this.sub_id_to_channel.get(uid+"_"+sub_id);
        if (channel == null){
            uid_to_handler.get(uid).getProtocol().sendErrorFrame("INVALID ID", msg);
            return;
        }
        ConcurrentLinkedQueue<String> sub_ids = this.channel_to_sub_ids.get(channel);
        sub_ids.remove(uid+"_"+sub_id);
        this.channel_to_sub_ids.put(channel, sub_ids);
        this.sub_id_to_channel.remove(uid+"_"+sub_id);
    }

}
