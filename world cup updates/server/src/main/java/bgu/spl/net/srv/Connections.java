package bgu.spl.net.srv;
import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;

public interface Connections<T> {

    void send(int connectionId, T msg);

    void send(String channel, T msg);

    String registerUser(String login, String pass, int id);

    public void initIdToHandler(ConnectionHandler<T> handler);

    public ConnectionHandler<T> getConnectionHandler(Integer id);

    public boolean isConnected(Integer uid);

    public void subscribe(String sub_id,int uid,String channel);

    public void unsubscribe(String sub_id,int uid,T msg);

    public void disconnect(String login);

}
