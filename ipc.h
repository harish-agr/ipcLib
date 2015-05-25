/*

ipc.h

Author: Steffen Peleikis
Version: 0.1


This library acts as a event-driven plug'n'play ipc-lib.
It enables your processes with a messageinbox with 3 different queues.

Commands: These messages are, like the title says, simple commands, sent from one process to another.
Replies: These messages are just replies to commands: like ACK on receive, DONE when done etc.
Urgent: The messages won't be blocked by other messages. Use this messagetype if you want the command be executed directly,
without getting queued in the commandlist.

The normal procedure for this project is to act on one (and only one) command at the time.
This means, that your Queues are blocked, until you manually unlock them.

Take a look at the motor.c/.h - process for a simple example.

Normaly, you want to act on commands, reply to them, send other commands and act on the replies.
Most of the urgent-messages are system-messages like PING etc.
Use urgent message STOP to stop the car from driving.

usage:
------

- include this file into your main
- implement the callback-functions onCommand, onReply, onUrgent.
- call ipcInit(char* name) at the beginning, to init you inboxlisteners.
- call ipcQuit() at the end of your programm.
- In between these calls, implement your processec mainloop, to prevent the process from stopping.

*/


#ifndef IPC_H_INCLUDED
#define IPC_H_INCLUDED

#ifndef IPC_ROOT
#define IPC_ROOT "/home/pi/dev/rover/" /// TODO: make this really usable. atm only this value is working, idk why.
#endif // IPC_ROOT

#include <stdbool.h>
#include <pthread.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <time.h>
#include <unistd.h>

/// ////////////////////////// ENUMS

typedef enum{
    URGENT = 1,     // urgent messages that should be executed immediately.
    REPLY,          // replies to commands.
    COMMAND,        // commands itself.
}ipcType;



/// ////////////////////////// STRUCTS
typedef struct {
    long mtype;
    int msgType;
    int id;
    char data [128];
    char sender[32];
    char recipient[32];
}ipcMsg;


/// ///////////////////////// FUNCTION DEFS TO BE USED FROM USING PROCESS


/**
    Call this function at the beginning of your programm whith a string containing the name of this processes inbox.
    This name MUST be unique through all your processes.
    All messages sent to this string-name will be sent to this process.
*/
int ipcInit(char* inboxname);


/**
    Use this function to create and send a command to a given process with a given id.

    Params:
    char* recipient: A string containing the name of the recipients inbox (set with ipcInit()).
    int command: The type of command you want to send. For readabillity, define a enum for this value.
    void* data: The data you want to send. Restricted to 128b at the moment. Change in the struct above if needed)
    int size: the size of the data-object.
*/
int ipcCreateCommand(char* recipient, int command, int id, void* data, int size);

/**
    Use this to reply to a previously received message, if no further data than a plain type is needed.

    Params:
    ipcMsg* msg: the previusly received message.
    int type: the reply you want to send. Define a enum to make it readable (eg: ACK, REJ etc).
*/
int ipcReply(ipcMsg* msg, int type);

/**
    Use this to reply to a previously received message, if additional data is needed for the reply.

    Params:
    ipcMsg* msg: the previusly received message.
    int type: the reply you want to send. Define a enum to make it readable (eg: ACK, REJ etc).
    void* data: The data you want to send. Restricted to 128b at the moment. Change in the struct above if needed)
    int size: the size of the data-object.
*/
int ipcReplyData(ipcMsg* msg, int type, void* data, int size);

/**
    Sends a handcrafted message.

    ipcMsg* msg: Pointer to the message you want to sent.
*/
int ipcSendMsg(ipcMsg* msg);



/**
    Returns the command currently stored in the commandBuffer.
*/
ipcMsg* ipcGetCommandBuffer();

/**
    Returns the reply currently stored in the replyBuffer.
*/
ipcMsg* ipcGetReplyBuffer();

/**
    Returns the urgent command currently stored in the urgent-commandBuffer.
*/
ipcMsg* ipcGetUrgentBuffer();


/**
locks the command buffer.
is done automatically on msgreceive atm.
once a buffer is locked, no more messages will be received til it gets unlocked.
*/
int ipcLockCommandBuffer();

/**
locks the reply buffer.
is done automatically on msgreceive atm.
once a buffer is locked, no more messages will be received til it gets unlocked.
*/
int ipcLockReplyBuffer();


/**
locks the urgent buffer.
is done automatically on msgreceive atm.
once a buffer is locked, no more messages will be received til it gets unlocked.
*/
int ipcLockUrgentBuffer();

/**
Unlocks the locked commandbuffer.
unlock it to get your next message. the previous message will get overwritten.
make a backup if you want to hold it.
*/
int ipcUnlockCommandBuffer();

/**
Unlocks the locked replybuffer.
unlock it to get your next message. the previous message will get overwritten.
make a backup if you want to hold it.
*/
int ipcUnlockReplyBuffer();

/**
Unlocks the locked urgentbuffer.
unlock it to get your next message. the previous message will get overwritten.
make a backup if you want to hold it.
*/
int ipcUnlockUrgentBuffer();


/**
has to be called at the end of your process to securely close the message queue.
*/
int ipcQuit();

/// //////////////////////// HAS TO BE IMPLEMENTED IN USING PROCESS


/**
This is the Callbackmethod for all incoming commands.
implement it in your code to make it work.

Params:
ipcMsg* command: You will get the pointer to the current command.
*/
int onCommand(ipcMsg* command);

/**
This is the Callbackmethod for all incoming replies.
implement it in your code to make it work.

Params:
ipcMsg* reply: You will get the pointer to the current reply.
*/
int onReply(ipcMsg* reply);

/**
This is the Callbackmethod for all incoming urgentcommnds.
implement it in your code to make it work.

Params:
ipcMsg* urgent: You will get the pointer to the current urgent command.
*/
int onUrgent(ipcMsg* urgent);





/// //////////////////////// FIELDS




#endif // IPC_H_INCLUDED
