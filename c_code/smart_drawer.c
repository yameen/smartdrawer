#include <stdlib.h>
#include <nfc/nfc.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define BASE_URL_FORMAT "http://10.10.4.129/%s"
#define WHAT_IS_URL_EXT_FORMAT "whatis?uid=%s"
#define ADD_USER_URL_EXT_FORMAT "adduser?uid=%s&firstname=%s&surname=%s"
#define ADD_DEVICE_URL_EXT_FORMAT "adddevice?uid=%s&name=%s&description=%s"
#define ASSIGN_DEVICE_URL_EXT_FORMAT "assigndevice?userid=%s&deviceid=%s"
#define UNASSIGN_DEVICE_URL_EXT_FORMAT "unassigndevice?deviceid=%s"

#define URL_SIZE 256
#define JSON_BUFFER_SIZE (256 * 1024)

char *get_card_id_from_target(nfc_target);
int make_json_request(char *, char *);
void construct_what_is_url(char *, int, char *);
void construct_add_device_url(char *, int, char *, char *, char *);
void construct_assign_device_url(char *, int, char *, char *);
void construct_unassign_device_url(char *, int, char *);
void assign_device(char *, char *);
void unassign_device(char *);
void add_user(char *);
void add_device(char *);
void promptScanCardNFC(); 
void unassign_device_logic(json_t *, char *);
void promptThisIsAssignedToReturn(char *, char *);
void promptDeviceHasBeenAssignedTo(char *, char *);
void promptDeviceHasBeenUnassigned();
void promptDeviceHasBeenAdded();
void promptUserHasBeenAdded();

