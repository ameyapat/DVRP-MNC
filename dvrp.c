#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#define STDIN 0
#define INF 999

struct topo
{
 int no_servers;
 int no_edges;
 int serv_id[5];
 char* ip[5];
 int portno[5];
 //int cost[5];
 //int neigh[5];
};
struct neigh
{
 int neigh_id;
 int neigh_cost;
 int val;
 int rec_count;
 int disabled;
}neigh_table[10];    // NEIGHBOUR TABLE

int cost_table[5][5];  // COST TABLE

struct routab          // ROUTING TABLE
{
 int dest;
 int nxthop;
 int noh;	
};

struct m1        // Part of message containing info of server sending packet
   {
	int16_t no_upd;
	int16_t src_port;
	char ip[4];
   };
  
  struct m2          // Part of message containing info about nth server in routing table
  {
	  char sip[4];
	  int16_t nxt_port;
	  int16_t nxt_id;
	  int16_t pad;
	  int16_t nxt_cost;
  };	  
	
   struct fmsg   // Final msg, combination of m1 and m2
   {
	struct m1 ms1;
	struct m2 mg[5];   
	   
	};
	
struct sockaddr_in server;
	
void send_vect(struct topo ,struct fmsg ,char* ,int ,int );
void ctoc(unsigned char* ip,char* t);
void recv_vect(struct topo ,struct fmsg ,int );
int least(int* ,int );
void calc_table();
void disp_cost_table();
void disp_rout_table();
void pack();

int myserverid;
int sentpack;
int recvpack;
int crash;
int interval;
struct routab rout_table[5];  // ROUTING TABLE
time_t curr_time;

