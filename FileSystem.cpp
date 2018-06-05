#include "FileSystem.h"

//����fwrite
//���ø�ʽ��fwrite(buf,sizeof(buf),1,fp);
//�ɹ�д�뷵��ֵΪ1(��count)
//
//���ø�ʽ��fwrite(buf,1,sizeof(buf),fp);
//�ɹ�д���򷵻�ʵ��д������ݸ���(��λΪByte)

//fflush���������ѻ����������ݽ�������д�� ����ctrl+c�������

//�ļ�ϵͳĬ����Ϣ
FileSystem::FileSystem(char* name)
{
	Init_SysCommand();											//��ʼ��������������
	UserSize = sizeof(User);									//user����С
	SuperBlockSize = sizeof(SuperBlock);						//SuperBlock���Ĵ�С
	InodeBitmapSize = BLOCK_NUM;								//inode���ձ�Ĵ�С
	BlockBitmapSize = BLOCK_NUM;								//block���ձ�Ĵ�С
	InodeSize = sizeof(Inode)*BLOCK_NUM;						//Inode�����С
	BlockSize = BLOCK_SIZE*BLOCK_NUM;							//Block�����С
	LoginSize = sizeof(char)*64;								//��¼ʱ�������С


	SuperBlockOffset = UserSize;								//SuperBlock����ƫ������������User����
	InodeBitmapOffset = SuperBlockOffset + SuperBlockSize;		//inodeBitmap����ƫ������ʼλ��
	BlockBitmapOffset = InodeBitmapOffset + InodeBitmapSize;	//BlockBitmap����ƫ������ʼλ��
	InodeOffset = BlockBitmapOffset + BlockBitmapOffset;		//Inode�����ƫ����
	BlockOffset = InodeOffset + InodeSize;						//Block�����ƫ��
	LoginOffset = BlockOffset + BlockSize;						//Login����ƫ��

	fp = NULL;													//�ļ�ָ���ȸ�Ϊ��
	if(name == NULL || strcmp(name,"") == 0)					//���û��ָ���ļ�ϵͳ���Ͳ���Ĭ�ϵ�����
	{
		strncpy(system_name,DEFAULT_SYS_NAME,strlen(DEFAULT_SYS_NAME));
		system_name[strlen(DEFAULT_SYS_NAME)] = '\0';
	}
	else														//���ָ�������ִ���16λ������Ĳ�Ҫ
	{ 
		if(strlen(name) > 16)
		{
			strncpy(system_name,name,16);
			system_name[16] = '\0';
		}
		else
		{
			strcpy(system_name,name);
		}
	}
}


FileSystem::~FileSystem(void)
{
}

//��ʼ���ļ�ϵͳ
int FileSystem::InitSystem()
{
	OpenFileSystem();
	login();
	command();
	return 0;
}

//�ȴ�����
void FileSystem::command()
{
	//�������������Ϊ128 
	char input[128];
	while (true)
	{
		showpath();
		fgets(input,128,stdin);
		if(input[strlen(input)-1] == '\n')
			input[strlen(input)-1] = '\0';
		switch (analysis(input))
		{
			case SUDO:
				break;
			case CLS:
				clear();
				break;
			case EXIT:
				exitSystem();
				break;	
			case HELP:
				help();
				break;
			case SYSINFO:
				systemInfo();
				break;
			case MKDIR:
				createFile(cmd[1],1);
				break;
			case TOUCH:
				createFile(cmd[1],0);
				break;
			case LS:
				ls(cmd[1]);
				break;
			case CD:
				cd(cmd[1]);
				break;
			case PWD:
				pwd();
				break;
			case PASSWD:
				passwd();
				break;
			case RM:
				rm(cmd[1]);
				break;
			case NANO:
				nano(cmd[1]);
				break;
			case CAT:
				cat(cmd[1]);
				break;
			default:
				printf("\033[1;31mUnkown Command!!!	Please Check Your Input...\033[0m\n");
				break;
		}
	}
}

