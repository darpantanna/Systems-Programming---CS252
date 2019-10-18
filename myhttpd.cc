
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "signal.h"
//#include <pthread>
int QueueLength = 5;

const char * passkey = "GG";
void processCGI(int socket, char * path);


pthread_mutexattr_t t;
pthread_mutex_t t1;

// Processes time request
void processRequest( int socket );
void processDirectory(int socket, char * path, int slash);
void poolSlave( int socket );
void processThread( int socket );
void sortBySize(char * path, struct dirent ** ents, int length, int reverse);
void sortByName(struct dirent ** ents, int length, int reverse);
void sortByModified(char * path, struct dirent ** ents, int length, int reverse);

extern "C" void zombie(int sig);

void poolSlave( int socket ) {
	while ( 1 ) {
		pthread_mutex_lock(&t1);
		struct sockaddr_in cIP;
		int al = sizeof(cIP);
		int slaveSocket = accept(socket, (struct sockaddr *)&cIP, (socklen_t *)&al);
		pthread_mutex_unlock(&t1);
		if( slaveSocket < 0) {
			if(errno == EINTR) {
				continue;
			}	
			perror("accept");
			exit(-1);
		}
		processRequest(slaveSocket);
		close(socket);
	}
}

void processThread( int socket ) {
	processRequest( socket );
	close( socket );
}
int
main( int argc, char ** argv )
{
  // Print usage if not enough arguments
	int port;
	int conc = 0;
	if(argc == 2) {
		port = atoi(argv[1]);
		conc = 0;
				
		
	}
	else if(argc == 3) {
		port=atoi(argv[2]);
		if(argv[1][0] == '-') {
			if(argv[1][1] == 't') {
				conc = 1;
				fprintf(stderr,"Thread Mode\n");
			}
			else if (argv[1][1] == 'f') {
				conc = 2;
				fprintf(stderr,"Process Mode\n");

			}
			else if (argv[1][1] == 'p') {
				conc = 3;
				fprintf(stderr, "Pool Mode\n");

			}else {
				conc = 0;
				fprintf(stderr, "Normal Mode\n");

			}
		}
	}
	else if ( argc > 3 ) {
		fprintf( stderr, "%s", usage );
   		exit( -1 );
	}
	else {
		conc = 0;
		if( argc > 2) {
			port = atoi(argv[1]);
		}
		else {
			port = 3100;	
		}
		fprintf(stderr, "Normal Mode\n");

	}	
  // Get the port from the arguments
  //port = atoi( argv[1] );
  struct sigaction sa; //ZOMBIE
  sa.sa_handler = zombie;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  int er = sigaction(SIGCHLD, &sa, NULL);
  if(er != 0) {
  	perror("zombie");
	exit(-1);
  } 
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

  if(conc == 1) {
		while(1) {
			fprintf(stderr, "Concentration 1");
			struct sockaddr_in clientIPAddress;
    		int alen = sizeof( clientIPAddress ); 
			int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);

    		if ( slaveSocket < 0 ) {
    		  perror( "accept" );
      			exit( -1 );
    		}
		pthread_t id;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setscope(&a, PTHREAD_SCOPE_SYSTEM);
	
		pthread_create(&id, &a, (void * (*) (void *))processThread ,(void *)slaveSocket);

		}
  }

  if(conc != 3) {
  while ( 1 ) {

    // Accept incoming connections
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress ); 
	int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);

    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }
	if(conc == 1) {
		fprintf(stderr, "PRocess thread\n");
		pthread_t id;
		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setscope(&a, PTHREAD_SCOPE_SYSTEM);
	
		pthread_create(&id, &a, (void * (*) (void *))processThread ,(void *)slaveSocket);
	}
	else if(conc == 2) {
		
		pid_t slave = fork();
		if(slave == 0) {
			fprintf(stderr, "\r\nConnected a client\r\n");
    		// Process request.
    		processRequest( slaveSocket );
	
	    	// Close socket
    		close( slaveSocket );
  			exit(EXIT_SUCCESS);
		}
		close(slaveSocket);
	}else {
		processRequest( slaveSocket);
		close(slaveSocket);	
	}
 	}
  }
  else {
  	pthread_mutexattr_init(&t);
	pthread_mutex_init(&t1, &t);
 	pthread_attr_t a;
	pthread_attr_init(&a);
	pthread_t id[5];
	pthread_attr_setscope(&a, PTHREAD_SCOPE_SYSTEM);

	int i = 0;
	while(i < QueueLength) {
		pthread_create(&id[i], &a, (void * (*) (void *)) poolSlave,(void *) masterSocket);
	}	
	pthread_join(id[0], NULL);
  } 
}

