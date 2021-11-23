/***************************************************
Copyright,2012,,All	Rights Reserved
文件名:esRtUpdate.c
作者:leiyong
版本:0.9
完成日期:2012年4月
描述:东软路由处理文件
函数列表：
     1.
修改历史：
  01,before 12-03-31,eastSoft created.
  02,12-03-31,Leiyong,将其融入dlzd中

***************************************************/
#include "common.h"

#ifdef PLUG_IN_CARRIER_MODULE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termio.h>
#include <termios.h>
#include <errno.h>
#include <sys/reboot.h>

#include "hardwareConfig.h"
#include "teRunPara.h"
#include "lcdGui.h"
#include "userInterface.h"

#define RT_COM	"/dev/ttyS3"
#define UPDATE_ROUTER_FILE  "/bin/esRtFile"

#define RT_UPD_FRAME_MAX_LEN		        0x300
#define OFFSET_OF(obj_type,mb)          ((int)&(((obj_type*)0)->mb))
#define LITTLE_ENDIAN_BYTE_TO_SHORT(x)  ((*((unsigned char *)(x)+1))<<8 |\
                                         (*((unsigned char *)(x)+0)))

//for old RT(version 002), the protocol frame len segmnet is 1byte;
//but new protocol frame len segment is 2bytes
//#define OLD_RT				                  0x00

//static unsigned char _updating_flag = 0x00, update_step = 0;
//ly,2012-4-1,去掉static
unsigned char _updating_flag = 0x00, update_step = 0;

static int updating_fd = -1;
static int rt_uart_fd =-1;

struct _rt_updating
{
	unsigned short realfile_headsz;
	unsigned char  version[2];
	unsigned char  date[3];
	unsigned short block_cnt;
	unsigned short block_sz;
	int 		       file_sz;

	unsigned short start_block_no;
	unsigned short send_blk_cnt;
};
static struct _rt_updating updating_file;

unsigned char checksum (const unsigned char *data, int len)
{
  unsigned char cs = 0;

  while(len-- > 0)
  {
    cs += *data++;
  }
    
  return(cs);
}

struct strCommConfig
{
  unsigned int baud;
  unsigned int bits;
  char         parity;
};

int _setTerm(int fd, struct strCommConfig comm)
{
  struct termio term_attr,oterm_attr;
  
  if (ioctl(fd, TCGETA, &oterm_attr) < 0)
  {
  	return(-1);
  }
  
  if (ioctl(fd, TCGETA, &term_attr) < 0)
  {
  	return(-1);
  }
	
  term_attr.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | IGNCR | ICRNL | ISTRIP);
  term_attr.c_lflag &= ~(ISIG | ECHO | ICANON | NOFLSH );  //| XCLUDE
  term_attr.c_cflag &= ~CBAUD;

	switch (comm.baud)
  {
    case 300:
      term_attr.c_cflag |= B300;
      break;
    
    case 600:
      term_attr.c_cflag |= B600;
      break;
    
    case 1200:
      term_attr.c_cflag |= B1200;
      break;
    
    case 2400:
      term_attr.c_cflag |= B2400;
      break;
    
    case 4800:
      term_attr.c_cflag |= B4800;
      break;
    
    case 19200:
      term_attr.c_cflag |= B19200;
      break;
    
    case 38400:
      term_attr.c_cflag |= B38400;
      break;
    
    case 57600:
      term_attr.c_cflag |= B57600;
      break;
    
    case 115200:
      term_attr.c_cflag |= B115200;
      break;
    
    case 9600:
    default:
      term_attr.c_cflag |= B9600;
  }
    
  //data bits
  switch (comm.bits)
  {
    case 7:
      term_attr.c_cflag |= CS7;
      break;
      
    case 8:
    default:
      term_attr.c_cflag |= CS8;
  }
  
  //Check bit
  switch (comm.parity)
  {
    case 'o':
    case 'O':
      term_attr.c_cflag |= PARENB;
      term_attr.c_cflag |= PARODD;
      break;
      
    case 'e':
    case 'E':
      term_attr.c_cflag |= PARENB;
      term_attr.c_cflag &= ~PARODD;
      break;
      
    case 'n':
    case 'N':
    default:
      term_attr.c_cflag &= ~PARENB;
  }
  
  //stop bit
  term_attr.c_cflag &=  ~CSTOPB;  //|= CSTOPB;

  term_attr.c_oflag &=~(OPOST | ONLCR | OCRNL);
  term_attr.c_cc[VMIN] = 0;
  term_attr.c_cc[VTIME] = 1;      //the overtime waiting data arrived is 0.5s

  if (ioctl(fd, TCSETAW, &term_attr) < 0)
  {
  	return(-1);
  }
  
  if (ioctl(fd, TCFLSH, 2) < 0)
  {
  	return(-1);
  }

  return(0);
}
 
