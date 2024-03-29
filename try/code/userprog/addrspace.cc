// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;
    unsigned int wBytes,rBytes;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
//-------------------------------------------------------------
    if(machine->swap == NULL)
    {
        printf("Unable to open swapfile \n");
        return;
    }
    NoffHeader swapNoffH = noffH;
    wBytes = machine->swap->WriteAt((char *)&swapNoffH, sizeof(noffH), machine->lastVpn*PageSize);

    DEBUG('a', "Write the first %d bytes  back to swap \n",wBytes);
    //-------------------------------------------------------------------------------
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
//when swap file is used , annotate this
    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory
//using swap file to store the prog pages
    realNumPages = numPages;
    if(numPages> NumPhysPages)numPages = NumPhysPages;


    DEBUG('a',"realNumPages is %d , ***PageSize is %d\n",realNumPages,PageSize);
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	//pageTable[i].physicalPage = i;

    pageTable[i].physicalPage = bitmap->Find(); ///use bitmap to allocate the phisical page
    if(i == 0) firstPhysicalPage = pageTable[i].physicalPage ;   // get the first Physical Page 
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }

//-----------------------------------------------------
    // copy noffH data from original file to swap file //   
    int swapOffset = 0;
    do
    {
        rBytes = executable->ReadAt((char *)&swapNoffH, sizeof(noffH),swapOffset);
        wBytes = machine->swap->WriteAt((char *)&swapNoffH, rBytes,
                   machine->lastVpn*PageSize+swapOffset);
    DEBUG('a', "Write %d bytes noffH.code back to swap file\n", wBytes);
    swapOffset += wBytes;
    }while(wBytes == sizeof(noffH));

    DEBUG('a',"The Size of swapfile is %d\n",machine->swap->Length());

    //----------------------------------------------------------------
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    if(firstPhysicalPage == 0) // only clear the memory when  first load a program
    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory

/*
//-----this is the original part--------------------------------------------------------------------------
    int codeVirtualAddr = noffH.code.virtualAddr + firstPhysicalPage*PageSize ;
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        //executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
        executable->ReadAt(&(machine->mainMemory[codeVirtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }

    int initDataVirtualAddr = noffH.initData.virtualAddr + firstPhysicalPage*PageSize;
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
       //executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
             executable->ReadAt(&(machine->mainMemory[initDataVirtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
//-----------------------------------------------------------------------------------------------------
*/

    
#ifdef USE_SWAP
    //--this is the segmentation fault new part-----------------------------------------------------------------------------
     int codeVirtualAddr = noffH.code.virtualAddr+machine->lastVpn*PageSize;
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
            noffH.code.virtualAddr, noffH.code.size);
        machine->noffCodeOffset = noffH.code.virtualAddr - noffH.code.inFileAddr;
        rBytes = machine->swap->ReadAt(&(machine->mainMemory[codeVirtualAddr]),
noffH.code.size, noffH.code.inFileAddr+machine->lastVpn*PageSize);
    }

    int    initDateVirtualAddr = noffH.initData.virtualAddr+machine->lastVpn*PageSize;
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
            noffH.initData.virtualAddr, noffH.initData.size);
        machine->noffInitDataOffset=noffH.initData.virtualAddr - noffH.initData.inFileAddr;
        rBytes=machine->swap->ReadAt(&(machine->mainMemory[initDateVirtualAddr]),
noffH.initData.size,noffH.initData.inFileAddr+machine->lastVpn*PageSize);

    DEBUG('a', "read %d bytes from swap file(%d)\n",rBytes, noffH.initData.size);

    }
    //----------------------------------------------------------------------------------------
#endif

    machine->lastVpn += numPages;     // update the lastVpn

    /// the following is used to test the use rate of the memory
    machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
        DEBUG('a', "MemoryTieUpRate = %f\n",machine->MemoryTieUpRate); 

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    int i;
    for(i = 0 ;i < numPages ; i++)
    {
        bitmap->Clear(pageTable[i].physicalPage);
    }
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    //-------------this part has been modified------------------------------------------------------------
    //machine->WriteRegister(StackReg, numPages * PageSize - 16);
    machine->WriteRegister(StackReg, realNumPages*PageSize -16);
    //---------th----------------------------
    //DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
    DEBUG('a',"Initializing stack register to %d\n", realNumPages*PageSize -16);
} 

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    pageTable = machine->pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    //need to clear the tlb
    //machine->tlb = pageTable;
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}


//--------------------------------------------------------
//SwitchPageTable
//
//----------------------------------------------
void
AddrSpace::SwitchPageTable(int frameNum)
{
    int physicalPage = pageTable[frameNum].physicalPage;
    int pageOffset = 0;
    if(bitmap->Test(machine->vpn))
        bitmap->Mark(machine->vpn);
    //-----------------------------physicalPage  how to get it
    // should get the virtual page from the disk and switch out one of the physical page! when PTEPageFualtException happens
    pageTable[frameNum].physicalPage = machine->vpn;
    pageTable[frameNum].virtualPage = machine->vpn;

    DEBUG('a',"allocate bit(%d) for pageTable[%d]\n",pageTable[frameNum].physicalPage,frameNum);
    ASSERT(pageTable[frameNum].physicalPage!=-1);
    pageTable[frameNum].valid = TRUE;
    pageTable[frameNum].use = FALSE ;
    pageTable[frameNum].dirty = FALSE;

    pageTable[frameNum].readOnly =FALSE;
    bitmap->Clear(physicalPage);
}