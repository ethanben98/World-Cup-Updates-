package bgu.spl.net.srv;
import bgu.spl.net.api.MessageEncoderDecoder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.Iterator;

public class MessageEncoderDecoderImpl implements MessageEncoderDecoder<Map<String, String>> {

    private byte[] bytes = new byte[1 << 10]; //start with 1k
    private int len = 0;
    private Map<String, String> msg_object = new HashMap<String, String>();
    private String stage = "command";
    /**
     * add the next byte to the decoding process
     *
     * @param nextByte the next byte to consider for the currently decoded
     * message
     * @return a message if this byte completes one or null if it doesnt.
     */

    @Override
    public Map<String, String> decodeNextByte(byte nextByte) {
        //notice that the top 128 ascii characters have the same representation as their utf-8 counterparts
        //this allow us to do the following comparison
        if (nextByte == '\n') {
            String str = popString();
            pushString(str);
        }
        else if(nextByte == '\u0000'){ // @TODO: check that this works
            stage = "command";
            len = 0;
            return msg_object;
        }
        if(nextByte != '\n' && nextByte != '\u0000'){
            pushByte(nextByte);
        }
        return null; //not a line yet
    }

    public byte[] encode(Map<String, String> msg_obj) {
        String msg = Utils.buildString(msg_obj);
        return (msg+'\u0000').getBytes(); //uses utf8 by default

    }

    

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }

        bytes[len++] = nextByte;
    }

    private String popString() {
        //notice that we explicitly requesting that the string will be decoded from UTF-8
        //this is not actually required as it is the default encoding in java.
        String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
        len = 0;
        return result;
    }

    private void pushString(String str) {
        if(str.equals("") && stage.equals("headers")){
            // we reached end of headers
            stage = "body";
            return;
        }
        if(stage.equals("command")){
            msg_object.clear();
            msg_object.put("command", str.toUpperCase());
            // change stage
            stage = "headers";
        }
        else{
            if(stage.equals("headers")){
                String[] header = str.split(" : ");
                msg_object.put(header[0], header[1]);
            }
            if(stage.equals("body")){
                String prev = "";
                if(msg_object.containsKey("body")){
                    prev = msg_object.get("body") + '\n';
                }
                msg_object.put("body", prev + str);
            }
        }
    }


}
