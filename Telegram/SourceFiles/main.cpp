/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/launcher.h"

//begin i2pgram changes

#include <fstream>
#include <string.h>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

//#include <filesystem>
#include <experimental/filesystem>

//_dcs
#include "config.h"

//pub keys
#include "mtproto/dc_options.h"

//namespace fs = std::filesystem;
namespace fs = std::experimental::filesystem;

//constexpr auto I2PGRAM_CONF_DEFAULT_FILE_NAME = "i2pgram.conf";

//constexpr auto appName = "i2pgram";


//static std::string configDir = "";

/*std::string GetDefaultI2pGramConfigDir() {
#if defined(WIN32) || defined(_WIN32)
		char localAppData[MAX_PATH];

		// check executable directory first
		if(!GetModuleFileName(NULL, localAppData, MAX_PATH))
		{
#if defined(WIN32_APP)
			MessageBox(NULL, TEXT("Unable to get application path!"), TEXT("i2pgram: error"), MB_ICONERROR | MB_OK);
#else
			fprintf(stderr, "Error: Unable to get application path!");
#endif
			exit(1);
		}
		else
		{
			auto execPath = boost::filesystem::path(localAppData).parent_path();

			// if config file exists in .exe's folder use it
			if(boost::filesystem::exists(execPath/I2PGRAM_CONF_DEFAULT_FILE_NAME))
				configDir = execPath.string ();
			else // otherwise %appdata%
			{
				if(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, localAppData) != S_OK)
				{
#if defined(WIN32_APP)
					MessageBox(NULL, TEXT("Unable to get AppData path!"), TEXT("i2pgram: error"), MB_ICONERROR | MB_OK);
#else
					fprintf(stderr, "Error: Unable to get AppData path!");
#endif
					exit(1);
				}
				else
					configDir = std::string(localAppData) + "\\" + appName;
			}
		}
		return;
#elif defined(MAC_OSX)
		char *home = getenv("HOME");
		configDir = (home != NULL && strlen(home) > 0) ? home : "";
		configDir += "/Library/Application Support/" + appName;
		return;
#else // other unix 
#if defined(ANDROID)
		const char * ext = getenv("EXTERNAL_STORAGE");
		if (!ext) ext = "/sdcard";
		if (boost::filesystem::exists(ext))
		{
			configDir = std::string (ext) + "/" + appName;
			return;
		}
#endif
		// otherwise use /data/files
		char *home = getenv("HOME");
		if (home != NULL && strlen(home) > 0) {
			configDir = std::string(home) + "/." + appName;
		} else {
			configDir = "/tmp/" + appName;
		}
		return;
#endif
}*/

std::string readFile(fs::path path)
{
    // Open the stream to 'lock' the file.
    std::ifstream f(path, std::ios::in | std::ios::binary);

    // Obtain the size of the file.
    const auto sz = fs::file_size(path);

    // Create a buffer.
    std::string result(sz, '\0');

    // Read the whole file into the buffer.
    f.read(&result[0], sz);

    return result;
}

void print_usage() {
	puts("i2pgram.\noptions: --dchost= --dcport= --dcpubkey0file= --dcpubkey1file=\npublic keys must be in -----BEGIN RSA PUBLIC KEY----- format.\n");
}

static Dc* _dcs=nullptr;
static int _dcsCount=0;

Dc *dcs() {
	return cTestMode() ? _testDcs : _dcs;
}

int dcsCount() {
	return cTestMode() ? sizeof(_testDcs) : _dcsCount;
}

//returns 0 iff success; !=0 - exitcode
int key(int index, std::string dcpubkey_filename) {
	if (!fs::exists(dcpubkey_filename)) {
		fprintf(stderr, "error: dc public key %d file not found at '%s'\n", index, dcpubkey_filename.c_str());
		return 1;
	}

	printf("public key %d file found at '%s', reading\n", index, dcpubkey_filename.c_str());
	std::string buf;
	try {
        buf = readFile(dcpubkey_filename);
    } catch(fs::filesystem_error& e) {
        fprintf(stderr,"error reading rsa public key %d file '%s': %s", index, dcpubkey_filename.c_str(), e.what());
		return 1;
    }

	MTP::setRSAPubKey(index, strdup(buf.c_str()));

	return 0;
}

