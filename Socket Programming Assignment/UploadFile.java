import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Base64;

public class UploadFile implements Runnable {
    private final PrintWriter out;
    private final RespProvid respProvid;
    private final String username;
    private final File file;
    private final String fileId;
    private final int chunkSize;
    private final String saveName;

    public UploadFile(PrintWriter out, RespProvid respProvid, String username, File file, String fileId,int chunkSize, String saveName) {
        this.out = out;
        this.respProvid = respProvid;
        this.username = username;
        this.file = file;
        this.fileId = fileId;
        this.chunkSize = chunkSize;
        this.saveName = saveName;
    }

    @Override

    public void run() {
    FileInputStream fis = null;
    try {
        fis = new FileInputStream(file);
        byte[] buffer = new byte[chunkSize];
        int read;
        int chunkNum = 0;

        while ((read = fis.read(buffer)) != -1) {
            byte[] chunk;
            if (read < buffer.length) {
                chunk = Arrays.copyOf(buffer, read);
            } else {
                chunk = buffer;
            }

            String encodedChunk = Base64.getEncoder().encodeToString(chunk);
            // sending each chunk to server
            // synchronized block to avoid interleaving of messages
            // when multiple threads are using the same PrintWriter
            
            synchronized (out) {
                out.println("UPLOAD_CHUNK:" + fileId + "," + username + "," + chunkNum + "," + encodedChunk);
            }

            String ack = respProvid.get();
            if (!ack.startsWith("Acknowledged")) {
                System.out.println("Upload aborted at chunk " + chunkNum);
                return;
            }

            chunkNum++;
        }
        // all the chunks when uploaded
        // sends the complete message to server
        synchronized (out) {
            out.println("UPLOAD_COMPLETE:" + fileId + "," + username + "," + saveName);
        }

        respProvid.get();

        System.out.println("\nUpload completed successfully.");
    } catch (Exception e) {
        System.out.println("Error during upload: " + e.getMessage());
    }
}

}
