import java.util.ArrayList;
import java.net.Socket;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

public class ClientHandler implements Runnable{
    
    public static ArrayList<ClientHandler> clientHandlers = new ArrayList<>();
    private Socket socket;
    private BufferedReader bufferedReader;
    private BufferedWriter bufferedWriter;
    private String clientUsername;

    public ClientHandler(Socket socket){
        String username1;
        try{
            this.socket = socket;
            this.bufferedWriter = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
            this.bufferedReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            // leer nombre de cliente y verificar que no este repetido
            while(true){
                username1 = bufferedReader.readLine();
                if(!isUsernameTaken(username1)){
                    break;
                } else {
                    bufferedWriter.write("Username already taken. Please try with other username.");
                    bufferedWriter.newLine();
                    bufferedWriter.flush();
                }
            }

            this.clientUsername = username1;
            clientHandlers.add(this); 
            broadcastMessage("SERVER: " + clientUsername + " has entered the chat!");

        } catch (IOException e) {
            closeEverything(socket, bufferedReader, bufferedWriter);
        }
    }
    
    public boolean isUsernameTaken(String username){
        for (ClientHandler clientHandler : clientHandlers){
            if (clientHandler.clientUsername.equals(username)){
                return true;
            }
        }
        return false;
    }

    @Override
    public void run() {
        String messageFromClient;

        while(socket.isConnected()){
            try {
                messageFromClient = bufferedReader.readLine();
                broadcastMessage(messageFromClient);
            } catch (IOException e) {
                closeEverything(socket, bufferedReader, bufferedWriter);
                break;
            }
        }

    }

    public void broadcastMessage(String messageToSend){
        for (ClientHandler clientHandler : clientHandlers){
            try{
                if(!clientHandler.clientUsername.equals(clientUsername)){
                    clientHandler.bufferedWriter.write(messageToSend);
                    clientHandler.bufferedWriter.newLine();
                    clientHandler.bufferedWriter.flush();
                }
            } catch (IOException e){
                closeEverything(socket, bufferedReader, bufferedWriter);
            }
        }
    }

    public void removeClientHandler(){
        clientHandlers.remove(this);
        broadcastMessage("Server: " + clientUsername + " has left the chat!");
    }

    public void closeEverything(Socket socket, BufferedReader bufferedReader, BufferedWriter bufferedWriter){
        removeClientHandler();
        try{
            if(bufferedReader != null){
                bufferedReader.close();
            }
            if (bufferedWriter != null){
                bufferedWriter.close();
            }
            if (socket != null){
                socket.close();
            }
        } catch (IOException e){
            e.printStackTrace();
        }
    }
}