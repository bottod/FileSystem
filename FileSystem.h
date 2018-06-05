/*
用户区域;Block区域
*/

#include<time.h>
#include<string.h>
#include<string>
using namespace std;

//定义默认文件系统的名字 不得超过16位！！
const char DEFAULT_SYS_NAME[] = "SysDefault";

//定义命令数量
const int COMMAND_NUM = 14;

//定义默认文件块大小以及默认文件块数量
const unsigned int BLOCK_SIZE = 1024;
const unsigned int BLOCK_NUM = 1024;

//命令结构
//该结构需要和SysCommand数组里的值一一对应
enum Command
{
	SUDO=0,
	CLS,
	EXIT,
	HELP,
	SYSINFO,
	MKDIR,
	TOUCH,
	LS,
	CD,
	PWD,
	PASSWD,
	RM,
	NANO,
	CAT
};

//用户结构
typedef struct
{
	char username[16];								//用户名，限制长度15个字符 最后一个为\0
	char password[16];								//用户密码，限制长度15个字符
}User;

//文件系统SuperBlock块结构
typedef struct{
    unsigned short blockSize;						//文件块大小
    unsigned int blockNum;							//文件块数量
    unsigned int inodeNum;							//i节点数量,即文件数量
    unsigned int blockFree;							//空闲块数量
}SuperBlock;


//文件系统Inode结构
typedef struct{
    unsigned int id;								//Inode索引
    char name[30];									//文件名，最大长度29
    unsigned char isDir;							//文件类型 0-file 1-dir
    unsigned int parentID;							//父目录inode索引
    unsigned int length;							//文件长度,最大为BLOCK_SIZE/sizeof(Fcb)
    time_t time;									//文件最后修改时间，从1970年1月1日00时00分00秒至今所经过的秒数
    unsigned int blockId;							//文件项所在的目录数据块的id，便于删除时定位
}Inode;

//文件的部分信息
typedef struct 
{
	unsigned int id;
	char name[30];
	unsigned char isDir;
	unsigned int blockId;
}Fcb;

class FileSystem
{
public:
	FileSystem(char* name);							//声明系统各个变量 默认信息
	virtual ~FileSystem();

	//初始化信息
	int InitSystem();								//系统初始化
	void OpenFileSystem();							//打开文件系统
	void CreateFileSystem();						//创建文件系统

	int findInodeID(char *name,int flag);			//找文件或目录

	//user区域操作
	void SetUser(User user);						//设置用户	
	void GetUser(User* puser);						//获取之前设置的用户信息

	//SuperBlock区域操作
	void SetSuperBlock(SuperBlock block);			//设置SuperBlock区域信息
	void GetSuperBlock(SuperBlock* pblock);			//获取文件系统的SuperBlock信息

	//BlockBitmap区域操作
	void SetBlockBitmap(unsigned char* bitmap,unsigned int start,unsigned int count);
	void GetBlockBitmap(unsigned char* bitmap);

	//InodeBitmap区域操作
    void SetInodeBitmap(unsigned char* bitmap, unsigned int start, unsigned int count);
	void GetInodeBitmap(unsigned char* bitmap);

	//Inode区域操作
	void GetInode(Inode *pInode, unsigned int id);
	void SetInode(Inode inode);

	void login();									//登录
	void SetLoginTime();							//设置登录时间
	void GetLoginTime();							//获取最后登录时间

	//用户命令
	void clear();									//清空
	void exitSystem();								//退出程序
	void help();									//系统的帮助文档
	void systemInfo();								//查看文件系统信息
	int createFile(char* name,unsigned short isDir);//创建文件或目录
	void ls(char* arg);								//显示list
	void cd(char* name);							//进入新目录
	void pwd();										//显示当前路径
	void passwd();									//修改密码
	void rm(char* name);							//删除文件
	void nano(char* name);							//写入文字到文件
	void cat(char *name);							//读取文件里的文字
	
	void command();									//等待命令
	void Init_SysCommand();							//定义系统命令
	int analysis(string str);						//解析命令
	void showpath();								//显示当前路径

private:
	char system_name[17];							//文件系统名字;限制长度16位
	FILE *fp;										//文件系统文件指针
	string current_path;							//当前系统路径
	SuperBlock SuperBlock_Info;						//文件系统块信息
	User user;										//用户
	Inode current_Inode;							//当前的目录i节点
	
	string SysCommand[COMMAND_NUM];					//系统支持的命令;总个数n个
	char cmd[5][10];								//命令行最多输入5个参数，每个参数最多9个字符

	char LastLoginTime[64];							//最后登录时间
	//由于最后考虑的时间，所以把他的偏移放到了最后，不改动前面的偏移

	unsigned char BlockBitmap[BLOCK_NUM];			//block对照表
	unsigned char InodeBitmap[BLOCK_NUM];			//inode对照表
	//文件系统各个区块大小
	unsigned short UserSize;						//用户区域的大小
	unsigned short SuperBlockSize;					//SuperBlock块区域的大小
	unsigned int BlockBitmapSize;					//block对照表的大小
	unsigned int InodeBitmapSize;					//inode对照表的大小
	unsigned int InodeSize;							//Inode区域大小
	unsigned int BlockSize;							//Block区域大小
	unsigned int LoginSize;							//登录时间的大小

	//各个区块之间距离开头的偏移量
	unsigned long SuperBlockOffset;					//SuperBlock块的偏移量
	unsigned long BlockBitmapOffset;				//block对照表的偏移量
	unsigned long InodeBitmapOffset;				//inode对照表的偏移量
	unsigned long InodeOffset;						//Inode区域的偏移
	unsigned long BlockOffset;						//Block区域的偏移
	unsigned long LoginOffset;						//登录时间偏移
};

