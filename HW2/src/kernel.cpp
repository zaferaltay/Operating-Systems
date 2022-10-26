
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


typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}


void swap(char *x, char *y) {
    char t = *x; *x = *y; *y = t;
}
 
// Function to reverse `buffer[i…j]`
char* reverse(char *buffer, int i, int j)
{
    while (i < j) {
        swap(&buffer[i++], &buffer[j--]);
    }
 
    return buffer;
}
 
// Iterative function to implement `itoa()` function in C
char* itoa(int value, char* buffer, int base)
{
    // invalid input
    if (base < 2 || base > 32) {
        return buffer;
    }
 
    // consider the absolute value of the number
    if (value<0)
    {
        value*=-1;
    }
    int n=value;
 
    int i = 0;
    while (n)
    {
        int r = n % base;
 
        if (r >= 10) {
            buffer[i++] = 65 + (r - 10);
        }
        else {
            buffer[i++] = 48 + r;
        }
 
        n = n / base;
    }
 
    // if the number is 0
    if (i == 0) {
        buffer[i++] = '0';
    }
 
    // If the base is 10 and the value is negative, the resulting string
    // is preceded with a minus sign (-)
    // With any other base, value is always considered unsigned
    if (value < 0 && base == 10) {
        buffer[i++] = '-';
    }
 
    buffer[i] = '\0'; // null terminate string
 
    // reverse the string and return it
    return reverse(buffer, 0, i - 1);
}


//------------------------STRUCTS --------------------------------------------------------------------

int arrayForSorting[40]={40,39,38,37,36,35,34,33,32,31,
                         30,29,28,27,26,25,24,23,22,21,
                         20,19,18,17,16,15,14,13,12,11,
                         10,9,8,7,6,5,4,3,2,1};

const int arraySize=40;
const int memorySize=10;
int replacementCounter=0;
int hitCounter=0;
int missCounter=0;
int fifolist[memorySize];
int pmCounter=0; //Memoryde ki eleman sayısı, memorysize'dan küçükse direkt oraya eklenir.


struct{
    int val;
    int rBit;
    int validBit;
    int counter; //for LRU
    int physicalFrame;

}typedef PageEntry;


struct
{
    PageEntry pagetable[arraySize];
    
}typedef VirtualMemory;
VirtualMemory vm;

struct
{
    int pages[memorySize];
    
}typedef PhysicalMemory;
PhysicalMemory pm;


struct{

    int diskPages[arraySize];

}typedef Disk;
Disk disk;


//---------------------ASISTANT FUNCTIONS------------------------------------------

void printVM(){    //VM YAZDIRIR
    char temp[100];

    for (int i = 0; i < arraySize; ++i)
    {   
        printf("Memory Page: ");
        printf(itoa(i,temp,10));
        printf("\n");

        printf("Page Value: ");
        printf(itoa(vm.pagetable[i].val,temp,10));
        printf("\n");

        printf("Page rBit: ");
        printf(itoa(vm.pagetable[i].rBit,temp,10));
        printf("\n");
        printf("Page validBit: ");
        printf(itoa(vm.pagetable[i].validBit,temp,10));
        printf("\n");
        printf("Page counter: ");
        printf(itoa(vm.pagetable[i].counter,temp,10));
        printf("\n");
        printf("Page physical FRame: ");
        printf(itoa(vm.pagetable[i].physicalFrame,temp,10));
        printf("\n");

        printf("---------------------------------");

    }
}

void printDisk(){   //DİSKİ YAZDIRIR
    char temp[100];
    for (int i = 0; i < arraySize; ++i)
    {
        printf("Page Value: ");
        printf(itoa(disk.diskPages[i],temp,10));
        printf("\n");

    }

}
void printMemory(){
    char temp[100];
    for (int i = 0; i < pmCounter; ++i)
    {
        printf(itoa(pm.pages[i],temp,10));
        printf("-");
    }
}
void printFifoList(){
    char temp[100];
    for (int i = 0; i < pmCounter; ++i)
    {
        printf(itoa(fifolist[i],temp,10));
        printf("-");
    }
}


