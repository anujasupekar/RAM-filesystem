#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

struct filesStruct{
	int isAFile;
	char *name;
	char *content;
	mode_t mode;
	struct filesStruct *subDirectory;
	struct filesStruct *next;
};

struct filesStruct *readNode(const char *path);
int storeFilesystem(struct filesStruct *parent,struct filesStruct *directoryList, int level, int fd,int offset);
void freeMemoryOf(struct filesStruct *directoryList);


struct filesStruct *ramfiles = NULL;
char *mountPoint;
void createAFile(const char *path, mode_t mode);
signed int totalMemory;
int saveFile = -1;
int writeMemory = 0;


int returnCount(const char *path)
{
	char *tempPath = strndup(path,strlen(path));
	int i, dirCount = 0;

	for(i=0; i<strlen(tempPath); i++)
	{
		if(tempPath[i] == '/'){
			dirCount++;
		}
	}
	return dirCount;
}

void createAFile(const char *path, mode_t mode)
{
	struct filesStruct *parentDirectory=NULL;
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	char *tempPath = strndup(path,strlen(path));
	int count=0;
	int foundDirectory=-1;

	if(mode == -1)
	{
		mode = 0666 | S_IFREG;
	}
	struct filesStruct *addToFile = (struct filesStruct*)malloc(sizeof(struct filesStruct));
	addToFile->isAFile= 1;
	addToFile->next=NULL;
	addToFile->mode=mode;
	addToFile->content=NULL;
	addToFile->subDirectory=NULL;

	char *pathPtr, *directoryName;
	pathPtr = strtok(tempPath, "/");

	count=returnCount(path);

	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory != 1)
			{
				directoryList=parentDirectory->subDirectory;
				addToFile->name = strndup(directoryName, strlen(directoryName));
				if(directoryList != NULL)
				{
					while(directoryList->next != NULL)
						directoryList=directoryList->next;
					directoryList->next = addToFile;
				}
				else
				{
					parentDirectory->subDirectory=addToFile;
				}
			}
			else
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory= -1;
					directoryName=pathPtr;
					parentDirectory=directoryList;
					directoryList=directoryList->subDirectory;
				}
			}
		}
	}
	else
	{
		addToFile->name = strndup(pathPtr, strlen(pathPtr));
		if(directoryList != NULL)
		{
			while(directoryList->next != NULL)
			{
				directoryList= directoryList->next;
			}
			directoryList->next=addToFile;
		}
		else
		{
			ramfiles=addToFile;
		}
	}
	free(tempPath);
}

void createADir(const char *path, mode_t mode)
{
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	struct filesStruct *parentDirectory; 
	int foundDirectory = -1;
	char *pathPtr, *directoryName;
	char *tempPath = strndup(path,strlen(path));

	if(mode == -1)
	{
		mode = 0755 | S_IFDIR;
	}

	struct filesStruct *addToDirectory = (struct filesStruct*)malloc(sizeof(struct filesStruct));
	addToDirectory->isAFile= 0;
	addToDirectory->next=NULL;
	addToDirectory->subDirectory=NULL;
	addToDirectory->mode=mode;

	int count = returnCount(path);

	pathPtr = strtok(tempPath, "/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory != 1)
			{
				directoryList=parentDirectory->subDirectory;
				addToDirectory->name = strndup(directoryName, strlen(directoryName));
				if(directoryList != NULL)
				{
					while(directoryList->next != NULL)
						directoryList=directoryList->next;
					directoryList->next = addToDirectory;
				}
				else
				{
					parentDirectory->subDirectory=addToDirectory;
				}
			}
			else
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory= -1;
					directoryName=pathPtr;
					parentDirectory=directoryList;
					directoryList=directoryList->subDirectory;
				}
			}
		}

	}
	else
	{
		addToDirectory->name = strndup(pathPtr, strlen(pathPtr));
		if(directoryList != NULL)
		{
			while(directoryList->next != NULL)
			{
				directoryList= directoryList->next;
			}
			directoryList->next=addToDirectory;
		}
		else
		{
			ramfiles=addToDirectory;
		}
	}
	free(tempPath);
}