int main()
{
FILE *top;
 char* line = NULL;
 ssize_t len = 0;
 ssize_t readline;

fd_set master_set, read_set;


struct fmsg f1,f2;   // f1 for send f2 for recv
struct topo topl_file,*ptr;
ptr=(struct topo *)malloc(sizeof(struct topo) * 5);
topl_file=*ptr;
int i,j;
for(i=0;i<5;i++)
topl_file.ip[i]=(char *)malloc(50);


for(i=0;i<10;i++)
{
	neigh_table[i].val=0;
	neigh_table[i].rec_count=0;
	neigh_table[i].disabled=0; 
}

for(i=0;i<5;i++)          // Assigning initial values in cost table as INF
for(j=0;j<5;j++)
{cost_table[i][j]=INF;
rout_table[i].nxthop=0;
rout_table[i].noh=INF;
}



//-----------------------------Message Format--------------------------------

	
//---------------------------------------------------------------------------
//Machine specific variables

int myneighbours[5];
int mycost[5];
char* myip;
myip=(char *)malloc(20);
int myport;
char* split;
char* fname=(char *)malloc(20);

char uip[5][50];
sentpack=0;
recvpack=0;
crash=0;
//--------------------------------read topology File---------------------------
while(1)
{
printf("Please execute command to read topology File server -t <fname> -i <interval> \n");

scanf("%s %s %s %s %s",uip[0],uip[1],uip[2],uip[3],uip[4]);


		if(strcmp(uip[0],"server")!=0)
		{
			printf("Invalid command, Please type in this format-> server -t <fname> -i <interval>\n");
			continue;
	    }
		else{
			
			if(strcmp(uip[1],"-t")!=0)
		      {
		        printf("Invalid command, Please type in this format-> server -t <fname> -i <interval> \n");
			   continue;
			}
			else
			  {
			   
			   strcpy(fname,uip[2]);
		       }
		     if(strcmp(uip[3],"-i")!=0)
		     {
		     printf("Invalid command, Please type in this format-> server -t <fname> -i <interval> \n");
		     continue;
		      }
		     else
		     {
				 
				interval=atoi(uip[4]);
				
				break; 
			 }
			}
}
top= fopen(fname,"r");
if(top==NULL)
 {
  puts("Cannot Open File");
  exit(1);
 }

readline = getline(&line, &len, top);
if(readline!=-1)
topl_file.no_servers=*line-'0';  // Getting the no. of Servers

readline = getline(&line, &len, top);
if(readline!=-1)
topl_file.no_edges=*line-'0';  // Getting the no. of Neighbours 

int count=0;
int cn=0;


while(count!=topl_file.no_servers && (readline = getline(&line, &len, top)) != -1)
  {
	    split=strtok(line," ");
	    topl_file.serv_id[cn]=*split-'0';
//printf("servid %d\n",topl_file.serv_id[cn]);
	    
	    split=strtok(NULL," ");
	   strcpy(topl_file.ip[cn],split);
	   //topl_file.ip[cn]=split;
	    
	    	    
	    split=strtok(NULL," ");
	    topl_file.portno[cn]=atoi(split);
	   // printf("Port No. %d\n",topl_file.portno[cn]); 	    
	    
		 cn++; 
		count++;  
  }
   
    printf("No. of Servers= %d \n",topl_file.no_servers);
    
    printf("No. of edges= %d \n",topl_file.no_edges);
    int p;
    for(p=0;p<5;p++)
    printf("ID- %d IP- %s PORT NO.- %d \n",topl_file.serv_id[p],topl_file.ip[p],topl_file.portno[p]);
    int temp;
    while((readline=getline(&line, &len, top))!= -1)
    {
		split=strtok(line," ");
		myserverid=*split-'0';
		
		split=strtok(NULL," ");
		temp=*split-'0';
		myneighbours[temp]=temp;
	    neigh_table[temp].neigh_id=temp;                   // add initial values in neighbour table
	    	
		split=strtok(NULL," ");
		mycost[temp]=*split-'0';
		neigh_table[temp].neigh_cost=mycost[temp];        //add cost to neighbour in neighbour table
		neigh_table[temp].val=1;
		printf("Neighbour ID- >%d cost->%d \n",neigh_table[temp].neigh_id,neigh_table[temp].neigh_cost);
	}
    
    fclose(top);
  //---------------------------DONE READING TOPOLOGY FILE-------------------------------------------
 
 //--------Add read values in all tables----------------------------------------------------------
   
   cost_table[myserverid-1][myserverid-1]=0;          // Cost to self is 0
   j=0;
    for(i=0;i<10;i++)
    {
		if(neigh_table[i].val==1 && j<5)
        {
		cost_table[myserverid-1][neigh_table[i].neigh_id-1]=neigh_table[i].neigh_cost;
        rout_table[neigh_table[i].neigh_id-1].nxthop=neigh_table[i].neigh_id;
        rout_table[neigh_table[i].neigh_id-1].noh=neigh_table[i].neigh_cost;
	    }
    }
   // printf("Cost Table after reading from topology File\n");
    //disp_cost_table();
    
    printf("Routing Table after reading from topology File\n");
    disp_rout_table();
   
   f1.ms1.no_upd=topl_file.no_edges;
 
 //------------------------------------------------------------------------------------
 //Find own Ip address and Port-no. from toplogy file and initialize a socket 
 
 for(i=0;i<5;i++)
 if(myserverid==topl_file.serv_id[i])
 {
  strcpy(myip,topl_file.ip[i]);
  myport=topl_file.portno[i];
 }
 
 printf("My id- My Ip- My port %d %s %d \n",myserverid,myip,myport);
 int sock=socket(AF_INET,SOCK_DGRAM,0);
 if(sock<0)
 perror("Socket failed because : ");
 
    unsigned char mip[40];
    ctoc(mip,myip);
    memset(&server,0,sizeof(struct sockaddr_in));
    //printf("MYIP-->%s\n",myip);
    server.sin_addr.s_addr = inet_addr(myip);
	server.sin_family = AF_INET;
	server.sin_port = htons(myport);
 int rc,on=1;
 rc=setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,&on, sizeof(int));
   if (rc < 0)
   {
      perror("setsockopt() failed");
      close(STDIN);
      exit(-1);
   }

   
   //Set socket to be non-blocking.  
   if(ioctl(STDIN, FIONBIO, (char *)&on)<0)
   {
      perror("ioctl() failed");
      close(STDIN);
      exit(-1);
   }
 
 if(bind(sock,(struct sockaddr *)&server, sizeof(server))<0)
 perror("Bind failed because: ");
 
  
  int desc;
  char buf[50]="";
  struct timeval *tv;
  tv = (struct timeval *)malloc(sizeof(struct timeval));
  (*tv).tv_sec = interval;
  (*tv).tv_usec = 0;
  int g;
  
  FD_ZERO(&master_set);
  FD_SET(STDIN, &master_set);
  FD_SET(sock, &master_set);
  
  char *spl;
  int up1,up2;
  char *up_cost;
  int disfl =0;
  int cv=0;
  time_t t1,t2;
  double dif;
  
  while(1)       // User input Loop
  {
	  
	 //printf("\n My ID = %d \n",myserverid); 
	 fflush(stdout);
    
	read_set=master_set;
	

	(*tv).tv_sec = interval;
    (*tv).tv_usec = 0;
	
	desc=select(sock+1,&read_set,NULL,NULL,tv);
	//printf("No of desc--> %d\n",desc);
	if(desc<0)
	perror("Select failed because : "); 
	
		 
	if(FD_ISSET(STDIN,&read_set))
	{
		printf("In STDIN\n");
		if((g=read(STDIN,buf,sizeof(buf)))<0)
		{
			perror("Error in STDIN because:");
			break;
		}
		buf[g-1]=0;
		spl=strtok(buf," ");
		//scanf("%s",buf);
		
		//printf("spl -> [%s]",spl);
		if(spl==NULL)
		continue;
		
		if(strcmp(spl,"step")==0)
		{
			if(crash==1)
			{
				printf("step Error : cannot send server has crashed");
				continue;
		    }
		  else
		  {  
		  send_vect(topl_file,f1,myip,myport,sock);
		  printf("step SUCCESS \n");
		  
		  continue;
	      }	
	    }
	    
		else if(strcmp(spl,"update")==0)
		{
			//scanf("%d",up1);
		  up1=atoi(strtok(NULL," "));
		  up2=atoi(strtok(NULL," "));
		  up_cost=strtok(NULL," ");
		  
		  if(up1!=myserverid)
		  {
			  printf("\n update Error : 1st parameter should be same as host server id \n");
			  continue;
	       }
	       else
	       {
			   if((strcmp(up_cost,"inf")==0) || (strcmp(up_cost,"INF")==0) )
		       {
					cost_table[up1-1][up2-1]=INF;
					for(i=0;i<10;i++)
					if(neigh_table[i].neigh_id==up2)
					neigh_table[i].neigh_cost=INF;  
			    }
			    else
			    {
			     cost_table[up1-1][up2-1]=atoi(up_cost);
				 for(i=0;i<10;i++)
				 if(neigh_table[i].neigh_id==up2)
				 neigh_table[i].neigh_cost=atoi(up_cost);
				}
		     printf("\n update SUCCESS \n");
		     continue;
	       }	
	    }
	    
	    else if(strcmp(spl,"packets")==0)
		{
		  pack();
		  printf("\n packets SUCCESS \n");
		  continue;	
	    }
	    
	    else if(strcmp(spl,"display")==0)
		{
			if(crash==1)
		     {
			   printf("Server %d has crashed, Routing Table can't be displayed \n",myserverid);
			   continue;
	         }
		  printf("\n Routing Table : \n");
		  disp_rout_table();
		  continue;	
	    }
	    
	    if(strcmp(spl,"disable")==0)
		{
		up1=atoi(strtok(NULL," "));
		  
		  for(i=0;i<10;i++)	
		  if(neigh_table[i].neigh_id==up1 && neigh_table[i].val==1)  // Checking if parameter is neighbour
		  {
			 disfl=neigh_table[i].neigh_id; 
		  }
		  if(disfl!=0)
		  {
		  printf("neigh id = %d",disfl);
		  for(i=0;i<10;i++)
		  {
			if(neigh_table[i].neigh_id==disfl)
			{
		     neigh_table[i].val=0;  // will not send
		     neigh_table[i].disabled=1;  
		     cost_table[myserverid-1][neigh_table[i].neigh_id-1]=INF;
		     cost_table[neigh_table[i].neigh_id-1][myserverid-1]=INF;	
			}  
		  }
		     
		   calc_table();
		   printf("\n disable SUCCESS \n");
		   continue;  
			  
		  }
		  else
		  {
		   printf("\n disable error : %d is not a neighbour of %d \n",up1,myserverid); 
		   continue;
	      }	
	    }
	    
	    if(strcmp(spl,"crash")==0)
		{
		  crash=1;
		  printf("\n crash SUCCESS \n");
		  continue;	
	    }
      else {
				printf("\nError wrong command");
				continue;
			}
      }
        
    else if(FD_ISSET(sock,&read_set) && crash==0)
		{
			
			printf("Receiving...\n");
			recv_vect(topl_file,f2,sock);
	        calc_table();
		}
		
   else
   {
	   if(crash==1)
	   {
		//Do nothing
		}
	   else
	   {
	 // Nothing happend in STDIN and recv
	
	   
	/* for(i=0;i<10;i++)
	  {
		if(neigh_table[i].val==1)  
		  {
			  if(neigh_table[i].disabled!=1)
			  {
			     if(neigh_table[i].rec_count>0)
			       {
						neigh_table[i].rec_count++;
						
						if(neigh_table[i].rec_count>3)
						{
						 printf("\n Did not receive from %d for three intervals\n",neigh_table[i].neigh_id);
						 cost_table[myserverid-1][neigh_table[i].neigh_id-1]=INF;
						 cost_table[neigh_table[i].neigh_id-1][myserverid-1]=INF;
						 neigh_table[i].disabled=1;
						 calc_table();
						 
						 }  
			        }
			       else
			       {
					   neigh_table[i].rec_count=1;
			       } 
			 } 
		  }
       } */
	  
	printf("Sending periodic Update \n");
	send_vect(topl_file,f1,myip,myport,sock);                   // Send vectors on periodic intervals
		
    }
    
   }
              
  }
   
  return 0;  
} 