void my_usleep(int useconds)
{
  struct timespec req_time;
  
  req_time.tv_sec = useconds / 1000000;
  useconds = useconds % 1000000;
  req_time.tv_nsec = useconds * 1000;

  nanosleep(&req_time, NULL);
}

int rt_write_n(const int fd,const void * buf, int len)
{
  int cnt = 0x00, ret,_len = len;

  while (_len > 0)
  {
    ret=write(fd, (const char*)buf+ cnt, _len);
    if (ret<1)
    {
      return(cnt);
    }
    
    cnt+=ret;
    _len -= ret;
    
    if (_len > 0x00)
    {
      my_usleep(100);
    }
  }
  return(cnt);
}

int rt_read_n(int fd, void * buf, int len)
{
  return(read(fd, (char*)buf, len));
}

int init_rt_com(void)
{
	struct strCommConfig    commconfig;
	
	commconfig.baud = 9600;                         //Initialize UART port
	commconfig.bits = 8;
	commconfig.parity = 'e';
	rt_uart_fd = -1;
	rt_uart_fd = open(RT_COM, O_RDWR | O_NONBLOCK);
  if (rt_uart_fd < 0)
  {
		printf("open %s error !\n", RT_COM);fflush(stdout);
    return -1;
  }
  else
  {
  	printf("open %s success!\n", RT_COM);
  }
 
	if (0 != _setTerm(rt_uart_fd, commconfig))
  {
		close(rt_uart_fd);
		rt_uart_fd = -1;
    printf("Initialize %s error !\n", RT_COM);fflush(stdout);
    return -1;
  }
  
	return(rt_uart_fd);
}

void print_debug_array(const unsigned char *tip, const unsigned char s[],int cnt, const unsigned char *fmt)
{
  int j,i,k,n=0x00;
  unsigned char buffer[0x100];

	printf("%s",tip);fflush(stdout);

  while (  cnt > 0x00 )
  {
    k = cnt;
    if ( k > 0x10 )
    {
    	k = 0x10;
    }
    
    for ( j = 0x00,i =0x00 ; j < k ; j++ )
    {
      i += sprintf(&buffer[i],fmt,s[n+j]);
    }
    printf("%s\n",buffer);
    fflush(stdout);

    cnt -= k;
    n += k;
  }
}

#pragma pack(1)
struct rt_update_frame
{
	unsigned char flag;
#if OLD_RT
	unsigned char len;
#else
	unsigned char len[2];
#endif
	unsigned char ctl;
	unsigned char infor_data[6];
	unsigned char afn;
	unsigned char fn[2];
	unsigned char data[1];
};
#pragma pack()

void init_rt_update_frame(struct rt_update_frame *pdata)
{
	const unsigned char head_data[] = {0x68, 0x00,
				#if !OLD_RT
										0x00,
				#endif
										0x47, //ctl
								  0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,//infor_data
								  0x01,0x01,0x00,//afn,fn
								 };
    memcpy((unsigned char *) pdata, head_data, sizeof(head_data));
}

int assemble_get_rt_ver(unsigned char buf[])
{
	int j = 0x00,len;
	unsigned char cs;
	struct rt_update_frame *pframe = (struct rt_update_frame *)buf;
	
	//68 0F 00 41 00 00 00 00 00 00 03 01 00 45 16 
	init_rt_update_frame(pframe);
	pframe->ctl = 0x41;
	memset(pframe->infor_data, 0x00, sizeof(pframe->infor_data));
	pframe->afn = 0x03;

	cs = checksum((const unsigned char*)&(pframe->ctl),  j +
				  OFFSET_OF(struct rt_update_frame, data) - OFFSET_OF(struct rt_update_frame, ctl) );
	pframe->data[j++] = cs;
	pframe->data[j++] = 0x16;

	len = OFFSET_OF(struct rt_update_frame, data) + j;

	pframe->len[0] = len & 0xFF;
	pframe->len[1] = (len >>8) & 0xFF;

	return(len);
}

