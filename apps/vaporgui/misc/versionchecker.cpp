#include "versionchecker.h"

#include <QtNetwork/qnetworkaccessmanager.h>
#include <QMainWindow>
//#include <QMessageBox>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vapor/Version.h>
#include "vcheckgui.h"

#ifndef WIN32
static char* const vfilepath = getenv("HOME");
#else
static char* const vfilepath = getenv("USERPROFILE");
#endif

class vfiledata
{
public:
	int major;
	int minor;
	int mminor;
	time_t time;
	bool optout;

	bool saveFile(char* filename)
	{
		FILE* file = fopen(filename, "w");
		if(!file) return false;
		fprintf(file, "%d.%d.%d\n%llx\n%d", major, minor, mminor, time, optout);
		fclose(file);
		return true;
	}
	bool loadFile(char* filename)
	{
		FILE* file = fopen(filename, "r");
		if(!file) return false;
		bool good = true;
		if(fscanf(file, "%d.%d.%d\n%llx\n%d", &major, &minor, &mminor, &time, &optout) < 5) good = false;
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
}

//This sends out an http request, and sets a callback so that when the request returns,
//  it will be handled by on_version_reply (another method of this class).
void VAPoR::VersionChecker::request(QString url)
{
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
	QNetworkAccessManager* manager = new QNetworkAccessManager(NULL);
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_version_reply(QNetworkReply*)));
	manager->get(QNetworkRequest(QUrl(url)));
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
        QStringList tok = r.split('.');
        bool ok = false;
		int webversion[3];
        if(tok.length() == 3)
        {
            for(int i = 0; i < tok.length(); i++)
            {
                webversion[i] = tok.at(i).toInt(&ok, 10);
                if(!ok) break;
            }
        }
        if(!ok)
        {
			//the string was not properly formatted
			bad = true;
            error = "The web version file contained a bogus version number";
        }
        else
		{
			char* output = new char[strlen("VAPoR version [11.11.11] is available!n")];
			sprintf(output, "VAPoR version [%d.%d.%d] is available!", webversion[0], webversion[1], webversion[2]);
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
				last.major = 0;
				last.minor = 0;
				last.mminor = 0;
				last.optout = false;
				last.time = 0;
				prior = false;
			}
			
			//is the user's version old?
			bool old = 
			(
				VetsUtil::Version::GetMajor() < webversion[0]
				||
				(
					VetsUtil::Version::GetMajor() == webversion[0]
					&&
					(
						VetsUtil::Version::GetMinor() < webversion[1]
						||
						(
							VetsUtil::Version::GetMinor() == webversion[1]
							&&
							VetsUtil::Version::GetMinorMinor() < webversion[2]
						)
					)
				)
			);

			//is the last checked version older than the latest?
			bool checkold = 
			(
				last.major < webversion[0]
				||
				(
					last.major == webversion[0]
					&&
					(
						last.minor < webversion[1]
						||
						(
							last.minor == webversion[1]
							&&
							last.mminor < webversion[2]
						)
					)
				)
			);

			//don't notify if user is up-to-date or if we already noted on this webversion
			//don't notify if the user opted out of the current version's notifications
			//writes the file either way (write file only if !prior or if we will notify user)
			if(!prior || (!last.optout && old) || (last.optout && old && checkold))
			{
				last.major = webversion[0];
				last.minor = webversion[1];
				last.mminor = webversion[2];
				last.time = time(NULL);
				//only notify if the version is old
				if(old)
				{
					vCheckGUI msg(output, &last.optout);
					msg.exec();
				}
				last.saveFile(vfilename);
			}
			delete[] output;
			delete[] vfilename;
		}
    }
    reply->deleteLater();
	waiting = false;
}
