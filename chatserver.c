#include "slist.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/select.h> 
#include <sys/socket.h> 
#include <signal.h> 
#include <string.h> 
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h> 
#include <sys/types.h> 
#include <arpa/inet.h> 

#define MAX_LENGTH_LINE 4096


slist_t* list;//for signal;

void error(char *msg)
{      
	perror(msg);      
	exit(1); 
}

void sighandler(int signum)// signal ^C
{
	
	slist_node_t *ptr1= list->head;//master socket in head
	ptr1=ptr1->next;
	slist_node_t *ptr2= list->head;
	while(ptr1!=NULL)//close socket
	{
		close(*((int*)(ptr1->data)));
		ptr1=ptr1->next;
	}
	close(*((int*)(ptr2->data)));//close master socket
	slist_destroy(list,SLIST_FREE_DATA);//free list node
	free(list);
	error(" caught signal\n");
}


int main(int argc, char *argv[]) 
{   

	fd_set readfd;//temp file descriptor-read
	fd_set writefd;//temp file descriptor-write
	fd_set masterfd;//master file descriptor

	struct sockaddr_in serv_addr; //server address
	struct sockaddr_in cli_addr;//client address

	int fdMax;
	char buffer[MAX_LENGTH_LINE+1]={0};
	char buf[MAX_LENGTH_LINE+1]={0};//1 for '\0'
      
	int portno; //port number
	int sockfd,newfd,lenMessage,len; 
	socklen_t adderlen;   

	if (argc !=2)
	{          
		error("Usage: server<port> \n"); 
	}

	FD_ZERO(&masterfd);//clear the master
	FD_ZERO(&readfd);//clear the temp sets
	FD_ZERO(&writefd);//clear the temp sets
	sockfd = socket(AF_INET,SOCK_STREAM,0); 
	if (sockfd < 0)  
		error("ERROR opening socket"); 
	
	portno = atoi(argv[1]); 
	serv_addr.sin_family = AF_INET;  
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	serv_addr.sin_port = htons(portno); 
	
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)                 
		error("ERROR on binding");    
	  	
	if(listen(sockfd,5)<0)
		error("ERROR on listen"); 
	
	FD_SET(sockfd,&masterfd);//add sockfd to the master set
	fdMax=sockfd;//at the beginning worth 1  
	list=(slist_t*)malloc(sizeof(slist_t));//create list
	if(list==NULL)
	{
		error("ERROR malloc"); 
	}
	slist_init(list);

	int * masterSockfd=(int*)malloc(sizeof(int));//node for master socket
	if(masterSockfd==NULL)
	{
		error("ERROR malloc"); 
	}
	*masterSockfd=sockfd;
	int check=slist_append(list,masterSockfd);

	if(check!=0)
	{
		free(masterSockfd);
		error("ERROR on slist_append");
	}

	while(1)
	{
		signal(SIGINT,sighandler);//caught signal
		readfd=masterfd;//copy
		writefd=masterfd;//copy
		if(select(fdMax+1,&readfd,&writefd,NULL,NULL)==-1)//if somthing new
			error("ERROR on select");
		slist_node_t *ptr1= list->head;
		while(ptr1!=NULL)
		{
			if(FD_ISSET(*((int*)(ptr1->data)),&readfd))//check if socket in my  new group
			{
				if(*((int*)(ptr1->data))==sockfd)//new connection
				{
					adderlen=sizeof(cli_addr);
					newfd=accept(sockfd,(struct sockaddr*)&cli_addr, &adderlen);
					if(newfd<0)
						error("ERROR on accept"); 
					else
					{
						FD_SET(newfd,&masterfd);//add sockfd to the master set
						if(newfd>fdMax)
							fdMax=newfd;
						printf("new connection from %s on socket %d\n",inet_ntoa(cli_addr.sin_addr),newfd);
						masterSockfd=(int*)malloc(sizeof(int));
						if(masterSockfd==NULL)
						{
							error("ERROR malloc"); 
						}
						*masterSockfd=newfd;
						check=slist_append(list,masterSockfd);
						if(check!=0)
						{
							free(masterSockfd);
							error("ERROR on slist_append");
						}
					}
				}
				else
				{
					printf("server ready to read from %d\n",*((int*)(ptr1->data)));
					lenMessage=recv(*((int*)(ptr1->data)),buffer,MAX_LENGTH_LINE,MSG_PEEK);
					buffer[lenMessage+1]='\0';
					char *space=strstr(buffer,"\r\n");
					if(space!=NULL)
						len=read(*((int*)(ptr1->data)),buf,(space-buffer)+2);//read until "\r\n"
					else
						len=read(*((int*)(ptr1->data)),buf,MAX_LENGTH_LINE);//read all row-4096 char
					if(len==-1)
						perror("ERROR on read");
						
					buf[len+1]='\0';
					if(lenMessage<=0)//conenection closed by client
					{
						if(lenMessage==0)
							printf("socket %d disconnect\n",*((int*)(ptr1->data)));
						else
							perror("ERROR on recv");
						close(*((int*)(ptr1->data)));
						FD_CLR(*((int*)(ptr1->data)),&masterfd);
						slist_node_t *ptr= list->head;
						while(ptr1->data!=ptr->next->data)//delete node from list
						{
							ptr=ptr->next;						
						}
						if(ptr->next==list->tail)
						{
							list->tail=ptr;
						}
						free(ptr->next->data);
						slist_node_t *tmPtr1=ptr;
						slist_node_t *tmPtr=ptr->next->next;
						free(ptr->next);//free and next
						ptr1=tmPtr1;
						ptr1->next=tmPtr;
			
					}
					else//socket write to chat
					{
						slist_node_t *ptr2= list->head;
						while(ptr2!=NULL)
						{	
							if(FD_ISSET(*((int*)(ptr2->data)),&writefd))
							{
								if(*((int*)(ptr2->data))!=sockfd)
								{
									printf("server ready to write to %d\n",*((int*)(ptr2->data)));
									if(send(*((int*)(ptr2->data)),"guest",5,0)==-1)
										perror("ERROR on send");
									char *sdGust=(char*)malloc(sizeof(char)*(*((int*)(ptr1->data))));//print gust
									if(sdGust==NULL)
										error("ERROR on malloc"); 
									int tmp=*((int*)(ptr1->data));
									sprintf(sdGust,"%d",tmp);
									if(send(*((int*)(ptr2->data)),sdGust,strlen(sdGust),0)==-1)
										perror("ERROR on send");
									free(sdGust);
									if(send(*((int*)(ptr2->data)),":",1,0)==-1)
										perror("ERROR on send");
									if(send(*((int*)(ptr2->data)),buf,len,0)==-1)//print message
										perror("ERROR on send"); 
									if((len==MAX_LENGTH_LINE)&&(space==NULL))//linebreak
									{
										if(send(*((int*)(ptr2->data)),"\r\n",3,0)==-1)
											perror("ERROR on send");

									}
								}
							}
							ptr2=ptr2->next;
						}
					}
				}
			}
			ptr1=ptr1->next;
		}
	}
	return 0;
}