int main(int argc, const char *argv[]) {
	//initialise libcurl

	curl_global_init(CURL_GLOBAL_ALL);

	nfc_device *nfcDevice;
	
	//create NFC context and initialise
	nfc_context *nfcContext;
	nfc_init(&nfcContext);
	//check if init was successful
	if (nfcContext == NULL){
		printf("Unable to init libnfc (malloc)\n");
	    	exit(EXIT_FAILURE);
	}
	//open device based on context
	nfcDevice = nfc_open(nfcContext, NULL);
	if (nfcDevice == NULL) {
		printf("ERROR: %s\n", "Unable to open NFC device.");
		exit(EXIT_FAILURE);
	}

	//Set device to initiator mode
	if (nfc_initiator_init(nfcDevice) < 0) {
		nfc_perror(nfcDevice, "nfc_initiator_init");
		exit(EXIT_FAILURE);
	}

	printf("NFC reader: %s opened\n", nfc_device_get_name(nfcDevice));

	//Poll for MIFARE tags forever
	nfc_target nfcTarget;
	char keyboardInput;
	const uint8_t uiPollNr = 20;
  	const uint8_t uiPeriod = 2;
	
	const nfc_modulation nmModulations[1] = {
		{.nmt = NMT_ISO14443A, .nbr = NBR_106},
	};
	const size_t szModulations = 1;

	while(1) {
		//If polling fails to find a device, nfcDevice throws an error.
		//To hide this (and to tidy up generally), we'll clear the screen
		system("clear");
		//I'm not saying this is good programming, but hey, this is a Make
		promptScanCardNFC();

		//check if we've found a device
		if (nfc_initiator_poll_target(nfcDevice, nmModulations, szModulations, uiPollNr, uiPeriod, &nfcTarget) > 0) {
			system("clear");
			char *nfcID = get_card_id_from_target(nfcTarget);
			
			//if the id is invalid, loop again
			if(!nfcID) {
				printf("Did not scan properly, please try again\n");
	                        free(nfcID);
				sleep(2);
				continue;
			}
			
			printf("UID: %s\n", nfcID);
			char *url = malloc(URL_SIZE);
			construct_what_is_url(url, URL_SIZE, nfcID);
			
			char *json = malloc(JSON_BUFFER_SIZE);
			make_json_request(json, url);			

			json_error_t error;
			json_t *root;
			root = json_loads(json, 0, &error);
			//if json does not parse properly loop again!
			if(!root) {
    				fprintf(stderr, "json error: on line %d: %s\n", error.line, error.text);
    				printf("Malformed json. Please try again. If it still doesn't work, blame Yameen.\n");
				sleep(2);
				continue;
			}

			json_t *nfcType;
			nfcType = json_object_get(root, "type");
			
			if(!json_is_string(nfcType)) {
    				printf("Malformed json. Please try again. If it still doesn't work, blame Yameen.\n");
				sleep(2);
				continue;
			}
			
			if (strcmp(json_string_value(nfcType), "person") == 0 ){
				json_t *element;
				element = json_object_get(root, "element");
				if(json_is_object(element)){
					json_t *firstname = json_object_get(element, "firstname");
					if (json_is_string(firstname)){
						printf("Hello, %s\n", json_string_value(firstname));
						sleep(1);
						printf("If you want to check out a device, please scan it now\n\t--OR, if you want to cancel, just wait a few seconds.\n");
						if (nfc_initiator_poll_target(nfcDevice, nmModulations, szModulations, uiPollNr, uiPeriod, &nfcTarget) > 0) {
                                        		char *deviceUID = get_card_id_from_target(nfcTarget);
							
							if (verify_device(deviceUID) == 0) {
								assign_device(nfcID, deviceUID);
							} else {
								printf("Not recognised as a registered device.\n");
							}
	
						} else {
							system("clear");
							printf("Either you decided to cancel, or there was a read error.\nPlease start again\n");
						}
					}
				}
				sleep(3);

			} else if (strcmp(json_string_value(nfcType), "device") == 0 ) {
				printf("Device found!\n");
				printf("Unassigning device with ID: %s\n", nfcID);
				unassign_device_logic(root, nfcID);

			} else if (strcmp(json_string_value(nfcType), "addPersonCard") == 0 ) {
				printf("Add person token detected.\nPlease scan your BBC pass.\n");
				sleep(1);
				if (nfc_initiator_poll_target(nfcDevice, nmModulations, szModulations, uiPollNr, uiPeriod, &nfcTarget) > 0) {
					printf("id: %s\n", get_card_id_from_target(nfcTarget));
					nfcID = get_card_id_from_target(nfcTarget);
					add_user(nfcID);
				} else {
					system("clear");
					printf("Either you took too long to scan your card, or there was a read error.\nPlease start again\n");
					sleep(2);
				}

			} else if (strcmp(json_string_value(nfcType), "addDeviceCard") == 0 ) {
				printf("Add device token detected.\nPlease scan the device.\n");
				sleep(1);
				if (nfc_initiator_poll_target(nfcDevice, nmModulations, szModulations, uiPollNr, uiPeriod, &nfcTarget) > 0) {
					printf("id: %s\n", get_card_id_from_target(nfcTarget));
					nfcID = get_card_id_from_target(nfcTarget);
					add_device(nfcID);
				} else {
					system("clear");
					printf("Either you took too long to scan the device, or there was a read error.\nPlease start again\n");
					sleep(2);
				}

			} else if (strcmp(json_string_value(nfcType), "unknown") == 0 ){
				//throw error
				printf("Sorry, this is an unknown card type. Please start again.\n");

			} else {
				//throw error	
				printf("Sorry, this is an unknown card type. Please start again.\n");
			}

			free(nfcID);
			free(url);
			free(json);
			sleep(1);
		}
	}
}

//get user input. Stolen shamelessly from stack overflow
#define OK       0
#define NO_INPUT 1
#define TOO_LONG 2
static int getLine (char *prmpt, char *buff, size_t sz) {
	int ch, extra;

    // Get line with buffer overrun protection.
    if (prmpt != NULL) {
	printf ("%s", prmpt);
	fflush (stdout);
    }
    if (fgets (buff, sz, stdin) == NULL)
        return NO_INPUT;
    // If it was too long, there'll be no newline. In that case, we flush
    // to end of line so that excess doesn't affect the next call.
    if (buff[strlen(buff)-1] != '\n') {
	extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? TOO_LONG : OK;
    }

    // Otherwise remove newline and give string back to caller.
    buff[strlen(buff)-1] = '\0';
    return OK;
}

int verify_device(char *deviceUID) {

	char *url = malloc(URL_SIZE);
	construct_what_is_url(url, URL_SIZE, deviceUID);
	
	char *json = malloc(JSON_BUFFER_SIZE);
	make_json_request(json, url);			

	json_error_t error;
	json_t *root;
	root = json_loads(json, 0, &error);
	
	json_t *nfcType;
	nfcType = json_object_get(root, "type");
	
	int returnVal = 1;
		
	if (strcmp(json_string_value(nfcType), "device") == 0 ){
		returnVal = 0;
	}

	free(url);
	free(json);

	return returnVal;
}