static int ramGetAttr(const char *path, struct stat *stbuf)
{	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	int foundDirectory = -1;
	char *pathPtr;
	int response = -ENOENT;
	char *tempPath = strndup(path,strlen(path));
	if(strcmp(path, "/") == 0)
	{
		stbuf->st_nlink = 2;
		stbuf->st_mode = S_IFDIR | 0755;
		response=0;
	}
	else
	{
		pathPtr = strtok(tempPath, "/");
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory = 1;
					break;
				}
				directoryList=directoryList->next;
			}
				pathPtr = strtok(NULL,"/");

				if(foundDirectory != 1)
				{
					response = -ENOENT;
				}
				else 
				{
					if(directoryList->isAFile)
					{
						stbuf->st_nlink = 1;
						stbuf->st_mode= S_IFREG | 0444;
						if(directoryList->content != NULL)
								stbuf->st_size = strlen(directoryList->content);
					}
					else{
						stbuf->st_nlink = 2;
						stbuf->st_mode = S_IFDIR | 0755;
					}
					response=0;
					if(pathPtr == NULL)
					{
						break;
					}
					else
					{
						foundDirectory= -1;
						directoryList=directoryList->subDirectory;
						response= -ENOENT;
					}
				}

		}
	}
	free(tempPath);
	return response;
}

static int ramOpenDirectory(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int ramReadDirectory(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	int foundDirectory = 0;
	char *readFileName, *pathPtr;
	char *tempPath = strndup(path,strlen(path));

	if(strcmp(path, "/")== 0)
	{
		if(directoryList != NULL)
		{
			while(directoryList != NULL)
			{
				readFileName = strndup(directoryList->name, strlen(directoryList->name));
				filler(buf, readFileName, NULL, 0);
				free(readFileName);
				directoryList=directoryList->next;
			}
		}
	}
	else
	{
		pathPtr = strtok(tempPath, "/");
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory = 1;
					break;
				}
				directoryList= directoryList->next;
			}
			pathPtr= strtok(NULL, "/");
			if(foundDirectory == 1)
			{
				directoryList=directoryList->subDirectory;
				if(pathPtr == NULL){
					break;
				}
				else{
					foundDirectory = -1;
				}
			}
		}
		while(directoryList != NULL)
		{
			readFileName = strndup(directoryList->name, strlen(directoryList->name));
			filler(buf, readFileName, NULL, 0);
			free(readFileName);
			directoryList=directoryList->next;
		}
	}
	free(tempPath);
	return 0;
}

static int ramMakeDirectory(const char *path, mode_t mode)
{
	int sizeOfDirectory = sizeof(struct filesStruct);
	char *tempPath, *pathPtr, *directoryName;
	struct filesStruct *parentDirectory;
	int count = 0; 
	int foundDirectory=-1;
	tempPath = strndup(path,strlen(path));
	struct filesStruct *directoryList=ramfiles;
	if((totalMemory- sizeOfDirectory) < 0)
	{
		fprintf(stderr, "%s\n","No enough space");
		return -ENOSPC;
	}

	if(mode == -1)
	{
		mode = 0755 | S_IFDIR;
	}

	struct filesStruct *addToDirectory = (struct filesStruct*)malloc(sizeof(struct filesStruct));
	addToDirectory->isAFile= 0;
	addToDirectory->next=NULL;
	addToDirectory->subDirectory=NULL;
	addToDirectory->mode=mode;

	count = returnCount(path);

	pathPtr = strtok(tempPath, "/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory != 1)
			{
				directoryList=parentDirectory->subDirectory;
				addToDirectory->name = strndup(directoryName, strlen(directoryName));
				if(directoryList != NULL)
				{
					while(directoryList->next != NULL)
						directoryList=directoryList->next;
					directoryList->next = addToDirectory;
				}
				else
				{
					parentDirectory->subDirectory=addToDirectory;
				}
			}
			else
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory= -1;
					directoryName=pathPtr;
					parentDirectory=directoryList;
					directoryList=directoryList->subDirectory;
				}
			}
		}

	}
	else
	{
		addToDirectory->name = strndup(pathPtr, strlen(pathPtr));
		if(directoryList != NULL)
		{
			while(directoryList->next != NULL)
			{
				directoryList= directoryList->next;
			}
			directoryList->next=addToDirectory;
		}
		else
		{
			ramfiles=addToDirectory;
		}
	}
	sizeOfDirectory=sizeOfDirectory+strlen(addToDirectory->name);
	totalMemory=totalMemory- sizeOfDirectory;
	free(tempPath);
	//createADir(path, mode);
	return 0;
}

