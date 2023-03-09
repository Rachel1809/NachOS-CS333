// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"

#define MaxFileLength 32
#define READWRITE 0
#define READONLY 1
#define STD_IN 2
#define STD_OUT 3

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

char *User2System(int virtAddr, int limit)
{
	int i; // chi so index
	int oneChar;
	char *kernelBuf = NULL;
	kernelBuf = new char[limit + 1]; // can cho chuoi terminal
	if (kernelBuf == NULL)
		return kernelBuf;

	memset(kernelBuf, 0, limit + 1);

	for (i = 0; i < limit; i++)
	{
		kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
		kernelBuf[i] = (char)oneChar;
		if (oneChar == 0)
			break;
	}
	return kernelBuf;
}

int System2User(int virtAddr, int len, char *buffer)
{
	if (len < 0)
		return -1;
	if (len == 0)
		return len;
	int i = 0;
	int oneChar = 0;
	do
	{
		oneChar = (int)buffer[i];
		kernel->machine->WriteMem(virtAddr + i, 1, oneChar);
		i++;
	} while (i < len && oneChar != 0);
	return i;
}

void IncreasePC()
{
	/* set previous programm counter (debugging only)*/
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case SyscallException:
		switch (type)
		{
		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;

		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			break;

		case SC_Create:
		{
			// Input: Dia chi tu vung nho user cua ten file
			// Output: -1 = Loi, 0 = Thanh cong
			// Chuc nang: Tao ra file voi tham so la ten file
			int virtAddr;
			char *filename;
			DEBUG(dbgSys, "\nSC_CreateFile call ...");
			DEBUG(dbgSys, "\nReading virtual address of filename");

			virtAddr = kernel->machine->ReadRegister(4);
			DEBUG(dbgSys, "\nReading filename.");

			filename = User2System(virtAddr, MaxFileLength + 1);
			if (sizeof(filename) == 0)
			{
				printf("\nFile name is not valid");
				kernel->machine->WriteRegister(2, -1); // Return -1 to reg R2
				break;
			}

			if (filename == NULL)
			{
				printf("\nNot enough memory in system");
				kernel->machine->WriteRegister(2, -1);
				delete filename;
				break;
			}
			DEBUG(dbgSys, "\nFinish reading filename.");

			if (!kernel->fileSystem->Create(filename, 0))
			{
				printf("\nError create file '%s'", filename);
				kernel->machine->WriteRegister(2, -1);
				delete filename;
				break;
			}

			// Tao file thanh cong
			kernel->machine->WriteRegister(2, 0);
			printf("\nSuccessfully create file '%s' \n\n", filename);

			delete filename;
			break;
		}

		case SC_Open:
		{
			// OpenFileID Open(char *name, int type)
			// Input: arg1: name, arg2: type
			// Output: return OpenFileID if success, else -1
			DEBUG(dbgSys, "\nSC_Open call ...");
			DEBUG(dbgSys, "\nReading virtual address of filename");
			int virtAddr = kernel->machine->ReadRegister(4);
			int type = kernel->machine->ReadRegister(5);
			char *filename;

			DEBUG(dbgSys, "\nReading filename.");

			filename = User2System(virtAddr, MaxFileLength);

			int block = kernel->fileSystem->BlankSpace();
			if (block != -1)
			{

				if (type == READWRITE || type == READONLY) // allow read and readwrite
				{
					kernel->fileSystem->FilePtr[block] = kernel->fileSystem->Open(filename, type);
					if (kernel->fileSystem->FilePtr[block] != NULL) // Open successfully
					{
						printf("\nSuccessfully open file '%s' (Read/ReadWrite) at FileID: %d\n", filename, block);
						kernel->machine->WriteRegister(2, block); // return OpenFileID
					}
					else
					{
						printf("\nError: File '%s' not exist in the directory\n", filename);
						kernel->machine->WriteRegister(2, -1);
					}
				}
				else if (type == STD_IN) // stdin
				{
					printf("\nOpen file stdin\n");
					kernel->machine->WriteRegister(2, 0); // return OpenFileID
				}
				else if (type == STD_OUT) // stdout
				{
					printf("\nOpen file stdout\n");
					kernel->machine->WriteRegister(2, 1); // return OpenFileID
				}
				else
				{
					printf("\nError: Fail open file '%s' type '%d'\n", filename, type);
					kernel->machine->WriteRegister(2, -1);
				}
				for (int i = 0; i < MAX_FILE_OPEN; i++)
				{
					printf("%d\n", kernel->fileSystem->FilePtr[i] == NULL);
				}
				delete[] filename;
				break;
			}
			printf("\nFail open file because over %d file descriptors\n", MAX_FILE_OPEN);

			kernel->machine->WriteRegister(2, -1);

			delete[] filename;
			break;
		}
		case SC_Close:
		{
			// Input id cua file(OpenFileID)
			//  Output: 0: success, -1 fail
			int FileID = kernel->machine->ReadRegister(4); // Read ID from reg 4
			if (FileID >= 0 && FileID < MAX_FILE_OPEN)
			{
				if (kernel->fileSystem->FilePtr[FileID]) // if success
				{
					delete kernel->fileSystem->FilePtr[FileID];
					kernel->fileSystem->FilePtr[FileID] = NULL;
					kernel->machine->WriteRegister(2, 0);
					printf("\nSuccessfully close file at FileID: %d\n", FileID);

					for (int i = 0; i < MAX_FILE_OPEN; i++)
					{
						printf("%d\n", kernel->fileSystem->FilePtr[i] == NULL);
					}
					break;
				}
				else
				{
					printf("\nCannot close file not exist\n");
				}
			}
			else
			{
				printf("\nError: Close file out of %d file descriptors table\n", MAX_FILE_OPEN);
			}
			kernel->machine->WriteRegister(2, -1);
			break;
		}

		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		IncreasePC();
		return;

	// Nhung exception khac thi in ra mot thong bao loi
	case PageFaultException:
		DEBUG(dbgSys, "No valid translation found\n");
		printf("No valid translation found\n");
		SysHalt();
		break;

	case ReadOnlyException:
		DEBUG(dbgSys, "Write attempted to page marked \"read-only\"\n");
		printf("Write attempted to page marked \"read-only\"\n");
		SysHalt();
		break;

	case BusErrorException:
		DEBUG(dbgSys, "Translation resulted in an invalid physical address\n");
		printf("Translation resulted in an invalid physical address\n");
		SysHalt();
		break;

	case AddressErrorException:
		DEBUG(dbgSys, "Unaligned reference or one that was beyond the end of the address space\n");
		printf("Unaligned reference or one that was beyond the end of the address space\n");
		SysHalt();
		break;

	case OverflowException:
		DEBUG(dbgSys, "Integer overflow in add or sub\n");
		printf("Integer overflow in add or sub\n");
		SysHalt();
		break;

	case IllegalInstrException:
		DEBUG(dbgSys, "Unimplemented or reserved instr\n");
		printf("Unimplemented or reserved instr\n");
		SysHalt();
		break;

	case NumExceptionTypes:
		DEBUG(dbgSys, "Number exception types\n");
		printf("Number Exception types\n");
		SysHalt();
		break;

	default:
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	ASSERTNOTREACHED();
}