void send_vect(struct topo topl_file,struct fmsg f1,char* myip,int myport,int sock)
{
  int i,j=0;
  int k=0;
  int neigh_id[5];  // id of neighbours
  int neigh_id1[5];
  int pno[5];
  int pno1[5];
  char* ip[5];
  char* ip1[5];
  int cost[5];
  int no_neigh=0;
  
  if(crash==1)
  return ;
  for(i=0;i<10;i++)                  // Storing neighbour id and cost in temp arrays
  {
	  if(neigh_table[i].val==1 && neigh_table[i].disabled!=1)
	   {
		   neigh_id1[j]=neigh_table[i].neigh_id;
		   //cost[j]=neigh_table[i].neigh_cost;
		   //cost[j]=cost_table[myserverid-1][neigh_table[i].neigh_id-1];
		   j++;
		   no_neigh++;
	   }
  }	
	
  for(i=0;i<5;i++)              
   {
     //topl_file.serv_id[p],topl_file.ip[p],topl_file.portno[p]
     for(j=0;j<5;j++)
     {
       if(neigh_id1[i]==topl_file.serv_id[j])
       {
         ip1[k]=topl_file.ip[j];
         pno1[k]=topl_file.portno[j];
         k++;	
	   }
     }    
    }
	
	
  for(i=0;i<5;i++)              
   {
	   cost[i]=cost_table[myserverid-1][i];
	   neigh_id[i]=i+1;
         ip[i]=topl_file.ip[i];
         pno[i]=topl_file.portno[i]; 
    }
    
	f1.ms1.no_upd=5;
	f1.ms1.src_port=myport;
	ctoc((unsigned char*)f1.ms1.ip,myip);
	//printf("Sender Ip %s \n",myip);
	
	for(i=0;i<5;i++)
	{
	  ctoc((unsigned char*)f1.mg[i].sip,ip[i]);	
	  f1.mg[i].nxt_port=pno[i];
	  f1.mg[i].nxt_id=neigh_id[i];
	  f1.mg[i].nxt_cost=cost[i];
    }
    
   struct sockaddr_in sender;                     // Send to all neighbours
    for(i=0;i<no_neigh;i++)
    {
    memset(&sender,0,sizeof(struct sockaddr_in));
    //printf("IP sent to ->%s\n",ip[i]);
    sender.sin_addr.s_addr = inet_addr(ip1[i]);
	sender.sin_family = AF_INET;
	sender.sin_port = htons(pno1[i]);
	
	 if(sendto(sock,(void *)&f1,sizeof(f1),0,(struct sockaddr*)&sender,sizeof(struct sockaddr_in))<0)
	    {
			perror("Send Failed Because :");
	    }
	  else
	  printf("\n Sent update to Neighbour %d \n",neigh_id1[i]);
	  sentpack++;
	  //printf("\n Packet no. sent =%d \n",sentpack);  
    }
}


