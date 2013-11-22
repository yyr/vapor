#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QtNetwork/qnetworkreply.h>

namespace VAPoR
{
//made a class because I need to use slots
class VersionChecker : QObject
{
	Q_OBJECT
public:
	//url is a parameter so we can change the url any time
	//the constructor will send off the http request
	VersionChecker();
	//letting C++ generate automatic destructor
	//since this class has no special members
	
	//are we waiting for a response?
	inline bool isWaiting(){return waiting;}
	//is there a problem?
	inline bool hasError(){return bad;}
	//gives an error message if problem, else an empty string
	inline QString errorString(){return error;}
	void request(QString url);

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