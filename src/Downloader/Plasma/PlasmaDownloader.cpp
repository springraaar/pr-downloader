#include "soap/soapContentServiceSoap12Proxy.h"
#include "soap/ContentServiceSoap.nsmap"
#include "PlasmaDownloader.h"
#include "../../FileSystem.h"
#include "../../Util.h"



CPlasmaDownloader::CPlasmaDownloader(){
	this->torrentPath=fileSystem->getSpringDir()+PATH_DELIMITER +  "torrent" + PATH_DELIMITER;
	fileSystem->createSubdirs(this->torrentPath);
}

std::list<IDownload>* CPlasmaDownloader::search(const std::string& name, IDownload::category category){
	DEBUG_LINE("%s",name.c_str());
	ContentServiceSoap12Proxy service;
	_ns1__DownloadFile file;
	_ns1__DownloadFileResponse result;
	std::string tmpname=name;
	file.internalName=&tmpname;
	std::list<IDownload>* dlres;
	int res;
	res=service.DownloadFile(&file, &result);
	if (res != SOAP_OK){
		printf("Soap error: %d: %s\n",res, service.soap_fault_string());
		return NULL;
	}
	if (!result.DownloadFileResult){
		printf("No file found for criteria %s\n",name.c_str());
		return NULL;
	}

	std::vector<std::string>::iterator it;
	dlres=new std::list<IDownload>();

	IDownload::category cat;
	switch (result.resourceType){
		case ns1__ResourceType__Map:
			cat=IDownload::CAT_MAPS;
			break;
		case ns1__ResourceType__Mod:
			cat=IDownload::CAT_MODS;
			break;
		default:
			DEBUG_LINE("Unknown category in result\n");
			cat=IDownload::CAT_NONE;
			break;
	}
	if (result.links->string.size()==0){
		printf("got no mirror in plasmaresoult\n");
		return false;
	}

	std::string torrent;
	torrent.copy((char*)result.torrent->__ptr,result.torrent->__size);
//	simple .torrent parser to get filename: need to parse for example
// :name27:Tech Annihilation v1.08.sd7:
// -> search :name, search next ":" convert to int, read name

	int pos=torrent.find(":name");
	int end=torrent.find(":",pos);
	int len=atoi(torrent.substr(pos,end).c_str());
	std::string fileName=torrent.substr(end,len);

	DEBUG_LINE("Got filename "%s" from torrent\n",fileName);

	IDownload* dl=NULL;
	for (it=result.links->string.begin();it!=result.links->string.end(); ++it){
		if(dl==NULL)
				dl=new IDownload((*it).c_str(),fileName,cat);
		dl->addMirror((*it).c_str());
	}
	for (it=result.dependencies->string.begin();it!=result.dependencies->string.end(); ++it){
		dl->addDepend((*it).c_str());
	}
	dlres->push_back(*dl);
	return dlres;
}

bool CPlasmaDownloader::download(IDownload& download){
	DEBUG_LINE("%s",download.name.c_str());
	return httpDownload->download(download);
}
