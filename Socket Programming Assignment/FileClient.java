import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;

public class FileClient {
    private static final String SERVER_HOST = "localhost";
    private static final int SERVER_PORT = 6666;

    private Socket socket;
    private PrintWriter out;
    private BufferedReader in;
    private String username;
    private BlockingQueue<String> responseQueue = new LinkedBlockingQueue<>();

    public static void main(String[] args) {
        new FileClient().start();
    }

    public void start() {
        try {
            socket = new Socket(SERVER_HOST, SERVER_PORT);
            out = new PrintWriter(socket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            new Thread(new ResponseHandler()).start();

            String prompt = getResponse();
            if ("ENTER_USERNAME".equals(prompt)) {
                Scanner scanner = new Scanner(System.in);
                System.out.print("Enter username: ");
                username = scanner.nextLine().trim();
                out.println(username);

                String resp = getResponse();
                if (resp.startsWith("ERROR")) {
                    System.out.println(resp.substring(6));
                    return;
                }
                System.out.println(resp.substring(8));
                showMenu(scanner);
            }
        } catch (Exception e) {
            System.out.println("Connection failed: " + e.getMessage());
        }
    }

    private String getResponse() throws InterruptedException {
        String resp = responseQueue.take();
        return resp;
    }

    private void showMenu(Scanner scanner) {
        while (true) {
            System.out.println("\n===== FILE TRANSFER SYSTEM =====");
            System.out.println("  MENU OPTIONS:");
            System.out.println("1. List all connected clients");
            System.out.println("2. View own uploaded files");
            System.out.println("3. List public files of a user");
            System.out.println("4. Upload a file to server");
            System.out.println("5. Download file from server");
            System.out.println("6. Send file request to user");
            System.out.println("7. Check unread messages");
            System.out.println("8. View transfer history");
            System.out.println("9. Exit System");
            System.out.print("Choose option: ");

            int choice;
            try {
                choice = Integer.parseInt(scanner.nextLine());
            } catch (Exception e) {
                System.out.println("Please enter a number!");
                continue;
            }

            try {
                switch (choice) {
                    case 1 -> listClients();
                    case 2 -> listOwnFiles();
                    case 3 -> listPublicFiles(scanner);
                    case 4 -> uploadFile(scanner);
                    case 5 -> downloadFile(scanner);
                    case 6 -> sendFileRequest(scanner);
                    case 7 -> viewMessages();
                    case 8 -> viewHistory();
                    case 9 -> {
                        System.out.println("Goodbye!");
                        socket.close();
                        return;
                    }
                    default -> System.out.println("Invalid option");
                }
            } catch (Exception e) {
                System.out.println("Error: " + e.getMessage());
            }
        }
    }

    private void listClients() throws InterruptedException {
        out.println("LIST_CLIENTS:");
        String resp = getResponse();

        if (resp.startsWith("CLIENTS:")) {
            String[] clients = resp.substring(8).split(",");
            System.out.println("\n==== Client List ====");
            // for (String client : clients) {
            // if (!client.isEmpty()) {
            // System.out.println("- " + client);
            // }
            
            System.out.println("  STATUS    │ USERNAME");
           
            int onlineCount = 0;
            int offlineCount = 0;
            for (String client : clients) {
                if (!client.isEmpty()) {
                    boolean isOnline = client.contains("(online)");
                    String username = client.replace("(online)", "").replace("(offline)", "");
                    String status = isOnline ? " ONLINE" : " OFFLINE";

                    System.out.printf(" %-10s │ %s%n", status, username);
                    if (isOnline)
                        onlineCount++;
                    else
                        offlineCount++;
                }
            }
            // System.out.println("");
            // System.out.println(" Total Users: " + (onlineCount + offlineCount));
            // System.out.println(" Online: " + onlineCount + "  │  Offline: " + offlineCount);
    
        }
    }
    

    private void listOwnFiles() throws InterruptedException {
        out.println("LIST_OWN_FILES:");
        String resp = getResponse();

        if (resp.startsWith("OWN_FILES:")) {
            String[] files = resp.substring(10).split(",");
            System.out.println("\n==== My Uploaded Files ====");

            System.out.println("  VISIBILITY │ FILENAME");
            int publicCount = 0;
            int privateCount = 0;
            for (String file : files) {
                if (!file.isEmpty()) {
                    
                    int lastParen = file.lastIndexOf('(');
                    String filename = file.substring(0, lastParen);
                    String visibility = file.substring(lastParen + 1, file.length() - 1);

                    String vis = "public".equals(visibility) ? "PUBLIC" : "PRIVATE";
                    System.out.printf(" %-10s │ %s%n", vis, filename);

                    if ("public".equals(visibility))
                        publicCount++;
                    else
                        privateCount++;
                }
            }

            if (files.length == 1 && files[0].isEmpty()) {
                System.out.println("             │ No files uploaded yet");
            }

                // System.out.println("");
                // System.out.println(" Total Files: " + (publicCount + privateCount));
                // System.out.println(" Public: " + publicCount + "  │  Private:  " + privateCount);
        }
    }

    private void listPublicFiles(Scanner scanner) throws InterruptedException {
        System.out.print("Enter username: ");
        String targetUser = scanner.nextLine();
        out.println("LIST_PUBLIC_FILES:" + targetUser);

        String resp = getResponse();
        if (resp.startsWith("ERROR")) {
            System.out.println(resp.substring(6));
            return;
        }

        // if (resp.startsWith("PUBLIC_FILES:")) {
        //     String[] files = resp.substring(13).split(",");
        //     System.out.println("\n=== Public Files of " + targetUser + " ===");
        //     for (String file : files) {
        //         if (!file.isEmpty()) {
        //             System.out.println("- " + file);
        //         }
        //     }
        // }
         if (resp.startsWith("PUBLIC_FILES:")) {
            String[] files = resp.substring(13).split(",");
     
            System.out.println("\n==== PUBLIC FILES FROM: " + targetUser.toUpperCase() + " ====");


            if (files.length == 1 && files[0].isEmpty()) {
                System.out.println("  No public files available from this user");
            } else {
                System.out.println("  AVAILABLE FILES:");
                System.out.println("");
                for (String file : files) {
                    if (!file.isEmpty()) {
                        System.out.println(" - " + file);
                    }
                }
            }
            // System.out.println("");
            // System.out.println(" Total Public Files: " + (files[0].isEmpty() ? 0 : files.length));
   
        }
    }

    private void startFileTransfer(Runnable transferTask) {
        new Thread(transferTask).start();
    }


        private void uploadFile(Scanner scanner) throws IOException, InterruptedException {
       

        System.out.print(" Enter file path to upload: ");
        String path = scanner.nextLine();
        File file = new File(path);
        if (!file.exists()) {
            System.out.println("ERROR: File not found! Please check the path and try again.");
            return;
        }

        System.out.println("SUCCESS: File found: " + file.getName() + " (" + file.length() + " bytes)");
        System.out.print("Save as (press Enter to keep original name): ");
        String saveFileName = scanner.nextLine();
        final String saveName = saveFileName.isEmpty() ? file.getName() : saveFileName;

        System.out.print("Visibility (public/private) [default: private]: ");
        String visibility = scanner.nextLine().toLowerCase();
        if (visibility.isEmpty())
            visibility = "private";

        if (!visibility.equals("public") && !visibility.equals("private")) {
            System.out.println("WARNING: Invalid visibility. Using 'private' as default.");
            visibility = "private";
        }

        System.out.print("Fulfilling a request? Enter request ID (or press Enter for none): ");
        String reqIdStr = scanner.nextLine();
        long reqId = -1;
        try {
            reqId = Long.parseLong(reqIdStr);
        } catch (Exception ignored) {
        }

        String reqData = saveName + "," + file.length() + "," + visibility;
        if (reqId != -1)
            reqData += "," + reqId;

        out.println("UPLOAD_REQUEST:" + reqData);

        String resp = getResponse();
        if (resp.startsWith("ERROR")) {
            System.out.println("Server rejected: " + resp.substring(6));
            return;
        }
        if (!resp.startsWith("UPLOAD_READY:")) {
            System.out.println("Unexpected response: " + resp);
            return;
        }

        String[] parts = resp.substring(13).split(",");
        String fileId = parts[0];
        int chunkSize = Integer.parseInt(parts[1]);

        startFileTransfer(new UploadFile(out, () -> getResponse(), username, file, fileId, chunkSize, saveName));

        System.out.println("Upload started in background! You can continue using the menu.");
    }

    private void downloadFile(Scanner scanner) throws IOException, InterruptedException {
        System.out.print("Enter file owner username: ");
        String owner = scanner.nextLine();

        System.out.print("Enter filename: ");
        String filename = scanner.nextLine();

        startFileTransfer(new DownloadFile(out, () -> getResponse(), owner, filename));

         System.out.println("Download started in background! You can continue using the menu.");
    }

    private void sendFileRequest(Scanner scanner) throws InterruptedException {
        System.out.print("Enter recipient username (or 'ALL' for broadcast): ");
        String client = scanner.nextLine();

        System.out.print("Enter file description/request details: ");
        String description = scanner.nextLine();

        try {
            ReqSend.send(out, () -> getResponse(), client, description);
        } catch (Exception e) {
            System.out.println("Error: " + e.getMessage());
        }
    }

    private void viewMessages() throws InterruptedException {
        out.println("VIEW_MESSAGES:");
        String resp = getResponse();

        if (resp.startsWith("MESSAGES:")) {
            String messagesStr = resp.substring(9);
            if (messagesStr.isEmpty()) {
                System.out.println("   No unread messages");
            } else {
                String[] messages = messagesStr.split("\\|");
                System.out.println("\n=== Unread Messages ===");
                for (String msg : messages) {
                    if (!msg.isEmpty())
                        System.out.println("- " + msg);
                }
            }
        }
    }

    private void viewHistory() throws InterruptedException {
        out.println("VIEW_HISTORY:");
        String resp = getResponse();

        if (resp.startsWith("HISTORY:")) {
            String historyStr = resp.substring(8);
                           System.out.println("\n==== TRANSFER HISTORY ==== " );
            if (historyStr.isEmpty()) {
                System.out.println("No history available");
            } else {
                String[] entries = historyStr.split("\\|");
                System.out.println("\n=== Upload/Download History ===");
                for (String entry : entries) {
                    if (!entry.isEmpty())
                        System.out.println(entry);
                }
            }
        }
    }
   
    class ResponseHandler implements Runnable {
        @Override
        public void run() {
            try {
                String line;
                while ((line = in.readLine()) != null) {

                    if (line.startsWith("NEW_MESSAGE:")) {

                        continue;
                    }

                    responseQueue.put(line);
                }
            } catch (Exception e) {
                System.out.println("\nServer connection lost.");
            }
        }
    }
}