#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "SysStatChecker.h"

//pre-compile processings
#define TEST_PHRASE     "for he today that sheds his blood with me shall be my brothers!\n"
#define CMD_NAME_LENGTH   32
#define SD_CHK_BUFFER_LEN   128
#define CONFIG_PATH_MAX   256
#define CONFIG_CMD_MAX    256
#define SD_PATH     "/media/mmcblk0p1/"

//declarations

//private function declarations

//a wrapper function to start the system module monitor
static res_code start(callback_t callback,unsigned int sleep_time);

//a generic handler function to ALL exceptions on my platform
static void handler(void* arg);

//a function that checks whether a device path exists  
static int isExist(char* path);

//a function that gets system configurations for my platform 
static char* getConfig(const char* key);

//a function that checks whether an ethernet wire is plugged in
static int isPluggedIn(int port);

//a standard function that tells whether the inserted SD card is well-functioning
static int std_sdcard(void* arg);

//a standard function that tells whether an ethernet wire is plugged in 
static int std_lan(void* arg);

//a standard function that tells whether wifi is in the "UP RUNNING" state
static int std_wifi(void* arg);

//a generic standard function that tells whether the device paths of character devices exist
static int std_generic(void* arg);

//a function that converts characters in a string into LOWER CASE
static void toLowerCase(char* string);

//a call-back function to report the status of various devices on board
static void callback(long* flag);

//a main function to illustrate the usage of this simple framework
int main(int argc,char* argv[]){
    char recv[CMD_NAME_LENGTH];
    res_code res;
    
    if((res = start(callback,5)) != RES_OK){
        fprintf(stderr,"Error starting thread!\n");
        return -1;
    }
    while(true){
        memset(recv, 0, sizeof(recv));
        printf("please enter your command:\n");
        scanf("%s",recv);
        printf("your command is %s~\n",recv);
        if(strlen(recv) == 0){
            fprintf(stderr,"invalid command~\n");
        }else{
            toLowerCase(recv);  
        }
        if(strcmp(recv,"stop") == 0){
            if((res = stopScanning()) != RES_OK){
                fprintf(stderr,"Error stopping thread!\n");
            }else{
                printf("checking thread stopped!\n"); 
            }
            break;
        }else{
            fprintf(stderr,"command NOT supported~!\n");
        }
    }
    
    return 0;
}

