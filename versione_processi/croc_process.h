#ifndef CROC_PROCESS_H
#define CROC_PROCESS_H

pid_t croc_process(IPCHandles *ipc, Object croc, int stream_index, int stream_objs_index, int timeout);

#endif