void ctoc(unsigned char* ip,char* t)       // Splits ip in char*(parameter 2) to char [4] (parameter 1)
{
	//printf("1st line ctoc \n");
    char str[80];
   strcpy(str,t);
   //printf("In ctoc 459\n");
   const char s[2] = ".";
   char *token;
   int j=0;
   /* get the first token */
   token = strtok(str,s);
   //printf("In ctoc 465\n");
   ip[0]=atoi(token);  
   //printf("In ctoc 467\n");
   /* walk through other tokens */
   j=1;
   while(token != NULL && j<4 ) 
   {
      token = strtok(NULL, s);
      ip[j]=atoi(token);
     // printf( "-ctoc- %d\n", ip[j]);
      j++;
      
   }
	//printf("In ctoc 478\n");
}

void recv_vect(struct topo topl_file,struct fmsg f2,int sock)
{
  int i;
  
  int recv_id;
  
 int siz=sizeof(server);
 if(crash==1)
  return ;
  	
if(recvfrom(sock,(void *)&f2,sizeof(f2),0,(struct sockaddr*)&server,&siz)<0)
 {
		perror("\nError in recv because :");
 }
 else
 {
   for(i=0;i<5;i++)
   {
	if(f2.ms1.src_port==topl_file.portno[i])
	  {
		  recv_id=topl_file.serv_id[i];
		  //printf("\nid- %d IP- %s",recv_id,f2.ms1.ip);
	  }	  
   }
   
   
   
   for(i=0;i<10;i++)
   {
	if(neigh_table[i].neigh_id==recv_id)
	   {
	     if(neigh_table[i].disabled==1)
	     {
			 printf("\nNeighbour %d is disabled\n",recv_id);
			 return ;
	     }
	      else
	      {
			neigh_table[i].rec_count=0;// DO Nothing 
	      }
	   }
	    
   }
   
    printf("Recv_id = %d \n",recv_id);
	for(i=0;i<f2.ms1.no_upd;i++)
	{
		printf("Received from %d that it is connected to %d with cost %d \n",recv_id,f2.mg[i].nxt_id,f2.mg[i].nxt_cost);
		cost_table[recv_id-1][f2.mg[i].nxt_id-1]=f2.mg[i].nxt_cost;
	    cost_table[f2.mg[i].nxt_id-1][recv_id-1]=f2.mg[i].nxt_cost;
	   //rout_table[f2.mg[i].nxt_id-1].noh=cost_table[recv_id-1][f2.mg[i].nxt_id-1];
	     
    }
    cost_table[recv_id-1][recv_id-1]=0;
    calc_table();
    //printf("Cost table after recv row from %d \n",recv_id);
    //disp_cost_table();
    //printf("Routing Table after recv row\n");
    //disp_rout_table();
    recvpack++; 
 }	

}