static int ramRemoveDir(const char *path)
{
	struct filesStruct *directoryList= NULL;
	struct filesStruct *prevDirectory= NULL;
	struct filesStruct *parentDirectory = NULL; 
	directoryList = prevDirectory= ramfiles;
	
	int foundDirectory = -1;
	int count = 0;
	char *pathPtr;
	char *tempPath = strndup(path,strlen(path));

	count=returnCount(path);
	pathPtr = strtok(tempPath,"/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name) == 0)
				{
					foundDirectory=1;
					break;
				}
				prevDirectory=directoryList;
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory == 1)
			{
				if(pathPtr == NULL)
				{
					break;
				}
				else
				{
					foundDirectory=-1;
					parentDirectory=directoryList;
					directoryList=directoryList->subDirectory;
					prevDirectory=directoryList;
				}
			}
		}
		if(directoryList->subDirectory != NULL)
			return -ENOTEMPTY;
		if(strcmp(prevDirectory->name, directoryList->name) != 0){
			prevDirectory->next=directoryList->next;
		}
		else{
			parentDirectory->subDirectory=directoryList->next;
		}
	}
	else
	{
		while(directoryList->next != NULL)
		{
			if(strcmp(pathPtr, directoryList->name)==0)
			{
				break;
			}
			prevDirectory=directoryList;
			directoryList=directoryList->next;
		}
		if(directoryList->subDirectory != NULL){
			return -EPERM;
		}
		if(strcmp(prevDirectory->name, directoryList->name) != 0){
			prevDirectory->next=directoryList->next;
		}
		else{
			ramfiles=directoryList->next;
		}

	}
	int sizeOfDirectory = sizeof(struct filesStruct) + strlen(directoryList->name);
	totalMemory = totalMemory + sizeOfDirectory;
	free(directoryList->name);
	free(directoryList);
	free(tempPath);
	return 0;
}

static int ramUnlink(const char *path)
{
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	struct filesStruct *parentDirectory= NULL;
	struct filesStruct *prevDirectory= NULL;
	prevDirectory = ramfiles;
	int foundDirectory = -1;
	int count = 0;
	char *pathPtr;
	char *tempPath = strndup(path,strlen(path));

	count=returnCount(path);

	pathPtr=strtok(tempPath, "/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				prevDirectory=directoryList;
				directoryList=directoryList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(foundDirectory == 1)
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory = -1;
					parentDirectory = directoryList;
					directoryList=directoryList->subDirectory;
					prevDirectory=directoryList;
				}
			}
			
		}
		if(strcmp(prevDirectory->name, directoryList->name) == 0)
		{
			parentDirectory->subDirectory = directoryList->next;
		}
		else
		{
			prevDirectory->next = directoryList->next;
		}
	}
	else
	{
		while(directoryList->next != NULL)
		{
			if(strcmp(directoryList->name, pathPtr)==0)
				break;
			prevDirectory=directoryList;
			directoryList=directoryList->next;
		}
		if(strcmp(directoryList->name, prevDirectory->name) != 0)
		{
			prevDirectory->next=directoryList->next;
		}
		else{
			ramfiles=directoryList->next;
		}
	}
	int sizeOfDirectory = 0;
	if(directoryList->content != NULL){
		sizeOfDirectory = strlen(directoryList->content);
	}
	sizeOfDirectory=sizeof(struct filesStruct) + strlen(directoryList->name) + sizeOfDirectory;
	totalMemory=totalMemory+sizeOfDirectory;
	free(directoryList->name);
	if(directoryList->content != NULL)
		free(directoryList->content);
	free(directoryList);
	return 0;
}

