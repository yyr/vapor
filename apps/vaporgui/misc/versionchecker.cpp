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

VAPoR::VersionChecker::VersionChecker()
{
	//old url: "http://www.vis.ucar.edu/~milesrl/vapor-version"
	//set up state trackers
	waiting = false;
	bad = false;
	error = "";
}

void VAPoR::VersionChecker::request(QString url)
{
	//build the filename based on OS
	char* vfilename = new char[strlen(vfilepath) + strlen("/.vapor-version") + 1];
	vfilename[0] = 0;
	strcat(vfilename, vfilepath);
	strcat(vfilename, "/.vapor-version");

	//check for previous
	vfiledata vfile;
	//handle timing and optout
	if(vfile.loadFile(vfilename) && (time(NULL) - vfile.time < (60 * 60 * 24 * 30) || vfile.optout == true)) return;
	//check our access rights for target dir
	FILE* acheck = 0;
	acheck = fopen(vfilename, "a");
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

void VAPoR::VersionChecker::on_version_reply(QNetworkReply* reply)
{
	//get contents of reply
    QString r = reply->readAll();
    if(reply->error() != QNetworkReply::NoError)
    {
        bad = true;
        error = reply->errorString();
    }
    else
    {
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
			if(!last.loadFile(vfilename))
			{
				last.major = 0;
				last.minor = 0;
				last.mminor = 0;
				last.optout = false;
				last.time = 0;
				prior = false;
			}
			
			//don't notify if user is up-to-date or if we already noted on this webversion
			if
			(
				// first time ever notifying user
				prior == false
				||
				( // current version less than web version, last checked less than web version
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
					)
					&& 
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
					)
				)
			)
			{
				last.major = webversion[0];
				last.minor = webversion[1];
				last.mminor = webversion[2];
				last.time = time(NULL);
				vCheckGUI msg(output, &last.optout);
				msg.exec();
				last.saveFile(vfilename);
			}
		}
    }
    reply->deleteLater();
	waiting = false;
}
