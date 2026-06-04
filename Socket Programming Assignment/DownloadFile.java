import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.util.Base64;

public class DownloadFile implements Runnable {
    private final PrintWriter out;
    private final RespProvid respProvid;
    private final String owner;
    private final String filename;

    public DownloadFile(PrintWriter out, RespProvid respProvid, String owner, String filename) {
        this.out = out;
        this.respProvid = respProvid;
        this.owner = owner;
        this.filename = filename;
    }

    @Override
    public void run() {
        try {
            
            synchronized (out) {
                out.println("DOWNLOAD:" + owner + "," + filename);
            }
            // 
            String response = respProvid.get();
            if (response.startsWith("ERROR")) {
                System.out.println("Download failed: " + response.substring(6));
                return;
            }

            if (!response.startsWith("DOWNLOAD_START:")) {
                return;
            }

            String[] parts = response.substring(15).split(",");
            String downloadFilename = parts[0];
            
            File downloadDir = new File("client_downloads");
            if (!downloadDir.exists())
                downloadDir.mkdirs();
            
            File outputFile = new File(downloadDir, downloadFilename);
            // reading the chunks and writing to file
        
            try (FileOutputStream fos = new FileOutputStream(outputFile)) {
                String line;
                while (true) {
                    line = respProvid.get();
                    if ("DOWNLOAD_COMPLETE".equals(line)) {
                        break;
                    }

                    if (line.startsWith("CHUNK:")) {
                        // extracting base64 data from the chunk message and writing to file 
                        String base64Data = line.substring(6).split(",", 2)[1];
                        byte[] chunkData = Base64.getDecoder().decode(base64Data);
                        fos.write(chunkData);
                    }
                }
            }
            System.out.println("\n Download completed successfully. " );
        } catch (Exception e) {
                       // System.out.println(" Download failed!");
        }
    }
}