static int ramOpenFile(const char *path, struct fuse_file_info *fi)
{
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	int foundDirectory = -1;
	int count = 0;
	char *pathPtr;
	char *tempPath = strndup(path,strlen(path));

	count=returnCount(path);

	pathPtr=strtok(tempPath, "/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(foundDirectory == 1)
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory=-1;
					directoryList=directoryList->subDirectory;
				}
			}
		}
	}
	else
	{
		while(directoryList->next != NULL)
		{
			if(strcmp(pathPtr, directoryList->name) == 0)
			{
				foundDirectory=1;
				break;
			}
			directoryList=directoryList->next;
		}
	}
	free(tempPath);
	if(foundDirectory)
		return 0;
	else
		return -ENOENT;
}

static int ramReadFromFile(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct filesStruct *directoryList = NULL;
	int i=0;
	while(directoryList == NULL)
	{
		i++;
		directoryList=readNode(path);
		if(i>20){
			return 0;
		}
	}
	if(directoryList->content == NULL)
	{
		size = 0;
	}
	else
	{
		size_t len;
		len = strlen(directoryList->content);
		if(offset<len)
		{
			if(offset +size >len)
				size=len- offset;
			memcpy(buf, directoryList->content+offset, size);
			buf[size] = '\0';
		}
		else
			size=0;
	}
	return size;
}


static int ramWriteToFile(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	int foundDirectory = -1;
	int count = 0;
	signed int sizeOfRAM =0;
	char *pathPtr;
	char *tempPath = strndup(path,strlen(path));

	count=returnCount(path);

	pathPtr = strtok(tempPath,"/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr = strtok(NULL,"/");
			if(foundDirectory == 1)
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory=-1;
					directoryList=directoryList->subDirectory;
				}
			}
		}
	}
	else
	{
		while(directoryList->next != NULL)
		{
			if(strcmp(pathPtr, directoryList->name) == 0)
			{
				break;
			}
			directoryList=directoryList->next;
		}
	}
	if(directoryList->content != NULL)
	{
		int newsize, len;
		newsize=offset+size;
		len=strlen(directoryList->content);

		if(newsize >len){
			sizeOfRAM = totalMemory - (newsize - len);
			if(sizeOfRAM <0){
				fprintf(stderr, "%s\n","No enough space");
				return -ENOSPC;
			}
			char *temp = malloc(newsize+1);
			memset(temp,0,newsize+1);
			strncpy(temp,directoryList->content,len);
			temp[len] = '\0';
			free(directoryList->content);
			directoryList->content = temp;
			totalMemory = totalMemory + len - (offset+size);
			writeMemory = writeMemory + offset + size - len;
		}
		int i, j=0;
		for(i=offset; i<offset+size; i++){
			directoryList->content[i] = buf[j];
			j++;
		}
		directoryList->content[i] = '\0';
	}
	else
	{
		signed int len= strlen(buf);
		sizeOfRAM = totalMemory - len;
		if(sizeOfRAM < 0){
			fprintf(stderr,"%s","No enough space");
			return -ENOSPC;
		}
		char *temp = malloc(size+1);
		memset(temp,0,size+1);
		strncpy(temp,buf,size);
		temp[size] = '\0';
		directoryList->content = temp;
		totalMemory = totalMemory - size;
		writeMemory = writeMemory + size;
	}
	free(tempPath);
	return size;
}

