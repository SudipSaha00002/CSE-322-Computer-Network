import java.util.ArrayList;
import java.util.List;

public class FileTransfer {
    String filename;
    long fileSize;
    int chunkSize;
    String visibility;
    String reqId;
    List<byte[]> chunks = new ArrayList<>();
    long size = 0;

    public FileTransfer(String filename, long fileSize, int chunkSize, String visibility, String reqId) {
        this.filename = filename;
        this.fileSize = fileSize;
        this.chunkSize = chunkSize;
        this.visibility = visibility;
        this.reqId = reqId;
    }

    // method to add a chunk of data to the transfer to store the chunk in the correct index
    // and updating  the total size of the received data
    public void addChunk(int chunkNum, byte[] data) {
        while (chunks.size() <= chunkNum)
            chunks.add(null);
        chunks.set(chunkNum, data);
        size += data.length;
    }
}