//���ļ�ϵͳ����������ھʹ���
void FileSystem::OpenFileSystem()
{
	if((fp = fopen(system_name,"rb")) == NULL)							//���������ָ�����ļ�ϵͳ�ʹ�����
		CreateFileSystem();
	else
	{
		printf("Opening FileSystem...\n");
		 if ((fp=fopen(system_name,"rb+")) == NULL)
        {
            printf("Open FileSystem '%s' error...\n", system_name);
            ::exit(-1);
        }
        GetUser(&user);
		GetSuperBlock(&SuperBlock_Info);
		GetInodeBitmap(InodeBitmap);
		GetBlockBitmap(BlockBitmap);

		GetInode(&current_Inode,0);
		current_path = current_Inode.name;

	}
}

//�����ļ�ϵͳ
void FileSystem::CreateFileSystem()
{
	char temp_username[1024];
	char temp_password[1024];										//��ֹ�û��������  �����û��������ģ����򲻻����1024���ַ�

	printf("Creating FileSystem...\n");
	if((fp = fopen(system_name,"wb+")) == NULL)
	{
		printf("Create FileSystem %s error...\n",system_name);
		::exit(-1);
	}
	printf("\033[1;31mYou Need Register A User To Login !!!\033[0m\n\n");
	printf("username:");
	//fgets����������ַ����������������Ļ��з�'\n'
	fgets(temp_username,sizeof(temp_username),stdin);
	if(strlen(temp_username) > 16)
	{
		strncpy(user.username,temp_username,16);
		user.username[15] = '\0';
		printf("\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n");
	}
	else
	{
		strcpy(user.username,temp_username);
		user.username[strlen(user.username)-1] = '\0';
	}
	printf("password:\033[30;40m");
	fgets(temp_password,sizeof(temp_password),stdin);
	if(strlen(temp_password) > 16)
	{
		strncpy(user.password,temp_password,16);
		user.password[15] = '\0';
		printf("\033[0m\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n\n");
	}
	else
	{
		printf("\033[0m");
		strcpy(user.password,temp_password);
		user.password[strlen(user.password)-1] = '\0';
	}
	printf("\033[1;33mPlease Wait A Moment!!! The File System is Creating Now!!!\033[0m\n\n");
	SetUser(user);
	//��ʼ��Block��Ϣ
	SuperBlock_Info.blockSize = BLOCK_SIZE;
	SuperBlock_Info.blockNum = BLOCK_NUM;
	SuperBlock_Info.inodeNum = 1;
	SuperBlock_Info.blockFree = BLOCK_NUM-1;
	SetSuperBlock(SuperBlock_Info);

	//��ʼ�����Ŷ��ձ�
	unsigned long i;
	//root  Inode and Block 
	InodeBitmap[0] = 1;
	BlockBitmap[0] = 1;
	for(i = 1;i <BLOCK_NUM;i++)
	{
		InodeBitmap[i] = 0;
		BlockBitmap[i] = 0;
	}
	SetInodeBitmap(InodeBitmap, 0, InodeBitmapSize);			//1024
	SetBlockBitmap(BlockBitmap, 0, BlockBitmapSize);

	//��ʼ��Inode��Block���� �������� �Լ�����LastLogin����
	unsigned long len = 0;
	len += (sizeof(Inode) + BLOCK_SIZE) * BLOCK_NUM + sizeof(LastLoginTime);
	for(i = 0;i < len;i++)
		fputc(0,fp);

	//��ʼ�����ڵ㣻�����ø��ڵ��inode��Ϣ
	current_Inode.id = 0;
	strcpy(current_Inode.name,"/");
	current_Inode.isDir = 1;
	current_Inode.parentID = 0;
	current_Inode.length = 0;
	time(&(current_Inode.time));
	current_Inode.blockId = 0;
	//д����ڵ��Inode
	SetInode(current_Inode);
	SetLoginTime();

	current_path = current_Inode.name;
}