static int ramTruncate(const char *path, off_t offset)
{
	return 0;
}


struct filesStruct *readNode(const char *path)
{
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	int foundDirectory = -1;
	int count = 0;
	char *pathPtr;
	char *tempPath = strndup(path,strlen(path));

	count=returnCount(path);

	pathPtr=strtok(tempPath,"/");

	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory==1)
			{
				if(pathPtr == NULL){
					break;
				}
				else{
					directoryList=directoryList->subDirectory;
					foundDirectory=-1;
				}
			}
		}
	}
	else
	{
		while(directoryList != NULL)
		{
			if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
		}
	}
	free(tempPath);
	return directoryList;
}


static int ramCreateFile(const char *path, mode_t mode, struct fuse_file_info *fi)
{	
	struct filesStruct *parentDirectory=NULL;
	struct filesStruct *directoryList= NULL;
	directoryList = ramfiles;
	char *tempPath = strndup(path,strlen(path));
	int count=0;
	int foundDirectory=-1;

	if(mode == -1)
	{
		mode = 0666 | S_IFREG;
	}
	struct filesStruct *addToFile = (struct filesStruct*)malloc(sizeof(struct filesStruct));
	addToFile->isAFile= 1;
	addToFile->next=NULL;
	addToFile->mode=mode;
	addToFile->content=NULL;
	addToFile->subDirectory=NULL;

	char *pathPtr, *directoryName;
	pathPtr = strtok(tempPath, "/");

	count=returnCount(path);

	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory != 1)
			{
				directoryList=parentDirectory->subDirectory;
				addToFile->name = strndup(directoryName, strlen(directoryName));
				if(directoryList != NULL)
				{
					while(directoryList->next != NULL)
						directoryList=directoryList->next;
					directoryList->next = addToFile;
				}
				else
				{
					parentDirectory->subDirectory=addToFile;
				}
			}
			else
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory= -1;
					directoryName=pathPtr;
					parentDirectory=directoryList;
					directoryList=directoryList->subDirectory;
				}
			}
		}
	}
	else
	{
		addToFile->name = strndup(pathPtr, strlen(pathPtr));
		if(directoryList != NULL)
		{
			while(directoryList->next != NULL)
			{
				directoryList= directoryList->next;
			}
			directoryList->next=addToFile;
		}
		else
		{
			ramfiles=addToFile;
		}
	}
	free(tempPath);
	//createAFile(path, mode);
	return 0;
}

struct filesStruct *findPathOF(const char *source, int previous)
{
	struct filesStruct *parentDirectory=NULL;
	struct filesStruct *prevDirectory=NULL;
	struct filesStruct *directoryList= NULL;
	directoryList = prevDirectory= ramfiles;
	char *pathPtr;
	int count=0;
	int foundDirectory=-1;
	char *tempPath = strdup(source);

	count=returnCount(source);

	pathPtr = strtok(tempPath,"/");
	if(count != 1)
	{
		while(pathPtr != NULL)
		{
			while(directoryList != NULL)
			{
				if(strcmp(pathPtr, directoryList->name)==0)
				{
					foundDirectory=1;
					break;
				}
				prevDirectory=directoryList;
				directoryList=directoryList->next;
			}
			pathPtr=strtok(NULL,"/");
			if(foundDirectory == 1)
			{
				if(pathPtr == NULL){
					break;
				}
				else
				{
					foundDirectory= -1;
					parentDirectory=directoryList;
					directoryList=directoryList->subDirectory;
					prevDirectory=directoryList;
				}
			}
		}
	}
	else
	{
		while(directoryList->next != NULL)
		{
			if(strcmp(pathPtr, directoryList->name)==0)
			{
				break;
			}
			prevDirectory=directoryList;
			directoryList=directoryList->next;
		}
	}
	if (previous ==0)
		return directoryList;
	else if(previous == 1)
		return prevDirectory;
	else if(previous ==2)
		return parentDirectory;
	return NULL;
}