int assemble_rt_start_update(unsigned char buf[])
{
	int j = 0x00,len;
	unsigned char cs;
	struct rt_update_frame *pframe = (struct rt_update_frame *)buf;

	init_rt_update_frame(pframe);
	pframe->data[j++] = 0x01;
	pframe->data[j++] = updating_file.version[0];//version
	pframe->data[j++] = updating_file.version[1];//version
	pframe->data[j++] = updating_file.block_cnt & 0xFF;//blk_cnt
	pframe->data[j++] = (updating_file.block_cnt >> 8) & 0xFF;//blk_cnt
	pframe->data[j++] = updating_file.block_sz;//blk_sz

	cs = checksum((const unsigned char*)&(pframe->ctl),  j +
				  OFFSET_OF(struct rt_update_frame, data) - OFFSET_OF(struct rt_update_frame, ctl) );
	pframe->data[j++] = cs;
	pframe->data[j++] = 0x16;

	len = OFFSET_OF(struct rt_update_frame, data) + j;

#if OLD_RT
	pframe->len = len & 0xFF;
#else
	pframe->len[0] = len & 0xFF;
	pframe->len[1] = (len >>8) & 0xFF;
#endif
	return(len);
}

int assemble_req_rt_unupd_blks(unsigned char buf[])
{
	int j = 0x00,len;
	unsigned char cs;
	struct rt_update_frame *pframe = (struct rt_update_frame *)buf;

	init_rt_update_frame(pframe);
	pframe->data[j++] = 0x03;
	pframe->data[j++] = updating_file.version[0];//version
	pframe->data[j++] = updating_file.version[1];//version

	cs = checksum((const unsigned char*)&(pframe->ctl),  j +
				  OFFSET_OF(struct rt_update_frame, data) - OFFSET_OF(struct rt_update_frame, ctl) );
	pframe->data[j++] = cs;
	pframe->data[j++] = 0x16;

	len = OFFSET_OF(struct rt_update_frame, data) + j;
#if OLD_RT
	pframe->len = len & 0xFF;
#else
	pframe->len[0] = len & 0xFF;
	pframe->len[1] = (len >>8) & 0xFF;
#endif

	return(len);
}

static int assemble_rt_data(unsigned char buf[])
{
	int j = 0x00,len,read_len;
	unsigned char cs;
	struct rt_update_frame *pframe = (struct rt_update_frame *)buf;

	init_rt_update_frame(pframe);
	pframe->data[j++] = 0x02;

	pframe->data[j++] = updating_file.start_block_no & 0xFF;
	pframe->data[j++] = (updating_file.start_block_no >>8) & 0xFF;
	pframe->data[j++] = updating_file.version[0];//version
	pframe->data[j++] = updating_file.version[1];//version
	pframe->data[j++] = updating_file.block_sz;

	read_len = updating_file.block_sz;
	if(    (updating_file.start_block_no == updating_file.block_cnt)// -1) //the last block
		&& (updating_file.file_sz % updating_file.block_sz >0) )
	{
		read_len = updating_file.file_sz % updating_file.block_sz;
	}

	lseek(updating_fd,
		  updating_file.realfile_headsz + (updating_file.start_block_no -1)* updating_file.block_sz,
		  SEEK_SET);
	read(updating_fd, &pframe->data[j], read_len);

	if (updating_file.block_sz - read_len>0)
	{
		memset(&pframe->data[j + read_len], 0x00, updating_file.block_sz - read_len);
	}
	j+= updating_file.block_sz;
	cs = checksum((const unsigned char*)&(pframe->ctl),  j +
				  OFFSET_OF(struct rt_update_frame, data) - OFFSET_OF(struct rt_update_frame, ctl) );

	pframe->data[j++] = 0x00;//is for special, nothing meanful

	pframe->data[j++] = cs;
	pframe->data[j++] = 0x16;

	len = OFFSET_OF(struct rt_update_frame, data) + j;
#if OLD_RT
	pframe->len = len & 0xFF;
#else
	pframe->len[0] = len & 0xFF;
	pframe->len[1] = (len >>8) & 0xFF;
#endif
	return(len);

}