//��¼
void FileSystem::login()
{
	char temp_username[1024];
	char temp_password[1024];										//��ֹ�û��������  �����û��������ģ����򲻻����1024���ַ�

	char username[17];												//�������ȡ16�ᵼ��ջ�����⣬���������������Ĵ�Ͷ��ˣ���16�Ҿ����߼��Ͻ���ͨ�����ܺ�ĳЩ�������ڲ�ʵ���й�
	char password[17];
	printf("\033[1;34mNow Login First !!!\033[0m\n\n");
	while (true)
	{
		printf("username:");
		//fgets����������ַ����������������Ļ��з�'\n'
		fgets(temp_username,sizeof(temp_username),stdin);
		if(strlen(temp_username) > 16)
		{
			strncpy(username,temp_username,16);
			username[15] = '\0';
			printf("\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n\n");
		}
		else
		{
			strcpy(username,temp_username);
			username[strlen(username)-1] = '\0';
		}
		printf("password:\033[30;40m");		//����ɫ���ڸ�
		fgets(temp_password,sizeof(temp_password),stdin);
		if(strlen(temp_password) > 16)
		{
			strncpy(password,temp_password,16);
			password[15] = '\0';
			printf("\033[0m\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n\n");
		}
		else
		{
			printf("\033[0m");
			strcpy(password,temp_password);
			password[strlen(password)-1] = '\0';
		}
		clear();//����������Ĭ������һ��
		//����Ǵ洢������
		if(strcmp(username,user.username)==0 && strcmp(password,user.password)==0)
		{
			printf("\033[1;33m\nCongratulation!!!\nSuccessfully Unlocked This System!!!\nNow Enjoy it.....\nEnter help for help...\033[0m\n");
            GetLoginTime();
			printf("\033[1;33mLast Login: %s\033[0m\n\n",LastLoginTime);
			SetLoginTime();
			break;
		}
        else
        {
            printf("\033[1;4;31musername or password is not correct,please try again...\033[0m\n");
        }
	}
}

//����
void FileSystem::clear()
{
	system("cls");						
}

//��ʾ��ǰ·��
void FileSystem::showpath()
{
	printf("\033[1;32m\n%s@localhost %s\033[0m\n",user.username,current_path.data());
	printf("\033[1m$ ");
}

//��ʼ������
void  FileSystem::Init_SysCommand()
{
	SysCommand[0] = "sudo";
	SysCommand[1] = "cls";
	SysCommand[2] = "exit";
	SysCommand[3] = "help";
	SysCommand[4] = "sysinfo";
	SysCommand[5] = "mkdir";
	SysCommand[6] = "touch";
	SysCommand[7] = "ls";
	SysCommand[8] = "cd";
	SysCommand[9] = "pwd";
	SysCommand[10] = "passwd";
	SysCommand[11] = "rm";
	SysCommand[12] = "nano";
	SysCommand[13] = "cat";
}

