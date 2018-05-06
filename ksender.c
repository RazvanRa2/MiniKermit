// Copyright (c), 2018, Razvan Radoi, first of his name
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

#define SENTCODE 1  /* package has been sent succesfully */
#define UNSENTCODE 0  /* package could not be sent and never will be */
#define RESENDCODE 2  /* package fas NACKed and has to be resent with new seq */
#define NEUTRALCODE -1 /* generic reset value */

#define MAXSEQ 64 /* maximum value for the sequence number */

#define ERROR -1  /* defined for the UNSENTCODE case */

/* Method returns an S type package*/
msg getSendInitMessage() {
    msg t; /* t will be the returned message*/

    MiniKermit miniKermit;  /* miniKermit data structres holds the package*/
    SPacketData dataS; /*auxiliary data structure for S type packets*/

    dataS.maxl = MAXL;  /* assigning implicit values */
    dataS.time = TIMELIMIT;
    dataS.npad = ZERO;
    dataS.padc = ZERO;
    dataS.eol = IMPLICITEOL;
    dataS.qctl = ZERO;
    dataS.qbin = ZERO;
    dataS.chkt = ZERO;
    dataS.rept = ZERO;
    dataS.capa = ZERO;
    dataS.r = ZERO;

    memcpy(miniKermit.data, &dataS, sizeof(SPacketData));  /* creating MK */
    miniKermit.soh = IMPLICITSOH;
    miniKermit.len = sizeof(MiniKermit) - 2;
    miniKermit.seq = ZERO;
    miniKermit.type = STYPE;
    miniKermit.mark = IMPLICITEOL;  /* mark = eol = 0x0D */

    t.len = sizeof(int) + sizeof(MiniKermit);
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));  /* premature payload */

    miniKermit.check = crc16_ccitt(t.payload,
        sizeof(miniKermit) - sizeof(char) - sizeof(short int)); /* find crc */

    memcpy(t.payload, &miniKermit, sizeof(miniKermit)); /* add crc to payload */
    return t;  /* return final package */
}

/* Method returns an F type package */
msg getFileNameMessage(char * name, int seqNo) {
    msg t;  /* t will be the returned message */
    MiniKermit miniKermit;  /* auxiliary data structure helps create t */

    miniKermit.soh = IMPLICITSOH;  /* adding implicit values */
    miniKermit.len = sizeof(MiniKermit) - 2;
    miniKermit.seq = seqNo;
    miniKermit.type = FTYPE;
    miniKermit.mark = IMPLICITEOL;

    memcpy(miniKermit.data, name, strlen(name) + 1);  /* copying file name */
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    t.len = sizeof(int) + sizeof(miniKermit);
    miniKermit.check = crc16_ccitt(t.payload, /* creating crc from fake t */
        sizeof(miniKermit) - sizeof(char) - sizeof(short int));
    memcpy(t.payload, &miniKermit, sizeof(miniKermit)); /* final message t */

    return t;
}

/* Method returns a Z type package */
msg getEOFMessage(int seqNo) {
    msg t;  /* t will be the returned message */
    MiniKermit miniKermit;  /* miniKermit helps create t */

    miniKermit.soh = IMPLICITSOH; /* adding implicit values */
    miniKermit.len = sizeof(MiniKermit) - 2;
    miniKermit.seq = seqNo;
    miniKermit.type = ZTYPE;
    miniKermit.data[0] = '\0';
    miniKermit.mark = IMPLICITEOL;

    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    miniKermit.check = crc16_ccitt(t.payload,/* generate crc from incomplete t*/
        sizeof(miniKermit) - sizeof(char) - sizeof(short int));
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));

    return t;  /* return final package */
}

/* Method returns B type package */
msg getEOTMessage(int seqNo) {
    /* Method is no different from eof, packages only differ by (MK)"type" */
    msg t;
    MiniKermit miniKermit;

    miniKermit.soh = IMPLICITSOH;
    miniKermit.len = sizeof(MiniKermit) - 2;
    miniKermit.seq = seqNo;
    miniKermit.type = BTYPE;
    miniKermit.data[0] = '\0';
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    miniKermit.check = crc16_ccitt(t.payload,
        sizeof(miniKermit) - sizeof(char) - sizeof(short int));
    miniKermit.mark = IMPLICITEOL;
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));

    return t;
}
/* Method returns a D type package *wink* */
msg getDataMessage(int seqNo, char readBuffer[MAXSIZE], int size) {
    msg t;  /* t will be the returned message */
    MiniKermit miniKermit;  /* miniKermit helps create t */

    miniKermit.soh = IMPLICITSOH; /* adding implicit values */
    miniKermit.len = size;
    miniKermit.seq = seqNo;
    miniKermit.type = DTYPE;
    memcpy(miniKermit.data, readBuffer, MAXSIZE); /*adding filename in mk.data*/
    miniKermit.mark = IMPLICITEOL;

    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    t.len = sizeof(int) + sizeof(miniKermit);
    miniKermit.check = crc16_ccitt(t.payload,  /*determining crc*/
        sizeof(miniKermit) - sizeof(char) - sizeof(short int));
    memcpy(t.payload, &miniKermit, sizeof(miniKermit)); /* final package */

    return t;
}
/* Method checks if a package is an ACK*/
bool isACK(msg *y) {
    char * charPtr = (char *) y; /*first byre of package*/
    if (*(charPtr + 7) == YTYPE) { /* sizeof(int)(len) + 3 * sizeof(char)(MK) */
        return true; /* charPtr now points to miniKermit.type*/
    } /*if type is Y (ACK), return true*/
    return false; /* else, return false */
}

