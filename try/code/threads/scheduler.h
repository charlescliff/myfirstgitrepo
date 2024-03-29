// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
//#include "mylist.h"

#include "thread.h"
#include "timer.h"
#define InitialPri 20
#define RunPri 50
#define AdaptPri 10
#define ClockTime 20n0
#define MiniClockTime 50
enum  SchedPolicy
  {SCHED_PRIO_P,
   SCHED_MLQ,
   SCHED_SJF,
   SCHED_SRTN,
   SCHED_FCFS,
   SCHED_RR,
   SCHED_MLFQ,
   SCHED_PRIO_NP
  };
// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();			// Initialize list of ready threads 
    ~Scheduler();			// De-allocate ready list

    void ReadyToRun(Thread* thread);	// Thread can be dispatched.
    Thread* FindNextToRun();		// Dequeue first thread on the ready 
					// list, if any, and return thread.
    void Run(Thread* nextThread);	// Cause nextThread to start running
    void Print();			// Print contents of ready list
    void updatePriority();
    int getLastScheduleTime();
    void SetSchedPolicy(SchedPolicy pol);
    void Resume(Thread *thread);
    void Suspend(Thread *thread);
    void SetNumOfQueues(int level);
    void InterruptHandler( int dummy);
    bool ShouldISwitch(Thread *oldThread, Thread *newThread);
    SchedPolicy getPolicy(){return policy;};
    int getlastSwitchTime(){return lastSwitchTime;};
    void setlastSwitchTime(int time){lastSwitchTime = time;};
    bool CheckIfHigherEnter();
  private:
   // MyList <Thread *> *readyList;  		// queue of threads that are ready to run,
      List *readyList;				// but not running
      List *readyList_1;
      List *readyList_2;
    int lastSwitchTime;
    Timer *ptimer;
    List  *suspendedList;
    SchedPolicy policy;
    
};

#endif // SCHEDULER_H