//returns 0 on success or exitcode
//fills _dcs[0] with host and port from program options, and dc pubkey too
int parse_options_and_apply(int argc, char *argv[]) {
	std::string dc_host="127.0.0.1";
	int dc_port=0;
	std::string dcpubkey0_filename="dcpubkey0.txt";
	std::string dcpubkey1_filename="dcpubkey1.txt";

	int c;

	while (1) {
       static struct option long_options[] = {
           { "dcport",   required_argument, 0, 0 },
           { "dchost",   required_argument, 0, 0 },
           { "dcpubkey0file", required_argument, 0, 0 },
           { "help", no_argument, 0, 1 },
           { "dcpubkey1file", required_argument, 0, 0 },
           { 0, 0, 0, 0 }
       };
       int option_index = 0;
       c = getopt_long (argc, argv, "", long_options, &option_index);

       /* Detect the end of the options. */
       if (c == -1)
         break;

       switch (c) {
         case 1:
			print_usage();
			return 1;
			
         case 0:
           /* If this option set a flag, do nothing else now. */
           //if (long_options[option_index].flag != 0)
           //  break;

           printf ("--%s: ", long_options[option_index].name);
           if (optarg) {
			   printf ("with arg '%s': ", optarg); 
			   switch (option_index) {
					case 0: //dcport
						dc_port = atoi(optarg);
				        printf ("dc_port %d\n", dc_port);
						break;
					case 1: //dchost
					    printf ("dc_host `%s'\n", optarg);
					    dc_host = strdup(optarg);
						break;
					case 2: //dcpubkey0file
					    printf ("dcpubkey0 file `%s'\n", optarg);
					    dcpubkey0_filename = strdup(optarg);
						break;
					case 4: //dcpubkey1file
					    printf ("dcpubkey1 file `%s'\n", optarg);
					    dcpubkey1_filename = strdup(optarg);
						break;
				   	default: 
					   fprintf ( stderr, "bug: invalid option_index %d.\n", option_index );
					   return 1;
			   }
           } else {
			   	fprintf ( stderr, "error: no arg specified for '--%s'.\n", long_options[option_index].name );
				return 1;
		   }
           break;

         case '?':
           /* getopt_long already printed an error message. */
           break;

         default:
		    fprintf(stderr, "bug: invalid c: %d\n", (int)c);
		    return 1;
         }
     }

	if (dc_port == 0) {
		perror("error: dc_port must be specified\n");
		return 1;
	}

   /* Print any remaining command line arguments (not options). */
   if (optind < argc) {
       fprintf ( stderr, "error: non-option elements found. elements: " );
       while (optind < argc) fprintf ( stderr, "%s ", argv[optind++] );
       fprintf ( stderr, "\n");
	   return 1;
   }

	//GetDefaultI2pGramConfigDir();

	//uint16_t port

	const auto DCS_CNT=5;
	_dcs=(Dc*)malloc(DCS_CNT*sizeof(Dc));
	if (!_dcs) {
		perror("error: out of memory\n");
		return 1;
	}
	_dcsCount=DCS_CNT;

	dc_port = 0x0ffff & dc_port;

	printf("using dc: %s:%d.\n", dc_host.c_str(), (int)dc_port);

	for(int i=0;i<DCS_CNT;++i) {
		Dc* dc = &_dcs[i];
		dc->id = i + 1;
		dc->ip = strdup(dc_host.c_str());
		dc->port = dc_port;
		//printf("using dc port: %d\n", (int)dc.port);

		if(i==0) { int retcode = key(0, dcpubkey0_filename); if (retcode!=0) return retcode; }
		else { int retcode = key(i, dcpubkey1_filename); if (retcode!=0) return retcode; }
	}
	printf("starting!\n");
	return 0;
}

//end i2pgram changes

int main(int argc, char *argv[]) {
	//begin i2pgram changes
	int poresult = parse_options_and_apply(argc, argv);
	if(poresult!=0)return poresult;
	//end i2pgram changes

	const auto launcher = Core::Launcher::Create(argc, argv);
	return launcher ? launcher->exec() : 1;
}
