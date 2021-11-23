#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
  int  pid;
	int  fd, len;
  char buffer[4096], *p, *q, pName[64];
  unsigned char ifFound = 0;
  unsigned char ifFoundDialer = 0;
  
  //2012-07-25,10���ʼ���,��Ϊ�ڲ���ʱ����,��ʱ���豸�������ݳ���
  sleep(10);

  while(1)
  {
  	ifFound = 0;
  	ifFoundDialer = 0;
  	
  	for(pid=790; pid<4096; pid++)
    {
	    sprintf(buffer, "/proc/%d/stat", pid);
	    fd=open(buffer, O_RDONLY);
	    
	    len = read(fd, buffer, sizeof(buffer)-1);
      if (len<0)
      {
      	continue;
      }
	    close(fd);
	    buffer[len] = '\0';

      p = buffer;
      p = strchr(p, '(')+1;
	    q = strrchr(p, ')');
	    len = q-p;
	    if (len>64)
	    {
	    	len = 64;
	    }
	    memcpy(pName, p, len);
	    pName[len] = 0;
	    
	    //printf("this process %d name is:%s\n", pid,pName);
	    
	    if (strcmp(pName,"dlzd")==0 || strcmp(pName,"DLZD")==0)
	    {
	    	ifFound = 1;
	    }

	    if (strcmp(pName,"modemDialer")==0 || strcmp(pName,"MODEMDIALER")==0)
	    {
	    	ifFoundDialer = 1;
	    }
	    
	    if (ifFound==1 && ifFoundDialer==1)
	    {
	    	break;
	    }
    }
    
    if (ifFound==0)
    {
      printf("processWatch����dlzd");
    	
    	if (system("dlzd -r &")==0)
    	{
    		printf("�ɹ�\n");
      }
      else
      {
    		printf("ʧ��\n");
      }
    }

    if (ifFoundDialer==0)
    {
      printf("processWatch����modemDialer");
    	
    	if (system("modemDialer &")==0)
    	{
    		printf("�ɹ�\n");
      }
      else
      {
    		printf("ʧ��\n");
      }
    }
    
    sleep(5);
  }
  
  return 0;
}


