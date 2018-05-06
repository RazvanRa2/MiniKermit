// Copyright (c), 2018, Razvan Radoi, first of his name
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

/*Method finds out the type of a package */
char getMessageType(msg analysedMessage) {
    MiniKermit miniKermit;  /* first, copy the payload in miniKermit*/
    memcpy(&miniKermit, analysedMessage.payload,
         analysedMessage.len - sizeof(int));
    return miniKermit.type;  /* then, return the type */
}

/* Method returns the seq. no. of a packet*/
char getMessageSeq(msg analysedMessage) {
    MiniKermit miniKermit; /* first, copy the payload in miniKermit*/
    memcpy(&miniKermit, analysedMessage.payload,
        analysedMessage.len - sizeof(int));
    return miniKermit.seq;  /* then, return the seq */
}
/* Method checks if package was "deteriorated" during transfer */
bool valid(msg r) {
    /* first generate the reciever's crc for the recieved package */
    short int localCRC = crc16_ccitt(r.payload,
        sizeof(MiniKermit) - sizeof(char) - sizeof(short int));
    MiniKermit miniKermit;
    memcpy(&miniKermit, r.payload, r.len - sizeof(int));
    /* then, find the crc written inside the package */
    unsigned short messageCRC = miniKermit.check;
    if (localCRC == messageCRC) { /* if they're the same, good */
        return true;
    }
    return false;  /* if they're not, oh well...*/
}

/* Method creates a Y type message */
msg getACKMessage(char seqNo) {
    msg t; /* t will be the ACK message */
    MiniKermit miniKermit; /* auxiliary, used for creating t */

    miniKermit.soh = IMPLICITSOH; /* adding implicit values */
    miniKermit.len = sizeof(MiniKermit) - 2;
    miniKermit.seq = 0;
    miniKermit.type = YTYPE;  /* Y = ack */
    miniKermit.data[0] = '\0'; /* no data here, sir, move along */
    miniKermit.mark = IMPLICITEOL;

    /* the following lines are not necessary to be honest*/
    /* acks and nacks don't get corrupted */
    /* but i felt like including CRCs too */

    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    miniKermit.check = crc16_ccitt(t.payload, /* why do i even do this */
        sizeof(miniKermit) - sizeof(char) - sizeof(short int));
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    t.len = sizeof(int) + sizeof(MiniKermit);

    return t;
}

/* Method creates an N type message */
msg getNACKMessage(char seqNo) {
    /* Method is no different from the previous one */
    /* The only difference is the miniKermit.type */
    msg t;
    MiniKermit miniKermit;

    miniKermit.soh = IMPLICITSOH;
    miniKermit.len = sizeof(MiniKermit) - 2;
    miniKermit.seq = seqNo;
    miniKermit.type = NTYPE;
    miniKermit.data[0] = '\0';
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    miniKermit.check = crc16_ccitt(t.payload,
        sizeof(miniKermit) - sizeof(char) - sizeof(short int));
    miniKermit.mark = IMPLICITEOL;
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));
    t.len = sizeof(int) + sizeof(MiniKermit);

    return t;
}

/* Method returns the special ACK used for confirming S type messages*/
msg getSPacketConfirmation(msg r) {
    msg t; /*t will be the final ack(S)*/
    MiniKermit miniKermit;
    memcpy(&miniKermit, r.payload, r.len - sizeof(int)); /* copying payload */

    miniKermit.type = YTYPE; /* modifying ack */
    memcpy(t.payload, &miniKermit, sizeof(miniKermit));

    return t;  /* returning final answer*/
}

int main(int argc, char** argv) {
    printf("RECIEVER STARTED \n");
    msg r, t; /* r = recieved message, t = nack/ack */
    char messageType; /* auxiliary, used for determining message type */
    char rSeq; /* seq. no. of a recieved message */
    MiniKermit miniKermit;  /* auxiliary */
    int fileID;  /* the file that will be opened */
    char fileName[260] = "recv_"; /* the name of the output file */
    bool sPacketFlag = false; /* flag used to determine ack type */

    init(HOST, PORT);

    while (recv_message(&r)) {  /* for each recieved message */
        if (valid(r)) {  /* first check if it's a valid one */
            messageType = getMessageType(r); /* then, find the type */
            rSeq = getMessageSeq(r); /* find it's seq no */
            switch(messageType) { /* then, by type...*/
                case 'S' :
                    printf("Data transmition starting...\n");
                    sPacketFlag = true; /* we have to send a special ack */
                    break;

                case 'F' :
                    printf("Recieved file name \n");
                    memcpy(&miniKermit, r.payload, r.len - sizeof(int));
                    strcat(fileName, miniKermit.data); /*generate output name*/
                    printf("File name: %s\n", fileName); /*print the file name*/
                    /* create or open, or trunc, by case... */
                    fileID = open(fileName, O_WRONLY | O_CREAT | O_TRUNC);
                    strcpy(fileName, "recv_"); /*reset filename for next file*/
                    break;

                case 'D' :
                    printf("Recieved file content \n");
                    memcpy(&miniKermit, r.payload, sizeof(MiniKermit));
                    /*write the data inside the mini kermit package in file */
                    write(fileID, miniKermit.data, miniKermit.len);
                    break;

                case 'Z' :
                    printf("File transfer has ended! Closing output file...\n");
                    /* all file data has been transfered => close the file */
                    close(fileID);
                    break;

                case 'B' :
                    /* Nothing to do, just show a message */
                    printf("Data transmition has ended!\n");
                    break;

                default:
                    /*Error message case, package name is wrong*/
                    /*Or there's a highly unlikely crc dishonest match*/
                    perror("Package type error\n");
                    return -1;
            }

            if (sPacketFlag) { /*if we have to get an ACK for an S package */
                t = getSPacketConfirmation(r); /* do that */
                sPacketFlag = false;  /* then reset the flag */
            } else {
                t = getACKMessage(rSeq); /* else, get a generic ack message */
            }
        } else { /* the recieved message is invalid [!(valid(r))] */
            t = getNACKMessage(rSeq); /* we have to get a nack message */
        }
        send_message(&t);  /* send the ack/nack for each recieved package */
    }

	return 0;
}