static int ramRename(const char *source, const char *destination)
{
	struct filesStruct *directoryList=findPathOF(source,0);
	struct filesStruct *prevDirectory = findPathOF(source,1);
	int count=0;

	count=returnCount(source);

	if(count != 1)
	{
		if(strcmp(prevDirectory->name, directoryList->name)==0)
		{
			struct filesStruct *parentDirectory=findPathOF(source,2);
			parentDirectory->subDirectory=directoryList->next;
		}
		else{
			prevDirectory->next = directoryList->next;
		}
	}
	else
	{
		if(strcmp(directoryList->name, prevDirectory->name)!=0){
			prevDirectory->next = directoryList->next;
		}
		else
			ramfiles = directoryList->next;
	}
	directoryList->next=NULL;
	if(directoryList->isAFile !=0)
	{
		createAFile(destination, -1);
	}
	else{
		createADir(destination, -1);
	}

	struct filesStruct *destDirectoryList=findPathOF(destination,0);
	struct filesStruct *destPrevDirectory = findPathOF(destination,1);

	if(directoryList->isAFile !=0)
	{
		if(directoryList->content == NULL)
		{
			destDirectoryList->content=NULL;
		}
		else
		{
			destDirectoryList->content=strdup(directoryList->content);
		}
	}
	else
	{
		if(strcmp(destDirectoryList->name, destPrevDirectory->name) != 0)
		{
			destPrevDirectory->next=directoryList;
			directoryList->next = destDirectoryList->next;
			directoryList->name=strdup(destDirectoryList->name);
		}
		else
		{
			struct filesStruct *parentOfDir=findPathOF(destination,2);
			if(parentOfDir != NULL)
			{
				printf("name: %s\n",parentOfDir->name);
				parentOfDir->subDirectory=directoryList;
				directoryList->name=strdup(destDirectoryList->name);
				directoryList->next=destDirectoryList->next;
			}
			else
			{
				printf("Is Null \n");
				ramfiles=directoryList;
				directoryList->name=strdup(destDirectoryList->name);
				directoryList->next = destDirectoryList->next;
			}
		}
	}
	return 0;
}

