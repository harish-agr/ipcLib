#include "ipc.h"


/// /////////////////// PRIVATES
char*   _inboxName;
bool    _isRunning;
key_t   _msgQueueKey;
int     _msgQueue;

ipcMsg *_commandBuffer, *_urgentBuffer, *_replyBuffer;

bool _commandLock;
bool _urgentLock;
bool _replyLock;

const int _PROJECT_KEY = 1337;

/// ///////// THREADS

pthread_t _t_inbox;



/// /////////////////// PRIVATE FUNCTIONS

/// //////// THREADS
/**
This is the main inbox-thread.
It loops over the three msg-priorties seperately to avoid urgent messages to get blocked by ordinary commands.

At the moment, it is just one thread, i _think_ it should be three? at the moment the execution of a command WILL
actually block the thread(?).

This might be okay in cases where the execution is done almost immediately (dubin).
but might fail in other cases (motor executing a whole trajectory-command).

TODO: changed this, before someone finds it ^^


*/
void* _ipcInbox(void* arg)
{
    while(_isRunning == true)
    {
        usleep(1000);
        // only read, if the commandlock is removed.
        // the user is sepsonible to remove it.
        if(_commandLock == false)
        {
            //read the msg from the msgQueue and write it to the buffer
            int result = msgrcv(_msgQueue, _commandBuffer, sizeof(ipcMsg)-sizeof(long), COMMAND, IPC_NOWAIT);
            if(result != -1)
            {
                ipcLockCommandBuffer();
                onCommand(_commandBuffer);
            }
            else
            {
                if(errno != ENOMSG)
                    printf("%s: command error code %d\n", _inboxName, errno);
                    fflush(stdout);
            }
        }
        // just like the commandqueue
        if(!_urgentLock)
        {
            int result = msgrcv(_msgQueue, _urgentBuffer, sizeof(ipcMsg)-sizeof(long), URGENT, IPC_NOWAIT);
            if(result != -1)
            {
                ipcLockUrgentBuffer();
                onUrgent(_urgentBuffer);
            }
            else
            {
                if(errno != ENOMSG)
                    printf("%s: urgent error code %d\n", _inboxName, errno);
                    fflush(stdout);
            }

        }

        //just like the command queue
        if(!_replyLock)
        {
            int result = msgrcv(_msgQueue, _replyBuffer, sizeof(ipcMsg)-sizeof(long), REPLY, IPC_NOWAIT);
            if(result != -1)
            {
                ipcLockReplyBuffer();
                onReply(_replyBuffer);
            }
            else
            {
                if(errno != ENOMSG)
                    printf("%s: reply error code %d\n", _inboxName, errno);
                    fflush(stdout);
            }

        }

    }
    return NULL;
}

/// //////////////////// PUBLICS

/**
    Call this function at the beginning of your programm whith a string containing the name of this processes inbox.
    This name MUST be unique through all your processes.
    All messages sent to this string-name will be sent to this process.

*/
int ipcInit(char* inboxname)
{
    _inboxName = inboxname;
    _isRunning = true;

	ipcUnlockCommandBuffer();
	ipcUnlockReplyBuffer();
	ipcUnlockUrgentBuffer();

    char file[80];
    strcpy(file, IPC_ROOT);
    strcat(file, _inboxName);
    printf("%s\n", file);
    _msgQueueKey = ftok(file, _PROJECT_KEY);
    _msgQueue = msgget(_msgQueueKey, 0644 | IPC_CREAT);

    printf("%s: my key %d, my queue %d\n", _inboxName, _msgQueueKey, _msgQueue);
    fflush(stdout);
    if (pthread_create(&_t_inbox, NULL, _ipcInbox, NULL) != 0)
	{
		printf("%s: Error: can't init inbox\n", _inboxName);
		fflush(stdout);
		exit(EXIT_FAILURE);
	}

    return 1;
}

int ipcLockCommandBuffer()
{
    _commandLock = true;
    return 1;
}

int ipcLockReplyBuffer()
{
    _replyLock = true;
    return 1;
}

int ipcLockUrgentBuffer()
{
    _urgentLock = true;
    return 1;
}

int ipcUnlockCommandBuffer()
{
    free(_commandBuffer);
    _commandBuffer = (ipcMsg*)malloc(sizeof(ipcMsg));
    _commandLock = false;
    return 1;
}

int ipcUnlockReplyBuffer()
{
    free(_replyBuffer);
    _replyBuffer = (ipcMsg*)malloc(sizeof(ipcMsg));
    _replyLock = false;
    return 1;
}

int ipcUnlockUrgentBuffer()
{
    free(_urgentBuffer);
    _urgentBuffer = (ipcMsg*)malloc(sizeof(ipcMsg));
    _urgentLock = false;
    return 1;
}

ipcMsg* ipcGetCommandBuffer()
{
    return _commandBuffer;
}

ipcMsg* ipcGetReplyBuffer()
{
    return _replyBuffer;
}

ipcMsg* ipcGetUrgentBuffer()
{
    return _urgentBuffer;
}




int ipcCreateCommand(char* recipient, int command, int id, void* data, int size)
{
    ipcMsg msg;
    msg.id = id;
    msg.msgType = command;
    msg.mtype = COMMAND;
    memcpy(msg.data, data, size);
    strcpy(msg.recipient, recipient);
    strcpy(msg.sender, _inboxName);
    ipcSendMsg(&msg);
    return 1;
}


int ipcSendMsg(ipcMsg* msg)
{
    char file[80];
    strcpy(file, IPC_ROOT);
    strcat(file, msg->recipient);
    key_t msgQueueKey = ftok(file, _PROJECT_KEY);


    int msgQueue = msgget(msgQueueKey, 0644 | IPC_CREAT);

    int result = msgsnd(msgQueue, (void*)msg, sizeof(ipcMsg)-sizeof(long), IPC_NOWAIT);
    //printf("%s: sending to key %d, queue %d with resultcode %d\n", _inboxName, msgQueueKey, msgQueue, result);
    return result;
}

int ipcReply(ipcMsg* command, int type)
{
    ipcMsg reply;
    reply.id = command->id;
    reply.msgType = type;
    reply.mtype = REPLY;
    strcpy(reply.recipient, command->sender);
    strcpy(reply.sender, command->recipient);
    ipcSendMsg(&reply);

    return 1;
}

int ipcReplyData(ipcMsg* command, int type, void* data, int size)
{
    ipcMsg reply;
    reply.id = command->id;
    reply.msgType = type;
    reply.mtype = REPLY;
    memcpy(reply.data, data, size);
    strcpy(reply.recipient, command->sender);
    strcpy(reply.sender, command->recipient);
    ipcSendMsg(&reply);

    return 1;

}


int ipcQuit()
{
    _isRunning = false;
    //pthread_join(_t_inbox, NULL);
    msgctl(_msgQueue, IPC_RMID, (struct msqid_ds *) NULL);
    return 1;
}
