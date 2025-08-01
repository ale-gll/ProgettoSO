#ifndef CROC_PROCESS_H
#define CROC_PROCESS_H


pid_t croc_process(IPCHandles *ipc, Object croc, int stream_index, int stream_crocs_index, int delay, int pid_arr_index);

void handle_croc_kill_signal();

#endif