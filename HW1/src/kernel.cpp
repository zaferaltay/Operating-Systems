
#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

TaskManager taskManager;

int lock=0;

void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}




class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};





















char mybuffer[10]={'e','e','e','e','e','e','e','e','e','e'};
int full=0;
int empty=10;
int count22=0;


#define FALSE 0
#define TRUE 1
#define N 2


int turn=0;
int interested[2]={0};

void enter_criticalregion(int process){
    int other;
    other=1-process;
    interested[process]=TRUE;
    turn=process;
    while(turn==process && interested[other]==TRUE);

}

void leave_criticalregion(int process){
    interested[process]=FALSE;
}



//produced item 1
//consumed item 0

void producer()
{   
    //producer process num----------->1
    int mynum=1;
    while(true){

        while(full==10);  //FULL İÇİN DOWN FULL
        enter_criticalregion(mynum);
        

        //while(lock==0) lock++; //---------------> Race Condition and Lock
        

        int temp=0;
        mybuffer[count22]='f';
        full++;
        empty--;
        count22++;
        

        int i=0;
        while(i<100000000){i++;}
        printf("I added a something\n");
       

        leave_criticalregion(mynum);
        //lock--;  //------------------------------->Race Condition and Lock

    }
}
void consumer()
{   
    
    int mynum=0;
    while(true){
        
        while(empty==10); 
        enter_criticalregion(mynum);


        //while(lock==0) lock++; //---------------> Race Condition and Lock 
        
        full--;
        empty++;
        mybuffer[count22]='e';
        count22--;


        
        int i=0;
        while(i<100000000){i++;}
        printf("I removed a something\n");

        leave_criticalregion(mynum);
        //lock--; //------------------------------->Race Condition and Lock

    }
}










void taskA(){
    while(1){
        int i=0;
        while(i<500000000){i++;}
        printf("a");
    }
}
void taskB(){
    while(1){
        int i=0;
        while(i<500000000){i++;}
        printf("b");
    }
}

void taskC(){
    int j=0;
        //taskManager.pthread_yield(3);
    while(1){
        int i=0;
        while(i<500000000){i++;}
        printf("c");
    }
}

void taskD(){
    int j=0;
    while(j<3){
        
        int i=0;
        while(i<500000000){i++;}
        printf("D");
        j++;
    }
    taskManager.pthread_terminate(4);
}

void taskE(){
    taskManager.pthread_join(4);
    while(1){
        int i=0;
        while(i<500000000){i++;}
        printf("E");
    }
}
void taskF(){
    while(1){
       
        int i=0;
        while(i<500000000){i++;}
        printf("F");
    }
}



typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
   


    GlobalDescriptorTable gdt;
    
/* THREAD FONKSİYONLARI DENEME 
    Task task1(&gdt, taskA,1);
    Task task2(&gdt, taskB,2);
    Task task5(&gdt, taskE,5);
    Task task3(&gdt, taskC,3);
    Task task4(&gdt, taskD,4);
    Task task6(&gdt, taskF,6);

    taskManager.pthread_create(&task1);
    taskManager.pthread_create(&task5);
    taskManager.pthread_create(&task3);
    taskManager.pthread_create(&task2);
    taskManager.pthread_create(&task4);
    taskManager.pthread_create(&task6);
   */
    
   Task task1(&gdt, producer,1);
    Task task2(&gdt, consumer,2);
   
    taskManager.pthread_create(&task1);
    taskManager.pthread_create(&task2);
 
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    
  
    
    #ifdef GRAPHICSMODE
        Desktop desktop(320,200, 0x00,0x00,0xA8);
    #endif
    
    DriverManager drvManager;
    
        #ifdef GRAPHICSMODE
            KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            PrintfKeyboardEventHandler kbhandler;
            KeyboardDriver keyboard(&interrupts, &kbhandler);
        #endif
        drvManager.AddDriver(&keyboard);
        
    
        #ifdef GRAPHICSMODE
            MouseDriver mouse(&interrupts, &desktop);
        #else
            MouseToConsole mousehandler;
            MouseDriver mouse(&interrupts, &mousehandler);
        #endif
        drvManager.AddDriver(&mouse);
        
        PeripheralComponentInterconnectController PCIController;
        PCIController.SelectDrivers(&drvManager, &interrupts);

        VideoGraphicsArray vga;
        
 
        drvManager.ActivateAll();
        
 

    #ifdef GRAPHICSMODE
        vga.SetMode(320,200,8);
        Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
        desktop.AddChild(&win2);
    #endif


    interrupts.Activate();
    
    while(1)
    {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
    }
}
