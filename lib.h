#ifndef LIB
#define LIB

#define MAXL 0xFA  /* 250 */
#define TIMELIMIT 0x05  /* 5 seconds is the time limit for each package */
#define ZERO 0x00
#define MAXSIZE 250  /* max data size in a MiniKermit package */

#define IMPLICITEOL 0X0D
#define IMPLICITSOH 0x01

#define STYPE 'S'  /* defining each package type */
#define FTYPE 'F'
#define DTYPE 'D'
#define ZTYPE 'Z'
#define BTYPE 'B'

#define YTYPE 'Y'
#define NTYPE 'N'


#define forever while(1)  /* and ever */


typedef struct {
    int len;
    char payload[1400];
} msg;

/* MiniKermit package defined accordingly*/
typedef struct {
    unsigned char soh;
    unsigned char len;
    unsigned char seq;
    char type;
    char data[250];
    unsigned short check;
    unsigned char mark;
} __attribute__((__packed__)) MiniKermit;

/* S package data region, defined accordingly */
typedef struct {
    unsigned char maxl;
    unsigned char time;
    unsigned char npad;
    unsigned char padc;
    unsigned char eol;
    unsigned char qctl;
    unsigned char qbin;
    unsigned char chkt;
    unsigned char rept;
    unsigned char capa;
    unsigned char r;
} __attribute__((__packed__)) SPacketData;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif
