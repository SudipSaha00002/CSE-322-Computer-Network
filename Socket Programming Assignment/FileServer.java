import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class FileServer {

    static final int PORT = 6666;
    static final long MAX_BUFFER_SIZE = 100L * 1024 * 1024 * 1024;
    static final int MIN_CHUNK_SIZE = 50 * 1024 * 1024;
    static final int MAX_CHUNK_SIZE = 100 * 1024 * 1024;
    static final java.util.Random random = new java.util.Random();
    static final SimpleDateFormat DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    static final java.util.Set<String> loginUser = ConcurrentHashMap.newKeySet();
    static final Map<String, ClientThread> clientThread = new ConcurrentHashMap<>();
    static final Map<String, List<String>> pendingMsg = new ConcurrentHashMap<>();
    static final Map<String, FileTransfer> fileTransfer = new ConcurrentHashMap<>();//file transfer when requested
    static final Map<Long, String> reqIdTransfer = new ConcurrentHashMap<>();//in order to track the request id with the tranfer file
    static long currBuffetSize = 0;
    static long ReqId = 1;
    static final File USERS_FILE = new File("server_users.txt");
    static final java.util.Set<String> logedInUser = ConcurrentHashMap.newKeySet();
    static final File MESSAGES_FILE = new File("server_messages.txt");

    public static void main(String[] args) {
        System.out.println("File Server starting on port " + PORT + "...");

        loadlogedInUsers();
        loadPendingMsg();

        Runtime.getRuntime().addShutdownHook(new Thread(() -> { // Clrt+C
            System.out.println("Server shutting down ...");
            savePendingMsg();
        }));

        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            while (true) {
                Socket clientSocket = serverSocket.accept();
                new Thread(new ClientThread(clientSocket)).start();
                // handling each client in a new thread
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    //loading all the login users from the server
    // to keep track of known users
    // so that messages can be delivered even if they are offline
    static void loadlogedInUsers() {
        if (USERS_FILE.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(USERS_FILE))) {
                String line;
                while ((line = br.readLine()) != null) {
                    String user = line.trim();
                    if (!user.isEmpty()) {
                        logedInUser.add(user); 
                    }
                }
            } catch (IOException e) {
                System.out.println("Warning: Could not load known users: " + e.getMessage());
            }
        }
    }

    // saving all the loged in users to the server
    static void saveLogedInUsers(String username) {
        if (logedInUser.add(username)) { // true only if it was new
            try (PrintWriter pw = new PrintWriter(new FileWriter(USERS_FILE, true))) {
                pw.println(username);//adding new user to the server
            } catch (IOException e) {
                System.out.println("Warning: Could not save user: " + e.getMessage());
            }
        }
    }

    //loading all the pending messages from the server
    // to deliver them when the user logs in
    // 
    static void loadPendingMsg() {
        if (MESSAGES_FILE.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(MESSAGES_FILE))) {
                String line;
                while ((line = br.readLine()) != null) {
                    if (line.contains(":")) {
                        String[] parts = line.split(":", 2);
                        String user = parts[0];
                        String msg = parts[1];
                        pendingMsg.computeIfAbsent(user, k -> new ArrayList<>()).add(msg);
                    }
                }
            } catch (IOException e) {
                System.out.println("Could not load messages.");
            }
        }
    }


   static synchronized void savePendingMsg() {
    BufferedWriter bw = null;

    try {
        bw = new BufferedWriter(new FileWriter(MESSAGES_FILE));

        for (String user : pendingMsg.keySet()) {
            List<String> msgs = pendingMsg.get(user);

            for (String msg : msgs) {
                bw.write(user + ":" + msg);
                bw.newLine();
            }
        }

    } catch (IOException e) {
        System.out.println("Could not save messages.");
    } 

}

}