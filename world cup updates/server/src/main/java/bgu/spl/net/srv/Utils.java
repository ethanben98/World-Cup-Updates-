package bgu.spl.net.srv;
import java.util.Map;
import java.util.Set;
import java.util.Iterator;

public class Utils {
    public static String buildString(Map<String, String> msg_obj){
        String msg =  msg_obj.get("command") + '\n';
        Set<String> keys = msg_obj.keySet();
        Iterator<String> itr = keys.iterator(); 
        while(itr.hasNext()){
            String key = itr.next();
            if(key.equals("command") || key.equals("body")){
                continue;
            }
            msg += key + ":" + msg_obj.get(key) + "\n";
        }
        msg += "\n";
        msg += msg_obj.get("body") + '\n';
        return msg;
    }
}
