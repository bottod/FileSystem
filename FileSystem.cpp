#include "FileSystem.h"

//关于fwrite
//调用格式：fwrite(buf,sizeof(buf),1,fp);
//成功写入返回值为1(即count)
//
//调用格式：fwrite(buf,1,sizeof(buf),fp);
//成功写入则返回实际写入的数据个数(单位为Byte)

//fflush用于立即把缓冲区的内容进行物理写入 避免ctrl+c引起错误

//文件系统默认信息
FileSystem::FileSystem(char* name)
{
	Init_SysCommand();											//初始化所有命令名字
	UserSize = sizeof(User);									//user区大小
	SuperBlockSize = sizeof(SuperBlock);						//SuperBlock区的大小
	InodeBitmapSize = BLOCK_NUM;								//inode对照表的大小
	BlockBitmapSize = BLOCK_NUM;								//block对照表的大小
	InodeSize = sizeof(Inode)*BLOCK_NUM;						//Inode区域大小
	BlockSize = BLOCK_SIZE*BLOCK_NUM;							//Block区域大小
	LoginSize = sizeof(char)*64;								//登录时间区域大小


	SuperBlockOffset = UserSize;								//SuperBlock区域偏移量，紧接着User区域
	InodeBitmapOffset = SuperBlockOffset + SuperBlockSize;		//inodeBitmap区域偏移量起始位置
	BlockBitmapOffset = InodeBitmapOffset + InodeBitmapSize;	//BlockBitmap区域偏移量起始位置
	InodeOffset = BlockBitmapOffset + BlockBitmapOffset;		//Inode区域的偏移量
	BlockOffset = InodeOffset + InodeSize;						//Block区域的偏移
	LoginOffset = BlockOffset + BlockSize;						//Login区域偏移

	fp = NULL;													//文件指针先赋为空
	if(name == NULL || strcmp(name,"") == 0)					//如果没有指定文件系统，就采用默认的名字
	{
		strncpy(system_name,DEFAULT_SYS_NAME,strlen(DEFAULT_SYS_NAME));
		system_name[strlen(DEFAULT_SYS_NAME)] = '\0';
	}
	else														//如果指定的名字大于16位，后面的不要
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

//初始化文件系统
int FileSystem::InitSystem()
{
	OpenFileSystem();
	login();
	command();
	return 0;
}

//等待命令
void FileSystem::command()
{
	//限制输入命令长度为128 
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

//打开文件系统，如果不存在就创建
void FileSystem::OpenFileSystem()
{
	if((fp = fopen(system_name,"rb")) == NULL)							//如果不存在指定的文件系统就创建它
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

//创建文件系统
void FileSystem::CreateFileSystem()
{
	char temp_username[1024];
	char temp_password[1024];										//防止用户输入过多  除非用户过于无聊，否则不会大于1024个字符

	printf("Creating FileSystem...\n");
	if((fp = fopen(system_name,"wb+")) == NULL)
	{
		printf("Create FileSystem %s error...\n",system_name);
		::exit(-1);
	}
	printf("\033[1;31mYou Need Register A User To Login !!!\033[0m\n\n");
	printf("username:");
	//fgets函数读入的字符串中最后包含读到的换行符'\n'
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
	//初始化Block信息
	SuperBlock_Info.blockSize = BLOCK_SIZE;
	SuperBlock_Info.blockNum = BLOCK_NUM;
	SuperBlock_Info.inodeNum = 1;
	SuperBlock_Info.blockFree = BLOCK_NUM-1;
	SetSuperBlock(SuperBlock_Info);

	//初始化两张对照表
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

	//初始化Inode和Block区域 数据区域 以及最后的LastLogin区域
	unsigned long len = 0;
	len += (sizeof(Inode) + BLOCK_SIZE) * BLOCK_NUM + sizeof(LastLoginTime);
	for(i = 0;i < len;i++)
		fputc(0,fp);

	//初始化根节点；并设置根节点的inode信息
	current_Inode.id = 0;
	strcpy(current_Inode.name,"/");
	current_Inode.isDir = 1;
	current_Inode.parentID = 0;
	current_Inode.length = 0;
	time(&(current_Inode.time));
	current_Inode.blockId = 0;
	//写入根节点的Inode
	SetInode(current_Inode);
	SetLoginTime();

	current_path = current_Inode.name;
}

//登录
void FileSystem::login()
{
	char temp_username[1024];
	char temp_password[1024];										//防止用户输入过多  除非用户过于无聊，否则不会大于1024个字符

	char username[17];												//这里如果取16会导致栈出问题，不清楚问题在哪里，改大就对了，但16我觉得逻辑上讲的通，可能和某些函数的内部实现有关
	char password[17];
	printf("\033[1;34mNow Login First !!!\033[0m\n\n");
	while (true)
	{
		printf("username:");
		//fgets函数读入的字符串中最后包含读到的换行符'\n'
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
		printf("password:\033[30;40m");		//用颜色来掩盖
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
		clear();//输入完密码默认清屏一次
		//如果是存储的密码
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

//清屏
void FileSystem::clear()
{
	system("cls");						
}

//显示当前路径
void FileSystem::showpath()
{
	printf("\033[1;32m\n%s@localhost %s\033[0m\n",user.username,current_path.data());
	printf("\033[1m$ ");
}

//初始化命令
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

//解析命令
int FileSystem::analysis(string str)
{
	//命令行最多输入5个参数，每个参数最多9个字符
	for(int i = 0;i < 5;i++)
		cmd[i][0] = '\0';
	sscanf(str.c_str(), "%s %s %s %s %s",cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
	for(int i = 1; i < COMMAND_NUM; i++)			//从一开始 错开sudo
    {
        if(strcmp(cmd[0], SysCommand[i].c_str()) == 0)
        {
            return i;
        }
    }
	if(strcmp(cmd[0], "") == 0)
    {
        return 0;//先拿sudo代替空命令直接回车
    }
	 return -1;
}

//退出程序
void FileSystem::exitSystem()
{
	fclose(fp);
	printf("\033[1;32mBye!\033[0m\n");
	exit(0);
}

//系统帮助文档
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

//设置user信息
void FileSystem::SetUser(User user)
{
	if(fp == NULL)
        return;
    rewind(fp);//重新调整文件内指针到文件开始位置
    fwrite(&user, UserSize, 1, fp);
	fflush(fp);
}

//获取之前设置的user信息
void FileSystem::GetUser(User *puser)
{
	if(fp == NULL)
        return;
	rewind(fp);
	fread(puser,UserSize,1,fp);
}

//设置Block块区域信息
void FileSystem::SetSuperBlock(SuperBlock block)
{
	if(fp == NULL)
        return;
	fseek(fp,SuperBlockOffset,SEEK_SET);
	fwrite(&block,SuperBlockSize,1,fp);
	fflush(fp);
}

//获取Block块区域的信息
void FileSystem::GetSuperBlock(SuperBlock *pblock)
{
	if(fp == NULL)
        return;
	fseek(fp,SuperBlockOffset,SEEK_SET);
	fread(pblock,SuperBlockSize,1,fp);	
}

//查看文件系统信息
void FileSystem::systemInfo()
{
    printf("Sum of block number:%d\n",SuperBlock_Info.blockNum);
    printf("Each block size:%d\n",SuperBlock_Info.blockSize);
    printf("Free of block number:%d\n",SuperBlock_Info.blockFree);
    printf("Sum of inode number:%d\n",SuperBlock_Info.inodeNum);
}

//设置BlockBitmap
void FileSystem::SetBlockBitmap(unsigned char* bitmap,unsigned int start,unsigned int count)
{
	if(fp == NULL)
		return;
	fseek(fp,BlockBitmapOffset+start,SEEK_SET);
	fwrite(bitmap+start,count,1,fp);
	fflush(fp);
}

//得到之前设置的BlockBitmap
void FileSystem::GetBlockBitmap(unsigned char* bitmap)
{
	if(fp == NULL)
		return;
	fseek(fp,BlockBitmapOffset,SEEK_SET);
	fread(bitmap,BlockBitmapSize,1,fp);
}

//设置InodeBitmap
void FileSystem::SetInodeBitmap(unsigned char* bitmap,unsigned int start,unsigned int count)
{
	if(fp == NULL)
		return;
	fseek(fp,InodeBitmapOffset+start,SEEK_SET);
	fwrite(bitmap+start,count,1,fp);
	fflush(fp);
}

//得到之前设置的InodeBitmap
void FileSystem::GetInodeBitmap(unsigned char* bitmap)
{
	if(fp == NULL)
		return;
	fseek(fp,InodeBitmapOffset,SEEK_SET);
	fread(bitmap,InodeBitmapSize,1,fp);
}

//设置Inode信息
void FileSystem::SetInode(Inode inode)
{
	if(fp == NULL )
        return;
    fseek(fp, InodeOffset+sizeof(Inode)*inode.id, SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fp);
	fflush(fp);
}


//取出一条inode
void FileSystem::GetInode(Inode *pInode, unsigned int id)
{
    if(fp == NULL || pInode == NULL)
        return;
    fseek(fp, InodeOffset+sizeof(Inode)*id, SEEK_SET);
    fread(pInode, sizeof(Inode), 1, fp);
}

//创建文件或者目录
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

//显示文件
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

//找文件或者目录 flag = 0 means find a file and flag = 1 means find a dir
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
	
	if(i == parentInode->length)			//如果没找到
		fileInodeID = -1;

	delete fcb;
	delete parentInode;
	return fileInodeID;
}

//进入新目录
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
			
			//顺序很重要
			GetInode(&current_Inode,current_Inode.parentID);		
		}
	}
}

//显示当前路径
void FileSystem::pwd()
{
	printf("\n%s\n",current_path.c_str());
}

//修改当前用户名的密码
void  FileSystem::passwd()
{

	char temp_password[1024];				//防止用户输入过多  除非用户过于无聊，否则不会大于1024个字符
	char passwordfirst[17];
	char passwordsecond[17];
	char temp_oldpassword[1024];//用户输入缓冲区
	char oldpassword[17];//用户输入的旧密码
	char existpasswd[17];//真实的旧密码
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

//设置最后登录时间
void FileSystem::SetLoginTime()
{
	time_t timep;
	time(&timep);
	strftime(LastLoginTime,sizeof(LastLoginTime),"%Y-%m-%d %H:%M:%S",localtime(&timep));
	fseek(fp,LoginOffset,SEEK_SET);
	fwrite(LastLoginTime,sizeof(LastLoginTime),1,fp);
	fflush(fp);
}

//得到最后登录时间
void FileSystem::GetLoginTime()
{
	fseek(fp,LoginOffset,SEEK_SET);
	fread(LastLoginTime,sizeof(LastLoginTime),1,fp);
}

//删除文件 关于删除目录还需要处理很多迭代的问题，所以分开吧(目录底下还有文件，要一起删掉，更新bitmap)
void FileSystem::rm(char* name)
{
	unsigned int fileInodeID,i;
	Inode *fileInode = new Inode();
	Inode *parentInode = new Inode();
	Fcb fcb[25]; //大致值应该在BLOCK_SIZE / sizeof(Fcb) 1024/40
	if((findInodeID(name,0) == -1) && (findInodeID(name,1) ==-1))
	{
		printf("\033[1;31mThere is no file or dir named %s  ...\033[0m\n",name);
		return;
	}
	else
	{
		//得到文件的InodeID
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
		//清空block区域
		fseek(fp,BlockOffset+parentInode->blockId*BLOCK_SIZE,SEEK_SET);
		for(i = 0; i < BLOCK_SIZE; i++)
			fputc(0,fp);
		//重新写入fcb信息(文件简易信息)  连续写入
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
	int fileInodeID,i=0;//i用于记录字数
	char c;//c记录输入
	Inode *fileInode = new Inode();
	if((findInodeID(name,0) == -1) && (findInodeID(name,1) == -1))
	{
		printf("\033[1;31mThere is no file or dir named %s  ...\033[0m\n",name);
		return;
	}
	else
	{
		//得到文件的InodeID
		if((fileInodeID = findInodeID(name,0)) == -1)
		{
			printf("\033[1;31mYou Can't Write words to A Dir\033[0m\n");
			return;
		}
		GetInode(fileInode,fileInodeID);
		fseek(fp,BlockOffset+fileInode->blockId*BLOCK_SIZE,SEEK_SET);
		//清空文件区域
		for(i = 0; i < BLOCK_SIZE; i++)
			fputc(0,fp);
		//重新写入文件
		fseek(fp,BlockOffset+fileInode->blockId*BLOCK_SIZE,SEEK_SET);
		printf("\033[1;33mNow Input Your Words(You Can Only Write 1000 Words):\033[0m\n");
		printf("\033[1;33mUse '#' To End Your Input!!\n\n\033[1;32m");
		i = 0;//i值重新赋值为0
		while ((c=getchar()) != '#')
		{
			fputc(c,fp);
			i++;
			if(i > BLOCK_SIZE-1)//如果超出字数则强制跳出不写入
				break;
		}
		getchar();//取出输入缓冲区中多余的换行符'\n'
		fileInode->length = i - 1;
		SetInode(*fileInode);
	}
	delete fileInode;
}

//read words from file
void FileSystem::cat(char* name)
{
	int fileInodeID;
	char c;//c记录输出
	Inode *fileInode = new Inode();
	if((findInodeID(name,0) == -1) && (findInodeID(name,1) == -1))
	{
		printf("\033[1;31mThere is no file or dir named %s  ...\033[0m\n",name);
		return;
	}
	else
	{
		//得到文件的InodeID
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