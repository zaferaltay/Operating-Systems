#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "myStructures.h"
#include <math.h>


int main(int argc, char const *argv[])
{
	


	struct SuperBlock sp;
	int blockSize;
	if(argc!=3)
	{
		printf("Invalid number of argument!\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		sp.inodeSize = (int) sizeof(struct INode);
		sp.blockSize = atoi(argv[1]) * 1024;
		blockSize=sp.blockSize;
		if( sp.blockSize <= 0){
			printf("%s\n", "Block Size should be greater than 0");
			return 0;
	    }
		
		sp.numberOfBlock=FILE_SIZE/sp.blockSize;
		sp.startFreeSpaceManagPos=ceil((float)sizeof(struct SuperBlock)/blockSize);
		
		sp.inodeFreeListStartPos=sp.startFreeSpaceManagPos;
		printf("%d\n",sp.inodeFreeListStartPos );
		sp.inodeStartPos=sp.startFreeSpaceManagPos+ceil((float)(NUMOFINODE+sp.numberOfBlock)/blockSize);
		printf("%d\n",sp.inodeStartPos );
		sp.blockStartPos=sp.inodeStartPos+ceil((float)(sizeof(struct INode)*NUMOFINODE)/blockSize);
		printf("%d\n",sp.blockStartPos );

		

	}


	FILE *fp = fopen(argv[2], "w+");

	if (fp == NULL)
	{
		perror("Error while opening the file.\n");
		exit(EXIT_FAILURE);
	}

	fseek(fp, FILE_SIZE - 1 , SEEK_SET);
	fputc(EOF ,fp);
	
//-------------------------------------------------SUPER BLOCK-------------------------
	fseek(fp, 0, SEEK_SET);
	fwrite (&sp, sizeof(struct SuperBlock), 1, fp);

//-------------------------------------------------------------------------------------
//-------------------------------------------------FREE LIST MANAG.--------------------	
	int inodeFreeListStartPosByte=sp.inodeFreeListStartPos*blockSize;
	fseek(fp, inodeFreeListStartPosByte, SEEK_SET);
	

	for (int i = 0; i < NUMOFINODE; i++) //inode ları 0ladım
	{
		fputc('0',fp);
	}

	sp.blockFreeListStartPos=inodeFreeListStartPosByte+NUMOFINODE;
	
	fseek(fp, sp.blockFreeListStartPos, SEEK_SET);
	for (int i = 0; i < sp.numberOfBlock; i++) //tüm blocklisti 0ladım.
	{
		fputc('0',fp);
	}
	fseek(fp, sp.blockFreeListStartPos, SEEK_SET);

	for (int i = 0; i < sp.blockStartPos; i++) //root block başlayana kadar hepsini 1ledim. Çünkü kullanılıyor.
	{
		fputc('1',fp);
	}
//-------------------------------------------------------------------------------------


//-------------------------------------------------INODES------------------------------	

	int inodeStartPosByte=sp.inodeStartPos*blockSize;
	fseek(fp, inodeStartPosByte, SEEK_SET);

	struct INode firstInode;
	fwrite (&firstInode, sizeof(struct INode), NUMOFINODE, fp);


	


//-------------------------------------------------------------------------------------

//-----------------------------------------------ROOT----------------------------------

	int rootStartPosByte=sp.blockStartPos*blockSize;
	fseek(fp,rootStartPosByte,SEEK_SET);

	struct DirectoryEntry root;
	root.inodeNum=0;
	root.inner=0;
	strcpy(root.directoryName,"root");
	fwrite(&root, sizeof(struct DirectoryEntry), 1, fp);





	inodeStartPosByte=sp.inodeStartPos*blockSize;
	fseek(fp,inodeStartPosByte,SEEK_SET);
	struct INode firstInode2;
	

	firstInode2.inodeNum=0;
	firstInode2.fileSize=ceil(sizeof(struct DirectoryEntry)/blockSize); 
	strcpy(firstInode2.fileName,"root");
	firstInode2.fileType=1;
	firstInode2.blockNumbers[0]=sp.blockStartPos;
	for(int i = 1; i < TOTAL_BLOCKS; ++i)
	{
		firstInode2.blockNumbers[i] = -1;
	}


	time_t creationTime;
	creationTime = time(0);
	firstInode2.creationTime=creationTime;
	firstInode2.lastModificationTime = creationTime;

	fseek(fp,inodeStartPosByte,SEEK_SET);
	fwrite(&firstInode2,sizeof(struct INode),1,fp);


	fseek(fp,inodeFreeListStartPosByte,SEEK_SET);
	fputc('1',fp);

	printf("%d\n",inodeFreeListStartPosByte);
	printf("%d\n",sp.blockFreeListStartPos);
	printf("%d\n",sp.inodeStartPos*blockSize);
	fseek(fp,sp.blockFreeListStartPos,SEEK_SET);
	for (int i = 0; i < sp.blockStartPos; ++i)
	{
		fputc('1',fp);
	}
	


//---------------------------------------------------------------------------------------



	struct INode inp;
	fseek(fp,inodeStartPosByte,SEEK_SET);
	fread(&inp,sizeof(struct INode),1,fp);
	printf("Inode name:%s\n",inp.fileName);
	printf("Root ınode num:%d\n",inp.inodeNum );

	fseek(fp,sp.blockFreeListStartPos,SEEK_SET);




	fclose(fp);




	return 0;
}
