#include <stdlib.h>
#include <nfc/nfc.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define BASE_URL_FORMAT "http://10.10.4.129/%s"
#define ADD_USER_URL_EXT_FORMAT "adduser?uid=%s&firstname=%s&lastname=%s"
#define WHAT_IS_URL_EXT_FORMAT "whatis?uid=%s"
#define URL_SIZE 256
#define JSON_BUFFER_SIZE (256 * 1024)

char *get_card_id_from_target(nfc_target);
int make_json_request(char *, char *);
void construct_what_is_url(char *, int, char *);

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

	//Set up MIFARE tag struct (ISO14443A)
	const nfc_modulation nfcModulationMifare = {
		.nmt = NMT_ISO14443A,
		.nbr = NBR_106,
	};

	//Poll for MIFARE tags forever
	nfc_target nfcTarget;
	char keyboardInput;

	while(1) {
		printf("Enter any key to scan a card, \"q\" to quit\n");
		//check for keyboard input 
		scanf("%c", &keyboardInput);
		if(keyboardInput == 'q'){
			break;
		}

		//check if we've found a device
		if (nfc_initiator_select_passive_target(nfcDevice, nfcModulationMifare, NULL, 0, &nfcTarget) > 0) {
			char *nfcID = get_card_id_from_target(nfcTarget);
			
			//if the id is invalid, loop again
			if(!nfcID) {
				printf("Did not scan properly, please try again\n");
	                        free(nfcID);
				continue;
			}
			
			printf("UID: %s\n", nfcID);
			char *url = malloc(URL_SIZE);
			construct_what_is_url(url, URL_SIZE, nfcID);
			
			char *json = malloc(JSON_BUFFER_SIZE);
			make_json_request(json, url);			

			printf("json response:\n%s", json);

			/*json_error_t error;
			json_t *root;
			root = json_loads(text, &error)
			//if json does not parse properly loop again!
			if(!root) {
    				fprintf(stderr, "json error: on line %d: %s\n", error.line, error.text);
    				continue;
			}*/
						
			free(nfcID);
			free(url);
			free(json);
			sleep(1);
		}
	}
}

void construct_what_is_url(char *urlBuf, int urlSize, char* nfcID) {
	char tmpUrl[URL_SIZE];
        snprintf(tmpUrl, urlSize, BASE_URL_FORMAT, WHAT_IS_URL_EXT_FORMAT);
        snprintf(urlBuf, urlSize, tmpUrl, nfcID);
	
	printf("Url constructed: %s\n", urlBuf);
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
