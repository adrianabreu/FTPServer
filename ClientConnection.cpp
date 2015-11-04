//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//  Clase que atiende una petición FTP.
// 
//****************************************************************************

//PLS http://simplestcodings.blogspot.com.es/2012/09/ftp-implementation-in-c.html

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <netdb.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
 #include <fcntl.h>

#include <sys/stat.h> 
#include <iostream>
#include <fstream> //For use read
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"

#include <sys/sendfile.h>

ClientConnection::ClientConnection(int s) {
   int sock = (int)(s);
   char buffer[MAX_BUFF];
   control_socket = s;
   fd = fdopen(s, "a+"); //a+ escribir por el final 
   if (fd == NULL){
	    //std::cout << "Connection closed :(" << std::endl; //Chivato
	    fclose(fd);
      close(control_socket);
      ok = false;
      return;
   }
   pasv_=false;
   ok = true;
   logged=false;

   data_socket = -1;

   //Determinación de la carpeta raíz para la implementación del cd
   getcwd(raiz,sizeof(raiz));
   while(raiz[long_raiz]!='\0')
	   long_raiz++;
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
}


int connect_TCP(uint32_t address,  uint16_t  port) {
   struct sockaddr_in sin;
   struct hostent *hent;
   int s;
   char str[10];
   
   //std::cout << "Pasar address a str" << std::endl;
   sprintf( str, "%u", address);

   //std::cout << "memset" << std::endl;
   memset(&sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   sin.sin_port = htons(port);
   
   //std::cout << "gethostbyname" << std::endl;
   sin.sin_addr.s_addr = address;
   
   //std::cout << "Constructor del socket" << std::endl;
   s = socket(AF_INET, SOCK_STREAM, 0);

   if(s < 0){
          errexit("No se puede crear el socket: %s :(\n", strerror(errno));
   }
	//std::cout << "Socket creado correctamente" << std::endl;
   if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
      errexit("No se puede conectar con %s: %s\n :(", address, strerror(errno));
	}

	//std::cout << "FINAL" << std::endl;
   return s;
}

void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}
    
#define COMMAND(cmd) strcmp(command, cmd)==0

// Esta función es la que atiende las peticiones
// Aquí es donde se implementan las acciones asociadas a los comandos.
// Véase el ejemplo del comando USER.
// Si considera que debe añadir algún otro comando siéntase libre
// de hacerlo. Asimismo, puede añadir tantos métodos auxiliares como
// sea necesario.

