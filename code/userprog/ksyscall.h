/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

void SysHalt()
{
  kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

// int SysOpen(char *fileName, int type)
// {
//   if (type != 0 && type != 1)
//     return -1;

//   int id = kernel->fileSystem->Open(fileName);
//   if (id == -1)
//     return -1;
//   DEBUG(dbgSys, "\nOpened file");
//   return id;
// }

// int SysClose(int id)
// {
//   return kernel->fileSystem->Close(id);
// }

#endif /* ! __USERPROG_KSYSCALL_H__ */
