import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Base64;

public class UploadFileFunc {
    private UploadFileFunc() {
    }

    public static void uploadRequest(ClientThread thread, String data) throws IOException {

        String[] parts = data.split(",", 4);

        String filename = parts[0];
        long fileSize = Long.parseLong(parts[1]);

        String visibility = "private";
        if (parts.length > 2 && parts[2] != null && !parts[2].isEmpty()) {
            visibility = parts[2];
        }

        Long reqId = null;
        if (parts.length > 3 && parts[3] != null && !parts[3].equals("-1")) {
            reqId = Long.parseLong(parts[3]);
            // uploads for requests are always public
            visibility = "public";
        }

        // check server buffer size
        synchronized (FileServer.class) {
            long reqSize = FileServer.currBuffetSize + fileSize;

            if (reqSize > FileServer.MAX_BUFFER_SIZE) {
                thread.out.println("ERROR:Server buffer full. Try again later.");
                return;
            }

            FileServer.currBuffetSize = reqSize;
        }

        int chunkRange = FileServer.MAX_CHUNK_SIZE - FileServer.MIN_CHUNK_SIZE + 1;
        int chunkSize = FileServer.MIN_CHUNK_SIZE + FileServer.random.nextInt(chunkRange);

        // generating unique file id
        // combining current time and random long value

        String fileId = System.nanoTime() + "_" + Math.abs(FileServer.random.nextLong());

        FileTransfer transfer = new FileTransfer(
                filename,
                fileSize,
                chunkSize,
                visibility,
                reqId == null ? null : String.valueOf(reqId));

        FileServer.fileTransfer.put(fileId + "," + thread.username, transfer);

        thread.out.println("UPLOAD_READY:" + fileId + "," + chunkSize);
    }

    // it is receiving one chunk of dada at a time from the client
    // decoding the base64 data and storing it in the file transfer object
    public static void uploadChunk(ClientThread thread, String data) throws IOException {
        String[] parts = data.split(",", 4);
        String fileId = parts[0];
        String user = parts[1];
        int chunkNum = Integer.parseInt(parts[2]);
        String base64Data = parts[3];

        FileTransfer transfer = FileServer.fileTransfer.get(fileId + "," + user);
        if (transfer == null) {
            thread.out.println("ERROR:Transfer not found");
            return;
        }

        byte[] chunkData = Base64.getDecoder().decode(base64Data);
        transfer.addChunk(chunkNum, chunkData);

        thread.out.println("Acknowledged");
    }

    public static void uploadComplete(ClientThread thread, String data) throws IOException {
        String[] parts = data.split(",", 3);
        String fileId = parts[0];
        String user = parts[1];
        String finalName = parts[2];

        // retriving file transfer object
        // which contains all chunks and metadata
        FileTransfer transfer = FileServer.fileTransfer.get(fileId + "," + user);

        // checking for missing or incomplete file
        // if transfer is null or received size does not match expected file size
        // notify the client of the error and log the failure
        if (transfer == null || transfer.size != transfer.fileSize) {
            thread.out.println("ERROR:Incomplete file-size mismatch");
            thread.addMessage(user, "Upload of " + finalName + " failed - incomplete.");

            if (transfer != null) {
                thread.logAction(finalName, "upload", "failed - incomplete");

                synchronized (FileServer.class) {
                    FileServer.currBuffetSize -= transfer.fileSize;
                }
            }

            FileServer.fileTransfer.remove(fileId + "," + user);
            return;
        }

        // adjusting server buffer for completed upload
        synchronized (FileServer.class) {
            FileServer.currBuffetSize -= transfer.fileSize;
        }

        // writing chunks to final file
        File outFile = new File("server_files/" + thread.username, finalName);
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(outFile);
            for (byte[] chunk : transfer.chunks) {
                if (chunk != null) {
                    fos.write(chunk);
                }
            }
        } catch (Exception e) {
            thread.out.println("ERROR:Server write error");
            thread.addMessage(user, "Upload of " + finalName + " failed - server write error.");
            thread.logAction(finalName, "upload", "failed - write error");
            FileServer.fileTransfer.remove(fileId + "," + user);
            return;
        }

        // saving metadata and logging the file upload action

        thread.saveFileMetadata(finalName, transfer.visibility);
        thread.logAction(finalName, "upload", "success");
        FileServer.fileTransfer.remove(fileId + "," + user);

        thread.out.println("SUCCESS:File uploaded successfully!");
        thread.addMessage(thread.username, "File " + finalName + " uploaded successfully!.");

        // notifying if the upload file is for a request to a client
        // the requester is notified about the successful upload
        if (transfer.reqId != null) {
            Long reqId = Long.parseLong(transfer.reqId);
            String requester = FileServer.reqIdTransfer.get(reqId);
            if (requester != null) {
                thread.addMessage(requester, "User '" + thread.username + "' uploaded '" + finalName
                        + "' for your Request #" + transfer.reqId + "!");
            }
        }
    }

}
