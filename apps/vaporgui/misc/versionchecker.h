#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QtNetwork/qnetworkreply.h>

/**
------------------------------------------------------------------
TO CHECK FOR UPDATES AND CONDITIONALLY NOTIFY THE USER...
==================================================================
QString version_file_url = "http://www.somewebsite.com/version-file";
VersionChecker vc = VersionChecker();
vc.request(version_file_url);

------------------------------------------------------------------
WHEN REQUEST IS CALLED...
==================================================================
if(haveCheckedBefore && currentTime() - lastCheck >= 30 days) return;

newestVersion = getLatestVersionNumber();
if(!newestVersion) return;
if(localVersion >= newestVersion) return;

if(userOptedOut && getLastCheckedVersion >= newestVersion) return;

showNotification(newestVersion);
saveInfoAboutThisCheck(newestVersion, currentTime(), userOptedOut);

 */

namespace VAPoR
{
class VersionChecker : QObject
{
	Q_OBJECT
public:
	VersionChecker();
	//letting C++ generate automatic destructor
	//since this class has no special members

	//sends a network request for the given url. the target file should have a version number "x.y.z"
	//the reply is handled by on_version_reply. if appropriate, on_version_reply will notify the user.
	//on_version_reply handles most of the conditional logic on notifying the user, as well as file IO
	void request(QString url);
	
	//are we waiting for a response?
	inline bool isWaiting(){return waiting;}
	//is there a problem?
	inline bool hasError(){return bad;}
	//gives an error message if problem, else an empty string
	inline QString errorString(){return error;}

private:
    bool waiting;
	bool bad;
	QString error;

private slots:
    //gets the response, notifies user if there's a new version
    void on_version_reply(QNetworkReply* reply);
};

}

#endif //VERSIONCHECKER_H