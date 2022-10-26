
#include <multitasking.h>

using namespace myos;
using namespace myos::common;


Task::Task(GlobalDescriptorTable *gdt, void entrypoint(),int num)
{
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    taskNum=num;
}

Task::~Task()
{
}

        
TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    return true;
}

bool TaskManager::pthread_create(Task* task)
{
    AddTask(task);
}

void TaskManager::pthread_terminate(int num){
    int counter;
    for (int i = 0; i < numTasks; i++)
    {
        if (num==tasks[i]->taskNum)
        {   
            tasks[i]->taskNum=-99;
            counter=i;
            for ( ;counter <numTasks ; )
            {
                
            }

            numTasks--;
            return;
        }
    }
}


void TaskManager::pthread_yield(int num){
    int counter;
    for (int i = 0; i < numTasks; i++)
    {
        if (num==tasks[i]->taskNum)
        {   
            Task *t=tasks[i];
            
            for (counter=i ; counter <numTasks ; counter++)
            {
                tasks[counter]=tasks[counter+1];
            }
            tasks[numTasks]=t;  
            return;
        }
    }
}


void TaskManager::pthread_join(int num){
   
    int threadflag=0;
    while(threadflag==0){
        threadflag=1;
       for (int i = 0; i < numTasks; ++i)
       {
           if (num==tasks[i]->taskNum)
           {
               threadflag=0;
               break;
           }
       }
    }


}


CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask]->cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;

    return tasks[currentTask]->cpustate;
}

    