//��������
int FileSystem::analysis(string str)
{
	//�������������5��������ÿ���������9���ַ�
	for(int i = 0;i < 5;i++)
		cmd[i][0] = '\0';
	sscanf(str.c_str(), "%s %s %s %s %s",cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
	for(int i = 1; i < COMMAND_NUM; i++)			//��һ��ʼ ��sudo
    {
        if(strcmp(cmd[0], SysCommand[i].c_str()) == 0)
        {
            return i;
        }
    }
	if(strcmp(cmd[0], "") == 0)
    {
        return 0;//����sudo���������ֱ�ӻس�
    }
	 return -1;
}

//�˳�����
void FileSystem::exitSystem()
{
	fclose(fp);
	printf("\033[1;32mBye!\033[0m\n");
	exit(0);
}

//ϵͳ�����ĵ�
void FileSystem::help()
{
	printf("command: \n\
    help        ---  show help menu \n\
    cls         ---  clear the screen \n\
    exit        ---  exit this system\n\
    sysinfo     ---  show system base infomation\n\
    mkdir       ---  make directory   \n\
    touch       ---  create a new file \n\
    ls          ---  show files\n\
    cd          ---  change into a new path\n\
    pwd         ---  show current path\n\
    password    ---  modify your password\n\
    rm          ---  delete a file(Not Dir)\n\
    nano        ---  write words in file\n\
    cat         ---  read words in file");
}

//����user��Ϣ
void FileSystem::SetUser(User user)
{
	if(fp == NULL)
        return;
    rewind(fp);//���µ����ļ���ָ�뵽�ļ���ʼλ��
    fwrite(&user, UserSize, 1, fp);
	fflush(fp);
}

//��ȡ֮ǰ���õ�user��Ϣ
void FileSystem::GetUser(User *puser)
{
	if(fp == NULL)
        return;
	rewind(fp);
	fread(puser,UserSize,1,fp);
}

//����Block��������Ϣ
void FileSystem::SetSuperBlock(SuperBlock block)
{
	if(fp == NULL)
        return;
	fseek(fp,SuperBlockOffset,SEEK_SET);
	fwrite(&block,SuperBlockSize,1,fp);
	fflush(fp);
}

//��ȡBlock���������Ϣ
void FileSystem::GetSuperBlock(SuperBlock *pblock)
{
	if(fp == NULL)
        return;
	fseek(fp,SuperBlockOffset,SEEK_SET);
	fread(pblock,SuperBlockSize,1,fp);	
}

//�鿴�ļ�ϵͳ��Ϣ
void FileSystem::systemInfo()
{
    printf("Sum of block number:%d\n",SuperBlock_Info.blockNum);
    printf("Each block size:%d\n",SuperBlock_Info.blockSize);
    printf("Free of block number:%d\n",SuperBlock_Info.blockFree);
    printf("Sum of inode number:%d\n",SuperBlock_Info.inodeNum);
}

//����BlockBitmap
void FileSystem::SetBlockBitmap(unsigned char* bitmap,unsigned int start,unsigned int count)
{
	if(fp == NULL)
		return;
	fseek(fp,BlockBitmapOffset+start,SEEK_SET);
	fwrite(bitmap+start,count,1,fp);
	fflush(fp);
}

//�õ�֮ǰ���õ�BlockBitmap
void FileSystem::GetBlockBitmap(unsigned char* bitmap)
{
	if(fp == NULL)
		return;
	fseek(fp,BlockBitmapOffset,SEEK_SET);
	fread(bitmap,BlockBitmapSize,1,fp);
}

//����InodeBitmap
void FileSystem::SetInodeBitmap(unsigned char* bitmap,unsigned int start,unsigned int count)
{
	if(fp == NULL)
		return;
	fseek(fp,InodeBitmapOffset+start,SEEK_SET);
	fwrite(bitmap+start,count,1,fp);
	fflush(fp);
}

//�õ�֮ǰ���õ�InodeBitmap
void FileSystem::GetInodeBitmap(unsigned char* bitmap)
{
	if(fp == NULL)
		return;
	fseek(fp,InodeBitmapOffset,SEEK_SET);
	fread(bitmap,InodeBitmapSize,1,fp);
}

//����Inode��Ϣ
void FileSystem::SetInode(Inode inode)
{
	if(fp == NULL )
        return;
    fseek(fp, InodeOffset+sizeof(Inode)*inode.id, SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fp);
	fflush(fp);
}


//ȡ��һ��inode
void FileSystem::GetInode(Inode *pInode, unsigned int id)
{
    if(fp == NULL || pInode == NULL)
        return;
    fseek(fp, InodeOffset+sizeof(Inode)*id, SEEK_SET);
    fread(pInode, sizeof(Inode), 1, fp);
}

//�����ļ�����Ŀ¼
int FileSystem::createFile(char* name,unsigned short isDir)
{
	unsigned int i,nowBlockID,nowInodeID;
	Inode* fileInode = new Inode();
	Inode* parentInode = new Inode();
	Fcb* fcb = new Fcb();

	//available InodeID
	//BLOCK_NUM is the same as INODE_NUM
	for (i = 0; i < BLOCK_NUM; i++)
	{
		if(i != BLOCK_NUM-1)
		{
			if(InodeBitmap[i] == 0)
			{
				nowInodeID = i;
				break;
			}
		}
		else
		{
			printf("InodeBitmap is Full!!! Can't Create!!!\n");
			return -1;
		}
	}

	//available blockID
	for (i = 0; i < BLOCK_NUM; i++)
	{
		if(i != BLOCK_NUM-1)
		{
			if(BlockBitmap[i] == 0)
			{
				nowBlockID = i;
				break;
			}
		}
		else
		{
			printf("BlockBitmap is Full!!! Can't Create!!!\n");
			return -1;
		}
	}

	//update parent dir info
	fseek(fp,InodeOffset+current_Inode.id*sizeof(Inode),SEEK_SET);
	fread(parentInode,sizeof(Inode),1,fp);

	//init fileInode
	if(name == NULL || strcmp(name,"") == 0)
	{
		printf("\033[1;31mAt least one parameter is required When use this command!!!\033[0m\n");
		return -1;
	}
	else
	{
		if(strlen(name) > 29)
		{
			strncpy(fileInode->name,name,29);
			fileInode->name[29] = '\0';
		}
		else
		{
			strcpy(fileInode->name,name);
		}

		for(i=0;i<parentInode->length;i++)
		{
			fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE+i*sizeof(Fcb),SEEK_SET);
			fread(fcb,sizeof(Fcb),1,fp);
			if(strcmp(name,fcb->name) == 0)
			{
				printf("\033[1;31mWarning! You Can't Use This Name!!\n");
				return -1;
			}
		}
	}
	fileInode->id = nowInodeID;
	fileInode->blockId = nowBlockID;
	fileInode->parentID = current_Inode.id;
	time(&(fileInode->time));
	if(isDir)
		fileInode->isDir = 1;
	else
		fileInode->isDir = 0;
	fileInode->length = 0;

	//write file info to Inode
	fseek(fp,InodeOffset+sizeof(Inode)*nowInodeID,SEEK_SET);
	fwrite(fileInode,sizeof(Inode),1,fp);
	fflush(fp);

	//update superblock and bitmap
	SuperBlock_Info.blockFree -= 1;
	SuperBlock_Info.inodeNum += 1;
	SetSuperBlock(SuperBlock_Info);
	InodeBitmap[nowInodeID] = 1;
	SetInodeBitmap(InodeBitmap,nowInodeID*sizeof(unsigned char),sizeof(unsigned char));
	BlockBitmap[nowBlockID] = 1;
	SetBlockBitmap(BlockBitmap,nowBlockID*sizeof(unsigned char),sizeof(unsigned char));

	//init fcb info
	strcpy(fcb->name,fileInode->name);
	fcb->id = fileInode->id;
	fcb->isDir = fileInode->isDir;
	fcb->blockId = fileInode->blockId;

	//parent block
	if((parentInode->length + 1) * sizeof(Fcb) < BLOCK_SIZE)
	{
		fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE+parentInode->length*sizeof(Fcb),SEEK_SET);
		fwrite(fcb,sizeof(Fcb),1,fp);
		parentInode->length += 1;
		fseek(fp,InodeOffset+current_Inode.id*sizeof(Inode),SEEK_SET);
		fwrite(parentInode,sizeof(Inode),1,fp);
		fflush(fp);
	}
	else
	{
		delete fileInode;
		delete parentInode;
		delete fcb;
		printf("Can't create More in this dir");
		return -1;
	}



	delete fileInode;
	delete parentInode;
	delete fcb;
	fflush(fp);
	return 0;
	
}

//��ʾ�ļ�
void FileSystem::ls(char *arg)
{
	unsigned int i;
	Fcb *fcb = new Fcb();
	Inode *parentInode = new Inode();
	fseek(fp,InodeOffset+current_Inode.id*sizeof(Inode),SEEK_SET);
	fread(parentInode,sizeof(Inode),1,fp);
	if(arg == NULL || strcmp(arg,"") == 0)
	{
		//list info
		for(i=0;i<parentInode->length;i++)
		{
			fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE+i*sizeof(Fcb),SEEK_SET);
			fread(fcb,sizeof(Fcb),1,fp);
			if(fcb->isDir)
			{
				printf("\033[1;32m%s\033[0m",fcb->name);
				printf("    ");
			}
			else
			{
				printf("%s",fcb->name);
				printf("    ");
			}
		}
		printf("\n");
	}
	else if(strcmp(arg,"-l") == 0)
	{
		//list info
		for(i=0;i<parentInode->length;i++)
		{
			fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE+i*sizeof(Fcb),SEEK_SET);
			fread(fcb,sizeof(Fcb),1,fp);

			printf("%s   isDir:%d   blockID:%d\n",fcb->name,fcb->isDir,fcb->blockId);
		}
	}
	else
	{
		printf("\033[1;31mNo Such arg \n");
	}
}

