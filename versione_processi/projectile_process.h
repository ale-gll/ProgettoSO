
#ifndef PROJECTILE_PROCESS
#define PROJECTILE_PROCESS

#define PROJECTILE_PROCESS_COOLDOWN 55000 

pid_t proj_process(IPCHandles *ipc, Object proj, int stream_index);

#endif