import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class ClientThread implements Runnable {
    final Socket socket;
    PrintWriter out;
    InputStream in;
    String username;
    boolean login = false;

    public ClientThread(Socket socket) {
        this.socket = socket;
    }

    @Override
    public void run() {
        try {
            out = new PrintWriter(socket.getOutputStream(), true);
            in = socket.getInputStream();

            out.println("ENTER_USERNAME");
            username = readLine();

            if (username == null || username.trim().isEmpty()) {
                out.println("ERROR:Invalid username");
                socket.close();
                return;
            }
            username = username.trim();

            if (FileServer.loginUser.contains(username)) { // client is already loged in
                out.println("ERROR:User already connected");
                socket.close();
                return;
            }

            FileServer.loginUser.add(username);
            FileServer.clientThread.put(username, this);
            login = true;
            FileServer.saveLogedInUsers(username); // separate the user directory for files for each user
            File userDir = new File("server_files/" + username);
            if (!userDir.exists())
                userDir.mkdirs();

            out.println("SUCCESS:Connected as " + username);
            System.out.println(username + " connected");

            List<String> pending = FileServer.pendingMsg.get(username); // sending unread messages to the client
            if (pending != null && !pending.isEmpty()) {
                for (String m : pending) {
                    out.println("NEW_MESSAGE:" + m);
                }
            }

            String req;
            while ((req = readLine()) != null) {
                handleRequest(req);
            }

        } catch (Exception e) {
            System.out.println("Connection failed: " + e.getMessage());
        } finally {
            if (login && username != null) {
                cleanup();
            }
        }
    }

    private String readLine() throws IOException {
        StringBuilder sb = new StringBuilder();
        int ch;

        while (true) {
            ch = in.read();
            if (ch == -1) {
                break;
            }

            if (ch == '\n') {
                break;
            }

            if (ch != '\r') {
                sb.append((char) ch);
            }
        }

        if (ch == -1 && sb.length() == 0) {
            return null;
        }

        return sb.toString();
    }

  
    private void handleRequest(String request) throws IOException {
        String[] parts = request.split(":", 2);
        String command = parts[0];

        switch (command) {
            case "LIST_CLIENTS":
                listClients();
                break;

            case "LIST_OWN_FILES":
                listOwnFiles();
                break;

            case "LIST_PUBLIC_FILES":
                listPublicFiles(parts[1]);
                break;

            case "UPLOAD_REQUEST":
                UploadFileFunc.uploadRequest(this, parts[1]);
                break;

            case "UPLOAD_CHUNK":
                UploadFileFunc.uploadChunk(this, parts[1]);
                break;

            case "UPLOAD_COMPLETE":
                UploadFileFunc.uploadComplete(this, parts[1]);
                break;

            case "DOWNLOAD":
                DownloadService.handleDownload(this, parts[1]);
                break;

            case "FILE_REQUEST":
                ReqFile.fileReq(this, parts[1]);
                break;

            case "VIEW_MESSAGES":
                viewMessages();
                break;

            case "VIEW_HISTORY":
                viewHistory();
                break;

            default:
                System.out.println("Unknown command: " + command);
        }
    }



    private void listClients() {
        StringBuilder sb = new StringBuilder("CLIENTS:");

        List<String> users = new ArrayList<>(FileServer.logedInUser);
        Collections.sort(users);

        for (String user : users) {
            if (FileServer.loginUser.contains(user)) {
                sb.append(user).append("(online)");
            } else {
                sb.append(user).append("(offline)");
            }
            sb.append(",");
        }

        out.println(sb.toString());
    }



    private void listOwnFiles() {
        File userDir = new File("server_files/" + username);
        StringBuilder sb = new StringBuilder("OWN_FILES:");

        File[] files = userDir.listFiles();

        if (files != null) {
            for (File f : files) {
                String name = f.getName();

                if (name.endsWith(".log") || name.endsWith(".meta")) {
                    continue;
                }

                String visibility = getFileVisibility(f);
                sb.append(name)
                        .append("(")
                        .append(visibility)
                        .append("),");
            }
        }

        out.println(sb.toString());
    }

    private void listPublicFiles(String targetUser) {
        File userDir = new File("server_files/" + targetUser);

        if (!userDir.exists() || !userDir.isDirectory()) {
            out.println("ERROR:User not found");
            return;
        }

        StringBuilder sb = new StringBuilder("PUBLIC_FILES:");
        File[] files = userDir.listFiles();

        if (files != null) {
            for (File f : files) {
                String name = f.getName();

                if (name.endsWith(".log") || name.endsWith(".meta")) {
                    continue;
                }

                if ("public".equals(getFileVisibility(f))) {
                    sb.append(name).append(",");
                }
            }
        }

        out.println(sb.toString());
    }

    void viewMessages() {
        List<String> messages = FileServer.pendingMsg.getOrDefault(username, new ArrayList<>());
        StringBuilder sb = new StringBuilder("MESSAGES:");
        for (String msg : messages) {
            sb.append(msg).append("|");
        }
        FileServer.pendingMsg.put(username, new ArrayList<>()); // clear after reading
        FileServer.savePendingMsg();
        out.println(sb.toString());
    }


    void viewHistory() throws IOException {
        File logFile = new File("server_files/" + username + "/" + username + ".log");
        StringBuilder sb = new StringBuilder("HISTORY:");

        if (logFile.exists()) {
            BufferedReader br = null;
            try {
                br = new BufferedReader(new FileReader(logFile));
                String line;

                while ((line = br.readLine()) != null) {
                    sb.append(line).append("|");
                }
            } catch (IOException e) {
                // Handle exception if needed
            }
        }

        out.println(sb.toString());
    }


    //saving visibility metadata for files
    // 
    void saveFileMetadata(String filename, String visibility) throws IOException {
        File metaFile = new File("server_files/" + username + "/" + filename + ".meta");
        BufferedWriter bw = null;

        try {
            bw = new BufferedWriter(new FileWriter(metaFile));
            bw.write(visibility);
            bw.newLine();
        } catch (IOException e) {
            if (bw != null) {
                bw.close();
            }
        }
    }


//
    String getFileVisibility(File file) {
        File metaFile = new File(file.getParent(), file.getName() + ".meta");

        if (!metaFile.exists()) {
            return "private";
        }

        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader(metaFile));
            String line = br.readLine();
            if (line != null) {
                return line;
            }
        } catch (IOException e) {
            return "private";
        } 

        return "private";
    }

    void logAction(String filename, String action, String status) throws IOException {
        File logFile = new File("server_files/" + username + "/" + username + ".log");
        BufferedWriter bw = null;

        try {
            bw = new BufferedWriter(new FileWriter(logFile, true));
            String logEntry = FileServer.DATE_FORMAT.format(new Date()) + " - " + filename + " - " + action + " - "+ status;
            bw.write(logEntry);
            bw.newLine();
        } 
        catch (IOException e) {
            if (bw != null) {
                bw.close();
            }
        }
    }

    void addMessage(String user, String message) {
        FileServer.pendingMsg.computeIfAbsent(user, k -> new ArrayList<>()).add(message);

        FileServer.savePendingMsg();

        ClientThread thread = FileServer.clientThread.get(user);
        if (thread != null) {
            thread.out.println("NEW_MESSAGE:" + message);
        }
    }

    void cleanup() {
        if (username != null) {
            FileServer.loginUser.remove(username);
            FileServer.clientThread.remove(username);
            System.out.println(username + " disconnected");

            Iterator<Map.Entry<String, FileTransfer>> iterator = FileServer.fileTransfer.entrySet().iterator();
            while (iterator.hasNext()) {
                Map.Entry<String, FileTransfer> entry = iterator.next();
                if (entry.getKey().endsWith("," + username)) {
                    synchronized (FileServer.class) {
                        FileServer.currBuffetSize -= entry.getValue().fileSize;
                    }
                    iterator.remove();
                }
            }
        }
        try {
            socket.close();
        } catch (IOException ignored) {
        }

        FileServer.savePendingMsg();
    }
}