void construct_add_user_url(char *urlBuf, int urlSize, char *uid, char *firstName, char *lastName) {
	char *escapedFirstname = curl_escape(firstName, 0);
	char *escapedSurname = curl_escape(lastName, 0);
	char tmpUrl[URL_SIZE];

	snprintf(tmpUrl, urlSize, BASE_URL_FORMAT, ADD_USER_URL_EXT_FORMAT);
	snprintf(urlBuf, urlSize, tmpUrl, uid, escapedFirstname, escapedSurname);
	
	free(escapedFirstname);
	free(escapedSurname);
}

void add_user(char *userID) {
	int returnCode;
	int triesRemaining = 3;
	
	printf("Adding user with ID: %s\n", userID);		

	//get firstname	
	char firstName[20];
	do {
		returnCode = getLine("Please enter your first name =>\n", firstName, sizeof(firstName));
		triesRemaining--;
	} while ( ((returnCode == 1) || (returnCode == 2)) && (triesRemaining > 0)); 
	
	if (strcmp(firstName, "cancel") == 0) return;

	//get surname	
	char surname[20];
	triesRemaining = 3;

	do {
		returnCode = getLine("Please enter your surname =>\n", surname, sizeof(surname));
		triesRemaining--;
	} while ( ((returnCode == 1) || (returnCode == 2)) && (triesRemaining > 0)); 
	
	if (strcmp(surname, "cancel") == 0) return;

	//construct url
	char *url = malloc(URL_SIZE);
	construct_add_user_url(url, URL_SIZE, userID, firstName, surname);
	//hit url	

	char *json = malloc(JSON_BUFFER_SIZE);
	make_json_request(json, url);			
	
	json_error_t error;
	json_t *root;
	root = json_loads(json, 0, &error);
	
	json_t *jsonElement;
	jsonElement = json_object_get(root, "error");
	
	if(json_is_false(jsonElement)){
		promptUserHasBeenAdded();
	} else {
		jsonElement = json_object_get(root, "message");
                printf("Oh no! Failed to add user!\n");
                printf("Reason:\n");
                printf("%s\n", json_string_value(jsonElement));
	}
	
	free(url);
	free(json);
	sleep(2);
}

void construct_add_device_url(char *urlBuf, int urlSize, char *uid, char *deviceName, char *deviceDescription) {
	char *escapedUID = curl_escape(uid, 0);
	char *escapedDeviceName = curl_escape(deviceName, 0);
	char *escapedDeviceDescription = curl_escape(deviceDescription, 0);
	char tmpUrl[URL_SIZE];

	snprintf(tmpUrl, urlSize, BASE_URL_FORMAT, ADD_DEVICE_URL_EXT_FORMAT);
	snprintf(urlBuf, urlSize, tmpUrl, escapedUID, escapedDeviceName, escapedDeviceDescription);
	
	system("clear");
	promptDeviceHasBeenAdded();
	
	free(escapedUID);
	free(escapedDeviceName);
	free(escapedDeviceDescription);
}

void add_device(char *nfcID) {
	int returnCode;
	int triesRemaining = 3;
	
	printf("Add device token detected.\nAdding device!\n");

	//get device name	
	char deviceName[20];
	do {
		returnCode = getLine("Please enter a name for your device=>\n", deviceName, sizeof(deviceName));
		triesRemaining--;
	} while ( ((returnCode == 1) || (returnCode == 2)) && (triesRemaining > 0)); 
	
	if (strcmp(deviceName, "cancel") == 0) return;

	//get device description
	char deviceDescription[30];
	triesRemaining = 3;

	do {
		returnCode = getLine("Please enter a (short!) description for your device =>\n", deviceDescription, sizeof(deviceDescription));
		triesRemaining--;
	} while ( ((returnCode == 1) || (returnCode == 2)) && (triesRemaining > 0)); 
	
	if (strcmp(deviceDescription, "cancel") == 0) return;

	//construct url
	char *url = malloc(URL_SIZE);
	construct_add_device_url(url, URL_SIZE, nfcID, deviceName, deviceDescription);
	
	//hit url	
	char *json = malloc(JSON_BUFFER_SIZE);
	make_json_request(json, url);			
	
	json_error_t error;
	json_t *root;
	root = json_loads(json, 0, &error);
	
	json_t *jsonElement;
	jsonElement = json_object_get(root, "error");
	
	if(json_is_false(jsonElement)){
                printf("Successfully added device \"%s\"!\n", deviceName);
	} else {
		jsonElement = json_object_get(root, "message");
                printf("Oh no! Failed to add device!\n");
                printf("Reason:\n");
                printf("%s\n", json_string_value(jsonElement));
	}
	
	free(url);
	free(json);
}

void construct_assign_device_url(char *urlBuf, int urlSize, char *userID, char *deviceUID) {	
	char *escapedUserID = curl_escape(userID, 0);
	char *escapedDeviceUID = curl_escape(deviceUID, 0);
	char tmpUrl[URL_SIZE];

	snprintf(tmpUrl, urlSize, BASE_URL_FORMAT, ASSIGN_DEVICE_URL_EXT_FORMAT);
	snprintf(urlBuf, urlSize, tmpUrl, escapedUserID, escapedDeviceUID);

	free(escapedUserID);
	free(escapedDeviceUID);
}

void assign_device(char *userID, char *deviceUID) {
	printf("Assigning device!\n");

	char *url = malloc(URL_SIZE);
	construct_assign_device_url(url, URL_SIZE, userID, deviceUID);

	//hit url	
	char *json = malloc(JSON_BUFFER_SIZE);
	make_json_request(json, url);			
	
	json_error_t error;
	json_t *root;
	root = json_loads(json, 0, &error);
	
	json_t *jsonElement;
	jsonElement = json_object_get(root, "error");


	
	if(json_is_false(jsonElement)){
        	construct_what_is_url(url, URL_SIZE, userID);

        	make_json_request(json, url);

        	root = json_loads(json, 0, &error);

        	jsonElement = json_object_get(root, "element");

        	promptDeviceHasBeenAssignedTo((char *)json_string_value(json_object_get(jsonElement, "firstname")), (char *)json_string_value(json_object_get(jsonElement, "surname")));	

	} else {
		jsonElement = json_object_get(root, "message");
                printf("Oh no! Failed to assign device!\n");
                printf("Reason:\n");
                printf("%s\n", json_string_value(jsonElement));
	}
	
	free(url);
	free(json);
}

void construct_unassign_device_url(char *urlBuf, int urlSize, char *deviceUID) {
	char *escapedDeviceUID = curl_escape(deviceUID, 0);
	char tmpUrl[URL_SIZE];

	snprintf(tmpUrl, urlSize, BASE_URL_FORMAT, UNASSIGN_DEVICE_URL_EXT_FORMAT);
	snprintf(urlBuf, urlSize, tmpUrl, escapedDeviceUID);

	free(escapedDeviceUID);
}

void unassign_device(char *nfcID) {	

	//construct url
	char *url = malloc(URL_SIZE);
	construct_unassign_device_url(url, URL_SIZE, nfcID);

	//hit url	
	char *json = malloc(JSON_BUFFER_SIZE);
	make_json_request(json, url);

	system("clear");
	promptDeviceHasBeenUnassigned();
	free(json);	
			
	free(url);
	//todo error handling? json response? response code?		
}

void unassign_device_logic(json_t *whatIsDeviceRoot, char *deviceUID) {
	json_t *jsonElement;
	jsonElement = json_object_get(whatIsDeviceRoot, "element");
	
	json_t *userID;
	userID = json_object_get(jsonElement, "userid");

	if (json_is_null(userID)) {
		printf("Device isn't assigned to anyone!\n");
		return;
	} 
		
	char *url = malloc(URL_SIZE);
	construct_what_is_url(url, URL_SIZE, (char *)json_string_value(userID));

	char *json = malloc(JSON_BUFFER_SIZE);
	make_json_request(json, url);			

	json_error_t error;
	json_t *root;
	root = json_loads(json, 0, &error);

	json_t *element;
	element = json_object_get(root, "element");

	promptThisIsAssignedToReturn((char *)json_string_value(json_object_get(element, "firstname")), (char *)json_string_value(json_object_get(element, "surname")));
	//printf("Device is assigned to %s %s\n\n", (char *)json_string_value(json_object_get(element, "firstname")), (char *)json_string_value(json_object_get(element, "surname")));
	
	//printf("Press y to unassign or n to cancel\n");

	char choice;
	scanf(" %c", &choice);
	
	if (choice == 'y') {
		unassign_device(deviceUID);
	} else {
		printf("cancelling...\n");
	}
}

void construct_what_is_url(char *urlBuf, int urlSize, char* nfcID) {
	char *escapedUID = curl_escape(nfcID, 0);
	char tmpUrl[URL_SIZE];
        snprintf(tmpUrl, urlSize, BASE_URL_FORMAT, WHAT_IS_URL_EXT_FORMAT);
        snprintf(urlBuf, urlSize, tmpUrl, nfcID);
	
	free(escapedUID);
}

//Memory is dynamically allocated. Remember to free the string later on.
char* get_card_id_from_target(nfc_target nfcTarget) {
	int size = 3+nfcTarget.nti.nai.szUidLen;
	//each byte is written as 2 characters, plus the null terminator
	char *card_id = (char*) malloc ((2*size+1) * (sizeof(char))) ;
	char *card_id_ptr = card_id;

	int i;
	//grab the ATQA
	for(i = 0; i < 2; i++){
		card_id_ptr += sprintf(card_id_ptr, "%02X",nfcTarget.nti.nai.abtAtqa[i]); 
	}
	//concatenate the SAK
	card_id_ptr += sprintf(card_id_ptr, "%02X",nfcTarget.nti.nai.btSak); 
	//concatenate the UID
	for(i = 0; i < nfcTarget.nti.nai.szUidLen; i++){
		card_id_ptr += sprintf(card_id_ptr, "%02X",nfcTarget.nti.nai.abtUid[i]); 
	}
	return card_id;
}

struct jsonResult {
	char *data;
	int pos;
};

static size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
	struct jsonResult *result = (struct jsonResult *)stream;

	if(result->pos + size * nmemb >= JSON_BUFFER_SIZE - 1) {
		fprintf(stderr, "error: too small buffer\n");
        	return 0;
    	}
	memcpy(result->data + result->pos, ptr, size * nmemb);
	result->pos += size * nmemb;
	return size * nmemb;
}

//returns 1 if request failed, 0 for success.
//Doesn't take the size of the json buffer into account as the writing is handled by the curl_callback function,
//and it is not simple to pass this value through.
int make_json_request(char* jsonBuf, char* url) {
	CURL *curl;
	CURLcode res;
	char *data = NULL; 	

	curl = curl_easy_init();
	if(curl) {	
		struct jsonResult jsonResult = {
			.data = jsonBuf,
			.pos = 0
		};

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback); 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResult);

		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
              		curl_easy_strerror(res));
			return 1;
		}
		    		
		//null terminate json
		jsonBuf[jsonResult.pos] = '\0';
		
		curl_easy_cleanup(curl);
	}
	return 0;
}
 
void promptScanCardNFC() {
	printf("   _____                 \n");
	printf("  / ____|                \n");
	printf(" | (___   ___ __ _ _ __  \n");
	printf("  \\___ \\ / __/ _` | '_ \\ \n");
	printf("  ____) | (_| (_| | | | |\n");
	printf(" |_____/ \\___\\__,_|_| |_|\n");
	printf("                         \n");
	printf("                         \n");
	printf("  _____ _____                       _____             _          \n");
	printf(" |_   _|  __ \\                     |  __ \\           (_)         \n");
	printf("   | | | |  | |      ___  _ __     | |  | | _____   ___  ___ ___ \n");
	printf("   | | | |  | |     / _ \\| '__|    | |  | |/ _ \\ \\ / / |/ __/ _ \\\n");
	printf("  _| |_| |__| |    | (_) | |       | |__| |  __/\\ V /| | (_|  __/\n");
	printf(" |_____|_____/      \\___/|_|       |_____/ \\___| \\_/ |_|\\___\\___|\n");
	printf("                                                                 \n");
	printf("                                                                 \n");
}