//���ļ�����Ŀ¼ flag = 0 means find a file and flag = 1 means find a dir
int FileSystem::findInodeID(char* name,int flag)
{
	unsigned int i,fileInodeID;
	Inode *parentInode = new Inode();
	Fcb *fcb = new Fcb();

	//read current inode info
	fseek(fp,InodeOffset+current_Inode.id*sizeof(Inode),SEEK_SET);
	fread(parentInode,sizeof(Inode),1,fp);

	//read fcb in this directory
	for(i=0;i<parentInode->length;i++)
	{
		fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE+i*sizeof(Fcb),SEEK_SET);
		fread(fcb,sizeof(Fcb),1,fp);
		if(flag)
		{
			if((fcb->isDir == 1) && (strcmp(name,fcb->name) == 0))
			{
				fileInodeID = fcb->id;
				break;
			}
		}
		else
		{
			if((fcb->isDir == 0) && (strcmp(name,fcb->name) == 0))
			{
				fileInodeID = fcb->id;
				break;
			}
		}
	}
	
	if(i == parentInode->length)			//���û�ҵ�
		fileInodeID = -1;

	delete fcb;
	delete parentInode;
	return fileInodeID;
}

//������Ŀ¼
void FileSystem::cd(char *name)
{
	int fileInodeID;
	if (strcmp(name,"..") != 0)
	{
		fileInodeID = findInodeID(name,1);
		if(fileInodeID == -1)
			printf("\033[1;31mThere have no dir named %s   ...",name);
		else
		{
			GetInode(&current_Inode,fileInodeID);

			current_path += current_Inode.name; 
			current_path += "/";
		}
	}
	else
	{
		if(current_Inode.id != 0)
		{	
			string nowpath = current_Inode.name;
			nowpath += "/";
			current_path.erase(current_path.end() - strlen(nowpath.c_str()),current_path.end());
			
			//˳�����Ҫ
			GetInode(&current_Inode,current_Inode.parentID);		
		}
	}
}

//��ʾ��ǰ·��
void FileSystem::pwd()
{
	printf("\n%s\n",current_path.c_str());
}