//a wrapper funciton to start system modules checking 
static res_code start(callback_t callback,unsigned int sleep_time){
    res_code result;
    Entry_t sdcard;
    Entry_t wifi;
    Entry_t lan;
    Entry_t lte;
    Entry_t gps;
    Entry_t dmr;
    Entry_t codec;
    Entry_t play;

    //sdcard module
    sdcard.name = "sdcard";
    sdcard.handler = handler;
    sdcard.handler_arg = (void*)sdcard.name;
    sdcard.std = std_sdcard;
    sdcard.std_arg = NULL;
    sdcard.next = NULL;

    //lan module
    lan.name = "lan";
    lan.handler = handler;
    lan.handler_arg = (void*)lan.name;
    lan.std = std_lan;
    lan.std_arg = (void*)0x2;
    lan.next = NULL;

    //wifi module
    wifi.name = "wifi";
    wifi.handler = handler;
    wifi.handler_arg = (void*)wifi.name;
    wifi.std = std_wifi;
    wifi.std_arg = (void*)0x1;
    wifi.next = NULL;

    //lte module
    lte.name = "lte";
    lte.handler = handler;
    lte.handler_arg = (void*)lte.name;
    lte.std = std_generic;
    lte.std_arg = (void*)"lte.path";
    lte.next = NULL;

    //gps module
    gps.name = "gps";
    gps.handler = handler;
    gps.handler_arg = (void*)gps.name;
    gps.std = std_generic;
    gps.std_arg = (void*)"gps.path";
    gps.next = NULL;

    //dmr module
    dmr.name = "dmr";
    dmr.handler = handler;
    dmr.handler_arg = (void*)dmr.name;
    dmr.std = std_generic;
    dmr.std_arg = (void*)"dmr.path";
    dmr.next = NULL;

    //codec module
    codec.name = "codec";
    codec.handler = handler;
    codec.handler_arg = (void*)codec.name;
    codec.std = std_generic;
    codec.std_arg = (void*)"codec.path";
    codec.next = NULL;

    //play module
    play.name = "play";
    play.handler = handler;
    play.handler_arg = (void*)play.name;
    play.std = std_generic;
    play.std_arg = (void*)"play.path";
    play.next = NULL;

    //register entries 
    
    //register the entry for checking the status of SD card
    result = registerEntry(&sdcard);
    if(result != RES_OK){
        printf("Error registering the entry sdcard~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for checking the plugging of an ethernet wire
    result = registerEntry(&lan);
    if(result != RES_OK){
        printf("Error registering the entry lan~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for checking the status of WiFi 
    result = registerEntry(&wifi);
    if(result != RES_OK){
        printf("Error registering the entry wifi~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for validating the path of the LTE device 
    result = registerEntry(&lte);
    if(result != RES_OK){
        printf("Error registering the entry lte~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for validating the path of the GPS device
    result = registerEntry(&gps);
    if(result != RES_OK){
        printf("Error registering the entry gps~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for validating the path of the DMR device
    result = registerEntry(&dmr);
    if(result != RES_OK){
        printf("Error registering the entry dmr~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for validating the path of the audio CODEC device
    result = registerEntry(&codec);
    if(result != RES_OK){
        printf("Error registering the entry codec~!\n");
        return RES_EREGISTER;
    }
    
    //register the entry for validating the path of the audio PLAY device
    result = registerEntry(&play);
    if(result != RES_OK){
        printf("Error registering the entry play~!\n");
        return RES_EREGISTER;
    }
    //start scanning
    result = startScanning(callback,sleep_time);
    if(result != RES_OK){
        printf("Error launching the scanning thread~!\n");
        return result;
    }
    return RES_OK;
}

static void handler(void* arg){
    printf("this is a user-defined handler for %s\n",(char*)arg);
}

static int std_sdcard(void* arg){
    int result = -1;
    char chk_tmp[SD_CHK_BUFFER_LEN];
    FILE* fptr = fopen(SD_PATH".sd_tmp","w+");
    if(fptr == NULL){
        goto error;
    }
    result = fputs(TEST_PHRASE,fptr);
    if(result <= 0){
        goto error;
    }
    
    fflush(fptr);
    rewind(fptr);
    memset(chk_tmp,0,SD_CHK_BUFFER_LEN);
    fgets(chk_tmp,SD_CHK_BUFFER_LEN,fptr);
    
    result = strlen(chk_tmp);
    if(result == 0){
        printf("reading contents of .sd_tmp returns zero-length string!\n");
        goto error;
    }
    if(strncmp(chk_tmp,TEST_PHRASE,strlen(TEST_PHRASE)) == 0){
        fclose(fptr);
        return 0;
    }else{
        fclose(fptr);
        return 1;
    }
error:
    if(fptr != NULL){
        fclose(fptr);
        fptr = NULL;
    }
    
    return 1;
}

static int std_lan(void* arg){
    int pnum = (int)arg;
    return !isPluggedIn(pnum);
}

static int std_wifi(void* arg){
    int result = -1;
    result = system("ifconfig ra0 | grep UP 1> /dev/null");
    if(result == 0){
        return 0;
    }else{
        return 1;
    }
}

static int isExist(char* path){
    struct stat statbuf;
    if(path == NULL){
        return 0;
    }
    if(stat(path,&statbuf) != 0){
        return 0;
    }
    if(S_ISCHR(statbuf.st_mode)){
        return 1;
    }else{
        return 0;
    }
}
static int isPluggedIn(int port){
    int pnum;
    unsigned int val; 
    static char buf[CONFIG_PATH_MAX];
    char cmd[CONFIG_CMD_MAX];
    char *nl;
    FILE *fp;
    if(port < 0 || port > 3){
        fprintf(stderr,"Wrong port number~!\n");
        return 0;
    }
    snprintf(cmd,CONFIG_CMD_MAX,"mii_mgr -g -p %d -r 1",port);
    memset(buf, 0, sizeof(buf));
    
    if( (fp = popen(cmd, "r")) == NULL ){
        fprintf(stderr,"popen returns NULL!\n%s\n",strerror(errno));
        goto error;
    }
      
    if(!fgets(buf, sizeof(buf), fp)){
        fprintf(stderr,"fgets return empty string!\n");
        pclose(fp);
        goto error;
    }
  
    if(!strlen(buf)){
        fprintf(stderr,"zero length buf!\n");
        pclose(fp);
        goto error;
    }
    pclose(fp);
    sscanf(buf,"Get: phy[%d].reg[1] = %x\n",&pnum,&val);
    if((val & (1 << 2)) > 0){
        return 1;
    }else{
        return 0;
    }
    
error:
    fprintf(stderr,"warning, CANNOT check the ethernet connection status!\n");
    return 0;
}

static char* getConfig(const char* key){
    static char buf[CONFIG_PATH_MAX];
    char cmd[CONFIG_CMD_MAX];
    char *nl;
    FILE *fp;
  
    if(key == NULL){
        goto error;
    }
    snprintf(cmd,CONFIG_CMD_MAX,"nvram_get 2860 %s",key);
    memset(buf, 0, sizeof(buf));
  
    if( (fp = popen(cmd, "r")) == NULL )
        goto error;
  
    if(!fgets(buf, sizeof(buf), fp)){
        pclose(fp);
        goto error;
    }
  
    if(!strlen(buf)){
        pclose(fp);
        goto error;
    }
    pclose(fp);
  
    if(nl = strchr(buf, '\n'))
        *nl = '\0';
  
    return buf;
error:
    printf( "warning, cant find specified key!\n");
    return NULL;
}

static int std_generic(void* arg){ 
    char* key = (char*)arg;
    char* path = getConfig(key);
    return !isExist(path);
}

static void callback(long* flag){
        if(flag == NULL)
        return;
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("\nREPORT FLAG = %08lx\n",*flag);
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

static void toLowerCase(char* string){
    int i = 0;
    if(string == NULL || strlen(string) == 0)
        return;
    while(string[i] != '\0'){
        string[i] = tolower(string[i]);
        ++i;
    }
          
}