static int rt_update_get_good_frame(unsigned char data[], int maxlen, int afn)
{
  int len,headsz,frame_start =0;
  struct rt_update_frame *pfrmhead=NULL;
  int data_len = 0;
	
	len = rt_read_n(rt_uart_fd, data, maxlen);
	if (len >0)
	{
		print_debug_array("get a frame from router!!\n",data,len,"%02x ");
	}

	headsz = sizeof(struct rt_update_frame) + 2;

  frame_preprocess_lbl:
  if ( len  < headsz )
	{
		printf("rcv %d ,need  %d, too short\n",len,headsz);fflush(stdout);
		return(-1);
	}
  
  for ( frame_start =0 ; frame_start < len; frame_start++ )
  {
    if (data[frame_start] == 0x68)
    {
    	break;
    }
  }
  
  if ( frame_start == len)
  {
    printf("get here %s %d\n",__FILE__,__LINE__);
    fflush(stdout);
    
    return(-1);
  }

  if ( frame_start >0 )
  {
    printf("move it ,frame_start =%d\n",frame_start);fflush(stdout);
		len -= frame_start;
    memmove(&data[0], &data[frame_start], len);
  }

  frame_start = 0x00;
  if ( len  < headsz )
  {
    printf("too short, get here %s %d\n",__FILE__,__LINE__);
    fflush(stdout);
    return(-1);
  }

	pfrmhead = (struct rt_update_frame *)data;
#if OLD_RT
	data_len = pfrmhead->len;
#else
  data_len = LITTLE_ENDIAN_BYTE_TO_SHORT(pfrmhead->len);
#endif
  if ( data_len > RT_UPD_FRAME_MAX_LEN )
	{
move_data_lbl:
		frame_start = OFFSET_OF(struct rt_update_frame, len);
		len -= frame_start;
		memmove(&data[0], &data[frame_start], len);
		goto frame_preprocess_lbl;
	}

  if (data_len > len)
  {
    printf("get here %s %d\n",__FILE__,__LINE__);
    fflush(stdout);
    return(-1);
  }

  if (0x16 != data[data_len - 1])
  {
  	goto move_data_lbl;
  }

	if ( data[data_len -2] != checksum((const unsigned char*)&(pfrmhead->ctl), data_len - 1 -sizeof(pfrmhead->len)- 2 ) )
  {
    goto move_data_lbl;
  }

	if ( (afn != pfrmhead->afn) || (0x01 != pfrmhead->fn[0])
		 ||(0x00 != pfrmhead->fn[1]) )
	{
    goto move_data_lbl;
  }

  return(data_len);
}

static void finish_updating(int result)
{
	if (0x00 == result)
	{
	   printf("******************************************rt update failed*****************************\n");
	   fflush(stdout);
	}
	else
	{
	   printf("******************************************rt update success*****************************\n");
	   fflush(stdout);
	}

	update_step = 0x00;
	_updating_flag = 0x00;

	if (updating_fd >0)
	{
		close(updating_fd);
		updating_fd = -1;
	}
	
	if (rt_uart_fd >0)
	{
		close(rt_uart_fd);
		rt_uart_fd =-1;
	}
}

void open_router_file()
{
	int trycnt = 0x100;

	while((updating_fd <=  0x00) && (trycnt-- > 0x0))
	{
		updating_fd = open(UPDATE_ROUTER_FILE,O_RDONLY);
	}
	
	if(updating_fd<=  0x00)
	{
		printf(">>>>>>>>>>>>can not open router file<<<<<<<<<<<<\n");
		fflush(stdout);
	}
}
 
//0--normal; 1--overtime; 2--update failed
static int check_over_time(time_t over_time,int *ptry_cnt)
{
	if (over_time >= time(NULL))
	{
		return(0);
  }
	 
	(*ptry_cnt)--;
	if(*ptry_cnt < 0x00)
	{
		finish_updating(0);
		return(2);
	}
	return(1);
}

