/* 
  Thread and multicore functions for GLaDOS.
  
  Built around the UEFI Multiprocessing Service Protocol:
    https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/MpService.h
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"

// Seems like you get to define your own "AP_PROCEDURE" type.
//  (maybe this is so you can put your own type on Buffer?)
typedef VOID(EFIAPI * EFI_AP_PROCEDURE) (IN OUT VOID *Buffer);

#include "efi/protocol/MpService.h"

// A definitely-not-working lock implementation.
class TerribleLock {
public:
    volatile int locked; // 0: unlocked.  1: locked
    TerribleLock() { locked=0; }
    void lock(void) {
        while (locked!=0) pause_CPU(); 
        // FIXME: race condition here!  What if some other thread grabs the lock?
        locked=1;
    }
    void unlock(void) {
        locked=0;
    }
};

// Lock on creation, unlock on destruction.
template <class Mutex>
class lock_guard {
public:
    Mutex &l;
    lock_guard(Mutex &l_) :l(l_) { l.lock(); }
    ~lock_guard() { l.unlock(); }
};

TerribleLock printLock;

// This function gets run to print info about each core.
void printCore(void *ignored)
{
    lock_guard<TerribleLock> scope(printLock);
    int local=3;
    int *ptr=&local; 
    print(" says hello! ");
    print((UINTN)ptr);
    print(" is address of stack\n");
}


/* Manages thread startup on multiple cores */
class MulticoreHardware {
public:
    MulticoreHardware() {
        EFI_GUID guid = EFI_MP_SERVICES_PROTOCOL_GUID;
        UEFI_CHECK(ST->BootServices->LocateProtocol(&guid,0,(void **)&mp));
    }
    
    void printCores() {
        UINTN ncores=0, nenabled=0;
        mp->GetNumberOfProcessors(mp,&ncores,&nenabled);
        print("Cores: "); print((int)ncores);
        print(" Enabled: "); print((int)nenabled);
        print("\n");
        
        // Run on our own core:
        print("Boot core "); printCore(0);
        
        // Run all the other cores (one at a time)
        for (int core=1;core<nenabled;core++)
        {
            print("Core "); print(core);
            mp->StartupThisAP(mp, printCore, core,
                0,0,0,0);
        }
    }
    
    // Run code on all cores at once:
    void testCores() {
        print("All cores at once: ");
        mp->StartupAllAPs(mp, printCore, false,
            0,0,0,0);
    }

private:
    EFI_MP_SERVICES_PROTOCOL *mp;
};


void print_threads()
{
    MulticoreHardware mh;
    mh.printCores();
}

void test_threads() 
{
    MulticoreHardware mh;
    mh.printCores();
    mh.testCores();
}


