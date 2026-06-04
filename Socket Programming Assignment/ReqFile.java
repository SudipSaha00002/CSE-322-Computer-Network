import java.util.HashSet;
import java.util.Set;

public class ReqFile {
    private ReqFile() {
    }

    public static void fileReq(ClientThread thread, String data) {
        String[] p = data.split(",", 2);
        String client = p[0].trim();
        String desc = p[1];

        long reqId = FileServer.ReqId++;
        String msg = "File Request #" + reqId + " from " + thread.username + ": " + desc;
        FileServer.reqIdTransfer.put(reqId, thread.username);

        System.out.println("File request from " + thread.username + " -> " +("ALL".equalsIgnoreCase(client) ? "EVERYONE" : client) + " (ID: " + reqId + ")");

        if ("ALL".equalsIgnoreCase(client)) {
            //send msg to all except the sender
            // 
            Set<String> clients = new HashSet<>(FileServer.logedInUser);
            clients.addAll(FileServer.pendingMsg.keySet());
            clients.remove(thread.username);

            for (String target : clients) {
                thread.addMessage(target, msg);
            }
        } else {
            //this ensure that the receiver client is known to the server
            // 
            if (!FileServer.logedInUser.contains(client)) {
                FileServer.logedInUser.add(client);
            }
            thread.addMessage(client, msg);
        }

        thread.out.println("SUCCESS:Request sent with ID " + reqId);
    }
}
