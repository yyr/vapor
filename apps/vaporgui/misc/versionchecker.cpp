#include "versionchecker.h"

#include <sstream>

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vapor/Version.h>
#include "vcheckgui.h"
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QMainWindow>
//#include <QMessageBox>

#ifdef	Darwin
#include <sys/utsname.h>
#endif


#ifndef WIN32
static char* const vfilepath = getenv("HOME");
#else
static char* const vfilepath = getenv("USERPROFILE");
#endif

namespace {

bool qnam_supported() {

#ifndef	Darwin
	return(true);
#else

	struct utsname myuname;

    int rc = uname(&myuname);
	if (rc<0) return(false);

    int major = 0;
    istringstream iss (myuname.release);
    iss >> major;

	// Not supported in pre Mac OSX 10.6 (uname release 10.x)
	//
	return(major>10);	
#endif
}

}


class vfiledata
{
public:
	string version;
	time_t time;
	bool optout;

	bool saveFile(char* filename)
	{
		FILE* file = fopen(filename, "w");
		if(!file) return false;

		int major, minor, mminor;
		string rc;
		VetsUtil::Version::Parse(version, major, minor, mminor, rc);
		if (! rc.empty()) {
			fprintf(file, "%d.%d.%d.%s\n%llx\n%d", major, minor, mminor, rc.c_str(), time, optout);
		}
		else {
			fprintf(file, "%d.%d.%d\n%llx\n%d", major, minor, mminor, time, optout);
		}
		fclose(file);
		return true;
	}
	bool loadFile(char* filename)
	{
		FILE* file = fopen(filename, "r");
		if(!file) return false;
		bool good = true;
		char buf[1024];
		if(fscanf(file, "%s\n%llx\n%d", buf, &time, &optout) < 3) good = false;
		version = buf;
		fclose(file);
		return good;
	}
};

//Only for initializing internal variables
VAPoR::VersionChecker::VersionChecker()
{
	//old url: "http://www.vis.ucar.edu/~milesrl/vapor-version"
	//set up state trackers
	waiting = false;
	bad = false;
	error = "";
	_manager = NULL;
}

VAPoR::VersionChecker::~VersionChecker() {
	if (_manager) delete _manager;
}

//This sends out an http request, and sets a callback so that when the request returns,
//  it will be handled by on_version_reply (another method of this class).
void VAPoR::VersionChecker::request(QString url)
{
	if (! qnam_supported()) return;

	//build the version check filename based on OS
	char* vfilename = new char[strlen(vfilepath) + strlen("/.vapor-version") + 1];
	vfilename[0] = 0;
	strcat(vfilename, vfilepath);
	strcat(vfilename, "/.vapor-version");

	//check for previous version checks
	vfiledata vfile;
	//handle timing and optout
	if(vfile.loadFile(vfilename) && (time(NULL) - vfile.time < (60 * 60 * 24 * 30))) return;
	//check our access rights for dir of version check file
	FILE* acheck = 0;
	acheck = fopen(vfilename, "a");
	delete[] vfilename;
	if(!acheck) return;
	fclose(acheck);

	//set state
	waiting = true;
	bad = false;
	//send the network request
	if (! _manager) _manager = new QNetworkAccessManager(NULL);
	connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_version_reply(QNetworkReply*)));
	_manager->get(QNetworkRequest(QUrl(url)));
}

//processes the reply to our request (here's where the actual checking and notification happens)
void VAPoR::VersionChecker::on_version_reply(QNetworkReply* reply)
{
	//get contents of reply
    QString r = reply->readAll();
    if(reply->error() != QNetworkReply::NoError)
    {
		//something went wrong with the request
        bad = true;
        error = reply->errorString();
    }
    else
    {
		//get the version from the returned string, make sure that string is properly formatted
		string webversion = r.toStdString();

		int webmajor, webminor, webmminor;
		string webrc;

		VetsUtil::Version::Parse(webversion, webmajor, webminor, webmminor, webrc);

		std::ostringstream oss;
		oss << "VAPoR version ";
		oss << webmajor;
		oss << ".";
		oss << webminor;
		oss << ".";
		oss << webmminor;
		if (! webrc.empty()) {
			oss << ".";
			oss << webrc;
		}
		oss << " is available!" ;

		string output = oss.str();

		//build the filename based on OS
		char* vfilename = new char[strlen(vfilepath) + strlen("/.vapor-version") + 1];
		vfilename[0] = 0;
		strcat(vfilename, vfilepath);
		strcat(vfilename, "/.vapor-version");

		vfiledata last;
		bool prior = true;
		//get information on the previous version check
		if(!last.loadFile(vfilename))
		{
			last.version = "";
			last.optout = false;
			last.time = 0;
			prior = false;
		}
			
		//is the user's version old?
		string myversion = VetsUtil::Version::GetVersionString();
		bool old = VetsUtil::Version::Compare(myversion, webversion) < 0;

		//is the last checked version older than the latest?
		bool checkold = VetsUtil::Version::Compare(last.version, webversion) < 0;

		//don't notify if user is up-to-date or if we already noted on this webversion
		//don't notify if the user opted out of the current version's notifications
		//writes the file either way (write file only if !prior or if we will notify user)
		if(!prior || (!last.optout && old) || (last.optout && old && checkold)) {
			last.version = webversion;
			last.time = time(NULL);
			//only notify if the version is old
			if(old)
			{
				vCheckGUI msg(output.c_str(), &last.optout);
				msg.exec();
			}
			last.saveFile(vfilename);
		}
			delete[] vfilename;
	}
    reply->deleteLater();
	waiting = false;
}
