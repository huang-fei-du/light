#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"

typedef void (* CMDFUNC)();
typedef struct _cmdnum
{
  int     cmd;
  CMDFUNC callback;
}CMDNUM,*PCMDNUM;

//结构 － 主站IP地址和端口(AFN04-F3)
typedef struct
{
  unsigned char ipAddr[4];           //主用IP地址
  unsigned char port[2];             //主用端口地址
  unsigned char ipAddrBak[4];        //备用IP地址
  unsigned char portBak[2];          //备用端口地址
  unsigned char apn[16];             //APN
}IP_AND_PORT;

//结构 - 虚拟专网用户名、密码(AFN04-FN16)
typedef struct
{
	unsigned char vpnName[32];         //虚拟专网用户名
	unsigned char vpnPassword[32];     //虚拟专网密码
}VPN;

//结构 - 地址域 F121
typedef struct
{
   unsigned char a1[2];              //行政区划码
   unsigned char a2[2];              //终端地址
   unsigned char a3;                 //主站地址和组地址标志
}ADDR_FIELD;

 //结构 - 终端IP地址和端口(AFN04-07)
 typedef struct
 {
   unsigned char teIpAddr[4];        //终端IP地址
   unsigned char mask[4];            //子网掩码地址
   unsigned char gateWay[4];         //网关地址
   unsigned char proxyType;          //代理类型
   unsigned char proxyServer[4];     //代理服务器地址
   unsigned char proxyPort[2];       //代理服务器端口
   unsigned char proxyLinkType;      //代理服务器连接方式
   unsigned char userNameLen;        //用户名长度m
   unsigned char userName[20];       //用户名
   unsigned char passwordLen;        //密码长度n
   unsigned char password[20];       //密码
   unsigned char listenPort[2];      //终端侦听端口
   unsigned char ethIfLoginMs;       //是否使用以太网登录主站
 }TE_IP_AND_PORT;

unsigned char eth0Ip[4];             //本机IP地址
unsigned char eth0Mask[4];           //本机IP地址掩码
unsigned char gateWay[4];            //默认网关
unsigned char eth0Mac[6];            //以太网MAC

void modifyIp();
void modifyBackupIp();
void modifyPort();
void modifyBackupPort();
void modifyApn();
void modifyAddr();
void showInfo();
void exitWithoutSave();
void exitWithSave();
void parsecmd(int cmd);
int getcmdnum(int cmd);
int selectParameter(unsigned char afn, unsigned char fn, unsigned char *para, unsigned short len);
void modifyEth0Ip();
void modifyEth0Mask();
void modifyGateway();
void modifyEthMac();
void readIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4]);
void modifyVpnUser();
void modifyVpnPass();
void useEthLogin(void);


CMDNUM cmdlist[] = {
 {1, modifyIp},
 {2, modifyBackupIp},
 {3, modifyPort},
 {4, modifyBackupPort},
 {5, modifyApn},
 {6, modifyAddr},
 {7, modifyEthMac},
 {8, modifyEth0Ip},
 {9, modifyEth0Mask},
 {10, modifyGateway},
 {11, modifyVpnUser},
 {12, modifyVpnPass},
 {13, useEthLogin},
 {88, showInfo},
 {98, exitWithoutSave},
 {99, exitWithSave}
};

IP_AND_PORT    ipAndPort;
VPN            vpn;
ADDR_FIELD     addrField;
TE_IP_AND_PORT teIpAndPort;  //终端IP地址和端口(AFN04-FN7)

sqlite3   *sqlite3Db;



int main(int argc, char* argv[])
{
  int cmd;
  int ret = 0;
  
  bzero(&ipAndPort, sizeof(IP_AND_PORT));
  bzero(&addrField, sizeof(ADDR_FIELD));
  
  //open database source
  sqlite3_open("powerData.db", &sqlite3Db);
  
  //查询数据库，取得原始数据
  selectParameter(0x04,   3, (unsigned char *)&ipAndPort, sizeof(IP_AND_PORT));
  selectParameter(0x04, 121, (unsigned char *)&addrField, sizeof(ADDR_FIELD));
 	selectParameter(0x04,  16, (unsigned char *)&vpn, sizeof(VPN));
 	selectParameter(0x04,   7, (unsigned char *)&teIpAndPort, sizeof(TE_IP_AND_PORT)); //F7

  readIpMaskGateway(eth0Ip, eth0Mask, gateWay);

  showInfo();
  
  while(1)
  {
    fflush(stdin);
    printf("\nplease input the number of your choise>");
    scanf("%d", &cmd);
    getchar();
    parsecmd(cmd);
  }
	
	sqlite3_close(sqlite3Db);
  return 0;
}

