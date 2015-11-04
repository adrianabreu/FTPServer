#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdio>
#include <cstdint>

const int MAX_BUFF=1000;

class ClientConnection {
public:
    ClientConnection(int s);
    ~ClientConnection();
    
    void WaitForRequests();
    void stop();

    
private:  
   bool ok; // 	Esta variable es un flag que evita que el servidor
	    // escuche peticiones si ha habido errores en la inicialización.
   
   bool logged; //Esta variable impide que usuarios no logueados utilicen los comandos
   
   bool pasv_; //Esta variable nos ayuda a abrir los sockets en las funciones 
    
   FILE *fd;	 // Descriptor de fichero en C. Se utiliza para que
		 // socket de la conexión de control tenga esté bufferado
		 // y se pueda manipular como un fichero a la C, con
		 // fprintf, fscanf, etc.
   FILE *fdata;
  
    char command[MAX_BUFF];  // Buffer para almacenar el comando.
    char arg[MAX_BUFF];      // Buffer para almacenar los argumentos. 
    
    int data_socket;         // Descriptor de socket para la conexion de datos;
    int control_socket;      // Descriptor de socket para al conexión de control;
    
    bool parar;
    char raiz[100];
    size_t long_raiz;
};

#endif
