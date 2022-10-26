#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include<stdbool.h>  
#include <math.h>

#include "myStructures.h"

struct SuperBlock sp;
int findSlash(char *path){
	int slashCounter=0;
	int i=0;
	
	while(path[i]!='\0'){

		if (path[i]=='\\')
		{
			slashCounter++;
		}
		i++;
	}
	return slashCounter;
}
bool findEntryInCurrentFile(struct DirectoryEntry de,char* name){

	
	if (de.inner>0)
	{
		for (int i = 0; i < de.inner; i++)
		{			
			if(strcmp(de.directoryNames[i],name)==0){
				return true;
			}
		}
	}
	return false;
}

void myMkdir(FILE * fp, char * filesystem, char * op, char * path){
	char directoryPath[1024];
	strcpy(directoryPath,path);
	int slashNum=findSlash(directoryPath);  //Slash sayısı dosya sayısını verir.
	struct INode mynode;
	int i=0;
	while(path[i]!='\0'){
		path[i]=path[i+1];
		i++;
	}
	slashNum--;
	struct DirectoryEntry temp;

	fseek(fp,sp.blockStartPos*sp.blockSize,SEEK_SET);
	fread(&temp, sizeof(struct DirectoryEntry), 1, fp);
	while(slashNum>=0){

		if (slashNum==0)//Roota ekleme yapmalıyız.
		{		
			
			bool isOccur=findEntryInCurrentFile(temp,path);


			if (isOccur==true)
			{
				printf("ERROR. This directory/file already exist.\n");
			}
			else{
				
				int usedInodeCounter=0;
				fseek(fp,sp.inodeFreeListStartPos*sp.blockSize,SEEK_SET);
				char inode;
				while(1){
					fread(&inode, sizeof(char), 1, fp);
					if (inode=='0')
					{	
						
						break;
					}
					else{
						usedInodeCounter++;
						
					}

				}
				fseek(fp,sp.inodeStartPos*sp.blockSize,SEEK_SET);

				
				mynode.inodeNum=usedInodeCounter;
				mynode.fileSize=ceil((float)sizeof(temp)/sp.blockSize)*sp.blockSize;
				
				time_t creationTime;
				creationTime = time(0);
				mynode.creationTime=creationTime;
				mynode.lastModificationTime = creationTime;
				strcpy(mynode.fileName,path);
				mynode.fileType=1; //directory
				int usedBlock=0;

				fseek(fp,sp.blockFreeListStartPos,SEEK_SET);

				char blockState;
				while(true){
					fread(&blockState,sizeof(char),1,fp);
					if (blockState=='1')
					{	
						
						usedBlock++;
					}
					else{
						break;
					}

				}

				mynode.blockNumbers[0]=usedBlock;
				for (int i = 1; i < 13; ++i)
				{
					mynode.blockNumbers[i]=-1;
				}



				int ctr=0;
				
				if (temp.inner==10)
				{
					printf("ERROR. The directory couldn't added\n");
					return ;
				}
				fseek(fp,sp.inodeStartPos*sp.blockSize+sizeof(struct INode)*usedInodeCounter,SEEK_SET);			
				fwrite(&mynode, sizeof(struct INode), 1, fp);

				strcpy(temp.directoryNames[temp.inner],path);
				temp.inodeNumbers[temp.inner]=usedInodeCounter;
				temp.inner=temp.inner+1;

				struct INode indd;
				fseek(fp,sp.inodeStartPos*sp.blockSize,SEEK_SET);
				int p=0;
				while (p < usedInodeCounter)
				{
					if (p==temp.inodeNum)
					{
						break;
					}
					p++;
				}
				fseek(fp,sp.inodeStartPos*sp.blockSize+sizeof(struct INode)*p,SEEK_SET);
				fread(&indd,sizeof(struct INode),1,fp);
				int qwe=indd.blockNumbers[0];

				fseek(fp,qwe*sp.blockSize,SEEK_SET);//------------------
				fwrite(&temp,sizeof(struct DirectoryEntry),1,fp);


				fseek(fp,sp.blockStartPos*sp.blockSize,SEEK_SET);

				struct DirectoryEntry abc;
				fread(&abc,sizeof(struct DirectoryEntry),1,fp);


				
				printf("Dİrectory Added.");
				



				struct DirectoryEntry temp2;
				temp2.inner=0;
				strcpy(temp2.directoryName,path);
				fseek(fp,(usedBlock+1)*sp.blockSize,SEEK_SET);
				fwrite(&temp2,sizeof(struct DirectoryEntry),1,fp);
				fseek(fp,(sp.inodeFreeListStartPos*sp.blockSize)+usedInodeCounter,SEEK_SET);
				fputc('1',fp);
				fseek(fp,sp.blockFreeListStartPos+usedBlock,SEEK_SET);
				fputc('1',fp);

			
			}
			slashNum--;
		}
		else{
			char path2[100]="";      //Bir slahs silme işlemi
			int i=0;
			while(path[i]!='\\'){
				path2[i]=path[i];
				i++;
			}
			path2[i]='\0';
		
			i++;
			int tempe=i;
			while(path[i]!='\0'){
				path[i-tempe]=path[i];
				i++;
			}
			path[i-tempe]='\0';



			bool isOccur=findEntryInCurrentFile(temp,path2);
			if (isOccur==false)
			{
				printf("This %s file not exist.\n",path2);
			}
			else{
				int j=0;
			
				while(strcmp(temp.directoryNames[j],path2)!=0 && j<10){
					j++;
				
				}
				
				int nodeNum=temp.inodeNumbers[j];
				struct INode ind;
				fseek(fp,sp.inodeStartPos*sp.blockSize,SEEK_SET);
				int q=0;
				while(q<NUMOFINODE){
					fread(&ind,sizeof(struct INode),1,fp);
					if (ind.inodeNum==nodeNum)
					{
						break;
					}
					q++;
				}
				int blockk=ind.blockNumbers[0];
				
				fseek(fp,blockk*sp.blockSize,SEEK_SET);
				fread(&temp,sizeof(struct DirectoryEntry),1,fp);
				

			}

			slashNum--;
		}
	}

}
void myDir(FILE * fp, char * filesystem, char * op, char * path){
	char directoryPath[1024];
	strcpy(directoryPath,path);
	int slashNum=findSlash(directoryPath);  //Slash sayısı dosya sayısını verir.
	struct INode mynode;
	int i=0;
	while(path[i]!='\0'){
		path[i]=path[i+1];
		i++;
	}
	slashNum--;
	struct DirectoryEntry temp;

	fseek(fp,sp.blockStartPos*sp.blockSize,SEEK_SET);
	fread(&temp, sizeof(struct DirectoryEntry), 1, fp);
	while(slashNum>=0){

		if (slashNum==0)//Roota ekleme yapmalıyız.
		{	


			if (strcmp(path,"root")==0)
			{
				for (int i = 0; i < temp.inner; ++i)
				{
					printf("%s\n",temp.directoryNames[i]);
				}
				return;
			}
			if (strcmp(path,temp.directoryName)!=0)
			{
				printf("This file not occur");
				return;
			}
			
			for (int i = 0; i < temp.inner; ++i)
			{
				printf("%s\n",temp.directoryNames[i]);
			}

			
			slashNum--;
		}			
		else{
			char path2[100]="";      //Bir slahs silme işlemi
			int i=0;
			while(path[i]!='\\'){
				path2[i]=path[i];
				i++;
			}
			path2[i]='\0';
			
			i++;
			int tempe=i;
			while(path[i]!='\0'){
				path[i-tempe]=path[i];
				i++;
			}
			path[i-tempe]='\0';



			bool isOccur=findEntryInCurrentFile(temp,path2);
			if (isOccur==false)
			{
				printf("This %s file not exist.\n",path2);
			}
			else{
				int j=0;
				
				while(strcmp(temp.directoryNames[j],path2)!=0 && j<10){
					j++;
			
				}
				
				int nodeNum=temp.inodeNumbers[j];
				struct INode ind;
				fseek(fp,sp.inodeStartPos*sp.blockSize,SEEK_SET);
				int q=0;
				while(q<NUMOFINODE){
					fread(&ind,sizeof(struct INode),1,fp);
					if (ind.inodeNum==nodeNum)
					{
						break;
					}
					q++;
				}
				int blockk=ind.blockNumbers[0];
				
				fseek(fp,blockk*sp.blockSize,SEEK_SET);
				fread(&temp,sizeof(struct DirectoryEntry),1,fp);
				

			}

			slashNum--;
		}
	}
}
void myDumpe2fs(FILE * fp, char * filesystem, char * op){
	printf("%-50s %d(byte) \n", "Block Size : ", sp.blockSize);
	printf("%-50s %d \n", "Number of i-nodes : ", NUMOFINODE);
	printf("%-50s %d \n", "Number of blocks : ", sp.numberOfBlock);
	

	int usedInodeCounter=0;
	fseek(fp,sp.inodeFreeListStartPos*sp.blockSize,SEEK_SET);
	char inode;
	while(1){
		fread(&inode, sizeof(char), 1, fp);
		if (inode=='0')
		{	
					
			break;
		}
		else{
			usedInodeCounter++;
					
		}

	}


	printf("%-50s %d \n", "Number of free i-nodes : ", NUMOFINODE-usedInodeCounter);
	printf("%-50s %d(byte) \n", "Size of super block : ", (int)sizeof(struct SuperBlock)*sp.blockSize);
	printf("%-50s %d(byte) \n", "Size of free-space management : ", (sp.inodeStartPos-sp.startFreeSpaceManagPos)*sp.blockSize);
	printf("%-50s %d(byte) \n", "Size of an i-node : ", sp.inodeSize);
	printf("%-50s %d(byte) \n", "Size of an entry : ", (int)sizeof(struct DirectoryEntry));
	printf("%-50s %d \n", "Start block adress of super block : ", 0);
	printf("%-50s %d \n", "Start block adress of free-space management : ", sp.startFreeSpaceManagPos*sp.blockSize);
	printf("%-50s %d \n", "Start block adress of i-nodes : ", sp.inodeStartPos*sp.blockSize);
	printf("%-50s %d \n", "Start block adress of root directory : ", sp.blockStartPos*sp.blockSize);
	printf("%-50s %d \n", "Start block adress of data blocks : ", (sp.blockStartPos+1)*sp.blockSize);
	printf("-----------------------------------------------------------------------------\n");
	struct INode mynode;
	
	fseek(fp,sp.inodeFreeListStartPos*sp.blockSize,SEEK_SET);
	usedInodeCounter=0;
	while(1){
		fread(&inode, sizeof(char), 1, fp);
		if (inode=='0')
		{	
					
			break;
		}
		else{
			usedInodeCounter++;
					
		}

	}
	for( int i = 0; i < usedInodeCounter; i++ ){
		fseek(fp, (sp.inodeStartPos*sp.blockSize) + sizeof(struct INode) * i, SEEK_SET );
		fread(&mynode, sizeof(struct INode), 1, fp);
		if(mynode.fileSize != -1 ){
			printf("Name: %s\t",mynode.fileName);
			if( mynode.fileType == 0 ){
				printf("Mode: %-15s ", "FILE");
			}
			else{
				printf("Mode: %-15s ", "DIRECTORY");
			}
			printf("I-node id: %-6d ", mynode.inodeNum);
			printf("size: %-5d \n", mynode.fileSize);

			printf("\n-----------------------------------------------------------------------------\n");
		}
	}
}

int main(int argc, char const *argv[])
{
	


	char op[100];
	char path[100];
	char filesystem[100];
	
	strcpy(filesystem,argv[1]);
    
    strcpy(op,argv[2]); 
    
	char * fileSystemData;							// take arguments.
    fileSystemData = strdup(argv[1]);  

	FILE *fp = fopen(fileSystemData, "rb+");
	if( fp == NULL){
		perror("Error!");
		return 0;
	}
	fseek(fp,0,SEEK_SET);
	fread(&sp, sizeof(struct SuperBlock), 1, fp);
	sp.blockFreeListStartPos=sp.inodeFreeListStartPos*sp.blockSize+NUMOFINODE;

	if (argc==4)
	{		
		strcpy(path,argv[3]);
	}


	if( strcmp(op, "mkdir") == 0 ){								// run the commands.
		myMkdir(fp, filesystem, op, path);
	}
	else if( strcmp(op, "dumpe2fs") == 0 ){
		myDumpe2fs(fp, filesystem, op);
	}
	else if (strcmp(op, "dir") == 0)
	{
		myDir(fp, filesystem, op, path);
	}
	else{
		printf("This command not found\n");
	}

	return 0;
}