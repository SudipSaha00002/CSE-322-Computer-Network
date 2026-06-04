import java.io.PrintWriter;

public class ReqSend {
    private ReqSend() {
    }

    public static void send(PrintWriter out, RespProvid respProvid, String recipient, String description)throws Exception {
        out.println("FILE_REQUEST:" + recipient + "," + description);
        String response = respProvid.get();
        if (response.startsWith("SUCCESS")) {
            System.out.println(response.substring(8));
        }
    }
}