//�޸ĵ�ǰ�û���������
void  FileSystem::passwd()
{

	char temp_password[1024];				//��ֹ�û��������  �����û��������ģ����򲻻����1024���ַ�
	char passwordfirst[17];
	char passwordsecond[17];
	char temp_oldpassword[1024];//�û����뻺����
	char oldpassword[17];//�û�����ľ�����
	char existpasswd[17];//��ʵ�ľ�����
	printf("\033[1;32mNow Modify Your Password!!!\033[0m\n");
	printf("Old Password:\033[30;40m");
	fgets(temp_oldpassword,sizeof(temp_oldpassword),stdin);
	if(strlen(temp_oldpassword) > 16)
	{
		strncpy(oldpassword,temp_oldpassword,16);
		oldpassword[15] = '\0';
		printf("\033[0m\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n\n");
	}
	else
	{
		printf("\033[0m");
		strcpy(oldpassword,temp_oldpassword);
		oldpassword[strlen(oldpassword)-1] = '\0';
	}
	fseek(fp,sizeof(char)*16,SEEK_SET);
	fread(existpasswd,sizeof(char)*16,1,fp);
	if(strcmp(existpasswd,oldpassword) == 0)
	{
		printf("New Password:\033[30;40m");
		fgets(temp_password,sizeof(temp_password),stdin);
		if(strlen(temp_password) > 16)
		{
			strncpy(passwordfirst,temp_password,16);
			passwordfirst[15] = '\0';
			printf("\033[0m\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n\n");
		}
		else
		{
			printf("\033[0m");
			strcpy(passwordfirst,temp_password);
			passwordfirst[strlen(passwordfirst)-1] = '\0';
		}

		printf("New Password Confirm:\033[30;40m");
		fgets(temp_password,sizeof(temp_password),stdin);
		if(strlen(temp_password) > 16)
		{
			strncpy(passwordsecond,temp_password,16);
			passwordsecond[15] = '\0';
			printf("\033[0m\033[1;31mWaring!!! Your Input is too long!!! We will cat only 15 characters!!!\033[0m\n\n");
		}
		else
		{
			printf("\033[0m");
			strcpy(passwordsecond,temp_password);
			passwordsecond[strlen(passwordsecond)-1] = '\0';
		}
		if(strcmp(passwordfirst,passwordsecond) == 0 )
		{
			printf("\033[1;33mModify Password Successfully!!!\033[0m\n");
			fseek(fp,sizeof(char)*16,SEEK_SET);
			fwrite(passwordsecond,sizeof(char)*16,1,fp);
			fflush(fp);
		}
		else
		{
			printf("\033[1;31mThe two passwords are different!!\033[0m\n");
			return;
		}
	}
	else
	{
		printf("\033[1;31mOld Password Error!!!\033[0m\n");
		return;
	}
}

//��������¼ʱ��
void FileSystem::SetLoginTime()
{
	time_t timep;
	time(&timep);
	strftime(LastLoginTime,sizeof(LastLoginTime),"%Y-%m-%d %H:%M:%S",localtime(&timep));
	fseek(fp,LoginOffset,SEEK_SET);
	fwrite(LastLoginTime,sizeof(LastLoginTime),1,fp);
	fflush(fp);
}

//�õ�����¼ʱ��
void FileSystem::GetLoginTime()
{
	fseek(fp,LoginOffset,SEEK_SET);
	fread(LastLoginTime,sizeof(LastLoginTime),1,fp);
}