void updating_rt_process(void)
{
	unsigned char tmpbuff[RT_UPD_FRAME_MAX_LEN];
	int len,cnt,result;
	static time_t next_over_time;
	static int try_cnt = 0x00;
	struct rt_update_frame *pupd_frame;
	char   say[50];

	if(0x00 == update_step)
	{
		try_cnt = 30;
		update_step = 0x01;
		goto init_com_lbl;
	}
	else
	  if(0x01 == update_step)  //第1步打开串口并读取路由当前版本
	{
init_com_lbl:
		if(init_rt_com() <0)
		{
			try_cnt --;
			if (try_cnt <0)
			{
				finish_updating(0);
		  }
		  
			return;
		}
		close(updating_fd);
		updating_fd = -1;
		open_router_file();
		if(updating_fd <0)
		{
			finish_updating(0);
			return;
		}
		try_cnt = 15;
reget_rtver_lbl:
		len = assemble_get_rt_ver(tmpbuff);
		rt_write_n(rt_uart_fd, tmpbuff, len);
    print_debug_array(">>>>send data to router\n", tmpbuff, len, "%02x ");

		update_step = 0x02;
		next_over_time = time(NULL)+ 0x02;
	}
	else if(0x02 == update_step)  //第2步,根据读回的当前路由版本及文件的版本信息判断是否需要升级
	{
		len = rt_update_get_good_frame(tmpbuff, sizeof(tmpbuff), 0x03);
		if(len > 0)
		{
			pupd_frame = (struct rt_update_frame *)tmpbuff;
			if(len >= OFFSET_OF(struct rt_update_frame, data) + 9 + 2 )
			{
				if (0x00 == memcmp(&pupd_frame->data[7], updating_file.version, 2))
				{
					if (0x00 == memcmp(&pupd_frame->data[4], updating_file.date, 3))
					{
						printf("****the router version and date is same, not need to update!!!\n");fflush(stdout);
						finish_updating(0);
          	
          	guiLine(10,55,150,105,0);
            guiLine(10,55,10,105,1);
            guiLine(150,55,150,105,1);
            guiLine(10,55,150,55,1);
            guiLine(10,105,150,105,1);
            guiDisplay(17,72,"版本相同无需升级",1);
            lcdRefresh(55,105);
						
						return;
					}
					updating_file.version[0] ++;
					if (0x00 == updating_file.version[0]) updating_file.version[0]++;
				}
				update_step = 0x03;
				goto re_activate_upd_lbl;
			}
		}
		result = check_over_time(next_over_time, &try_cnt);
		if (0x02 == result) return;
		if (0x01 == result) goto reget_rtver_lbl;
	}
    else if(0x03 == update_step)  //第3步,启动升级
    {
re_activate_upd_lbl:
		len = assemble_rt_start_update(tmpbuff);
		rt_write_n(rt_uart_fd, tmpbuff, len);
        print_debug_array(">>>>send data to router\n", tmpbuff, len, "%02x ");

		update_step = 0x04;
		next_over_time = time(NULL)+ 0x02;
	}
	else if(0x04 == update_step)   //第4步,根据路由返回的信息决定是否继续升级
	{
		if(rt_update_get_good_frame(tmpbuff, sizeof(tmpbuff), 0x01) > 0)
		{
			pupd_frame = (struct rt_update_frame *)tmpbuff;
			if(0x01 == pupd_frame->data[0])
			{
				if(0xFF == pupd_frame->data[3])
				{
					printf("router is not need to update!!!");fflush(stdout);
					finish_updating(0);
					return;
				}
				else if(0x00 == pupd_frame->data[3])
				{
					update_step = 0x10;
					goto req_unupdated_lbl;
			    }
		  }
	   }
		result = check_over_time(next_over_time, &try_cnt);
		if (0x02 == result) return;
		if (0x01 == result) goto re_activate_upd_lbl;
	}
	else if(0x10 == update_step)  //第5步,查询路由上需要传输的块数
	{
  req_unupdated_lbl:
		len = assemble_req_rt_unupd_blks(tmpbuff);
		rt_write_n(rt_uart_fd, tmpbuff, len);
        print_debug_array(">>>>send data to router\n", tmpbuff, len, "%02x ");
		update_step = 0x11;
		next_over_time = time(NULL)+ 0x05;
	}
	else if(0x11 == update_step)  //第6步,计算应该从哪一块开始传送数据
	{
		if(rt_update_get_good_frame(tmpbuff, sizeof(tmpbuff), 0x01) > 0)
		{
			pupd_frame = (struct rt_update_frame *)tmpbuff;
			if(0x03 == pupd_frame->data[0])
			{
				if   (   (0x01 == pupd_frame->data[1])
					   && (0x00 == memcmp(&pupd_frame->data[2], updating_file.version, 2)) )
				{
					printf("router update finish!!!\n");fflush(stdout);
					finish_updating(1);

          //路由程序升级完成,重启
          sleep(2);      
          reboot(RB_AUTOBOOT);

					return;
				}
				if (0x00 == pupd_frame->data[4])
				{
					printf("datablk cnt =0!!!\n");fflush(stdout);
					finish_updating(0);
					
          //路由程序传输完成,重启
          sleep(2);      
          reboot(RB_AUTOBOOT);
          
					return;
				}

				updating_file.start_block_no = LITTLE_ENDIAN_BYTE_TO_SHORT(&pupd_frame->data[5]);
				updating_file.send_blk_cnt = LITTLE_ENDIAN_BYTE_TO_SHORT(&pupd_frame->data[7]);

				if(updating_file.start_block_no > updating_file.block_cnt)
					//(updating_file.start_block_no > updating_file.block_cnt-1)
				{
					printf("start block no: %d > block_cnt:%d!!!\n", updating_file.start_block_no, updating_file.block_cnt);fflush(stdout);
					finish_updating(0);
					return;
				}

				if(updating_file.start_block_no + updating_file.send_blk_cnt -1 > updating_file.block_cnt)//-1
				{
					updating_file.send_blk_cnt = updating_file.block_cnt +1 - updating_file.start_block_no;
				}
				update_step =0x20;
				goto send_data_rt_lbl;
			}
		}
		result = check_over_time(next_over_time, &try_cnt);
		if (0x02 == result) return;
		if (0x01 == result) goto req_unupdated_lbl;
	}
	else if(0x20 == update_step)  //第7步,传送数据
	{
send_data_rt_lbl:

		if(updating_file.send_blk_cnt <=0) goto req_unupdated_lbl;

		cnt = 3;
		while ((updating_file.send_blk_cnt >0) &&(cnt >0))
		{
			len = assemble_rt_data(tmpbuff);
			rt_write_n(rt_uart_fd, tmpbuff, len);

			printf(">>>>send update data to router,len = %d, no = %d, rest count = %d\n",
				   len, updating_file.start_block_no, updating_file.send_blk_cnt-1);fflush(stdout);
			
		 #ifndef CQDL_CSM
			sprintf(say,"RT数据->%d/%d",updating_file.start_block_no,updating_file.start_block_no+updating_file.send_blk_cnt-1);
			showInfo(say);
	   #endif

  		if (updating_file.start_block_no==1)
  		{
      	if (menuInLayer>0)
      	{
      	  guiLine(10,55,150,105,0);
          guiLine(10,55,10,105,1);
          guiLine(150,55,150,105,1);
          guiLine(10,55,150,55,1);
          guiLine(10,105,150,105,1);
          guiDisplay(17,72,"开始传输路由文件",1);
          lcdRefresh(55,105);
        }
  		}
			
			//print_debug_array(tmpbuff, len);
			updating_file.start_block_no ++;
			updating_file.send_blk_cnt -- ;
			cnt --;
		}
		
		if (updating_file.send_blk_cnt==0)
		{
    	if (menuInLayer>0)
    	{
    	  guiLine(10,55,150,105,0);
        guiLine(10,55,10,105,1);
        guiLine(150,55,150,105,1);
        guiLine(10,55,150,55,1);
        guiLine(10,105,150,105,1);
        guiDisplay(17,72,"路由文件传输完成",1);
        lcdRefresh(55,105);
      }
		 
		 #ifndef CQDL_CSM
      showInfo("路由文件传输完成!");
     #endif
		}
	}
}

