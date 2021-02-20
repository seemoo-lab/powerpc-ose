#include "../include/symbols.h"
#include "exaddr.h"

#define FLAGPARAM "-p"
#define RESULTPARAM "-r"
#define ADDR_SIZE 4

void* GLOBAL_RESULT;

void* exaddr_func(int argc, char* addr, void **param);
void parseCmdData(char *blob, unsigned int length, char* result);

void exaddr(int argc, char **argv){
    if(argc<2){
        printf("Syntax: %s <address(hex)> [options]\n"
            "Options:\n"
            "%s <number> <format> <value>...\tExecutes parameterized functions with a specified number \n"
            "\t\t\t\tof arguments (max. 5). Each argument is preceded by a \n"
            "\t\t\t\tformat-specifier like %%d,%%x,etc.\n"
            "\t\t\texample:  %s 2 %%x 0xb16b00b5 %%d 42\n",argv[0],FLAGPARAM,FLAGPARAM);
        return;
    }
    //uint size depends on addr-length, 32 or 64 bit. Alternatively use char*
    char* addr;
    //argv[1], since argv[0] is the command-name
    sscanf(argv[1], "%x", &addr);
    printf("executing address %x\n", addr);
    
    void *param[7];
    char printResult = 0;
    int resultBytecount = 0;

    if(argc>2){       
        int vargCount=0, vargCountTemp=0;// vargPos=-1;
        
        //iterating over argv for different flags & arguments
        for(int i=2; i<argc; i++){

            if(memcmp(FLAGPARAM, argv[i], 2)==0){
                //"-p" expects subsequently a specified number of arguments
                sscanf(argv[i+1], "%d", &vargCount);
                vargCountTemp = vargCount;
                //vargPos = i+2;
                i++;
            }else if(vargCountTemp>0){
                //iterating over "-p" arguments. An arguments is actually a pair 
                //of a formatstring followed by the actual argument
                if(memcmp("%s", argv[i], 2)==0){
                    //sscanf(argv[i+1], "%s", param[vargCount-vargCountTemp]);
                    param[vargCount-vargCountTemp] = argv[i+1];
                    //printf("%d: %s Val %s\n",i,argv[i],param[vargCount-vargCountTemp]);
                } else {
                    sscanf(argv[i+1], argv[i], &param[vargCount-vargCountTemp]);
                    //printf("%d: %s Val %d\n",i,argv[i],param[vargCount-vargCountTemp]);
                }

                i++;
                vargCountTemp--;
            } else if(memcmp(RESULTPARAM, argv[i], 2)==0 && printResult==0){
                //"-r" expects subsequently a specified number of arguments
                if(memcmp("%s", argv[i+1], 2)==0){
                    printResult = 's';
                }else{
                    printResult = 1;
                    sscanf(argv[i+1], "%d", &resultBytecount);
                }
                break;
                
            }
            
        }

        GLOBAL_RESULT = exaddr_func(vargCount, addr, (void**)param);

            

    }else{
        GLOBAL_RESULT = exaddr_func(0, addr, (void**)param);
    }

    if(printResult == 's'){
        printf("result string at 0x%x:\n",GLOBAL_RESULT);
        printf("%s\n",GLOBAL_RESULT);
    } else if(printResult !=0){
        printf("result bytes at 0x%x:\n",GLOBAL_RESULT);
        char* membyteAddr = (char*)GLOBAL_RESULT;
        for(int i=0; i<resultBytecount; i++){
            printf("%x",membyteAddr[i]);
            if((i+1)%4==0)printf("\n");
        }
         printf("\n");
    }


}

void* exaddr_func(int argc, char* addr, void **args){

    //since our symbol-table doesn't have js-"spread"-like functions to convert argument-arrays to individual arguments,
    //we have to work with fixed numbers of arguments

    //Intention: any function can be cast to void(*)(void) and the function parameter void* can be any type
    if(argc==0){
    
        return ((void* (*)(void))addr)();
        
    } else {
    
        return ((void* (*)(void*, ...))addr)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        
    }

}




void parseCmdData(char *blob, unsigned int length, char* result){

    
    unsigned int resultCounter = 0;
    unsigned int tmpLength;
    unsigned int i = 0;

    void* temp = 0x0;
    uint32_t blockType = 0x0;
    while(i<(length)){


        memcpy(&blockType, &blob[i], sizeof(blockType));
        i = i + ADDR_SIZE;
        printf("Type %x\n",blockType);
        memcpy(&tmpLength, &blob[i], sizeof(tmpLength));
        switch(blockType){
            case 0: 
            {

                i = i + ADDR_SIZE;
                memcpy(&result[resultCounter], &blob[i], tmpLength);
                i = i + tmpLength;
                resultCounter = resultCounter+tmpLength;
            }
            break;
            case 1: 
            {

                i = i + ADDR_SIZE;
                temp = (void*)&(blob[i]);
                memcpy(&result[resultCounter], &temp, ADDR_SIZE);
                i = i + tmpLength;
                resultCounter = resultCounter+ADDR_SIZE;
                temp = 0x0;
            }
            break;
            /**
            case 2: 
            {
                i = i + sizeof(tmpLength);
                parseCmdData((char*)&(blob[i]), tmpLength, (char*)&(result[resultCounter]));
                i = i + tmpLength;
                resultCounter = resultCounter+tmpLength;
            }
            break;
            */
            default:
            {
                return;
            }
        }
        //printf("\n");
        
    }
    
    
}