void promptThisIsAssignedToReturn(char *firstName, char *surname) {
	printf("\n");
	printf("  _______ _     _              _            _          \n");
	printf(" |__   __| |   (_)            | |          (_)         \n");
	printf("    | |  | |__  _ ___       __| | _____   ___  ___ ___ \n");
	printf("    | |  | '_ \\| / __|     / _` |/ _ \\ \\ / / |/ __/ _ \\\n");
	printf("    | |  | | | | \\__ \\    | (_| |  __/\\ V /| | (_|  __/\n");
	printf("    |_|  |_| |_|_|___/     \\__,_|\\___| \\_/ |_|\\___\\___|\n");
	printf("                                                                      \n");
	printf("  _                        _                      _      _            \n");
	printf(" (_)                      (_)                    | |    | |         _ \n");
	printf("  _ ___       __ _ ___ ___ _  __ _ _ __   ___  __| |    | |_ ___   (_)\n");
	printf(" | / __|     / _` / __/ __| |/ _` | '_ \\ / _ \\/ _` |    | __/ _ \\     \n");
	printf(" | \\__ \\    | (_| \\__ \\__ \\ | (_| | | | |  __/ (_| |    | || (_) |  _ \n");
	printf(" |_|___/     \\__,_|___/___/_|\\__, |_| |_|\\___|\\__,_|     \\__\\___/  (_)\n");
	printf("                              __/ |                                   \n");
	printf("                             |___/                                    \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("**********************************************************************\n");
	printf("                                  %s %s                                \n", firstName, surname);
	printf("**********************************************************************\n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("  _____      _                         _             _____                                ___  \n");
	printf(" |  __ \\    | |                       | |           |  __ \\                              |__ \\ \n");
	printf(" | |__) |___| |_ _   _ _ __ _ __      | |_ ___      | |  | |_ __ __ ___      _____ _ __     ) |\n");
	printf(" |  _  // _ \\ __| | | | '__| '_ \\     | __/ _ \\     | |  | | '__/ _` \\ \\ /\\ / / _ \\ '__|   / / \n");
	printf(" | | \\ \\  __/ |_| |_| | |  | | | |    | || (_) |    | |__| | | | (_| |\\ V  V /  __/ |     |_|  \n");
	printf(" |_|  \\_\\___|\\__|\\__,_|_|  |_| |_|     \\__\\___/     |_____/|_|  \\__,_| \\_/\\_/ \\___|_|     (_)  \n");
	printf("   __               __          __  \n");
	printf("  / /              / /          \\ \\ \n");
	printf(" | |    _   _     / /   _ __     | |\n");
	printf(" | |   | | | |   / /   | '_ \\    | |\n");
	printf(" | |   | |_| |  / /    | | | |   | |\n");
	printf(" | |    \\__, | /_/     |_| |_|   | |\n");
	printf("  \\_\\    __/ |                  /_/ \n");
	printf("        |___/                       \n");                                                                                                                        
}

void promptDeviceHasBeenAssignedTo(char *firstName, char *surname) {
	printf("  _____             _               _                    _                     \n");
	printf(" |  __ \\           (_)             | |                  | |                    \n");
	printf(" | |  | | _____   ___  ___ ___     | |__   __ _ ___     | |__   ___  ___ _ __  \n");
	printf(" | |  | |/ _ \\ \\ / / |/ __/ _ \\    | '_ \\ / _` / __|    | '_ \\ / _ \\/ _ \\ '_ \\ \n");
	printf(" | |__| |  __/\\ V /| | (_|  __/    | | | | (_| \\__ \\    | |_) |  __/  __/ | | |\n");
	printf(" |_____/ \\___| \\_/ |_|\\___\\___|    |_| |_|\\__,_|___/    |_.__/ \\___|\\___|_| |_|\n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                _                      _     _            \n");
	printf("               (_)                    | |   | |         _ \n");
	printf("   __ _ ___ ___ _  __ _ _ __   ___  __| |   | |_ ___   (_)\n");
	printf("  / _` / __/ __| |/ _` | '_ \\ / _ \\/ _` |   | __/ _ \\     \n");
	printf(" | (_| \\__ \\__ \\ | (_| | | | |  __/ (_| |   | || (_) |  _ \n");
	printf("  \\__,_|___/___/_|\\__, |_| |_|\\___|\\__,_|    \\__\\___/  (_)\n");
	printf("                   __/ |                                  \n");
	printf("                  |___/                                   \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("**********************************************************************\n");
	printf("                               %s %s                                  \n", firstName, surname);
	printf("**********************************************************************\n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");

}

void promptDeviceHasBeenUnassigned() {
	printf("  _____             _               _                    _                     \n");
	printf(" |  __ \\           (_)             | |                  | |                    \n");
	printf(" | |  | | _____   ___  ___ ___     | |__   __ _ ___     | |__   ___  ___ _ __  \n");
	printf(" | |  | |/ _ \\ \\ / / |/ __/ _ \\    | '_ \\ / _` / __|    | '_ \\ / _ \\/ _ \\ '_ \\ \n");
	printf(" | |__| |  __/\\ V /| | (_|  __/    | | | | (_| \\__ \\    | |_) |  __/  __/ | | |\n");
	printf(" |_____/ \\___| \\_/ |_|\\___\\___|    |_| |_|\\__,_|___/    |_.__/ \\___|\\___|_| |_|\n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("  _    _                     _                      _    _ \n");
	printf(" | |  | |                   (_)                    | |  | |\n");
	printf(" | |  | |_ __   __ _ ___ ___ _  __ _ _ __   ___  __| |  | |\n");
	printf(" | |  | | '_ \\ / _` / __/ __| |/ _` | '_ \\ / _ \\/ _` |  | |\n");
	printf(" | |__| | | | | (_| \\__ \\__ \\ | (_| | | | |  __/ (_| |  |_|\n");
	printf("  \\____/|_| |_|\\__,_|___/___/_|\\__, |_| |_|\\___|\\__,_|  (_)\n");
	printf("                                __/ |                      \n");
	printf("                               |___/                       \n");

}

void promptDeviceHasBeenAdded() {
	printf("  _____             _               _                    _                     \n");
	printf(" |  __ \\           (_)             | |                  | |                    \n");
	printf(" | |  | | _____   ___  ___ ___     | |__   __ _ ___     | |__   ___  ___ _ __  \n");
	printf(" | |  | |/ _ \\ \\ / / |/ __/ _ \\    | '_ \\ / _` / __|    | '_ \\ / _ \\/ _ \\ '_ \\ \n");
	printf(" | |__| |  __/\\ V /| | (_|  __/    | | | | (_| \\__ \\    | |_) |  __/  __/ | | |\n");
	printf(" |_____/ \\___| \\_/ |_|\\___\\___|    |_| |_|\\__,_|___/    |_.__/ \\___|\\___|_| |_|\n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("              _     _          _    _ \n");
	printf("     /\\      | |   | |        | |  | |\n");
	printf("    /  \\   __| | __| | ___  __| |  | |\n");
	printf("   / /\\ \\ / _` |/ _` |/ _ \\/ _` |  | |\n");
	printf("  / ____ \\ (_| | (_| |  __/ (_| |  |_|\n");
	printf(" /_/    \\_\\__,_|\\__,_|\\___|\\__,_|  (_)\n");

}

void promptUserHasBeenAdded() {
	printf("  _    _                    _                    _                     \n");
	printf(" | |  | |                  | |                  | |                    \n");
	printf(" | |  | |___  ___ _ __     | |__   __ _ ___     | |__   ___  ___ _ __  \n");
	printf(" | |  | / __|/ _ \\ '__|    | '_ \\ / _` / __|    | '_ \\ / _ \\/ _ \\ '_ \\ \n");
	printf(" | |__| \\__ \\  __/ |       | | | | (_| \\__ \\    | |_) |  __/  __/ | | |\n");
	printf("  \\____/|___/\\___|_|       |_| |_|\\__,_|___/    |_.__/ \\___|\\___|_| |_|\n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("                                                                      \n");
	printf("              _     _          _    _ \n");
	printf("     /\\      | |   | |        | |  | |\n");
	printf("    /  \\   __| | __| | ___  __| |  | |\n");
	printf("   / /\\ \\ / _` |/ _` |/ _ \\/ _` |  | |\n");
	printf("  / ____ \\ (_| | (_| |  __/ (_| |  |_|\n");
	printf(" /_/    \\_\\__,_|\\__,_|\\___|\\__,_|  (_)\n");                                                                       
                                                                       
}
