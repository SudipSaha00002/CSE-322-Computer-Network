import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Base64;

public class DownloadService {
    private DownloadService() {
    }


    public static void handleDownload(ClientThread thread, String data) throws IOException {
        String[] parts = data.split(",", 2);
        String owner = parts[0];
        String filename = parts[1];

        File file = new File("server_files/" + owner + "/" + filename);

        if (!file.exists()) {
            thread.out.println("ERROR:File not found");
            return;
        }

        // for access control check
        // verifying if the requesting user has permission to download the file
        // here my file is ok but the other file is only public
        String visibility = thread.getFileVisibility(file);
        if (!owner.equals(thread.username) && !"public".equals(visibility)) {
            thread.out.println("ERROR:File is private");
            return;
        }

        // initate download
        // sending header to client with file info
        thread.out.println("DOWNLOAD_START:" + filename + "," + file.length());
        // reading and sending file in chunks
        FileInputStream fis = null;
        try {
            fis = new FileInputStream(file);
            byte[] buffer = new byte[FileServer.MAX_CHUNK_SIZE];
            int bytesRead;
            int chunkNum = 0;

            while ((bytesRead = fis.read(buffer)) != -1) {
                byte[] chunk;
                if (bytesRead < buffer.length) {
                    chunk = Arrays.copyOf(buffer, bytesRead);
                } else {
                    chunk = buffer;
                }

                String encodedChunk = Base64.getEncoder().encodeToString(chunk);
                thread.out.println("CHUNK:" + chunkNum + "," + encodedChunk);
                chunkNum++;
            }
        } catch (IOException e) {
            
        } 

       
        thread.out.println("DOWNLOAD_COMPLETE");
        thread.addMessage(thread.username, "Download of " + filename + " completed successfully!.");
        thread.logAction(filename, "download", "success");
    }

}