int selectParameter(unsigned char afn, unsigned char fn, unsigned char *para, unsigned short len)
{
	sqlite3_stmt *stat;	
	char         *sql;	
	unsigned int result;
	
	//查询数据
	sql = "select * from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		memcpy(para, sqlite3_column_blob(stat, 2), len);
		sqlite3_finalize(stat);

		return 1;
	}

	sqlite3_finalize(stat);

	return 0;
}

void saveParameter(unsigned char fn, unsigned char *para, unsigned short len)
{
	sqlite3_stmt *stat;
	char *sql;
	unsigned char afn;
	
	afn = 0x04;
	sql = "delete from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
	
	
	sql = "insert into base_info(acc_afn, acc_fn, acc_data) values(?, ?, ?)";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, para, len, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*
* Modify Server IP.
*/
void modifyIp()
{
	char changeFlg[20];
	char newIp[16];
	
	int i = 0;
	int j = 0;
	
	while(1)
	{
		printf("You want to change server ip? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[i]))
	{
		printf("please input new server ip:");
		scanf("%s", newIp);
		
		bzero(ipAndPort.ipAddr, 4);
		while(newIp[i] != '\0' && i < strlen(newIp))
		{
			while(newIp[i] != '.' && i < strlen(newIp))
			{
				ipAndPort.ipAddr[j] = ipAndPort.ipAddr[j] * 10 + (newIp[i] - '0');
				i++;
			}
			j++;
			i++;
		}
	}
	
	return;
}

/*
* Modify Server Bark IP.
*/
void modifyBackupIp()
{
	char changeFlg[20];
	char newIp[16];
	
	int i = 0;
	int j = 0;
	
	while(1)
	{
		printf("You want to change server backup ip? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new server ip:");
		scanf("%s", newIp);
	
	  bzero(ipAndPort.ipAddrBak, 4);
		while(newIp[i] != '\0' && i < strlen(newIp))
		{
			while(newIp[i] != '.' && i < strlen(newIp))
			{
				ipAndPort.ipAddrBak[j] = ipAndPort.ipAddrBak[j] * 10 + (newIp[i] - '0');
				i++;
			}
			j++;
			i++;
		}
	}
	
	return;
}

/*
* Modify Server Port.
*/
void modifyPort()
{
	char changeFlg[20];
	int newPort;
	
	while(1)
	{
		printf("You want to change server port? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new server port:");
		scanf("%d", &newPort);
		
		ipAndPort.port[0] = newPort & 0xFF;
		ipAndPort.port[1] = (newPort >> 8) & 0xFF;
	}
	
	return;
}

/*
* Modify Server Bark Port.
*/
void modifyBackupPort()
{
	char changeFlg[20];
	int newPort;
	
	while(1)
	{
		printf("You want to change server backup port? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new server port:");
		scanf("%d", &newPort);
		
		ipAndPort.portBak[0] = newPort & 0xFF;
		ipAndPort.portBak[1] = (newPort >> 8) & 0xFF;
	}
	
	return;
}

/*
* Modify Server APN.
*/
void modifyApn()
{
	char changeFlg[20];
	unsigned char tmpapn[16];
	
	int i;
	
	while(1)
	{
		printf("You want to change apn? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new apn:");
		scanf("%s", tmpapn);
		
		bzero(ipAndPort.apn, 16);
		for(i = 0; i < 16; i ++)
		{
			ipAndPort.apn[i] = tmpapn[i];
		}
	}
	
	return;
}

/*
* Modify Terminal Address.
*/
void modifyAddr()
{
	char changeFlg[20];
	char addr[9];
	
	int i, j = 0;
	
	while(1)
	{
		printf("You want to change terminal address? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new terminal address:");
		scanf("%s", addr);
		
		for(i = 0; i < 8; i++)
		{
			if(addr[i] >= 48 && addr[i] <= 57)
			{
				addr[i] = addr[i] - '0';
			}
			else if((addr[i] >= 65 && addr[i] <= 70) || (addr[i] >= 97 && addr[i] <= 102))
			{
				switch(addr[i])
				{
					case 65:
					case 97:
					  addr[i] = 0x0A;
					  break;
					  
					case 66:
					case 98:
					  addr[i] = 0x0B;
					  break;
					  
					case 67:
					case 99:
						addr[i] = 0x0C;
					  break;
					  
					case 68:
					case 100:
						addr[i] = 0x0D;
					  break;
					  
					case 69:
					case 101:
						addr[i] = 0x0E;
					  break;
					  
					case 70:
					case 102:
						addr[i] = 0x0F;
					  break;
				}
			}
			else
			{
				printf("Error Input.\n");
				return;
			}
		}
		
		addrField.a1[0] = addr[2];
		addrField.a1[0] = (addrField.a1[0] << 4) | addr[3];
		addrField.a1[1] = addr[0];
		addrField.a1[1] = (addrField.a1[1] << 4) | addr[1];
		
		addrField.a2[0] = addr[6];
		addrField.a2[0] = (addrField.a2[0] << 4) | addr[7];
		addrField.a2[1] = addr[4];
		addrField.a2[1] = (addrField.a2[1] << 4) | addr[5];
	}
	
	return;
}

/*
* Modify eth0 IP.
*/
void modifyEth0Ip()
{
	char changeFlg[20];
	char newIp[16];
	
	int i = 0;
	int j = 0;
	
	while(1)
	{
		printf("You want to change Eth ip? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[i]))
	{
		printf("please input new eth0 ip:");
		scanf("%s", newIp);
		
		bzero(eth0Ip, 4);
		while(newIp[i] != '\0' && i < strlen(newIp))
		{
			while(newIp[i] != '.' && i < strlen(newIp))
			{
				eth0Ip[j] = eth0Ip[j] * 10 + (newIp[i] - '0');
				i++;
			}
			j++;
			i++;
		}
	}
	
	return;
}

/*
* Modify eth0 Mask.
*/
void modifyEth0Mask()
{
	char changeFlg[20];
	char newIp[16];
	
	int i = 0;
	int j = 0;
	
	while(1)
	{
		printf("You want to change Eth Mask? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[i]))
	{
		printf("please input new eth0 mask:");
		scanf("%s", newIp);
		
		bzero(eth0Mask, 4);
		while(newIp[i] != '\0' && i < strlen(newIp))
		{
			while(newIp[i] != '.' && i < strlen(newIp))
			{
				eth0Mask[j] = eth0Mask[j] * 10 + (newIp[i] - '0');
				i++;
			}
			j++;
			i++;
		}
	}
	
	return;
}

/*
* Modify Gateway.
*/
void modifyGateway()
{
	char changeFlg[20];
	char newIp[16];
	
	int i = 0;
	int j = 0;
	
	while(1)
	{
		printf("You want to change default gateway? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[i]))
	{
		printf("please input new default gateway:");
		scanf("%s", newIp);
		
		bzero(gateWay, 4);
		while(newIp[i] != '\0' && i < strlen(newIp))
		{
			while(newIp[i] != '.' && i < strlen(newIp))
			{
				gateWay[j] = gateWay[j] * 10 + (newIp[i] - '0');
				i++;
			}
			j++;
			i++;
		}
	}
	
	return;
}

/*
* Modify modifyEthMac.
*/
void modifyEthMac()
{
	char changeFlg[20];
	char newIp[16];
	
	int i = 0;
	int j = 0;
	
	while(1)
	{
		printf("You want to change eth mac?[y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[i]))
	{
		printf("please input new eth mac:");
		scanf("%s", newIp);
		
		bzero(eth0Mac, 6);
		while(newIp[i] != '\0' && i < strlen(newIp))
		{
			while(newIp[i] != ':' && i < strlen(newIp))
			{
				eth0Mac[j]<<=4;
				if (newIp[i]>='a' && newIp[i]<='f')
				{
				  eth0Mac[j] += (newIp[i] - 87);
				}
				else
				{
				  if (newIp[i]>='A' && newIp[i]<='F')
				  {
				    eth0Mac[j] += (newIp[i] - 55);
				  }
				  else
				  {
				    eth0Mac[j] += (newIp[i] - '0');
				  }
				}
				i++;
			}
			j++;
			i++;
		}
	}
	
	return;
}

/*
* Modify VPN User Name.
*/
void modifyVpnUser()
{
	char changeFlg[20];
	unsigned char tmpVpnUser[33];
	
	int i;
	
	while(1)
	{
		printf("You want to change Vpn User Name? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new VPN User Name:");
		scanf("%s", tmpVpnUser);
		
		bzero(vpn.vpnName, 32);
		for(i = 0; i < 32; i ++)
		{
			vpn.vpnName[i] = tmpVpnUser[i];
		}
	}
	
	return;
}

/*
* Modify VPN User Name.
*/
void modifyVpnPass()
{
	char changeFlg[20];
	unsigned char tmpVpnPass[33];
	
	int i;
	
	while(1)
	{
		printf("You want to change Vpn Password? [y/n]:");
		scanf("%s", changeFlg);
		if(('Y' == toupper(changeFlg[0]) || 'N' == toupper(changeFlg[0])) && strlen(changeFlg) == 1)
		{
			break;
		}
	}
	
	if('Y' == toupper(changeFlg[0]))
	{
		printf("please input new VPN Password:");
		scanf("%s", tmpVpnPass);
		
		bzero(vpn.vpnPassword, 32);
		for(i = 0; i < 32; i ++)
		{
			vpn.vpnPassword[i] = tmpVpnPass[i];
		}
	}
	
	return;
}

/*
* Modify use eth login mainstation
*/
void useEthLogin(void)
{
	char changeFlg[20];
	unsigned char tmpVpnPass[33];
	
	int i;
	
	while(1)
	{
		printf("You want to use eth login mainstation:? [1-True/0-False]:");
		scanf("%s", changeFlg);
		if('1' == toupper(changeFlg[0]))
		{
			teIpAndPort.ethIfLoginMs = 0x55;
			break;
		}
		else
		{
		  if('0' == toupper(changeFlg[0]))
		  {
			  teIpAndPort.ethIfLoginMs = 0;
			  break;
		  }
		  else
		  {
		  	printf("Input error,no change.\n");
		  	break;
		  }
		}
	}
	
	return;
}

/*
* Show infomation.
*/
void showInfo()
{
	int i;
	int tmpData;
	
  printf("*****************************************************\n");
  printf("* 1:Server IP:%d.%d.%d.%d\n", ipAndPort.ipAddr[0], ipAndPort.ipAddr[1], ipAndPort.ipAddr[2], ipAndPort.ipAddr[3]);
  printf("* 2:Server Backup IP:%d.%d.%d.%d\n", ipAndPort.ipAddrBak[0], ipAndPort.ipAddrBak[1], ipAndPort.ipAddrBak[2], ipAndPort.ipAddrBak[3]);
  
  tmpData = (ipAndPort.port[1] << 8) | ipAndPort.port[0];
  printf("* 3:Server Port:%04d\n", tmpData);
  
  tmpData = (ipAndPort.portBak[1] << 8) | ipAndPort.portBak[0];
  printf("* 4:Server Backup Port:%04d\n", tmpData);
  printf("* 5:APN:%s\n", ipAndPort.apn);
  printf("* 6:Terminal Address:%02X%02X%02X%02X\n", addrField.a1[1], addrField.a1[0], addrField.a2[1], addrField.a2[0]);
  printf("* 7:ETH MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",eth0Mac[0],eth0Mac[1],eth0Mac[2],eth0Mac[3],eth0Mac[4],eth0Mac[5]);
  printf("* 8:ETH IP Address:%d.%d.%d.%d\n",eth0Ip[0],eth0Ip[1],eth0Ip[2],eth0Ip[3]);
  printf("* 9:ETH IP Mask:%d.%d.%d.%d\n",eth0Mask[0],eth0Mask[1],eth0Mask[2],eth0Mask[3]);
  printf("* 10:Default Gateway:%d.%d.%d.%d\n",gateWay[0],gateWay[1],gateWay[2],gateWay[3]);
  printf("* 11:VPN User Name:%s\n", vpn.vpnName);
  printf("* 12:VPN User Password:%s\n", vpn.vpnPassword);
  printf("* 13:Use Eth login Mainstation:%s\n", (teIpAndPort.ethIfLoginMs==0x55)?"True":"False");
  printf("* 88:Show Infomation.\n");
  printf("* 98:Exit Without Save.\n");
  printf("* 99:Exit With Save.\n");
  printf("*****************************************************\n");
  
}

/*
* readIpMaskGateway
*/
void readIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4])
{
  FILE *fpOfRcs;
  int  i;
  unsigned char ipPtr, j;
  char say[100];  

  if ((fpOfRcs=fopen("/etc/init.d/rcS","r+"))==NULL)
  {
  	printf("Can't open rcS\n");  	
  }
  
  //查找IP地址及掩码
  i=0;
  while(i<50)
  {
    i++;
    fgets(say, 100, fpOfRcs);
    //printf("%s\n", say);
    
    if (strstr(say,"ifconfig eth0 hw ether"))
    {
    	ipPtr = 23;
    	
    	for(j=0;j<6;j++)
    	{
    	  if (say[ipPtr]>='a')
    	  {
    	    eth0Mac[j] = (say[ipPtr]-87)<<4;
    	  }
    	  else
    	  {
    	  	 if (say[ipPtr]>='A')
    	  	 {
    	        eth0Mac[j] = (say[ipPtr]-55)<<4;
    	  	 }
    	  	 else
    	  	 {
    	  	 	  eth0Mac[j] = (say[ipPtr]-0x30)<<4;
    	  	 }
    	  }

    	  if (say[ipPtr+1]>='a')
    	  {
    	    eth0Mac[j] |= (say[ipPtr+1]-87);
    	  }
    	  else
    	  {
    	  	 if (say[ipPtr+1]>='A')
    	  	 {
    	        eth0Mac[j] |= (say[ipPtr+1]-55);
    	  	 }
    	  	 else
    	  	 {
    	  	 	  eth0Mac[j] |= (say[ipPtr+1]-0x30);
    	  	 }
    	  }
    	  
    	  ipPtr+=3;
    	}
    }
    
    if (strstr(say,"netmask"))
    {
    	//IP地址
      ipPtr = 14;
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //只有一位数
      	  {
      	     ip[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //只有二位数
        	   {
      	        ip[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        ip[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //只有一位数
      	  {
      	     ip[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //只有二位数
        	   {
      	        ip[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        ip[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    	
    	ipPtr+=8;
    	//IP地址掩码
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //只有一位数
      	  {
      	     mask[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //只有二位数
        	   {
      	        mask[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        mask[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //只有一位数
      	  {
      	     mask[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //只有二位数
        	   {
      	        mask[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        mask[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    }
    
    if (strstr(say,"route add default gw"))
    {
    	//IP地址
      ipPtr = 21;
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //只有一位数
      	  {
      	     gw[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //只有二位数
        	   {
      	        gw[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        gw[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //只有一位数
      	  {
      	     gw[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //只有二位数
        	   {
      	        gw[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        gw[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    	
    	break;
    }
  }
  fclose(fpOfRcs);
}

/*
* saveIpMaskGateway
*/
void saveIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4])
{
  FILE *fpOfRcs;
  int  i, j;
  char say[100];  

  if ((fpOfRcs=fopen("/etc/init.d/rcS","r+"))==NULL)
  {
  	printf("Can't open rcS\n");  	
  }
  else
  {
    i=0;
    while(i<50)
    {
      i++;
      fgets(say, 100, fpOfRcs);
      
      if (strstr(say,"netmask"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3],mask[0],mask[1],mask[2],mask[3]);
        
        for(j=strlen(say); j<52; j++)
        {
        	 strcat(say," ");
        }
        
        fputs(say, fpOfRcs);
  
      	printf("eth0 ip and mask saved!\n");
      }
      
      if (strstr(say,"route add default gw"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"route add default gw %d.%d.%d.%d dev eth0", gw[0], gw[1], gw[2], gw[3]);
        
        for(j=strlen(say); j<43; j++)
        {
        	 strcat(say," ");
        }
        
        fputs(say, fpOfRcs);
  
      	printf("default gateway saved!\n");
      	
      	break;
      }
      
      if (strstr(say,"ifconfig eth0 hw ether"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"ifconfig eth0 hw ether %02x:%02x:%02x:%02x:%02x:%02x ", eth0Mac[0], eth0Mac[1], eth0Mac[2], eth0Mac[3], eth0Mac[4], eth0Mac[5]);
        fputs(say, fpOfRcs);
  
      	printf("eth0 mac saved!\n");
      }
    }
    fclose(fpOfRcs);
  }
}

/*
* Exit With Save Infomation
*/
void exitWithSave()
{
  saveIpMaskGateway(eth0Ip,eth0Mask,gateWay);
  
	saveParameter(  3, (unsigned char *)&ipAndPort, sizeof(IP_AND_PORT));
	saveParameter(121, (unsigned char *)&addrField, sizeof(ADDR_FIELD));
	saveParameter( 16, (unsigned char *)&vpn, sizeof(VPN));
	saveParameter(  7, (unsigned char *)&teIpAndPort, sizeof(TE_IP_AND_PORT));

	sqlite3_close(sqlite3Db);
	exit(0);
}

/*
* Exit Without Save Infomation
*/
void exitWithoutSave()
{
	sqlite3_close(sqlite3Db);
	exit(0);
}

void parsecmd(int cmd)
{
   int num = -1;
   num=getcmdnum(cmd);
   if(num==-1)
   {
     printf("No such commond \n");
     return;
   }
   else
   {
     cmdlist[num].callback();
   }
}

int getcmdnum(int cmd)
{
	int i = 0;
  for(i=0;i<sizeof(cmdlist)/sizeof(CMDNUM);i++)
  {
    if(cmdlist[i].cmd == cmd)
    {
      return i;
    }
  }
  
  return -1;  
}
