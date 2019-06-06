#include "server.h"

int sockfd = 0; 			//套接字
int new_fd = 0;
FILE * fp = NULL; 			//文件指针，用于指向保存聊天记录的文件
int max = 0; 				//max 表示当前 client 数组中最大的用户的 i 值
static int client[MAXFD]; 	//保存所有客户端对应的套接口描述符
static fd_max = 0;
int snd_index = 0;
pthread_t tid_acpt, tid_check = -1;//线程ID

int n_fd = 4;
char * name_fd[MAXFD][NAMESIZE] = {'\0'}; //用于保存用户的账户名，以及与描述符的对应关系

void handler(int num)
{
	int i = 0;
	pthread_detach(tid_acpt); //分离线程
	pthread_detach(tid_check);
	fclose(fp);
	//删除accpt()返回的套接字
	for(i =4;i<4+fd_max*2;i++)
		close(i);
	close(sockfd);
	pthread_cancel(tid_acpt);//结束线程
	pthread_cancel(tid_check);
	printf("已回收系统资源\n");
	exit(1);

}


int main(void)
{

	signal(SIGINT,handler); //注册清理函数

	TCP_Init();  //初始化套接字
	/*创建线程等待接受客户端连接请求*/
	if (pthread_create(&tid_acpt, NULL, wait_client_connect, (void *) & tid_check) != 0)
	{
		fprintf(stderr, "Creat pthread Error:%s\n", strerror(errno));
		close(sockfd);
		exit(1);
	}

	pthread_join(tid_acpt, NULL);//连接 等待客户端连接线程结束
	if (tid_check != -1)
		pthread_join(tid_check, NULL);

	close(sockfd);
	fclose(fp);
	exit(0);
}




/********************************************************************************
 * 函数功能：TCP套接字初始化，包括套接字的创建，绑定，监听
 * 参数：
 * 返回值:
 *
 * */