int get_version(void)
{
  unsigned char temp[0x40],fhead[] = "eastsoft router";

	open_router_file();
	if(updating_fd <0)
	{
		return(-1);
	}
	if (0x40 != read(updating_fd, temp, 0x40))//the newest router file has 64 bytes header
	{
		guiLine(10,55,150,105,0);
    guiLine(10,55,10,105,1);
    guiLine(150,55,150,105,1);
    guiLine(10,55,150,55,1);
    guiLine(10,105,150,105,1);
    guiDisplay(17,72,"读取路由文件错误",1);
    lcdRefresh(55,105);

		printf("read %s error,check it!!!\n",UPDATE_ROUTER_FILE);fflush(stdout);
		return(-1);
	}
	
	if (0x00 != memcmp(temp, fhead, sizeof(fhead)))
	{
	  //printf("%s version is %02x%02x!!!\n", UPDATE_ROUTER_FILE, ver[0], ver[1]);

		//disorder the version
		//updating_file.version[0] = ver[1];
		//updating_file.version[1] = ver[0];
		printf("%s has no version infor!!!\n", UPDATE_ROUTER_FILE);
		fflush(stdout);
		updating_file.version[0] = 0x01;
		updating_file.version[1] = 0x00;		
		memset(updating_file.date, 0x00, sizeof(updating_file.date));
		updating_file.realfile_headsz = 0x00;
		return(0x00);
	}
	else
	{
		printf("%s version is %02x%02x, date is %02x-%02x-%02x\n", 
				  UPDATE_ROUTER_FILE, temp[17],temp[16], temp[20],temp[19],temp[18]);
		fflush(stdout);
		memcpy(updating_file.version, &temp[16], 2);
		memcpy(updating_file.date, &temp[18], 3);
		updating_file.realfile_headsz = 64;
		return(0x00);		 
	}
	
	return(-1);
}


