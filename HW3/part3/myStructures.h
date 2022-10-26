#define FILE_SIZE 8388608
#define TOTAL_BLOCKS 13
#define NUMOFINODE 100


struct SuperBlock
{
	int inodeSize;
	int inodeStartPos;
	
	int blockSize;
	int numberOfBlock;
	int blockStartPos;

	int startDatasPos;
	int startFreeSpaceManagPos;

	int inodeFreeListStartPos;
	int blockFreeListStartPos;

	int sizeDirectory;

};



struct INode
{
	int inodeNum;
	int fileSize;
	time_t creationTime;
	time_t lastModificationTime;
	char fileName[14];
	int fileType;		// 0 for regular files, 1 for directory files
	int blockNumbers[TOTAL_BLOCKS];

};


struct DirectoryEntry
{	
	int inner;
	int inodeNum;
	char directoryName[14];
	int inodeNumbers[10];
	char directoryNames[10][14];
};
