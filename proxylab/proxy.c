
/*
 * proxy.c - A simple HTTP proxy that caches web objects 
 * 
 * proxy: client->proxy->server->proxy->client 
 * 
 */
#include <stdio.h>
#include "csapp.h"
#include "cache.h"

extern ProxyCache cache;
#define CONN "Connection"
#define PROXY_CONN "Proxy-Connection"
#define HOST "Host"
#define USER_AGENT "User-Agent"
#define DEFAULT_PORT "80"
#define HTTP_VERSION "HTTP/1.0"
#define CONTENT_TYPE "Content-type"
#define CONTENT_LEN "Content-length"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_conn_hdr = "Proxy-Connection: close\r\n";

void doit(int fd);
void build_requesthdrs(rio_t *clirio,char *hostname,char *path,char *requesthdrs);
void parse_uri(char *uri,char *hostname,char *path,char *port);
void *thread(void *vargp);

int main(int argc,char **argv)
{
    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    /* Check command line args */
    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
    }
    init_cache();
    signal(SIGPIPE, SIG_IGN); // ignore sigpipe
    listenfd = Open_listenfd(argv[1]);
    while (1) {
		clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
		*connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid,NULL,thread,(void *)connfd);
    }
    return 0;
}

void *thread(void *vargp){
    int connfd = *(int *)vargp;
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
}


void doit(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filepath[MAXLINE], hostname[MAXLINE], requesthdrs[MAXLINE], port[MAXLINE];
    char urltag[MAXLINE],cachebuf[MAX_OBJECT_SIZE];
    int clifd;
    rio_t rio, clirio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //read request line
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //parse request line
    if (strcasecmp(method, "GET")) {                     //only deal with "GET" request
        return;
    } 
    parse_uri(uri, hostname, filepath, port);  
    sprintf(urltag, "%s%s%s", hostname, filepath, port); 

    if( find_cache(urltag, fd) != -1){  // if find, just write to client 
         return;
    }

    build_requesthdrs(&rio, hostname, filepath, requesthdrs);    
    
    clifd = open_clientfd(hostname, port);  // clifd in proxy connect to server
    if(clifd < 0){
        printf("Connection failed\n");
        return;
    }
    Rio_readinitb(&clirio,clifd);
    Rio_writen(clifd,requesthdrs,strlen(requesthdrs));

    size_t bufsize = 0, n;
    while((n=Rio_readlineb(&clirio,buf,MAXLINE))!=0)
    {
        printf("Proxy received %u bytes,then send\n",n);
        bufsize += n;
        if(bufsize < MAX_OBJECT_SIZE)strcat(cachebuf,buf);
        Rio_writen(fd,buf,n);
    }
    Close(clifd);
    if(bufsize <= MAX_OBJECT_SIZE)add_cache(urltag,cachebuf); 
}

/*
 * Mind: Host, User-Agent, Connection, and Proxy-Connection
 * except Method, headers are not case sensitive.
 */
void  build_requesthdrs(rio_t *client_rio,char *hostname,char *path, char *requesthdrs)
{
    char buf[MAXLINE];
    sprintf(requesthdrs,"GET %s %s\r\n%s%s%s", path, HTTP_VERSION, 
                                            user_agent_hdr, prox_conn_hdr, conn_hdr); 
    int mask = hostname?1:2;
    while(mask && Rio_readlineb(client_rio,buf,MAXLINE))
    {
        if(strncasecmp(buf,CONTENT_LEN,strlen(CONTENT_LEN))){
            strcat(requesthdrs,buf);    // content len is necessary for HTTP/1.0
            mask --;
            continue;
        }
        if(!hostname && strncasecmp(buf,HOST,strlen(HOST))){
            strcat(requesthdrs,buf);
            mask --;
            continue;
        }
    }
    // add host header and clrf
    if(hostname) sprintf(requesthdrs, "%sHost: %s\r\n\r\n",requesthdrs, hostname);
    return ;
}

/*
 * parse_uri - parse URI into hostname, filename and port
 * Request-URI    = absoluteURI | abs_path 
 *             
 */
void parse_uri(char *uri,char *hostname,char *path,char *port)
{
    char *hostpos, *portpos;
    if( (hostpos = strstr(uri,"//"))!=NULL){ // absoluteURI:"http:" "//" host [ ":" port ] [ abs_path ]
        hostpos += 2;
        portpos = strstr(hostpos,":");
        if(portpos){
            int portval;
            *portpos = '\0';
            if(hostpos)sscanf(hostpos,"%s",hostname);
            sscanf(portpos+1,"%d%s",&portval,path);
            sprintf(port,"%d",portval);
        }
        else{
            char *pathpos;
            
            if((pathpos = strstr(hostpos, "/"))!=NULL){
                *pathpos = '\0';
                sscanf(hostpos,"%s",hostname);
                *pathpos = '/';
                sscanf(pathpos,"%s",path);
            }
            else{
                sscanf(hostpos,"%s",hostname);
                strcpy(path,"/");
            }
        }
    }
    else{   // abs_path: /*
        port = DEFAULT_PORT;
        path = uri;
        hostname = NULL;
    }
    return;
}