int init_rt_update()
{
	struct stat buf;
	unsigned char temp[30];

	if ((0x00 !=  stat(UPDATE_ROUTER_FILE, &buf)) || (0x00 == (buf.st_mode & S_IFREG)))
	{
		printf("%s do not exists, put the file in the terminal first!!!\n", UPDATE_ROUTER_FILE);
		fflush(stdout);
    
   #ifndef CQDL_CSM
    sprintf((char *)temp,"文件%s不存在!",UPDATE_ROUTER_FILE);
    showInfo((char *)temp);
   #endif
		
		return(-1);
	}

	if (0x00 != get_version())
	{
		close(updating_fd);
		
		return(-1);
	}

	update_step = 0x00;
	_updating_flag = 0x01;

	updating_file.file_sz = buf.st_size - updating_file.realfile_headsz;
	updating_file.block_sz = 200;
	updating_file.block_cnt = updating_file.file_sz / updating_file.block_sz;
	if(updating_file.file_sz % updating_file.block_sz >0)
	{
		updating_file.block_cnt++;
  }
	printf("***************************************rt_update start******************************************\n");
	fflush(stdout);

  guiLine(10,55,150,105,0);
  guiLine(10,55,10,105,1);
  guiLine(150,55,150,105,1);
  guiLine(10,55,150,55,1);
  guiLine(10,105,150,105,1);
  guiDisplay(17,72,"开始升级东软路由",1);
  lcdRefresh(55,105);
	return(0);
}

/*
int main (void)
{
 // unsigned char ver[2];
	
#if OLD_RT
	printf("This is old rt update program:len=1byte\n");
#else
	printf("This is new rt update program:len=2bytes\n");
#endif
  fflush(stdout);

  if (0x00 != init_rt_update())
  {
  	return(0);
  }

  while (_updating_flag)
  {
    updating_rt_process();
    my_usleep(1000000);
  }
  
  return(0x00);
}
*/

/**************************************************
函数名称:threadOfUpRt
功能描述:升级路由线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void *threadOfUpRt(void *arg)
{
  int i=0;
  
  #if OLD_RT
	  printf("This is old rt update program:len=1byte\n");
  #else
	  printf("This is new rt update program:len=2bytes\n");
  #endif
  fflush(stdout);
  
  close(fdOfCarrier);

  if (0x00 != init_rt_update())
  {
    guiLine(10,55,150,105,0);
    guiLine(10,55,10,105,1);
    guiLine(150,55,150,105,1);
    guiLine(10,55,150,55,1);
    guiLine(10,105,150,105,1);
    guiDisplay(25,72,"升级初始化失败!",1);
    lcdRefresh(55,105);

  	goto backMain;
  }

  while (1)
  {
    if (_updating_flag)
    {
      updating_rt_process();
    }
    else
    {
    	break;
    }

    my_usleep(1000000);
  }
  
backMain:
	
	upRtFlag=0;
	
  usleep(500000);
  carrierUartInit();
  
  usleep(500000);
  resetCarrierFlag();
}

#endif    //PLUG_IN_CARRIER_MODULE