/* Method represents the sender's routine for awaiting package confirmation*/
int awaitConfirmation(msg t) {
    int errorCnt = 0;

    do {
        msg *y = receive_message_timeout(5000); /* await ack/nack*/
        if (y == NULL) {  /*if nothing gets here, listen again*/
            errorCnt++;
            if (errorCnt == 3) {  /* if it happens 3 times tho*/
                break; /* send the unsent code*/
            }
            send_message(&t); /* resend message, if that is the case */
        } else {
            if (isACK(y)) { /* if we recieved an ack */
                return SENTCODE; /* confirm */
            } else {
                return RESENDCODE; /* if it's a NACK, resend */
            }
        }
    } forever;

    return UNSENTCODE; /* ack/nack will never be recieved -> end transmition */
}

int main(int argc, char** argv) {
    msg t;  /* t is a general structure for messages to get put in */
    int filesToBeSent = argc - 1;  /* number of files that need to be sent */
    int currentSentSeqNo = 0;  /* current package sequence number */
    int packageRecieved = NEUTRALCODE;  /* please reffer to defines for codes */
    char readBuffer[250];  /* utilitary buffer used for file reading */

    init(HOST, PORT);

    do {
        t = getSendInitMessage(); /* send initial package */
        currentSentSeqNo = (currentSentSeqNo + 1) % MAXSEQ;  /* inc seq no */
        send_message(&t);  /* send S package */
        packageRecieved = awaitConfirmation(t);  /* await confirmation */
        printf("packageRecieved: %d\n", packageRecieved);
    }   while(packageRecieved == RESENDCODE);
    if (packageRecieved == UNSENTCODE) {  /* if we can't get an ack/nack */
        return ERROR;  /* we immidiately end the transmition */
    }
    packageRecieved = NEUTRALCODE;  /* reset recieved code to neutral */

    /* for each file that has to be sent */
    for(int fileCnt = 1; fileCnt <= filesToBeSent; fileCnt++) {
        do { /* send the package's name*/
            t = getFileNameMessage(argv[fileCnt], currentSentSeqNo);
            currentSentSeqNo = (currentSentSeqNo + 1) % MAXSEQ;
            send_message(&t);
            packageRecieved = awaitConfirmation(t);
        }   while(packageRecieved == RESENDCODE);
        if (packageRecieved == UNSENTCODE) {
            return ERROR;
        }
        packageRecieved = NEUTRALCODE;

        int fileID = open(argv[fileCnt], O_RDONLY); /* open the file */
        int contentSize = 0;
        while ((contentSize = read(fileID, readBuffer,  MAXSIZE)) > 0) {
            do {    /* send file data in chunks of data*/
                t = getDataMessage(currentSentSeqNo, readBuffer, contentSize);
                currentSentSeqNo = (currentSentSeqNo + 1) % MAXSEQ;
                send_message(&t);
                packageRecieved = awaitConfirmation(t);
            }   while(packageRecieved == RESENDCODE);

            if (packageRecieved == UNSENTCODE) {
                return ERROR;
            }
            packageRecieved = NEUTRALCODE;
        }
        /* at this point, the whole file has been transmited */
        /* we have to send the eof package */
        do {  /*sending the eof package */
            t = getEOFMessage(currentSentSeqNo);
            currentSentSeqNo = (currentSentSeqNo + 1) % MAXSEQ;
            send_message(&t);
            packageRecieved = awaitConfirmation(t);
        } while(packageRecieved == RESENDCODE);
        if (packageRecieved == UNSENTCODE) {
            return ERROR;
        }
        packageRecieved = NEUTRALCODE;
    }
    /* at this point, all files have been transfered */
    /* we have to send the end of transmition package */
    do {
        t = getEOTMessage(currentSentSeqNo); /* sending B type package */
        currentSentSeqNo = (currentSentSeqNo + 1) % MAXSEQ;
        send_message(&t);
        packageRecieved = awaitConfirmation(t);
    } while (packageRecieved == RESENDCODE);
    if (packageRecieved == UNSENTCODE) {
        return ERROR;
    }

    return 0;  /* Pfew, all done. Commenting your code is hard work. */
}