void TCP_Init(void)
{
	struct sockaddr_in server_addr;  //服务器地址结构

	/* 服务器端开始建立 socket 描述符 */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Socket error:%s\n\a", strerror(errno));
		exit(1);
	}
	/* 服务器端填充 sockaddr 结构 */
	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET; 				//IPV4协议
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//本地IP地址
	//server_addr.sin_addr.s_addr = inet_addr("192.168.122.1");//本地IP地址
	server_addr.sin_port = htons(PORT);				//端口号

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *) & reuse, sizeof(int));

	/* 捆绑 sockfd 描述符 */
	if (bind(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
	{
		fprintf(stderr, "Bind error:%s\n\a", strerror(errno));
		exit(1);
	}
	/* 监听 sockfd 描述符 */
	if (listen(sockfd, LISTMAX) == -1)
	{
		fprintf(stderr, "Listen error:%s\n\a", strerror(errno));
		exit(1);
	}
	printf("等待用户连接中......\n");
}



/****************************************************************************
 * 该线程等待客户端连接线程入口函数，
 * 将新的套接字存入全局数组 static int client[MAXFD]中
 *
 * */
void * wait_client_connect(void * pthidcheck)
{

	struct sockaddr_in client_addr; //用来保存连接服务器的客户端信息的数据结构
	int sin_size, portnumber;
	char clientname[NAMESIZE] = {0};//客户端昵称
	char new_client_inform[NEWCLIENTINFORMSIZE] = {0};//新上线的用户信息缓冲区
	static int index = 0;
	char strhost[16];
	time_t sec = 0;
	struct tm * p_time = NULL;

	while (1)
	{
		if (max >= MAXFD)
		{
			printf("已经达到人数上线\n");
			continue;
		}
		sin_size = sizeof(struct sockaddr_in);

		/* 服务器阻塞,直到客户程序建立连接 */
		if ((new_fd = accept(sockfd, (struct sockaddr *)(&client_addr), &sin_size)) == -1)
		{
			fprintf(stderr, "Accept error:%s\n\a", strerror(errno));
			exit(1);
		}
		printf("new_fd = %d\n",new_fd);
		fd_max++;
		client[max] = new_fd;//将新的描述符添加到描述符集合中

		if (read(client[max], clientname, sizeof(clientname))==-1)
		{//接收新上线的用户的昵称
			fprintf(stderr, "Read client name error:%s\n", strerror(errno));
			close(sockfd);
			exit(1);
		}
		max++;//当前人数+1

		strcpy((char *)name_fd[n_fd++],clientname);//将用户名保存到数组中，用于绑定用户名和返回的描述符
	//	n_fd +=2;
		sec = time(NULL);
		p_time = localtime(&sec);
		/*
		   sprintf(new_client_inform, "N%d-%d-%d %0d:%0d:%0d", (1900 + p_time->tm_year), (1 +
		   p_time->tm_mon), (p_time->tm_mday), (p_time->tm_hour), (p_time->tm_min), (p_time->tm_sec)); 
		   */
		sprintf(new_client_inform, "N%0d:%0d:%0d %d-%d-%d", (p_time->tm_hour), (p_time->tm_min), (p_time->tm_sec), (1900 + p_time->tm_year), (1 +p_time->tm_mon), (p_time->tm_mday)); 
		sprintf(new_client_inform + TIMESIZE, " 新用户 %s 进入聊天室", clientname);

		//向聊天室中所有的客户通知新用户上线
		int snd_index = 0;
		while (snd_index < max) //
		{
			if (write(client[snd_index], new_client_inform, sizeof(new_client_inform)) == -1)
			{
				fprintf(stderr, "Write Error:%s\n", strerror(errno));
				close(sockfd);
				exit(1);
			}
			snd_index++;
		}

		/*创建线程接收收客户端的消息并转发给所有其它在线客户端，同时打开文件,并保存聊天记录*/
		if (max == 1)
		{
			if (pthread_create((pthread_t *)pthidcheck, NULL, check_client_send, NULL)!=0)
			{
				fprintf(stderr, "Creat pthread Error:%s\n", strerror(errno));
				close(sockfd);
				exit(1);
			}
			fp = fopen("serv_msgrd.txt", "ab");//打开文件，用于保存消息记录
			if (fp == NULL)
			{
				fprintf(stderr, "Fopen Error:%s\a\n", strerror(errno));
				close(sockfd);
				exit(1);
			}
		}

		else if (max > 1)
		{

			pthread_cancel(*(pthread_t *)pthidcheck);
			if (pthread_create((pthread_t *)pthidcheck, NULL, check_client_send, NULL)!=0)
			{
				fprintf(stderr, "Creat pthread Error:%s\n", strerror(errno));
				fclose(fp);
				close(sockfd);
				exit(1);
			}


		}
		printf("%s\n", new_client_inform + 1);
		printf("%s\n", new_client_inform + TIMESIZE);
		fprintf(fp, "%s\n", new_client_inform + 1);
		fprintf(fp, "%s\n", new_client_inform + TIMESIZE);
	}
}



/**********************************************************
 * 函数：check_client_send   监听转发线程入口函数
 * 功能：监听客户端的消息发送状态，如果接收到消息，调用转发函数，将消息转发给所有的客户端
 * 参数：
 * 返回值：无
 * */
void * check_client_send(void * arg) //监听转发线程入口函数
{
	static int index = 0;//
	int num = 0;
	int i = 0;
	fd_set rdset;
	FD_ZERO(&rdset);   //清空轮循集合
	write(client[0],"你已经成为该聊天室的管理员",42);
	while (1)
	{
		if (max > 0)
		{
			for (index = 0; index < max; index++)
				FD_SET(client[index], &rdset);//将所有的描述符添加到描述符集合
			//阻塞监听所有的客户端是否发送消息，返回事件的个数
			num = select(FD_SETSIZE, &rdset, NULL, NULL, NULL);
			switch (num)
			{
				case 0:continue;
				case -1:
					   perror("select error!");
					   exit(-1);
				default:
					   for (index = 0, i = 0; index < max && i < num; index++)
						   if (FD_ISSET(client[index], &rdset))
						   {//轮循判断是否是当前描述符发生事件变化
							   i++;
							   recv_and_send_to_all_client(index);//调用函数接收客户端的消息
						   }
			}
		}
	}
}



/****************************************************
 * 函数：recv_and_send_to_all_client
 * 功能：将接收到的客户端信息转发给所有的客户端
 * 参数：发送消息的客户端的套接字描述符
 * 返回值：无
 *
 * */
void recv_and_send_to_all_client(int index)
{
	int i = 0;
	int nbytes = 0;
	char sendbuf[SENDBUFSIZE] = {0};
	time_t sec = 0;
	struct tm * p_time = NULL;

	nbytes = 0;
	nbytes = read(client[index], sendbuf, sizeof(sendbuf)); //接收当前消息
	sendbuf[0] = 'M'; //'M'表示正常聊天信息

	if (nbytes > 0)
	{ //如果接收到消息
		if(index == 0 && strcmp(&sendbuf[50],"t")==0)//管理员踢出用户消息
		{
			write(client[0],"请输入要踢出的昵称:",sizeof(sendbuf));
			read(client[0],sendbuf,sizeof(sendbuf));
			sendbuf[0] = 'M'; //'M'表示正常聊天信息
			for(i = 0;i<max;i++)
			{
		//		printf("kick %s\n",&sendbuf[50]);
				if((strcmp((const char *)name_fd[4+i],&sendbuf[50]))==0)	
				{
					printf("已踢出%s\n",&sendbuf[50]);
					strcpy(&sendbuf[52],"quit");
					write(client[i],sendbuf,sizeof(sendbuf));
					max--;
					n_fd--;

				}


			}
			send_notice_to_all();
		}
		else
		{
			sec = time(NULL);
			p_time = localtime(&sec);
			sprintf(sendbuf + NAMESIZE, " %0d:%0d:%0d %d-%d-%d\n",(p_time->tm_hour), (p_time->tm_min), (p_time->tm_sec), (1900 + p_time->tm_year), (1 +p_time->tm_mon), (p_time->tm_mday)); 
			/*
			   sprintf(sendbuf + NAMESIZE, " %d-%d-%d %0d:%0d:%0d\n", (1900 + p_time->tm_year), (1 +
			   p_time->tm_mon), (p_time->tm_mday), (p_time->tm_hour), (p_time->tm_min), (p_time->tm_sec));
			   */			
			snd_index = 0;
			/*以下循环将消息转发给所有客户端*/
			while (snd_index < max)
			{
				if (write(client[snd_index], sendbuf, sizeof(sendbuf)) == -1)
				{
					fprintf(stderr, "Write Error:%s\n", strerror(errno));
					close(sockfd);
					fclose(fp);
					exit(1);
				}
				snd_index++;
			}

			/*如果某个客户端下线，则删除全局数组 client 中对应的套接字,并通知所有用户当前聊天室人数*/
			if (strcmp(sendbuf + NAMESIZE + TIMESIZE, "QUIT") == 0 || strcmp(sendbuf + NAMESIZE +TIMESIZE, "Quit") == 0
					|| strcmp(sendbuf + NAMESIZE + TIMESIZE, "quit") == 0)
			{
				for (; index < max - 1; index++)
					client[index] = client[index + 1];
				max--;
				send_notice_to_all();
			}
			/*服务器打印出消息内容并保存消息到文件*/
			printf("%s", sendbuf + 1);
			printf("%s", sendbuf + NAMESIZE);
			printf(" %s\n", sendbuf + NAMESIZE + TIMESIZE);
			fprintf(fp, "%s", sendbuf + 1);
			fprintf(fp, "%s", sendbuf + NAMESIZE);
			fprintf(fp, "%s\n", sendbuf + NAMESIZE + TIMESIZE);
		}
	}
}


//通知所有在线客户，当前在线人数
void send_notice_to_all(void)
{
	/*以下循环将消息转发给所有客户端*/
	snd_index = 0;
	char left_clit[30]={'\0'};
	sprintf(left_clit,"N当前聊天室人数:%d",max);
	while (snd_index < max)
	{
		if (write(client[snd_index],left_clit, sizeof(left_clit)) == -1)
		{
			fprintf(stderr, "Write Error:%s\n", strerror(errno));
			close(sockfd);
			fclose(fp);
			exit(1);
		}
		snd_index++;
	}

}