void
processRequest( int fd )
{
	fprintf(stderr, "\n\n-----\nProcessing Request\n");
  // Buffer used to store the name received from the client
  const int MaxLength = 1024;
  char curr_string[ MaxLength + 1 ];
  int length = 0;
  int n;


  // Currently character read
  unsigned char newChar;

  // Last character read
  unsigned char oldChar = 0;

  //
  // The client should send <name><cr><lf>
  // Read the name of the client character by character until a
  // <CR><LF> is found.
  //
 
	char * docpath = (char *)malloc(MaxLength+1);

  int seenGet=0;
  int seenDocpath = 0;

  //FIX THIS
  while (n = read( fd, &newChar, sizeof(newChar) )  > 0 ) {
		length++;
		if(newChar == ' ') {
			if(seenGet == 0) {
				seenGet=1;
				length--;
			}
			else if(seenDocpath == 0) {
				seenDocpath =1;
				curr_string[length-1] = 0;
				strcpy(docpath,curr_string);
				length=0;
			}
		}
		else {
			oldChar = newChar;
			if(seenGet) {
				curr_string[length-1] = newChar;
			}else {
				length--;
			}
		}	
		if(newChar == '\n' && curr_string[length-2] == '\r'  && curr_string[length-3] == '\n' && curr_string[length-4] == '\r') {
			break;
		}

  }

	curr_string[ length-1] = 0;


	fprintf(stderr, "The docpath is: %s\n", docpath);	
    int slash = 0;
  if(docpath[strlen(docpath)-1] == '/') {
		slash = 1;
  }	  
    //Verify Key
	char * key = strtok(strdup(docpath), "/");
	fprintf(stderr, "Key: %s\n", key);
	if(key == NULL) {
		fprintf(stderr, "INVALID KEY!\n");
		return;
	} else if(!strcmp(key, passkey)) {
		//Correct key continue
	}else {
		fprintf(stderr, "INVALID KEY!\n");
		return;
	}
	
	char cwd[256] = {0};
	getcwd(cwd, sizeof(cwd));
	fprintf(stderr, "CWD: %s\n", cwd);
	
	char * filepath;	
	char * firstDirectory = strtok(strdup(docpath), "/");
	firstDirectory = strtok(NULL, "/");
	fprintf(stderr, "Directory: %s\n", firstDirectory);
	docpath+=strlen(passkey);
	docpath++;

		if(firstDirectory == NULL) {
		const char * extension = "/http-root-dir/htdocs/index.html";
		filepath = (char*)malloc(strlen(cwd) + strlen(extension) +1);
		strcpy(filepath, cwd);
		strcat(filepath, extension);
	}else if(!strcmp(firstDirectory, "icons") || !strcmp(firstDirectory, "htdocs") || !strcmp(firstDirectory, "cgi-bin")) {
		const char * extension = "/http-root-dir";
		filepath = (char*)malloc(strlen(docpath) + strlen(cwd) + strlen(extension) +1);
		strcpy(filepath, cwd);
		strcat(filepath, extension);
		strcat(filepath, docpath);

	}else {
		const char * extension = "/http-root-dir/htdocs";
		filepath = (char*)malloc(strlen(docpath) + strlen(cwd) + strlen(extension) +1);
		strcpy(filepath, cwd);
		strcat(filepath, extension);
		strcat(filepath, docpath);
	
	}
	fprintf(stderr, "Fixed File Path: %s\n", filepath);

	if(strstr(filepath, "cgi-bin")) {
		processCGI(fd, filepath);
		return;
	}

	char fullpath[200] = {0};
	realpath(filepath, fullpath);
	fprintf(stderr, "Corrected full path: %s\n", fullpath);

	const char * extension = "/http-root-dir";
	char * minFilePath = (char*)malloc(strlen(cwd) + strlen(extension) + 1);
	strcpy(minFilePath, cwd);
	strcat(minFilePath, extension);

	if(fullpath < minFilePath) {
		return;
	}

	char content[256] = {0};
	if(strstr(fullpath, ".html") != 0) {
		strcpy(content, "text/html");
	}else if(strstr(fullpath, ".gif") != 0) {
		strcpy(content, "image/gif");
	}
	else {
		strcpy(content, "text/plain");
	}
	fprintf(stderr, "Content Type: %s\n", content);
 	

	struct stat s;
	char * temp = strdup(fullpath);
	char * argLocation = strstr(temp, "?");
	if(argLocation) {
		*argLocation = '\0';
	}
	stat(temp, &s);
	if(S_ISDIR(s.st_mode)) {
		fprintf(stderr, "Request for Directory\n");
		processDirectory(fd, fullpath, slash);
		return;
	}	

	FILE * op;
	op = fopen(fullpath, "r");
	if(op <= 0) {
		char * response= (char *)malloc(5000);
		sprintf(response, "HTTP/1.1 404 File Not Found \r\n Server: DARPAN\r\n Content-type: %s\r\n\r\nFile not Found", content);
		write(fd, response, strlen(response));	
	}	
	else {
		char * response = (char *) malloc(5000);
		sprintf(response, "HTTP/1.1 200 OK\r\nServer: DARPAN\r\nContent-type: %s\r\n\r\n", content);
		write(fd, response, strlen(response));
		int count = 0;
		char c;
		while(count = read(fileno(op), &c, sizeof(c))) {
			if(write(fd, &c, sizeof(c)) != count) {
				perror("write");
				return;
			}
		}
		fclose(op);

	}	
}

