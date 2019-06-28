#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <expat.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <avahi-common/malloc.h>
#include <avahi-core/log.h>

void start(void *data, const char *el, const char **attr);
void end(void *data, const char *el);
void get_service_specific_data(char *serv_name);
void get_MACAddress();
void get_HostName();

/* Keep track of the current level in the XML tree */
int Depth;

#define MAXCHARS 100000

//Global Variables
char text_data[1024];
char *service_name = NULL;
int service_matched = 0, hostname = 0, mac_addr = 0;
FILE *fp = NULL;

void get_MACAddress() {
    fp = popen("busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/eth0 xyz.openbmc_project.Network.MACAddress MACAddress", "r");
    char* buff = avahi_malloc(sizeof(char) * 1024);
    memset(buff,0,100);
    if(fp!=NULL) {
        while ( fgets( buff, 100, fp ) != NULL ) { }
    } else
        avahi_log_error("Cannot execute busctl command to fetch MAC Address\n");

    pclose(fp);
    strcat(text_data, buff);
}

void get_HostName() {
    fp = popen("busctl get-property xyz.openbmc_project.Network /xyz/openbmc_project/network/config xyz.openbmc_project.Network.SystemConfiguration HostName", "r");
    char* buff = avahi_malloc(sizeof(char) * 1024);
    memset(buff,0,100);
    if(fp!=NULL) {
        while ( fgets( buff, 100, fp ) != NULL ) { }
    } else
        avahi_log_error("Cannot execute busctl comman to fetch Hostname\n");

    pclose(fp);
    strcat(text_data, buff);
}

void start(void *data, const char *el, const char **attr)
{
    int i;

    for (i = 0; attr[i]; i += 2) {
        if(Depth == 1 && strcmp(service_name, attr[i + 1]) == 0){
            service_matched = 1;
        }
        else if(strcmp(service_name, attr[i + 1]) != 0 && Depth == 1){
            service_matched = 0;
        }
        else if(Depth == 2 && service_matched == 1){
            if(strcmp(attr[i + 1], "HostName") == 0)
                hostname = 1;
            if(strcmp(attr[i + 1], "MACAddress") == 0)
                mac_addr = 1;
        }
    }

    Depth++;
}

void end(void *data, const char *el)
{
    Depth--;
}

void get_service_specific_data(char *serv_name){
    memset(text_data, 0, sizeof(text_data));

    service_name = serv_name;
    FILE *f = NULL;
    size_t size;
    char *xmltext = avahi_malloc(MAXCHARS);

    XML_Parser parser;
    parser = XML_ParserCreate(NULL);
    if (parser == NULL) {
        avahi_log_error("XML Parser not created\n");
    }

    XML_SetElementHandler(parser, start, end);
    f = fopen("/etc/avahi/services/service_text_data.xml", "r");
    if (f == NULL){
        avahi_log_error("Error: %s\n", strerror(errno));
    }

    /* Slurp the XML file in the buffer xmltext */
    size = fread(xmltext, sizeof(char), MAXCHARS, f);
    if (XML_Parse(parser, xmltext, strlen(xmltext), XML_TRUE) == XML_STATUS_ERROR) {
        avahi_log_error("Cannot parse service_text_data.xml, file may be too large or not well-formed XML\n");
    }
    fclose(f);
    XML_ParserFree(parser);
    avahi_log_info("Successfully parsed %i characters in file service_text_data.xml\n", size);

    if(hostname == 1 && mac_addr == 1){
        strcat(text_data, "Hostname : ");
        get_HostName();
        strcat(text_data, ";");
        strcat(text_data, " Mac Address : ");
        get_MACAddress();
    } else if(hostname == 1) {
        strcat(text_data, "Hostname : ");
        get_HostName();
    } else if(mac_addr == 1) {
        strcat(text_data, "Mac Address : ");
        get_MACAddress();
    } else
        strcat(text_data, "");
    //avahi_log_info("%s", text_data);
}