void ramDestroyPath (void *destroy)
{
	if(saveFile == 1){
		struct filesStruct *temp = ramfiles;
		int fd = open(mountPoint,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
		storeFilesystem(temp, temp,1,fd,0);
		close(fd);
	}
	struct filesStruct *directoryList = ramfiles;
	freeMemoryOf(directoryList);
}

static int ramutimens(const char* path, const struct timespec tv[2]){
	return 0;
}

int storeFilesystem(struct filesStruct *parent,struct filesStruct *directoryList, int level, int fd,int offset)
{
	char dir[10],dirStruct[10000];
	int written = 0;
	if(level != 1)
		sprintf(dir,"%s%s%s","#dir",parent->name,"#");
	else
		sprintf(dir,"%s","#ROOT#");
		
	written = pwrite(fd,dir,strlen(dir),offset);
	offset = written + offset;
	
	while(directoryList != NULL){
		if(directoryList->isAFile == 0){
			sprintf(dirStruct,"%s%s%s%d%s%d%s%s%s","#node#",directoryList->name,"|",directoryList->isAFile,"|",(directoryList->subDirectory != NULL)?1:0,"|","NONE","#endnode#");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
		}

		if(directoryList->subDirectory != NULL){
			offset = storeFilesystem(directoryList,directoryList->subDirectory,level +1,fd,offset);
		}else if (directoryList->isAFile){
						sprintf(dirStruct,"%s%s%s%d%s%d%s","#node#",directoryList->name,"|",directoryList->isAFile,"|",(directoryList->subDirectory != NULL)?1:0,"|");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
			written = pwrite(fd,directoryList->content != NULL ?directoryList->content:"NONE",directoryList->content != NULL ?strlen(directoryList->content):strlen("NONE"),offset);
			offset = written + offset;
			sprintf(dirStruct,"%s","#endnode#");
			written = pwrite(fd,dirStruct,strlen(dirStruct),offset);
			offset = written + offset;
			}
		directoryList = directoryList->next;
	}
	if(level != 1)
		sprintf(dir,"%s%s%s","#enddir",parent->name,"#");
	else
		sprintf(dir,"%s","#ENDROOT#");
	
	written = pwrite(fd, dir, strlen(dir), offset);
	offset=written+offset;
	return offset; 
}

void freeMemoryOf(struct filesStruct *directoryList)
{
	int sizeOfDirectory=0;
	while(directoryList!= NULL)
	{
		if(directoryList->subDirectory != NULL)
		{
			freeMemoryOf(directoryList->subDirectory);
		}
		if(directoryList->isAFile != 1)
			sizeOfDirectory = strlen(directoryList->name) + sizeof(struct filesStruct);
		else
		{
			int length=0;
			if(directoryList->content == NULL)
				length = 0;
			else
				length = strlen(directoryList->content);
						
			sizeOfDirectory = strlen(directoryList->name) + length +sizeof(struct filesStruct);
		}
		
		totalMemory = totalMemory +sizeOfDirectory;
		free(directoryList->name);
		if(directoryList->isAFile==1)
		{ if(directoryList->content != NULL)
			free(directoryList->content);	
		
		}
		free(directoryList);
		directoryList=directoryList->next;
	}
}


int match(char *a, char *b)
{
	int position =0;
	char *x, *y;
	x=a; y=b;

	while(*a)
    {
      	while(*x==*y)
	    {
	    	y++;
	    	x++;
	    	if(*x=='\0'||*y=='\0')
	        	break;
	    }
      	if(*y=='\0')
         	break;
        position++;
      	a++;
      	
      	x = a;
      	y = b;
   }
   if(*a)
      return position;
   else
      return -1;
}

char *substring(char *string, int position, int length)
{
	int i=0;
	char *pointer;
	pointer = (char*)malloc(length+1);
	if(pointer==NULL)
	{
		perror("failed to malloc");
		exit(EXIT_FAILURE);
	}
	while(i<position-1)
	{
		string++;
		i++;
	}
	strncpy(pointer,string,length);
	return pointer;
}

int loadFileSystem(struct filesStruct *parentDirectory,struct filesStruct *child,char *loaddir,int level)
{
	int start = 0,end = 0;
	int hasChild = -1;
	char *nodeName,*localdir,*contentVal;
	localdir = strdup(loaddir);
	char dirstart[100] = "\0";
	char dirend[100] = "\0";
	char *filePtr,*nodeptr;

	start = match(localdir,"#node#");
	end = match(localdir,"#endnode#");

	while(start != -1 || end != -1)
	{
		int remLength;
		int subsize = end - (start + strlen("#node#"));
		filePtr = (substring(localdir,start + strlen("#node#")+1,subsize));
		nodeptr = strdup(filePtr);
		struct filesStruct *addNode = (struct filesStruct *)malloc(sizeof(struct filesStruct));
		nodeName = strtok(nodeptr,"|");
		addNode->name = strdup(nodeName);
		addNode->isAFile = atoi(strtok(NULL,"|"));
		hasChild = atoi(strtok(NULL,"|"));
		contentVal = strtok(NULL,"|");
		addNode->content = strdup(contentVal);
		addNode->subDirectory = NULL;
		addNode->next = NULL;

		if((totalMemory - strlen(nodeName) - sizeof(struct filesStruct *)) < 0)
		{
			fprintf(stderr,"%s","NO ENOUGH SPACE");
			return -ENOSPC;
		}
		else
			totalMemory = totalMemory - strlen(nodeName) - sizeof(struct filesStruct *);

		struct filesStruct *directoryList;
		if(level != 1)
		{
			directoryList = parentDirectory->subDirectory;
			if(directoryList == NULL)
			{
				parentDirectory->subDirectory = addNode;
				directoryList = parentDirectory->subDirectory;
			}
			else
			{
				while(directoryList->next != NULL)
					directoryList = directoryList->next;
				directoryList->next = addNode;
				directoryList = addNode;
			}
		}
		else
		{
			directoryList = ramfiles;
			if(directoryList != NULL)
			{
				while(directoryList->next != NULL)
					directoryList = directoryList->next;
				directoryList->next = addNode;
				directoryList = addNode;
			}
			else
			{
				ramfiles = addNode;
				directoryList = ramfiles;
			}
		}
		remLength = strlen(localdir)-(strlen("#endnode#") + subsize);
		localdir = (substring(localdir,end + strlen("#endnode#")+1,remLength));
		if(hasChild == 1){
			sprintf(dirstart,"%s%s%s","#dir",nodeName,"#");
			sprintf(dirend,"%s%s%s","#enddir",nodeName,"#");
			start = match(localdir,dirstart);
			end = match(localdir,dirend);
			int subsize = end - (start + strlen(dirstart));
			filePtr = substring(localdir,start + strlen(dirstart)+1,subsize);
			loadFileSystem(directoryList,directoryList->subDirectory,filePtr,level+1);
			remLength = strlen(localdir)-(strlen(dirend) + subsize);
			localdir = substring(localdir,end + strlen(dirend)+1,remLength);
		}
		start = match(localdir,"#node#");
		end = match(localdir,"#endnode#");
		free(nodeptr);
	}
	free(localdir);
	return 0;
}

static struct fuse_operations xmp_oper = {
	.getattr	= ramGetAttr,
	.readdir	= ramReadDirectory,
	.mkdir		= ramMakeDirectory,
	.unlink		= ramUnlink,
	.rmdir		= ramRemoveDir,
	.open		= ramOpenFile,
	.read		= ramReadFromFile,
	.write		= ramWriteToFile,
	.create		= ramCreateFile,
	.opendir 	= ramOpenDirectory,
	.destroy	= ramDestroyPath,
	.rename		= ramRename,
	.truncate	= ramTruncate,
	.utimens	= ramutimens,
};

int main(int argc, char *argv[])
{
	char *filebuf ;
	ramfiles = NULL;
	int argCount = 0;

	if(argc == 4 ){
		saveFile = 1;
		mountPoint = argv[3];
	}else if (argc == 3){
		saveFile = 0;
	}else {
		perror("Too many arguments. Proper usage: ramdisk <mount_point> <size> [<filename>]");
		exit(-1);
	}
	totalMemory = atoi(argv[2]);
	argCount = 2;
	
	totalMemory = totalMemory * 1000 * 1000;
	if(saveFile == 1){
		FILE *fp;
		fp=fopen(mountPoint, "r");

		if (fp != NULL) {
			if (fseek(fp, 0L, SEEK_END) == 0) {
				long bufsize = ftell(fp);
				if (bufsize == -1) { /*printf("Error on buffsize\n");*/}
				filebuf = malloc(sizeof(char) * (bufsize + 1));
				if (fseek(fp, 0L, SEEK_SET) != 0) { /*printf("Error on seek\n"); */}
				size_t newLen = fread(filebuf, sizeof(char), bufsize, fp);
				if (newLen == 0) {
					fputs("Error reading file", stderr);
				} else {
					filebuf[++newLen] = '\0';
				}
			}
			fclose(fp);

			int start,end;
			char *filePtr;
			start = match(filebuf,"#ROOT#");
			end = match(filebuf,"#ENDROOT#");

			int subsize = end - (start + strlen("#ROOT#"));
			filePtr = strdup(substring(filebuf,start + strlen("#ROOT#")+1,subsize));
			struct filesStruct *temp = ramfiles;
			loadFileSystem(temp,temp,filePtr,1);

			free(filePtr);
			
		}
	}

	return fuse_main(argCount, argv, &xmp_oper, NULL);
}