void ClientConnection::WaitForRequests() { //LAS ORDENES DEL CLIENTE SE TRANSFORMAN A LA NORMA
   if (!ok) {
      return;
   }
   //Imprimimos el codigo de control
   fprintf(fd, "220 Service ready\n");
   while(!parar) {  
      
      fscanf(fd, "%s", command);
      
      if (COMMAND("USER")) {
         fscanf(fd, "%s", arg);
         std::cout << "Arg: " << arg << std::endl;
         
         bool encontrado=false;
         char buffer[MAX_BUFF]; //bufer temporal
         FILE* users; //Fichero
         users=fopen("conf.dat","r"); //archivo que hace de bbdd
         if(users==NULL){
           fprintf(fd,"530 Error, verifique la coherencia del programa\n");
           std::cout<<"Maybe conf.dat is missing?" << std::endl;
         }
         else{
            while(!feof(users) && !encontrado){
              fscanf(users,"%s\n",buffer);
              char *nombre;
              //std::cout << "Buffer:" << buffer << std::endl;
              nombre=strtok(buffer,":");
              if(strcmp(arg,nombre)==0)
                 encontrado=true;
              
            }
            fclose(users);
            if(encontrado)
               fprintf(fd, "331 User name ok, need password\n");
               //std::cout << "USER: " << arg << std::endl;  //Chivato
	          else
               fprintf(fd, "332 User not exist on the bbdd\n");
            //fallo 550
	          //fprintf(fd, "530 Not logged in\n");
        }
      }
      else if (COMMAND("PWD")) {
         char curdir[100]; //Reservamos un char
         getcwd(curdir,sizeof(curdir)); //La funcion getcwd esta en unistd.h
         //std::cout << "PWD:" << curdir << std::endl;  //Chivato
         fprintf(fd, "257 %s\n",curdir); //Imprimimos 257 que es el codigo correspondiente
      }
      else if (COMMAND("PASS")) {
         fscanf(fd, "%s", arg);
         //std::cout << "PASS: " << arg << std::endl;  //Chivato
         //fallo 530
         char curdir[100];
         bool encontrado=false;
         char buffer[MAX_BUFF]; //bufer temporal
         FILE* users; //Fichero
         users=fopen("conf.dat","r"); //archivo que hace de bbdd
         if(users==NULL){
           fprintf(fd,"530 Error, verifique la coherencia del programa\n");
           std::cout<<"Maybe conf.dat is missing?" << std::endl;
         }
         else{
            while(!feof(users) && !encontrado){
              fscanf(users,"%s\n",buffer);
              char *nombre;

              nombre=strchr(buffer,':'); 
              nombre++;
              std::cout << "Buffer:" << nombre << std::endl;
              if(strcmp(arg,nombre)==0)
                 encontrado=true;
              
            }
            fclose(users);
            if(encontrado){
               fprintf(fd, "230 User logged in\n");
               logged=true;
            }
               //std::cout << "USER: " << arg << std::endl;  //Chivato
            else
               fprintf(fd, "530 User not loggend in\n");
         }
      }
      else if (COMMAND("PORT")) { //RECIBE IP Y PUERTO
		     fscanf(fd, "%s", arg);
		     //std::cout << "Reservas de memoria" << std::endl;
		     std::cout << "ARG: " << arg << std::endl;
		     char *cp = &arg[0];
		     int i;  
         unsigned char buf[6];
         //std::cout << "Bucle" << std::endl;      
         for (cp, i = 0; i < 6; i++) {
            //std::cout << "Iteracion " << i << ": ";  
			      buf[i] = atoi(cp);
            //std::cout << "Buf[" << i << "]: " << (int)buf[i] << std::endl;   
            cp = strchr(cp, ',');     
            cp++;   
         }  
		     //std::cout << "Transformacion" << std::endl;
		     int port;
         port = buf[4]<<8|buf[5];

         unsigned int ip = *(unsigned int*)&buf[0]; 
         std::cout << "IP: " << ip << std::endl;
         //unsigned short port = *(unsigned short*)&buf[4];
         std::cout << "PORT: " << port << std::endl;
         //std::cout << "Llamar connect_TCP" << std::endl;
         ip = *((int*)(buf));
         data_socket=connect_TCP(ip, port);
         fdata = fdopen(data_socket, "a+"); //a+ escribir por el final
         if (fdata == NULL){
            std::cout << "Connection closed :(" << std::endl;
            fclose(fdata);
            close(data_socket);
            fprintf(fd, "425 Connection failed\n");
			      ok = false;
		     }
		     fprintf(fd, "200 PORT OK\n");
      }
      else if (COMMAND("PASV")) {

         struct sockaddr_in pasvaddr;   
         socklen_t len;   
         unsigned int ip;   
         unsigned short port;   
         int s;
    
         len = sizeof(pasvaddr);   
         s = socket(AF_INET, SOCK_STREAM, 0);
         if(s < 0){
            errexit("No se puede crear el socket: %s :(\n", strerror(errno));
		     }   
         getsockname(control_socket, (struct sockaddr*)&pasvaddr, &len);   
         pasvaddr.sin_port = 0;   
         if (bind(s, (struct sockaddr*)&pasvaddr, sizeof(pasvaddr)) < 0) {    
            close(s);   
            s = -1;     
         }      
         if (listen(s, 5) < 0)
            errexit("Fallo en el listen: %s:(\n", strerror(errno));
            //len = sizeof(pasvaddr); //ya estaba hecho   
         getsockname(s, (struct sockaddr*)&pasvaddr, &len);   
         ip = ntohl(pasvaddr.sin_addr.s_addr);   
         port = ntohs(pasvaddr.sin_port);   
         printf("local bind: %s:%d\n", inet_ntoa(pasvaddr.sin_addr), port);
         data_socket=s; //Guardamos el socket en el atributo, sin utilizarlo
         pasv_=true; //Habilitamos el bool de la conexion si ponemos aqui el accept, peta
         fprintf(fd, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\n", (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, (port>>8)&0xff, port&0xff);
      }
      else if (COMMAND("CWD")) {
         fscanf(fd, "%s", arg);
		     //std::cout << "CWD A: " << arg << std::endl; //Chivatos
		     char curdir[100];
		     getcwd(curdir,sizeof(curdir));
		     if((strcmp (raiz, curdir))<0){
			      //std::cout << "Por debajo de raiz" << std::endl;
			      if(chdir(arg)==0){
			         //std::cout << "CWD OK" << std::endl; //Chivato  
			         fprintf(fd, "200 Cambio realizado\n");
				    }
				    else{
			         //std::cout << "CWD FAIL" << std::endl;  
			         fprintf(fd, "431 Fallo\n");
				    }
		     }
		     else if((strcmp (raiz, curdir))==0){
			      //std::cout << "En la raiz" << std::endl;
			      if(strcmp(arg,"..")!=0)
				       if(chdir(arg)==0){
					        //std::cout << "CWD OK" << std::endl;//Hola  
					        fprintf(fd, "200 Cambio realizado\n");
               }
               else{
                 //std::cout << "CWD FAIL" << std::endl;  
                 fprintf(fd, "431 Fallo\n");
               }
            else
					     fprintf(fd, "431 No puedes salir de la raiz del server\n");
         }
         else{
           //std::cout << "Encima de la raiz" << std::endl;
           //std::cout << "CWD FAIL" << std::endl;  
           fprintf(fd, "431 Fallo\n");
         }
      }
      else if (COMMAND("STOR") ) {
         fscanf(fd, "%s", arg);
         if(logged){ //bool de comprobacion de loggin
            
            //std::cout << "ARG: " << arg << std::endl;
            //Modo pasv
            if(pasv_){ //Si el pasv esta activo
               //PROBANDO ACCEPT
               struct sockaddr_in fsin;
               socklen_t alen; //FALLO FALLO
               alen=sizeof(fsin); //FIXED FALLO!!
               data_socket = accept(data_socket, (struct sockaddr *)&fsin, &alen); //Utilizamos el accept
               fdata = fdopen(data_socket, "a+"); //Y abrimos el fdata
            }

            char buf[MAX_BUFF];//Buffer de datos
            int fdd;//Descriptor del fichero
            int n;//donde leeremos el fichero
            bool correct=true; //booleano para comprobar la transmisión del archivo 
             //std::cout << "Abrimos el descriptor de fichero" << std::endl;

            fdd=open(arg, O_RDWR|O_CREAT,S_IRWXU);//Argumento,modo de apertura,permisos rwx para el user
            fprintf(fd, "150 File ok, creating connection\n");//Mensaje de control
            fflush(fd);//Vaciado del buffer
            if(fdd<0){
               std::cout << "Error" << std::endl;
               fprintf(fd, "550 open error\n");
            }
            //std::cout << "Empezamos a leer" << std::endl;
            do{
               //std::cout << recv(data_socket,buf,sizeof(buf),0) << std::endl;
               n = read(data_socket,buf,sizeof(buf));
               if(n<0)
                  if(errno==EINTR)//Comprobacion del tipo de error
                     continue; //Si es un eintr reintetamos la iteracion 
                  else //Si hay otro tipo de error 
                     correct=false; //Nos cargamos el buffer
               if(write(fdd,buf,n)<0){
                  errexit("error en el fichero %s\n",errno);
                  correct=false;
                  break;
		           }
            }while(n>0);
            //std::cout << "Vamos a cerrar los ficheros y descriptores" << std::endl;
            fclose(fdata);//Ceramos el buffer de datos
            close(fdd);//Cerramos el descritor de fichero
            close(data_socket);//Cerramos el socket de datos
            //std::cout << "Fin" << std::endl;
            if(correct)
               fprintf(fd, "226 File received correctly\n");//Mandamos el mensaje de control
            else
               fprintf(fd, "550 file error\n");
         }
         else{
            fprintf(fd,"532 Necesita cuenta para almacenar archivos\n");
         }
      }
      else if (COMMAND("SYST")) {
         fprintf(fd, "215 UNIX Type: L8.\n");
      }
      else if (COMMAND("TYPE")) {
         fscanf(fd, "%s", arg);
         fprintf(fd, "200 TYPE OK\n");
      }
      else if (COMMAND("RETR")) {

         fscanf(fd, "%s", arg);
         //Modo pasv
         if(logged){
            if(pasv_){ //Si el pasv esta activo
               //PROBANDO ACCEPT
               struct sockaddr_in fsin;
               socklen_t alen; //FALLO FALLO
               alen=sizeof(fsin); //FIXED FALLO!!
               data_socket = accept(data_socket, (struct sockaddr *)&fsin, &alen); //Utilizamos el accept
               fdata = fdopen(data_socket, "a+"); //Y abrimos el fdata
            }
            bool correcto=true;
            char buf[MAX_BUFF];
            int fdd;//Descriptor del fichero
            int n;//donde leeremos el fichero
            fprintf(fd, "150 File status ok\n");
            //std::cout << "Abrimos el descriptor de fichero" << std::endl;
            fdd=open(arg, O_RDONLY);
            if(fdd<0){
               std::cout << "Error" << std::endl;
               fprintf(fd, "451 Error al leer fichero\n");
	          }
            //std::cout << "Empezamos a leer" << std::endl;
            else{
               do{
                  n = read(fdd, buf, sizeof(buf));
                  if(n<0)
                     if(errno==EINTR)
                        continue;
                     else
                        correcto=false;
                     //std::cout << buf << std::endl;
                  if(send(data_socket,buf,n,0)<0){
                     errexit("error en el envio %s\n",errno);
                     correcto=false;
                     break;
		              }
                }while(n>0);
            }
            //std::cout << send(data_socket, "Hello, world!\n", 13, 0) << std::endl;
            close(fdd);
            fclose(fdata);
            close(data_socket);
            std::cout << "Fin" << std::endl;
            if(correcto)
               fprintf(fd, "226 File received correctly\n");
            else
               fprintf(fd, "426 Fallo en el envio\n");
         }
         else{
            fprintf(fd,"532 Need account for get\n");
         }
      }
      else if (COMMAND("QUIT")) {
         parar=true;
         if(chdir(raiz)==0)
         std::cout << "Vuelta a la raiz" << std::endl;
		     fprintf(fd, "Client Goodbye!\n");
      }
      else if (COMMAND("LIST")) {
		     fprintf(fd, "125 List started OK\n");
         if(pasv_){ //Si el pasv esta activo
            //PROBANDO ACCEPT
            struct sockaddr_in fsin;
            socklen_t alen; //FALLO FALLO
            alen=sizeof(fsin); //FIXED FALLO!!
            data_socket = accept(data_socket, (struct sockaddr *)&fsin, &alen); //Utilizamos el accept
            fdata = fdopen(data_socket, "a+"); //Y abrimos el fdata
         }
         DIR *fd_dir;
         struct dirent *s_dirent;
         struct stat buff;
         char curdir[100]; //Reservamos un char	
         
         getcwd(curdir,sizeof(curdir));
         std::cout << "LIST DE: " << curdir << std::endl;
         fd_dir = opendir(curdir);
         //std::cout << "Abrimos el directorio" << std::endl;			
			   if(fd_dir == 0){
				    printf("Error opening directory\n");
			   }
         //std::cout << "Leemos directorio" << std::endl;
			   while((s_dirent = readdir(fd_dir)) != NULL){
				    //std::cout << s_dirent << std::endl;
            if((strcmp(s_dirent->d_name,"."))&&(strcmp(s_dirent->d_name,".."))) //Evitamos . y ..
               fprintf( fdata, "%s\r\n",s_dirent->d_name);
			   } 
         //std::cout << "Cerramos el directorio" << std::endl;
			   closedir(fd_dir);
         //std::cout << "Cerramos el fichero" << std::endl;
		     fclose(fdata);
         //std::cout << "Cerramos el socket" << std::endl;
		     close(data_socket);
		     fprintf(fd, "226 List completed successfully\n");
      }
      else{
	       fprintf(fd, "502 Command not implemented.\n"); 
         fflush(fd);
	       printf("Comando : %s %s\n", command, arg);
	       printf("Error interno del servidor\n");
      }
      
   } 
   fclose(fd);
   //std::cout << "Conexion cerrada" << std::endl;
   return;
  
};
