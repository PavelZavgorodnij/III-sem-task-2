#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<pthread.h>
#include<dirent.h>
#include<iterator>
#include<stdlib.h>
#include<stdio.h>
#include<string>
#include<vector>
#include<malloc.h>
#include<fcntl.h>
#include<string.h>
#include<sys/wait.h>
#include<utime.h>
#include<sys/times.h>
#include<set>
#include<iostream>
using namespace std;
//------------------------------------------------------------------------
struct shared{
	set<string>paths_set;
};
struct info{
	int id;
	string dir;
	string dir_to;
	string dit_to_origin;
	shared * infos;
	pthread_mutex_t mutex;
};
//------------------------------------------------------------------------
void *potok(void * arg){
	info * a = (info *)arg;
	DIR *d = opendir(a->dir.c_str());
	if (d == NULL){
		perror(a->dir.c_str());
		return NULL;
	}
	for(dirent *de = readdir(d); de != NULL; de = readdir(d)){
		string fqn = a->dir + "/" + de->d_name;
		string fqn_to = a->dir_to + "/" + de->d_name;
		if(de->d_type != DT_DIR){
			pthread_mutex_lock(&a->mutex);
			int found = a->infos->paths_set.count(fqn);
			if(found == 0){
				a->infos->paths_set.insert(fqn);
				pthread_mutex_unlock(&a->mutex);
				string fqn_to_old = fqn_to + ".old";
				remove(fqn_to_old.c_str());
				rename(fqn_to.c_str(), fqn_to_old.c_str());
				struct stat orgnl_stat;
				stat(fqn.c_str(), &orgnl_stat);
				struct utimbuf orgnl_times;
				orgnl_times.actime = orgnl_stat.st_atime;
				orgnl_times.modtime = orgnl_stat.st_mtime;
				int original = open(fqn.c_str(), O_RDONLY);
				char * buf = (char *) malloc(orgnl_stat.st_size);
				read(original, buf, orgnl_stat.st_size);
				close(original);
				int copy = open(fqn-to.c_str(), O_WRONLY|O_CREAT, orgnl_stat.st_mode);
				write(copy, buf, orgnl_stat.st_size);
				close(copy);
				utime(fqn_to.c_str(), &orgnl_times);
				free(buf);
			} else {
				pthread_mutex_unlock(&a->mutex);
			}
		} else {
			if(strcmp(de->d_name, ".") == 0) continue;
			if(strcmp(de->d_name, "..") == 0) continue;
			if(fqn == a->dir_to_origin) continue;
			struct stat orgnl;
			stat(fqn.c_str(), &orgnl);
			mkdir(fqn_to.c_str(), orgnl.st_mode);
			string lol = a->dir;
			string lol_to = a->dir_to;
			a->dir = fqn;
			a->dir_to = fqn_to;
			int * lll = (int *) potok(a);
			a->dir = lol;
			a->dir_to = lol_to;
		}
	}
	closedir(d);
	return NULL;
}
void *real_potok(void * arg){
	info * a = (info *)arg;
	int * lll = (int *) potok(a);
	return NULL;
}
//------------------------------------------------------------------------
int main(int argc, char **argv){
	int how_much = (argc > 1 ? atoi(argv[1]) : 4);
	string folder = (argc > 2 ? argv[2] : ".");
	string folder_to = (argc > 3 ? argv[3] : "./test");
	struct stat fldr;
	stat(folder.c_str(), &fldr);
	mkdir(folder_to.c_str(), fldr.st_mode);
	char fpath[100000];
	getcwd(fpath, 100000);
	if(folder[0] == '/'){}
	else {
		if(folder[0] == '.' && folder.size() < 2){
			folder = fpath;
		} else if(folder[0] == '.' && folder[1] == '/'){
			folder.erase(folder.begin());
			folder = fpath + folder;
		} else if(folder[0] == '.' && folder[1] == '.' && folder.size() < 3) {
			string ppp = fpath;
			for(int j = ppp.size() - 1; ppp[j] != '/'; j--)
				ppp.erase(ppp.begin() + j);
			ppp.erase(ppp.begin() + ppp.size() - 1);
			folder = ppp;
		} else if(folder[0] == '.' && folder[1] == '.' && folder[2] == '/') {
			string ppp = fpath;
			for(int j = ppp.size() - 1; ppp[j] != '/'; j--)
				ppp.erase(ppp.begin() + j);
			folder.erase(folder.begin(), folder.begin() + 3);
			folder = ppp + folder;
		} else {
			folder = fpath + ("/" + folder);
		}
	}
	if(folder_to[0] == '/'){}
	else {
		if(folder_to[0] == '.' && folder_to.size() < 2){
			folder_to = fpath;
		} else if(folder_to[0] == '.' && folder_to[1] == '/'){
			folder_to.erase(folder_to.begin());
			folder_to = fpath + folder_to;
		} else if(folder_to[0] == '.' && folder_to[1] == '.' && folder_to.size() < 3) {
			string ppp = fpath;
			for(int j = ppp.size() - 1; ppp[j] != '/'; j--)
				ppp.erase(ppp.begin() + j);
			ppp.erase(ppp.begin() + ppp.size() - 1);
			folder_to = ppp;
		} else if(folder_to[0] == '.' && folder_to[1] == '.' && folder_to[2] == '/') {
			string ppp = fpath;
			for(int j = ppp.size() - 1; ppp[j] != '/'; j--)
				ppp.erase(ppp.begin() + j);
			folder_to.erase(folder_to.begin(), folder_to.begin() + 3);
			folder_to = ppp + folder_to;
		} else {
			folder_to = fpath + ("/" + folder_to);
		}
	}
	shared info1;
	vector<info>infos;
	infos.resize(how_much);
	pthread_mutex_t mutex1;
	pthread_mutex_init(&mutex1, NULL);
	vector<pthread_t>threads;
	threads.resize(how_much);
	for(int i = 0; i < how_much; i++){
		infos[i].id = i;
		infos[i].dir = folder;
		infos[i].dir_to = folder_to;
		infos[i].dir_to_origin = folder_to;
		infos[i].infos = &info1;
		infos[i].mutex = mutex1;
		pthread_create(&threads[i], NULL, real_potok, &infos[i]);
	}
	for(int i = 0; i < how_much; i++){
		pthread_join(threads[i], NULL);
	}
}



























