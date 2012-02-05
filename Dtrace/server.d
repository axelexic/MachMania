#pragma D option flowindent

fbt::mach_msg_overwrite_trap:entry
/execname == "mach_ipc_server"/
{self->t=1;} 

fbt:::
/self->t != NULL && execname == "mach_ipc_server"/
{} 

fbt::mach_msg_overwrite_trap:return
/self->t != NULL && execname == "mach_ipc_server"/
{self->t = NULL;}