void initializeSystem(){    //DİSK,VM DOLDURUR.

    for (int i = 0; i < arraySize; ++i)  //Her page entry'yi ayarlar. Ardından page table'a ekler + Diske doldurur.
    {
        PageEntry temp;
        temp.val=arrayForSorting[i];
        temp.rBit=0;
        temp.validBit=0;
        temp.counter=120-i;  //FOR LRU  ------------------------------------------> DEMO İÇİN DÜZENLEME YAP
        temp.physicalFrame=-1;
        vm.pagetable[i]=temp;
        disk.diskPages[i]=temp.val;
    }
}


bool findInMemory(int n){  //aranan page değeri memoryde varsa true,yoksa false döndürür.
    for (int i = 0; i < memorySize; ++i)
    {
        if (n==pm.pages[i])
        {   
            hitCounter++;
            return true;
        }
    }
    missCounter++;
    return false;
}

int findInVM(int n){                //Virtual Memoryde aranan page'in indexini verir.
    for (int i = 0; i < arraySize; ++i)
    {
        if (vm.pagetable[i].val==n)
        {
            return i;
        }
    }
    return 0;
}

int findInDisk(int n){                    //Diskte aranan page'in indexini verir.
    for (int i = 0; i < arraySize; ++i)
    {
        if (disk.diskPages[i]==n)
        {
            return i;
        }
    }
    return 0;
}

//-------------------------REPLACEMENTS--------------------

void firstInFirstOut(int newpage){                      //Fifolisttin ilk elemanındaki page'i memoryde bulur kaldırır.
                                                        //Yeni gelecek page'i memory'de onun yerine ekler.
    //SİLİNECEK                                         //Fifo listi günceller.
    printf("Mem: ");
    printMemory();
    printf("\n");
    printf("Fif: ");
    printFifoList();
    printf("\n");
    /*for(int j=0;j<80000;j++){
        for (int k = 0; k < 20000; k++)
        {
                //SLEEP
        }
    }
    */
    int i=0;
    int temp=fifolist[i];
    int vmIndex=findInVM(temp);
    vm.pagetable[vmIndex].physicalFrame=-1;
    vm.pagetable[vmIndex].validBit=0;

    for (i = 0; i < memorySize-1; i++)
    {   
        fifolist[i]=fifolist[i+1];

    }
    fifolist[memorySize-1]=newpage;
    
    for (int i = 0; i < memorySize; ++i)
    { 
        if (pm.pages[i]==temp)
        {
            pm.pages[i]=newpage;
            int vmIndex=findInVM(newpage);
            vm.pagetable[vmIndex].physicalFrame=i;
            vm.pagetable[vmIndex].rBit=1;
            vm.pagetable[vmIndex].validBit=1;
            replacementCounter++;
        }   
    }
}
void secondC(int newpage){
    

   int i=0;

   while(1){                                    //Fifo listteki ilk elemanın rbiti 0'sa fifo fonskiyonunu çağırır,1 ise 0 yapıp en sona atar kaydırır.
        int temp=fifolist[0];
        int vmIndex=findInVM(temp);             //0 olan fifoda kaldırılır. Memory'e eklenir. Page table güncellenir.Fİfo list güncellenir.

        printf("Fif: ");
        printFifoList();
        printf("\n");

        
        if (vm.pagetable[vmIndex].rBit==0)     //
        {
            firstInFirstOut(newpage);
            return;
        }
        else{
            vm.pagetable[vmIndex].rBit=0;
            for (int i = 0; i < memorySize-1; i++)
            {
                fifolist[i]=fifolist[i+1];
            }
            fifolist[memorySize-1]=temp;
        }
   }

}

void LRUSoftware(int newpage){           //Minimum counter'a sahip page'i arar. Onu kaldırır. Yeni gelen page'i onun yerine ekler.
    
    int min=99999;                          //Page table'ı günceller.
    int index=-1;
    int pageNum=-1;                         

    for (int i = 0; i < memorySize; i++)   
    {   
        int xx=pm.pages[i];
        int vmIndex=findInVM(xx);
        if (min>vm.pagetable[vmIndex].counter)
        {
           min=vm.pagetable[vmIndex].counter;
           index=i;
           pageNum=xx;
        }
    }
        //SİLİNECEK
    printf("Mem: ");
    printMemory();
    printf("\n");
    printf("Fif: ");
    printFifoList();
    printf("\n");
    /*  for(int j=0;j<80000;j++){
            for (int k = 0; k < 20000; k++)
            {
                //SLEEP
            }
        }
    */
    int vmIndex=findInVM(pageNum);
    vm.pagetable[vmIndex].physicalFrame=-1;
    int vmIndex2=findInVM(newpage);
    vm.pagetable[vmIndex2].physicalFrame=index;
    pm.pages[index]=newpage;
    replacementCounter++;

}