void calc_table()
{
  int i,j,k; 
  int c=0;
  
  
  int nei[4];
  
  for(j=0;j<10;j++)                  // Finding Neighbours
  {
	  if(neigh_table[j].val==1 && neigh_table[j].disabled==0)
      {
		  nei[c++]=neigh_table[j].neigh_id-1;
		 // printf("\nnei->%d",nei[c-1]+1);
       }
   }     
  int c1[c];
  int temp[c];  
  int c2[c][5];
     
  for(j=0;j<c;j++)
  {
	  c1[j]=cost_table[myserverid-1][nei[j]];  // Storing costs from myid to each neighbour
  }
  
  /*for(j=0;j<c;j++)
  printf("c1[%d]=%d\n",j,c1[j]);*/
  
  for(i=0;i<5;i++)  //destn
  {
   for(j=0;j<c;j++)  //neighbour
   {
    c2[j][i]=cost_table[nei[j]][i];
    //printf("c2[%d][%d]=%d  ",j,i,c2[j][i]);
   }
  } 
  
  int y=0;
  int h;
  for(y=0;y<5;y++)
  {
     //printf("y outside loop= %d",y);
     if(y==(myserverid-1))
     {
		 cost_table[myserverid-1][y]=0;
		 continue;
      }
      for(i=0;i<c;i++)
         {
			//printf("k= %d i=%d c=%d\n",k,i,c); 
	      temp[i]=c1[i]+c2[i][y];
	      //printf("temp[%d]=%d\n",i,temp[i]);
         }
       rout_table[y].noh=cost_table[myserverid-1][y];
            for(h=0;h<c;h++)
			if(temp[h]==cost_table[myserverid-1][y])
			rout_table[y].nxthop=nei[h]+1;
			
       if(cost_table[myserverid-1][y]>least(temp,c))  
        {
			for(h=0;h<c;h++)
			if(temp[h]==least(temp,c))
			rout_table[y].nxthop=nei[h]+1;
			
			cost_table[myserverid-1][y]=least(temp,c);
			cost_table[y][myserverid-1]=least(temp,c);
			rout_table[y].noh=cost_table[myserverid-1][y];
		}
			   
  }
  int inc=0;
  for(k=0;k<5;k++)
  {
	 for(j=0;j<c;j++) 
	 {
	 if((k+1) != nei[j] && (k+1)!=myserverid)
	 {  
		for(inc=0;inc<5;inc++)
		if(inc==k)
		cost_table[k][k]=0;
	    else
	    cost_table[k][inc]=cost_table[inc][k];
      }
     }  
  }
    
 /* printf("Cost table after Calculation \n");
  disp_cost_table(); 
  printf("Rout table after Calculation \n");
  disp_rout_table(); */	
	
}
int least(int* temp,int c)
{	
 	int i;
 	int ls=temp[0];
 	for(i=0;i<c;i++)
 	{
 	 //printf("i=%d in Temp -> [ %d ] \n",i,temp[i]);
 	 if(temp[i]<ls && temp[i]!='\0')
 	  ls=temp[i];
    }
    //printf("\n Least = %d \n",ls);
 return ls;	
}

void disp_cost_table()
{
	int i,j;
for(i=0;i<5;i++)
    {
		for(j=0;j<5;j++)
        printf("%d \t",cost_table[i][j]);
		printf("\n");
	}
}

void disp_rout_table()
{
	int i;
	if(crash==1)
		{
			printf("Server %d has crashed, Routing Table can't be displayed \n",myserverid);
			return ;
	    }
	printf("\n Dest. \t| Next Router \t| Cost \n");
	printf("----------------------------------------");
for(i=0;i<5;i++)
    {
		
		if((i+1)==myserverid)
		continue;
		
		else
		{
	    printf("\n %d  \t    %d   \t    %d \n",i+1,rout_table[i].nxthop,rout_table[i].noh);
        }
	}
}

void pack()
{

printf("\n No. of packets received since last call= %d \n",recvpack);
recvpack=0;	
}	