extern "C" void zombie(int sig) {
	int process = wait3(0, 0 ,NULL);
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void processDirectory(int socket, char * path, int slash) {
	char * sortOption;
	if(strstr(path, "?")) {
		strtok(strdup(path), "?");
	   	sortOption	= strtok(NULL, "?");
		sortOption = sortOption + 2;
		*(strstr(path, "?")) = '\0';
	}else {
		sortOption = strdup("HN");
	}

	DIR * dir = opendir(path);
	if(dir == NULL) {
		exit(2);
	}else {
		//Directory opened

		char * response1 = (char *)malloc(10000);
		sprintf(response1, "HTTP/1.1 200 OK\r\nServer: DARPAN\r\nContent-type: text/html\r\n\r\n<html><head><title>Index of %s</title></head><body><h1>Index of %s </h1><p3>Sort By</p3><br />", path, path);
		write(socket, response1, strlen(response1));
		free(response1);

		const char * response2 = "<a href=\"?O=HN\">Alphabetical</a><br /><a href=\"?O=LN\">Alphabetical Reveersed</a><br /><a href=\"?O=HS\">Greatest Size</a><br /><a href=\"?O=LS\">Lowest Size</a><br /><a href=\"?O=HM\">Last Modified </a><br /><a href=\"?O=LM\">First Modified</a><br /><br /><br />";
		write(socket, response2, strlen(response2));


		struct dirent * ent;
		struct dirent ** ents = (struct dirent **) malloc(sizeof(ent)*30);
		int entsLength = 0;
		while( (ent = readdir(dir)) != NULL) {
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			ents[entsLength] = ent;
			entsLength++;
		}
		fprintf(stderr, "Sort Option: %s\n", sortOption);
		if(!strcmp(sortOption, "HN")) {
			sortByName(ents, entsLength, 0);
		}else if(!strcmp(sortOption, "LN")) {
			sortByName(ents, entsLength, 1);
		}else if(strstr(sortOption, "HS")) {
			sortBySize(path, ents, entsLength, 1);
		}else if(strstr(sortOption, "LS")) {
			sortBySize(path, ents, entsLength, 0);
		}else if(!strcmp(sortOption, "HM")) {
			sortByModified(path, ents, entsLength, 1);
		}else {
			sortByModified(path, ents, entsLength, 0);
		}
		char * response3;
		for(int i = 0; i < entsLength; i++) {
			response3 = (char *) malloc(5000);
			
			char * path1 = (char*) malloc(strlen(path) + 100);
			strcpy(path1, path);
			strcat(path1, ents[i]->d_name);

			struct stat buffer1;
			stat(path1, &buffer1);

			char * size = (char *) malloc(100);
			sprintf(size, "%ld", buffer1.st_size);

			char * modificationTime = (char *)malloc(100);
			sprintf(modificationTime, "%ld", buffer1.st_mtim.tv_sec);
			

			sprintf(response3, "<a href=%s>%s, %s, %s</a><br />", ents[i]->d_name, ents[i]->d_name, size, modificationTime);
			write(socket, response3, strlen(response3));

			free(size);
			free(modificationTime);
			free(path1);
			free(response3);
		}	

		const char * response4="<br /><br /><img src =\"http://iphone-tricks.com/files/2015/09/back-button.jpg\" alt = \"[BACK]\" style=\"width:50px;height:50px;\">";
		write(socket, response4, strlen(response4));
		
		char * response5 = (char *) malloc(200);
		if(slash) {
			sprintf(response5, "<a href=\"..\">BACK</a>");
		}else {
			sprintf(response5, "<a href=\".\">BACK</a>");

		}
		write(socket, response5, strlen(response5));



	write(socket, "</body>\n", 8);
		closedir(dir);
	}
}

void sortByName(struct dirent ** ents, int length, int reverse) {
	if(!reverse) {
		for(int i = 0; i < length; i++) {
			for(int j = 0; j < length-1; j++) {
				if(strcmp(ents[j]->d_name, ents[j+1]->d_name) > 0) {
					struct dirent * temp = ents[j];
					ents[j] = ents[j+1];
					ents[j+1] = temp;
				}
			}
		}
	}
	else {
		for(int i = 0; i < length; i++) {
			for(int j = 0; j < length-1; j++) {
				if(strcmp(ents[j]->d_name, ents[j+1]->d_name) < 0) {
					struct dirent * temp = ents[j];
					ents[j] = ents[j+1];
					ents[j+1] = temp;
				}
			}
		}

	}
}

void sortBySize(char * path, struct dirent ** ents, int length, int reverse) {
	fprintf(stderr, "PLEASE WORK: %s\n", path);
	if(!reverse) {
		fprintf(stderr, "Sorting By High SIze\n");
		for(int i = 0; i < length; i++) {
			for(int j = 0; j < length - 1; j++) {
				char * path1 = (char *) malloc(strlen(path) + 100);
				char * path2 = (char *) malloc(strlen(path) + 100);
				strcpy(path1, path);
				strcpy(path2, path);
				strcat(path1, ents[j]->d_name);
				strcat(path2, ents[j+1]->d_name);
				struct stat buffer1;
				struct stat buffer2;
				stat(path1, &buffer1);
				stat(path2, &buffer2);
				////fprintf(stderr, "Path 1 size: %ld", buffer1.st_size);
				//fprintf(stderr, "Path 2 size: %ld", buffer2.st_size);	
				if(buffer1.st_size > buffer2.st_size) {
					struct dirent * temp = ents[j];
					ents[j] = ents[j+1];
					ents[j+1] = temp;
				}
				free(path1);
				free(path2);	
			}
		}	
		
	}
	else {
		for(int i = 0; i < length; i++) {
			for(int j = 0; j < length - 1; j++) {
				char * path1 = (char *) malloc(strlen(path) + 100);
				char * path2 = (char *) malloc(strlen(path) + 100);
				strcpy(path1, path);
				strcpy(path2, path);
				strcat(path1, ents[j]->d_name);
				strcat(path2, ents[j+1]->d_name);
				struct stat buffer1;
				struct stat buffer2;
				stat(path1, &buffer1);
				stat(path2, &buffer2);
				
				if(buffer1.st_size <= buffer2.st_size) {
					struct dirent * temp = ents[j];
					ents[j] = ents[j+1];
					ents[j+1] = temp;
				}
				free(path1);
				free(path2);	
			}
		}	
	}
}
void sortByModified(char * path, struct dirent ** ents, int length, int reverse) {
if(!reverse) {
		for(int i = 0; i < length; i++) {
			for(int j = 0; j < length - 1; j++) {
				char * path1 = (char *) malloc(strlen(path) + 100);
				char * path2 = (char *) malloc(strlen(path) + 100);
				strcpy(path1, path);
				strcpy(path2, path);
				strcat(path1, ents[j]->d_name);
				strcat(path2, ents[j+1]->d_name);
				struct stat buffer1;
				struct stat buffer2;
				stat(path1, &buffer1);
				stat(path2, &buffer2);
				
				if(buffer1.st_mtim.tv_sec > buffer2.st_mtim.tv_sec) {
					struct dirent * temp = ents[j];
					ents[j] = ents[j+1];
					ents[j+1] = temp;
				}
				free(path1);
				free(path2);	
			}
		}	
	}
	else {
		for(int i = 0; i < length; i++) {
			for(int j = 0; j < length - 1; j++) {
				char * path1 = (char *) malloc(strlen(path) + 100);
				char * path2 = (char *) malloc(strlen(path) + 100);
				strcpy(path1, path);
				strcpy(path2, path);
				strcat(path1, ents[j]->d_name);
				strcat(path2, ents[j+1]->d_name);
				struct stat buffer1;
				struct stat buffer2;
				stat(path1, &buffer1);
				stat(path2, &buffer2);
				
				if(buffer1.st_mtim.tv_sec <= buffer2.st_mtim.tv_sec) {
					struct dirent * temp = ents[j];
					ents[j] = ents[j+1];
					ents[j+1] = temp;
				}
				free(path1);
				free(path2);	
			}
		}	
	}

}

void processCGI(int socket, char * path) {
	const char * response1 = "HTTP/1.1 200 OK\r\nServer: DARPAN\r\n";
	write(socket, response1, strlen(response1));
	int PID = fork();
	char * arguments;
	if(PID == 0) {
		//char * arguments;
		if(strstr(path, "?")) {
			strtok(strdup(path), "?");
			arguments = strtok(NULL, "?");
			arguments+=2;
			*(strstr(path,"?")) = '\0';

		} else {
			arguments = strdup("NONE");
		}
		setenv("REQUEST_METHOD", "GET", 1);
		setenv("QUERY_STRING", arguments, 1);
		dup2(socket, 1);

		char * argv[1];
		argv[0] = NULL;
		execvp(path, argv);
		_exit(1);
	}
	return;
}