//------------SORT ALGORITHM---------------------------------------------------------------
void bubbleSort(int arr[], int n,int algorithm){              // bubble sort function

    for(int i = 0; i < n; i++){
        for(int j = 0; j < n-i-1; j++){         // loop until n-i-1(further is unnecessary)

            if (!findInMemory(arr[j]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
            {
                int vmIndex=findInVM(arr[j]);
                int diskIndex=findInDisk(arr[j]);
                if (pmCounter<memorySize)
                {
                    pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                    vm.pagetable[vmIndex].physicalFrame=pmCounter;
                    vm.pagetable[vmIndex].rBit=1;
                    vm.pagetable[vmIndex].validBit=1;
                    fifolist[pmCounter]=disk.diskPages[diskIndex];
                    pmCounter++;

                }
                else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                    if (algorithm==1)
                    {
                        firstInFirstOut(arr[j]);
                    }
                    else if(algorithm==2){
                        secondC(arr[j]);
                    }
                    else{
                        LRUSoftware(arr[j]);
                    }
                }

            }
            if (!findInMemory(arr[j+1]))  //Page Replacement for arr[j+1] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
            {
                int vmIndex=findInVM(arr[j+1]);
                int diskIndex=findInDisk(arr[j+1]);
                if (pmCounter<memorySize)
                {
                    pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                    vm.pagetable[vmIndex].physicalFrame=pmCounter;
                    vm.pagetable[vmIndex].rBit=1;
                    vm.pagetable[vmIndex].validBit=1;
                    fifolist[pmCounter]=disk.diskPages[diskIndex];
                    pmCounter++;

                }
                else{                                    //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                    if (algorithm==1)
                    {
                        firstInFirstOut(arr[j+1]);
                    }
                    else if(algorithm==2){
                        secondC(arr[j+1]);
                    }
                    else{
                        LRUSoftware(arr[j+1]);
                    }
                }

            }
            if( arr[j] > arr[j+1]){
                int temp = arr[j];              // swap elements for sorting them
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            } 
        }
    }
}

void quickSort(int arr[], int n,int algorithm){               // quick sort function
    if ( n >= 2 ){
        int pivot = arr[n / 2];     // pivot element is selected as middle element

        if (!findInMemory(arr[n / 2]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
        {
            int vmIndex=findInVM(arr[n / 2]);
            int diskIndex=findInDisk(arr[n / 2]);
            if (pmCounter<memorySize)
            {
                pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                vm.pagetable[vmIndex].physicalFrame=pmCounter;
                vm.pagetable[vmIndex].rBit=1;
                vm.pagetable[vmIndex].validBit=1;
                fifolist[pmCounter]=disk.diskPages[diskIndex];
                pmCounter++;
            }
            else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                if (algorithm==1)
                {
                    firstInFirstOut(arr[n / 2]);
                }
                else if(algorithm==2){
                    secondC(arr[n / 2]);
                }
                else{
                    LRUSoftware(arr[n / 2]);
                }
            }
        }

        int i = 0, j = n - 1;
        for (;; i++, j--){
            while (arr[i] < pivot){     // find element that is on left-side but higher than pivot                 
                if (!findInMemory(arr[i]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
                {
                    int vmIndex=findInVM(arr[i]);
                    int diskIndex=findInDisk(arr[i]);
                    if (pmCounter<memorySize)
                    {
                        pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                        vm.pagetable[vmIndex].physicalFrame=pmCounter;
                        vm.pagetable[vmIndex].rBit=1;
                        vm.pagetable[vmIndex].validBit=1;
                        fifolist[pmCounter]=disk.diskPages[diskIndex];
                        pmCounter++;
                    }
                    else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                        if (algorithm==1)
                        {
                            firstInFirstOut(arr[i]);
                        }
                        else if(algorithm==2){
                            secondC(arr[i]);
                        }
                        else{
                            LRUSoftware(arr[i]);
                        }
                    }
                }

                i++;  
            }


            while (arr[j] > pivot){     // find element that is on right-side but lower than pivot

                if (!findInMemory(arr[j]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
                {
                    int vmIndex=findInVM(arr[j]);
                    int diskIndex=findInDisk(arr[j]);
                    if (pmCounter<memorySize)
                    {
                        pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                        vm.pagetable[vmIndex].physicalFrame=pmCounter;
                        vm.pagetable[vmIndex].rBit=1;
                        vm.pagetable[vmIndex].validBit=1;
                        fifolist[pmCounter]=disk.diskPages[diskIndex];
                        pmCounter++;
                    }
                    else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                        if (algorithm==1)
                        {
                            firstInFirstOut(arr[j]);
                        }
                        else if(algorithm==2){
                            secondC(arr[j]);
                        }
                        else{
                            LRUSoftware(arr[j]);
                        }
                    }
                } 

                j--;    
            } 
            if (i >= j){
                break;  
            } 
            int temp = arr[i];          // swap elements according to pivot element
            arr[i] = arr[j];
            arr[j] = temp;
        }
        quickSort(arr, i,algorithm);      // left partition
        quickSort(arr + i, n - i,algorithm);  // right partition
    }
}

void insertionSort(int arr[], int n,int algorithm)
{   

    //-----------------------------
    int i, key, j;
    for (i = 1; i < n; i++)
    {   
    //--------------------------------

        if (!findInMemory(arr[i]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
        {
            int vmIndex=findInVM(arr[i]);
            int diskIndex=findInDisk(arr[i]);
            if (pmCounter<memorySize)
            {
                pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                vm.pagetable[vmIndex].physicalFrame=pmCounter;
                vm.pagetable[vmIndex].rBit=1;
                vm.pagetable[vmIndex].validBit=1;
                fifolist[pmCounter]=disk.diskPages[diskIndex];
                pmCounter++;
            }
            else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                if (algorithm==1)
                {
                    firstInFirstOut(arr[i]);
                }
                else if(algorithm==2){
                    secondC(arr[i]);
                }
                else{
                    LRUSoftware(arr[i]);
                }
            }
        }

        //-------------------
        key = arr[i];
        j = i - 1;
        //-------------------

        if (!findInMemory(arr[j]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
        {
            int vmIndex=findInVM(arr[j]);
            int diskIndex=findInDisk(arr[j]);
            if (pmCounter<memorySize)
            {
                pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                vm.pagetable[vmIndex].physicalFrame=pmCounter;
                vm.pagetable[vmIndex].rBit=1;
                vm.pagetable[vmIndex].validBit=1;
                fifolist[pmCounter]=disk.diskPages[diskIndex];
                pmCounter++;
            }
            else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                if (algorithm==1)
                {
                    firstInFirstOut(arr[j]);
                }
                else if(algorithm==2){
                    secondC(arr[j]);
                }
                else{
                    LRUSoftware(arr[j]);
                }
            }
        }        
 
        // Move elements of arr[0..i-1], 
        // that are greater than key, to one
        // position ahead of their
        // current position
        //--------------------------------------
        while (j >= 0 && arr[j] > key)
        {
        //--------------------------------------

            if (!findInMemory(arr[j+1]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
            {
                int vmIndex=findInVM(arr[j+1]);
                int diskIndex=findInDisk(arr[j+1]);
                if (pmCounter<memorySize)
                {
                    pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                    vm.pagetable[vmIndex].physicalFrame=pmCounter;
                    vm.pagetable[vmIndex].rBit=1;
                    vm.pagetable[vmIndex].validBit=1;
                    fifolist[pmCounter]=disk.diskPages[diskIndex];
                    pmCounter++;
                }
                else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                    if (algorithm==1)
                    {
                        firstInFirstOut(arr[j+1]);
                    }
                    else if(algorithm==2){
                        secondC(arr[j+1]);
                    }
                    else{
                        LRUSoftware(arr[j+1]);
                    }
                }
            }
            if (!findInMemory(arr[j]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
            {
                int vmIndex=findInVM(arr[j]);
                int diskIndex=findInDisk(arr[j]);
                if (pmCounter<memorySize)
                {
                    pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                    vm.pagetable[vmIndex].physicalFrame=pmCounter;
                    vm.pagetable[vmIndex].rBit=1;
                    vm.pagetable[vmIndex].validBit=1;
                    fifolist[pmCounter]=disk.diskPages[diskIndex];
                    pmCounter++;
                }
                else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                    if (algorithm==1)
                    {
                        firstInFirstOut(arr[j]);
                    }
                    else if(algorithm==2){
                        secondC(arr[j]);
                    }
                    else{
                        LRUSoftware(arr[j]);
                    }
                }
            }  

        //-------------------------------                
            arr[j + 1] = arr[j];
            j = j - 1;
        //-------------------------------    
        }

        if (!findInMemory(arr[j]))  //Page replacement for arr[j] + Eğer memory dolu değilse direkt memory'e ekleme yapılır.
        {
            int vmIndex=findInVM(arr[j]);
            int diskIndex=findInDisk(arr[j]);
            if (pmCounter<memorySize)
            {
                pm.pages[pmCounter]=disk.diskPages[diskIndex]; //get page from disk
                vm.pagetable[vmIndex].physicalFrame=pmCounter;
                vm.pagetable[vmIndex].rBit=1;
                vm.pagetable[vmIndex].validBit=1;
                fifolist[pmCounter]=disk.diskPages[diskIndex];
                pmCounter++;
            }
            else{                                                           //Eğer memory doluysa algoritmaya göre page replacement yapılır.
                if (algorithm==1)
                {
                    firstInFirstOut(arr[j]);
                }
                else if(algorithm==2){
                    secondC(arr[j]);
                }
                else{
                    LRUSoftware(arr[j]);
                }
            }
        }  
        arr[j + 1] = key;
    }
}

//-------------------------------------------------------------------------------------------

extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
   


    GlobalDescriptorTable gdt;    
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    char temp[100];

    // FIFO-->1      SC--->2         LRU----->3
    // BUBBLE-->0    INSERTION->>1   QUICK--->2 

    
    initializeSystem();
    //printVM();
    //printDisk();

    int algo=1;
    int pageRep=2;

    if (algo==0)
    {
        if (pageRep==1)
        {
            bubbleSort(arrayForSorting,arraySize,1);   
        }
        else if (pageRep==2)
        {
            bubbleSort(arrayForSorting,arraySize,2); 
        }
        else if (pageRep==3)
        {
            bubbleSort(arrayForSorting,arraySize,3); 
        }
        else{
            printf("Invalid Page Replacement Algorithm \n");
        }
    }
    else if(algo==1){
        if (pageRep==1)
        {
            insertionSort(arrayForSorting,arraySize,1);   
        }
        else if (pageRep==2)
        {
            insertionSort(arrayForSorting,arraySize,2); 
        }
        else if (pageRep==3)
        {
            insertionSort(arrayForSorting,arraySize,3); 
        }
        else{
            printf("Invalid Page Replacement Algorithm \n");
        }
    }
    else if(algo==2){
         if (pageRep==1)
        {
            quickSort(arrayForSorting,arraySize,1);   
        }
        else if (pageRep==2)
        {
            quickSort(arrayForSorting,arraySize,2); 
        }
        else if (pageRep==3)
        {
            quickSort(arrayForSorting,arraySize,3); 
        }
        else{
            printf("Invalid Page Replacement Algorithm \n");
        }
    }
    else{
        printf("Invalid Sorting Algortihm\n");
        
    }

    //bubbleSort(arrayForSorting,arraySize,3);
    //insertionSort(arrayForSorting,arraySize,3);
    //quickSort(arrayForSorting,arraySize,3);
    for (int i = 0; i < arraySize; ++i)
    {
        printf(itoa(arrayForSorting[i],temp,10));
        printf("-");
    }
    printf("\n \n");
    printf("Mem : ");
    printMemory();
    printf("\n");
    printf("Fİfo List : ");
    printFifoList();

    printf("\nTotal Page Replacement : ");
    printf(itoa(replacementCounter,temp,10));

    printf("\nTotal Hit : ");
    printf(itoa(hitCounter,temp,10));

    printf("\nTotal Miss : ");
    printf(itoa(missCounter,temp,10));

    printf("\n Sorted Array Now: ");
    for (int i = 0; i < arraySize; ++i)
    {
        printf(itoa(arrayForSorting[i],temp,10));
        printf("-");
    }
   
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