//ɾ���ļ� ����ɾ��Ŀ¼����Ҫ����ܶ���������⣬���Էֿ���(Ŀ¼���»����ļ���Ҫһ��ɾ��������bitmap)
void FileSystem::rm(char* name)
{
	unsigned int fileInodeID,i;
	Inode *fileInode = new Inode();
	Inode *parentInode = new Inode();
	Fcb fcb[25]; //����ֵӦ����BLOCK_SIZE / sizeof(Fcb) 1024/40
	if((findInodeID(name,0) == -1) && (findInodeID(name,1) ==-1))
	{
		printf("\033[1;31mThere is no file or dir named %s  ...\033[0m\n",name);
		return;
	}
	else
	{
		//�õ��ļ���InodeID
		if((fileInodeID = findInodeID(name,0)) == -1)
		{
			printf("\033[1;31mYou Can't Use this Command to delete A Dir\033[0m\n");
			return;
		}
		GetInode(fileInode,fileInodeID);
		GetInode(parentInode,current_Inode.id);

		for(i=0;i<parentInode->length;i++)
		{
			fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE+i*sizeof(Fcb),SEEK_SET);
			fread(&fcb[i],sizeof(Fcb),1,fp);
		}
		//���block����
		fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE,SEEK_SET);
		for(i = 0; i < BLOCK_SIZE; i++)
			fputc(0,fp);
		//����д��fcb��Ϣ(�ļ�������Ϣ)  ����д��
		fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE,SEEK_SET);
		for(i = 0; i < parentInode->length; i++)
		{
			if(strcmp(fcb[i].name,name) != 0)
			{
				fwrite(&fcb[i],sizeof(Fcb),1,fp);
			}
		}
		
		parentInode->length -= 1;
		SetInode(*parentInode);

		InodeBitmap[fileInodeID] = 0;
		SetInodeBitmap(InodeBitmap,fileInodeID*sizeof(unsigned char),sizeof(unsigned char));
		BlockBitmap[fileInode->blockId] = 0;
		SetBlockBitmap(BlockBitmap,fileInode->blockId*sizeof(unsigned char),sizeof(unsigned char));

		SuperBlock_Info.blockFree += 1;
		SuperBlock_Info.inodeNum -= 1;
		SetSuperBlock(SuperBlock_Info);
	}

	delete fileInode;
	delete parentInode;
}

//Write words to file
void FileSystem::nano(char *name)
{
	int fileInodeID,i=0;//i���ڼ�¼����
	char c;//c��¼����
	Inode *fileInode = new Inode();
	if((findInodeID(name,0) == -1) && (findInodeID(name,1) == -1))
	{
		printf("\033[1;31mThere is no file or dir named %s  ...\033[0m\n",name);
		return;
	}
	else
	{
		//�õ��ļ���InodeID
		if((fileInodeID = findInodeID(name,0)) == -1)
		{
			printf("\033[1;31mYou Can't Write words to A Dir\033[0m\n");
			return;
		}
		GetInode(fileInode,fileInodeID);
		fseek(fp,BlockOffset+fileInode->blockId*BLOCK_SIZE,SEEK_SET);
		//����ļ�����
		for(i = 0; i < BLOCK_SIZE; i++)
			fputc(0,fp);
		//����д���ļ�
		fseek(fp,BlockOffset+fileInode->blockId*BLOCK_SIZE,SEEK_SET);
		printf("\033[1;33mNow Input Your Words(You Can Only Write 1000 Words):\033[0m\n");
		printf("\033[1;33mUse '#' To End Your Input!!\n\n\033[1;32m");
		i = 0;//iֵ���¸�ֵΪ0
		while ((c=getchar()) != '#')
		{
			fputc(c,fp);
			i++;
			if(i > BLOCK_SIZE-1)//�������������ǿ��������д��
				break;
		}
		getchar();//ȡ�����뻺�����ж���Ļ��з�'\n'
		fileInode->length = i - 1;
		SetInode(*fileInode);
	}
	delete fileInode;
}

//read words from file
void FileSystem::cat(char* name)
{
	int fileInodeID;
	char c;//c��¼���
	Inode *fileInode = new Inode();
	if((findInodeID(name,0) == -1) && (findInodeID(name,1) == -1))
	{
		printf("\033[1;31mThere is no file or dir named %s  ...\033[0m\n",name);
		return;
	}
	else
	{
		//�õ��ļ���InodeID
		if((fileInodeID = findInodeID(name,0)) == -1)
		{
			printf("\033[1;31mYou Can't Read words From A Dir\033[0m\n");
			return;
		}
		GetInode(fileInode,fileInodeID);
		fseek(fp,BlockOffset+fileInode->blockId*BLOCK_SIZE,SEEK_SET);
		
		//read words
		int tmplen = fileInode->length;
		printf("\n\033[1;33m");
		while (tmplen >= 0)
		{
			c=fgetc(fp);
			putchar(c);
			tmplen--;
		}
		printf("\033[0m\n");
	}
	delete fileInode;
}